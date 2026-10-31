file(GLOB_RECURSE audio_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(Audio STATIC ${audio_source_files})
add_library(XLAIR::Audio ALIAS Audio)

target_compile_features(Audio PUBLIC cxx_std_23)
target_compile_definitions(Audio PRIVATE NO_S3D_USING)
target_include_directories(Audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(Audio PUBLIC Siv3D::Siv3D)
set_target_properties(Audio PROPERTIES FOLDER "Libraries")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
