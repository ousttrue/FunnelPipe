set(TARGET_NAME Gpu)
add_library(${TARGET_NAME}
    CommandQueue.cpp
    RenderTarget.cpp
    SwapChain.cpp
    CommandList.cpp
    ResourceItem.cpp
    Uploader.cpp
    Mesh.cpp
    Heap.cpp
    Texture.cpp
    ConstantBuffer.cpp
    #
    # Material.cpp
    # RootSignature.cpp
    # SceneMapper.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
    ${EXTERNAL_DIR}/plog/include
    )
target_link_libraries(${TARGET_NAME} PUBLIC
    Drawlist
    d3d12
    )
