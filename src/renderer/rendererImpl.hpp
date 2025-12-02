#pragma once

// この順序でインクルードすること
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <unordered_map>

#include "src/renderer/mesh/drawArray.hpp"

#include "src/renderer/renderer.hpp"
#include "src/renderer/gpuMemoryImpl.hpp"

// TODO: 実装分離
class RendererImpl
{
public:
	GLFWwindow* window;
	VkSwapchainKHR swapChain;
	VkSemaphore imageAvailableSemaphore[2]; // ダブルバッファ
	VkSemaphore renderFinishedSemaphore[2];
	VkFence inFlightFence[2];
	VkCommandBuffer CB[2]; // ダブルバッファ

	VkQueue queue;
	VkExtent2D swapChainExtent = {};
	uint32_t swapChainImageCount;
	VkImage swapChainImages[2];
	VkImageView* swapChainImageViews;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkFormat swapChainImageFormat;
	VkColorSpaceKHR swapChainColorSpace;

	bool isProcessing[2] = { false, false };

	VkViewport viewport = {}; // フレームバッファのどこにマップされるか
	VkRect2D scissor = { };

	VkDescriptorPool descriptorPool;

	class GraphicsPipelineImpl
	{
	public:
		std::string name;
		VkPipeline graphicsPipeline;
		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
		std::vector<VkDescriptorSetLayout> pDescriptorSetLayout;
		VkFramebuffer * pFrameBuffer;
	};

	class VertexInputStateImpl
	{
	public:
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VertexInputStateImpl()
			: vertexInputInfo{}
			, bindingDescriptions{}
			, attributeDescriptions{}
		{
		}
	};

	std::unordered_map<std::string, GraphicsPipelineImpl*> graphicsPipelineMap;
	std::unordered_map<std::string, VkShaderModule> shaderModuleMap;
	std::unordered_map<std::string, VertexInputStateImpl*> vertexInputStateImplMap;

	VkDevice logicalDevice;
	uint32_t memory_type_index;
	uint32_t memory_type_index_host_local;

	void CreateBuffer(GpuMemoryImpl& gpuMemoryImpl, Renderer::BufferCreateUsage usage, size_t size)
	{
		VkBufferUsageFlags usageFlag;
		switch (usage)
		{
		case Renderer::BufferCreateUsage::Uniform:
			usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case Renderer::BufferCreateUsage::Vertex:
			usageFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case Renderer::BufferCreateUsage::Transfer:
			usageFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;
		}

		VkBufferCreateInfo uboBufferCreateInfo;
		uboBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		uboBufferCreateInfo.size = size;
		uboBufferCreateInfo.usage = usageFlag;
		uboBufferCreateInfo.flags = 0; // 未使用
		uboBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //
		uboBufferCreateInfo.pNext = nullptr;

		CreateBuffer(uboBufferCreateInfo, gpuMemoryImpl);

		VkMemoryRequirements uboMemoryRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, gpuMemoryImpl.buffer, &uboMemoryRequirements);


		VkMemoryAllocateInfo MAInfo3;
		MAInfo3.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MAInfo3.pNext = nullptr; //拡張機能を使わないのならnullptr
		MAInfo3.allocationSize = uboMemoryRequirements.size;
		MAInfo3.memoryTypeIndex = memory_type_index;

		size = uboBufferCreateInfo.size;

		AllocateDeviceMemory(MAInfo3, gpuMemoryImpl);

