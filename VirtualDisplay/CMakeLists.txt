cmake_minimum_required(VERSION 3.25)
project(VirtualDisplay VERSION 1.0.0 LANGUAGES CXX C)

# This is a helper utility for generating the folder layout in VS.
include(GenVsFilters)
include(SetCompileFlags)
include(CheckCompiler)
include(CheckCPU)

CheckCompiler()
CheckTargetArch(GS_ARCHS)


file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.rc")
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h" "include/*.inl")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

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
find_package(glm REQUIRED)
find_package(EASTL REQUIRED)
find_package(Vulkan REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)

add_dependencies(${PROJECT_NAME} SDL3::SDL3)

target_link_libraries(${PROJECT_NAME} PUBLIC tauutils::tauutils glm::glm SDL3::SDL3 EASTL::EASTL Vulkan::Vulkan vulkan-memory-allocator::vulkan-memory-allocator)

if(Vulkan_MoltenVK_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::MoltenVK)
endif()

if(APPLE)
    # Enable Metal for Vulkan on MacOS
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DVK_USE_PLATFORM_METAL_EXT)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC -DGLM_ENABLE_EXPERIMENTAL -DEA_PRAGMA_ONCE_SUPPORTED)

SetCompileFlags(${PROJECT_NAME} PUBLIC PRIVATE)
