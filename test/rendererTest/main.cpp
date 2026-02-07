
#include "src/utils/mathfunc/mathUtils.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/mesh/drawArray.hpp"
#include "src/utils/memory/allocator.hpp"

#include <cstring>
#include <filesystem>

#include "src/utils/fileloader/OBJLoader.hpp"


class DrawObject
{
public:
	Renderer::DescriptorSetInterface descriptorSetInterface;
	DrawVertexArray<BasicVertex> drawArray;
	DrawVertexArray<int32_t> indexdrawArray;

	Renderer::GpuBuffer uboBuffer;
	Renderer::GpuTexture textureMemory;

	Renderer::DescriptorWriterParams descriptorWriterParams;

	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	DrawObject(TypeAllocator<BasicVertex>& vertexAllocator, TypeAllocator<int32_t>& intAllocator, uint32_t vertexCount, uint32_t indexCount)
		: drawArray(0, vertexAllocator), indexdrawArray(0, intAllocator, true), vertexCount(vertexCount), indexCount(indexCount)
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

		textureMemory;
		textureMemory = renderer.CreateGpuTexture(128, 128, Renderer::ImageFormat::RGBA8_SNORM);

		Renderer::GpuBuffer stagingBuffer = renderer.CreateGpuBuffer(128 * 128 * 4, Renderer::Transfer);
		uint32_t* stagingBufferCpu = nullptr;
		renderer.GetCpuMemoryPointer(stagingBuffer, (void**)&stagingBufferCpu);
		for (uint32_t i = 0; i < 128; i++) {
			for (uint32_t j = 0; j < 128; j++) {
				if ((i / 8 + j / 8) % 2 == 0) {
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

		Renderer::DescriptorWriterParams::DescriptorInfo uboDescriptorInfo;
		uboDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
		uboDescriptorInfo.bindingNum = 0;
		uboDescriptorInfo.count = 1;
		uboDescriptorInfo.pResources.resize(1);
		uboDescriptorInfo.pResources[0] = uboBuffer.pGpuMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(uboDescriptorInfo);
		Renderer::DescriptorWriterParams::DescriptorInfo textureDescriptorInfo;
		textureDescriptorInfo.type = Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler;
		textureDescriptorInfo.bindingNum = 1;
		textureDescriptorInfo.count = 1;
		textureDescriptorInfo.pResources.resize(1);
		textureDescriptorInfo.pResources[0] = textureMemory.pGpuTextureMemoryImpl;
		descriptorWriterParams.descriptorInfos.push_back(textureDescriptorInfo);
	}

	void WriteDescriptorSet(Renderer& renderer)
	{
		renderer.WriteDescriptorSet(descriptorWriterParams, descriptorSetInterface);
	}
};

int main()
{
	Renderer::InitializeParams rendererInitializeParams;
	rendererInitializeParams.isDebugMode = true;
	rendererInitializeParams.windowSize = ivec2(1280, 720);
	rendererInitializeParams.windowName = "Renderer Test";

	Renderer renderer;
	renderer.Initialize(rendererInitializeParams);

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

	Renderer::VertexAttributeLayout vertexAttributeLayout;
	Renderer::CreateVertexAttributeLayout2<BasicVertex>(&vertexAttributeLayout);
	renderer.RegisterVertexInputStateImpl3(&vertexAttributeLayout);

	graphicsPipelineParams.vertexLayoutName = vertexAttributeLayout.name;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout;
	Renderer::DescriptorSetBindingParams persMatUboDescriptorSetLayout;
	persMatUboDescriptorSetLayout.bindingNum = 0;
	persMatUboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	persMatUboDescriptorSetLayout.count = 1;
	persMatUboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout.descriptorSetBindingParams.push_back(&persMatUboDescriptorSetLayout);
	descriptorSetLayout.isBindless = false;

	Renderer::DescriptorSetLayoutParams descriptorSetLayout2;
	Renderer::DescriptorSetBindingParams uboDescriptorSetLayout;
	uboDescriptorSetLayout.bindingNum = 0;
	uboDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::UniformBuffer_bit;
	uboDescriptorSetLayout.count = 1;
	uboDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Vertex_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&uboDescriptorSetLayout);

	Renderer::DescriptorSetBindingParams textureDescriptorSetLayout;
	textureDescriptorSetLayout.bindingNum = 1;
	textureDescriptorSetLayout.type = Renderer::DescriptorSetBindingParams::Texture_bit;
	textureDescriptorSetLayout.count = 1;
	textureDescriptorSetLayout.shaderStage = Renderer::DescriptorSetBindingParams::Fragment_bit;
	descriptorSetLayout2.descriptorSetBindingParams.push_back(&textureDescriptorSetLayout);
	descriptorSetLayout2.isBindless = false;

	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout);
	graphicsPipelineParams.descriptorSetParams.push_back(&descriptorSetLayout2);

