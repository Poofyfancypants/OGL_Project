#pragma pack_matrix(row_major)

struct INPUT_VERTEX
{
	float4 posIn : POSITION;
	float3 uvwIn : UVW;
};

struct OUTPUT_VERTEX
{
	float4 posOut : SV_POSITION;
	float3 uvw : UV;
};

cbuffer THIS_IS_VRAM : register( b0 )
{
	float4x4 worldMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
};

OUTPUT_VERTEX main( INPUT_VERTEX fromVertexBuffer )
{

	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;
	
	float4 localH = float4(fromVertexBuffer.posIn.xyz, 1);

	localH = mul(localH, worldMatrix);
	localH = mul(localH, viewMatrix);
	localH = mul(localH, projMatrix);

	sendToRasterizer.posOut = localH;
	sendToRasterizer.uvw = fromVertexBuffer.posIn;

	return sendToRasterizer;
}