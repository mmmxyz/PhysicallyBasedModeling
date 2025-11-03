#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

struct HeapDebugInfo
{
	std::string debugFlag;
};

class RootAllocator
{
public:
	void* allocate(std::size_t byteSize, HeapDebugInfo debugInfo)
	{
		std::cout << debugInfo.debugFlag << std::endl;
		return std::malloc(byteSize);
	}

	void deallocate(void * p)
	{
		std::free(p);
	}
};

template<class T>
class TypeAllocator
{
public:
	using value_type = T;
	RootAllocator* m_pRootAllocator = nullptr;
	HeapDebugInfo m_debugInfo;

	TypeAllocator(RootAllocator* pRootAllocator, std::string debugName)
	{
		m_pRootAllocator = pRootAllocator;
		m_debugInfo.debugFlag = debugName;
	}

	template<class U>
	TypeAllocator(const TypeAllocator<U>& allocator)
	{
		this->m_pRootAllocator = allocator.m_pRootAllocator;
		this->m_debugInfo = allocator.m_debugInfo;
	}

	T* allocate(std::size_t size)
	{
		return static_cast<T*>(m_pRootAllocator->allocate(sizeof(T) * size, m_debugInfo));
	}

	void deallocate(T* p, std::size_t size)
	{
		m_pRootAllocator->deallocate(p);
	}
};

template<class T, class U>
bool operator==(const TypeAllocator<T>&, const TypeAllocator<U>&) {
	return true;
}

template<class T, class U>
bool operator!=(const TypeAllocator<T>&, const TypeAllocator<U>&) {
	return false;
}