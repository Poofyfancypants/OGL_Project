#include <iostream>
#include <ctime>
#include "XTime.h"

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")

#include <DirectXMath.h>

#include "Trivial_VS.csh"
#include "Trivial_PS.csh"
#include "Depth_VS.csh"

using namespace std;
using namespace DirectX;

#define BACKBUFFER_WIDTH	500
#define BACKBUFFER_HEIGHT	500
#define SAFE_RELEASE(p) {if(p){p->Release(); p = NULL;}}

struct SEND_TO_VRAM
{
	XMFLOAT4 constColor;
	XMFLOAT2 constantOffset;
	XMFLOAT2 padding;
};

struct Camera
{
	XMMATRIX worldMatrix;
	XMMATRIX viewMatrix;
	XMMATRIX projMatrix;
};

class DEMO_APP
{
	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	XTime timer;

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	ID3D11RenderTargetView *renderTarget;

	IDXGISwapChain *swapChain;
	D3D11_VIEWPORT viewport;

	ID3D11Resource * pBB;

	ID3D11InputLayout *layout;
	ID3D11InputLayout *depthlayout;

	Camera viewFrustum;

	ID3D11VertexShader *vertShader;
	ID3D11VertexShader *depthShader;
	ID3D11PixelShader *pixelShader;

	ID3D11Buffer *constantBuffer;

	ID3D11Buffer *indexBuffer1;
	ID3D11Buffer *vertBuffer1;

	ID3D11Texture2D *depthStencil = NULL;

	ID3D11DepthStencilView *depthView;
	ID3D11RasterizerState *rastState;

public:



	XMMATRIX tMatrix;

	DEMO_APP(HINSTANCE hinst, WNDPROC proc);
	bool Run();
	bool ShutDown();
};

//************************************************************
//************ CREATION OF OBJECTS & RESOURCES ***************
//************************************************************

DEMO_APP::DEMO_APP(HINSTANCE hinst, WNDPROC proc)
{
	// ****************** BEGIN WARNING ***********************//
	// WINDOWS CODE, I DON'T TEACH THIS YOU MUST KNOW IT ALREADY!
	application = hinst;
	appWndProc = proc;

	WNDCLASSEX  wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = appWndProc;
	wndClass.lpszClassName = L"DirectXApplication";
	wndClass.hInstance = application;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	//wndClass.hIcon			= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FSICON));
	RegisterClassEx(&wndClass);

	RECT window_size = { 0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT };
	AdjustWindowRect(&window_size, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindow(L"DirectXApplication", L"Lab 1a Line Land", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, window_size.right - window_size.left, window_size.bottom - window_size.top,
		NULL, NULL, application, this);

	ShowWindow(window, SW_SHOW);
	//********************* END WARNING ************************//

	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapDesc.BufferDesc.Width = BACKBUFFER_WIDTH;
	swapDesc.BufferDesc.Height = BACKBUFFER_HEIGHT;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapDesc.SampleDesc.Count = 8;
	swapDesc.SampleDesc.Quality = 0;

	swapDesc.BufferCount = 1;

	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = window;
	swapDesc.Windowed = true;
	swapDesc.Flags = NULL;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	D3D_FEATURE_LEVEL featureLevel = featureLevels[0];

	D3D11_CREATE_DEVICE_FLAG deviceFlag;

	if (_DEBUG)
		deviceFlag = D3D11_CREATE_DEVICE_DEBUG;
	else
		deviceFlag = D3D11_CREATE_DEVICE_SINGLETHREADED;

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		deviceFlag,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&swapDesc,
		&swapChain,
		&device,
		&featureLevel,
		&context
		);

	swapChain->GetBuffer(0, __uuidof(pBB), reinterpret_cast<void**>(&pBB));
	device->CreateRenderTargetView(pBB, NULL, &renderTarget);

	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = BACKBUFFER_WIDTH;
	viewport.Height = BACKBUFFER_HEIGHT;

	D3D11_BUFFER_DESC constbuff_desc;
	ZeroMemory(&constbuff_desc, sizeof(constbuff_desc));
	constbuff_desc.Usage = D3D11_USAGE_DYNAMIC;
	constbuff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constbuff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constbuff_desc.ByteWidth = sizeof(Camera);

	D3D11_TEXTURE2D_DESC depth_desc;

	depth_desc.Width = BACKBUFFER_WIDTH;
	depth_desc.Height = BACKBUFFER_HEIGHT;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_D32_FLOAT;
	depth_desc.SampleDesc.Count = 8;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_desc.CPUAccessFlags = 0;
	depth_desc.MiscFlags = 0;
	HRESULT h = device->CreateTexture2D(&depth_desc, NULL, &depthStencil);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthS_desc;
	ZeroMemory(&depthS_desc, sizeof(depthS_desc));
	depthS_desc.Format = DXGI_FORMAT_D32_FLOAT;
	depthS_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	depthS_desc.Texture2D.MipSlice = 0;
	HRESULT d = device->CreateDepthStencilView(depthStencil, &depthS_desc, &depthView);

	D3D11_SUBRESOURCE_DATA bufferData;
	ZeroMemory(&bufferData, sizeof(bufferData));
	bufferData.pSysMem = &viewFrustum;
	HRESULT c = device->CreateBuffer(&constbuff_desc, &bufferData, &constantBuffer);

	HRESULT Vs = device->CreateVertexShader(Trivial_VS, sizeof(Trivial_VS), NULL, &vertShader);
	HRESULT Ds = device->CreateVertexShader(Depth_VS, sizeof(Depth_VS), NULL, &depthShader);
	HRESULT Ps = device->CreatePixelShader(Trivial_PS, sizeof(Trivial_PS), NULL, &pixelShader);

	D3D11_INPUT_ELEMENT_DESC vLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC depthLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HRESULT IL = device->CreateInputLayout(vLayout, 2, Trivial_VS, sizeof(Trivial_VS), &layout);
	HRESULT IDL = device->CreateInputLayout(depthLayout, 2, Depth_VS, sizeof(Depth_VS), &depthlayout);

	viewFrustum.worldMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	viewFrustum.viewMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -20, 1 };

	float yScale = (1 / (tan(0.5f*1.57f)));
	float xScale = (yScale * (BACKBUFFER_WIDTH / BACKBUFFER_HEIGHT));

	viewFrustum.projMatrix.r[0].m128_f32[0] = xScale;
	viewFrustum.projMatrix.r[0].m128_f32[1] = 0;
	viewFrustum.projMatrix.r[0].m128_f32[2] = 0;
	viewFrustum.projMatrix.r[0].m128_f32[3] = 0;
	viewFrustum.projMatrix.r[1].m128_f32[0] = 0;
	viewFrustum.projMatrix.r[1].m128_f32[1] = yScale;
	viewFrustum.projMatrix.r[1].m128_f32[2] = 0;
	viewFrustum.projMatrix.r[1].m128_f32[3] = 0;
	viewFrustum.projMatrix.r[2].m128_f32[0] = 0;
	viewFrustum.projMatrix.r[2].m128_f32[1] = 0;
	viewFrustum.projMatrix.r[2].m128_f32[2] = (100.0f / (100.0f - 0.1f));
	viewFrustum.projMatrix.r[2].m128_f32[3] = 1;
	viewFrustum.projMatrix.r[3].m128_f32[0] = 0;
	viewFrustum.projMatrix.r[3].m128_f32[1] = 0;
	viewFrustum.projMatrix.r[3].m128_f32[2] = (-(100.0f*0.1f) / (100.0f - .1f));
	viewFrustum.projMatrix.r[3].m128_f32[3] = 0;

	viewFrustum.viewMatrix = XMMatrixInverse(0, viewFrustum.viewMatrix); //DETERMINENT is most likely WRONG

}

