file(GLOB_RECURSE audio_test_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(AudioTests ${audio_test_source_files})

target_compile_features(AudioTests PRIVATE cxx_std_23)
target_compile_definitions(AudioTests PRIVATE NO_S3D_USING)
target_link_libraries(AudioTests PRIVATE XLAIR::Audio)
set_target_properties(AudioTests PROPERTIES FOLDER "Tests" WIN32_EXECUTABLE OFF)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(AudioTests
        PRIVATE
            "${FOUNDATION_FRAMEWORK}"
            "${CORE_FOUNDATION_FRAMEWORK}"
    )

    set_target_properties(AudioTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "AudioTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.AudioTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME AudioTests COMMAND AudioTests)
