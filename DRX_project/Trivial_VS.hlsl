#pragma pack_matrix(row_major)

struct INPUT_VERTEX
{
	float3 coordinate : POSITION;
};

struct OUTPUT_VERTEX
{
	float4 colorOut : COLOR;
	float4 projectedCoordinate : SV_POSITION;
};

cbuffer THIS_IS_VRAM : register( b0 )
{
	float4 constantColor;
	float2 constantOffset;
	float2 padding;
};

OUTPUT_VERTEX main( INPUT_VERTEX fromVertexBuffer )
{
	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;
	
	sendToRasterizer.projectedCoordinate.xyz = fromVertexBuffer.coordinate.xyz;
	sendToRasterizer.projectedCoordinate.w = 1;
		
	//sendToRasterizer.projectedCoordinate.xy += constantOffset;
	
	sendToRasterizer.colorOut = constantColor;

	return sendToRasterizer;
}