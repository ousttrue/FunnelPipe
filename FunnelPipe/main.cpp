#include "Application.h"
#include <frame_metrics.h>
#include <Win32Window.h>
#include "save_windowplacement.h"
#include <filesystem>
#include <iostream>
#include <ToUnicode.h>

const auto CLASS_NAME = L"FunnelPipeClass";
const auto WINDOW_NAME = L"FunnelPipe";

int main(int argc, char **argv)
{
    screenstate::Win32Window window(CLASS_NAME);
    auto hwnd = window.Create(WINDOW_NAME);
    if (!hwnd)
    {
        std::cerr << "fail to window.Create";
        return 1;
    }
    window.Show();

    Application app(argc, argv);

    auto windowconf = std::filesystem::current_path().append("FunnelPipe.window.json").u16string();
    windowplacement::Restore(hwnd, SW_SHOW, (const wchar_t *)windowconf.c_str());
    window.OnDestroy = [hwnd, conf = windowconf]() {
        windowplacement::Save(hwnd, (const wchar_t *)conf.c_str());
    };

    {
        screenstate::ScreenState state;
        while (true)
        {
            frame_metrics::new_frame();
            frame_metrics::scoped s("frame");
            {
                frame_metrics::scoped ss("window");
                if (!window.TryGetInput(&state))
                {
                    break;
                }
            }
            {
                frame_metrics::scoped ss("app");
                app.OnFrame(hwnd, state);
            }
        }
    }

    return 0;
}
