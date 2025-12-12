
#include "src/utils/mathfunc/mathUtils.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/mesh/drawArray.hpp"
#include "src/utils/memory/allocator.hpp"

#include <filesystem>

#include <fstream>

int main()
{
	Renderer::InitializeParams rendererInitializeParams;
	rendererInitializeParams.isDebugMode = true;
	rendererInitializeParams.windowSize  = ivec2(1280, 720);
	rendererInitializeParams.windowName  = "Renderer Test";

	Renderer renderer;
	renderer.Initialize(rendererInitializeParams);

	Renderer::GraphicsPipelineParams graphicsPipelineParams;

	Renderer::ShaderStageParams vertexShaderStage;
	vertexShaderStage.stageType  = Renderer::ShaderStageParams::Vertex;
	vertexShaderStage.shaderPath = "ShaderBinary/test.vert.spv";

	Renderer::ShaderStageParams fragmentShaderStage;
	fragmentShaderStage.stageType  = Renderer::ShaderStageParams::Fragment;
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
	Renderer::DescriptorSetBindingParams uboDescriptorSetLayout;
	uboDescriptorSetLayout.bindingNum  = 0;
	uboDescriptorSetLayout.type	   = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	uboDescriptorSetLayout.count	   = 1;
	uboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;

	descriptorSetLayout.descriptorSetBindingParams.push_back(&uboDescriptorSetLayout);

	Renderer::DescriptorSetBindingParams textureDescriptorSetLayout;
	textureDescriptorSetLayout.bindingNum  = 1;
	textureDescriptorSetLayout.type	       = Renderer::DescriptorSetBindingParams::Texture_bit;
	textureDescriptorSetLayout.count       = 1;
	textureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;

	descriptorSetLayout.descriptorSetBindingParams.push_back(&textureDescriptorSetLayout);

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);

	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name	     = "RenderPass";
	renderPassParams.attachments = {
		{ Renderer::AttachmentParams::Format::SameAsSwapChain,
		    true,
		    true },
	};
	renderPassParams.subpasses = {
		{ {
		      0,
		  },
		    -1 }
	};

	renderer.CreateRenderPass(renderPassParams);

	graphicsPipelineParams.renderPassName	= "RenderPass";
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);

	RootAllocator RootAllocator;
	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "vertexAllocator");
	auto drawArray = DrawVertexArray<BasicVertex>(3, vertexAllocator);

	drawArray[0].position(0) = -0.5f;
	drawArray[0].position(1) = 0.0f;
	drawArray[0].position(2) = 0.0f;
	drawArray[0].uv(0)	 = 0.0f;
	drawArray[0].uv(1)	 = 1.0f;

	drawArray[1].position(0) = 0.5f;
	drawArray[1].position(1) = 0.0f;
	drawArray[1].position(2) = 0.0f;
	drawArray[1].uv(0)	 = 1.0f;
	drawArray[1].uv(1)	 = 1.0f;

	drawArray[2].position(0) = 0.0f;
	drawArray[2].position(1) = 0.5f;
	drawArray[2].position(2) = 0.0f;
	drawArray[2].uv(0)	 = 0.5f;
	drawArray[2].uv(1)	 = 0.0f;

	renderer.InitializeVertexArray(&drawArray);
	renderer.UpdateVertexArray(&drawArray);

	Renderer::GpuBuffer uboBuffer = renderer.CreateGpuBuffer(32, Renderer::Uniform);
	void* uboCpuPtr		      = nullptr;
	renderer.GetCpuMemoryPointer(uboBuffer, &uboCpuPtr);
	float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
	*uboFloatCpuPtr	      = 0.0f;
	renderer.UnmapCpuMemoryPointer(uboBuffer);

	Renderer::GpuTexture textureMemory;
	textureMemory = renderer.CreateGpuTexture(128, 128);

	Renderer::GpuBuffer stagingBuffer = renderer.CreateGpuBuffer(128 * 128 * 4, Renderer::Transfer);
	uint32_t* stagingBufferCpu	  = nullptr;
	renderer.GetCpuMemoryPointer(stagingBuffer, (void**)&stagingBufferCpu);
	for (uint32_t i = 0; i < 128; i++) {
		for (uint32_t j = 0; j < 128; j++) {
			if ((i / 8 + j / 8) % 2 == 0) {
				stagingBufferCpu[128 * i + j] = 0xFF555555;
			} else {
				stagingBufferCpu[128 * i + j] = 0xFFFFFFFF;
			}
		}
	}
	renderer.UnmapCpuMemoryPointer(stagingBuffer);
	renderer.TransferStagingBufferToImage(stagingBuffer, textureMemory);

	Renderer::DescriptorSetInterface descriptorSetInterface = renderer.CreateDescriptorSetInterface("testPipeline", 0);
	Renderer::DescriptorWriterParams descriptorWriterParams;
	Renderer::DescriptorWriterParams::DescriptorInfo uboDescriptorInfo;
	uboDescriptorInfo.type	     = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	uboDescriptorInfo.bindingNum = 0;
	uboDescriptorInfo.count	     = 1;
	uboDescriptorInfo.pResources.resize(1);
	uboDescriptorInfo.pResources[0] = uboBuffer.pGpuMemoryImpl;
	descriptorWriterParams.descriptorInfos.push_back(&uboDescriptorInfo);
	Renderer::DescriptorWriterParams::DescriptorInfo textureDescriptorInfo;
	textureDescriptorInfo.type	 = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
	textureDescriptorInfo.bindingNum = 1;
	textureDescriptorInfo.count	 = 1;
	textureDescriptorInfo.pResources.resize(1);
	textureDescriptorInfo.pResources[0] = textureMemory.pGpuTextureMemoryImpl;
	descriptorWriterParams.descriptorInfos.push_back(&textureDescriptorInfo);
	renderer.WriteDescriptorSet(descriptorWriterParams, descriptorSetInterface);

	Renderer::DrawParams drawParams;
	drawParams.vertexArray.push_back(drawArray.getGpuMemoryImpl());
	drawParams.descriptorSetInterface = descriptorSetInterface;
	drawParams.graphicsPipelineName	  = "testPipeline";


	auto persMat = makeProjectionMatrix(0.1, 100.0, 1.0, 1.0, 1.0, 1.0);

	uint32_t counter = 0;
	while (renderer.DrawCondition()) {
		renderer.DrawStart();

		renderer.GetCpuMemoryPointer(uboBuffer, &uboCpuPtr);
		uboFloatCpuPtr	= static_cast<float*>(uboCpuPtr);
		*uboFloatCpuPtr = std::sin(counter / 50.0f) * 0.6f;
		renderer.UnmapCpuMemoryPointer(uboBuffer);

		Renderer::UpdatePushConstantParams updatePushConstantParams;
		updatePushConstantParams.graphicsPipelineName = "testPipeline";

		int32_t foo = ((counter / 30) % 2 == 0) ? 1 : -1;

		updatePushConstantParams.pData	     = &foo;
		updatePushConstantParams.size	     = sizeof(int32_t);
		updatePushConstantParams.shaderStage = Renderer::ShaderStage::VertexBit | Renderer::ShaderStage::FragmentBit;

		renderer.UpdatePushConstant(updatePushConstantParams);

		renderer.Draw(drawParams);

		counter++;

		renderer.DrawEnd();
	}

	return 0;
}
