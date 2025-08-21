function(GenVsFilters FileGroup)
    foreach(sourceFile IN LISTS ${FileGroup})
        # Figure out base folder
        if(sourceFile MATCHES "[\\/]?include[\\/]")
            # Set the base folder to the 'include' directory.
            file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}/include" "${sourceFile}")
            set(groupBase "include")
        elseif(sourceFile MATCHES "[\\/]?src[\\/]")
            # Set the base folder to the 'src' directory.
            file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}/src" "${sourceFile}")
            set(groupBase "src")
        else()
            # fallback (won't happen unless CMakeLists is wrong)
            file(RELATIVE_PATH relPath "${CMAKE_CURRENT_SOURCE_DIR}" "${sourceFile}")
            set(groupBase "")
        endif()

        # Get the directory name
        get_filename_component(dirPath "${relPath}" PATH)

        # Create the Visual Studio Filter
        if(NOT dirPath STREQUAL "")
            string(REPLACE "/" "\\" vsGroup "${dirPath}")

            if(groupBase STREQUAL "")
                source_group("${vsGroup}" FILES "${sourceFile}")
            else()
                source_group("${groupBase}\\${vsGroup}" FILES "${sourceFile}")
            endif()
        else()
            if(groupBase STREQUAL "")
                source_group("" FILES "${sourceFile}")
            else()
                source_group("${groupBase}" FILES "${sourceFile}")
            endif()
        endif()
    endforeach()
endfunction()
