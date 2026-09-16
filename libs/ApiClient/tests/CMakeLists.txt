file(GLOB_RECURSE api_client_test_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(ApiClientTests ${api_client_test_source_files})

target_compile_features(ApiClientTests PRIVATE cxx_std_23)
target_compile_definitions(ApiClientTests PRIVATE NO_S3D_USING)
target_include_directories(ApiClientTests PRIVATE ../src)
target_link_libraries(ApiClientTests PRIVATE XLAIR::ApiClient)
set_target_properties(ApiClientTests PROPERTIES FOLDER "Tests" WIN32_EXECUTABLE OFF)
if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(ApiClientTests
        PRIVATE
            "${FOUNDATION_FRAMEWORK}"
            "${CORE_FOUNDATION_FRAMEWORK}"
    )

    set_target_properties(ApiClientTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "ApiClientTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.ApiClientTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME ApiClientTests COMMAND ApiClientTests)
