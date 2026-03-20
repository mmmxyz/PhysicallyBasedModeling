// =============================================================================
// ライティングシェーダ (ベースシェーディング第2階層)
// =============================================================================
// GBufferPass の Subpass 1 で動作する.
// Subpass 0 (gbuffer.slang) が書き込んだ G-Buffer を Input Attachment 形式で読み取り,
// 以下の処理を行ってスワップチェーンに最終カラーを出力する:
//   1. G-Buffer からアルベド.法線.座標.メタリック/ラフネスを取得
//   2. Cook-Torrance BRDF による物理ベースライティング (点光源)
//   3. PCF 5x5 シャドウマッピング (Reversed-Z 対応)
//   4. トーンマッピング
//   5. sRGB ガンマ補正 (リニア空間 -> sRGB)
//
// フルスクリーン三角形 (頂点3つ, 頂点バッファ不要) でスクリーン全体をカバーする.
// =============================================================================

// 点光源データ:
//   lightPos         : ワールド空間の点光源位置
//   lightIntensity   : 光源強度
//   lightColor       : 光源色 RGB
//   cameraPos        : カメラ位置 (視線ベクトル V の計算に使用)
//   lightPersMatrix  : シャドウマップ座標変換用透影行列
//   lightCameraMatrix: シャドウマップ座標変換用ビュー行列
struct LightData {
    float3 lightPos;
    float lightIntensity;
    float3 lightColor;
    float padding2;
    float3 cameraPos;
    float padding3;
    float4x4 lightPersMatrix;
    float4x4 lightCameraMatrix;
};

// G-Buffer の Input Attachment (Subpass 0 からの入力)
[[vk::input_attachment_index(0)]]
[[vk::binding(0, 0)]]
SubpassInput<float4> albedoInput;

[[vk::input_attachment_index(1)]]
[[vk::binding(1, 0)]]
SubpassInput<float4> normalInput;

[[vk::input_attachment_index(2)]]
[[vk::binding(2, 0)]]
SubpassInput<float4> positionInput;

[[vk::input_attachment_index(3)]]
[[vk::binding(3, 0)]]
SubpassInput<float4> metallicRoughnessInput;

[[vk::binding(0, 1)]]
ConstantBuffer<LightData> light;

[[vk::binding(1, 1)]]
Texture2D shadowMap;
[[vk::binding(1, 1)]]
SamplerState shadowSampler;

struct VSOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv;
};

// 頂点シェーダ: 頂点バッファ不要のフルスクリーン三角形を生成する
[shader("vertex")]
VSOutput vertexMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = positions[vertexID] * 0.5 + 0.5;
    return output;
}

static const float PI = 3.14159265358979323846;

float D_GGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float3 F_Schlick(float HdotV, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotL, float NdotV, float roughness)
{
    return G_SchlickGGX(NdotL, roughness) * G_SchlickGGX(NdotV, roughness);
}

// ワールド座標をシャドウマップ空間に変換し PCF 5x5 でシャドウ係数を計算する.
// 返り値: 1.0 = 完全点灯, 0.2 = 完全影
// - Reversed-Z 対応の slope ベースバイアスを適用
// - PCF: sampledDepth > currentDepth + bias -> 点灯 (0.0 は影, 1.0 は点灯)
// - 最終: lerp(0.2, 1.0, shadow)
float ComputeShadowFactor(float3 worldPos, float3 normal)
{
    float4 shadowClipPos = mul(light.lightPersMatrix, mul(light.lightCameraMatrix, float4(worldPos, 1.0)));

    float w = shadowClipPos.w;
    if (abs(w) < 0.0001) return 1.0;

    float3 shadowNdc = shadowClipPos.xyz / w;
    float2 shadowUv = shadowNdc.xy * 0.5 + 0.5;
    float currentDepth = shadowNdc.z;

    if (shadowUv.x < 0.0 || shadowUv.x > 1.0 || shadowUv.y < 0.0 || shadowUv.y > 1.0) {
        return 1.0;
    }

    float3 lightDir = normalize(light.lightPos - worldPos);
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float slopeFactor = sqrt(1.0 - cosTheta * cosTheta) / max(cosTheta, 0.01);
    float bias = clamp(0.0005 * slopeFactor + 0.0002, 0.0002, 0.002);

    float shadow = 0.0;
    uint wi, h;
    shadowMap.GetDimensions(wi, h);
    float2 texelSize = 1.0 / float2(wi, h);

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            float2 offset = float2(x, y) * texelSize;
            float sampledDepth = shadowMap.Sample(shadowSampler, shadowUv + offset).r;
            shadow += (sampledDepth > (currentDepth + bias)) ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;

    return lerp(0.2, 1.0, shadow);
}

// フラグメントシェーダ:
// G-Buffer からデータ取得 -> Cook-Torrance BRDF -> シャドウ -> トーンマップ -> sRGB 出力
[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    float4 albedo            = albedoInput.SubpassLoad();
    float4 normalEncoded     = normalInput.SubpassLoad();
    float4 position          = positionInput.SubpassLoad();
    float4 metallicRoughness = metallicRoughnessInput.SubpassLoad();

    if (albedo.a < 0.01) {
        discard;
    }

    float3 N        = normalize(normalEncoded.xyz * 2.0 - 1.0);
    float3 worldPos = position.xyz;
    float  metallic  = metallicRoughness.r;
    float  roughness = clamp(metallicRoughness.g, 0.04, 1.0);

    float3 V = normalize(light.cameraPos - worldPos);
    float3 L = normalize(light.lightPos - worldPos);

    float NdotL = dot(N, L);
    float NdotV = dot(N, V);

    if (NdotL <= 0.0 || NdotV <= 0.0) {
        float3 kd_ambient = (1.0 - metallic) * albedo.rgb;
        return float4(pow(kd_ambient * 0.03, 1.0 / 2.2), 1.0);
    }

    float3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float3 F0 = lerp(float3(0.04), albedo.rgb, metallic);

    float alpha = roughness * roughness;

    float  D = D_GGX(NdotH, alpha);
    float3 F = F_Schlick(HdotV, F0);
    float  G = G_Smith(NdotL, NdotV, roughness);

    float3 specular = (D * F * G) / max(4.0 * NdotL * NdotV, 0.0001);

    float3 kd = (float3(1.0) - F) * (1.0 - metallic);
    float3 diffuse = kd * albedo.rgb / PI;

    float lightDist   = length(light.lightPos - worldPos);
    float attenuation = 1.0 / max(lightDist * lightDist, 0.0001);

    float shadowFactor = ComputeShadowFactor(worldPos, N);

    float3 radiance = light.lightColor * light.lightIntensity * attenuation;
    float3 Lo = (diffuse + specular) * NdotL * radiance * shadowFactor;

    float3 ambient = kd * albedo.rgb * 0.03;

    float3 finalColor = Lo + ambient;

    finalColor = pow(max(finalColor, 0.0), 1.0 / 2.2);

    return float4(finalColor, 1.0);
}