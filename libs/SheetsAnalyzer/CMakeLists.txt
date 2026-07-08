add_library(SheetsAnalyzer STATIC
    src/SheetsAnalyzer.cpp
    include/SheetsAnalyzer/SheetsAnalyzer.hpp
)

add_library(XLAIR::SheetsAnalyzer ALIAS SheetsAnalyzer)

target_compile_features(SheetsAnalyzer PUBLIC cxx_std_23)

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
