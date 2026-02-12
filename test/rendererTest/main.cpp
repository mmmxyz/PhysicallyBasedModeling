
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
	DrawVertexArray<BasicVertex> drawArray;
	DrawVertexArray<int32_t> indexdrawArray;

	Renderer::GpuBuffer uboBuffer;
	Renderer::GpuTexture textureMemory;

	Renderer::DescriptorWriterParams descriptorWriterParams;

	uint32_t vertexCount = 0;
	uint32_t indexCount  = 0;

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

		uboBuffer	= renderer.CreateGpuBuffer(32, Renderer::Uniform);
		void* uboCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(uboBuffer, &uboCpuPtr);
		float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
		*uboFloatCpuPtr	      = 0.0f;
		renderer.UnmapCpuMemoryPointer(uboBuffer);

		textureMemory;
		textureMemory = renderer.CreateGpuTexture(128, 128, Renderer::ImageFormat::RGBA8_SNORM);

		Renderer::GpuBuffer stagingBuffer = renderer.CreateGpuBuffer(128 * 128 * 4, Renderer::Transfer);
		uint32_t* stagingBufferCpu	  = nullptr;
		renderer.GetCpuMemoryPointer(stagingBuffer, (void**)&stagingBufferCpu);
		for (uint32_t i = 0; i < 128; i++) {
			for (uint32_t j = 0; j < 128; j++) {
				if ((i / 16 + j / 16) % 2 == 0) {
					stagingBufferCpu[128 * i + j] = 0xFF555555;
				} else {
					stagingBufferCpu[128 * i + j] = 0xFFFFFFFF;
				}
			}
		}
		renderer.UnmapCpuMemoryPointer(stagingBuffer);
		renderer.TransferStagingBufferToImage(stagingBuffer, textureMemory);

		descriptorSetInterface = renderer.CreateDescriptorSetInterface("testPipeline", 1);

		Renderer::DescriptorWriterParams::DescriptorInfo uboDescriptorInfo;
		uboDescriptorInfo.type	     = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		uboDescriptorInfo.bindingNum = 0;
		uboDescriptorInfo.count	     = 1;
		uboDescriptorInfo.pResources.resize(1);
		uboDescriptorInfo.pResources[0] = uboBuffer.pGpuMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(uboDescriptorInfo);
		Renderer::DescriptorWriterParams::DescriptorInfo textureDescriptorInfo;
		textureDescriptorInfo.type	 = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
		textureDescriptorInfo.bindingNum = 1;
		textureDescriptorInfo.count	 = 1;
		textureDescriptorInfo.pResources.resize(1);
		textureDescriptorInfo.pResources[0] = textureMemory.pGpuTextureMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(textureDescriptorInfo);
	}

	void WriteDescriptorSet(Renderer& renderer)
	{
		renderer.WriteDescriptorSet(descriptorWriterParams, descriptorSetInterface);
	}
};

void CreateRenderPass(Renderer& renderer)
{
	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name	     = "RenderPass";
	renderPassParams.attachments = {
		{
		    Renderer::ImageFormat::SameAsSwapChain,
		    false,
		    true,
		    Renderer::ImageLayout::ColorAttachmentOptimal,
		    Renderer::ImageLayout::PresentSrcKHR,
		    Renderer::UseSwapChainAttachment,
		},
		{ Renderer::ImageFormat::DEPTH32_SFLOAT,
		    false,
		    true,
		    Renderer::ImageLayout::DepthStencilAttachmentOptimal,
		    Renderer::ImageLayout::DepthStencilAttachmentOptimal,
		    Renderer::DepthAttachment }
	};
	renderPassParams.subpasses = {
		{ {
		      0,
		  },
		    {

		    },
		    1 }
	};
	renderPassParams.dependencies = {
		{ -1, 0 },
	};

	renderer.CreateRenderPass(renderPassParams);

	Renderer::RenderPassParams clearRenderPassParams;
	clearRenderPassParams.name	  = "ClearRenderPass";
	clearRenderPassParams.attachments = {
		{
		    Renderer::ImageFormat::SameAsSwapChain,
		    true,
		    true,
		    Renderer::ImageLayout::Undefined,
		    Renderer::ImageLayout::ColorAttachmentOptimal,
		    Renderer::UseSwapChainAttachment,
		},
		{ Renderer::ImageFormat::DEPTH32_SFLOAT,
		    true,
		    true,
		    Renderer::ImageLayout::Undefined,
		    Renderer::ImageLayout::DepthStencilAttachmentOptimal,
		    Renderer::DepthAttachment }
	};
	clearRenderPassParams.subpasses = {
		{ {
		      0,
		  },
		    {

		    },
		    1 }
	};
	clearRenderPassParams.dependencies = {
		{ -1, 0 },
	};
	clearRenderPassParams.isClearRenderPass	  = true;
	clearRenderPassParams.clearRenderPassName = "RenderPass";

	renderer.CreateRenderPass(clearRenderPassParams);
}

