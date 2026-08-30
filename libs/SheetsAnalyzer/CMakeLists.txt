file(GLOB_RECURSE sheets_analyzer_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_library(SheetsAnalyzer STATIC ${sheets_analyzer_source_files})

add_library(XLAIR::SheetsAnalyzer ALIAS SheetsAnalyzer)

target_compile_features(SheetsAnalyzer PUBLIC cxx_std_23)

target_compile_definitions(SheetsAnalyzer PRIVATE NO_S3D_USING)

target_include_directories(SheetsAnalyzer
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(SheetsAnalyzer
    PUBLIC
        Siv3D::Siv3D
)

set_target_properties(SheetsAnalyzer PROPERTIES
    FOLDER "Libraries"
)