		vkBindBufferMemory(logicalDevice, gpuMemoryImpl.buffer, gpuMemoryImpl.deviceMemory, 0);
	}

	void GetCpuMemoryPointer(GpuMemoryImpl& gpuMemoryImpl, void** ppData)
	{
		VkResult result = vkMapMemory(logicalDevice, gpuMemoryImpl.deviceMemory, 0, VK_WHOLE_SIZE, 0, ppData);
		if (result != VK_SUCCESS)
		{
			std::cout << "faild to map memory!!!" << std::endl;
			exit(1);
		}
	}

	void UnmapCpuMemoryPointer(GpuMemoryImpl& gpuMemoryImpl)
	{
		vkUnmapMemory(logicalDevice, gpuMemoryImpl.deviceMemory);
	}

	void CreateBuffer(VkBufferCreateInfo& createInfo, GpuMemoryImpl& gpuMemoryImpl)
	{
		VkResult result = vkCreateBuffer(logicalDevice, &createInfo, nullptr, &gpuMemoryImpl.buffer);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}

	void AllocateDeviceMemory(VkMemoryAllocateInfo allocateInfo, GpuMemoryImpl& gpuMemoryImpl)
	{
		VkResult result = vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, &gpuMemoryImpl.deviceMemory);
		if (result != VK_SUCCESS)
		{
			std::cout << "faild to allocate device memory!!!" << std::endl;
			exit(1);
		}
	}

	void CreateImage(uint32_t width, uint32_t height, GpuTextureMemoryImpl& gpuTextureMemoryImpl)
	{
		gpuTextureMemoryImpl.width = width;
		gpuTextureMemoryImpl.height = height;

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &gpuTextureMemoryImpl.image);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}

		VkMemoryRequirements textureMemoryRequirements;
		vkGetImageMemoryRequirements(logicalDevice, gpuTextureMemoryImpl.image, &textureMemoryRequirements);

		gpuTextureMemoryImpl.size = textureMemoryRequirements.size;

		VkMemoryAllocateInfo textureImageAllocateInfo = {};
		textureImageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		textureImageAllocateInfo.allocationSize = textureMemoryRequirements.size;
		textureImageAllocateInfo.memoryTypeIndex = memory_type_index_host_local; // テクスチャはホストローカルに置いたほうが良い

		AllocateDeviceMemory(textureImageAllocateInfo, gpuTextureMemoryImpl.gpuMemory);

		vkBindImageMemory(logicalDevice, gpuTextureMemoryImpl.image, gpuTextureMemoryImpl.gpuMemory.deviceMemory, 0);
	}

	void CreateImageView(GpuTextureMemoryImpl& gpuTextureMemoryImpl)
	{
		VkImageViewCreateInfo textureImageVCI = {};
		textureImageVCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		textureImageVCI.image = gpuTextureMemoryImpl.image;
		textureImageVCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		textureImageVCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		textureImageVCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		textureImageVCI.subresourceRange.baseMipLevel = 0;
		textureImageVCI.subresourceRange.levelCount = 1;
		textureImageVCI.subresourceRange.baseArrayLayer = 0;
		textureImageVCI.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(logicalDevice, &textureImageVCI, nullptr, &gpuTextureMemoryImpl.imageView);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}

	void CreateSampler(GpuTextureMemoryImpl& gpuTextureMemoryImpl)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkResult result = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &gpuTextureMemoryImpl.sampler);
		if (result != VK_SUCCESS)
		{
			exit(1);
		}
	}

	void TransferStagingBufferToImage(GpuMemoryImpl& stagingBufferMemory, GpuTextureMemoryImpl& textureMemory)
	{
		// GPU で転送

		VkCommandBufferBeginInfo textureCBBI = {};
		textureCBBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		textureCBBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(CB[0], &textureCBBI);



		VkImageMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = textureMemory.image;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memoryBarrier.subresourceRange.baseMipLevel = 0;
		memoryBarrier.subresourceRange.levelCount = 1;
		memoryBarrier.subresourceRange.layerCount = 1;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(CB[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);


		VkBufferImageCopy imageCopy = {};
		imageCopy.bufferOffset = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.bufferImageHeight = 0;
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.layerCount = 1;
		imageCopy.imageOffset = { 0,0,0 };
		imageCopy.imageExtent = { 128, 128,1 };

		vkCmdCopyBufferToImage(CB[0], stagingBufferMemory.buffer, textureMemory.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);


		memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(CB[0], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);


		vkEndCommandBuffer(CB[0]);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &CB[0];

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

		vkQueueWaitIdle(queue);

	}

};