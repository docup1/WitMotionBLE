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

# ---------- Сборка библиотеки ----------

add_library(wt9011lib SHARED py_module.cpp)
target_link_libraries(wt9011lib PRIVATE pybind11::embed Python3::Python)

# ---------- Копирование Python-файлов ----------

# Собираем все .py файлы из ../lib в runtime директорию
file(GLOB LIB_PY_FILES "${CMAKE_SOURCE_DIR}/../lib/*.py")
foreach(PY_FILE ${LIB_PY_FILES})
    configure_file(${PY_FILE} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()

# Добавляем путь в sys.path
target_compile_definitions(wt9011lib PRIVATE PYTHON_MODULE_PATH="${CMAKE_CURRENT_BINARY_DIR}")

# ---------- Qt-пример (опционально) ----------

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_executable(wt9011_demo main.cpp)
target_link_libraries(wt9011_demo PRIVATE Qt6::Core Qt6::Widgets pybind11::embed Python3::Python)

# Для Qt приложения также нужны Python-файлы
foreach(PY_FILE ${LIB_PY_FILES})
    configure_file(${PY_FILE} ${CMAKE_CURRENT_BINARY_DIR}/wt9011_demo COPYONLY)
endforeach()
