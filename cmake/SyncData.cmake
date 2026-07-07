if(NOT DEFINED APP_NAME OR APP_NAME STREQUAL "")
    set(APP_NAME "Application")
endif()

if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "${APP_NAME} data source directory not found: ${SOURCE_DIR}")
endif()

if(NOT DEFINED DESTINATION_DIR OR DESTINATION_DIR STREQUAL "")
    message(FATAL_ERROR "${APP_NAME} data destination directory is not set")
endif()

file(MAKE_DIRECTORY "${DESTINATION_DIR}")

file(GLOB_RECURSE source_files
    LIST_DIRECTORIES FALSE
    RELATIVE "${SOURCE_DIR}"
    "${SOURCE_DIR}/*"
)

file(GLOB_RECURSE destination_files
    LIST_DIRECTORIES FALSE
    RELATIVE "${DESTINATION_DIR}"
    "${DESTINATION_DIR}/*"
)

foreach(relative_path IN LISTS destination_files)
    if(NOT EXISTS "${SOURCE_DIR}/${relative_path}")
        file(REMOVE "${DESTINATION_DIR}/${relative_path}")
    endif()
endforeach()

foreach(relative_path IN LISTS source_files)
    get_filename_component(relative_directory "${relative_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${DESTINATION_DIR}/${relative_directory}")
    file(COPY_FILE
        "${SOURCE_DIR}/${relative_path}"
        "${DESTINATION_DIR}/${relative_path}"
        ONLY_IF_DIFFERENT
    )
endforeach()
