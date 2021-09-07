cmake_minimum_required(VERSION 3.0.0)
project(FunnelPipe)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/bin)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

subdirs(
    ${EXTERNAL_DIR}/gizmesh/falg
    ${EXTERNAL_DIR}/gizmesh/gizmesh
    ${EXTERNAL_DIR}/gizmesh/example/window_example
    ${EXTERNAL_DIR}/gizmesh/example/camera_example    
    FrameData
    Hierarchy
    Gpu
    FrameMetrics
    FunnelPipe
    )
