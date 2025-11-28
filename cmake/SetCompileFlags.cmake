function(SetCompileFlags ProjectName PublicType PrivateType)
    # We use this to check for some compiler flags, mostly to disable warnings.
    include(CheckCCompilerFlag)

    # Set C++20
    target_compile_features(${ProjectName} ${PublicType} cxx_std_23)

    get_property(TARGET_TYPE TARGET "${ProjectName}" PROPERTY TYPE)

    if(IS_CLANG OR IS_GCC OR IS_ICC)
        if(MSVC_CLANG)
            target_compile_options(${ProjectName} ${PrivateType} "/W3")
        else()
            check_c_compiler_flag(-Wall HAS_W_ALL)
            if(HAS_W_ALL)
                target_compile_options(${ProjectName} ${PrivateType} "-Wall")
            endif()
        endif()

        check_c_compiler_flag(-Wextra HAS_W_EXTRA)
        if(HAS_W_EXTRA)
            target_compile_options(${ProjectName} ${PrivateType} "-Wextra")
        endif()

        check_c_compiler_flag(-pedantic HAS_PEDANTIC)
        if(HAS_PEDANTIC)
            target_compile_options(${ProjectName} ${PrivateType} "-pedantic")
        else()
            check_c_compiler_flag(-Wpedantic HAS_W_PEDANTIC)
            if(HAS_W_PEDANTIC)
                target_compile_options(${ProjectName} ${PrivateType} "-Wpedantic")
            endif()
        endif()

        check_c_compiler_flag(-Wundef HAS_W_UNDEF)
        if(HAS_W_UNDEF)
            target_compile_options(${ProjectName} ${PrivateType} "-Wundef")
        endif()

        check_c_compiler_flag(-Wfloat-conversion HAS_W_FLOAT_CONVERSION)
        if(HAS_W_FLOAT_CONVERSION)
            target_compile_options(${ProjectName} ${PrivateType} "-Wfloat-conversion")
        endif()

        check_c_compiler_flag(-Wshadow HAS_W_SHADOW)
        if(HAS_W_SHADOW)
            target_compile_options(${ProjectName} ${PrivateType} "-Wshadow")
        endif()

        check_c_compiler_flag(-Wno-unknown-attributes HAS_W_NO_UNKNOWN_ATTRIBUTES)
        if(HAS_W_NO_UNKNOWN_ATTRIBUTES)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-unknown-attributes")
        endif()

        check_c_compiler_flag(-Wno-invalid-offsetof HAS_W_NO_INVALID_OFFSETOF)
        if(HAS_W_NO_INVALID_OFFSETOF)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-invalid-offsetof")
        endif()

        check_c_compiler_flag(-Wno-shift-count-overflow HAS_W_NO_SHIFT_COUNT_OVERFLOW)
        if(HAS_W_NO_SHIFT_COUNT_OVERFLOW)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-shift-count-overflow")
        endif()

        check_c_compiler_flag(-Wno-gnu-anonymous-struct HAS_W_NO_GNU_ANONYMOUS_STRUCT)
        if(HAS_W_NO_GNU_ANONYMOUS_STRUCT)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-gnu-anonymous-struct")
        endif()

        check_c_compiler_flag(-Wno-nested-anon-types HAS_W_NO_NESTED_ANON_TYPES)
        if(HAS_W_NO_NESTED_ANON_TYPES)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-nested-anon-types")
        endif()

        check_c_compiler_flag(-Wno-unused-parameter HAS_W_NO_UNUSED_PARAMETER)
        if(HAS_W_NO_UNUSED_PARAMETER)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-unused-parameter")
        endif()

        check_c_compiler_flag(-Wno-unused-private-field HAS_W_NO_UNUSED_PRIVATE_FIELD)
        if(HAS_W_NO_UNUSED_PRIVATE_FIELD)
            target_compile_options(${ProjectName} ${PrivateType} "-Wno-unused-private-field")
        endif()

        check_c_compiler_flag(-fno-omit-frame-pointer HAS_NO_OMIT_FRAME_POINTER)
        if(HAS_NO_OMIT_FRAME_POINTER)
            target_compile_options(${ProjectName} ${PrivateType} "-fno-omit-frame-pointer")
        endif()

        if(NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY" AND NOT ${PrivateType} STREQUAL "PRIVATE")
            if(IS_CLANG)
                check_c_compiler_flag("-fcolor-diagnostics" HAS_COLOR_DIAGNOSTICS)
                if(HAS_COLOR_DIAGNOSTICS)
                    target_compile_options(${TARGET} ${PrivateType} "-fcolor-diagnostics")
                endif()
            else()
                check_c_compiler_flag("-fdiagnostics-color=always" HAS_DIAGNOSTICS_COLOR_ALWAYS)
                if(HAS_DIAGNOSTICS_COLOR_ALWAYS)
                    target_compile_options(${TARGET} ${PrivateType} "-fcolor-diagnostics=always")
                endif()
            endif()
        endif()

        check_c_compiler_flag(-g HAS_GENERATE_DEBUG_INFO)
        if(HAS_GENERATE_DEBUG_INFO)
            target_compile_options(${ProjectName} ${PrivateType} "-g")
        endif()

        check_c_compiler_flag(-mf16c HAS_F16C)
        if(HAS_F16C)
            target_compile_options(${ProjectName} ${PrivateType} "-mf16c")
        endif()

        check_c_compiler_flag(-mfma HAS_FMA)
        if(HAS_FMA)
            target_compile_options(${ProjectName} ${PrivateType} "-mfma")
        endif()

        # Enable PIC
        set_target_properties(${ProjectName} PROPERTIES POSITION_INDEPENDENT_CODE ON)

        # This breaks on macOS, for ARM at least.
        if(NOT APPLE)
            # Attempt to enable Link Time Optimization
            set_target_properties(${ProjectName} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
        endif()
    endif()

    if(MSVC)
        # Disable exceptions and ignore some CRT warnings
        target_compile_definitions(${ProjectName} ${PrivateType} -D_CRT_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=1)

        set_target_properties(${ProjectName} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        
        target_compile_options(${ProjectName} ${PrivateType} "$<$<NOT:$<CONFIG:Debug>>:/Zi>")
        target_link_options(${ProjectName} ${PrivateType} "$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
        target_link_options(${ProjectName} ${PrivateType} "$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
        target_link_options(${ProjectName} ${PrivateType} "$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")

        check_c_compiler_flag(/wd5030 HAS_UNRECOGNIZED_ATTRIBUTES_WARNING)
        check_c_compiler_flag(/wd4251 HAS_DLL_INTERFACE_WARNING)

        if(HAS_UNRECOGNIZED_ATTRIBUTES_WARNING)
            target_compile_options(${ProjectName} ${PrivateType} /wd5030)
        endif()

        if(HAS_DLL_INTERFACE_WARNING)
            target_compile_options(${ProjectName} ${PrivateType} /wd4251)
        endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${ProjectName} ${PrivateType} -D_DEBUG)
    endif()
endfunction()
