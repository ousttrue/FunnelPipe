set(TARGET_NAME plog)
add_library(${TARGET_NAME} INTERFACE)
target_include_directories(${TARGET_NAME}
                           INTERFACE ${CMAKE_CURRENT_LIST_DIR}/plog/include)
