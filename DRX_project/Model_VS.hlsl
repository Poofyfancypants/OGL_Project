#pragma pack_matrix(row_major)
//Use for some object, get something showing

struct INPUT_VERTEX
{
	float4 coordinate : POS;
	float3 uvw : UVW;
	float3 nrm : NRM;
};

struct OUTPUT_VERTEX
{
	float4 posOut : SV_POSITION;
	float4 posW : POSITION;
	float3 uvwOut : UV;
	float3 nrmOut : NRM;
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
	float4 localH = float4(fromVertexBuffer.coordinate.xyz,1);

	localH = mul(localH, worldMatrix);
	sendToRasterizer.posW = localH;
	localH = mul(localH, viewMatrix);
	localH = mul(localH, projMatrix);

	sendToRasterizer.posOut = localH;
	sendToRasterizer.nrmOut = fromVertexBuffer.nrm;

	return sendToRasterizer;
}