cmake_minimum_required(VERSION 3.16)
project(WT9011DLL)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.12.0
)
FetchContent_MakeAvailable(pybind11)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

pybind11_add_module(sensor_api_module py_module.cpp)
add_executable(wt9011_demo main.cpp)
target_link_libraries(wt9011_demo PRIVATE pybind11::embed)
