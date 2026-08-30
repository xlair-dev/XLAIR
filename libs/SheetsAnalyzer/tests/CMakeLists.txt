file(GLOB_RECURSE sheets_analyzer_test_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(SheetsAnalyzerTests ${sheets_analyzer_test_source_files})

target_compile_features(SheetsAnalyzerTests PRIVATE cxx_std_23)
target_compile_definitions(SheetsAnalyzerTests PRIVATE NO_S3D_USING)
target_include_directories(SheetsAnalyzerTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

target_link_libraries(SheetsAnalyzerTests
    PRIVATE
        XLAIR::SheetsAnalyzer
)

set_target_properties(SheetsAnalyzerTests PROPERTIES
    FOLDER "Tests"
    WIN32_EXECUTABLE OFF
)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(SheetsAnalyzerTests
        PRIVATE
            "${FOUNDATION_FRAMEWORK}"
            "${CORE_FOUNDATION_FRAMEWORK}"
    )

    set_target_properties(SheetsAnalyzerTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "SheetsAnalyzerTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.SheetsAnalyzerTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME SheetsAnalyzerTests COMMAND SheetsAnalyzerTests)