	Renderer::RenderPassParams renderPassParams;
	renderPassParams.name = "RenderPass";
	renderPassParams.attachments = {
		{ Renderer::ImageFormat::SameAsSwapChain,
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

	graphicsPipelineParams.renderPassName = "RenderPass";
	graphicsPipelineParams.pushConstantSize = sizeof(uint32_t);

	renderer.CreateGraphicsPipeline(graphicsPipelineParams);


	//////////
	std::vector<fvec3> positions;
	std::vector<fvec3> normals;
	std::vector<fvec2> uvs;
	std::vector<uint32_t> faceindices;
	LoadOBJtoRenderTriangleMesh(
		"../../../../../resources/Bunny.obj",
		positions,
		normals,
		uvs,
		faceindices,
		fvec3(0.0f, 0.0f, 0.0f),
		0.5f);

	RootAllocator RootAllocator;
	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "vertexAllocator");
	TypeAllocator<int32_t> intAllocator(&RootAllocator, "intAllocator");

	DrawObject drawObject(vertexAllocator, intAllocator, positions.size(), faceindices.size());
	drawObject.Initialize(renderer);

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
	}

	for (uint32_t i = 0; i < faceindices.size(); i++) {
		drawObject.indexdrawArray[i] = static_cast<int32_t>(faceindices[i]);
	}


	renderer.UpdateVertexArray(&drawObject.drawArray);
	renderer.UpdateVertexArray(&drawObject.indexdrawArray);


	// perspective 
	auto persMat = makeProjectionMatrixVk(0.01, 10.0, 0.01, -0.01, 0.01, -0.01).transpose();

	auto persMatUbo = renderer.CreateGpuBuffer(sizeof(persMat), Renderer::Uniform);
	void* persMatUboCpuBuffer = nullptr;
	renderer.GetCpuMemoryPointer(persMatUbo, &persMatUboCpuBuffer);
	std::memcpy(persMatUboCpuBuffer, &persMat, sizeof(persMat));
	renderer.UnmapCpuMemoryPointer(persMatUbo);

	auto descriptorSetInterface = renderer.CreateDescriptorSetInterface("testPipeline", 0);

	Renderer::DescriptorWriterParams persUboDescriptorWriterParams;
	Renderer::DescriptorWriterParams::DescriptorInfo persUboDescriptorWriteParams;
	persUboDescriptorWriteParams.type = Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer;
	persUboDescriptorWriteParams.bindingNum = 0;
	persUboDescriptorWriteParams.count = 1;
	persUboDescriptorWriteParams.pResources.resize(1);
	persUboDescriptorWriteParams.pResources[0] = persMatUbo.pGpuMemoryImpl;
	persUboDescriptorWriterParams.descriptorInfos.push_back(persUboDescriptorWriteParams);

	renderer.WriteDescriptorSet(persUboDescriptorWriterParams, descriptorSetInterface);



	Renderer::DrawParams drawParams;
	drawParams.pVertexArray = drawObject.drawArray.getGpuMemoryImpl();
	drawParams.instanceCount = 1;
	drawParams.pIndexArray = drawObject.indexdrawArray.getGpuMemoryImpl();
	drawParams.count = drawObject.indexdrawArray.size();
	drawParams.descriptorSetInterfaces.push_back(drawObject.descriptorSetInterface);
	drawParams.descriptorSetInterfaces.push_back(descriptorSetInterface);
	drawParams.graphicsPipelineName = "testPipeline";

	uint32_t counter = 0;
	while (renderer.DrawCondition()) {
		renderer.DrawStart();

		void* uboCpuPtr = nullptr;
		renderer.GetCpuMemoryPointer(drawObject.uboBuffer, &uboCpuPtr);
		float* uboFloatCpuPtr = static_cast<float*>(uboCpuPtr);
		//*uboFloatCpuPtr = std::sin(counter / 50.0f) * 0.6f;
		*uboFloatCpuPtr = counter / 50.0f;
		renderer.UnmapCpuMemoryPointer(drawObject.uboBuffer);

		Renderer::UpdatePushConstantParams updatePushConstantParams;
		updatePushConstantParams.graphicsPipelineName = "testPipeline";

		int32_t foo = ((counter / 30) % 2 == 0) ? 1 : -1;

		updatePushConstantParams.pData = &foo;
		updatePushConstantParams.size = sizeof(int32_t);
		updatePushConstantParams.shaderStage = Renderer::ShaderStage::VertexBit | Renderer::ShaderStage::FragmentBit;

		renderer.UpdatePushConstant(updatePushConstantParams);

		renderer.Draw(drawParams);

		counter++;

		renderer.DrawEnd();
	}

	return 0;
}
