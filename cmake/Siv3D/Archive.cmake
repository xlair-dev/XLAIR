function(_siv3d_download url output_path expected_sha256)
    get_filename_component(output_directory "${output_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_directory}")

    if(EXISTS "${output_path}")
        file(SHA256 "${output_path}" actual_sha256)
        if(actual_sha256 STREQUAL expected_sha256)
            return()
        endif()

        message(STATUS "Removing an invalid cached download: ${output_path}")
        file(REMOVE "${output_path}")
    endif()

    set(temporary_path "${output_path}.tmp")
    file(REMOVE "${temporary_path}")

    message(STATUS "Downloading: ${url}")
    file(DOWNLOAD
        "${url}"
        "${temporary_path}"
        SHOW_PROGRESS
        TLS_VERIFY ON
        STATUS download_status
    )

    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        list(GET download_status 1 error_message)
        file(REMOVE "${temporary_path}")
        message(FATAL_ERROR "Download failed: ${error_message}")
    endif()

    file(SHA256 "${temporary_path}" actual_sha256)
    if(NOT actual_sha256 STREQUAL expected_sha256)
        file(REMOVE "${temporary_path}")
        message(FATAL_ERROR
            "SHA-256 mismatch for ${url}\n"
            "  expected: ${expected_sha256}\n"
            "  actual:   ${actual_sha256}")
    endif()

    file(RENAME "${temporary_path}" "${output_path}")
endfunction()

function(_siv3d_extract_archive archive_path destination)
    file(REMOVE_RECURSE "${destination}")
    file(MAKE_DIRECTORY "${destination}")
    file(ARCHIVE_EXTRACT
        INPUT "${archive_path}"
        DESTINATION "${destination}"
    )
endfunction()
