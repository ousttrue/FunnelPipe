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
#include <plog/Init.h>
#include "LogConfig.h"

class ApplicationImpl
{
    plog::ColorConsoleAppender<plog::MyFormatter> m_consoleAppender;
    plog::MyAppender<plog::MyFormatter> m_imGuiAppender;

    SceneManager m_scene;
    CameraView m_view;
    framedata::FrameData m_frameData;
    gui::Gui m_imgui;

    Renderer m_renderer;

    bool m_initialized = false;

public:
    ApplicationImpl(int argc, char **argv)
        : m_scene(argc, argv), m_renderer(256), m_view()
    {
        m_imGuiAppender.onWrite(std::bind(&gui::Gui::Log, &m_imgui, std::placeholders::_1));
        plog::init(plog::debug, &m_consoleAppender).addAppender(&m_imGuiAppender);

        m_frameData.ViewClearColor = {
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
            auto onOpen = std::bind(&SceneManager::OpenFile, &m_scene, std::placeholders::_1);
            m_imgui.OnFrame(state, onOpen, hwnd);

            // view
            {
                auto viewTexture = m_renderer.ViewTexture((size_t)&m_frameData);
                isShowView = m_view.ImGui(state, viewTexture, m_scene.Selected(), &m_frameData);
                if(!isShowView)
                {
                    m_renderer.ReleaseViewTexture(viewTexture);
                }
            }

            // model panel
            m_scene.ImGui(std::bind(&Renderer::GetTexture, &m_renderer, std::placeholders::_1));

            // update framedata
            m_frameData.Clear();
            if (isShowView)
            {
                m_scene.UpdateFrameData(&m_frameData);
                m_view.UpdateFrameData(&m_frameData);
                m_imgui.FrameData(m_frameData);
            }
        }

        // renderering
        {
            frame_metrics::scoped s("render");
            m_renderer.BeginFrame(hwnd, state.Width, state.Height);
            if (isShowView)
            {
                frame_metrics::scoped ss("view");
                m_renderer.View(m_frameData);
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
