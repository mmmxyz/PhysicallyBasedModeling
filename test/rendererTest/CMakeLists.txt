cmake_minimum_required(VERSION 3.10)

project(rendererTest CXX)

add_executable(rendererTest main.cpp "${CMAKE_CURRENT_BINARY_DIR}/ShaderBinary/test.vert.spv")

target_compile_definitions(rendererTest PRIVATE RESOURCE_DIR="${RESOURCE_DIR_PATH_CMAKE}")

target_include_directories(rendererTest PRIVATE
	../..
)

target_link_libraries(rendererTest PRIVATE
	rendererLib
)

get_target_property(EXE_OUTPUT_PATH rendererTest RUNTIME_OUTPUT_DIRECTORY)
if(NOT EXE_OUTPUT_PATH)
    set(EXE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}") # �f�t�H���g�̓r���h�f�B���N�g��
endif()

if(WIN32)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShaderBinary/test.vert.spv" "${CMAKE_CURRENT_BINARY_DIR}/ShaderBinary/shadow.vert.spv"
		COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/shader/build.bat" "${EXE_OUTPUT_PATH}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shader/test.slang" "${CMAKE_CURRENT_SOURCE_DIR}/shader/shadow.slang" )
elseif(UNIX)
	add_custom_command(TARGET rendererTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shader/build.sh "${EXE_OUTPUT_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/slangBinary/bin")
endif()
