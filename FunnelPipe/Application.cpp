#include "Application.h"
#include "SceneManager.h"
#include "Gui/Gui.h"
#include "Graphics/Renderer.h"
#include "View/CameraView.h"
#include <ScreenState.h>
#include <frame_metrics.h>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include "LogConfig.h"

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    SceneManager m_scene;

    Renderer m_renderer;
    gui::Gui m_imgui;

    CameraView m_view;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_scene(argc, argv), m_renderer(256), m_view()
    {
        m_imGuiAppender.onWrite(std::bind(&gui::Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);
    }

    void OnFrame(void *hwnd, const screenstate::ScreenState &state)
    {
        if (!m_initialized)
        {
            m_renderer.Initialize(hwnd);
            m_initialized = true;
        }

        // imgui
        bool isShowView = false;
        {
            frame_metrics::scoped s("imgui");
            // imgui
            m_imgui.OnFrame(state, std::bind(&SceneManager::OpenFile, &m_scene, std::placeholders::_1));

            // view
            auto viewTextureID = m_renderer.ViewTextureID(m_view.SceneView());
            isShowView = m_view.ImGui(state, viewTextureID);

            // model panel
            m_scene.ImGui();
        }

        // renderering
        {
            frame_metrics::scoped s("render");
            m_renderer.BeginFrame(hwnd, state.Width, state.Height);
            if (isShowView)
            {
                frame_metrics::scoped ss("view");
                auto sceneView = m_view.SceneView();
                // m_scene.Update();
                // UpdateDrawList();
                m_renderer.View(sceneView, m_scene.Drawlist());
            }
            m_renderer.EndFrame();
        }
    }

    // void UpdateDrawList()
    // {
    //     m_sceneView->UpdateDrawList(&m_scene);

    //     // gizmo
    //     if (m_sceneView->ShowGizmo)
    //     {
    //         auto mesh = m_view.GizmoMesh();
    //         if (mesh)
    //         {
    //             auto shader = mesh->submeshes[0].material->shader->Compiled();
    //             if (shader)
    //             {
    //                 m_gizmoBuffer = m_view.GizmoBuffer();
    //                 std::array<float, 16> matrix{
    //                     1, 0, 0, 0, //
    //                     0, 1, 0, 0, //
    //                     0, 0, 1, 0, //
    //                     0, 0, 0, 1, //
    //                 };
    //                 hierarchy::CBValue values[] =
    //                     {
    //                         {
    //                             .semantic = hierarchy::ConstantSemantics::NODE_WORLD,
    //                             .p = &matrix,
    //                             .size = sizeof(matrix),
    //                         }};
    //                 m_sceneView->Drawlist.PushCB(shader->VS.DrawCB(), values, _countof(values));
    //                 m_sceneView->Drawlist.Items.push_back({
    //                     .Mesh = m_view.GizmoMesh(),
    //                     .Vertices = {
    //                         .Ptr = m_gizmoBuffer.pVertices,
    //                         .Size = m_gizmoBuffer.verticesBytes,
    //                         .Stride = m_gizmoBuffer.vertexStride,
    //                     },
    //                     .Indices = {
    //                         .Ptr = m_gizmoBuffer.pIndices,
    //                         .Size = m_gizmoBuffer.indicesBytes,
    //                         .Stride = m_gizmoBuffer.indexStride,
    //                     },
    //                     .SubmeshIndex = 0,
    //                 });
    //             }
    //         }
    //     }
    // }
};

Application::Application(int argc, char **argv)
    : m_impl(new ApplicationImpl(argc, argv))
{
}

Application::~Application()
{
    delete m_impl;
}

void Application::OnFrame(void *hwnd, const screenstate::ScreenState &state)
{
    m_impl->OnFrame(hwnd, state);
}
