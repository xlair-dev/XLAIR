file(GLOB_RECURSE playfield_test_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(PlayfieldTests ${playfield_test_source_files})

target_compile_features(PlayfieldTests PRIVATE cxx_std_23)
target_compile_definitions(PlayfieldTests PRIVATE NO_S3D_USING)

target_link_libraries(PlayfieldTests
    PRIVATE
        XLAIR::Playfield
)

set_target_properties(PlayfieldTests PROPERTIES
    FOLDER "Tests"
    WIN32_EXECUTABLE OFF
)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(PlayfieldTests
        PRIVATE
            "${FOUNDATION_FRAMEWORK}"
            "${CORE_FOUNDATION_FRAMEWORK}"
    )

    set_target_properties(PlayfieldTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "PlayfieldTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.PlayfieldTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME PlayfieldTests COMMAND PlayfieldTests)
