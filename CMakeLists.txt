cmake_minimum_required(VERSION 3.22)

project(XLAIR VERSION 0.1.0 LANGUAGES CXX)

set(SIV3D_VERSION "0.6.16" CACHE STRING "Siv3D SDK version")
include(siv3d/Siv3D.cmake)

add_executable(XLAIR
    src/Main.cpp
    src/XLAIR/DataDirectory.hpp
)

target_compile_features(XLAIR PRIVATE cxx_std_20)
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
