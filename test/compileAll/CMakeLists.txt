cmake_minimum_required(VERSION 3.10)

project(CompileAll CXX)

add_executable(CompileAll main.cpp)

target_include_directories(CompileAll PRIVATE
	../..
)

target_link_libraries(CompileAll PRIVATE
	rendererLib
)