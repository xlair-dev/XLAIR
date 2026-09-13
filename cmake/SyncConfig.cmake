if(NOT DEFINED APP_NAME OR APP_NAME STREQUAL "")
    set(APP_NAME "Application")
endif()

if(NOT DEFINED CONFIG_FILE OR CONFIG_FILE STREQUAL "")
    message(FATAL_ERROR "${APP_NAME} local config file path is not set")
endif()

if(NOT DEFINED CONFIG_EXAMPLE_FILE OR NOT EXISTS "${CONFIG_EXAMPLE_FILE}")
    message(FATAL_ERROR "${APP_NAME} example config file not found: ${CONFIG_EXAMPLE_FILE}")
endif()

if(NOT DEFINED DESTINATION_FILE OR DESTINATION_FILE STREQUAL "")
    message(FATAL_ERROR "${APP_NAME} config destination file is not set")
endif()

if(EXISTS "${CONFIG_FILE}")
    set(source_file "${CONFIG_FILE}")
else()
    set(source_file "${CONFIG_EXAMPLE_FILE}")
endif()

get_filename_component(destination_directory "${DESTINATION_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${destination_directory}")
file(COPY_FILE "${source_file}" "${DESTINATION_FILE}" ONLY_IF_DIFFERENT)
