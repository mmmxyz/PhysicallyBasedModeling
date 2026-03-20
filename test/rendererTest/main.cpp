#include "src/utils/mathfunc/mathUtils.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/mesh/drawArray.hpp"
#include "src/utils/memory/allocator.hpp"

#include <cstring>
#include <filesystem>
#include <array>
#include <memory>
#include <vector>

#include "src/utils/fileloader/OBJLoader.hpp"
#include "src/utils/geometry/meshgenerator.hpp"
#include "src/utils/geometry/MeshConv.hpp"
#include "src/utils/geometry/IntOnMesh.hpp"

std::string GetShaderResourceDir()
{
	return SHADER_BINARY_DIR;
}

struct MaterialFlags {
	uint32_t useAlbedoTexture;
	uint32_t useMetallicRoughnessTexture;
	uint32_t useNormalTexture;
	uint32_t padding0;
};

Renderer::GpuTexture CreateDefaultTexture(Renderer& renderer, uint32_t rgba)
{
	Renderer::CreateImageParams params;
	params.width = 1;
	params.height = 1;
	params.format = Renderer::ImageFormat::RGBA8_UNORM;
	auto tex = renderer.CreateGpuTexture(params);

	auto staging = renderer.CreateGpuBuffer(4, Renderer::Transfer);
	uint32_t* cpu = nullptr;
	renderer.GetCpuMemoryPointer(staging, (void**)&cpu);
	*cpu = rgba;
	renderer.UnmapCpuMemoryPointer(staging);
	renderer.TransferStagingBufferToImage(staging, tex);

	return tex;
}

class DrawObject {
public:
	enum class TextureType {
		Albedo,
		MetallicRoughness,
		Normal,
	};

	Renderer::DescriptorSetInterface descriptorSetInterface;
	Renderer::DescriptorSetInterface descriptorSetInterfaceForShadow;
	DrawVertexArray<BasicVertex> drawArray;
	DrawVertexArray<int32_t> indexdrawArray;

	Renderer::GpuBuffer uboBuffer;
	Renderer::GpuBuffer srtMatrixBuffer;
	Renderer::GpuTexture textureMemory;
	Renderer::GpuTexture metallicRoughnessTexture;
	Renderer::GpuTexture normalTexture;

	fmat4 srtMatrix;
	MaterialFlags materialFlags = {};

	Renderer::DescriptorWriterParams descriptorWriterParams;
	Renderer::DescriptorWriterParams descriptorWriterParamsForShadow;

	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	DrawObject(TypeAllocator<BasicVertex>& vertexAllocator, TypeAllocator<int32_t>& intAllocator)
		: drawArray(0, vertexAllocator)
		, indexdrawArray(0, intAllocator, true)
	{
	}

	DrawObject(TypeAllocator<BasicVertex>& vertexAllocator, TypeAllocator<int32_t>& intAllocator, uint32_t vertexCount, uint32_t indexCount)
		: drawArray(0, vertexAllocator)
		, indexdrawArray(0, intAllocator, true)
		, vertexCount(vertexCount)
		, indexCount(indexCount)
	{
	}

	void Initialize(Renderer& renderer)
	{
		drawArray.resize(vertexCount);
		renderer.InitializeVertexArray(&drawArray);

		indexdrawArray.resize(indexCount);
		renderer.InitializeVertexArray(&indexdrawArray);

		uboBuffer = renderer.CreateGpuBuffer(32, Renderer::Uniform);
		void* uboCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(uboBuffer, &uboCpuPtr);
		float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
		*uboFloatCpuPtr = 0.0f;
		renderer.UnmapCpuMemoryPointer(uboBuffer);

		srtMatrix = fmat4::identity();
		materialFlags = { 0, 0, 0, 0 };
		srtMatrixBuffer = renderer.CreateGpuBuffer(sizeof(fmat4) + sizeof(MaterialFlags), Renderer::Uniform);
		UploadObjectData(renderer);

		descriptorSetInterface = renderer.CreateDescriptorSetInterface("testPipeline", 1);
		descriptorSetInterfaceForShadow = renderer.CreateDescriptorSetInterface("shadowTestPipeline", 1);

		Renderer::DescriptorWriterParams::DescriptorInfo uboDescriptorInfo;
		uboDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		uboDescriptorInfo.bindingNum = 0;
		uboDescriptorInfo.count = 1;
		uboDescriptorInfo.pResources.resize(1);
		uboDescriptorInfo.pResources[0] = uboBuffer.pGpuMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(uboDescriptorInfo);
		Renderer::DescriptorWriterParams::DescriptorInfo srtMatrixDescriptorInfo;
		srtMatrixDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		srtMatrixDescriptorInfo.bindingNum = 1;
		srtMatrixDescriptorInfo.count = 1;
		srtMatrixDescriptorInfo.pResources.resize(1);
		srtMatrixDescriptorInfo.pResources[0] = srtMatrixBuffer.pGpuMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(srtMatrixDescriptorInfo);

		Renderer::DescriptorWriterParams::DescriptorInfo srtMatrixDescriptorInfoForShadow;
		srtMatrixDescriptorInfoForShadow.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		srtMatrixDescriptorInfoForShadow.bindingNum = 0;
		srtMatrixDescriptorInfoForShadow.count = 1;
		srtMatrixDescriptorInfoForShadow.pResources.resize(1);
		srtMatrixDescriptorInfoForShadow.pResources[0] = srtMatrixBuffer.pGpuMemoryImpl;
		descriptorWriterParamsForShadow.descriptorInfos.push_back(srtMatrixDescriptorInfoForShadow);
	}

	void UploadObjectData(Renderer& renderer)
	{
		void* ptr = nullptr;
		renderer.GetCpuMemoryPointer(srtMatrixBuffer, &ptr);
		std::memcpy(ptr, srtMatrix.cmp, sizeof(fmat4));
		std::memcpy(static_cast<uint8_t*>(ptr) + sizeof(fmat4), &materialFlags, sizeof(MaterialFlags));
		renderer.UnmapCpuMemoryPointer(srtMatrixBuffer);
	}

	void WriteDescriptorSet(Renderer& renderer)
	{
		renderer.WriteDescriptorSet(descriptorWriterParams, descriptorSetInterface);
		renderer.WriteDescriptorSet(descriptorWriterParamsForShadow, descriptorSetInterfaceForShadow);
	}

	void SetTexture(Renderer& renderer, TextureType slot, uint32_t width, uint32_t height, const void* cpuData,
		Renderer::ImageFormat format = Renderer::ImageFormat::RGBA8_UNORM)
	{
		Renderer::CreateImageParams createImageParams;
		createImageParams.width = width;
		createImageParams.height = height;
		createImageParams.format = format;

		Renderer::GpuTexture& targetTex = (slot == TextureType::Albedo) ? textureMemory
			: (slot == TextureType::MetallicRoughness) ? metallicRoughnessTexture
			: normalTexture;
		targetTex = renderer.CreateGpuTexture(createImageParams);

		Renderer::GpuBuffer stagingBuffer = renderer.CreateGpuBuffer(width * height * 4, Renderer::Transfer);
		void* stagingBufferCpu = nullptr;
		renderer.GetCpuMemoryPointer(stagingBuffer, &stagingBufferCpu);
		std::memcpy(stagingBufferCpu, cpuData, width * height * 4);
		renderer.UnmapCpuMemoryPointer(stagingBuffer);
		renderer.TransferStagingBufferToImage(stagingBuffer, targetTex);

		switch (slot) {
		case TextureType::Albedo:
			materialFlags.useAlbedoTexture = 1;
			break;
		case TextureType::MetallicRoughness:
			materialFlags.useMetallicRoughnessTexture = 1;
			break;
		case TextureType::Normal:
			materialFlags.useNormalTexture = 1;
			break;
		}
		UploadObjectData(renderer);
	}
};

