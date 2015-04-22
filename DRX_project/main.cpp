#include <iostream>
#include <ctime>
#include "XTime.h"

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")

#include <DirectXMath.h>

#include "Trivial_VS.csh"
#include "Trivial_PS.csh"
#include "Texture_PS.csh"
#include "Texture_VS.csh"
#include "Model_VS.csh"
#include "Model_PS.csh"
#include "Depth_VS.csh"
#include "DDSTextureLoader.h"

#include "assets\teapot.h"

using namespace std;
using namespace DirectX;

#define BACKBUFFER_WIDTH	1024
#define BACKBUFFER_HEIGHT	768
#define SAFE_RELEASE(p) {if(p){p->Release(); p = NULL;}}

struct Camera
{
	XMMATRIX worldMatrix;
	XMMATRIX viewMatrix;
	XMMATRIX projMatrix;
};

struct CubeVert
{
	XMFLOAT4 position;
	XMFLOAT3 uvw;
};

struct ObjVert
{
	XMFLOAT3 pos;
	XMFLOAT3 uvw;
	XMFLOAT3 nrm;
};

void InputTransforms(float timeStep, Camera &viewFrustum);

class DEMO_APP
{
	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	XTime timer;
	//Setup
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	ID3D11RenderTargetView *renderTarget;

	IDXGISwapChain *swapChain;
	D3D11_VIEWPORT viewport;

	ID3D11Resource * pBB;
	ID3D11Buffer *constantBuffer;

	//Misc
	ID3D11VertexShader *depthShader; //depth kinda old, ready
	ID3D11InputLayout *depthlayout;


	//skybox (cube texture)
	ID3D11Buffer *cubeBuffer;
	ID3D11Buffer *cubeIndexBuffer;

	ID3D11InputLayout *uvlayout; //used skybox
	ID3D11ShaderResourceView *skyBoxSRV;

	ID3D11VertexShader *vertShader;
	ID3D11PixelShader *pixelShader;


	//Ground plane (texture)
	ID3D11Buffer *PlaneBuffer;
	ID3D11Buffer *planeIndexBuffer;

	//uvlayout
	ID3D11ShaderResourceView *planeSRV;

	ID3D11VertexShader *planeVsShader;
	ID3D11PixelShader *planePsShader;

	//Teapot (model)
	ID3D11Buffer *TeaBuffer;
	ID3D11Buffer *TeaIndexBuffer;

	//Generic model setup
	ID3D11InputLayout *modelLayout;
	ID3D11VertexShader *modelVertexShader; //Model
	ID3D11PixelShader *modelPixelShader; //Model

	//Perspective things?
	Camera viewFrustum;
	ID3D11Texture2D *depthStencil = NULL;

	ID3D11DepthStencilView *depthView;
	ID3D11RasterizerState *rastState;
	ID3D11RasterizerState *rastPlaneState;
	ID3D11SamplerState *sampleState;
	ID3D11SamplerState *samplePlaneState;

public:

	XMMATRIX skyPos;
	CubeVert cube[8];

	XMMATRIX planePos;
	CubeVert plane[4];

	//ObjVert Object[8];
	//XMMATRIX newObject;

	_OBJ_VERT_ teaOBJ[1641];

	XMMATRIX tMatrix; //this might be throwing some stuff off

	DEMO_APP(HINSTANCE hinst, WNDPROC proc);
	bool Run();
	bool ShutDown();
};

