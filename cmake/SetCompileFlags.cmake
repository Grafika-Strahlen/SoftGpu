# We use this to check for some compiler flags, mostly to disable warnings.
include(CheckCCompilerFlag)

function(SetCompileFlags ProjectName PublicType PrivateType)
    # Set C++20
    target_compile_features(${ProjectName} ${PublicType} cxx_std_20)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            # using clang with clang-cl front end

            target_compile_options(${ProjectName} ${PrivateType} -Wno-unknown-attributes)
        elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
            # using clang with regular front end

            target_compile_options(${ProjectName} ${PrivateType} -mf16c -mfma)

            target_compile_options(${ProjectName} ${PrivateType} -Wno-unknown-attributes -Wno-invalid-offsetof)

            # Enable PIC
            set_target_properties(${ProjectName} PROPERTIES POSITION_INDEPENDENT_CODE ON)

            # Attempt to enable Link Time Optimization
            set_target_properties(${ProjectName} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
        endif()
    endif()

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
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