void CreateRenderPassForShadowMap(Renderer& renderer)
{
	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name	     = "ShadowMapPass";
	renderPassParams.attachments = {
		{
		    Renderer::ImageFormat::DEPTH32_SFLOAT,
		    false,
		    true,
		    Renderer::ImageLayout::DepthStencilAttachmentOptimal,
		    Renderer::ImageLayout::ShaderReadOnlyOptimal,
		    Renderer::DepthAttachment,
		}
	};
	renderPassParams.subpasses = {
		{ {},
		    {

		    },
		    0 }
	};
	renderPassParams.dependencies = {
		{ -1, 0 },
	};

	renderer.CreateRenderPass(renderPassParams);

	Renderer::RenderPassParams clearRenderPassParams;
	clearRenderPassParams.name	  = "ClearShadowMapPass";
	clearRenderPassParams.attachments = {
		{
		    Renderer::ImageFormat::DEPTH32_SFLOAT,
		    true,
		    true,
		    Renderer::ImageLayout::Undefined,
		    Renderer::ImageLayout::DepthStencilAttachmentOptimal,
		    Renderer::DepthAttachment,
		},
	};
	clearRenderPassParams.subpasses = {
		{ {},
		    {},
		    1 }
	};
	clearRenderPassParams.dependencies = {
		{ -1, 0 },
	};
	clearRenderPassParams.isClearRenderPass	  = true;
	clearRenderPassParams.clearRenderPassName = "ShadowMapPass";

	renderer.CreateRenderPass(clearRenderPassParams);
}

void CreateGeometryPipeline(Renderer& renderer)
{

	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType  = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = "ShaderBinary/test.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType  = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = "ShaderBinary/test.frag.spv";

	graphicsPipelineParams.name = "testPipeline";
	graphicsPipelineParams.shaders.resize(2);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;
	graphicsPipelineParams.shaders[1] = &fragmentShaderStage;

	// kokoha bunri dekiru
	Renderer::VertexAttributeLayout vertexAttributeLayout;
	Renderer::CreateVertexAttributeLayout2<BasicVertex>(&vertexAttributeLayout);
	renderer.RegisterVertexInputStateImpl3(&vertexAttributeLayout);

	graphicsPipelineParams.vertexLayoutName = vertexAttributeLayout.name;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout;
	Renderer::DescriptorSetBindingParams persMatUboDescriptorSetLayout;
	persMatUboDescriptorSetLayout.bindingNum  = 0;
	persMatUboDescriptorSetLayout.type	  = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	persMatUboDescriptorSetLayout.count	  = 1;
	persMatUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&persMatUboDescriptorSetLayout);
	Renderer::DescriptorSetBindingParams lightUboDescriptorSetLayout;
	lightUboDescriptorSetLayout.bindingNum	= 1;
	lightUboDescriptorSetLayout.type	= Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	lightUboDescriptorSetLayout.count	= 1;
	lightUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&lightUboDescriptorSetLayout);
	descriptorSetLayout.isBindless = false;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout2;
	Renderer::DescriptorSetBindingParams uboDescriptorSetLayout;
	uboDescriptorSetLayout.bindingNum  = 0;
	uboDescriptorSetLayout.type	   = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	uboDescriptorSetLayout.count	   = 1;
	uboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&uboDescriptorSetLayout);

	Renderer::DescriptorSetBindingParams textureDescriptorSetLayout;
	textureDescriptorSetLayout.bindingNum  = 1;
	textureDescriptorSetLayout.type	       = Renderer::DescriptorSetBindingParams::Texture_bit;
	textureDescriptorSetLayout.count       = 1;
	textureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&textureDescriptorSetLayout);
	descriptorSetLayout2.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout2);

	graphicsPipelineParams.renderPassName	= "RenderPass";
	graphicsPipelineParams.subpassIndex	= 0;
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

