cmake_minimum_required(VERSION 3.25)
project(SoftGpu VERSION 1.0.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(HardwareCommon)
add_subdirectory(RISCV)
add_subdirectory(SoftGpu)
add_subdirectory(VirtualDisplay)
add_subdirectory(SoftGpuRunner)

if(WIN32)
    # add_subdirectory(VirtualBoxDevice)
    add_subdirectory(VirtualBoxDeviceLoader)
endif()
