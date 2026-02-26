// Window.cpp - Linux/SDL2 implementation

#include "Window.hpp"
#include <iostream>
#include <algorithm>
#include <cstdio>

// Use zenity for file dialogs if available on Linux/Steam Deck
static std::string RunZenity(const std::vector<std::string>& args)
{
    std::string cmd = "zenity";
    for (const auto& a : args) {
        cmd += " ";
        // Basic escaping for shell
        cmd += "'";
        std::string escaped = a;
        std::string out;
        for (char c : escaped) {
            if (c == '\'') out += "'\\''";
            else out += c;
        }
        cmd += out;
        cmd += "'";
    }
    cmd += " 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    std::string result;
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    pclose(pipe);

    // Remove trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result;
}

Window::Window(const std::string& class_name, const std::string& title)
    : m_sdl_window(nullptr)
    , m_class_name(class_name)
    , m_title(title)
    , m_user_resizable(true)
    , m_enabled(true)
    , m_quit_requested(false)
{
}

Window::~Window()
{
    if (m_sdl_window) {
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
    }
}

bool Window::Create(int width, int height, SizeMode /*mode*/)
{
    if (m_sdl_window) return true;

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (m_user_resizable) flags |= SDL_WINDOW_RESIZABLE;

    m_sdl_window = SDL_CreateWindow(
        m_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags
    );

    if (!m_sdl_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    return PostCreateInitialize();
}

void Window::Show(int /*show_command*/)
{
    if (m_sdl_window)
        SDL_ShowWindow(m_sdl_window);
}

bool Window::ProcessMessages()
{
    SDL_Event event;
    bool had_events = false;

    while (SDL_PollEvent(&event)) {
        had_events = true;
        if (event.type == SDL_QUIT) {
            m_quit_requested = true;
            return false;
        }
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                m_quit_requested = true;
                return false;
            }
        }
        HandleEvent(event);
    }
    return !m_quit_requested;
}

bool Window::ProcessWindowMessages()
{
    return ProcessMessages();
}

bool Window::HandleEvent(const SDL_Event& /*event*/)
{
    return true;
}

bool Window::SetCaption(const std::string& caption)
{
    m_title = caption;
    if (m_sdl_window) {
        SDL_SetWindowTitle(m_sdl_window, caption.c_str());
        return true;
    }
    return false;
}

std::string Window::GetCaption() const
{
    if (m_sdl_window)
        return SDL_GetWindowTitle(m_sdl_window);
    return m_title;
}

bool Window::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    return true;
}

bool Window::IsEnabled() const
{
    return m_enabled;
}

bool Window::SetWindowSize(int width, int height)
{
    if (!m_sdl_window) return false;
    SDL_SetWindowSize(m_sdl_window, width, height);
    return true;
}

bool Window::SetClientSize(int width, int height)
{
    return SetWindowSize(width, height);
}

bool Window::GetWindowSize(int& width, int& height) const
{
    if (!m_sdl_window) return false;
    SDL_GetWindowSize(m_sdl_window, &width, &height);
    return true;
}

bool Window::GetClientSize(int& width, int& height) const
{
    return GetWindowSize(width, height);
}

bool Window::SetWindowPosition(int x, int y)
{
    if (!m_sdl_window) return false;
    SDL_SetWindowPosition(m_sdl_window, x, y);
    return true;
}

bool Window::GetWindowPosition(int& x, int& y) const
{
    if (!m_sdl_window) return false;
    SDL_GetWindowPosition(m_sdl_window, &x, &y);
    return true;
}

bool Window::SetWindowRect(int x, int y, int width, int height, SizeMode /*mode*/)
{
    if (!m_sdl_window) return false;
    SDL_SetWindowPosition(m_sdl_window, x, y);
    SDL_SetWindowSize(m_sdl_window, width, height);
    return true;
}

bool Window::SetUserResizable(bool resizable)
{
    m_user_resizable = resizable;
    // SDL2 doesn't support changing resizability after creation
    return true;
}

void Window::CreateMenuBar()
{
    // No-op: menus not rendered natively on Linux in this implementation
}

HMENU Window::AddMenu(const std::string& name, const std::string& /*text*/)
{
    m_menu_order.push_back(name);
    return nullptr;
}

void Window::AddMenuItem(const std::string& menu_name, const std::string& item_name,
                          const std::string& /*text*/, const std::string& /*shortcut*/,
                          MenuCallback callback)
{
    MenuItemInfo info;
    info.name = item_name;
    info.callback = callback;
    info.enabled = true;
    info.checked = false;
    m_menu_items[item_name] = info;
    m_menu_items_order[menu_name].push_back(item_name);
}

HMENU Window::AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
                          const std::string& /*text*/)
{
    m_menu_order.push_back(submenu_name);
    m_menu_items_order[parent_name].push_back(submenu_name);
    return nullptr;
}

void Window::AddMenuSeparator(const std::string& /*menu_name*/)
{
    // No-op on Linux
}

void Window::SetMenuItemEnabled(const std::string& item_name, bool enabled)
{
    auto it = m_menu_items.find(item_name);
    if (it != m_menu_items.end())
        it->second.enabled = enabled;
}

void Window::SetMenuItemChecked(const std::string& item_name, bool checked)
{
    auto it = m_menu_items.find(item_name);
    if (it != m_menu_items.end())
        it->second.checked = checked;
}

bool Window::GetMenuItemChecked(const std::string& item_name) const
{
    auto it = m_menu_items.find(item_name);
    if (it != m_menu_items.end())
        return it->second.checked;
    return false;
}

std::string Window::ShowOpenFileDialog(const std::string& title,
                                        const std::string& /*filter*/,
                                        const std::string& /*default_ext*/,
                                        const std::string& initial_dir)
{
    std::vector<std::string> args = {
        "--file-selection",
        "--title", title
    };
    if (!initial_dir.empty()) {
        args.push_back("--filename");
        args.push_back(initial_dir + "/");
    }
    return RunZenity(args);
}

std::string Window::ShowSaveFileDialog(const std::string& title,
                                        const std::string& /*filter*/,
                                        const std::string& /*default_ext*/,
                                        const std::string& initial_dir,
                                        const std::string& default_name)
{
    std::vector<std::string> args = {
        "--file-selection",
        "--save",
        "--confirm-overwrite",
        "--title", title
    };
    if (!initial_dir.empty() || !default_name.empty()) {
        args.push_back("--filename");
        args.push_back(initial_dir + "/" + default_name);
    }
    return RunZenity(args);
}

std::vector<std::string> Window::ShowOpenMultipleFilesDialog(const std::string& title,
                                                               const std::string& /*filter*/,
                                                               const std::string& initial_dir)
{
    std::vector<std::string> args = {
        "--file-selection",
        "--multiple",
        "--separator", "\n",
        "--title", title
    };
    if (!initial_dir.empty()) {
        args.push_back("--filename");
        args.push_back(initial_dir + "/");
    }
    std::string result = RunZenity(args);
    std::vector<std::string> files;
    if (result.empty()) return files;

    size_t start = 0;
    while (start < result.size()) {
        size_t end = result.find('\n', start);
        if (end == std::string::npos) end = result.size();
        files.push_back(result.substr(start, end - start));
        start = end + 1;
    }
    return files;
}