void CreateShadowMapPipeline(Renderer& renderer)
{

	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType  = Renderer::ShaderStageVertex;
	vertexShaderStage.shaderPath = "ShaderBinary/test.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType  = Renderer::ShaderStageFragment;
	fragmentShaderStage.shaderPath = "ShaderBinary/test.frag.spv";

	graphicsPipelineParams.name = "testPipeline";
	graphicsPipelineParams.shaders.resize(2);
	graphicsPipelineParams.shaders[0] = &vertexShaderStage;
	graphicsPipelineParams.shaders[1] = &fragmentShaderStage;

	Renderer::VertexAttributeLayout vertexAttributeLayout;
	Renderer::CreateVertexAttributeLayout2<BasicVertex>(&vertexAttributeLayout);
	renderer.RegisterVertexInputStateImpl3(&vertexAttributeLayout);

	graphicsPipelineParams.vertexLayoutName = vertexAttributeLayout.name;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout;
	Renderer::DescriptorSetBindingParams persMatUboDescriptorSetLayout;
	persMatUboDescriptorSetLayout.bindingNum  = 0;
	persMatUboDescriptorSetLayout.type	  = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	persMatUboDescriptorSetLayout.count	  = 1;
	persMatUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&persMatUboDescriptorSetLayout);
	Renderer::DescriptorSetBindingParams lightUboDescriptorSetLayout;
	lightUboDescriptorSetLayout.bindingNum	= 1;
	lightUboDescriptorSetLayout.type	= Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	lightUboDescriptorSetLayout.count	= 1;
	lightUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&lightUboDescriptorSetLayout);
	descriptorSetLayout.isBindless = false;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout2;
	Renderer::DescriptorSetBindingParams uboDescriptorSetLayout;
	uboDescriptorSetLayout.bindingNum  = 0;
	uboDescriptorSetLayout.type	   = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	uboDescriptorSetLayout.count	   = 1;
	uboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&uboDescriptorSetLayout);

	Renderer::DescriptorSetBindingParams textureDescriptorSetLayout;
	textureDescriptorSetLayout.bindingNum  = 1;
	textureDescriptorSetLayout.type	       = Renderer::DescriptorSetBindingParams::Texture_bit;
	textureDescriptorSetLayout.count       = 1;
	textureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&textureDescriptorSetLayout);
	descriptorSetLayout2.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout2);

	graphicsPipelineParams.renderPassName	= "RenderPass";
	graphicsPipelineParams.subpassIndex	= 0;
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);
}

