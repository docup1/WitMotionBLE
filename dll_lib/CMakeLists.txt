cmake_minimum_required(VERSION 3.16)
project(WT9011Lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------- Python & pybind11 ----------
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.12.0
)
FetchContent_MakeAvailable(pybind11)

# ---------- Собираем основную DLL ----------
add_library(wt9011lib SHARED py_module.cpp)
target_link_libraries(wt9011lib PRIVATE pybind11::embed Python3::Python)

# Копируем .py файлы
file(GLOB LIB_PY_FILES "${CMAKE_SOURCE_DIR}/../lib/*.py")
foreach(PY_FILE ${LIB_PY_FILES})
    configure_file(${PY_FILE} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()

# ---------- Qt C++ демо (опционально) ----------
option(BUILD_QT_DEMO "Build Qt demo app" ON)

if(BUILD_QT_DEMO)
    find_package(Qt6 COMPONENTS Core Widgets QUIET)

    if(Qt6_FOUND)
        add_executable(wt9011_demo main.cpp)
        target_link_libraries(wt9011_demo PRIVATE Qt6::Core Qt6::Widgets pybind11::embed Python3::Python)
        foreach(PY_FILE ${LIB_PY_FILES})
            configure_file(${PY_FILE} ${CMAKE_CURRENT_BINARY_DIR}/wt9011_demo COPYONLY)
        endforeach()
    else()
        message(WARNING "Qt6 not found. Qt demo will not be built.")
    endif()
endif()
