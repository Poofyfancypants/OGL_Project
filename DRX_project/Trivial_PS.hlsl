textureCUBE skybox : register(t0);
sampler filter : register(s0);

struct INPUT_PIXEL
{
	float4 posOut : SV_POSITION;
	float3 uvw : UV;
};

float4 main(INPUT_PIXEL input) : SV_TARGET
{
	return skybox.Sample(filter, input.uvw);
}