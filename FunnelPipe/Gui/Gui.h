#pragma once
#include <ScreenState.h>
#include <functional>
#include <filesystem>

namespace gui
{

using FileOpenFunc = std::function<void(const std::filesystem::path &)>;

class Gui
{
    class GuiImpl *m_impl = nullptr;

public:
    Gui();
    ~Gui();
    void Log(const char *msg);
    void OnFrame(const screenstate::ScreenState &state,
                 const FileOpenFunc &open);
};

} // namespace gui
