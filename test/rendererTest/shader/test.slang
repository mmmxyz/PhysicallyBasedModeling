
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
	[[vk::location(1)]] float4 color;
};

struct PersMatrixData {
	float4x4 persMat;
};

struct LightData {
	float3 lightPos;
	float3 color;
};

struct UboData {
	float time;
	float3 padding;
};

[[vk::binding(0, 0)]] ConstantBuffer<PersMatrixData> persMatrix;
[[vk::binding(1, 0)]] ConstantBuffer<LightData> lightData;
[[vk::binding(0, 1)]] ConstantBuffer<UboData> uboData;

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


	if (theta < 0.0001)
	{
		return float3x3(1.0, 0.0, 0.0,
						0.0, 1.0, 0.0,
						0.0, 0.0, 1.0);
	}

	float3 nvec = normalize(vec);

	rotMatrix = cos(theta) * float3x3(1.0, 0.0, 0.0,
									   0.0, 1.0, 0.0,
									   0.0, 0.0, 1.0)
				+ (1 - cos(theta)) * outerProdut(nvec , nvec)
				+ sin(theta) * float3x3(0.0, -nvec.z, nvec.y,
										 nvec.z, 0.0, -nvec.x,
										 -nvec.y, nvec.x, 0.0);

	return rotMatrix;

}


[[shader("vertex")]] VSOutput vertexMain(VSInput input)
{
	VSOutput output;

	float3 pos = input.position;
	pos = mul( vectorToRotation(float3(0.0, 1.0, 0.0) * uboData.time) , pos );
	pos.z -= 2;
	pos.y -= 1.0;




	float4 pos4 = float4(pos, 1.0);
	pos4 = mul( persMatrix.persMat , pos4 );

	output.position = pos4;

	output.uv = input.uv ;


	float lightDistance = length(lightData.lightPos - pos);
	output.color = input.color * max( dot(lightData.lightPos, input.normal), 0.0) * (10.0 / (lightDistance * lightDistance)) + float4(0.1, 0.1, 0.1, 0.0);
	output.color.a = 1.0;


	return output;
}

struct FSInput {
	float4 position : SV_Position;
	[[vk::location(0)]] float2 uv;
	[[vk::location(1)]] float4 color;
};

struct FSOutput {
	[[vk::location(0)]] float4 color;
};

[[vk::binding(1, 1)]] Texture2D testTexture;
[[vk::binding(1, 1)]] SamplerState testSampler;

[[shader("fragment")]] FSOutput fragmentMain(FSInput input)
{
	FSOutput output;
	float2 uv = input.uv ;
	output.color = testTexture.Sample(testSampler, uv) * input.color;
	return output;
}
