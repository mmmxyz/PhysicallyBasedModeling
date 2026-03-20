
struct LightPersMatrixData {
	float4x4 persMat;
	float4x4 lightSrtMat;
};

struct SrtMatrixUboData {
	float4x4 srtMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<LightPersMatrixData> lightPersMatrix;
[[vk::binding(0, 1)]] ConstantBuffer<SrtMatrixUboData> srtMatrixUboData;


struct VSInput {
	[[vk::location(0)]] float3 position;
	[[vk::location(1)]] float3 normal;
	[[vk::location(2)]] float4 color;
	[[vk::location(3)]] float2 uv;
	[[vk::location(4)]] float4 tangent;
	[[vk::location(5)]] float roughness;
};

struct VSOutput {
	float4 position : SV_Position;
};

[[shader("vertex")]] VSOutput vertexMain(VSInput input)
{
	VSOutput output;

	float3 pos = input.position;
	pos = mul(srtMatrixUboData.srtMatrix, float4(pos, 1.0)).xyz;

	pos = mul(lightPersMatrix.lightSrtMat, float4(pos, 1.0)).xyz;

	float4 pos4 = float4(pos, 1.0);
	pos4	    = mul(lightPersMatrix.persMat, pos4);

	output.position = pos4;

	return output;
}
