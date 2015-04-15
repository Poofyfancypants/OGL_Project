#pragma pack_matrix(row_major)

struct INPUT_VERTEX
{
	float4 coordinate : POSITION;
	float4 color : COLOR;
};

struct OUTPUT_VERTEX
{
	float4 colorOut : COLOR;
	float4 projectedCoordinate : SV_POSITION;
};

// TODO: PART 3 STEP 2a
cbuffer THIS_IS_VRAM : register( b0 )
{
	float3 pos;
	float3 uvw;
	float3 nrm;
};

OUTPUT_VERTEX main( INPUT_VERTEX fromVertexBuffer )
{
	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;
	float4 localH = float4(fromVertexBuffer.coordinate);

	sendToRasterizer.colorOut = fromVertexBuffer.color;

	sendToRasterizer.projectedCoordinate = localH;

	return sendToRasterizer;
}