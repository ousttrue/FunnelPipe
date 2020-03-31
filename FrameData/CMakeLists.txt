set(TARGET_NAME FrameData)
add_library(${TARGET_NAME}
    FrameData.cpp
    FrameImage.cpp
    FrameTexture.cpp
    FrameMesh.cpp
    VertexBuffer.cpp
    Shader.cpp
    ShaderConstantVariable.cpp
    ToUnicode.cpp
    DirectoryWatcher.cpp
    ShaderManager.cpp
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
