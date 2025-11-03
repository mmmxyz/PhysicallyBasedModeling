#pragma once

#include "src/renderer/renderer.hpp"
#include "src/utils/geometry/array.hpp"

class GpuMemoryImpl;
class VertexInputStateImpl;

struct BasicVertex
{
	fvec3 position;
	fvec3 normal;
	fvec2 uv;
	fvec4 color;
};

template<class ValueType>
class DrawVertexArray : public ValueArray<ValueType>
{
private:
	GpuMemoryImpl* m_pGpuMemoryImpl = nullptr;
public:

	DrawVertexArray(TypeAllocator<ValueType>& alloc)
		: ValueArray<ValueType>(alloc)
	{
	}

	DrawVertexArray(uint32_t size, TypeAllocator<ValueType>& alloc)
		: ValueArray<ValueType>(size, alloc)
	{
	}
	void gpuInitialize(RendererImpl* pRendererImpl);
	void updateGpuMemory(RendererImpl* pRendererImpl);
	void draw(RendererImpl* pRendererImpl);

	GpuMemoryImpl* getGpuMemoryImpl()
	{
		return m_pGpuMemoryImpl;
	}
};