
struct PushConstant {
	[[vk::push_constant]] int flip;
};

[[vk::push_constant]] PushConstant pushConstant;

struct VSInput {
	[[vk::location(0)]] float3 position;
	[[vk::location(1)]] float3 normal;
	[[vk::location(2)]] float4 color;
	[[vk::location(3)]] float2 uv;
};

struct VSOutput {
	float4 position : SV_Position;
	[[vk::location(0)]] float2 uv;
};

struct UboData {
	float time;
	float3 padding;
};

[[vk::binding(0, 0)]] ConstantBuffer<UboData> uboData;

[[shader("vertex")]] VSOutput vertexMain(VSInput input)
{
	VSOutput output;

	output.position = float4(input.position, 1.0);
	output.position.x += uboData.time;
	output.position.y *= pushConstant.flip;
	output.uv = input.uv;

	return output;
}

////

struct FSInput {
	[[vk::location(0)]] float2 uv;
};

struct FSOutput {
	[[vk::location(0)]] float4 color;
};

[[vk::binding(1, 0)]] Texture2D testTexture;
[[vk::binding(1, 0)]] SamplerState testSampler;

[[shader("fragment")]] FSOutput fragmentMain(FSInput input)
{
	FSOutput output;
	output.color = testTexture.Sample(testSampler, input.uv);
	return output;
}
