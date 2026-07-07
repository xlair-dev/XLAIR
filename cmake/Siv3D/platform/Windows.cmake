# Config

set(SIV3D_ROOT "${SIV3D_SDK_CACHE_DIR}/Windows")
set(SIV3D_INCLUDE_DIR "${SIV3D_ROOT}/include")
set(SIV3D_LIB_DIR "${SIV3D_ROOT}/lib/Windows")
set(SIV3D_RUNTIME_RESOURCES_PATH "${SIV3D_ROOT}/runtime")
set(SIV3D_DOWNLOAD_DIR "${SIV3D_SDK_CACHE_DIR}/downloads")
set(SIV3D_WINDOWS_SDK_ZIP_PATH
    "${SIV3D_DOWNLOAD_DIR}/windows_sdk_${SIV3D_VERSION}.zip")
set(SIV3D_WINDOWS_SDK_EXTRACT_DIR
    "${SIV3D_DOWNLOAD_DIR}/windows_sdk_${SIV3D_VERSION}")
set(SIV3D_WINDOWS_SDK_EXTRACTED_ROOT
    "${SIV3D_WINDOWS_SDK_EXTRACT_DIR}/OpenSiv3D_SDK_${SIV3D_VERSION}")
set(SIV3D_WINDOWS_TEMPLATE_ZIP_PATH
    "${SIV3D_DOWNLOAD_DIR}/windows_template_${SIV3D_VERSION}.zip")
set(SIV3D_WINDOWS_TEMPLATE_EXTRACT_DIR
    "${SIV3D_DOWNLOAD_DIR}/windows_template_${SIV3D_VERSION}")