DEMO_APP::DEMO_APP(HINSTANCE hinst, WNDPROC proc)
{
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

	deviceFlag = D3D11_CREATE_DEVICE_DEBUG;

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

	//Shader Creation
	HRESULT Vs = device->CreateVertexShader(Trivial_VS, sizeof(Trivial_VS), NULL, &vertShader); //For skybox
	HRESULT Ps = device->CreatePixelShader(Trivial_PS, sizeof(Trivial_PS), NULL, &pixelShader); //For skybox
	HRESULT Ds = device->CreateVertexShader(Depth_VS, sizeof(Depth_VS), NULL, &depthShader); //Normal depth
	HRESULT mVs = device->CreateVertexShader(Model_VS, sizeof(Model_VS), NULL, &modelVertexShader); //For skybox
	HRESULT mPs = device->CreatePixelShader(Model_PS, sizeof(Model_PS), NULL, &modelPixelShader); //For skybox
	HRESULT pPs = device->CreatePixelShader(Texture_PS, sizeof(Texture_PS), NULL, &planePsShader); //For plane
	HRESULT pVs = device->CreateVertexShader(Texture_VS, sizeof(Texture_VS), NULL, &planeVsShader); //For skybox

	D3D11_RASTERIZER_DESC rast_desc; //sky
	ZeroMemory(&rast_desc, sizeof(rast_desc));
	rast_desc.AntialiasedLineEnable = true;
	rast_desc.CullMode = D3D11_CULL_FRONT;
	rast_desc.FillMode = D3D11_FILL_SOLID;
	rast_desc.FrontCounterClockwise = false;
	rast_desc.DepthBias = 0;
	rast_desc.SlopeScaledDepthBias = 0.0f;
	rast_desc.DepthBiasClamp = 0.0f;
	rast_desc.DepthClipEnable = true;
	rast_desc.ScissorEnable = false;
	rast_desc.MultisampleEnable = false;
	HRESULT r = device->CreateRasterizerState(&rast_desc, &rastState);

	D3D11_RASTERIZER_DESC rast_plane_desc; //plane
	ZeroMemory(&rast_desc, sizeof(rast_desc));
	rast_plane_desc.AntialiasedLineEnable = true;
	rast_plane_desc.CullMode = D3D11_CULL_BACK;
	rast_plane_desc.FillMode = D3D11_FILL_SOLID;
	rast_plane_desc.FrontCounterClockwise = false;
	rast_plane_desc.DepthBias = 0;
	rast_plane_desc.SlopeScaledDepthBias = 0.0f;
	rast_plane_desc.DepthBiasClamp = 0.0f;
	rast_plane_desc.DepthClipEnable = true;
	rast_plane_desc.ScissorEnable = false;
	rast_plane_desc.MultisampleEnable = false;
	HRESULT rP = device->CreateRasterizerState(&rast_plane_desc, &rastPlaneState);

	D3D11_SAMPLER_DESC sample_desc; //sky
	ZeroMemory(&sample_desc, sizeof(sample_desc));
	sample_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sample_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sample_desc.MinLOD = -FLT_MAX;
	sample_desc.MaxLOD = FLT_MAX;
	sample_desc.MaxAnisotropy = 1;
	sample_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	HRESULT s = device->CreateSamplerState(&sample_desc, &sampleState);

	D3D11_SAMPLER_DESC sample_plane_desc; //sky
	ZeroMemory(&sample_plane_desc, sizeof(sample_plane_desc));
	sample_plane_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sample_plane_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_plane_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_plane_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sample_plane_desc.MinLOD = -FLT_MAX;
	sample_plane_desc.MaxLOD = FLT_MAX;
	sample_plane_desc.MaxAnisotropy = 1;
	sample_plane_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	HRESULT sP = device->CreateSamplerState(&sample_plane_desc, &samplePlaneState);

	HRESULT sSr = CreateDDSTextureFromFile(device, L"../DRX_project/assets/Skybox/Skybox.dds", NULL, &skyBoxSRV);
	HRESULT pSr = CreateDDSTextureFromFile(device, L"../DRX_project/assets/swampfloor_seamless.dds", NULL, &planeSRV);
	//HRESULT oSr = CreateDDSTextureFromFile(device, L"", NULL, &objectSRV); 

	D3D11_INPUT_ELEMENT_DESC uvLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UVW", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_INPUT_ELEMENT_DESC depthLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC modeLLayout[] =
	{
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UVW", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NRM", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	skyPos = XMMatrixIdentity();
	planePos = XMMatrixIdentity();

	cube[0].position.x = -0.5;
	cube[0].position.y = -0.5;
	cube[0].position.z = -0.5;
	cube[0].position.w = 1.0;
	cube[1].position.x = 0.5;
	cube[1].position.y = -0.5;
	cube[1].position.z = -0.5;
	cube[1].position.w = 1;
	cube[2].position.x = -0.5;
	cube[2].position.y = 0.5;
	cube[2].position.z = -0.5;
	cube[2].position.w = 1;
	cube[3].position.x = 0.5;
	cube[3].position.y = 0.5;
	cube[3].position.z = -0.5;
	cube[3].position.w = 1;
	cube[4].position.x = -0.5;
	cube[4].position.y = -0.5;
	cube[4].position.z = 0.5;
	cube[4].position.w = 1;
	cube[5].position.x = 0.5;
	cube[5].position.y = -0.5;
	cube[5].position.z = 0.5;
	cube[5].position.w = 1;
	cube[6].position.x = -0.5;
	cube[6].position.y = 0.5;
	cube[6].position.z = 0.5;
	cube[6].position.w = 1;
	cube[7].position.x = 0.5;
	cube[7].position.y = 0.5;
	cube[7].position.z = 0.5;
	cube[7].position.w = 1;

	plane[0].position.x = -10;
	plane[0].position.y = -1;
	plane[0].position.z = -10;
	plane[0].position.w = 1.0;
	plane[0].uvw.x = 0;
	plane[0].uvw.y = 1;

	plane[1].position.x = -10;
	plane[1].position.y = -1;
	plane[1].position.z = 10;
	plane[1].position.w = 1.0;
	plane[1].uvw.x = 0;
	plane[1].uvw.y = 0;

	plane[2].position.x = 10;
	plane[2].position.y = -1;
	plane[2].position.z = -10;
	plane[2].position.w = 1.0;
	plane[2].uvw.x = 1;
	plane[2].uvw.y = 1;

	plane[3].position.x = 10;
	plane[3].position.y = -1;
	plane[3].position.z = 10;
	plane[3].position.w = 1.0;
	plane[3].uvw.x = 0;
	plane[3].uvw.y = 1;

	D3D11_BUFFER_DESC cube_desc;
	ZeroMemory(&cube_desc, sizeof(cube_desc));
	cube_desc.Usage = D3D11_USAGE_IMMUTABLE;
	cube_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cube_desc.ByteWidth = sizeof(CubeVert) * 8;
	cube_desc.CPUAccessFlags = NULL;

	D3D11_SUBRESOURCE_DATA cube_data;
	ZeroMemory(&cube_data, sizeof(cube_data));
	cube_data.pSysMem = cube;
	HRESULT l = device->CreateBuffer(&cube_desc, &cube_data, &cubeBuffer);

	UINT32 cubeIndices[] = { 0, 2, 1, 1, 2, 3, 1, 3, 7, 7, 5, 1, 2, 0, 4, 4, 6, 2, 4, 5, 6, 5, 7, 6, 2, 7, 3, 6, 7, 2, 0, 1, 5, 5, 4, 0 };

	D3D11_BUFFER_DESC cube_index_desc;
	ZeroMemory(&cube_index_desc, sizeof(cube_index_desc));
	cube_index_desc.Usage = D3D11_USAGE_IMMUTABLE;
	cube_index_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	cube_index_desc.CPUAccessFlags = NULL;
	cube_index_desc.ByteWidth = sizeof(UINT32) * 36;

	D3D11_SUBRESOURCE_DATA cube_index_data;
	ZeroMemory(&cube_index_data, sizeof(cube_index_data));
	cube_index_data.pSysMem = cubeIndices;
	HRESULT f = device->CreateBuffer(&cube_index_desc, &cube_index_data, &cubeIndexBuffer);


	D3D11_BUFFER_DESC plane_desc;
	ZeroMemory(&plane_desc, sizeof(plane_desc));
	plane_desc.Usage = D3D11_USAGE_IMMUTABLE;
	plane_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	plane_desc.ByteWidth = sizeof(CubeVert) * 4;
	plane_desc.CPUAccessFlags = NULL;

	D3D11_SUBRESOURCE_DATA plane_data;
	ZeroMemory(&plane_data, sizeof(plane_data));
	plane_data.pSysMem = plane;
	HRESULT pl = device->CreateBuffer(&plane_desc, &plane_data, &PlaneBuffer);

	D3D11_BUFFER_DESC plane_index_desc;
	ZeroMemory(&plane_index_desc, sizeof(plane_index_desc));
	plane_index_desc.Usage = D3D11_USAGE_IMMUTABLE;
	plane_index_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	plane_index_desc.CPUAccessFlags = NULL;
	plane_index_desc.ByteWidth = sizeof(UINT32) * 6;

	UINT32 planeIndices[] = { 0, 1, 2, 1, 3, 2 };

	D3D11_SUBRESOURCE_DATA plane_index_data;
	ZeroMemory(&plane_index_data, sizeof(plane_index_data));
	plane_index_data.pSysMem = planeIndices;
	HRESULT pI = device->CreateBuffer(&plane_index_desc, &plane_index_data, &planeIndexBuffer);

	//newObject = XMMatrixIdentity();
	/*Object[0].pos.x = -0.5;
	Object[0].pos.y = -0.5;
	Object[0].pos.z = -0.5;
	Object[0].pos.w = 1.0;
	Object[1].pos.x = 0.5;
	Object[1].pos.y = -0.5;
	Object[1].pos.z = -0.5;
	Object[1].pos.w = 1;
	Object[2].pos.x = -0.5;
	Object[2].pos.y = 0.5;
	Object[2].pos.z = -0.5;
	Object[2].pos.w = 1;
	Object[3].pos.x = 0.5;
	Object[3].pos.y = 0.5;
	Object[3].pos.z = -0.5;
	Object[3].pos.w = 1;
	Object[4].pos.x = -0.5;
	Object[4].pos.y = -0.5;
	Object[4].pos.z = 0.5;
	Object[4].pos.w = 1;
	Object[5].pos.x = 0.5;
	Object[5].pos.y = -0.5;
	Object[5].pos.z = 0.5;
	Object[5].pos.w = 1;
	Object[6].pos.x = -0.5;
	Object[6].pos.y = 0.5;
	Object[6].pos.z = 0.5;
	Object[6].pos.w = 1;
	Object[7].pos.x = 0.5;
	Object[7].pos.y = 0.5;
	Object[7].pos.z = 0.5;
	Object[7].pos.w = 1;*/

	D3D11_BUFFER_DESC tea_desc;
	ZeroMemory(&tea_desc, sizeof(tea_desc));
	tea_desc.Usage = D3D11_USAGE_IMMUTABLE;
	tea_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	tea_desc.ByteWidth = sizeof(OBJ_VERT) * 1641;
	tea_desc.CPUAccessFlags = NULL;

	D3D11_SUBRESOURCE_DATA tea_data;
	ZeroMemory(&tea_data, sizeof(tea_data));
	tea_data.pSysMem = teapot_data;
	HRESULT o = device->CreateBuffer(&tea_desc, &tea_data, &TeaBuffer);

	//UINT32 objIndices[] = { 0, 2, 1, 1, 2, 3, 1, 3, 7, 7, 5, 1, 2, 0, 4, 4, 6, 2, 4, 5, 6, 5, 7, 6, 2, 7, 3, 6, 7, 2, 0, 1, 5, 5, 4, 0 };

	D3D11_BUFFER_DESC tea_index_desc;
	ZeroMemory(&tea_index_desc, sizeof(tea_index_desc));
	tea_index_desc.Usage = D3D11_USAGE_IMMUTABLE;
	tea_index_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	tea_index_desc.CPUAccessFlags = NULL;
	tea_index_desc.ByteWidth = sizeof(UINT32) * 4632;

	D3D11_SUBRESOURCE_DATA tea_index_data;
	ZeroMemory(&tea_index_data, sizeof(tea_index_data));
	tea_index_data.pSysMem = teapot_indicies;
	HRESULT b = device->CreateBuffer(&tea_index_desc, &tea_index_data, &TeaIndexBuffer);

	HRESULT UVL = device->CreateInputLayout(uvLayout, 2, Trivial_VS, sizeof(Trivial_VS), &uvlayout);
	HRESULT IDL = device->CreateInputLayout(depthLayout, 2, Depth_VS, sizeof(Depth_VS), &depthlayout);
	HRESULT MDL = device->CreateInputLayout(modeLLayout, 3, Model_VS, sizeof(Model_VS), &modelLayout);

	viewFrustum.worldMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	viewFrustum.viewMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -10, 1 };

	float yScale = (1 / (tan(0.5f*1.57f)));
	float xScale = (yScale * ((float)BACKBUFFER_WIDTH / (float)BACKBUFFER_HEIGHT));

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

	viewFrustum.projMatrix= XMMatrixPerspectiveFovLH(XMConvertToRadians(75), ((float)BACKBUFFER_WIDTH / (float)BACKBUFFER_HEIGHT), 0.1f, 100.0f);

	//viewFrustum.viewMatrix = XMMatrixInverse(0, viewFrustum.viewMatrix);
}

