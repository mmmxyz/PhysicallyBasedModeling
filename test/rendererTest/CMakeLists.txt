cmake_minimum_required(VERSION 3.10)

project(rendererTest CXX)

add_executable(rendererTest main.cpp)

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
	add_custom_command(TARGET rendererTest POST_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shader/build.bat "${EXE_OUTPUT_PATH}")
elseif(UNIX)
	add_custom_command(TARGET rendererTest POST_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shader/build.sh "${EXE_OUTPUT_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/slangBinary/bin")
endif()
