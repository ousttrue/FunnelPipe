set(TARGET_NAME FunnelPipe)
add_executable(${TARGET_NAME}
    Renderer.cpp
    ${EXTERNAL_DIR}/imgui/imgui.cpp
    ${EXTERNAL_DIR}/imgui/imgui_draw.cpp
    ${EXTERNAL_DIR}/imgui/imgui_widgets.cpp
    ${EXTERNAL_DIR}/imgui/imgui_demo.cpp
    Gui/Gui.cpp
    Gui/ImGuiDX12.cpp
    Gui/ImGuiDX12FrameResources.cpp
    Gui/ImGuiImplScreenState.cpp
    View/CameraView.cpp
    View/Gizmo.cpp
    Application.cpp
    main.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/plog/include
    ${EXTERNAL_DIR}/nlohmann_json/include
    ${EXTERNAL_DIR}/imgui
    ${EXTERNAL_DIR}/gizmesh/example/camera_example
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    # falg
    window_example
    camera_example
    gizmesh   
    Hierarchy 
    FrameMetrics
    Gpu
    #
    dxgi
    d3dcompiler
    )
