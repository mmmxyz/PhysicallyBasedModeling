
struct LightPersMatrixData {
	float4x4 persMat;
};

struct VSInput {
	[[vk::location(0)]] float3 position;
	[[vk::location(1)]] float3 normal;
	[[vk::location(2)]] float4 color;
	[[vk::location(3)]] float2 uv;
};

[[shader("vertex")]] VSOutput vertexMain(VSInput input)
{
	VSOutput output;

	float3 pos = input.position;
	pos	   = mul(vectorToRotation(float3(0.0, 1.0, 0.0) * uboData.time), pos);
	pos.z -= 2;
	pos.y -= 1.0;

	float4 pos4 = float4(pos, 1.0);
	pos4	    = mul(persMatrix.persMat, pos4);

	output.position = pos4;

	output.uv = input.uv;

	float lightDistance = length(lightData.lightPos - pos);
	output.color	    = input.color * max(dot(lightData.lightPos, input.normal), 0.0) * (10.0 / (lightDistance * lightDistance)) + float4(0.1, 0.1, 0.1, 0.0);
	output.color.a	    = 1.0;

	return output;
}
