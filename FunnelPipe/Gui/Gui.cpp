#include "Gui.h"
#include <frame_metrics.h>
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "ImGuiImplScreenState.h"
#include <plog/Log.h>
#include <mutex>
#include <functional>
// #include <IconsFontAwesome4.h>

static uint32_t s_colors[] = {
    IM_COL32(255, 0, 0, 200),
    IM_COL32(0, 255, 0, 200),
    IM_COL32(0, 0, 255, 200),
    IM_COL32(0, 255, 255, 200),
    IM_COL32(255, 0, 255, 200),
    IM_COL32(255, 255, 0, 200),
};

#include <shobjidl.h>
#include <wrl/client.h>
std::wstring OpenFileDialog(const std::wstring &folder, void *hwnd)
{
    Microsoft::WRL::ComPtr<IFileOpenDialog> pFileOpen;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                IID_PPV_ARGS(&pFileOpen))))
    {
        return L"";
    }

    COMDLG_FILTERSPEC fileTypes[] = {
        {L"3D format", L"*.vrm;*.glb;"},
        {L"vrm format", L"*.vrm"},
        {L"gltf binary format", L"*.glb"},
        {L"all", L"*.*"},
    };
    if (FAILED(pFileOpen->SetFileTypes(_countof(fileTypes), fileTypes)))
    {
        return L"";
    }
    // if (FAILED(pFileOpen->SetDefaultExtension(L".vrm")))
    // {
    //     return L"";
    // }
    if (FAILED(pFileOpen->Show((HWND)hwnd)))
    {
        return L"";
    }

    Microsoft::WRL::ComPtr<IShellItem> pItem;
    if (FAILED(pFileOpen->GetResult(&pItem)))
    {
        return L"";
    }

    PWSTR pszFilePath;
    if (FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
    {
        return L"";
    }
    std::wstring result(pszFilePath);
    CoTaskMemFree(pszFilePath);

    // DWORD len = GetCurrentDirectoryW(0, NULL);
    // std::vector<wchar_t> dir(len);
    // GetCurrentDirectoryW((DWORD)dir.size(), dir.data());
    // if(dir.back()==0)
    // {
    //     dir.pop_back();
    // }
    // std::wcout << std::wstring(dir.begin(), dir.end()) << std::endl;

    return result;
}

struct ExampleAppLog
{
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool AutoScroll;           // Keep scrolling if already at the bottom
    std::mutex m_mutex;

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    // may from another thread
    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        std::lock_guard<std::mutex> scoped(m_mutex);

        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        {
            std::lock_guard<std::mutex> scoped(m_mutex);
            const char *buf = Buf.begin();
            const char *buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have a random access on the result on our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
                // especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
                // Here we instead demonstrate using the clipper to only process lines that are within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
                // Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
                // both of which we can handle since we an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
                // Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char *line_start = buf + LineOffsets[line_no];
                        const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        ImGui::TextUnformatted(line_start, line_end);
                    }
                }
                clipper.End();
            }
        }

        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};

