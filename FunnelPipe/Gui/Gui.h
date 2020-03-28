#pragma once
// #include <hierarchy.h>
#include <ScreenState.h>

namespace gui
{

class Gui
{
    class GuiImpl *m_impl = nullptr;

public:
    Gui();
    ~Gui();
    void Log(const char *msg);
    void OnFrame(const screenstate::ScreenState &state);
};

} // namespace gui
