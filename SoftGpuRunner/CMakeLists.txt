cmake_minimum_required(VERSION 3.25)
project(SoftGpuRunner VERSION 1.0.0 LANGUAGES CXX C)

# This is a helper utility for generating the folder layout in VS.
include(GenVsFilters)
include(SetCompileFlags)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.rc")
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h" "include/*.inl")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Generate the "filters" for VS. This basically just creates an intuitive folder layout.
GenVsFilters(SOURCES)
GenVsFilters(HEADERS)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "include" FILES ${HEADERS})

# Set the include directory.
target_include_directories(${PROJECT_NAME} PUBLIC include)
# Set the source directory.
target_include_directories(${PROJECT_NAME} PRIVATE src)

find_package(TauUtils REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC tauutils::tauutils HardwareCommon RISCV SoftGpu VirtualDisplay)

SetCompileFlags(${PROJECT_NAME} PUBLIC PRIVATE)