void CreateRenderPass(Renderer& renderer)
{
	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name = "RenderPass";
	renderPassParams.attachments = {
		{
			Renderer::ImageFormat::SameAsSwapChain,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::PresentSrcKHR,
			Renderer::UseSwapChainAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			false, // isInputAttatchment
		},
		{
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		}
	};
	renderPassParams.subpasses = {
		{
			{ 0, },
			{ },
			1
		}
	};
	renderPassParams.dependencies = {
		{ -1, 0 },
	};

	renderer.CreateRenderPass(renderPassParams);

	Renderer::RenderPassParams clearRenderPassParams;
	clearRenderPassParams.name = "ClearRenderPass";
	clearRenderPassParams.attachments = {
		{
			Renderer::ImageFormat::SameAsSwapChain,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::UseSwapChainAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			false, // isInputAttatchment
		},
		{
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		}
	};
	clearRenderPassParams.subpasses = {
		{
			{ 0, },
			{ },
			1
		}
	};
	clearRenderPassParams.dependencies = {
		{ -1, 0 },
	};
	clearRenderPassParams.isClearRenderPass = true;
	clearRenderPassParams.clearRenderPassName = "RenderPass";

	renderer.CreateRenderPass(clearRenderPassParams);
}


void CreateGBufferRenderPass(Renderer& renderer)
{
	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name = "GBufferPass";
	renderPassParams.attachments = {
		{ // 0: SwapChain (最終出力)
			Renderer::ImageFormat::SameAsSwapChain,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::PresentSrcKHR,
			Renderer::UseSwapChainAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			false, // isInputAttatchment
		},
		{ // 1: Albedo - カラーアタッチメント + Input Attachment
			Renderer::ImageFormat::RGBA8_UNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::AlbedoAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 2: Normal - カラーアタッチメント + Input Attachment
			Renderer::ImageFormat::RGBA16_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::NormalAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 3: Position - カラーアタッチメント + Input Attachment
			Renderer::ImageFormat::RGBA32_FLOAT,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::PositionAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 4: MetallicRoughness - カラーアタッチメント + Input Attachment
			Renderer::ImageFormat::RGBA8_UNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::MetallicRoughnessAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 5: Depth
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		}
	};
	renderPassParams.subpasses = {
		{
			{ 1, 2, 3, 4 },
			{ },
			5
		},
		{
			{ 0 },
			{ 1, 2, 3, 4 },
			-1
		}
	};
	renderPassParams.dependencies = {
		{ -1, 0 },
		{ 0, 1 },
	};

	renderer.CreateRenderPass(renderPassParams);

	Renderer::RenderPassParams clearRenderPassParams;
	clearRenderPassParams.name = "ClearGBufferPass";
	clearRenderPassParams.attachments = {
		{ // 0
			Renderer::ImageFormat::SameAsSwapChain,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::UseSwapChainAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			false, // isInputAttatchment
		},
		{ // 1: Albedo
			Renderer::ImageFormat::RGBA8_UNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::AlbedoAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 2: Normal
			Renderer::ImageFormat::RGBA16_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::NormalAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 3: Position
			Renderer::ImageFormat::RGBA32_FLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::PositionAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 4: MetallicRoughness
			Renderer::ImageFormat::RGBA8_UNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::MetallicRoughnessAttachment,
			true,  // isColorAttatchment
			false, // isDepthStencilAttatchment
			true,  // isInputAttatchment
		},
		{ // 5: Depth
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		}
	};
	clearRenderPassParams.subpasses = {
		{
			{ 0, 1, 2, 3, 4 },
			{ },
			5
		}
	};
	clearRenderPassParams.dependencies = {
		{ -1, 0 },
	};
	clearRenderPassParams.isClearRenderPass = true;
	clearRenderPassParams.clearRenderPassName = "GBufferPass";

	renderer.CreateRenderPass(clearRenderPassParams);
}

void CreateRenderPassForShadowMap(Renderer& renderer)
{
	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name = "ShadowMapPass";
	renderPassParams.attachments = {
		{
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		}
	};
	renderPassParams.subpasses = {
		{
			{ },
			{ },
			0
		}
	};
	renderPassParams.dependencies = {
		{ -1, 0 },
	};

	renderer.CreateRenderPass(renderPassParams);

	Renderer::RenderPassParams clearRenderPassParams;
	clearRenderPassParams.name = "ClearShadowMapPass";
	clearRenderPassParams.attachments = {
		{
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false, // isColorAttatchment
			true,  // isDepthStencilAttatchment
			false, // isInputAttatchment
		},
	};
	clearRenderPassParams.subpasses = {
		{
			{ },
			{ },
			0
		}
	};
	clearRenderPassParams.dependencies = {
		{ -1, 0 },
	};
	clearRenderPassParams.isClearRenderPass = true;
	clearRenderPassParams.clearRenderPassName = "ShadowMapPass";

	renderer.CreateRenderPass(clearRenderPassParams);
}

Renderer::VertexAttributeLayout RegisterVertexAttribute(Renderer& renderer)
{
	Renderer::VertexAttributeLayout vertexAttributeLayout;
	Renderer::CreateVertexAttributeLayout2<BasicVertex>(&vertexAttributeLayout);
	renderer.RegisterVertexInputStateImpl3(&vertexAttributeLayout);

	return vertexAttributeLayout;
}

