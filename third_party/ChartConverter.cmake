include(FetchContent)

FetchContent_Declare(Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG 1499b14e4906a2890f5cee1547c8848db261753d # v0.6.1
)
set(Rust_TOOLCHAIN nightly CACHE STRING "Rust toolchain used by ChartConverter")
FetchContent_MakeAvailable(Corrosion)

FetchContent_Declare(ChartConverter
    GIT_REPOSITORY https://github.com/xlair-dev/ChartConverter.git
    GIT_TAG f5ddc904fa4c24084dbf80153783718644a05f75
)
FetchContent_MakeAvailable(ChartConverter)

corrosion_import_crate(
    MANIFEST_PATH "${chartconverter_SOURCE_DIR}/crates/chart-converter-ffi/Cargo.toml"
    CRATE_TYPES staticlib
    LOCKED
)

set(chart_converter_include_dir "${CMAKE_CURRENT_BINARY_DIR}/chart-converter/include")
file(MAKE_DIRECTORY "${chart_converter_include_dir}")
corrosion_set_env_vars(chart_converter_ffi "CHART_CONVERTER_HEADER_DIR=${chart_converter_include_dir}")
target_include_directories(chart_converter_ffi-static INTERFACE "${chart_converter_include_dir}")
