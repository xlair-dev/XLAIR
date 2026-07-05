# Config

set(SIV3D_ROOT "${SIV3D_SDK_CACHE_DIR}/macOS")
set(SIV3D_INCLUDE_DIR "${SIV3D_ROOT}/include")
set(SIV3D_LIB_DIR "${SIV3D_ROOT}/lib/macOS")
set(SIV3D_RUNTIME_RESOURCES_PATH "${SIV3D_ROOT}/runtime")
set(SIV3D_DOWNLOAD_DIR "${SIV3D_SDK_CACHE_DIR}/downloads")
set(SIV3D_EXTRACTED FALSE)
if(EXISTS "${SIV3D_INCLUDE_DIR}/Siv3D.hpp"
    AND EXISTS "${SIV3D_LIB_DIR}/libSiv3D.a"
    AND IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/engine")
    set(SIV3D_EXTRACTED TRUE)
endif()

set(SIV3D_MACOS_ZIP_URL "https://siv3d.jp/downloads/Siv3D/siv3d_v${SIV3D_VERSION}_macOS.zip")

# Download and extract Siv3D SDK

if(NOT SIV3D_EXTRACTED)
    set(SIV3D_MACOS_TEMP_ZIP_PATH "${SIV3D_DOWNLOAD_DIR}/macos_${SIV3D_VERSION}.zip")
    set(SIV3D_MACOS_TEMP_INNER_DIR "${SIV3D_DOWNLOAD_DIR}/siv3d_v${SIV3D_VERSION}_macOS")

    # 一時ディレクトリを作成
    file(MAKE_DIRECTORY "${SIV3D_DOWNLOAD_DIR}")

    # ZIPファイルのダウンロード
    if(NOT EXISTS "${SIV3D_MACOS_TEMP_ZIP_PATH}")
        message(STATUS "Downloading Siv3D SDK from: ${SIV3D_MACOS_ZIP_URL}")
        file(DOWNLOAD
            "${SIV3D_MACOS_ZIP_URL}"
            "${SIV3D_MACOS_TEMP_ZIP_PATH}"
            SHOW_PROGRESS
            TLS_VERIFY ON
            STATUS download_status
        )
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            list(GET download_status 1 error_message)
        message(FATAL_ERROR "Failed to download ZIP file: ${error_message}")
        endif()
    endif()

    # ZIPファイルの展開
    message(STATUS "Extracting Siv3D SDK")

    if(NOT EXISTS "${SIV3D_MACOS_TEMP_INNER_DIR}")
    execute_process(
        COMMAND unzip -q "${SIV3D_MACOS_TEMP_ZIP_PATH}" -d "${SIV3D_DOWNLOAD_DIR}"
        RESULT_VARIABLE unzip_result
        OUTPUT_QUIET
        ERROR_QUIET
        )
        if(NOT unzip_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract ZIP file: ${unzip_result}")
        endif()
    endif()

    file(COPY "${SIV3D_MACOS_TEMP_INNER_DIR}/lib/macOS/" DESTINATION "${SIV3D_LIB_DIR}")
    file(COPY "${SIV3D_MACOS_TEMP_INNER_DIR}/include/" DESTINATION "${SIV3D_INCLUDE_DIR}")
    file(COPY "${SIV3D_MACOS_TEMP_INNER_DIR}/examples/empty/App/engine"
        DESTINATION "${SIV3D_RUNTIME_RESOURCES_PATH}")
endif()

# Setup Siv3D::Siv3D target

find_library(AVFoundation_FRAMEWORK AVFoundation)
find_library(AudioToolbox_FRAMEWORK AudioToolbox)
find_library(CoreMedia_FRAMEWORK CoreMedia)

add_library(Siv3D::Siv3D STATIC IMPORTED)

set_target_properties(Siv3D::Siv3D PROPERTIES
    IMPORTED_LOCATION "${SIV3D_LIB_DIR}/libSiv3D.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SIV3D_INCLUDE_DIR};${SIV3D_INCLUDE_DIR}/ThirdParty"
)

target_link_libraries(Siv3D::Siv3D INTERFACE
    "${SIV3D_LIB_DIR}/boost/libboost_filesystem.a"
    "${SIV3D_LIB_DIR}/freetype/libfreetype.a"
    "${SIV3D_LIB_DIR}/harfbuzz/libharfbuzz.a"
    "${SIV3D_LIB_DIR}/libgif/liblibgif.a"
    "${SIV3D_LIB_DIR}/libjpeg-turbo/libturbojpeg.a"
    "${SIV3D_LIB_DIR}/libogg/libogg.a"
    "${SIV3D_LIB_DIR}/libpng/libpng16.a"
    "${SIV3D_LIB_DIR}/libtiff/libtiff.a"
    "${SIV3D_LIB_DIR}/libvorbis/libvorbis.a"
    "${SIV3D_LIB_DIR}/libvorbis/libvorbisenc.a"
    "${SIV3D_LIB_DIR}/libvorbis/libvorbisfile.a"
    "${SIV3D_LIB_DIR}/libwebp/libwebp.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_core.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_imgcodecs.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_imgproc.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_objdetect.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_photo.a"
    "${SIV3D_LIB_DIR}/opencv/libopencv_videoio.a"
    "${SIV3D_LIB_DIR}/opus/libopus.a"
    "${SIV3D_LIB_DIR}/opus/libopusfile.a"
    "${SIV3D_LIB_DIR}/zlib/libzlib.a"
    "-lcurl"
    ${AVFoundation_FRAMEWORK}
    ${AudioToolbox_FRAMEWORK}
    ${CoreMedia_FRAMEWORK}
)

function(_siv3d_platform_add_resources target resource_paths resource_types resource_relative_paths)
    list(LENGTH resource_paths path_count)
    if(path_count EQUAL 0)
        return()
    endif()

    math(EXPR last_index "${path_count} - 1")
    foreach(i RANGE ${last_index})
        list(GET resource_paths ${i} resource_path)
        list(GET resource_relative_paths ${i} resource_relpath)
        get_filename_component(resource_directory_relpath "${resource_relpath}" DIRECTORY)

        set_source_files_properties("${resource_path}" PROPERTIES
            HEADER_FILE_ONLY TRUE
            MACOSX_PACKAGE_LOCATION "Resources/${resource_directory_relpath}"
        )
    endforeach()

    target_sources(${target} PRIVATE ${resource_paths})
endfunction()

message(STATUS "Configured Siv3D SDK [Mac]: ${SIV3D_ROOT}")
message(STATUS "  Include: ${SIV3D_INCLUDE_DIR}")
message(STATUS "  Library: ${SIV3D_LIB_DIR}")
