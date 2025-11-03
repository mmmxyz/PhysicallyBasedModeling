
#include "src/renderer/renderer.hpp"
#include "src/renderer/rendererImpl.hpp"
#include "src/renderer/gpuMemoryImpl.hpp"
#include "src/renderer/mesh/drawArray.hpp"

#include <array>
#include <fstream>
#include "src/utils/logger/logger.hpp"


void Renderer::CreateGraphicsPipeline(Renderer::GraphicsPipelineParams& graphicsPipelineParams)
{
	Logger logger;
	logger.isEnabled = true;

	VkResult result;

	auto createShaderModule = [&](const char* filePath) -> VkShaderModule {

		if (m_pImpl->shaderModuleMap.find(filePath) != m_pImpl->shaderModuleMap.end())
		{
			return m_pImpl->shaderModuleMap[filePath];
		}

		uint32_t shaderCodeSize = 0;
		char* shaderCode = nullptr;
		std::ifstream ifs(filePath, std::ios::binary);
		if (!ifs)
		{
			logger << "failed to find shader binary file : " << filePath << std::endl;
			exit(1);
		}
		ifs.seekg(0, std::ios::end);
		shaderCodeSize = ifs.tellg();
		shaderCode = new char[shaderCodeSize];
		ifs.seekg(0);

		ifs.read(shaderCode, shaderCodeSize);

		VkShaderModuleCreateInfo SMCI;
		SMCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		SMCI.pNext = nullptr;
		SMCI.flags = 0;// 予約
		SMCI.codeSize = shaderCodeSize;
		SMCI.pCode = reinterpret_cast<uint32_t*>(shaderCode);

		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(m_pImpl->logicalDevice, &SMCI, nullptr, &shaderModule);

		m_pImpl->shaderModuleMap[filePath] = shaderModule;

		return shaderModule;
		};

	VkPipelineShaderStageCreateInfo shaderStages[16] = {};
	for (int i = 0; i < graphicsPipelineParams.shaders.size(); i++)
	{
		ShaderStageParams& shaderStageParam = *graphicsPipelineParams.shaders[i];
		logger << "Shader Stage " << shaderStageParam.stageType << ":"
			<< " Path: " << shaderStageParam.shaderPath << std::endl;

		VkShaderModule shaderModule = createShaderModule(shaderStageParam.shaderPath.data());

		shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].pNext = nullptr;
		shaderStages[i].flags = 0; // 予約
		shaderStages[i].stage = (shaderStageParam.stageType == ShaderStageParams::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[i].module = shaderModule;
		shaderStages[i].pName = "main"; // エントリポイントの指定（関数名）
		shaderStages[i].pSpecializationInfo; // 特殊化定数に使う constant_id で与えられる変数に値を与える
	}

	VkPipelineCache piplineCache = {}; // キャッシュをディスクなどに保存できる

	//// State を設定

	// 動的に決められる = パイプラインの再作成を要求しない
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;


	auto createDescriptorSetLayoutBinding = [&](DescriptorSetBindingParams& descriptorSetLayoutParam, VkDescriptorSetLayoutBinding& layoutBinding) -> void {
		layoutBinding.binding = descriptorSetLayoutParam.bindingNum;
		switch (descriptorSetLayoutParam.type)
		{
		case DescriptorSetBindingParams::UniformBuffer_bit:
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case DescriptorSetBindingParams::Sampler_bit:
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case DescriptorSetBindingParams::Texture_bit:
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		}
		layoutBinding.descriptorCount = descriptorSetLayoutParam.count;
		layoutBinding.stageFlags = 0;
		if (descriptorSetLayoutParam.shaderStage & DescriptorSetBindingParams::Vertex_bit)
		{
			layoutBinding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (descriptorSetLayoutParam.shaderStage & DescriptorSetBindingParams::Fragment_bit)
		{
			layoutBinding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		};

	const int descriptorSetCounter = graphicsPipelineParams.descriptorSetParams.size();

	m_pImpl->pDescriptorSetLayout = new VkDescriptorSetLayout[descriptorSetCounter];
	for (int i = 0; i < descriptorSetCounter; i++)
	{
		VkDescriptorSetLayoutBinding layoutBindings[256] = {};

		auto descriptorSetLayoutParam = graphicsPipelineParams.descriptorSetParams[i];
		for (int j = 0; j < descriptorSetLayoutParam->descriptorSetBindingParams.size(); j++)
		{
			createDescriptorSetLayoutBinding(*descriptorSetLayoutParam->descriptorSetBindingParams[j], layoutBindings[j]);
		}


		VkDescriptorSetLayoutCreateInfo DSLCI = {};
		DSLCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DSLCI.bindingCount = descriptorSetLayoutParam->descriptorSetBindingParams.size();
		DSLCI.pBindings = layoutBindings;

		result = vkCreateDescriptorSetLayout(m_pImpl->logicalDevice, &DSLCI, nullptr, &m_pImpl->pDescriptorSetLayout[i]);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}





	VkPipelineInputAssemblyStateCreateInfo PIASCI = {};
	PIASCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	PIASCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PIASCI.primitiveRestartEnable = VK_FALSE; // _STRIP 系の topology のとき，インデックスが 0xFFFF のとき打ち切るかどうか












	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	// dynamic state で viewport と scissor を指定したので↑の構造体には両者を格納しない


	//// rasterizer

	VkPipelineRasterizationStateCreateInfo PRSC = {};
	PRSC.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	PRSC.depthClampEnable = VK_FALSE; // near, far を越えると discard するのではなく clamp される， shadow map 作るときに便利
	PRSC.rasterizerDiscardEnable = VK_FALSE; // 
	PRSC.polygonMode = VK_POLYGON_MODE_FILL; // line だけ， point だけ描画したいなど
	PRSC.lineWidth = 1.0f; //
	PRSC.cullMode = VK_CULL_MODE_NONE;
	PRSC.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// fragment slope に応じて depth 値を修正できる
	PRSC.depthBiasEnable = VK_FALSE; // 無効（Zファイティング抑制に使われる）
	PRSC.depthBiasConstantFactor = 0.0f;
	PRSC.depthBiasClamp = 0.0f;
	PRSC.depthBiasSlopeFactor = 0.0f;

	// multiSample: 三角形の edge 周辺で複数ポリゴンからピクセルに割り当てられるときの処理
	VkPipelineMultisampleStateCreateInfo PMSC = {};
	PMSC.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	PMSC.sampleShadingEnable = VK_FALSE;
	PMSC.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	PMSC.minSampleShading = 1.0;
	PMSC.pSampleMask = nullptr;
	PMSC.alphaToCoverageEnable = VK_FALSE;
	PMSC.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo PDSSC = {};
	PDSSC.depthBoundsTestEnable = VK_TRUE;
	PDSSC.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
	PDSSC.depthWriteEnable = VK_TRUE;
	PDSSC.depthBoundsTestEnable = VK_FALSE; // 境界テストはOFF
	PDSSC.stencilTestEnable = VK_FALSE;


	VkPipelineColorBlendAttachmentState PCBAS = {};
	PCBAS.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	PCBAS.blendEnable = VK_TRUE;
	PCBAS.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	PCBAS.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	PCBAS.colorBlendOp = VK_BLEND_OP_ADD;
	PCBAS.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	PCBAS.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	PCBAS.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo PCBSC = {};
	PCBSC.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	PCBSC.pNext = nullptr;
	PCBSC.logicOpEnable = VK_FALSE;
	PCBSC.attachmentCount = 1;
	PCBSC.pAttachments = &PCBAS;

	// パイプラインレイアウト作成
	VkPipelineLayoutCreateInfo PLCI = {};
	PLCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PLCI.flags = 0;
	PLCI.pNext = nullptr;
	PLCI.setLayoutCount = 0;
	PLCI.pSetLayouts = nullptr;
	PLCI.pushConstantRangeCount = 0; // uniform buffer は使わない
	PLCI.pPushConstantRanges = nullptr;
	PLCI.setLayoutCount = descriptorSetCounter;
	PLCI.pSetLayouts = m_pImpl->pDescriptorSetLayout;

	result = vkCreatePipelineLayout(m_pImpl->logicalDevice, &PLCI, nullptr, &m_pImpl->pipelineLayout);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	// vkDestroyPipelineLayout(logicaldevice, pipelineLayout, nullptr);



	/////// RenderPath 作成

	// ここでは swap chain の画像を表すカラーバッファのアタッチメントを作成
	VkAttachmentDescription colorAttachDescription = {};
	colorAttachDescription.format = m_pImpl->swapChainImageFormat;
	colorAttachDescription.samples = VK_SAMPLE_COUNT_1_BIT; // multi sample しない
	colorAttachDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// attachment 参照
	VkAttachmentReference colorAttachRef = {};
	colorAttachRef.attachment = 0; // index = 0
	colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // color 最適

	// subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef; // これで layout(location = 0) out vec4 outColor ができる

	// render pass
	VkRenderPassCreateInfo RPCI = {};
	RPCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RPCI.attachmentCount = 1;
	RPCI.pAttachments = &colorAttachDescription;
	RPCI.subpassCount = 1;
	RPCI.pSubpasses = &subpass;

	result = vkCreateRenderPass(m_pImpl->logicalDevice, &RPCI, nullptr, &m_pImpl->renderPass);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	///vkDestroyRenderPass(logicaldevice, renderPass, nullptr);

		// Graphic Pipeline

	VkGraphicsPipelineCreateInfo GPCI = { };
	GPCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	GPCI.pNext = nullptr;
	GPCI.stageCount = 2;
	GPCI.pStages = shaderStages;
	GPCI.pVertexInputState = &m_pImpl->vertexInputStateImplMap["struct BasicVertex"]->vertexInputInfo;
	GPCI.pInputAssemblyState = &PIASCI;
	GPCI.pViewportState = &viewportState;
	GPCI.pRasterizationState = &PRSC;
	GPCI.pMultisampleState = &PMSC;
	GPCI.pDepthStencilState = &PDSSC;
	GPCI.pColorBlendState = &PCBSC;
	GPCI.pDynamicState = &dynamicState;

	GPCI.layout = m_pImpl->pipelineLayout;

	GPCI.renderPass = m_pImpl->renderPass;
	GPCI.subpass = 0; // index = 0;

	GPCI.basePipelineHandle = VK_NULL_HANDLE; // 不使用
	GPCI.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(m_pImpl->logicalDevice, VK_NULL_HANDLE, 1, &GPCI, nullptr, &m_pImpl->graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}


	// framebuffer 作成

	VkFramebuffer* frameBuffers = new VkFramebuffer[m_pImpl->swapChainImageCount];
	m_pImpl->frameBuffers = frameBuffers;
	for (int i = 0; i < m_pImpl->swapChainImageCount; i++)
	{
		VkFramebufferCreateInfo FCI = {};
		FCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FCI.flags = 0;
		FCI.renderPass = m_pImpl->renderPass;
		FCI.attachmentCount = 1;
		FCI.pAttachments = &m_pImpl->swapChainImageViews[i];
		FCI.width = m_pImpl->surfaceCapabilities.currentExtent.width;
		FCI.height = m_pImpl->surfaceCapabilities.currentExtent.height;
		FCI.layers = 1;

		result = vkCreateFramebuffer(m_pImpl->logicalDevice, &FCI, nullptr, &frameBuffers[i]);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}
}


Renderer::GpuBuffer Renderer::CreateGpuBuffer(uint32_t size, Renderer::BufferCreateUsage usage)
{
	GpuMemoryImpl* pGpuMemoryImpl = new GpuMemoryImpl();
	m_pImpl->CreateBuffer(*pGpuMemoryImpl, usage, size);

	return { size, pGpuMemoryImpl };
}

Renderer::GpuTexture Renderer::CreateGpuTexture(uint32_t width, uint32_t height)
{
	GpuTextureMemoryImpl* pGpuTextureMemoryImpl = new GpuTextureMemoryImpl();
	m_pImpl->CreateImage(width, height, *pGpuTextureMemoryImpl);
	m_pImpl->CreateImageView(*pGpuTextureMemoryImpl);
	m_pImpl->CreateSampler(*pGpuTextureMemoryImpl);
	return { width, height, pGpuTextureMemoryImpl->size, pGpuTextureMemoryImpl };
}

Renderer::DescriptorSetInterface Renderer::CreateDescriptorSetInterface(int set)
{
	DescriptorSetImpl* pDescriptorSetImpl = new DescriptorSetImpl();
	pDescriptorSetImpl->set = set;

	VkDescriptorSetAllocateInfo DSAI = {};
	DSAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	DSAI.descriptorPool = m_pImpl->descriptorPool;
	DSAI.descriptorSetCount = 1;
	DSAI.pSetLayouts = &m_pImpl->pDescriptorSetLayout[set];

	VkResult result = vkAllocateDescriptorSets(m_pImpl->logicalDevice, &DSAI, &pDescriptorSetImpl->descriptorSet);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	return { set, pDescriptorSetImpl };
}

void Renderer::WriteDescriptorSet(Renderer::DescriptorWriterParams& descriptorWriteParams, Renderer::DescriptorSetInterface& descriptorSetInterface)
{
	VkWriteDescriptorSet writeDescriptorSets[128];
	VkDescriptorBufferInfo bufferInfos[64];
	uint32_t bufferInfoCounter = 0;
	VkDescriptorImageInfo imageInfos[64];
	uint32_t imageInfoCounter = 0;

	for (int i = 0; i < descriptorWriteParams.descriptorInfos.size(); i++)
	{
		Renderer::DescriptorWriterParams::DescriptorInfo& descriptorInfo = *descriptorWriteParams.descriptorInfos[i];
		if (descriptorInfo.type == Renderer::DescriptorWriterParams::DescriptorInfo::UniformBuffer)
		{
			for (int j = 0; j < descriptorInfo.count; j++)
			{
				GpuMemoryImpl* pGpuMemoryImpl = reinterpret_cast<GpuMemoryImpl*>(descriptorInfo.pResources[j]);
				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = pGpuMemoryImpl->buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;
				bufferInfos[bufferInfoCounter++] = bufferInfo;
			}
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSetInterface.pDescriptorSetImpl->descriptorSet;
			writeDescriptorSet.dstBinding = descriptorInfo.bindingNum;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = descriptorInfo.count;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = bufferInfos + bufferInfoCounter - descriptorInfo.count;;
			writeDescriptorSets[i] = writeDescriptorSet;
		}
		else if (descriptorInfo.type == Renderer::DescriptorWriterParams::DescriptorInfo::Combined_Image_Sampler)
		{
			for (int j = 0; j < descriptorInfo.count; j++)
			{
				GpuTextureMemoryImpl* pGpuTextureMemoryImpl = reinterpret_cast<GpuTextureMemoryImpl*>(descriptorInfo.pResources[j]);
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = pGpuTextureMemoryImpl->imageView;
				imageInfo.sampler = pGpuTextureMemoryImpl->sampler;
				imageInfos[imageInfoCounter++] = imageInfo;
			}
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSetInterface.pDescriptorSetImpl->descriptorSet;
			writeDescriptorSet.dstBinding = descriptorInfo.bindingNum;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = descriptorInfo.count;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = imageInfos + imageInfoCounter - descriptorInfo.count;
			writeDescriptorSets[i] = writeDescriptorSet;
		}
	}
	vkUpdateDescriptorSets(m_pImpl->logicalDevice, 2, writeDescriptorSets, 0, nullptr);


}

void Renderer::GetCpuMemoryPointer(Renderer::GpuBuffer& gpuMemory, void** ppData)
{
	m_pImpl->GetCpuMemoryPointer(*gpuMemory.pGpuMemoryImpl, ppData);
}

void Renderer::UnmapCpuMemoryPointer(Renderer::GpuBuffer& gpuMemoryImpl)
{
	m_pImpl->UnmapCpuMemoryPointer(*gpuMemoryImpl.pGpuMemoryImpl);
}

void Renderer::TransferStagingBufferToImage(GpuBuffer& stagingBuffer, GpuTexture& textureMemory)
{
	m_pImpl->TransferStagingBufferToImage(*stagingBuffer.pGpuMemoryImpl, *textureMemory.pGpuTextureMemoryImpl);
}



void Renderer::Initialize(InitializeParams& initializeParams)
{
	const bool isDebugMode = initializeParams.isDebugMode;
	VkResult result;
	Logger logger;
	logger.isEnabled = true;

	m_pImpl = new RendererImpl();

	// まずは GLFW の初期化

	// glfw の設定
	if (!glfwInit())
	{
		logger << "fail to initialize glfw!!!" << std::endl;
		exit(1);
	}

	// ウィンドウサイズは変えられない
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	if (glfwVulkanSupported() != GLFW_TRUE)
	{
		logger << "GLFW does not support vulkan!!!" << std::endl;
		exit(1);
	}

	// glfw に必要なインスタンスの拡張機能を検索
	uint32_t glfwExtensionCount;
	const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	logger << "GLFW extensions count: " << glfwExtensionCount << std::endl;
	for (int i = 0; i < glfwExtensionCount; i++)
	{
		logger << i << " th Extension: " << glfwExtensionNames[i] << std::endl;
	}

	const uint32_t instanceExtensionCount = glfwExtensionCount + (isDebugMode ? 2 : 0);
	const char* instanceExtensionNames[16];
	if (isDebugMode)
	{
		instanceExtensionNames[0] = VK_KHR_DISPLAY_EXTENSION_NAME;
		instanceExtensionNames[1] = VK_KHR_SURFACE_EXTENSION_NAME;
	}
	for (int i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensionNames[i + (isDebugMode ? 2 : 0)] = glfwExtensionNames[i];
	}

	logger << std::endl;

	////////////////////
	///instanceの作成///
	////////////////////

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;			       //リンクの次の構造体はなし
	appInfo.pApplicationName = "Vulkan アプリケーション テスト"; //アプリケーションの名前を格納したヌル終端文字列へのポインタ，ちなみにASCII文字における0の値は，nullでなくてnulらしい
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	       //アプリケーションのバージョン，
	appInfo.pEngineName = nullptr;			       //使用するエンジンの名前
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2; //アプリケーションが期待するvulkan apiのバージョン，実行に必要な最小のバージョンを設定する

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr; //リンクの次の構造体はなし
	createInfo.flags = 0;	      //
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = 0;	      //有効にするインスタンスレイヤの数
	createInfo.ppEnabledLayerNames = nullptr; //有効にするインスタンスレイヤの名前のベクタ(ヌル終端された文字列のポインタの配列)
	createInfo.enabledExtensionCount = instanceExtensionCount;	      //拡張機能の数
	createInfo.ppEnabledExtensionNames = instanceExtensionNames; //拡張機能の名前のベクタ


	//////////////////////////////////
	///インスタンスのレイヤの有効化///
	//////////////////////////////////

	// デバッグモードの場合はVK_LAYER_KHRONOS_validationを有効化する．
	constexpr const char* LAYER_NAME = "VK_LAYER_KHRONOS_validation";

	uint32_t numILayer;
	result = vkEnumerateInstanceLayerProperties(&numILayer, nullptr);
	if (result != VK_SUCCESS) {
		logger << "failed to get a number of ILayer!!!" << std::endl;
		exit(1);
	}
	logger << "Instance Layer size: " << numILayer << std::endl;
	VkLayerProperties* pILPs = new VkLayerProperties[numILayer];
	result = vkEnumerateInstanceLayerProperties(&numILayer, pILPs);
	if (result != VK_SUCCESS) {
		logger << "failed to get properties of ILayer!!!" << std::endl;
		exit(1);
	}

	int32_t Layerindex = -1;
	for (uint32_t i = 0; i < numILayer; i++) {
		logger << std::endl;
		logger << "ILayer[" << i << "]" << std::endl;
		logger << "ILayer Name: " << pILPs[i].layerName << std::endl;
		logger << "Spec Version: " << VK_VERSION_MAJOR(pILPs[i].specVersion) << "." << VK_VERSION_MINOR(pILPs[i].specVersion) << std::endl;
		logger << "Impl Version: " << VK_VERSION_MAJOR(pILPs[i].implementationVersion) << "." << VK_VERSION_MINOR(pILPs[i].implementationVersion) << std::endl;
		logger << "description: " << pILPs[i].description << std::endl;

		//有効化するレイヤであるか
		if (std::strcmp(pILPs[i].layerName, LAYER_NAME) == 0)
			Layerindex = i;
	}
	logger << std::endl;

	//有効化するレイヤが存在しないとき
	if (Layerindex == -1) {
		logger << LAYER_NAME << " is not available!!!" << std::endl;
		exit(1);
	}

	const char* ppILT[1];
	//ppILT[0] = LAYER_NAME;
	ppILT[0] = pILPs[Layerindex].layerName;
	//どちらでもよいのだが，createInfoのフィールドにはこのまま使えるということ．

	if (isDebugMode)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = ppILT;
	}

	//////////////
	///拡張機能///
	//////////////

	//ここでは一覧を出力するだけ

	uint32_t numIExtension;
	result = vkEnumerateInstanceExtensionProperties(nullptr, &numIExtension, nullptr);
	if (result != VK_SUCCESS) {
		logger << "failed to get a number of IExtension!!!" << std::endl;
		exit(1);
	}
	logger << "Instance Extension size: " << numIExtension << std::endl;
	VkExtensionProperties* pIEPs = new VkExtensionProperties[numIExtension];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &numIExtension, pIEPs);
	if (result != VK_SUCCESS) {
		logger << "failt to get properties of IExtension!!!" << std::endl;
		exit(1);
	}

	for (uint32_t i = 0; i < numIExtension; i++) {
		logger << std::endl;
		logger << "IExtension[" << i << "]" << std::endl;
		logger << "IExtension Name: " << pIEPs[i].extensionName << std::endl;
		logger << "Spec Version: " << VK_VERSION_MAJOR(pIEPs[i].specVersion) << "." << VK_VERSION_MINOR(pIEPs[i].specVersion) << std::endl;
	}
	logger << std::endl;

	//インスタンスの作成

	VkInstance instance;
	result = vkCreateInstance(&createInfo, nullptr, &instance);
	// 第二引数ではアプリ側で用意した VkAllocationCallbacks を渡すことができる．nullptr を渡すとVulkan実装のアロケータを使う

	if (result != VK_SUCCESS) {
		logger << "failed to create instance!!!" << std::endl;
		exit(1);
	}

	//////////////////
	///物理デバイス///
	//////////////////

	uint32_t numPD;
	result = vkEnumeratePhysicalDevices(instance, &numPD, nullptr);
	if (result != VK_SUCCESS) {
		logger << "failed to check physical devices!!!" << std::endl;
		exit(1);
	}
	logger << "Physical Device Size: " << numPD << std::endl;
	if (numPD <= 0) {
		logger << "no physical devices!!!" << std::endl;
		exit(1);
	}

	VkPhysicalDevice* pPDs = new VkPhysicalDevice[numPD];
	result = vkEnumeratePhysicalDevices(instance, &numPD, pPDs);
	if (result != VK_SUCCESS) {
		logger << "failed to check physical devices!!!" << std::endl;
		exit(1);
	}

	const int maxPdSize = 32;
	if (numPD > maxPdSize)
	{
		logger << "numPD is greater than " << maxPdSize << " !!!" << std::endl;
		exit(1);
	}

	//各物理デバイスごとの情報を格納する領域を確保
	VkPhysicalDeviceProperties pPDPs[maxPdSize];
	VkPhysicalDeviceFeatures pPDFs[maxPdSize];
	uint32_t pNumLPs[maxPdSize];
	VkLayerProperties* ppLPs[maxPdSize];
	VkPhysicalDeviceMemoryProperties pPDMPs[maxPdSize];
	uint32_t pNumQFPs[maxPdSize];
	VkQueueFamilyProperties* ppQFPs[maxPdSize];

	//選択する物理デバイス
	int32_t physical_device_index = -1;
	//選択するキューファミリ
	int32_t queue_family_index = -1;
	// 選択されたキューファミリがサポートするキューの数
	int32_t queue_family_queue_count = -1;
	// 選択されたメモリタイプ
	int32_t memory_type_index = -1;
	int32_t memory_type_index_host_local = -1;

	//各物理デバイスに対して
	for (uint32_t i = 0; i < numPD; i++) {
		//プロパティを取得
		vkGetPhysicalDeviceProperties(pPDs[i], &pPDPs[i]);

		//VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU であるものを選択
		if (physical_device_index == -1 && pPDPs[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			physical_device_index = i;
		}

		//プロパティを出力
		logger << std::endl;
		logger << "Phsycal Device[" << i << "]";
		if (physical_device_index == int32_t(i))
			logger << " <= select this" << std::endl;
		else
			logger << std::endl;
		logger << "Name: " << pPDPs[i].deviceName << std::endl;
		logger << "Suported Vulkan Version: " << VK_VERSION_MAJOR(pPDPs[i].apiVersion) << "." << VK_VERSION_MINOR(pPDPs[i].apiVersion) << std::endl;
		//なぜかVK_API_VERSION_VARIANTが存在しない
		logger << "Device Type: ";
		switch (pPDPs[i].deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			logger << "VK_PHYSICAL_DEVICE_TYPE_OTHER" << std::endl;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			logger << "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" << std::endl;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			logger << "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU" << std::endl;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			logger << "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU" << std::endl;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			logger << "VK_PHYSICAL_DEVICE_TYPE_CPU" << std::endl;
			break;
		}
		//オプション機能の情報を出力
		vkGetPhysicalDeviceFeatures(pPDs[i], &pPDFs[i]);
		//ここではオプションの機能には触れない

		// デバイスのレイヤーを列挙
		vkEnumerateDeviceLayerProperties(pPDs[i], &pNumLPs[i], nullptr);
		ppLPs[i] = new VkLayerProperties[pNumLPs[i]];
		vkEnumerateDeviceLayerProperties(pPDs[i], &pNumLPs[i], ppLPs[i]);
		for (uint32_t j = 0; j < pNumLPs[i]; j++)
		{
			// VK_LAYER_KHRONOS_validation がないと困るが，確認してない
		}

		//メモリタイプの情報を出力
		vkGetPhysicalDeviceMemoryProperties(pPDs[i], &pPDMPs[i]);

		logger << "MemoryHeap Count: " << pPDMPs[i].memoryHeapCount << std::endl;
		for (uint32_t j = 0; j < pPDMPs[i].memoryHeapCount; j++)
		{
			logger << "MemoryHeap[" << j << "]" << std::endl;
			logger << "\tHeapSize: " << pPDMPs[i].memoryHeaps[j].size / (1024 * 1024) << " MB" << std::endl;
		}

		logger << "MemoryType Count: " << pPDMPs[i].memoryTypeCount << std::endl;
		//各メモリタイプに対して
		for (uint32_t j = 0; j < pPDMPs[i].memoryTypeCount; j++) {
			// memorytype の選択
			if (physical_device_index == int32_t(i) && memory_type_index == -1)
			{
				constexpr uint32_t requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				if (pPDMPs[i].memoryTypes[j].propertyFlags & requiredFlags)
				{
					memory_type_index = j;
					m_pImpl->memory_type_index = j;
				}
			}
			if (physical_device_index == int32_t(i) && memory_type_index_host_local == -1)
			{
				constexpr uint32_t requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				if (pPDMPs[i].memoryTypes[j].propertyFlags & requiredFlags)
				{
					memory_type_index_host_local = j;
					m_pImpl->memory_type_index_host_local = j;
				}
			}

			logger << "MemoryType[" << j << "]";
			if (physical_device_index == int32_t(i) && memory_type_index == int32_t(j))
				logger << " <= General Select This" << std::endl;
			else if (physical_device_index == int32_t(i) && memory_type_index_host_local == int32_t(j))
				logger << " <= HostLocal Select This" << std::endl;
			else
				logger << std::endl;

			logger << "\tHeap Index: " << pPDMPs[i].memoryTypes[j].heapIndex << std::endl;
			// メモリがデバイスにローカルであるかどうか
			logger << "\tVK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: " << ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & pPDMPs[i].memoryTypes[j].propertyFlags) ? "True" : "False") << std::endl;
			// メモリ割り当てがホストから直接アクセスできるか（できなければデバイス専用）
			logger << "\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: " << ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & pPDMPs[i].memoryTypes[j].propertyFlags) ? "True" : "False") << std::endl;
			// ホストとデバイスで同時にアクセスするとき，そのアクセスが二つの間でコヒーレントであること（そうでないならキャッシュを明示的にフラッシュする必要があるかも）
			logger << "\tVK_MEMORY_PROPERTY_HOST_COHERENT_BIT: " << ((VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & pPDMPs[i].memoryTypes[j].propertyFlags) ? "True" : "False") << std::endl;
			// ホストでキャッシュされるかどうか
			logger << "\tVK_MEMORY_PROPERTY_HOST_CACHED_BIT: " << ((VK_MEMORY_PROPERTY_HOST_CACHED_BIT & pPDMPs[i].memoryTypes[j].propertyFlags) ? "True" : "False") << std::endl;
			// デバイスからしかアクセスできないかどうか，これが真のとき， VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT は偽でなくてはならない
			logger << "\tVK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT: " << ((VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT & pPDMPs[i].memoryTypes[j].propertyFlags) ? "True" : "False") << std::endl;
		}

		//キューファミリの情報を出力
		vkGetPhysicalDeviceQueueFamilyProperties(pPDs[i], &pNumQFPs[i], nullptr);
		ppQFPs[i] = new VkQueueFamilyProperties[pNumQFPs[i]];
		vkGetPhysicalDeviceQueueFamilyProperties(pPDs[i], &pNumQFPs[i], ppQFPs[i]);
		logger << "Queue Family Property Count: " << pNumQFPs[i] << std::endl;
		//各キューファミリに対して
		for (uint32_t j = 0; j < pNumQFPs[i]; j++) {

			//選択された物理デバイスの場合
			//Graphics Operationが使えるQueue Familyを選択
			if (physical_device_index == int32_t(i) && queue_family_index == -1 && (ppQFPs[i][j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				queue_family_index = j;
			}

			logger << "Queue Family Properties[" << j << "]";
			if (physical_device_index == int32_t(i) && queue_family_index == int32_t(j))
				logger << " <= Select This" << std::endl;
			else
				logger << std::endl;
			//キューの能力
			// グラフィックス操作
			logger << "\tGraphics Operation: " << ((ppQFPs[i][j].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "True" : "False") << std::endl;
			// コンピュート操作
			logger << "\tCompute Operation: " << ((ppQFPs[i][j].queueFlags & VK_QUEUE_COMPUTE_BIT) ? "True" : "False") << std::endl;
			// バッファとかイメージの転送操作
			logger << "\tTransfer Operation: " << ((ppQFPs[i][j].queueFlags & VK_QUEUE_TRANSFER_BIT) ? "True" : "False") << std::endl;
			// スパースバインディング（よくわからん）
			logger << "\tSparse Binding Operation: " << ((ppQFPs[i][j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "True" : "False") << std::endl;
			//ファミリ内のキューの数
			logger << "\tQueueCount: " << ppQFPs[i][j].queueCount << std::endl;
			if (physical_device_index == int32_t(i) && queue_family_index == int32_t(j))
			{
				queue_family_queue_count = ppQFPs[i][j].queueCount;
			}
			//その他
			logger << "\tTimestampValid: " << ppQFPs[i][j].timestampValidBits << std::endl;
			logger << "\tMinImageTimestampGranularity: "
				<< ppQFPs[i][j].minImageTransferGranularity.width << ", "
				<< ppQFPs[i][j].minImageTransferGranularity.height << ", "
				<< ppQFPs[i][j].minImageTransferGranularity.depth << std::endl;
		}
	}
	logger << std::endl;

	// GLFW を使うにあたり image presentation が使えるかどうかのチェック（物理デバイスとキューファミリ）
	if (!glfwGetPhysicalDevicePresentationSupport(instance, pPDs[physical_device_index], queue_family_index))
	{
		logger << "The selected physical device and queue family does not support image presentation" << std::endl;
		exit(1);
	}

	//論理デバイスの作成

	VkDeviceQueueCreateInfo DQCInfo;
	DQCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	DQCInfo.pNext = nullptr; // 拡張機能を使わないのでnull
	DQCInfo.flags = 0;		    //現在のversionではこの属性は使われない
	DQCInfo.queueFamilyIndex = queue_family_index; //使用するキューファミリの指定，vkGetPhysicalDeviceQueueFamilyPropertiesで得られる情報をもとに決める．
	DQCInfo.queueCount = queue_family_queue_count;		    //使うキューの数，当該キューファミリがこの数のキューを使える必要がある．
	float* queuePrioritiesArray = new float[queue_family_queue_count];
	for (uint32_t i = 0; i < queue_family_queue_count; i++)
	{
		queuePrioritiesArray[i] = 1.0f;
	}
	DQCInfo.pQueuePriorities = queuePrioritiesArray; //それぞれのキューに送られる作業の優先度(0.0以上1.0以下のfloat型)を格納する配列を与える．nullptrとするとすべて同じにする．
	//pQueuePrioritiesの解釈される優先順位の段階は，vkGetPhysicalDeviceQueueFamilyPropertiesで得られるVkPhysicalDEviceLimitsのdiscreteQueuePrioritiesフィールドで確認できる(2段階など)．

	// 拡張機能の列挙
	uint32_t suppurtedDeviceExtensionCount;
	result = vkEnumerateDeviceExtensionProperties(pPDs[physical_device_index], nullptr, &suppurtedDeviceExtensionCount, nullptr);
	if (result != VK_SUCCESS)
	{
		logger << "Fail to get number of device extension." << std::endl;
		exit(1);
	}
	VkExtensionProperties* supportedDeviceExtensions = new VkExtensionProperties[suppurtedDeviceExtensionCount];
	result = vkEnumerateDeviceExtensionProperties(pPDs[physical_device_index], nullptr, &suppurtedDeviceExtensionCount, supportedDeviceExtensions);
	if (result != VK_SUCCESS)
	{
		logger << "Fail to get device extension." << std::endl;
		exit(1);
	}

	for (int i = 0; i < suppurtedDeviceExtensionCount; i++)
	{
		logger << "Extension of PhysicalDevice " << i << " th extension" << std::endl;
		logger << supportedDeviceExtensions->extensionName << " Version: " << supportedDeviceExtensions->specVersion << std::endl;
	}

	// 必要な拡張機能
	const uint32_t deviceExtensionCount = 1;
	const char** deviceExtensionName = new const char* [deviceExtensionCount];
	deviceExtensionName[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME; // スワップチェーン作成に必要な拡張のためのマクロ

	VkDeviceCreateInfo DCInfo;
	DCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DCInfo.pNext = nullptr;
	DCInfo.flags = 0; //現在のversionではこの属性は使われない
	DCInfo.queueCreateInfoCount = 1; //キューは複数作成できるが，ここでは一つのキューだけを与える，
	DCInfo.pQueueCreateInfos = &DQCInfo; // 複数のキューファミリーを割り当てる場合はここで配列を指定する．今は一つしか指定しないので単なるポインタを渡す．
	DCInfo.enabledLayerCount = 1; // "VK_LAYER_KHRONOS_validation が使えると仮定してる
	DCInfo.ppEnabledLayerNames = &LAYER_NAME;
	DCInfo.enabledExtensionCount = deviceExtensionCount; //ここでは拡張機能は設定しない
	DCInfo.ppEnabledExtensionNames = deviceExtensionName;
	DCInfo.pEnabledFeatures = nullptr; //ここではオプション機能は設定しない，有効化されたオプション機能はこの変数に書き込まれる．
	//サポートされるオプション機能についてはvkGetPhysicalDeviceFeatures()で確認できる．

	VkDevice logicaldevice;
	result = vkCreateDevice(pPDs[physical_device_index], &DCInfo, nullptr, &logicaldevice);
	if (result != VK_SUCCESS) {
		logger << "failed to check physical devices!!!" << std::endl;
		exit(1);
	}

	vkGetDeviceQueue(logicaldevice, queue_family_index, 0, &m_pImpl->queue);
	m_pImpl->logicalDevice = logicaldevice;

	//デバイスレベルのレイヤ，↑で数を取得しただけで確認してないのでここで確認

	uint32_t numDLayer;
	result = vkEnumerateDeviceLayerProperties(pPDs[physical_device_index], &numDLayer, nullptr);
	if (result != VK_SUCCESS) {
		logger << "failed to get a number of DLayer!!!" << std::endl;
		exit(1);
	}
	logger << "Device Layer size: " << numDLayer << std::endl;
	VkLayerProperties* pDLPs = new VkLayerProperties[numDLayer];
	result = vkEnumerateDeviceLayerProperties(pPDs[physical_device_index], &numDLayer, pDLPs);
	if (result != VK_SUCCESS) {
		logger << "failed to get properties of Layer!!!" << std::endl;
		exit(1);
	}
	for (uint32_t i = 0; i < numDLayer; i++) {
		logger << std::endl;
		logger << "DLayer[" << i << "]" << std::endl;
		logger << "DLayer Name: " << pDLPs[i].layerName << std::endl;
		logger << "Spec Version: " << VK_VERSION_MAJOR(pDLPs[i].specVersion) << "." << VK_VERSION_MINOR(pDLPs[i].specVersion) << std::endl;
		logger << "Impl Version: " << VK_VERSION_MAJOR(pDLPs[i].implementationVersion) << "." << VK_VERSION_MINOR(pDLPs[i].implementationVersion) << std::endl;
		logger << "description: " << pDLPs[i].description << std::endl;
	}
	logger << std::endl;

	//////////////////////////////////////////////////////////////////////
	///////////////////////////コマンド///////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	// コマンドバッファ用のメモリプール（コマンドプール）を作成
	VkCommandPoolCreateInfo CPCI;
	CPCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CPCI.pNext = nullptr;
	CPCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // 各コマンドバッファが再記録を許可
	CPCI.queueFamilyIndex = queue_family_index;

	VkCommandPool CP;
	result = vkCreateCommandPool(logicaldevice, &CPCI, nullptr, &CP);
	if (result != VK_SUCCESS)
	{
		logger << "fail to create command pool!!!" << std::endl;
		exit(1);
	}

	// コマンドバッファを作成
	VkCommandBufferAllocateInfo CBAI;
	CBAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CBAI.pNext = nullptr;
	CBAI.commandPool = CP;
	CBAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // コマンドバッファのレベル，とりあえず一次
	CBAI.commandBufferCount = 2; // 作成するコマンドバッファの数

	result = vkAllocateCommandBuffers(logicaldevice, &CBAI, m_pImpl->CB);
	if (result != VK_SUCCESS)
	{
		logger << "fail to create command buffer!!!" << std::endl;
		exit(1);
	}

	////////////////////////////////////////////////////////////////////////
	///////////////////////////プレゼンテーション///////////////////////////
	////////////////////////////////////////////////////////////////////////

	const int32_t windowWidth = initializeParams.windowSize.x;
	const int32_t windowHeight = initializeParams.windowSize.y;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan no test dayo", nullptr, nullptr);
	m_pImpl->window = window;
	if (window == nullptr)
	{
		logger << "fail to create window!!!" << std::endl;
		exit(1);
	}
	// glfwDestroyWindow(window);

	VkSurfaceKHR surface;
	result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS)
	{
		logger << "fail to create surface" << std::endl;
		exit(1);
	}

	// スワップチェーン作成に必要な情報を取得

	// 指定した物理デバイスのキューファミリが作成したサーフェスのプレゼンテーションをサポートしているかどうかの確認
	VkBool32 presentationSupported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(pPDs[physical_device_index], queue_family_index, surface, &presentationSupported);
	if (presentationSupported != VK_TRUE)
	{
		exit(1);
	}

	// サーフェスの Capability を取得
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pPDs[physical_device_index], surface, &m_pImpl->surfaceCapabilities);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	// 必要なやつを確認
	logger << m_pImpl->surfaceCapabilities.currentExtent.width << " , " << m_pImpl->surfaceCapabilities.currentExtent.height << std::endl;
	m_pImpl->swapChainExtent = m_pImpl->surfaceCapabilities.currentExtent;

	// サーフェスが対応するフォーマットを取得
	uint32_t supportedFormatCount = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(pPDs[physical_device_index], surface, &supportedFormatCount, nullptr);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	logger << "supported format count; " << supportedFormatCount << std::endl;
	VkSurfaceFormatKHR* supportedFormats = new VkSurfaceFormatKHR[supportedFormatCount];
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(pPDs[physical_device_index], surface, &supportedFormatCount, supportedFormats);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	uint32_t selectedSurfaceFormatIndex = 0;
	for (uint32_t i = 0; i < supportedFormatCount; i++)
	{
		if (supportedFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && supportedFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			selectedSurfaceFormatIndex = i;
		}
	}

	m_pImpl->swapChainImageFormat = supportedFormats[selectedSurfaceFormatIndex].format;
	m_pImpl->swapChainColorSpace = supportedFormats[selectedSurfaceFormatIndex].colorSpace;


	// スワップチェーン作成
	VkSwapchainCreateInfoKHR SCCI;
	SCCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SCCI.pNext = nullptr;
	SCCI.flags = 0u; // まだ仕様がない
	SCCI.surface = surface; // ここでサーフェスを渡す
	SCCI.minImageCount = 2; // ダブルバッファ
	SCCI.imageFormat = supportedFormats[selectedSurfaceFormatIndex].format; // フォーマット，
	SCCI.imageColorSpace = supportedFormats[selectedSurfaceFormatIndex].colorSpace;
	SCCI.imageExtent = m_pImpl->surfaceCapabilities.currentExtent;
	SCCI.imageArrayLayers = 1; // ステレオ視を使わないので 1
	SCCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SCCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // イメージが複数キューで共有されるかどうか，とりあえず exclusive にしておく
	SCCI.queueFamilyIndexCount = 0; // ↑が exclusive なのでこの値は無視される
	SCCI.pQueueFamilyIndices = nullptr; // ↑が exclusive なのでこの値は無視される
	SCCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // イメージの変換は不要
	SCCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // アルファ合成はしないので opaque とする
	SCCI.presentMode = VK_PRESENT_MODE_FIFO_KHR; // 垂直同期させる，同期させないときは immediate か mailbox とする
	SCCI.clipped = false; // 見えていない部分であっても処理を走らせる
	SCCI.oldSwapchain = VK_NULL_HANDLE; // oldSwapchain はない，リサイクルしたいときは使える

	VkSwapchainKHR swapchain;
	result = vkCreateSwapchainKHR(logicaldevice, &SCCI, nullptr, &m_pImpl->swapChain);
	if (result != VK_SUCCESS)
	{
		logger << "fail to create swapchain" << std::endl;
		exit(1);
	}

	// イメージへのハンドルを取得
	uint32_t swapchainImageCount; // ↑で指定した 2 は最小の数なので正確な数を取得する
	vkGetSwapchainImagesKHR(logicaldevice, m_pImpl->swapChain, &swapchainImageCount, nullptr);
	logger << "image count: " << swapchainImageCount << std::endl;
	vkGetSwapchainImagesKHR(logicaldevice, m_pImpl->swapChain, &swapchainImageCount, m_pImpl->swapChainImages);

	m_pImpl->swapChainImageCount = swapchainImageCount;
	m_pImpl->swapChainImageViews = new VkImageView[swapchainImageCount];
	for (int i = 0; i < swapchainImageCount; i++)
	{
		VkImageViewCreateInfo IVCI = {};
		IVCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		IVCI.image = m_pImpl->swapChainImages[i];
		IVCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		IVCI.format = supportedFormats[selectedSurfaceFormatIndex].format;
		IVCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		IVCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		IVCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		IVCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		IVCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // color target に使う
		IVCI.subresourceRange.baseMipLevel = 0; // mipmap も multiple layer も使わない
		IVCI.subresourceRange.levelCount = 1;
		IVCI.subresourceRange.baseArrayLayer = 0;
		IVCI.subresourceRange.layerCount = 1;

		result = vkCreateImageView(logicaldevice, &IVCI, nullptr, &m_pImpl->swapChainImageViews[i]);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}






	// ubo
	m_pImpl->CreateBuffer(m_pImpl->uniformBufferImpl1, Renderer::Uniform, 256);
	m_pImpl->GetCpuMemoryPointer(m_pImpl->uniformBufferImpl1, &m_pImpl->mappedData);
	memset(m_pImpl->mappedData, 0, 16);
	(*static_cast<float*>(m_pImpl->mappedData)) = 0.6f;
	//vkUnmapMemory(logicaldevice, uboBufferDeviceMemory);




	// texture
	GpuTextureMemoryImpl textureMemory;
	m_pImpl->CreateImage(128, 128, textureMemory);
	m_pImpl->CreateImageView(textureMemory);



	GpuMemoryImpl stagingBufferMemory;
	m_pImpl->CreateBuffer(stagingBufferMemory, Renderer::Transfer, textureMemory.width * textureMemory.height * 4);

	uint32_t* stagingBufferCpu;
	m_pImpl->GetCpuMemoryPointer(stagingBufferMemory, (void**)&stagingBufferCpu);
	for (uint32_t i = 0; i < 128; i++)
	{
		for (uint32_t j = 0; j < 128; j++)
		{
			if ((i / 8 + j / 8) % 2 == 0)
			{
				stagingBufferCpu[128 * i + j] = 0xFF555555;
			}
			else
			{
				stagingBufferCpu[128 * i + j] = 0xFFFFFFFF;

			}
		}
	}
	m_pImpl->UnmapCpuMemoryPointer(stagingBufferMemory);

	// GPU で転送
	m_pImpl->TransferStagingBufferToImage(stagingBufferMemory, textureMemory);










	m_pImpl->CreateSampler(textureMemory);






































	VkDescriptorSetLayoutBinding layoutBindings[2] = { {},{} };
	layoutBindings[0].binding = 0; //binding = 0
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[1].binding = 1; //binding = 1
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo DSLCI = {};
	DSLCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	DSLCI.bindingCount = 2;
	DSLCI.pBindings = layoutBindings;

	VkDescriptorSetLayout descriptorSetLayout;
	result = vkCreateDescriptorSetLayout(logicaldevice, &DSLCI, nullptr, &descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}





	VkDescriptorPoolSize poolSize[2];
	// ubo用
	poolSize[0] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 10;
	// テクスチャ用
	poolSize[1] = {};
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 10;

	VkDescriptorPoolCreateInfo DPCI = {};
	DPCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	DPCI.poolSizeCount = 2;
	DPCI.pPoolSizes = poolSize;
	DPCI.maxSets = 10;
	DPCI.flags = 0;

	result = vkCreateDescriptorPool(logicaldevice, &DPCI, nullptr, &m_pImpl->descriptorPool);
	if (result != VK_SUCCESS)
	{
		std::cout << "faild to create descriptor pool !!!" << std::endl;
		exit(1);
	}



	VkDescriptorSetAllocateInfo DSAI = {};
	DSAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	DSAI.descriptorPool = m_pImpl->descriptorPool;
	DSAI.descriptorSetCount = 1;
	DSAI.pSetLayouts = &descriptorSetLayout;

	result = vkAllocateDescriptorSets(logicaldevice, &DSAI, &m_pImpl->descriptorSet);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}








	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = m_pImpl->uniformBufferImpl1.buffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = 16;

	VkDescriptorImageInfo descriptorImageInfo = {};
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptorImageInfo.imageView = textureMemory.imageView;
	descriptorImageInfo.sampler = textureMemory.sampler;

	VkWriteDescriptorSet WDS[2] = { {},{} };
	WDS[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WDS[0].dstSet = m_pImpl->descriptorSet;
	WDS[0].dstBinding = 0;
	WDS[0].dstArrayElement = 0;
	WDS[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	WDS[0].descriptorCount = 1;
	WDS[0].pBufferInfo = &descriptorBufferInfo;

	WDS[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WDS[1].dstSet = m_pImpl->descriptorSet;
	WDS[1].dstBinding = 1;
	WDS[1].dstArrayElement = 0;
	WDS[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WDS[1].descriptorCount = 1;
	WDS[1].pImageInfo = &descriptorImageInfo;
	vkUpdateDescriptorSets(logicaldevice, 2, WDS, 0, nullptr);








	VkPipelineInputAssemblyStateCreateInfo PIASCI = {};
	PIASCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	PIASCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PIASCI.primitiveRestartEnable = VK_FALSE; // _STRIP 系の topology のとき，インデックスが 0xFFFF のとき打ち切るかどうか

	// viewport と scissor は動的に決められる用途で使われやすい（そういう想定になっている）
	m_pImpl->viewport.x = 0.0f;
	m_pImpl->viewport.y = 0.0f;
	m_pImpl->viewport.width = m_pImpl->surfaceCapabilities.currentExtent.width;
	m_pImpl->viewport.height = m_pImpl->surfaceCapabilities.currentExtent.height;
	m_pImpl->viewport.maxDepth = 1.0f;
	m_pImpl->viewport.minDepth = 0.0f;

	m_pImpl->scissor.extent = m_pImpl->surfaceCapabilities.currentExtent;
	m_pImpl->scissor.offset = { 0,0 };












	///// render pass の開始

	VkSemaphoreCreateInfo SCI;
	SCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	SCI.pNext = nullptr;
	SCI.flags = 0;

	result = vkCreateSemaphore(logicaldevice, &SCI, nullptr, &m_pImpl->imageAvailableSemaphore[0]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	result = vkCreateSemaphore(logicaldevice, &SCI, nullptr, &m_pImpl->imageAvailableSemaphore[1]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	result = vkCreateSemaphore(logicaldevice, &SCI, nullptr, &m_pImpl->renderFinishedSemaphore[0]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	result = vkCreateSemaphore(logicaldevice, &SCI, nullptr, &m_pImpl->renderFinishedSemaphore[1]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}

	VkFenceCreateInfo FCI;
	FCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FCI.flags = 0;
	FCI.pNext = nullptr;
	result = vkCreateFence(logicaldevice, &FCI, nullptr, &m_pImpl->inFlightFence[0]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}
	result = vkCreateFence(logicaldevice, &FCI, nullptr, &m_pImpl->inFlightFence[1]);
	if (result != VK_SUCCESS)
	{
		exit(1);
	}

}


void Renderer::Draw(DrawParams& drawParams)
{
	uint32_t counter = 0;
	uint32_t frameBufferIndex = 999;

	while (!glfwWindowShouldClose(m_pImpl->window))
	{
		VkResult result;

		glfwPollEvents();

		// このフレームで使う GPU リソースのIdを決定．ただしプレゼン完了のセマフォは frameBufferIndex と一致させるので注意．
		uint32_t gpuIndex = (counter) % 2;
		counter++;

		if (m_pImpl->isProcessing[gpuIndex])
		{
			vkWaitForFences(m_pImpl->logicalDevice, 1, &m_pImpl->inFlightFence[gpuIndex], VK_TRUE, UINT64_MAX);
			vkResetFences(m_pImpl->logicalDevice, 1, &m_pImpl->inFlightFence[gpuIndex]);
			m_pImpl->isProcessing[gpuIndex] = false;
		}

		if (frameBufferIndex == 999)
		{
			result = vkAcquireNextImageKHR(m_pImpl->logicalDevice, m_pImpl->swapChain, UINT64_MAX, m_pImpl->imageAvailableSemaphore[gpuIndex], VK_NULL_HANDLE, &frameBufferIndex);
		}

		// Commandbuffer
		vkResetCommandBuffer(m_pImpl->CB[gpuIndex], 0);

		// コマンドバッファにコマンドを記録
		// コマンドバッファーへのアクセスは同期される必要がある
		// 複数スレッドでひとつのコマンドバッファにコマンドを書き込まないことを保障する or  スレッドごとにコマンドを持つ

		// コマンドバッファの開始とリセット
		VkCommandBufferBeginInfo CBBI;
		CBBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; // 
		CBBI.pNext = nullptr;
		CBBI.flags = 0; // とりあえず 0 にする．マルチパスレンダリングなどでは設定が必要
		CBBI.pInheritanceInfo = nullptr; // 一次のコマンドバッファでは使われない

		result = vkBeginCommandBuffer(m_pImpl->CB[gpuIndex], &CBBI);
		if (result != VK_SUCCESS)
		{
			std::cout << "fail to begin command buffer!!!" << std::endl;
			exit(1);
		}

		VkRenderPassBeginInfo RPBI = {};
		RPBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RPBI.renderPass = m_pImpl->renderPass;
		RPBI.framebuffer = m_pImpl->frameBuffers[frameBufferIndex];
		RPBI.renderArea.offset = { 0, 0 };
		RPBI.renderArea.extent = m_pImpl->swapChainExtent;
		VkClearValue clearColor;
		clearColor.color = { 0.0, 0.0, 0.0, 1.0 };
		RPBI.clearValueCount = 1;
		RPBI.pClearValues = &clearColor;
		// renderpass 開始を記録
		vkCmdBeginRenderPass(m_pImpl->CB[gpuIndex], &RPBI, VK_SUBPASS_CONTENTS_INLINE); // renderpass command は一次コマンドで実行される

		// graphicPipeline を bind
		vkCmdBindPipeline(m_pImpl->CB[gpuIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pImpl->graphicsPipeline);

		vkCmdBindDescriptorSets(m_pImpl->CB[gpuIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pImpl->pipelineLayout, 0, 1, &m_pImpl->descriptorSet, 0, nullptr);

		memset(m_pImpl->mappedData, 0, 16);
		m_pImpl->temp += 0.01;
		(*static_cast<float*>(m_pImpl->mappedData)) = std::sin(m_pImpl->temp) * 0.6;

		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT が真なので flush の必要はないが一応
		VkMappedMemoryRange memoryRange2;
		memoryRange2.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange2.pNext = nullptr;
		memoryRange2.memory = m_pImpl->uniformBufferImpl1.deviceMemory;
		memoryRange2.offset = 0;
		memoryRange2.size = VK_WHOLE_SIZE;
		result = vkFlushMappedMemoryRanges(m_pImpl->logicalDevice, 1, &memoryRange2);
		if (result != VK_SUCCESS)
		{
			std::cout << "faild to flush memory!!!" << std::endl;
			exit(1);
		}


		VkDeviceSize vertexBufferOffsets = 0;
		vkCmdBindVertexBuffers(m_pImpl->CB[gpuIndex], 0, 1, &drawParams.vertexArray[0]->buffer, &vertexBufferOffsets);

		// 動的に決める state を設定
		vkCmdSetViewport(m_pImpl->CB[gpuIndex], 0, 1, &m_pImpl->viewport);
		vkCmdSetScissor(m_pImpl->CB[gpuIndex], 0, 1, &m_pImpl->scissor);

		// draw
		vkCmdDraw(m_pImpl->CB[gpuIndex], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_pImpl->CB[gpuIndex]);
		result = vkEndCommandBuffer(m_pImpl->CB[gpuIndex]);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}

		// submit


		VkPipelineStageFlags waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_pImpl->imageAvailableSemaphore[gpuIndex];
		submitInfo.pWaitDstStageMask = &waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_pImpl->CB[gpuIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_pImpl->renderFinishedSemaphore[frameBufferIndex];

		vkQueueSubmit(m_pImpl->queue, 1, &submitInfo, m_pImpl->inFlightFence[gpuIndex]);

		VkPresentInfoKHR PI = {};
		PI.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PI.waitSemaphoreCount = 1;
		PI.pWaitSemaphores = &m_pImpl->renderFinishedSemaphore[frameBufferIndex];
		PI.swapchainCount = 1;
		PI.pSwapchains = &m_pImpl->swapChain;
		PI.pImageIndices = &frameBufferIndex;
		vkQueuePresentKHR(m_pImpl->queue, &PI);

		m_pImpl->isProcessing[gpuIndex] = true;
		std::cout << "Loop GpuIndex:" << gpuIndex << " ImageIndex:" << frameBufferIndex << std::endl;

		frameBufferIndex = 999;
	}
}


void Renderer::RegisterVertexInputStateImpl3(VertexAttributeLayout* vertexAttributeLayout)
{
	auto* pVertexInputStateImpl = new RendererImpl::VertexInputStateImpl();
	m_pImpl->vertexInputStateImplMap[vertexAttributeLayout->name.data()] = pVertexInputStateImpl;

	pVertexInputStateImpl->bindingDescriptions.resize(1);
	pVertexInputStateImpl->bindingDescriptions[0].binding = vertexAttributeLayout->binding; // binding は vkCmdBindVertexBuffers の指定
	pVertexInputStateImpl->bindingDescriptions[0].stride = vertexAttributeLayout->stride;
	pVertexInputStateImpl->bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 各頂点ごとに切り替える（インスタンスごとに切り替えることも可能）

	pVertexInputStateImpl->attributeDescriptions.resize(vertexAttributeLayout->attributes.size());
	for (int i = 0; i < pVertexInputStateImpl->attributeDescriptions.size(); i++)
	{
		pVertexInputStateImpl->attributeDescriptions[i].binding = vertexAttributeLayout->attributes[i].binding;
		pVertexInputStateImpl->attributeDescriptions[i].location = vertexAttributeLayout->attributes[i].location;
		switch (vertexAttributeLayout->attributes[i].format)
		{
		case VertexAttributeLayout::Attributes::AttributeFormat::Error:
			pVertexInputStateImpl->attributeDescriptions[i].format = VK_FORMAT_UNDEFINED;
			break;
		case VertexAttributeLayout::Attributes::AttributeFormat::Vec2:
			pVertexInputStateImpl->attributeDescriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
			break;
		case VertexAttributeLayout::Attributes::AttributeFormat::Vec3:
			pVertexInputStateImpl->attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
			break;
		case VertexAttributeLayout::Attributes::AttributeFormat::Vec4:
			pVertexInputStateImpl->attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		}
		pVertexInputStateImpl->attributeDescriptions[i].offset = vertexAttributeLayout->attributes[i].offset;
	}

	pVertexInputStateImpl->vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pVertexInputStateImpl->vertexInputInfo.vertexBindingDescriptionCount = pVertexInputStateImpl->bindingDescriptions.size();
	pVertexInputStateImpl->vertexInputInfo.pVertexBindingDescriptions = pVertexInputStateImpl->bindingDescriptions.data();
	pVertexInputStateImpl->vertexInputInfo.vertexAttributeDescriptionCount = pVertexInputStateImpl->attributeDescriptions.size();
	pVertexInputStateImpl->vertexInputInfo.pVertexAttributeDescriptions = pVertexInputStateImpl->attributeDescriptions.data();
}