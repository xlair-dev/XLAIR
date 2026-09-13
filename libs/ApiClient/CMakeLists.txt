file(GLOB_RECURSE api_client_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_library(ApiClient STATIC ${api_client_source_files})

add_library(XLAIR::ApiClient ALIAS ApiClient)

target_compile_features(ApiClient PUBLIC cxx_std_23)
target_compile_definitions(ApiClient PRIVATE NO_S3D_USING)
target_include_directories(ApiClient PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ApiClient PUBLIC Siv3D::Siv3D)
set_target_properties(ApiClient PROPERTIES FOLDER "Libraries")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
