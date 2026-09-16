add_library(PicoSHA2 INTERFACE)
add_library(PicoSHA2::PicoSHA2 ALIAS PicoSHA2)

target_include_directories(PicoSHA2 SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/PicoSHA2"
)

set_target_properties(PicoSHA2 PROPERTIES FOLDER "ThirdParty")
