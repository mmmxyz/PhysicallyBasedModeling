
#include "src/renderer/mesh/drawArray.hpp"
#include "src/utils/mathfunc/mathfunc.hpp"
#include "src/utils/geometry/array.hpp"
#include "src/utils/memory/allocator.hpp"


int main()
{
	RootAllocator RootAllocator;

	auto sAllocator = TypeAllocator<int>(&RootAllocator, "");

	auto* temp = sAllocator.allocate(100);
	sAllocator.deallocate(temp, 100);

	TypeAllocator<BasicVertex> vertexAllocator(&RootAllocator, "aa");
	ValueArray<BasicVertex> tempVertex(200, vertexAllocator);

	DrawVertexArray<BasicVertex> drawArray(300, vertexAllocator);

	return 0;
}