// フォワードレンダリング用ジオメトリパイプライン
// - RenderPass 上で動作し、頂点シェーダ＋フラグメントシェーダで直接描画する
// - Set0: カメラ行列(binding0) + ライトデータ(binding1) + シャドウマップテクスチャ(binding2)
// - Set1: オブジェクト固有データ（UBO(binding0) + SRT行列(binding1) + テクスチャ(binding2)）
// - Reversed-Z デプステスト（Greater）、プッシュコンスタントあり
void CreateGeometryPipeline(Renderer& renderer, std::string vertexAttributeName)
{
	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = GetShaderResourceDir() + "/test.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = GetShaderResourceDir() + "/test.frag.spv";

	graphicsPipelineParams.name = "testPipeline";
	graphicsPipelineParams.shaders.resize(2);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;
	graphicsPipelineParams.shaders[1] = &fragmentShaderStage;

	graphicsPipelineParams.vertexLayoutName = vertexAttributeName;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout;
	Renderer::DescriptorSetBindingParams persMatUboDescriptorSetLayout;
	persMatUboDescriptorSetLayout.bindingNum = 0;
	persMatUboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	persMatUboDescriptorSetLayout.count = 1;
	persMatUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&persMatUboDescriptorSetLayout);
	Renderer::DescriptorSetBindingParams lightUboDescriptorSetLayout;
	lightUboDescriptorSetLayout.bindingNum = 1;
	lightUboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	lightUboDescriptorSetLayout.count = 1;
	lightUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit | Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&lightUboDescriptorSetLayout);
	Renderer::DescriptorSetBindingParams shadowTextureDescriptorSetLayout;
	shadowTextureDescriptorSetLayout.bindingNum = 2;
	shadowTextureDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	shadowTextureDescriptorSetLayout.count = 1;
	shadowTextureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&shadowTextureDescriptorSetLayout);
	descriptorSetLayout.isBindless = false;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout2;
	Renderer::DescriptorSetBindingParams uboDescriptorSetLayout;
	uboDescriptorSetLayout.bindingNum = 0;
	uboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	uboDescriptorSetLayout.count = 1;
	uboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&uboDescriptorSetLayout);

	Renderer::DescriptorSetBindingParams srtMatrixDesciptorSetLayout;
	srtMatrixDesciptorSetLayout.bindingNum = 1;
	srtMatrixDesciptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	srtMatrixDesciptorSetLayout.count = 1;
	srtMatrixDesciptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&srtMatrixDesciptorSetLayout);

	Renderer::DescriptorSetBindingParams textureDescriptorSetLayout;
	textureDescriptorSetLayout.bindingNum = 2;
	textureDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	textureDescriptorSetLayout.count = 1;
	textureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&textureDescriptorSetLayout);

	descriptorSetLayout2.isBindless = false;

	graphicsPipelineParams.depthOperator = Renderer::CompareOperator::Greater;
	graphicsPipelineParams.depthTestEnable = true;
	graphicsPipelineParams.depthWriteEnable = true;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout2);

	graphicsPipelineParams.renderPassName = "RenderPass";
	graphicsPipelineParams.subpassIndex = 0;
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

// G-Buffer書き込みパイプライン（遅延シェーディング第1段階）
// - GBufferPass の Subpass 0 で動作
// - 4つのカラーアタッチメント（Albedo, Normal, Position, MetallicRoughness）にMRTで出力
// - Set0: カメラ行列（binding0, Vertex）
// - Set1: オブジェクト固有データ
//   - binding0: SRT行列 + MaterialFlags（Vertex|Fragment）
//   - binding1: アルベドテクスチャ（Fragment）
//   - binding2: メタリック・ラフネステクスチャ（Fragment）
//   - binding3: 法線テクスチャ（Fragment）
// - Reversed-Z デプステスト（Greater）、デプス書き込みあり
void CreateGBufferPipeline(Renderer& renderer, std::string vertexAttributeName)
{
	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = GetShaderResourceDir() + "/gbuffer.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = GetShaderResourceDir() + "/gbuffer.frag.spv";

	graphicsPipelineParams.name = "gBufferPipeline";
	graphicsPipelineParams.shaders.resize(2);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;
	graphicsPipelineParams.shaders[1] = &fragmentShaderStage;

	graphicsPipelineParams.vertexLayoutName = vertexAttributeName;

	// 0 カメラ行列
	Renderer::DescriptorSetLayoutParams descriptorSetLayout0;
	Renderer::DescriptorSetBindingParams cameraMatrixBinding;
	cameraMatrixBinding.bindingNum = 0;
	cameraMatrixBinding.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	cameraMatrixBinding.count = 1;
	cameraMatrixBinding.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout0.descriptorSetBindingParams.push_back(&cameraMatrixBinding);
	descriptorSetLayout0.isBindless = false;

	// 1 オブジェクト固有（SRT行列、テクスチャ、マテリアルフラグ）
	Renderer::DescriptorSetLayoutParams descriptorSetLayout1;
	Renderer::DescriptorSetBindingParams srtMatrixBinding;
	srtMatrixBinding.bindingNum = 0;
	srtMatrixBinding.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	srtMatrixBinding.count = 1;
	srtMatrixBinding.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit | Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&srtMatrixBinding);

	Renderer::DescriptorSetBindingParams albedoTextureBinding;
	albedoTextureBinding.bindingNum = 1;
	albedoTextureBinding.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	albedoTextureBinding.count = 1;
	albedoTextureBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&albedoTextureBinding);

	Renderer::DescriptorSetBindingParams mrTextureBinding;
	mrTextureBinding.bindingNum = 2;
	mrTextureBinding.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	mrTextureBinding.count = 1;
	mrTextureBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&mrTextureBinding);

	Renderer::DescriptorSetBindingParams normalTextureBinding;
	normalTextureBinding.bindingNum = 3;
	normalTextureBinding.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	normalTextureBinding.count = 1;
	normalTextureBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&normalTextureBinding);
	descriptorSetLayout1.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout0);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout1);

	graphicsPipelineParams.depthOperator = Renderer::CompareOperator::Greater;
	graphicsPipelineParams.depthTestEnable = true;
	graphicsPipelineParams.depthWriteEnable = true;

	graphicsPipelineParams.renderPassName = "GBufferPass";
	graphicsPipelineParams.subpassIndex = 0;
	graphicsPipelineParams.pushConstantSize = 0;

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

