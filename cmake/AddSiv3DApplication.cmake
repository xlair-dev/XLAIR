function(xlair_add_siv3d_application target)
    cmake_parse_arguments(APP
        ""
        "BUNDLE_IDENTIFIER;BUNDLE_NAME;DATA_DIR;INFO_PLIST;OUTPUT_DIRECTORY;RESOURCES_DIR;SOURCE_DIR"
        "RESOURCE_ENTRIES"
        ${ARGN}
    )

    if(APP_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "xlair_add_siv3d_application: unexpected arguments: ${APP_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT APP_SOURCE_DIR)
        set(APP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
    endif()

    if(NOT APP_DATA_DIR)
        set(APP_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
    endif()

    if(NOT APP_RESOURCES_DIR)
        set(APP_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    endif()

    if(NOT APP_OUTPUT_DIRECTORY)
        set(APP_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/App/${target}")
    endif()

    if(EXISTS "${APP_OUTPUT_DIRECTORY}" AND NOT IS_DIRECTORY "${APP_OUTPUT_DIRECTORY}")
        file(REMOVE "${APP_OUTPUT_DIRECTORY}")
    endif()

    if(NOT APP_BUNDLE_NAME)
        set(APP_BUNDLE_NAME "${target}")
    endif()

    if(NOT APP_BUNDLE_IDENTIFIER)
        set(APP_BUNDLE_IDENTIFIER "dev.xlair.${target}")
    endif()

    if(NOT APP_INFO_PLIST AND EXISTS "${APP_SOURCE_DIR}/macOS/Info.plist")
        set(APP_INFO_PLIST "${APP_SOURCE_DIR}/macOS/Info.plist")
    endif()

    file(GLOB_RECURSE app_source_files CONFIGURE_DEPENDS
        "${APP_SOURCE_DIR}/*.cpp"
        "${APP_SOURCE_DIR}/*.hpp"
    )

    if(NOT app_source_files)
        message(FATAL_ERROR
            "xlair_add_siv3d_application: no source files found in ${APP_SOURCE_DIR}")
    endif()

    add_executable(${target} ${app_source_files})
    target_compile_features(${target} PRIVATE cxx_std_23)
    target_link_libraries(${target} PRIVATE Siv3D::Siv3D)

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${target}"
        RUNTIME_OUTPUT_DIRECTORY "${APP_OUTPUT_DIRECTORY}"
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_BUNDLE_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        FOLDER "Apps"
        SIV3D_PROJECT_RESOURCES_DIR "${APP_RESOURCES_DIR}"
    )

    if(APP_INFO_PLIST)
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${APP_INFO_PLIST}"
        )
    endif()

    if(TARGET "${XLAIR_COMPILE_COMMANDS_FALLBACK_TARGET}")
        add_dependencies(${target} "${XLAIR_COMPILE_COMMANDS_FALLBACK_TARGET}")
    endif()

    if(IS_DIRECTORY "${APP_DATA_DIR}")
        add_custom_target(${target}Data
            COMMAND "${CMAKE_COMMAND}"
                "-DAPP_NAME=${target}"
                "-DSOURCE_DIR=${APP_DATA_DIR}"
                "-DDESTINATION_DIR=${APP_OUTPUT_DIRECTORY}/data"
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/SyncData.cmake"
            COMMENT "Synchronizing ${target} runtime data"
            VERBATIM
        )
        set_target_properties(${target}Data PROPERTIES FOLDER "Apps")
        add_dependencies(${target} ${target}Data)
    endif()

    if(APP_RESOURCE_ENTRIES)
        siv3d_resources(${target} ${APP_RESOURCE_ENTRIES})
    endif()
endfunction()
