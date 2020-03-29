set(TARGET_NAME Hierarchy)
add_library(${TARGET_NAME}
    SceneNode.cpp
    Scene.cpp
    ParseGltf.cpp
    SceneModel.cpp
    SceneMeshSkin.cpp
    )
set_property(TARGET ${TARGET_NAME} 
    PROPERTY CXX_STANDARD 20
    )
target_include_directories(${TARGET_NAME} 
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${EXTERNAL_DIR}/gizmesh/falg
    PRIVATE
    ${EXTERNAL_DIR}/sukonbu/gltfformat/include
    ${EXTERNAL_DIR}/sukonbu/gltfformat/external_include
    ${EXTERNAL_DIR}/plog/include
    )
target_compile_definitions(${TARGET_NAME} PUBLIC
    NOMINMAX
    )
target_link_libraries(${TARGET_NAME} PUBLIC
    FrameData
    )
