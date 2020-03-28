set(TARGET_NAME FrameData)
add_library(${TARGET_NAME}
    DrawList.cpp
    SceneImage.cpp
    SceneMaterial.cpp
    SceneMesh.cpp
    VertexBuffer.cpp
    Shader.cpp
    ShaderManager.cpp
    ShaderWatcher.cpp
    ShaderConstantVariable.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME}
    PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE 
    ${EXTERNAL_DIR}/stb
    ${EXTERNAL_DIR}/plog/include
    )
target_link_libraries(${TARGET_NAME}
    PUBLIC
    falg
    )
