cmake_minimum_required(VERSION 3.25)
project(HardwareCommon VERSION 1.0.0 LANGUAGES CXX C)

# This is a helper utility for generating the folder layout in VS.
include(GenVsFilters)
include(SetCompileFlags)

file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h" "include/*.inl")

add_library(${PROJECT_NAME} INTERFACE ${HEADERS})

# Generate the "filters" for VS. This basically just creates an intuitive folder layout.
GenVsFilters(HEADERS)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET "HEADERS" BASE_DIRS "include" FILES ${HEADERS})

# Set the include directory.
target_include_directories(${PROJECT_NAME} INTERFACE include)

find_package(TauUtils REQUIRED)

target_link_libraries(${PROJECT_NAME} INTERFACE tauutils::tauutils)

SetCompileFlags(${PROJECT_NAME} INTERFACE INTERFACE)