// ライティングパイプライン（遅延シェーディング第2段階）
// - GBufferPass の Subpass 1 で動作
// - Input Attachment 経由で G-Buffer（Albedo, Normal, Position, MetallicRoughness）を読み取り
// - Cook-Torrance BRDF による PBR ライティング＋ PCF シャドウマッピングを実行
// - トーンマッピング＋ガンマ補正を適用し、スワップチェーンに最終カラーを出力
// - Set0: Input Attachments（binding0-3: Albedo, Normal, Position, MetallicRoughness）
// - Set1: ライトデータUBO（binding0）+ シャドウマップテクスチャ（binding1）
// - デプステスト無効、フルスクリーン三角形（頂点3つ）で描画
void CreateLightingPipeline(Renderer& renderer, std::string vertexAttributeName)
{
	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = GetShaderResourceDir() + "/lighting.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = GetShaderResourceDir() + "/lighting.frag.spv";

	graphicsPipelineParams.name = "lightingPipeline";
	graphicsPipelineParams.shaders.resize(2);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;
	graphicsPipelineParams.shaders[1] = &fragmentShaderStage;

	graphicsPipelineParams.vertexLayoutName = vertexAttributeName;

	// Albedo, Normal, Position, MetallicRoughness
	Renderer::DescriptorSetLayoutParams descriptorSetLayout0;

	Renderer::DescriptorSetBindingParams albedoInputBinding;
	albedoInputBinding.bindingNum = 0;
	albedoInputBinding.type = Renderer::DescriptorSetBindingParams::InputAttachment_bit;
	albedoInputBinding.count = 1;
	albedoInputBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout0.descriptorSetBindingParams.push_back(&albedoInputBinding);

	Renderer::DescriptorSetBindingParams normalInputBinding;
	normalInputBinding.bindingNum = 1;
	normalInputBinding.type = Renderer::DescriptorSetBindingParams::InputAttachment_bit;
	normalInputBinding.count = 1;
	normalInputBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout0.descriptorSetBindingParams.push_back(&normalInputBinding);

	Renderer::DescriptorSetBindingParams positionInputBinding;
	positionInputBinding.bindingNum = 2;
	positionInputBinding.type = Renderer::DescriptorSetBindingParams::InputAttachment_bit;
	positionInputBinding.count = 1;
	positionInputBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout0.descriptorSetBindingParams.push_back(&positionInputBinding);

	Renderer::DescriptorSetBindingParams metallicRoughnessInputBinding;
	metallicRoughnessInputBinding.bindingNum = 3;
	metallicRoughnessInputBinding.type = Renderer::DescriptorSetBindingParams::InputAttachment_bit;
	metallicRoughnessInputBinding.count = 1;
	metallicRoughnessInputBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout0.descriptorSetBindingParams.push_back(&metallicRoughnessInputBinding);
	descriptorSetLayout0.isBindless = false;

	// 1 ライト位置とシャドウマップ
	Renderer::DescriptorSetLayoutParams descriptorSetLayout1;
	Renderer::DescriptorSetBindingParams lightDataBinding;
	lightDataBinding.bindingNum = 0;
	lightDataBinding.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	lightDataBinding.count = 1;
	lightDataBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&lightDataBinding);

	Renderer::DescriptorSetBindingParams shadowMapBinding;
	shadowMapBinding.bindingNum = 1;
	shadowMapBinding.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	shadowMapBinding.count = 1;
	shadowMapBinding.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout1.descriptorSetBindingParams.push_back(&shadowMapBinding);
	descriptorSetLayout1.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout0);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout1);

	graphicsPipelineParams.depthOperator = Renderer::CompareOperator::Always;
	graphicsPipelineParams.depthTestEnable = false;
	graphicsPipelineParams.depthWriteEnable = false;

	graphicsPipelineParams.renderPassName = "GBufferPass";
	graphicsPipelineParams.subpassIndex = 1;
	graphicsPipelineParams.pushConstantSize = 0;

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

// シャドウマップ生成パイプライン（デプスオンリー）
// - ShadowMapPass で動作（カラーアタッチメント0個、デプスのみ）
// - 頂点シェーダのみ（フラグメントシェーダなし）でデプスバッファに書き込み
// - Set0: ライト視点の射影・ビュー行列（binding0, Vertex）
// - Set1: オブジェクトのSRT行列（binding0, Vertex）
// - Reversed-Z デプステスト（Greater）
void CreateShadowMapPipeline(Renderer& renderer, std::string vertexAttributeName)
{
	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = GetShaderResourceDir() + "/shadow.vert.spv";

	graphicsPipelineParams.name = "shadowTestPipeline";
	graphicsPipelineParams.shaders.resize(1);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;

	graphicsPipelineParams.vertexLayoutName = vertexAttributeName;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout;
	Renderer::DescriptorSetBindingParams lightMatrixUboDescriptorSetLayout;
	lightMatrixUboDescriptorSetLayout.bindingNum = 0;
	lightMatrixUboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	lightMatrixUboDescriptorSetLayout.count = 1;
	lightMatrixUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&lightMatrixUboDescriptorSetLayout);
	descriptorSetLayout.isBindless = false;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout2;
	Renderer::DescriptorSetBindingParams srtMatrixDesciptorSetLayout;
	srtMatrixDesciptorSetLayout.bindingNum = 0;
	srtMatrixDesciptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	srtMatrixDesciptorSetLayout.count = 1;
	srtMatrixDesciptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&srtMatrixDesciptorSetLayout);
	descriptorSetLayout2.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout2);

	graphicsPipelineParams.depthOperator = Renderer::CompareOperator::Greater;
	graphicsPipelineParams.depthTestEnable = true;
	graphicsPipelineParams.depthWriteEnable = true;

	graphicsPipelineParams.renderPassName = "ShadowMapPass";
	graphicsPipelineParams.subpassIndex = 0;
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

std::vector<Renderer::DrawParams> MakeDrawParamsForGeometryPipeline(Renderer& renderer, const std::vector<DrawObject*>& drawObjects, Renderer::GpuBuffer& persMatUbo, Renderer::GpuBuffer& lightDataUbo, Renderer::DescriptorSetInterface descriptorSetInterface, Renderer::GpuTexture& shadowMapTexture)
{
	Renderer::DescriptorWriterParams persAndLightUboDescriptorWriterParams;
	Renderer::DescriptorWriterParams::DescriptorInfo persUboDescriptorWriteParams;
	persUboDescriptorWriteParams.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	persUboDescriptorWriteParams.bindingNum = 0;
	persUboDescriptorWriteParams.count = 1;
	persUboDescriptorWriteParams.pResources.resize(1);
	persUboDescriptorWriteParams.pResources[0] = persMatUbo.pGpuMemoryImpl;
	persAndLightUboDescriptorWriterParams.descriptorInfos.push_back(persUboDescriptorWriteParams);
	Renderer::DescriptorWriterParams::DescriptorInfo lightDataUboDescriptorWriteParams;
	lightDataUboDescriptorWriteParams.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	lightDataUboDescriptorWriteParams.bindingNum = 1;
	lightDataUboDescriptorWriteParams.count = 1;
	lightDataUboDescriptorWriteParams.pResources.resize(1);
	lightDataUboDescriptorWriteParams.pResources[0] = lightDataUbo.pGpuMemoryImpl;
	persAndLightUboDescriptorWriterParams.descriptorInfos.push_back(lightDataUboDescriptorWriteParams);

	Renderer::DescriptorWriterParams::DescriptorInfo textureDescriptorInfo;
	textureDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
	textureDescriptorInfo.bindingNum = 2;
	textureDescriptorInfo.count = 1;
	textureDescriptorInfo.pResources.resize(1);
	textureDescriptorInfo.pResources[0] = shadowMapTexture.pGpuTextureMemoryImpl;
	persAndLightUboDescriptorWriterParams.descriptorInfos.push_back(textureDescriptorInfo);

	renderer.WriteDescriptorSet(persAndLightUboDescriptorWriterParams, descriptorSetInterface);

	std::vector<Renderer::DrawParams> drawParamsList;
	for (auto* obj : drawObjects) {
		Renderer::DrawParams dp;
		dp.pVertexArray = obj->drawArray.getGpuMemoryImpl();
		dp.instanceCount = 1;
		dp.pIndexArray = obj->indexdrawArray.getGpuMemoryImpl();
		dp.count = obj->indexdrawArray.size();
		dp.descriptorSetInterfaces.push_back(obj->descriptorSetInterface);
		dp.descriptorSetInterfaces.push_back(descriptorSetInterface);
		dp.graphicsPipelineName = "testPipeline";
		drawParamsList.push_back(dp);
	}
	return drawParamsList;
}

