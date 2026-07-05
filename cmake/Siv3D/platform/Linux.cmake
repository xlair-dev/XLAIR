find_package(Siv3D REQUIRED)

set(SIV3D_RUNTIME_RESOURCES_PATH
    "/usr/local/share/Siv3D/resources"
    CACHE PATH "Path to the runtime resources installed with Siv3D"
)

if(NOT IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/engine")
    message(FATAL_ERROR
        "Siv3D runtime resources not found: ${SIV3D_RUNTIME_RESOURCES_PATH}/engine")
endif()

function(_siv3d_platform_add_resources target resource_paths resource_types resource_relative_paths)
    list(LENGTH resource_paths path_count)
    if(path_count EQUAL 0)
        return()
    endif()

    math(EXPR last_index "${path_count} - 1")
    foreach(i RANGE ${last_index})
        list(GET resource_paths ${i} resource_abspath)
        list(GET resource_types ${i} resource_type)
        list(GET resource_relative_paths ${i} resource_relpath)

        if(resource_type STREQUAL "EMBED")
            set(destination_relpath "resources/${resource_relpath}")
        elseif(resource_type STREQUAL "COPY")
            set(destination_relpath "${resource_relpath}")
        endif()

        get_filename_component(resource_directory_relpath "${destination_relpath}" DIRECTORY)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target}>/${resource_directory_relpath}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${resource_abspath}"
                "$<TARGET_FILE_DIR:${target}>/${destination_relpath}"
            VERBATIM
        )
    endforeach()
endfunction()

message(STATUS "Configured Siv3D SDK [Linux package]")
