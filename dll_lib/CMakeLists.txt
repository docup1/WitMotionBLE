cmake_minimum_required(VERSION 3.16)
project(WT9011DLL)

set(CMAKE_CXX_STANDARD 17)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

pybind11_add_module(sensor_api_module py_module.cpp)
add_executable(wt9011_demo main.cpp)
target_link_libraries(wt9011_demo PRIVATE pybind11::embed)
