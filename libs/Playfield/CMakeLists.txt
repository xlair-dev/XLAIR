file(GLOB_RECURSE playfield_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_library(Playfield STATIC ${playfield_source_files})

add_library(XLAIR::Playfield ALIAS Playfield)

target_compile_features(Playfield PUBLIC cxx_std_23)

target_compile_definitions(Playfield PRIVATE NO_S3D_USING)

target_include_directories(Playfield
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(Playfield
    PUBLIC
        XLAIR::SheetsAnalyzer
)

set_target_properties(Playfield PROPERTIES
    FOLDER "Libraries"
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
