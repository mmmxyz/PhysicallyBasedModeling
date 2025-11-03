cmake_minimum_required (VERSION 3.14 FATAL_ERROR)

project ("PhysicallyBasedModeling")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON FORCE)

if(UNIX)
	set(COMMON_CXX_FLAGS "-Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif(WIN32 AND MSVC)
	set(COMMON_CXX_FLAGS "/W4")
	if(CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" FORCE)
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
endif()

add_subdirectory(src/renderer)
add_subdirectory(test/rendererTest)
add_subdirectory(test/compileAll)
add_subdirectory(test/math)