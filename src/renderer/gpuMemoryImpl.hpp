#pragma once

// この順序でインクルードすること
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

class GpuMemoryImpl
{
public:
	VkBuffer       buffer;
	VkDeviceMemory deviceMemory;
	uint32_t size;

	GpuMemoryImpl()
		: buffer(VK_NULL_HANDLE)
		, deviceMemory(VK_NULL_HANDLE)
	{
	}
};

class GpuTextureMemoryImpl
{
public:
	VkImage        image;
	uint32_t width;
	uint32_t height;
	uint32_t size;
	GpuMemoryImpl gpuMemory;
	VkImageView    imageView;
	VkSampler   sampler;
	GpuTextureMemoryImpl()
		: image(VK_NULL_HANDLE)
		, gpuMemory()
		, imageView(VK_NULL_HANDLE)
	{
	}
};

class DescriptorSetImpl
{
public:
	int set;
	VkDescriptorSet descriptorSet;
};