bool DEMO_APP::Run()
{
	timer.Signal();
	float timeStep = timer.Delta();
	D3D11_MAPPED_SUBRESOURCE mapped;
	float buffer[4] = { 0.0, 0.0, 1.0, 1.0 };

	context->OMSetRenderTargets(1, &renderTarget, depthView);
	context->RSSetViewports(1, &viewport);
	context->ClearRenderTargetView(renderTarget, buffer);
	context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1, 0);

	context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	XMMATRIX invView = XMMatrixInverse(NULL, viewFrustum.viewMatrix);

	((Camera*)mapped.pData)->worldMatrix = skyPos;
	((Camera*)mapped.pData)->viewMatrix = invView;
	((Camera*)mapped.pData)->projMatrix = viewFrustum.projMatrix;

	context->Unmap(constantBuffer, 0);
	context->VSSetConstantBuffers(0, 1, &constantBuffer);

	//Skybox
	unsigned int aef = 0;
	unsigned int adw = sizeof(CubeVert);
	context->RSSetState(rastState);
	context->PSSetShaderResources(0, 1, &skyBoxSRV);
	context->PSSetSamplers(0, 1, &sampleState);
	context->IASetVertexBuffers(0, 1, &cubeBuffer, &adw, &aef);
	context->IASetIndexBuffer(cubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->VSSetShader(vertShader, 0, 0);
	context->PSSetShader(pixelShader, 0, 0);
	context->IASetInputLayout(uvlayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->DrawIndexed(36, 0, 0);

	context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1, 0);

	context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	((Camera*)mapped.pData)->worldMatrix = XMMatrixIdentity();
	((Camera*)mapped.pData)->viewMatrix = invView;
	((Camera*)mapped.pData)->projMatrix = viewFrustum.projMatrix;

	context->Unmap(constantBuffer, 0);

	context->VSSetConstantBuffers(0, 1, &constantBuffer);
	//Draw Plane (similar to skybox)
	context->RSSetState(rastPlaneState);
	context->PSSetShaderResources(0, 1, &planeSRV);
	context->PSSetSamplers(0, 1, &samplePlaneState);
	context->IASetVertexBuffers(0, 1, &PlaneBuffer, &adw, &aef);
	context->IASetIndexBuffer(planeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->VSSetShader(planeVsShader, 0, 0);
	context->PSSetShader(planePsShader, 0, 0);
	context->IASetInputLayout(uvlayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->DrawIndexed(6, 0, 0);

	//Input
	InputTransforms(timeStep, viewFrustum);
	skyPos.r[3] = viewFrustum.viewMatrix.r[3];
	planePos.r[3] = XMVectorSet(0, 0, 0, 1);

	context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	((Camera*)mapped.pData)->worldMatrix = XMMatrixIdentity();
	((Camera*)mapped.pData)->viewMatrix = XMMatrixInverse(NULL, viewFrustum.viewMatrix);
	((Camera*)mapped.pData)->projMatrix = viewFrustum.projMatrix;

	context->Unmap(constantBuffer, 0);
	context->VSSetConstantBuffers(0, 1, &constantBuffer);

	//Draw Objects
	//Teapot
	unsigned int obj = sizeof(_OBJ_VERT_);
	context->IASetVertexBuffers(0, 1, &TeaBuffer, &obj, &aef);
	context->IASetIndexBuffer(TeaIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->VSSetShader(modelVertexShader, 0, 0);
	context->PSSetShader(modelPixelShader, 0, 0);
	context->IASetInputLayout(modelLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->DrawIndexed(4632, 0, 0);

	swapChain->Present(0, 0);

	return true;
}

void InputTransforms(float timeStep, Camera &viewFrustum)
{
	if (GetAsyncKeyState('W') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(0, 0, 5 * timeStep);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(-5 * timeStep, 0, 0);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(0, 0, -5 * timeStep);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}
	if (GetAsyncKeyState('E') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(5 * timeStep, 0, 0);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}
	if (GetAsyncKeyState('T') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(0, -5 * timeStep, 0);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}
	if (GetAsyncKeyState('Y') & 0x8000)
	{
		XMMATRIX translation = XMMatrixTranslation(0, 5 * timeStep, 0);
		viewFrustum.viewMatrix = XMMatrixMultiply(translation, viewFrustum.viewMatrix);
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		XMMATRIX rotateTemp = XMMatrixIdentity();
		rotateTemp.r[3] = viewFrustum.viewMatrix.r[3];
		viewFrustum.viewMatrix.r[3] = g_XMZero;
		viewFrustum.viewMatrix = XMMatrixMultiply(viewFrustum.viewMatrix, XMMatrixRotationY(timeStep));
		viewFrustum.viewMatrix.r[3] = rotateTemp.r[3];
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		XMMATRIX rotateTemp = XMMatrixIdentity();
		rotateTemp.r[3] = viewFrustum.viewMatrix.r[3];
		viewFrustum.viewMatrix.r[3] = g_XMZero;
		viewFrustum.viewMatrix = XMMatrixMultiply(viewFrustum.viewMatrix, XMMatrixRotationY(-timeStep));
		viewFrustum.viewMatrix.r[3] = rotateTemp.r[3];
	}

	if (GetAsyncKeyState('Z') & 0x8000)
	{
		XMMATRIX rotateTemp = XMMatrixIdentity();
		rotateTemp.r[3] = viewFrustum.viewMatrix.r[3];
		viewFrustum.viewMatrix.r[3] = g_XMZero;
		viewFrustum.viewMatrix = XMMatrixMultiply(viewFrustum.viewMatrix, XMMatrixRotationX(timeStep));
		viewFrustum.viewMatrix.r[3] = rotateTemp.r[3];
	}
	if (GetAsyncKeyState('C') & 0x8000)
	{
		XMMATRIX rotateTemp = XMMatrixIdentity();
		rotateTemp.r[3] = viewFrustum.viewMatrix.r[3];
		viewFrustum.viewMatrix.r[3] = g_XMZero;
		viewFrustum.viewMatrix = XMMatrixMultiply(viewFrustum.viewMatrix, XMMatrixRotationX(-timeStep));
		viewFrustum.viewMatrix.r[3] = rotateTemp.r[3];
	}
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
	SAFE_RELEASE(uvlayout);
	SAFE_RELEASE(depthlayout);
	SAFE_RELEASE(depthView);
	SAFE_RELEASE(depthStencil);
	SAFE_RELEASE(rastState);

	SAFE_RELEASE(depthShader);
	SAFE_RELEASE(vertShader);
	SAFE_RELEASE(pixelShader);
	SAFE_RELEASE(modelVertexShader);
	SAFE_RELEASE(modelPixelShader);

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