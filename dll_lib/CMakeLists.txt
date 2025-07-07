cmake_minimum_required(VERSION 3.10)
project(WT9011DLL)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

add_library(wt9011_dll SHARED wt9011_dll.cpp)
target_link_libraries(wt9011_dll PRIVATE pybind11::embed)
set_target_properties(wt9011_dll PROPERTIES SUFFIX ".dll")