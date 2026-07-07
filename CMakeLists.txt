cmake_minimum_required(VERSION 3.22)

project(XLAIR VERSION 0.1.0 LANGUAGES CXX)

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
    endif()
endif()

set(SIV3D_VERSION "0.6.16")
include(cmake/Siv3D/Siv3D.cmake)

file(GLOB_RECURSE XLAIR_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_executable(XLAIR ${XLAIR_SOURCE_FILES})

if(CMAKE_EXPORT_COMPILE_COMMANDS
    AND NOT XLAIR_COMPILE_COMMANDS_LINK_RESULT STREQUAL "0")
    add_custom_target(XLAIRCompileCommands
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${XLAIR_COMPILE_COMMANDS}"
        COMMENT "Copying compile_commands.json"
        VERBATIM
    )
    add_dependencies(XLAIR XLAIRCompileCommands)
endif()

target_compile_features(XLAIR PRIVATE cxx_std_23)
target_link_libraries(XLAIR PRIVATE Siv3D::Siv3D)

set_target_properties(XLAIR PROPERTIES
    OUTPUT_NAME "XLAIR"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/App"
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/macOS/Info.plist"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.XLAIR"
    MACOSX_BUNDLE_BUNDLE_NAME "XLAIR"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

set(XLAIR_DATA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(XLAIR_DATA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/App/data")

add_custom_target(XLAIRData
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_DIR=${XLAIR_DATA_SOURCE_DIR}
        -DDESTINATION_DIR=${XLAIR_DATA_OUTPUT_DIR}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SyncData.cmake"
    COMMENT "Synchronizing XLAIR runtime data"
    VERBATIM
)
add_dependencies(XLAIR XLAIRData)

siv3d_resources(
    XLAIR
    ALL     EMBED "engine/font"
    MACOS   EMBED "engine/lib/soundtouch"
    WINDOWS EMBED "engine/shader/d3d11"
    MACOS   EMBED "engine/shader/msl"
    LINUX   EMBED "engine/shader/essl"
    ALL     EMBED "engine/shader/glsl"
    ALL     EMBED "engine/soundfont"
    ALL     EMBED "engine/texture"
    WINDOWS COPY  "dll"
    MACOS   EMBED "icon.icns"
    WINDOWS EMBED "icon.ico"
)
