#include "Application.h"
#include "SceneManager.h"
#include "Gui/Gui.h"
#include "Graphics/Renderer.h"
#include "View/CameraView.h"
#include <ScreenState.h>
#include <frame_metrics.h>
#include <FrameData.h>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include "LogConfig.h"

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    SceneManager m_scene;
    CameraView m_view;
    hierarchy::DrawList m_drawlist;
    gui::Gui m_imgui;
    Renderer m_renderer;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_scene(argc, argv), m_renderer(256), m_view()
    {
        m_imGuiAppender.onWrite(std::bind(&gui::Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);

        m_drawlist.ViewClearColor = {
            0.3f,
            0.4f,
            0.5f,
            1.0f,
        };
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
            auto viewTextureID = m_renderer.ViewTextureID((size_t)&m_drawlist);
            isShowView = m_view.ImGui(state, viewTextureID, m_scene.Selected(), &m_drawlist);

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
                m_drawlist.Clear();
                m_scene.UpdateDrawlist(&m_drawlist, m_drawlist.ShowGrid);
                m_view.UpdateDrawlist(&m_drawlist);
                // LOGD << m_drawlist.CBRanges.size() << ", " << m_drawlist.Items.size();

                // buffer.b0Projection = drawlist.Projection;
                // buffer.b0View = drawlist.View;
                // buffer.b0LightDir = m_light->LightDirection;
                // buffer.b0LightColor = m_light->LightColor;
                // buffer.b0CameraPosition = drawlist.CameraPosition;
                // buffer.fovY = drawlist.CameraFovYRadians;
                // buffer.b0ScreenSize = {(float)drawlist.ViewWidth, (float)drawlist.ViewHeight};

                m_renderer.View(m_drawlist);
            }
            m_renderer.EndFrame();
        }
    }
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
