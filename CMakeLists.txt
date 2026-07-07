cmake_minimum_required(VERSION 3.22)

project(XLAIR VERSION 0.1.0 LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(XLAIR_COMPILE_COMMANDS "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json")
    file(RELATIVE_PATH XLAIR_COMPILE_COMMANDS_TARGET
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/compile_commands.json"
    )

    file(REMOVE "${XLAIR_COMPILE_COMMANDS}")
    file(CREATE_LINK
        "${XLAIR_COMPILE_COMMANDS_TARGET}"
        "${XLAIR_COMPILE_COMMANDS}"
        SYMBOLIC
        RESULT XLAIR_COMPILE_COMMANDS_LINK_RESULT
    )

    if(NOT XLAIR_COMPILE_COMMANDS_LINK_RESULT STREQUAL "0")
        message(WARNING
            "Could not create compile_commands.json symlink: "
            "${XLAIR_COMPILE_COMMANDS_LINK_RESULT}")

        add_custom_target(XLAIRCompileCommands
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${XLAIR_COMPILE_COMMANDS}"
            COMMENT "Copying compile_commands.json"
            VERBATIM
        )
        set_target_properties(XLAIRCompileCommands PROPERTIES FOLDER "Tools")
        set(XLAIR_COMPILE_COMMANDS_FALLBACK_TARGET XLAIRCompileCommands)
    endif()
endif()

set(SIV3D_VERSION "0.6.16")
include(cmake/Siv3D/Siv3D.cmake)
include(cmake/AddSiv3DApplication.cmake)

add_subdirectory(libs)
add_subdirectory(apps)
