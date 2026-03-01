
#include "src/utils/mathfunc/mathUtils.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/mesh/drawArray.hpp"
#include "src/utils/memory/allocator.hpp"

#include <cstring>
#include <filesystem>
#include <array>

#include "src/utils/fileloader/OBJLoader.hpp"
#include "src/utils/geometry/meshgenerator.hpp"
#include "src/utils/geometry/MeshConv.hpp"
#include "src/utils/geometry/IntOnMesh.hpp"

class DrawObject {
public:
	Renderer::DescriptorSetInterface descriptorSetInterface;
	Renderer::DescriptorSetInterface descriptorSetInterfaceForShadow;
	DrawVertexArray<BasicVertex> drawArray;
	DrawVertexArray<int32_t> indexdrawArray;

	Renderer::GpuBuffer uboBuffer;
	Renderer::GpuBuffer srtMatrixBuffer;
	Renderer::GpuTexture textureMemory;

	fmat4 srtMatrix;

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
		srtMatrixBuffer = renderer.CreateGpuBuffer(sizeof(fmat4), Renderer::Uniform);
		void* srtMatrixCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(srtMatrixBuffer, &srtMatrixCpuPtr);
		std::memcpy(srtMatrixCpuPtr, srtMatrix.cmp, sizeof(fmat4));
		renderer.UnmapCpuMemoryPointer(srtMatrixBuffer);

		textureMemory;
		Renderer::CreateImageParams createImageParams;
		createImageParams.width = 128;
		createImageParams.height = 128;
		createImageParams.format = Renderer::ImageFormat::RGBA8_SNORM;
		textureMemory = renderer.CreateGpuTexture(createImageParams);

		Renderer::GpuBuffer stagingBuffer = renderer.CreateGpuBuffer(128 * 128 * 4, Renderer::Transfer);
		uint32_t* stagingBufferCpu = nullptr;
		renderer.GetCpuMemoryPointer(stagingBuffer, (void**)&stagingBufferCpu);
		for (uint32_t i = 0; i < 128; i++) {
			for (uint32_t j = 0; j < 128; j++) {
				if ((i / 16 + j / 16) % 2 == 0) {
					stagingBufferCpu[128 * i + j] = 0xFF555555;
				}
				else {
					stagingBufferCpu[128 * i + j] = 0xFFFFFFFF;
				}
			}
		}
		renderer.UnmapCpuMemoryPointer(stagingBuffer);
		renderer.TransferStagingBufferToImage(stagingBuffer, textureMemory);

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
		Renderer::DescriptorWriterParams::DescriptorInfo textureDescriptorInfo;
		textureDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
		textureDescriptorInfo.bindingNum = 2;
		textureDescriptorInfo.count = 1;
		textureDescriptorInfo.pResources.resize(1);
		textureDescriptorInfo.pResources[0] = textureMemory.pGpuTextureMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(textureDescriptorInfo);

