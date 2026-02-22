
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
	[[vk::location(0)]] float3 worldPos;
	[[vk::location(1)]] float2 uv;
	[[vk::location(2)]] float3 normal;
	[[vk::location(3)]] float4 color;
	[[vk::location(4)]] float4 shadowUv;
	[[vk::location(5)]] float3 inverseZinLight;
};

struct PersMatrixData {
	float4x4 cameraMat;
	float4x4 persMat;
};

struct LightData {
	float3 lightPos;
	float3 color;
	float4x4 lightPersMat;
	float4x4 lightSrtMat;
};

struct UboData {
	float time;
	float3 padding;
};

struct SrtMatrixUboData {
	float4x4 srtMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<PersMatrixData> persMatrix;
[[vk::binding(1, 0)]] ConstantBuffer<LightData> lightData;
[[vk::binding(2, 0)]] Texture2D testShadowTexture;
[[vk::binding(2, 0)]] SamplerState testShadowSampler;

[[vk::binding(0, 1)]] ConstantBuffer<UboData> uboData;
[[vk::binding(1, 1)]] ConstantBuffer<SrtMatrixUboData> srtMatrixUboData;

float3x3 outerProdut(float3 a, float3 b)
{
	return float3x3(a.x * b.x, a.x * b.y, a.x * b.z,
	    a.y * b.x, a.y * b.y, a.y * b.z,
	    a.z * b.x, a.z * b.y, a.z * b.z);
}

float3x3 vectorToRotation(float3 vec)
{
	float3x3 rotMatrix;

	float theta = length(vec);

	if (theta < 0.0001) {
		return float3x3(1.0, 0.0, 0.0,
		    0.0, 1.0, 0.0,
		    0.0, 0.0, 1.0);
	}

	float3 nvec = normalize(vec);

	rotMatrix = cos(theta) * float3x3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)
	    + (1 - cos(theta)) * outerProdut(nvec, nvec)
	    + sin(theta) * float3x3(0.0, -nvec.z, nvec.y, nvec.z, 0.0, -nvec.x, -nvec.y, nvec.x, 0.0);

	return rotMatrix;
}

[[shader("vertex")]] VSOutput vertexMain(VSInput input)
{
	VSOutput output;

	float3 pos = input.position;

	pos = mul(srtMatrixUboData.srtMatrix, float4(pos, 1.0)).xyz;

	float3 worldPos = pos;
	output.worldPos = worldPos;

	output.normal = mul(srtMatrixUboData.srtMatrix, float4(input.normal, 0.0)).xyz;

	pos = mul(persMatrix.cameraMat, float4(pos, 1.0)).xyz;


	float4 pos4 = float4(pos, 1.0);
	pos4	    = mul(persMatrix.persMat, pos4);

	output.position = pos4;

	output.uv = input.uv;


	output.color = input.color;
	output.color.a = 1.0;


	float3 shadowMapPos = mul(lightData.lightSrtMat, float4(worldPos, 1.0)).xyz;
	float4 shadowMapPos4 = mul(lightData.lightPersMat, float4(shadowMapPos, 1.0));
	output.inverseZinLight.xy = shadowMapPos4.zw;
	output.shadowUv = shadowMapPos4;

	return output;
}

struct FSInput {
	float4 position : SV_Position;
	[[vk::location(0)]] float3 worldPos;
	[[vk::location(1)]] float2 uv;
	[[vk::location(2)]] float3 normal;
	[[vk::location(3)]] float4 color;
	[[vk::location(4)]] float4 shadowUv;
	[[vk::location(5)]] float3 inverseZinLight;
};

struct FSOutput {
	[[vk::location(0)]] float4 color;
};

[[vk::binding(2, 1)]] Texture2D testTexture;
[[vk::binding(2, 1)]] SamplerState testSampler;

[[shader("fragment")]] FSOutput fragmentMain(FSInput input)
{
	FSOutput output;
	float2 uv    = input.uv;

	float shadowMapValue = testShadowTexture.Sample(testShadowSampler, input.shadowUv.xy / input.shadowUv.w * 0.5 + float2(0.5) ).r;
	bool isInShadow = shadowMapValue > ( (input.shadowUv.z / input.shadowUv.w) + 0.0001);

	float shadowFactor = isInShadow ? 0.2 : 1.0;

	float lightDistance = length(lightData.lightPos - input.worldPos);
	output.color	    = shadowFactor * input.color * max(dot(lightData.lightPos, input.normal), 0.0) * (10.0 / (lightDistance * lightDistance)) + float4(0.1, 0.1, 0.1, 0.0);
	output.color.a	    = 1.0;

	output.color *= testTexture.Sample(testSampler, uv);

	return output;
}
