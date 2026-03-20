#pragma once

#include "src/renderer/renderer.hpp"
#include "src/utils/memory/array.hpp"

class GpuMemoryImpl;
class VertexInputStateImpl;

struct BasicVertex
{
	fvec3 position;
	fvec3 normal;
	fvec2 uv;
	fvec4 color;
	fvec4 tangent; // xyz = tangent, w = handedness (Å}1)
	float roughness;
};

template<class ValueType>
class DrawVertexArray : public ValueArray<ValueType>
{
private:
	GpuMemoryImpl* m_pGpuMemoryImpl = nullptr;
	bool m_isIndexBufffer = false;
public:

	DrawVertexArray(TypeAllocator<ValueType>& alloc, bool isIndexBuffer = false)
		: ValueArray<ValueType>(alloc), m_isIndexBufffer(isIndexBuffer)
	{
	}

	DrawVertexArray(uint32_t size, TypeAllocator<ValueType>& alloc, bool isIndexBuffer = false)
		: ValueArray<ValueType>(size, alloc), m_isIndexBufffer(isIndexBuffer)
	{
	}

	int32_t constexpr getValueTypeSize()
	{
		return sizeof(ValueType);
	}

	void gpuInitialize(RendererImpl* pRendererImpl);
	void updateGpuMemory(RendererImpl* pRendererImpl);
	void draw(RendererImpl* pRendererImpl);

	GpuMemoryImpl* getGpuMemoryImpl()
	{
		return m_pGpuMemoryImpl;
	}
};