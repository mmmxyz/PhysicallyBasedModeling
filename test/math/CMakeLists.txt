cmake_minimum_required(VERSION 3.10)

project(math CXX)

add_executable(mathProgram main.cpp)

target_include_directories(mathProgram PRIVATE
	../..
)

