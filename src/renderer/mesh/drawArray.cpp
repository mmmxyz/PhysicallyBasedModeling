
#include <cstring>

// ���̏����ŃC���N���[�h���邱��
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "src/renderer/rendererImpl.hpp"
#include "src/renderer/gpuMemoryImpl.hpp"
#include "src/renderer/mesh/drawArray.hpp"

template class DrawVertexArray<int32_t>;
template class DrawVertexArray<BasicVertex>;

template<class ValueType>
void DrawVertexArray<ValueType>::gpuInitialize(RendererImpl* pRendererImpl)
{
	VkDevice& logicaldevice = pRendererImpl->logicalDevice;

	m_pGpuMemoryImpl = new GpuMemoryImpl();

	pRendererImpl->CreateBuffer(*m_pGpuMemoryImpl, (m_isIndexBufffer) ? Renderer::BufferCreateUsage::VertexIndex : Renderer::BufferCreateUsage::Vertex, this->size() * getValueTypeSize());
}

template<class ValueType>
void DrawVertexArray<ValueType>::updateGpuMemory(RendererImpl* pRendererImpl)
{
	VkDevice& logicaldevice = pRendererImpl->logicalDevice;

	void* mappedData;
	VkResult result = vkMapMemory(logicaldevice, m_pGpuMemoryImpl->deviceMemory, 0, VK_WHOLE_SIZE, 0, &mappedData);
	if (result != VK_SUCCESS)
	{
		std::cout << "faild to map device memory!!!" << std::endl;
		exit(1);
	}

	std::memcpy(mappedData, this->data(), this->size() * getValueTypeSize());

	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ���^�Ȃ̂� flush �̕K�v�͂Ȃ����ꉞ
	VkMappedMemoryRange memoryRange;
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.pNext = nullptr;
	memoryRange.memory = m_pGpuMemoryImpl->deviceMemory;
	memoryRange.offset = 0;
	memoryRange.size = VK_WHOLE_SIZE;
	result = vkFlushMappedMemoryRanges(logicaldevice, 1, &memoryRange);
	if (result != VK_SUCCESS)
	{
		std::cout << "faild to flush memory!!!" << std::endl;
		exit(1);
	}

	// unmap
	vkUnmapMemory(logicaldevice, m_pGpuMemoryImpl->deviceMemory);
}