auto MakeDrawParamsForGeometryPipeline(Renderer& renderer, DrawObject drawObject1, DrawObject drawObject2, Renderer::GpuBuffer& persMatUbo, Renderer::GpuBuffer& lightDataUbo, Renderer::DescriptorSetInterface descriptorSetInterface)
{

	Renderer::DescriptorWriterParams persAndLightUboDescriptorWriterParams;
	Renderer::DescriptorWriterParams::DescriptorInfo persUboDescriptorWriteParams;
	persUboDescriptorWriteParams.type	= Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	persUboDescriptorWriteParams.bindingNum = 0;
	persUboDescriptorWriteParams.count	= 1;
	persUboDescriptorWriteParams.pResources.resize(1);
	persUboDescriptorWriteParams.pResources[0] = persMatUbo.pGpuMemoryImpl;
	persAndLightUboDescriptorWriterParams.descriptorInfos.push_back(persUboDescriptorWriteParams);
	Renderer::DescriptorWriterParams::DescriptorInfo lightDataUboDescriptorWriteParams;
	lightDataUboDescriptorWriteParams.type	     = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	lightDataUboDescriptorWriteParams.bindingNum = 1;
	lightDataUboDescriptorWriteParams.count	     = 1;
	lightDataUboDescriptorWriteParams.pResources.resize(1);
	lightDataUboDescriptorWriteParams.pResources[0] = lightDataUbo.pGpuMemoryImpl;
	persAndLightUboDescriptorWriterParams.descriptorInfos.push_back(lightDataUboDescriptorWriteParams);

	renderer.WriteDescriptorSet(persAndLightUboDescriptorWriterParams, descriptorSetInterface);

	Renderer::DrawParams drawParams1;
	drawParams1.pVertexArray  = drawObject1.drawArray.getGpuMemoryImpl();
	drawParams1.instanceCount = 1;
	drawParams1.pIndexArray	  = drawObject1.indexdrawArray.getGpuMemoryImpl();
	drawParams1.count	  = drawObject1.indexdrawArray.size();
	drawParams1.descriptorSetInterfaces.push_back(drawObject1.descriptorSetInterface);
	drawParams1.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams1.graphicsPipelineName = "testPipeline";

	Renderer::DrawParams drawParams2;
	drawParams2.pVertexArray  = drawObject2.drawArray.getGpuMemoryImpl();
	drawParams2.instanceCount = 1;
	drawParams2.pIndexArray	  = drawObject2.indexdrawArray.getGpuMemoryImpl();
	drawParams2.count	  = drawObject2.indexdrawArray.size();
	drawParams2.descriptorSetInterfaces.push_back(drawObject2.descriptorSetInterface);
	drawParams2.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams2.graphicsPipelineName = "testPipeline";

	return std::array<Renderer::DrawParams, 2> { drawParams1, drawParams2 };
}