bool DEMO_APP::Run()
{
	timer.Signal();
	float buffer[4] = { 0.0, 0.0, 1.0, 1.0 };

	context->OMSetRenderTargets(1, &renderTarget, depthView);
	context->RSSetViewports(1, &viewport);
	context->ClearRenderTargetView(renderTarget, buffer);
	context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1, 0);

	D3D11_MAPPED_SUBRESOURCE mapped;

	tMatrix = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	if (GetAsyncKeyState('W') & 0x8000)
		tMatrix.r[3].m128_f32[2] = -5.0f * timer.Delta();
	if (GetAsyncKeyState('A') & 0x8000)
		tMatrix.r[3].m128_f32[2] = 5.0f * timer.Delta();
	if (GetAsyncKeyState('S') & 0x8000)
		tMatrix.r[3].m128_f32[2] = 5.0f * timer.Delta();
	if (GetAsyncKeyState('D') & 0x8000)
		tMatrix.r[3].m128_f32[2] = -5.0f * timer.Delta();
	if (GetAsyncKeyState('Q') & 0x8000)
		tMatrix.r[3].m128_f32[2] = -5.0f * timer.Delta();
	if (GetAsyncKeyState('E') & 0x8000)
		tMatrix.r[3].m128_f32[2] = 5.0f * timer.Delta();

	viewFrustum.viewMatrix = XMMatrixMultiply(tMatrix, viewFrustum.viewMatrix);

	context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	((Camera*)mapped.pData)->worldMatrix = viewFrustum.worldMatrix;
	((Camera*)mapped.pData)->viewMatrix = viewFrustum.viewMatrix;
	((Camera*)mapped.pData)->projMatrix = viewFrustum.projMatrix;

	context->Unmap(constantBuffer, 0);
	context->VSSetConstantBuffers(0, 1, &constantBuffer);

	swapChain->Present(0, 0);

	return true;
}

//************************************************************
//************ DESTRUCTION ***********************************
//************************************************************

bool DEMO_APP::ShutDown()
{
	// TODO: PART 1 STEP 6

	SAFE_RELEASE(device);
	SAFE_RELEASE(context);
	SAFE_RELEASE(renderTarget);
	SAFE_RELEASE(swapChain);

	SAFE_RELEASE(pBB);
	SAFE_RELEASE(layout);
	SAFE_RELEASE(depthlayout);
	SAFE_RELEASE(depthView);
	SAFE_RELEASE(depthStencil);
	SAFE_RELEASE(rastState);

	SAFE_RELEASE(depthShader);
	SAFE_RELEASE(vertShader);
	SAFE_RELEASE(pixelShader);


	UnregisterClass(L"DirectXApplication", application);
	return true;
}

//************************************************************
//************ WINDOWS RELATED *******************************
//************************************************************

// ****************** BEGIN WARNING ***********************// 
// WINDOWS CODE, I DON'T TEACH THIS YOU MUST KNOW IT ALREADY!

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
	srand(unsigned int(time(0)));
	DEMO_APP myApp(hInstance, (WNDPROC)WndProc);
	MSG msg; ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && myApp.Run())
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_ESCAPE))
		message = WM_DESTROY;
	switch (message)
	{
	case (WM_DESTROY) : { PostQuitMessage(0); }
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
//********************* END WARNING ************************//