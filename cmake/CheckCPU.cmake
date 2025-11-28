function(CheckTargetArch DETECTED_ARCHS)
    set(search_archs ARM32 ARM64 ARM64EC RISCV32 RISCV64 X86 X64)
    set(detected_archs)

    # If we're on an Apple platform, the check is fairly straightforward,
    # mostly just being ARM64, and legacy x86_64 (we're not supporting PowerPC).
    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
        foreach(arch IN LISTS search_archs)
            # Set all architectures to disabled.
            set(GS_ARCH_${arch} "0" PARENT_SCOPE)
        endforeach()

        # Iterate through the enabled architectures in CMAKE_OSX_ARCHITECTURES.
        #   This applies to macOS, iOS, tvOS, visionOS, watchOS, etc, so this is
        # technically XNU architectures.
        foreach(xnu_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
            if(xnu_arch STREQUAL "x86_64")
                set(GS_ARCH_X64 "1" PARENT_SCOPE)
                list(APPEND detected_archs "X64")
            elseif(xnu_arch STREQUAL "arm64")
                set(GS_ARCH_ARM64 "1" PARENT_SCOPE)
                list(APPEND detected_archs "ARM64")
            endif()
        endforeach()

        set("${DETECTED_ARCHS}" "${detected_archs}" PARENT_SCOPE)

        return()
    endif()

    foreach(arch IN LISTS search_archs)
        if(GS_ARCH_${arch})
            list(APPEND detected_archs "${arch}")
        endif()
    endforeach()

    if(detected_archs)
        set("${DETECTED_ARCHS}" "${detected_archs}" PARENT_SCOPE)
        return()
    endif()

    set(arch_check_ARM32 "defined(__arm__) || defined(_M_ARM)")
    set(arch_check_ARM64 "defined(__aarch64__) || defined(_M_ARM64)")
    set(arch_check_ARM64EC "defined(_M_ARM64EC)")
    set(arch_check_RISCV32 "defined(__riscv) && defined(__riscv_xlen) && __riscv_xlen == 32")
    set(arch_check_RISCV64 "defined(__riscv) && defined(__riscv_xlen) && __riscv_xlen == 64")
    set(arch_check_X86 "defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) ||defined( __i386) || defined(_M_IX86)")
    set(arch_check_X64 "(defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)) && !defined(_M_ARM64EC)")

    set(src_vars "")
    set(src_main "")

    foreach(arch IN LISTS search_archs)
        set(detected_${arch} "0")

        string(APPEND src_vars "
#if ${arch_check_${arch}}
    #define ARCH_${arch} \"1\"
#else
    #define ARCH_${arch} \"0\"
#endif

const char* arch_${arch} = \"COMPILED_ARCH<${arch}=\" ARCH_${arch} \">\";
        ")

        string(APPEND src_main "
result += arch_${arch}[argc];")
    endforeach()

    set(src_arch_detect "
        ${src_vars}
int main(int argc, char* argv[])
{
    int result = 0;
    (void) argv;
    ${src_main}
    return result;
}
    ")

    if(CMAKE_C_COMPILER)
        set(ext ".c")
    elseif(CMAKE_CXX_COMPILER)
        set(ext ".cpp")
    else()
        enable_language(C)
        set(ext ".c")
    endif()

    set(path_src_arch_detect "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/GS_detect_arch${ext}")
    file(WRITE "${path_src_arch_detect}" "${src_arch_detect}")
    set(path_dir_arch_detect "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/GS_detect_arch")
    set(path_bin_arch_detect "${path_dir_arch_detect}/bin")

    set(msg "Detecting Target CPU Architecture")
    message(STATUS "${msg}")

    include(CMakePushCheckState)

    set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

    cmake_push_check_state(RESET)

    try_compile(GS_ARCH_CHECK_ALL
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/GS_detect_arch"
        SOURCES "${path_src_arch_detect}"
        COPY_FILE "${path_bin_arch_detect}"
    )

    cmake_pop_check_state()

    if(NOT GS_ARCH_CHECK_ALL)
        message(STATUS "${msg} - <ERROR>")
        message(WARNING "Failed to compile source detecting the target CPU architecture")
    else()
        set(re "COMPILED_ARCH<([_a-zA-Z0-9]+)=([01])>")
        file(STRINGS "${path_bin_arch_detect}" compiled_archs REGEX "${re}")

        foreach(compiled_arch IN LISTS compiled_archs)
            string(REGEX MATCH "${re}" _ "${compiled_arch}")

            if(NOT "${CMAKE_MATCH_1}" IN_LIST search_archs)
                message(WARNING "Unknown architecture: \"${CMAKE_MATCH_1}\"")
                continue()
            endif()

            set(arch "${CMAKE_MATCH_1}")
            set(arch_active "${CMAKE_MATCH_2}")
            set(detected_${arch} "${arch_active}")
        endforeach()

        foreach(arch IN LISTS search_archs)
            if(detected_${arch})
                list(APPEND detected_archs ${arch})
            endif()
        endforeach()
    endif()

    if(detected_archs)
        foreach(arch IN LISTS search_archs)
            set("GS_ARCH_${arch}" "${detected_${arch}}" CACHE BOOL "Detected architecture ${arch}")
        endforeach()

        message(STATUS "${msg} - ${detected_archs}")
    else()
        include(CheckCSourceCompiles)
        cmake_push_check_state(RESET)
        foreach(arch IN LISTS search_archs)
            if(NOT detected_archs)
                set(cache_variable "GS_ARCH_${arch}")
                set(test_src "
                    int main(int argc, char* argv[])
                    {
                        #if ${arch_check_${arch}}
                            return 0;
                        #else
                            fail
                        #endif
                    }
                ")

                check_c_source_compiles("${test_src}" "${cache_variable}")

                if(${cache_variable})
                    set(GS_ARCH_${arch} "1" CACHE BOOL "Detected architecture ${arch}")
                    set(detected_archs ${arch})
                else()
                    set(GS_ARCH_${arch} "0" CACHE BOOL "Detected architecture ${arch}")
                endif()
            endif()
        endforeach()
        cmake_pop_check_state()
    endif()

    set("${DETECTED_ARCHS}" "${detected_archs}" PARENT_SCOPE)
endfunction()