		Renderer::DescriptorWriterParams::DescriptorInfo srtMatrixDescriptorInfoForShadow;
		srtMatrixDescriptorInfoForShadow.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		srtMatrixDescriptorInfoForShadow.bindingNum = 0;
		srtMatrixDescriptorInfoForShadow.count = 1;
		srtMatrixDescriptorInfoForShadow.pResources.resize(1);
		srtMatrixDescriptorInfoForShadow.pResources[0] = srtMatrixBuffer.pGpuMemoryImpl;
		descriptorWriterParamsForShadow.descriptorInfos.push_back(srtMatrixDescriptorInfoForShadow);

	}

	void WriteDescriptorSet(Renderer& renderer)
	{
		renderer.WriteDescriptorSet(descriptorWriterParams, descriptorSetInterface);
		renderer.WriteDescriptorSet(descriptorWriterParamsForShadow, descriptorSetInterfaceForShadow);
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
			true,
			false,
			false,
		},
		{ Renderer::ImageFormat::DEPTH32_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false,
			true,
			false
		}
	};
	renderPassParams.subpasses = {
		{
			{
			  0,
			},
			{
			},
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
			true,
			false,
			false
		},
		{ Renderer::ImageFormat::DEPTH32_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false,
			true,
			false,
		}
	};
	clearRenderPassParams.subpasses = {
		{
			{
			  0,
			},
			{
			},
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
		{ // 0
			Renderer::ImageFormat::SameAsSwapChain,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::PresentSrcKHR,
			Renderer::UseSwapChainAttachment,
			true,
			false,
			false,
		},
		{ // 1
			Renderer::ImageFormat::RGBA8_SNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::AlbedoAttachment,
			true,
			false,
			true,
		},
		{ // 2
			Renderer::ImageFormat::RGBA32_SNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::NormalAttachment,
			true,
			false,
			true,
		},
		{ // 3
			Renderer::ImageFormat::RGBA32_SNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::PositionAttachment,
			true,
			false,
			true,
		},
		{ // 4
			Renderer::ImageFormat::RGBA32_SNORM,
			false,
			true,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::ImageLayout::ShaderReadOnlyOptimal,
			Renderer::MetallicRoughnessAttachment,
			true,
			false,
			true,
		},
		{ // 5
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			false,
			true,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false,
			true,
			false,
		}
	};
	renderPassParams.subpasses = {
		{
		  {
			  1,2,3,4
		  },
		  {
		  },
		  5
		},
		{
		  {
			  0
		  },
		  {
			  1,2,3,4
		  },
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
			true,
			false,
			false,
		},
		{ // 1
			Renderer::ImageFormat::RGBA8_SNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::AlbedoAttachment,
			true,
			false,
			false,
		},
		{ // 2
			Renderer::ImageFormat::RGBA32_SNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::NormalAttachment,
			true,
			false,
			false,
		},
		{ // 3
			Renderer::ImageFormat::RGBA32_SNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::PositionAttachment,
			true,
			false,
			false,
		},
		{ // 4
			Renderer::ImageFormat::RGBA32_SNORM,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::ColorAttachmentOptimal,
			Renderer::MetallicRoughnessAttachment,
			true,
			false,
			false,
		},
		{ // 5
			Renderer::ImageFormat::DEPTH32_SFLOAT,
			true,
			true,
			Renderer::ImageLayout::Undefined,
			Renderer::ImageLayout::DepthStencilAttachmentOptimal,
			Renderer::DepthAttachment,
			false,
			true,
			false,
		}
	};
	clearRenderPassParams.subpasses = {
		{
			{
			  0,1,2,3,4
			},
			{

			},
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
			false,
			true,
			false
		}
	};
	renderPassParams.subpasses = {
		{
			{
			},
			{
			},
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
			false,
			true,
			false
		},
	};
	clearRenderPassParams.subpasses = {
		{
			{
			},
			{
			},
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

void CreateGeometryPipeline(Renderer& renderer, std::string vertexAttributeName)
{

	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = "ShaderBinary/test.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = "ShaderBinary/test.frag.spv";

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

void CreateShadowMapPipeline(Renderer& renderer, std::string vertexAttributeName)
{
	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = "ShaderBinary/shadow.vert.spv";

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

auto MakeDrawParamsForGeometryPipeline(Renderer& renderer, DrawObject drawObject1, DrawObject drawObject2, Renderer::GpuBuffer& persMatUbo, Renderer::GpuBuffer& lightDataUbo, Renderer::DescriptorSetInterface descriptorSetInterface, Renderer::GpuTexture& shadowMapTexture)
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

	Renderer::DrawParams drawParams1;
	drawParams1.pVertexArray = drawObject1.drawArray.getGpuMemoryImpl();
	drawParams1.instanceCount = 1;
	drawParams1.pIndexArray = drawObject1.indexdrawArray.getGpuMemoryImpl();
	drawParams1.count = drawObject1.indexdrawArray.size();
	drawParams1.descriptorSetInterfaces.push_back(drawObject1.descriptorSetInterface);
	drawParams1.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams1.graphicsPipelineName = "testPipeline";

	Renderer::DrawParams drawParams2;
	drawParams2.pVertexArray = drawObject2.drawArray.getGpuMemoryImpl();
	drawParams2.instanceCount = 1;
	drawParams2.pIndexArray = drawObject2.indexdrawArray.getGpuMemoryImpl();
	drawParams2.count = drawObject2.indexdrawArray.size();
	drawParams2.descriptorSetInterfaces.push_back(drawObject2.descriptorSetInterface);
	drawParams2.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams2.graphicsPipelineName = "testPipeline";

	return std::array<Renderer::DrawParams, 2> { drawParams1, drawParams2 };
}

auto MakeDrawParamsForShadowPipeline(Renderer& renderer, DrawObject drawObject1, DrawObject drawObject2, Renderer::GpuBuffer& lightPersMatrixUbo, Renderer::DescriptorSetInterface descriptorSetInterface)
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

	Renderer::DrawParams drawParams1;
	drawParams1.pVertexArray = drawObject1.drawArray.getGpuMemoryImpl();
	drawParams1.instanceCount = 1;
	drawParams1.pIndexArray = drawObject1.indexdrawArray.getGpuMemoryImpl();
	drawParams1.count = drawObject1.indexdrawArray.size();
	drawParams1.descriptorSetInterfaces.push_back(drawObject1.descriptorSetInterfaceForShadow);
	drawParams1.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams1.graphicsPipelineName = "shadowTestPipeline";

	Renderer::DrawParams drawParams2;
	drawParams2.pVertexArray = drawObject2.drawArray.getGpuMemoryImpl();
	drawParams2.instanceCount = 1;
	drawParams2.pIndexArray = drawObject2.indexdrawArray.getGpuMemoryImpl();
	drawParams2.count = drawObject2.indexdrawArray.size();
	drawParams2.descriptorSetInterfaces.push_back(drawObject2.descriptorSetInterfaceForShadow);
	drawParams2.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams2.graphicsPipelineName = "shadowTestPipeline";

	return std::array<Renderer::DrawParams, 2> { drawParams1, drawParams2 };
}

void CreateRenderObject(Renderer& renderer, DrawObject& drawObject1, DrawObject& drawObject2)
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

	drawObject1.vertexCount = static_cast<uint32_t>(positions.size());
	drawObject1.indexCount = static_cast<uint32_t>(faceindices.size());
	drawObject1.Initialize(renderer);

	drawObject1.WriteDescriptorSet(renderer);

	for (uint32_t i = 0; i < positions.size(); i++) {
		drawObject1.drawArray[i].position(0) = positions[i].x;
		drawObject1.drawArray[i].position(1) = positions[i].y;
		drawObject1.drawArray[i].position(2) = positions[i].z;
		drawObject1.drawArray[i].normal(0) = normals[i].x;
		drawObject1.drawArray[i].normal(1) = normals[i].y;
		drawObject1.drawArray[i].normal(2) = normals[i].z;
		drawObject1.drawArray[i].uv(0) = uvs[i].x;
		drawObject1.drawArray[i].uv(1) = uvs[i].y;

		drawObject1.drawArray[i].color(0) = 1.0f;
		drawObject1.drawArray[i].color(1) = 1.0f;
		drawObject1.drawArray[i].color(2) = 1.0f;
		drawObject1.drawArray[i].color(3) = 1.0f;
	}

	for (uint32_t i = 0; i < faceindices.size(); i++) {
		drawObject1.indexdrawArray[i] = static_cast<int32_t>(faceindices[i]);
	}

	renderer.UpdateVertexArray(&drawObject1.drawArray);
	renderer.UpdateVertexArray(&drawObject1.indexdrawArray);

	/////

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
	for (int i = 0; i < vertSize; i++) {
		pVertData[i] = fvec3(pVertData2d[i].x, 0.0f, -pVertData2d[i].y);
	}

	normals = std::vector<fvec3>(vertSize, fvec3::zero());
	ComputeNormalFromVE(
		vertSize,
		pVertData,
		normals.data(),
		pIListData,
		pElsup,
		pElsup_index);

	drawObject2.vertexCount = vertSize;
	drawObject2.indexCount = iListSize;

	drawObject2.Initialize(renderer);
	drawObject2.WriteDescriptorSet(renderer);

	for (int i = 0; i < vertSize; i++) {
		drawObject2.drawArray[i].position(0) = pVertData[i].x;
		drawObject2.drawArray[i].position(1) = pVertData[i].y;
		drawObject2.drawArray[i].position(2) = pVertData[i].z;
		drawObject2.drawArray[i].normal(0) = normals[i].x;
		drawObject2.drawArray[i].normal(1) = normals[i].y;
		drawObject2.drawArray[i].normal(2) = normals[i].z;
		drawObject2.drawArray[i].uv(0) = pVertData[i].x / 5.0f + 0.5f;
		drawObject2.drawArray[i].uv(1) = pVertData[i].z / 5.0f + 0.5f;
		drawObject2.drawArray[i].color(0) = 1.0f;
		drawObject2.drawArray[i].color(1) = 1.0f;
		drawObject2.drawArray[i].color(2) = 1.0f;
		drawObject2.drawArray[i].color(3) = 1.0f;
	}

	for (uint32_t i = 0; i < iListSize; i++) {
		drawObject2.indexdrawArray[i] = static_cast<int32_t>(pIListData[i]);
	}

	renderer.UpdateVertexArray(&drawObject2.drawArray);
	renderer.UpdateVertexArray(&drawObject2.indexdrawArray);
}

struct LightData {
	fvec3 lightPos;
	float padding1;
	fvec3 color;
	float padding2;
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

	//////////
	RootAllocator RootAllocator;
	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "vertexAllocator");
	TypeAllocator<int32_t> intAllocator(&RootAllocator, "intAllocator");
	DrawObject drawObject1(vertexAllocator, intAllocator);
	DrawObject drawObject2(vertexAllocator, intAllocator);

	CreateRenderObject(renderer, drawObject1, drawObject2);

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
	lightData.color = fvec3(1.0f, 1.0f, 1.0f);

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
	auto [drawParams1, drawParams2] = MakeDrawParamsForGeometryPipeline(renderer, drawObject1, drawObject2, persMatUbo, lightDataUbo, descriptorSetInterface, shadowMapTexture);

	auto descriptorSetInterfaceShadow = renderer.CreateDescriptorSetInterface("shadowTestPipeline", 0);
	auto [drawShadowParams1, drawShadowParams2] = MakeDrawParamsForShadowPipeline(renderer, drawObject1, drawObject2, lightPersMatrixUbo, descriptorSetInterfaceShadow);

	//////

	uint32_t counter = 0;
	while (renderer.DrawCondition()) {
		renderer.DrawStart();

		void* uboCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(drawObject1.uboBuffer, &uboCpuPtr);
		float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
		//*uboFloatCpuPtr = std::sin(counter / 50.0f) * 0.6f;
		*uboFloatCpuPtr = counter / 50.0f;
		renderer.UnmapCpuMemoryPointer(drawObject1.uboBuffer);

		void* srtMatrixCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(drawObject1.srtMatrixBuffer, &srtMatrixCpuPtr);
		fmat4* srtMatrixCpu = static_cast<fmat4*>(srtMatrixCpuPtr);
		*srtMatrixCpu = fmat4::identity();
		(*srtMatrixCpu)(0, 3) = 0.0f;
		(*srtMatrixCpu)(1, 3) = 0.2f;
		(*srtMatrixCpu)(2, 3) = 0.0f;// std::cos(counter / 50.0f) * 1.0f;
		(*srtMatrixCpu) = (*srtMatrixCpu).transpose();
		renderer.UnmapCpuMemoryPointer(drawObject1.srtMatrixBuffer);

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
		beginClearShadowRenderPassParams.clearColorValues[0].depthStencil = {0.0, 0};
		renderer.BeginRenderPass(beginClearShadowRenderPassParams);
		renderer.EndRenderPass();

		Renderer::BeginRenderPassParams beginShadowRenderPassParams{ "ShadowMapPass" };
		renderer.BeginRenderPass(beginShadowRenderPassParams);


		renderer.Draw(drawShadowParams1);
		renderer.Draw(drawShadowParams2);

		renderer.EndRenderPass();

		//////////////////

		Renderer::BeginRenderPassParams beginClearRenderPassParams{ "ClearRenderPass" };
		beginClearRenderPassParams.clearColors.resize(2);
		beginClearRenderPassParams.clearColorValues.resize(2);
		beginClearRenderPassParams.clearColors[0] = Renderer::ClearColor;
		beginClearRenderPassParams.clearColorValues[0].color = fvec4{ 0.2, 0.6, 0.8, 1.0f };
		beginClearRenderPassParams.clearColors[1] = Renderer::ClearDepthStancil;
		beginClearRenderPassParams.clearColorValues[1].depthStencil = { 0.0, 0 };
		renderer.BeginRenderPass(beginClearRenderPassParams);
		renderer.EndRenderPass();

		Renderer::BeginRenderPassParams beginRenderPassParams{ "RenderPass" };
		renderer.BeginRenderPass(beginRenderPassParams);

		renderer.Draw(drawParams1);
		renderer.Draw(drawParams2);

		renderer.EndRenderPass();

		///////////////////

		renderer.DrawEnd();


		counter++;
	}

	return 0;
}
