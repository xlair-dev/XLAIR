add_executable(XLAIRSyncTests
    Main.cpp
    LocalCatalogSyncTest.cpp
    ../src/infra/api/LocalCatalogSync.cpp
    ../src/infra/api/MusicMetadata.cpp
)

target_compile_features(XLAIRSyncTests PRIVATE cxx_std_23)
target_include_directories(XLAIRSyncTests PRIVATE ../src)
target_link_libraries(XLAIRSyncTests PRIVATE XLAIR::ApiClient PicoSHA2::PicoSHA2)
set_target_properties(XLAIRSyncTests PROPERTIES FOLDER "Tests" WIN32_EXECUTABLE OFF)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(XLAIRSyncTests PRIVATE "${FOUNDATION_FRAMEWORK}" "${CORE_FOUNDATION_FRAMEWORK}")
    set_target_properties(XLAIRSyncTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "XLAIRSyncTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.XLAIRSyncTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME XLAIRSyncTests COMMAND XLAIRSyncTests)
