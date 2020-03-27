set(TARGET_NAME Drawlist)
add_library(${TARGET_NAME}
    DrawList.cpp
    )
target_include_directories(${TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
    )
