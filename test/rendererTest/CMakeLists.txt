cmake_minimum_required(VERSION 3.10)

project(rendererTest CXX)

# シェーダーバイナリのディレクトリ
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/ShaderBinary")

# シェーダーソースファイル
set(SHADER_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/shader/test.slang"
	"${CMAKE_CURRENT_SOURCE_DIR}/shader/shadow.slang"
	"${CMAKE_CURRENT_SOURCE_DIR}/shader/gbuffer.slang"
	"${CMAKE_CURRENT_SOURCE_DIR}/shader/lighting.slang"
)

# シェーダーバイナリファイル（OUTPUTとして指定）
set(SHADER_BINARIES
	"${SHADER_BINARY_DIR}/test.vert.spv"
	"${SHADER_BINARY_DIR}/test.frag.spv"
	"${SHADER_BINARY_DIR}/shadow.vert.spv"
	"${SHADER_BINARY_DIR}/gbuffer.vert.spv"
	"${SHADER_BINARY_DIR}/gbuffer.frag.spv"
	"${SHADER_BINARY_DIR}/lighting.vert.spv"
	"${SHADER_BINARY_DIR}/lighting.frag.spv"
)

add_executable(rendererTest main.cpp ${SHADER_BINARIES})

target_compile_definitions(rendererTest PRIVATE RESOURCE_DIR="${RESOURCE_DIR_PATH_CMAKE}")
target_compile_definitions(rendererTest PRIVATE SHADER_BINARY_DIR="${SHADER_BINARY_DIR}")

target_include_directories(rendererTest PRIVATE
	../..
)

target_link_libraries(rendererTest PRIVATE
	rendererLib
)

get_target_property(EXE_OUTPUT_PATH rendererTest RUNTIME_OUTPUT_DIRECTORY)
if(NOT EXE_OUTPUT_PATH)
    set(EXE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}") # デフォルトはビルドディレクトリ
endif()

if(WIN32)
	add_custom_command(
		OUTPUT ${SHADER_BINARIES}
		COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/shader/build.bat" "${EXE_OUTPUT_PATH}"
		DEPENDS ${SHADER_SOURCES}
		COMMENT "Compiling shaders: test, shadow, gbuffer, lighting"
	)
elseif(UNIX)
	add_custom_command(
		TARGET rendererTest
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shader/build.sh "${EXE_OUTPUT_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/slangBinary/bin"
		COMMENT "Compiling shaders for Linux"
	)
endif()