static void ImGui_Impl_Win32_UpdateMouseCursor()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
        return;
    }

    // Show OS mouse cursor
    LPTSTR win32_cursor = IDC_ARROW;
    switch (imgui_cursor)
    {
    case ImGuiMouseCursor_Arrow:
        win32_cursor = IDC_ARROW;
        break;
    case ImGuiMouseCursor_TextInput:
        win32_cursor = IDC_IBEAM;
        break;
    case ImGuiMouseCursor_ResizeAll:
        win32_cursor = IDC_SIZEALL;
        break;
    case ImGuiMouseCursor_ResizeEW:
        win32_cursor = IDC_SIZEWE;
        break;
    case ImGuiMouseCursor_ResizeNS:
        win32_cursor = IDC_SIZENS;
        break;
    case ImGuiMouseCursor_ResizeNESW:
        win32_cursor = IDC_SIZENESW;
        break;
    case ImGuiMouseCursor_ResizeNWSE:
        win32_cursor = IDC_SIZENWSE;
        break;
    case ImGuiMouseCursor_Hand:
        win32_cursor = IDC_HAND;
        break;
    case ImGuiMouseCursor_NotAllowed:
        win32_cursor = IDC_NO;
        break;
    }
    ::SetCursor(::LoadCursor(NULL, win32_cursor));
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
static void DockSpace(const std::function<void(const std::filesystem::path)> &open, void *hwnd)
{
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO &io = ImGui::GetIO();
    // if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    // else
    // {
    //     ShowDockingDisabledMessage();
    // }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("open"))
            {
                auto path = OpenFileDialog(L"", hwnd);
                open(path);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Docking"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.
            //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
            ImGui::Separator();
            // if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
            //     *p_open = false;
            ImGui::EndMenu();
        }
        // HelpMarker(
        //     "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!"
        //     "\n\n"
        //     " > if io.ConfigDockingWithShift==false (default):"
        //     "\n"
        //     "   drag windows from title bar to dock"
        //     "\n"
        //     " > if io.ConfigDockingWithShift==true:"
        //     "\n"
        //     "   drag windows from anywhere and hold Shift to dock"
        //     "\n\n"
        //     "This demo app has nothing to do with it!"
        //     "\n\n"
        //     "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)."
        //     "\n\n"
        //     "ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame."
        //     "\n\n"
        //     "(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

namespace gui
{
//
// GuiImpl
//
class GuiImpl
{
    std::unique_ptr<struct ExampleAppLog> m_logger;

public:
    GuiImpl()
        : m_logger(new ExampleAppLog)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_Impl_ScreenState_Init();

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.txt' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        {
            auto filename = "C:/Windows/Fonts/meiryo.ttc";
            float size_pixels = 24.0f;
            ImVector<ImWchar> ranges;
            ImFontGlyphRangesBuilder builder;
            // builder.AddText("奈也");
            // builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
            // builder.BuildRanges(&ranges);
            ImFont *font = io.Fonts->AddFontFromFileTTF(filename, size_pixels, NULL, ranges.Data);
        }
        // {
        //     float size_icon = 20.0f;
        //     ImFontConfig config;
        //     config.MergeMode = true;
        //     config.PixelSnapH = true;
        //     config.GlyphMinAdvanceX = size_icon;
        //     static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        //     io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", size_icon, &config, icon_ranges);
        //     // io.Fonts->Build();
        // }
    }

    ~GuiImpl()
    {
        ImGui::DestroyContext();
    }

    void Log(const char *msg)
    {
        m_logger->AddLog(msg);
    }

    void NewFrame(const screenstate::ScreenState &state,
                  const FileOpenFunc &open, void *hwnd)
    {
        // Start the Dear ImGui frame
        ImGui_Impl_ScreenState_NewFrame(state);
        if (state.Has(screenstate::MouseButtonFlags::CursorUpdate))
        {
            ImGui_Impl_Win32_UpdateMouseCursor();
        }

        ImGui::NewFrame();

        DockSpace(open, hwnd);

        Update();
    }

private:
    void Update()
    {
        ImGui::Begin("Performance");
        {
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            auto width = ImGui::GetWindowContentRegionWidth();
            const float TIME_RANGE = 2.0f / 60.0f;
            const float TIME_RANGE_INV = 1.0f / TIME_RANGE;
            {
                ImVec2 p = ImGui::GetCursorScreenPos();

                ImGui::PlotHistogram("frame", frame_metrics::imgui_plot, NULL, 60, 0, NULL, 0, TIME_RANGE, ImVec2(width, 100));

                //trying to add lines on top of my image here:
                p.y += 50;
                ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + width, p.y), IM_COL32(255, 0, 0, 200), 1.0f);
            }

            {
                ImVec2 p = ImGui::GetCursorScreenPos();
                int count;
                auto section = frame_metrics::get_sections(&count);
                for (int i = 0; i < count; ++i, ++section)
                {
                    float start = width * section->start * TIME_RANGE_INV;
                    float end = width * section->end * TIME_RANGE_INV;
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + start, p.y), ImVec2(p.x + end, p.y + 20), s_colors[i % _countof(s_colors)]);
                }
            }
        }
        ImGui::End();

        {
            m_logger->Draw("logger");
        }
        {
            ImGui::ShowDemoWindow();
        }
    }
};

//
// Gui
//
Gui::Gui()
    : m_impl(new GuiImpl)
{
}

Gui::~Gui()
{
    delete m_impl;
}

void Gui::Log(const char *msg)
{
    m_impl->Log(msg);
}

void Gui::OnFrame(const screenstate::ScreenState &state,
                  const FileOpenFunc &open, void *hwnd)
{
    m_impl->NewFrame(state, open, hwnd);
}

void Gui::FrameData(const framedata::FrameData &framedata)
{
    ImGui::Begin("FrameData");

    // ImGui::Text("With border:");
    ImGui::Columns(4, "mycolumns"); // 4-ways, with border
    ImGui::Separator();
    ImGui::Text("index");
    ImGui::NextColumn();
    ImGui::Text("mesh");
    ImGui::NextColumn();
    ImGui::Text("submesh");
    ImGui::NextColumn();
    ImGui::Text("material");
    ImGui::NextColumn();
    ImGui::Separator();
    // const char *names[3] = {"One", "Two", "Three"};
    // const char *paths[3] = {"/path/one", "/path/two", "/path/three"};
    static int selected = -1;
    int i = 0;
    for (auto &item : framedata.Drawlist)
    {
        char label[32];
        sprintf(label, "%04d", i);
        if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SpanAllColumns))
            selected = i;
        bool hovered = ImGui::IsItemHovered();

        // auto &mesh = model->meshes[i];
        ImGui::NextColumn();
        ImGui::Text(item.Mesh->name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%d", item.Submesh.drawCount);
        ImGui::NextColumn();
        ImGui::Text("%s", item.Submesh.material->Name.c_str());
        ImGui::NextColumn();

        ++i;
    }
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}

} // namespace gui
