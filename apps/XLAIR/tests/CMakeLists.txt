add_executable(GameJudgementTests
    Main.cpp
    JudgeEngineTest.cpp
    ../src/core/judgement/JudgeEngine.cpp
)
target_compile_features(GameJudgementTests PRIVATE cxx_std_23)
target_include_directories(GameJudgementTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(GameJudgementTests PRIVATE XLAIR::SheetsAnalyzer)
set_target_properties(GameJudgementTests PROPERTIES FOLDER "Tests" WIN32_EXECUTABLE OFF)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(GameJudgementTests PRIVATE "${FOUNDATION_FRAMEWORK}" "${CORE_FOUNDATION_FRAMEWORK}")
    set_target_properties(GameJudgementTests PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "GameJudgementTests"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.xlair.GameJudgementTests"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

add_test(NAME GameJudgementTests COMMAND GameJudgementTests)
