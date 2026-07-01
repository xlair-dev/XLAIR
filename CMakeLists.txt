cmake_minimum_required(VERSION 3.20)

project(Siv3DTemplate CXX)

find_package(Siv3D REQUIRED)

add_executable(Siv3DTemplate
    src/Main.cpp
)

target_link_libraries(Siv3DTemplate PRIVATE Siv3D::Siv3D)
target_compile_features(Siv3DTemplate PRIVATE cxx_std_20)

set(SIV3D_RUNTIME_RESOURCE_DIR /usr/local/share/Siv3D/resources)
set(SIV3D_RUNTIME_TARGET_DIR ${CMAKE_BINARY_DIR}/resources)

if(EXISTS "${SIV3D_RUNTIME_RESOURCE_DIR}")
    file(MAKE_DIRECTORY "${SIV3D_RUNTIME_TARGET_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${SIV3D_RUNTIME_RESOURCE_DIR}"
            "${SIV3D_RUNTIME_TARGET_DIR}"
    )
endif()
