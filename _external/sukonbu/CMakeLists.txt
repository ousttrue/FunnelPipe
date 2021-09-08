set(TARGET_NAME gltfformat)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(
  ${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/sukonbu/gltfformat/include)
target_link_libraries(${TARGET_NAME} INTERFACE json)
