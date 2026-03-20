// =============================================================================
// G-Buffer シェーダ (ベースシェーディング第1階層)
// =============================================================================
// GBufferPass の Subpass 0 で動作する.
// 各オブジェクトのジオメトリ情報を MRT (Multiple Render Targets) の
// 4つのカラーアタッチメントに書き出す:
//   attachment 1: Albedo (反射率/色)
//   attachment 2: Normal (ワールド空間法線, [0,1]にエンコード)
//   attachment 3: Position (ワールド空間座標)
//   attachment 4: MetallicRoughness (金属度 + 粗さ)
//
// 結果は後続の Subpass 1 (lighting.slang) の Input Attachment として読み取られ,
// PBR ライティング計算に使用される.
// =============================================================================

// ビュー行列と透影行列 (Reversed-Z) を保持するカメラデータ
struct CameraData {
    float4x4 cameraMatrix;
    float4x4 persMatrix;
};

// SRT行列とテクスチャ使用フラグを保持するオブジェクトデータ
struct ObjectData {
    float4x4 srtMatrix;
    uint useAlbedoTexture;
    uint useMetallicRoughnessTexture;
    uint useNormalTexture;
    uint padding0;
};

// Set0: カメラ定数バッファ
[[vk::binding(0, 0)]]
ConstantBuffer<CameraData> camera;

// Set1: オブジェクト定数バッファとテクスチャ群
[[vk::binding(0, 1)]]
ConstantBuffer<ObjectData> object;

[[vk::binding(1, 1)]]
Texture2D<float4> albedoTexture;
[[vk::binding(1, 1)]]
SamplerState textureSampler;

[[vk::binding(2, 1)]]
Texture2D<float4> metallicRoughnessTexture;
[[vk::binding(2, 1)]]
SamplerState metallicRoughnessSampler;

[[vk::binding(3, 1)]]
Texture2D<float4> normalTexture;
[[vk::binding(3, 1)]]
SamplerState normalSampler;

// 頂点シェーダ入力: position, normal, color, uv, tangent (xyz + w=接線方向符号), roughness
struct VSInput {
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float4 color;
    [[vk::location(3)]] float2 uv;
    [[vk::location(4)]] float4 tangent;
    [[vk::location(5)]] float roughness;
};

// 頂点シェーダ出力: クリップ空間座標, ワールド空間情報一式
struct VSOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 worldPos;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float4 color;
    [[vk::location(3)]] float2 uv;
    [[vk::location(4)]] float3 tangent;
    [[vk::location(5)]] float tangentW;
    [[vk::location(6)]] float roughness;
};

// G-Buffer への出力: 4つのカラーアタッチメント
struct PSOutput {
    [[vk::location(0)]] float4 albedo;
    [[vk::location(1)]] float4 normal;
    [[vk::location(2)]] float4 position;
    [[vk::location(3)]] float4 metallicRoughness;
};

// 頂点シェーダ: オブジェクト空間 -> ワールド空間 -> カメラ空間 -> クリップ空間に変換.
// 法線とタンジェントは SRT 行列の回転部分 (3x3) で変換する.
[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(object.srtMatrix, float4(input.position, 1.0));
    float4 viewPos = mul(camera.cameraMatrix, worldPos);
    output.position = mul(camera.persMatrix, viewPos);

    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)object.srtMatrix, input.normal));
    output.color = input.color;
    output.uv = input.uv;
    output.tangent = normalize(mul((float3x3)object.srtMatrix, input.tangent.xyz));
    output.tangentW = input.tangent.w;
    output.roughness = input.roughness;

    return output;
}

// フラグメントシェーダ: テクスチャフラグに応じてアルベド/法線/メタリックラフネスを決定し G-Buffer に書き出す.
// 法線マップ使用時は TBN 行列を構築してタンジェント空間 -> ワールド空間に変換する.
[shader("fragment")]
PSOutput fragmentMain(VSOutput input)
{
    PSOutput output;

    if (object.useAlbedoTexture != 0) {
        float4 texColor = albedoTexture.Sample(textureSampler, input.uv);
        output.albedo = texColor * input.color;
    } else {
        output.albedo = input.color;
    }

    float3 N = normalize(input.normal);
    if (object.useNormalTexture != 0) {
        float3 T = normalize(input.tangent);
        float3 B = cross(N, T) * input.tangentW;
        float3x3 TBN = float3x3(T, B, N);
        float3 texNormal = normalTexture.Sample(normalSampler, input.uv).xyz * 2.0 - 1.0;
        N = normalize(mul(texNormal, TBN));
    }
    output.normal = float4(N * 0.5 + 0.5, 1.0);

    output.position = float4(input.worldPos, 1.0);

    if (object.useMetallicRoughnessTexture != 0) {
        float2 mr = metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.uv).rg;
        output.metallicRoughness = float4(mr.r, mr.g, 0.0, 1.0);
    } else {
        output.metallicRoughness = float4(0.0, input.roughness, 0.0, 1.0);
    }

    return output;
}