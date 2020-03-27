set(TARGET_NAME FunnelPipe)
add_executable(${TARGET_NAME}
    Renderer.cpp
    Application.cpp
    main.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/plog/include
    ${EXTERNAL_DIR}/nlohmann_json/include
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    # falg
    window_example
    # gizmesh    
    FrameMetrics
    Gpu
    #
    dxgi
    )
