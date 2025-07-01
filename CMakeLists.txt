cmake_minimum_required(VERSION 3.14)
project(wt9011_ble LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)

add_executable(wt9011_ble
    src/main.cpp
    src/BleDeviceManager.cpp
    src/WT9011Parser.cpp
)

target_include_directories(wt9011_ble PRIVATE src)

target_link_libraries(wt9011_ble PRIVATE Qt6::Core Qt6::Bluetooth)
