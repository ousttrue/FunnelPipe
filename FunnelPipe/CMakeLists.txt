set(TARGET_NAME FunnelPipe)
add_executable(${TARGET_NAME}
    ${EXTERNAL_DIR}/imgui/imgui.cpp
    ${EXTERNAL_DIR}/imgui/imgui_draw.cpp
    ${EXTERNAL_DIR}/imgui/imgui_widgets.cpp
    ${EXTERNAL_DIR}/imgui/imgui_demo.cpp
    # Graphics/DX12/Renderer.cpp
    # Graphics/DX12/ImGuiDX12.cpp
    # Graphics/DX12/ImGuiDX12FrameResources.cpp

    Graphics/DX11/Renderer.cpp
    ${EXTERNAL_DIR}/imgui/examples/imgui_impl_dx11.cpp

    Graphics/SceneMapper.cpp
    Gui/Gui.cpp
    Gui/ImGuiImplScreenState.cpp
    View/CameraView.cpp
    View/Gizmo.cpp
    View/GuiView.cpp
    SceneManager.cpp
    Application.cpp
    main.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${EXTERNAL_DIR}/plog/include
    ${EXTERNAL_DIR}/nlohmann_json/include
    ${EXTERNAL_DIR}/imgui
    ${EXTERNAL_DIR}/imgui/examples
    ${EXTERNAL_DIR}/gizmesh/example/camera_example
    ${EXTERNAL_DIR}/nameof/include
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
    #
    d3d11
    )
