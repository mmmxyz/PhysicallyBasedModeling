cmake_minimum_required (VERSION 3.14 FATAL_ERROR)

project(rendererLib CXX)

add_library(rendererLib STATIC
	mesh/drawArray.cpp
    renderer.cpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/glfw glfw)

find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
	message("${Vulkan_INCLUDE_DIRS}")
	message("${Vulkan_LIBRARIES}")
	target_include_directories(rendererLib PRIVATE ${Vulkan_INCLUDE_DIRS})
	target_link_libraries(rendererLib PRIVATE ${Vulkan_LIBRARIES})
else()
	message(FATAL_ERROR "Vulkanないよ😭")
endif()

get_target_property(EXE_OUTPUT_PATH rendererLib RUNTIME_OUTPUT_DIRECTORY)
if(NOT EXE_OUTPUT_PATH)
    set(EXE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}") # デフォルトはビルドディレクトリ
endif()

target_link_libraries(rendererLib PRIVATE glfw)

target_include_directories(rendererLib PRIVATE
	../..
)