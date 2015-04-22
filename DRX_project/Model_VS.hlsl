#pragma pack_matrix(row_major)
//Use for some object, get something showing

struct INPUT_VERTEX
{
	float3 coordinate : POS;
	float3 uvw : UVW;
	float3 nrm : NRM;
};

struct OUTPUT_VERTEX
{
	float4 colorOut : COLOR;
	float4 projectedCoordinate : SV_POSITION;
};

cbuffer THIS_IS_VRAM : register(b0)
{
	float4x4 worldMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
};

OUTPUT_VERTEX main(INPUT_VERTEX fromVertexBuffer)
{
	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;
	float4 localH = float4(fromVertexBuffer.coordinate,1);

	sendToRasterizer.colorOut = float4(1, 1, 0, 1);

	localH = mul(localH, worldMatrix);
	localH = mul(localH, viewMatrix);
	localH = mul(localH, projMatrix);

	sendToRasterizer.projectedCoordinate = localH;

	return sendToRasterizer;
}