std::vector<Renderer::DrawParams> MakeDrawParamsForShadowPipeline(Renderer& renderer, const std::vector<DrawObject*>& drawObjects, Renderer::GpuBuffer& lightPersMatrixUbo, Renderer::DescriptorSetInterface descriptorSetInterface)
{
	Renderer::DescriptorWriterParams descriptorWriteParams;
	Renderer::DescriptorWriterParams::DescriptorInfo lightAndPersUboDescriptorWriteParams;
	lightAndPersUboDescriptorWriteParams.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	lightAndPersUboDescriptorWriteParams.bindingNum = 0;
	lightAndPersUboDescriptorWriteParams.count = 1;
	lightAndPersUboDescriptorWriteParams.pResources.resize(1);
	lightAndPersUboDescriptorWriteParams.pResources[0] = lightPersMatrixUbo.pGpuMemoryImpl;
	descriptorWriteParams.descriptorInfos.push_back(lightAndPersUboDescriptorWriteParams);

	renderer.WriteDescriptorSet(descriptorWriteParams, descriptorSetInterface);

	std::vector<Renderer::DrawParams> drawParamsList;
	for (auto* obj : drawObjects) {
		Renderer::DrawParams dp;
		dp.pVertexArray = obj->drawArray.getGpuMemoryImpl();
		dp.instanceCount = 1;
		dp.pIndexArray = obj->indexdrawArray.getGpuMemoryImpl();
		dp.count = obj->indexdrawArray.size();
		dp.descriptorSetInterfaces.push_back(obj->descriptorSetInterfaceForShadow);
		dp.descriptorSetInterfaces.push_back(descriptorSetInterface);
		dp.graphicsPipelineName = "shadowTestPipeline";
		drawParamsList.push_back(dp);
	}
	return drawParamsList;
}

std::vector<Renderer::DrawParams> MakeDrawParamsForGBufferPipeline(Renderer& renderer, const std::vector<DrawObject*>& drawObjects, Renderer::GpuBuffer& persMatUbo)
{
	auto gBufferCameraDescSet = renderer.CreateDescriptorSetInterface("gBufferPipeline", 0);
	{
		Renderer::DescriptorWriterParams writerParams;
		Renderer::DescriptorWriterParams::DescriptorInfo cameraMatInfo;
		cameraMatInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		cameraMatInfo.bindingNum = 0;
		cameraMatInfo.count = 1;
		cameraMatInfo.pResources.resize(1);
		cameraMatInfo.pResources[0] = persMatUbo.pGpuMemoryImpl;
		writerParams.descriptorInfos.push_back(cameraMatInfo);
		renderer.WriteDescriptorSet(writerParams, gBufferCameraDescSet);
	}

	std::vector<Renderer::DrawParams> drawParamsList;
	for (auto* obj : drawObjects) {
		auto gBufferObjectDescSet = renderer.CreateDescriptorSetInterface("gBufferPipeline", 1);
		{
			Renderer::DescriptorWriterParams writerParams;
			Renderer::DescriptorWriterParams::DescriptorInfo srtMatInfo;
			srtMatInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
			srtMatInfo.bindingNum = 0;
			srtMatInfo.count = 1;
			srtMatInfo.pResources.resize(1);
			srtMatInfo.pResources[0] = obj->srtMatrixBuffer.pGpuMemoryImpl;
			writerParams.descriptorInfos.push_back(srtMatInfo);
			Renderer::DescriptorWriterParams::DescriptorInfo texInfo;
			texInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
			texInfo.bindingNum = 1;
			texInfo.count = 1;
			texInfo.pResources.resize(1);
			texInfo.pResources[0] = obj->textureMemory.pGpuTextureMemoryImpl;
			writerParams.descriptorInfos.push_back(texInfo);
			Renderer::DescriptorWriterParams::DescriptorInfo mrTexInfo;
			mrTexInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
			mrTexInfo.bindingNum = 2;
			mrTexInfo.count = 1;
			mrTexInfo.pResources.resize(1);
			mrTexInfo.pResources[0] = obj->metallicRoughnessTexture.pGpuTextureMemoryImpl;
			writerParams.descriptorInfos.push_back(mrTexInfo);
			Renderer::DescriptorWriterParams::DescriptorInfo normalTexInfo;
			normalTexInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
			normalTexInfo.bindingNum = 3;
			normalTexInfo.count = 1;
			normalTexInfo.pResources.resize(1);
			normalTexInfo.pResources[0] = obj->normalTexture.pGpuTextureMemoryImpl;
			writerParams.descriptorInfos.push_back(normalTexInfo);
			renderer.WriteDescriptorSet(writerParams, gBufferObjectDescSet);
		}

		Renderer::DrawParams dp;
		dp.pVertexArray = obj->drawArray.getGpuMemoryImpl();
		dp.instanceCount = 1;
		dp.pIndexArray = obj->indexdrawArray.getGpuMemoryImpl();
		dp.count = obj->indexdrawArray.size();
		dp.descriptorSetInterfaces.push_back(gBufferCameraDescSet);
		dp.descriptorSetInterfaces.push_back(gBufferObjectDescSet);
		dp.graphicsPipelineName = "gBufferPipeline";
		drawParamsList.push_back(dp);
	}
	return drawParamsList;
}

Renderer::DrawParams MakeDrawParamsForLightingPipeline(Renderer& renderer, Renderer::GpuBuffer& lightDataUbo)
{
	// set 0: Input Attachments（G-Bufferから読み取り）
	auto lightingInputDescSet = renderer.CreateDescriptorSetInterface("lightingPipeline", 0);
	{
		auto albedoTex = renderer.GetRenderPassAttatchmentTexture("GBufferPass", Renderer::AlbedoAttachment);
		auto normalTex = renderer.GetRenderPassAttatchmentTexture("GBufferPass", Renderer::NormalAttachment);
		auto posTex = renderer.GetRenderPassAttatchmentTexture("GBufferPass", Renderer::PositionAttachment);
		auto mrTex = renderer.GetRenderPassAttatchmentTexture("GBufferPass", Renderer::MetallicRoughnessAttachment);

		Renderer::DescriptorWriterParams writerParams;
		auto pushInputAttatchmentDescriptorInfo = [&](Renderer::GpuTexture& tex, int bind) {
			Renderer::DescriptorWriterParams::DescriptorInfo info;
			info.type = Renderer::DescriptorWriterParams::DescriptorInfo::InputAttachment;
			info.bindingNum = bind;
			info.count = 1;
			info.pResources.resize(1);
			info.pResources[0] = tex.pGpuTextureMemoryImpl;
			writerParams.descriptorInfos.push_back(info);
			};
		pushInputAttatchmentDescriptorInfo(albedoTex, 0);
		pushInputAttatchmentDescriptorInfo(normalTex, 1);
		pushInputAttatchmentDescriptorInfo(posTex, 2);
		pushInputAttatchmentDescriptorInfo(mrTex, 3);
		renderer.WriteDescriptorSet(writerParams, lightingInputDescSet);
	}

	// set 1: ライトデータ + シャドウマップ
	auto lightingLightDescSet = renderer.CreateDescriptorSetInterface("lightingPipeline", 1);
	{
		auto shadowMapTexture = renderer.GetRenderPassAttatchmentTexture("ShadowMapPass", Renderer::AttatchmentLabel::DepthAttachment);

		Renderer::DescriptorWriterParams writerParams;
		Renderer::DescriptorWriterParams::DescriptorInfo lightInfo;
		lightInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		lightInfo.bindingNum = 0;
		lightInfo.count = 1;
		lightInfo.pResources.resize(1);
		lightInfo.pResources[0] = lightDataUbo.pGpuMemoryImpl;
		writerParams.descriptorInfos.push_back(lightInfo);

		Renderer::DescriptorWriterParams::DescriptorInfo shadowMapInfo;
		shadowMapInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
		shadowMapInfo.bindingNum = 1;
		shadowMapInfo.count = 1;
		shadowMapInfo.pResources.resize(1);
		shadowMapInfo.pResources[0] = shadowMapTexture.pGpuTextureMemoryImpl;
		writerParams.descriptorInfos.push_back(shadowMapInfo);

		renderer.WriteDescriptorSet(writerParams, lightingLightDescSet);
	}

	Renderer::DrawParams drawParams;
	drawParams.pVertexArray = nullptr;
	drawParams.instanceCount = 1;
	drawParams.pIndexArray = nullptr;
	drawParams.count = 3;
	drawParams.descriptorSetInterfaces.push_back(lightingInputDescSet);
	drawParams.descriptorSetInterfaces.push_back(lightingLightDescSet);
	drawParams.graphicsPipelineName = "lightingPipeline";

	return drawParams;
}

