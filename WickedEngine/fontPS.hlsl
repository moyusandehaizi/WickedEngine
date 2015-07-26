Texture2D xTexture:register (t0);
SamplerState Sampler;

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color;

	color = xTexture.Sample(Sampler,PSIn.tex);

	return color;
}