void CreateRenderObject(Renderer& renderer, DrawObject& drawObject1, DrawObject& drawObject2)
{
	std::vector<fvec3> positions;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	std::vector<uint32_t> faceindices;

	constexpr auto resourcePath = RESOURCE_DIR "/Bunny.obj";

	LoadOBJtoRenderTriangleMesh(
	    resourcePath,
	    positions,
	    normals,
	    uvs,
	    faceindices,
	    fvec3(0.0f, 0.0f, 0.0f),
	    0.5f);

	drawObject1.vertexCount = static_cast<uint32_t>(positions.size());
	drawObject1.indexCount	= static_cast<uint32_t>(faceindices.size());
	drawObject1.Initialize(renderer);

	drawObject1.WriteDescriptorSet(renderer);

	for (uint32_t i = 0; i < positions.size(); i++) {
		drawObject1.drawArray[i].position(0) = positions[i].x;
		drawObject1.drawArray[i].position(1) = positions[i].y;
		drawObject1.drawArray[i].position(2) = positions[i].z;
		drawObject1.drawArray[i].normal(0)   = normals[i].x;
		drawObject1.drawArray[i].normal(1)   = normals[i].y;
		drawObject1.drawArray[i].normal(2)   = normals[i].z;
		drawObject1.drawArray[i].uv(0)	     = uvs[i].x;
		drawObject1.drawArray[i].uv(1)	     = uvs[i].y;

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
	    5,
	    5,
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
	uint32_t* pElsup       = nullptr;
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
	drawObject2.indexCount	= iListSize;

	drawObject2.Initialize(renderer);
	drawObject2.WriteDescriptorSet(renderer);

	for (int i = 0; i < vertSize; i++) {
		drawObject2.drawArray[i].position(0) = pVertData[i].x;
		drawObject2.drawArray[i].position(1) = pVertData[i].y;
		drawObject2.drawArray[i].position(2) = pVertData[i].z;
		drawObject2.drawArray[i].normal(0)   = normals[i].x;
		drawObject2.drawArray[i].normal(1)   = normals[i].y;
		drawObject2.drawArray[i].normal(2)   = normals[i].z;
		drawObject2.drawArray[i].uv(0)	     = pVertData[i].x / 5.0f + 0.5f;
		drawObject2.drawArray[i].uv(1)	     = pVertData[i].z / 5.0f + 0.5f;
		drawObject2.drawArray[i].color(0)    = 1.0f;
		drawObject2.drawArray[i].color(1)    = 1.0f;
		drawObject2.drawArray[i].color(2)    = 1.0f;
		drawObject2.drawArray[i].color(3)    = 1.0f;
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
};

int main()
{
	Renderer::InitializeParams rendererInitializeParams;
	rendererInitializeParams.isDebugMode = true;
	rendererInitializeParams.windowSize  = ivec2(1280, 1280);
	rendererInitializeParams.windowName  = "Renderer Test";

	Renderer renderer;
	renderer.Initialize(rendererInitializeParams);

	/////////////

	CreateRenderPass(renderer);
	CreateRenderPassForShadowMap(renderer);

	//////////

	CreateGeometryPipeline(renderer);
	CreateShadowMapPipeline(renderer);

	//////////
	RootAllocator RootAllocator;
	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "vertexAllocator");
	TypeAllocator<int32_t> intAllocator(&RootAllocator, "intAllocator");
	DrawObject drawObject1(vertexAllocator, intAllocator);
	DrawObject drawObject2(vertexAllocator, intAllocator);

	CreateRenderObject(renderer, drawObject1, drawObject2);

	// perspective
	auto persMat = makeProjectionMatrixVk(0.01, 100.0, 0.01, -0.01, 0.01, -0.01).transpose();

	auto persMatUbo		  = renderer.CreateGpuBuffer(sizeof(persMat), Renderer::Uniform);
	void* persMatUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(persMatUbo, &persMatUboCpuBuffer);
	std::memcpy(persMatUboCpuBuffer, &persMat, sizeof(persMat));
	renderer.UnmapCpuMemoryPointer(persMatUbo);

	// light data
	LightData lightData;
	lightData.lightPos = fvec3(5.0f, 5.0f, 0.0f);
	lightData.color	   = fvec3(1.0f, 1.0f, 1.0f);

	auto lightDataUbo	    = renderer.CreateGpuBuffer(sizeof(LightData), Renderer::Uniform);
	void* lightDataUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(lightDataUbo, &lightDataUboCpuBuffer);
	std::memcpy(lightDataUboCpuBuffer, &lightData, sizeof(LightData));
	renderer.UnmapCpuMemoryPointer(lightDataUbo);

	/////

	auto descriptorSetInterface	= renderer.CreateDescriptorSetInterface("testPipeline", 0);
	auto [drawParams1, drawParams2] = MakeDrawParamsForGeometryPipeline(renderer, drawObject1, drawObject2, persMatUbo, lightDataUbo, descriptorSetInterface);

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

		lightData.lightPos = fvec3(6.0 * std::sin(counter / -20.0f), 5.0f, 6.0f * std::cos(counter / -20.0f));
		lightData.color	   = fvec3(1.0f, 1.0f, 1.0f);

		renderer.GetCpuMemoryPointer(lightDataUbo, &lightDataUboCpuBuffer);
		std::memcpy(lightDataUboCpuBuffer, &lightData, sizeof(LightData));
		renderer.UnmapCpuMemoryPointer(lightDataUbo);

		Renderer::UpdatePushConstantParams updatePushConstantParams;
		updatePushConstantParams.graphicsPipelineName = "testPipeline";

		int32_t foo = ((counter / 30) % 2 == 0) ? 1 : -1;

		updatePushConstantParams.pData	     = &foo;
		updatePushConstantParams.size	     = sizeof(int32_t);
		updatePushConstantParams.shaderStage = Renderer::ShaderStage::VertexBit | Renderer::ShaderStage::FragmentBit;

		renderer.UpdatePushConstant(updatePushConstantParams);

		Renderer::BeginRenderPassParams beginClearRenderPassParams { "ClearRenderPass" };
		renderer.BeginRenderPass(beginClearRenderPassParams);
		renderer.EndRenderPass();

		Renderer::BeginRenderPassParams beginRenderPassParams { "RenderPass" };
		renderer.BeginRenderPass(beginRenderPassParams);

		renderer.Draw(drawParams1);
		renderer.Draw(drawParams2);

		renderer.EndRenderPass();

		renderer.DrawEnd();

		counter++;
	}

	return 0;
}