// バニーオブジェクトを生成・初期化する
void CreateBunnyObject(Renderer& renderer, DrawObject& drawObject)
{
	std::vector<fvec3> positions;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	std::vector<uint32_t> faceindices;

	constexpr auto resourcePath = RESOURCE_DIR "/LightBunny.obj";

	LoadOBJtoRenderTriangleMesh(
		resourcePath,
		positions,
		normals,
		uvs,
		faceindices,
		fvec3(0.0f, 0.0f, 0.0f),
		1.f);

	drawObject.vertexCount = static_cast<uint32_t>(positions.size());
	drawObject.indexCount = static_cast<uint32_t>(faceindices.size());
	drawObject.Initialize(renderer);

	drawObject.metallicRoughnessTexture = CreateDefaultTexture(renderer, 0x00000000);
	drawObject.normalTexture = CreateDefaultTexture(renderer, 0x7F7F0000);
	{
		uint32_t checkerData[128 * 128];
		for (uint32_t i = 0; i < 128; i++)
			for (uint32_t j = 0; j < 128; j++)
				checkerData[128 * i + j] = ((i / 16 + j / 16) % 2 == 0) ? 0xFF555555 : 0xFFFFFFFF;
		drawObject.SetTexture(renderer, DrawObject::TextureType::Albedo, 128, 128, checkerData);
	}

	drawObject.WriteDescriptorSet(renderer);

	for (uint32_t i = 0; i < positions.size(); i++) {
		drawObject.drawArray[i].position(0) = positions[i].x;
		drawObject.drawArray[i].position(1) = positions[i].y;
		drawObject.drawArray[i].position(2) = positions[i].z;
		drawObject.drawArray[i].normal(0) = normals[i].x;
		drawObject.drawArray[i].normal(1) = normals[i].y;
		drawObject.drawArray[i].normal(2) = normals[i].z;
		drawObject.drawArray[i].uv(0) = uvs[i].x;
		drawObject.drawArray[i].uv(1) = uvs[i].y;

		drawObject.drawArray[i].color(0) = 1.0f;
		drawObject.drawArray[i].color(1) = 1.0f;
		drawObject.drawArray[i].color(2) = 1.0f;
		drawObject.drawArray[i].color(3) = 1.0f;

		drawObject.drawArray[i].tangent(0) = 1.0f;
		drawObject.drawArray[i].tangent(1) = 0.0f;
		drawObject.drawArray[i].tangent(2) = 0.0f;
		drawObject.drawArray[i].tangent(3) = 1.0f;
		drawObject.drawArray[i].roughness = 0.5f;
	}

	for (uint32_t i = 0; i < faceindices.size(); i++) {
		drawObject.indexdrawArray[i] = static_cast<int32_t>(faceindices[i]);
	}

	renderer.UpdateVertexArray(&drawObject.drawArray);
	renderer.UpdateVertexArray(&drawObject.indexdrawArray);
}

// 床（平面メッシュ）オブジェクトを生成・初期化する
void CreateFloorObject(Renderer& renderer, DrawObject& drawObject)
{
	fvec2* pVertData2d;
	fvec2* pRestVertData;
	uint32_t vertSize;
	uint32_t* pIListData;
	uint32_t iListSize;
	uint32_t* pEdgeData;
	uint32_t edgeSize;
	uint32_t* pInnerEdgeData;
	uint32_t innerEdgeSize;
	fvec4* pInnerEdgeCData;
	uint32_t boundarySize;
	uint32_t* pBoundaryData;
	fvec3 bias = fvec3(0.0f, 0.0f, 0.0f);

	RectTriangle(
		2,
		2,
		5.0f,
		5.0f,
		&pVertData2d,
		vertSize,
		&pIListData,
		iListSize,
		&pEdgeData,
		edgeSize,
		&pBoundaryData,
		boundarySize,
		fvec2(0.0f, 0.0f));

	uint32_t* pElsup_index = nullptr;
	uint32_t* pElsup = nullptr;
	ConvertEVtoVE(
		vertSize,
		pIListData,
		iListSize,
		&pElsup_index,
		&pElsup);

	fvec3* pVertData = new fvec3[vertSize];
	for (uint32_t i = 0; i < vertSize; i++) {
		pVertData[i] = fvec3(pVertData2d[i].x, 0.0f, -pVertData2d[i].y);
	}

	std::vector<fvec3> normals(vertSize, fvec3::zero());
	ComputeNormalFromVE(
		vertSize,
		pVertData,
		normals.data(),
		pIListData,
		pElsup,
		pElsup_index);

	drawObject.vertexCount = vertSize;
	drawObject.indexCount = iListSize;

	drawObject.Initialize(renderer);

	drawObject.metallicRoughnessTexture = CreateDefaultTexture(renderer, 0x00000000);
	drawObject.normalTexture = CreateDefaultTexture(renderer, 0x7F7F0000);
	{
		uint32_t checkerData[128 * 128];
		for (uint32_t i = 0; i < 128; i++)
			for (uint32_t j = 0; j < 128; j++)
				checkerData[128 * i + j] = ((i / 16 + j / 16) % 2 == 0) ? 0xFF555555 : 0xFFFFFFFF;
		drawObject.SetTexture(renderer, DrawObject::TextureType::Albedo, 128, 128, checkerData);
	}

	drawObject.WriteDescriptorSet(renderer);

	for (uint32_t i = 0; i < vertSize; i++) {
		drawObject.drawArray[i].position(0) = pVertData[i].x;
		drawObject.drawArray[i].position(1) = pVertData[i].y;
		drawObject.drawArray[i].position(2) = pVertData[i].z;
		drawObject.drawArray[i].normal(0) = normals[i].x;
		drawObject.drawArray[i].normal(1) = normals[i].y;
		drawObject.drawArray[i].normal(2) = normals[i].z;
		drawObject.drawArray[i].uv(0) = pVertData[i].x / 5.0f + 0.5f;
		drawObject.drawArray[i].uv(1) = pVertData[i].z / 5.0f + 0.5f;
		drawObject.drawArray[i].color(0) = 1.0f;
		drawObject.drawArray[i].color(1) = 1.0f;
		drawObject.drawArray[i].color(2) = 1.0f;
		drawObject.drawArray[i].color(3) = 1.0f;

		drawObject.drawArray[i].tangent(0) = 1.0f;
		drawObject.drawArray[i].tangent(1) = 0.0f;
		drawObject.drawArray[i].tangent(2) = 0.0f;
		drawObject.drawArray[i].tangent(3) = 1.0f;
		drawObject.drawArray[i].roughness = 0.5f;
	}

	for (uint32_t i = 0; i < iListSize; i++) {
		drawObject.indexdrawArray[i] = static_cast<int32_t>(pIListData[i]);
	}

	renderer.UpdateVertexArray(&drawObject.drawArray);
	renderer.UpdateVertexArray(&drawObject.indexdrawArray);
}