set(SIV3D_EXTRACTED FALSE)
if(EXISTS "${SIV3D_INCLUDE_DIR}/Siv3D.hpp"
    AND EXISTS "${SIV3D_LIB_DIR}/Siv3D.lib"
    AND IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/engine"
    AND IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/dll")
    set(SIV3D_EXTRACTED TRUE)
endif()

set(SIV3D_WINDOWS_ZIP_URL "https://siv3d.jp/downloads/Siv3D/manual/${SIV3D_VERSION}/OpenSiv3D_SDK_${SIV3D_VERSION}.zip")
set(SIV3D_WINDOWS_ZIP_SHA256
    "5819af501b3589d5bdd8052a97688af19a854c6a0af7d7f5bc70ae2a1975fcdd")
set(SIV3D_WINDOWS_TEMPLATE_ZIP_URL
    "https://siv3d.jp/downloads/Siv3D/manual/${SIV3D_VERSION}/OpenSiv3D_${SIV3D_VERSION}.zip")
set(SIV3D_WINDOWS_TEMPLATE_ZIP_SHA256
    "10c2707b37338c407fc11c28119e5da0ea232ed4cf09438a2b4bc90f75583ea0")

enable_language(RC)

# Download and extract Siv3D SDK

if(NOT SIV3D_EXTRACTED)
    _siv3d_download(
        "${SIV3D_WINDOWS_ZIP_URL}"
        "${SIV3D_WINDOWS_SDK_ZIP_PATH}"
        "${SIV3D_WINDOWS_ZIP_SHA256}"
    )
    _siv3d_download(
        "${SIV3D_WINDOWS_TEMPLATE_ZIP_URL}"
        "${SIV3D_WINDOWS_TEMPLATE_ZIP_PATH}"
        "${SIV3D_WINDOWS_TEMPLATE_ZIP_SHA256}"
    )
    _siv3d_extract_archive(
        "${SIV3D_WINDOWS_SDK_ZIP_PATH}"
        "${SIV3D_WINDOWS_SDK_EXTRACT_DIR}"
    )
    _siv3d_extract_archive(
        "${SIV3D_WINDOWS_TEMPLATE_ZIP_PATH}"
        "${SIV3D_WINDOWS_TEMPLATE_EXTRACT_DIR}"
    )

    file(COPY "${SIV3D_WINDOWS_SDK_EXTRACTED_ROOT}/lib/Windows/"
        DESTINATION "${SIV3D_LIB_DIR}")
    file(COPY "${SIV3D_WINDOWS_SDK_EXTRACTED_ROOT}/include/"
        DESTINATION "${SIV3D_INCLUDE_DIR}")
    file(COPY "${SIV3D_WINDOWS_TEMPLATE_EXTRACT_DIR}/App/engine"
        DESTINATION "${SIV3D_RUNTIME_RESOURCES_PATH}")
    file(COPY "${SIV3D_WINDOWS_TEMPLATE_EXTRACT_DIR}/App/dll"
        DESTINATION "${SIV3D_RUNTIME_RESOURCES_PATH}")
endif()

if(NOT EXISTS "${SIV3D_INCLUDE_DIR}/Siv3D.hpp"
    OR NOT EXISTS "${SIV3D_LIB_DIR}/Siv3D.lib"
    OR NOT IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/engine"
    OR NOT IS_DIRECTORY "${SIV3D_RUNTIME_RESOURCES_PATH}/dll")
    message(FATAL_ERROR "The Windows Siv3D SDK installation is incomplete: ${SIV3D_ROOT}")
endif()

file(REMOVE "${SIV3D_WINDOWS_SDK_ZIP_PATH}" "${SIV3D_WINDOWS_TEMPLATE_ZIP_PATH}")
file(REMOVE_RECURSE
    "${SIV3D_WINDOWS_SDK_EXTRACT_DIR}"
    "${SIV3D_WINDOWS_TEMPLATE_EXTRACT_DIR}"
)

# Setup Siv3D::Siv3D target

add_library(Siv3DWindows INTERFACE)
add_library(Siv3D::Siv3D ALIAS Siv3DWindows)

# lld-link misparses forward-slash library paths (e.g. /curl/... as flags).
# Pass -libpath per directory and link by filename only.
set(SIV3D_WINDOWS_LINK_DIRS
    "${SIV3D_LIB_DIR}"
    "${SIV3D_LIB_DIR}/boost"
    "${SIV3D_LIB_DIR}/curl"
    "${SIV3D_LIB_DIR}/freetype"
    "${SIV3D_LIB_DIR}/glew"
    "${SIV3D_LIB_DIR}/harfbuzz"
    "${SIV3D_LIB_DIR}/libgif"
    "${SIV3D_LIB_DIR}/libjpeg-turbo"
    "${SIV3D_LIB_DIR}/libogg"
    "${SIV3D_LIB_DIR}/libpng"
    "${SIV3D_LIB_DIR}/libtiff"
    "${SIV3D_LIB_DIR}/libvorbis"
    "${SIV3D_LIB_DIR}/libwebp"
    "${SIV3D_LIB_DIR}/Oniguruma"
    "${SIV3D_LIB_DIR}/opencv"
    "${SIV3D_LIB_DIR}/opus"
    "${SIV3D_LIB_DIR}/zlib"
)
foreach(link_dir IN LISTS SIV3D_WINDOWS_LINK_DIRS)
    target_link_options(Siv3DWindows INTERFACE "LINKER:-libpath:${link_dir}")
endforeach()

target_link_libraries(Siv3DWindows INTERFACE
    optimized Siv3D.lib
    debug     Siv3D_d.lib
    optimized libboost_filesystem-vc143-mt-s-x64-1_83.lib
    debug     libboost_filesystem-vc143-mt-sgd-x64-1_83.lib
    optimized libcurl.lib
    debug     libcurl-d.lib
    optimized freetype.lib
    debug     freetyped.lib
    optimized glew32s.lib
    debug     glew32sd.lib
    optimized harfbuzz.lib
    debug     harfbuzz_d.lib
    optimized libgif.lib
    debug     libgif_d.lib
    optimized turbojpeg-static.lib
    debug     turbojpeg-static_d.lib
    optimized libogg.lib
    debug     libogg_d.lib
    optimized libpng16.lib
    debug     libpng16_d.lib
    optimized tiff.lib
    debug     tiffd.lib
    optimized libvorbis_static.lib
    debug     libvorbis_static_d.lib
    optimized libvorbisfile_static.lib
    debug     libvorbisfile_static_d.lib
    optimized libwebp.lib
    debug     libwebp_debug.lib
    optimized Oniguruma.lib
    debug     Oniguruma_d.lib
    optimized opencv_core451.lib
    debug     opencv_core451d.lib
    optimized opencv_imgcodecs451.lib
    debug     opencv_imgcodecs451d.lib
    optimized opencv_imgproc451.lib
    debug     opencv_imgproc451d.lib
    optimized opencv_objdetect451.lib
    debug     opencv_objdetect451d.lib
    optimized opencv_photo451.lib
    debug     opencv_photo451d.lib
    optimized opencv_videoio451.lib
    debug     opencv_videoio451d.lib
    optimized opus.lib
    debug     opus_d.lib
    optimized opusfile.lib
    debug     opusfile_d.lib
    optimized zlib.lib
    debug     zlibd.lib
)

target_include_directories(Siv3DWindows INTERFACE
    "${SIV3D_INCLUDE_DIR}"
    "${SIV3D_INCLUDE_DIR}/ThirdParty"
)

function(_siv3d_platform_add_resources target resource_paths resource_types resource_relative_paths)
    list(LENGTH resource_paths path_count)
    if(path_count EQUAL 0)
        return()
    endif()

    set_target_properties(${target} PROPERTIES
        RESOURCE "${resource_paths}"
    )

    set(generated_rc_path "${CMAKE_BINARY_DIR}/${target}_resource.rc")
    set(generated_rc_content "# include <Siv3D/Windows/Resource.hpp>\n\n")

    math(EXPR last_index "${path_count} - 1")
    foreach(i RANGE ${last_index})
        list(GET resource_paths ${i} resource_abspath)
        list(GET resource_types ${i} resource_type)
        list(GET resource_relative_paths ${i} resource_relpath)

        if(resource_type STREQUAL "EMBED")
            list(APPEND embed_paths "${resource_abspath}")
            if(resource_relpath STREQUAL "icon.ico")
                set(generated_rc_content "${generated_rc_content}DefineResource(100, ICON, ${resource_abspath})\n")
            else()
                set(generated_rc_content "${generated_rc_content}DefineResource(${resource_relpath}, FILE, ${resource_abspath})\n")
            endif()
        elseif(resource_type STREQUAL "COPY")
            get_filename_component(resource_directory_relpath "${resource_relpath}" DIRECTORY)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target}>/${resource_directory_relpath}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${resource_abspath}"
                    "$<TARGET_FILE_DIR:${target}>/${resource_relpath}"
                VERBATIM
            )
        endif()
    endforeach()

    file(WRITE "${generated_rc_path}" "${generated_rc_content}")
    set_source_files_properties("${generated_rc_path}" PROPERTIES
        GENERATED TRUE
        OBJECT_DEPENDS "${embed_paths}"
    )
    target_sources(${target} PRIVATE "${generated_rc_path}")
    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:RC>:-I${SIV3D_INCLUDE_DIR}>"
    )
endfunction()

message(STATUS "Configured Siv3D SDK [Windows]: ${SIV3D_ROOT}")
message(STATUS "  Include: ${SIV3D_INCLUDE_DIR}")
message(STATUS "  Library: ${SIV3D_LIB_DIR}")
