set(TARGET_NAME FrameMetrics)
add_library(${TARGET_NAME}
    frame_metrics.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR})