struct LightData {
	fvec3 lightPos;
	float lightIntensity;
	fvec3 color;
	float padding2;
	fvec3 cameraPos;
	float padding3;
	fmat4 lightPersMatrix;
	fmat4 lightCameraMatrix;
};

struct PersMatrixData {
	fmat4 cameraMatrix;
	fmat4 persMatrix;
};

struct LightPersMatrixData {
	fmat4 lightPersMatrix;
	fmat4 lightCameraMatrix;
};

int main()
{
	Renderer::InitializeParams rendererInitializeParams;
	rendererInitializeParams.isDebugMode = true;
	rendererInitializeParams.windowSize = ivec2(1280, 1280);
	rendererInitializeParams.windowName = "Renderer Test";

	Renderer renderer;
	renderer.Initialize(rendererInitializeParams);

	/////////////

	CreateRenderPass(renderer);
	CreateRenderPassForShadowMap(renderer);
	CreateGBufferRenderPass(renderer);

	//////////

	auto vertexAttribute = RegisterVertexAttribute(renderer);

	/////////

	CreateGeometryPipeline(renderer, vertexAttribute.name);
	CreateShadowMapPipeline(renderer, vertexAttribute.name);
	CreateGBufferPipeline(renderer, vertexAttribute.name);
	CreateLightingPipeline(renderer, vertexAttribute.name);

	//////////
	RootAllocator RootAllocator;
	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "vertexAllocator");
	TypeAllocator<int32_t> intAllocator(&RootAllocator, "intAllocator");

	// 描画オブジェクトを配列で管理（今後のオブジェクト追加に備える）
	std::vector<std::unique_ptr<DrawObject>> drawObjects;
	drawObjects.push_back(std::make_unique<DrawObject>(vertexAllocator, intAllocator));
	drawObjects.push_back(std::make_unique<DrawObject>(vertexAllocator, intAllocator));

	CreateBunnyObject(renderer, *drawObjects[0]);
	CreateFloorObject(renderer, *drawObjects[1]);

	// ポインタ配列（各MakeDrawParams関数に渡す用）
	std::vector<DrawObject*> drawObjectPtrs;
	for (auto& obj : drawObjects) {
		drawObjectPtrs.push_back(obj.get());
	}

	// perspective
	auto persMat = makeProjectionMatrixVk(0.01, 100.0, 0.01, -0.01, 0.01, -0.01).transpose();
	auto cameraMat = makeCameraMatrix(fvec3(0.0f, 0.0f, 0.0f), fvec3(0.0f, 2.0f, 5.0f), fvec3(0.0f, 1.0f, 0.0f)).transpose();

	auto persMatUbo = renderer.CreateGpuBuffer(sizeof(PersMatrixData), Renderer::Uniform);
	void* persMatUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(persMatUbo, &persMatUboCpuBuffer);
	PersMatrixData* persMatrixDataCpuData = reinterpret_cast<PersMatrixData*>(persMatUboCpuBuffer);
	std::memcpy(&persMatrixDataCpuData->cameraMatrix, &cameraMat, sizeof(cameraMat));
	std::memcpy(&persMatrixDataCpuData->persMatrix, &persMat, sizeof(persMat));
	renderer.UnmapCpuMemoryPointer(persMatUbo);

	// light data
	LightData lightData;
	lightData.lightPos = fvec3(5.0f, 5.0f, 0.0f);
	lightData.lightIntensity = 80.0f;
	lightData.color = fvec3(1.0f, 1.0f, 1.0f);
	lightData.cameraPos = fvec3(0.0f, 2.0f, 5.0f);

	auto lightDataUbo = renderer.CreateGpuBuffer(sizeof(LightData), Renderer::Uniform);
	void* lightDataUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(lightDataUbo, &lightDataUboCpuBuffer);
	std::memcpy(lightDataUboCpuBuffer, &lightData, sizeof(LightData));
	renderer.UnmapCpuMemoryPointer(lightDataUbo);

	// light perspective matrix for shadow map
	LightPersMatrixData lightPersMatrixData;
	lightPersMatrixData.lightCameraMatrix = makeCameraMatrix(fvec3(0.0f, 0.0f, 0.0f), lightData.lightPos, fvec3(0.0f, 1.0f, 0.0f)).transpose();
	lightPersMatrixData.lightPersMatrix = makeProjectionMatrixVk(0.1, 100.0, 45.0f, 1.0f).transpose();

	auto lightPersMatrixUbo = renderer.CreateGpuBuffer(sizeof(LightPersMatrixData), Renderer::Uniform);
	void* lightPersMatrixUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(lightPersMatrixUbo, &lightPersMatrixUboCpuBuffer);
	LightPersMatrixData* lightPersMatrixDataCpu = reinterpret_cast<LightPersMatrixData*>(lightPersMatrixUboCpuBuffer);
	std::memcpy(lightPersMatrixDataCpu, &lightPersMatrixData, sizeof(LightPersMatrixData));
	renderer.UnmapCpuMemoryPointer(lightPersMatrixUbo);

	/////

	auto descriptorSetInterface = renderer.CreateDescriptorSetInterface("testPipeline", 0);
	auto shadowMapTexture = renderer.GetRenderPassAttatchmentTexture("ShadowMapPass", Renderer::AttatchmentLabel::DepthAttachment);
	auto forwardDrawParams = MakeDrawParamsForGeometryPipeline(renderer, drawObjectPtrs, persMatUbo, lightDataUbo, descriptorSetInterface, shadowMapTexture);

	auto descriptorSetInterfaceShadow = renderer.CreateDescriptorSetInterface("shadowTestPipeline", 0);
	auto shadowDrawParams = MakeDrawParamsForShadowPipeline(renderer, drawObjectPtrs, lightPersMatrixUbo, descriptorSetInterfaceShadow);

	///// 遅延シェーディング用のDescriptor設定

	auto gBufferDrawParams = MakeDrawParamsForGBufferPipeline(renderer, drawObjectPtrs, persMatUbo);
	auto lightingDrawParams = MakeDrawParamsForLightingPipeline(renderer, lightDataUbo);

	//////

	uint32_t counter = 0;
	while (renderer.DrawCondition()) {
		renderer.DrawStart();

		void* uboCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(drawObjects[0]->uboBuffer, &uboCpuPtr);
		float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
		*uboFloatCpuPtr = counter / 50.0f;
		renderer.UnmapCpuMemoryPointer(drawObjects[0]->uboBuffer);

		void* srtMatrixCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(drawObjects[0]->srtMatrixBuffer, &srtMatrixCpuPtr);
		fmat4* srtMatrixCpu = static_cast<fmat4*>(srtMatrixCpuPtr);
		*srtMatrixCpu = fmat4::identity();
		(*srtMatrixCpu)(0, 3) = 0.0f;
		(*srtMatrixCpu)(1, 3) = 0.2f;
		(*srtMatrixCpu)(2, 3) = 0.0f;
		(*srtMatrixCpu) = (*srtMatrixCpu).transpose();
		renderer.UnmapCpuMemoryPointer(drawObjects[0]->srtMatrixBuffer);

		lightData.lightPos = fvec3(3.0 * std::sin(counter / -60.0f), 9.0f, 3.0f * std::cos(counter / -60.0f));
		lightData.color = fvec3(1.0f, 1.0f, 1.0f);


		{
			lightPersMatrixData.lightCameraMatrix = makeCameraMatrix(fvec3(0.0f, 0.0f, 0.0f), lightData.lightPos, fvec3(0.0f, 1.0f, 0.0f)).transpose();

			lightData.lightCameraMatrix = lightPersMatrixData.lightCameraMatrix;
			lightData.lightPersMatrix = lightPersMatrixData.lightPersMatrix;
		}

		renderer.GetCpuMemoryPointer(lightDataUbo, &lightDataUboCpuBuffer);
		std::memcpy(lightDataUboCpuBuffer, &lightData, sizeof(LightData));
		renderer.UnmapCpuMemoryPointer(lightDataUbo);

		renderer.GetCpuMemoryPointer(lightPersMatrixUbo, &lightPersMatrixUboCpuBuffer);
		LightPersMatrixData* lightPersMatrixDataCpu = reinterpret_cast<LightPersMatrixData*>(lightPersMatrixUboCpuBuffer);
		std::memcpy(lightPersMatrixDataCpu, &lightPersMatrixData, sizeof(LightPersMatrixData));
		renderer.UnmapCpuMemoryPointer(lightPersMatrixUbo);


		Renderer::UpdatePushConstantParams updatePushConstantParams;
		updatePushConstantParams.graphicsPipelineName = "testPipeline";
		int32_t foo = ((counter / 30) % 2 == 0) ? 1 : -1;
		updatePushConstantParams.pData = &foo;
		updatePushConstantParams.size = sizeof(int32_t);
		updatePushConstantParams.shaderStage = Renderer::ShaderStage::VertexBit | Renderer::ShaderStage::FragmentBit;
		renderer.UpdatePushConstant(updatePushConstantParams);

		//////////////////

		Renderer::BeginRenderPassParams beginClearShadowRenderPassParams{ "ClearShadowMapPass" };
		beginClearShadowRenderPassParams.clearColors.resize(1);
		beginClearShadowRenderPassParams.clearColorValues.resize(1);
		beginClearShadowRenderPassParams.clearColors[0] = Renderer::ClearDepthStancil;
		beginClearShadowRenderPassParams.clearColorValues[0].depthStencil = { 0.0, 0 };
		renderer.BeginRenderPass(beginClearShadowRenderPassParams);
		renderer.EndRenderPass();

		Renderer::BeginRenderPassParams beginShadowRenderPassParams{ "ShadowMapPass" };
		renderer.BeginRenderPass(beginShadowRenderPassParams);


		for (auto& dp : shadowDrawParams) {
			renderer.Draw(dp);
		}

		renderer.EndRenderPass();

		//////////////////
		// 遅延シェーディング描画
		//////////////////

		// G-Bufferクリア
		Renderer::BeginRenderPassParams beginClearGBufferPassParams{ "ClearGBufferPass" };
		beginClearGBufferPassParams.clearColors.resize(6);
		beginClearGBufferPassParams.clearColorValues.resize(6);
		beginClearGBufferPassParams.clearColors[0] = Renderer::ClearColor;
		beginClearGBufferPassParams.clearColorValues[0].color = fvec4{ 0.2f, 0.6f, 0.8f, 1.0f };
		beginClearGBufferPassParams.clearColors[1] = Renderer::ClearColor;
		beginClearGBufferPassParams.clearColorValues[1].color = fvec4{ 0.0f, 0.0f, 0.0f, 0.0f };
		beginClearGBufferPassParams.clearColors[2] = Renderer::ClearColor;
		beginClearGBufferPassParams.clearColorValues[2].color = fvec4{ 0.0f, 0.0f, 0.0f, 0.0f };
		beginClearGBufferPassParams.clearColors[3] = Renderer::ClearColor;
		beginClearGBufferPassParams.clearColorValues[3].color = fvec4{ 0.0f, 0.0f, 0.0f, 0.0f };
		beginClearGBufferPassParams.clearColors[4] = Renderer::ClearColor;
		beginClearGBufferPassParams.clearColorValues[4].color = fvec4{ 0.0f, 0.0f, 0.0f, 0.0f };
		beginClearGBufferPassParams.clearColors[5] = Renderer::ClearDepthStancil;
		beginClearGBufferPassParams.clearColorValues[5].depthStencil = { 0.0f, 0 };
		renderer.BeginRenderPass(beginClearGBufferPassParams);
		renderer.EndRenderPass();

		// G-Bufferパス開始（Subpass 0: G-Buffer書き込み）
		Renderer::BeginRenderPassParams beginGBufferPassParams{ "GBufferPass" };
		renderer.BeginRenderPass(beginGBufferPassParams);

		// G-Bufferに書き込み
		for (auto& dp : gBufferDrawParams) {
			renderer.Draw(dp);
		}

		// Subpass 1: Lighting
		renderer.NextSubpass();
		renderer.Draw(lightingDrawParams);

		renderer.EndRenderPass();

		////////////////////
		//// フォワードレンダリング（既存の描画も残す）
		////////////////////

		//Renderer::BeginRenderPassParams beginClearRenderPassParams{ "ClearRenderPass" };
		//beginClearRenderPassParams.clearColors.resize(2);
		//beginClearRenderPassParams.clearColorValues.resize(2);
		//beginClearRenderPassParams.clearColors[0] = Renderer::ClearColor;
		//beginClearRenderPassParams.clearColorValues[0].color = fvec4{ 0.2, 0.6, 0.8, 1.0f };
		//beginClearRenderPassParams.clearColors[1] = Renderer::ClearDepthStancil;
		//beginClearRenderPassParams.clearColorValues[1].depthStencil = { 0.0, 0 };
		//renderer.BeginRenderPass(beginClearRenderPassParams);
		//renderer.EndRenderPass();

		//Renderer::BeginRenderPassParams beginRenderPassParams{ "RenderPass" };
		//renderer.BeginRenderPass(beginRenderPassParams);

		//renderer.Draw(drawParams1);
		//renderer.Draw(drawParams2);

		//renderer.EndRenderPass();

		///////////////////

		renderer.DrawEnd();


		counter++;
	}

	return 0;
}


