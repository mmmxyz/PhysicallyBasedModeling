#pragma once

#include <vector>

#include "src/utils/mathfunc/mathfunc.hpp"
#include "src/utils/memory/allocator.hpp"

template<class ValueType>
class ValueArray
{
protected:
	RootAllocator RootAllocator; // å„Ç≈è¡Ç∑

	std::vector<ValueType, TypeAllocator<ValueType> > buffer;
public:

	// å„Ç≈è¡Ç∑
	ValueArray()
		: buffer(*(new TypeAllocator<ValueType>(&RootAllocator, "aa")))
	{

	}

	ValueArray(TypeAllocator<ValueType>& alloc)
		: buffer(alloc)
	{
	}

	ValueArray(uint32_t size, TypeAllocator<ValueType>& alloc)
		: buffer(alloc)
	{
		buffer.resize(size);
	}

	ValueType& operator[](const uint32_t& index)
	{
		return buffer[index];
	}

	const ValueType& operator[](const uint32_t& index) const
	{
		return buffer[index];
	}

	uint32_t size()
	{
		return buffer.size();
	}

	ValueType* data()
	{
		return buffer.data();
	}

	void resize(uint32_t newSize)
	{
		buffer.resize(newSize);
	}

	void push_back(const ValueType& value)
	{
		buffer.push_back(value);
	}
};