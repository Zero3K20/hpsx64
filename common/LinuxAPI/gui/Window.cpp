// Window.cpp - Linux/SDL2 implementation

#include "Window.hpp"
#include "ConsoleWindow.hpp"
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <sstream>

// Use zenity for file dialogs if available on Linux/Steam Deck.
// Falls back to kdialog (KDE / Steam Deck Plasma desktop) if zenity is not found.
static std::string RunZenity(const std::vector<std::string>& args)
{
    // First try zenity (GTK)
    std::string cmd = "zenity";
    for (const auto& a : args) {
        cmd += " '";
        for (char c : a) {
            if (c == '\'') cmd += "'\\''";
            else cmd += c;
        }
        cmd += "'";
    }
    cmd += " 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        std::string result;
        char buf[1024];
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        int rc = pclose(pipe);
        if (rc == 0 && !result.empty()) {
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            return result;
        }
    }

    // zenity not found or returned no result; try kdialog (KDE/Steam Deck)
    // Build an equivalent kdialog command from the zenity-style args.
    // Supported translations: --file-selection → --getopenfilename
    //                         --file-selection --save → --getsavefilename
    //                         --title <t>           → --title <t>
    //                         --filename <f>        → starts in that directory
    bool is_save    = false;
    bool is_multi   = false;
    std::string title_arg;
    std::string filename_arg;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--save")             is_save    = true;
        if (args[i] == "--multiple")         is_multi   = true;
        if (args[i] == "--title"    && i + 1 < args.size()) title_arg    = args[++i];
        if (args[i] == "--filename" && i + 1 < args.size()) filename_arg = args[++i];
    }

    std::string kcmd = "kdialog";
    if (!title_arg.empty()) {
        kcmd += " --title '";
        for (char c : title_arg) { if (c == '\'') kcmd += "'\\''"; else kcmd += c; }
        kcmd += "'";
    }
    if (is_save)
        kcmd += " --getsavefilename";
    else
        kcmd += " --getopenfilename";

    if (!filename_arg.empty()) {
        kcmd += " '";
        for (char c : filename_arg) { if (c == '\'') kcmd += "'\\''"; else kcmd += c; }
        kcmd += "'";
    }

    // kdialog supports multiple selection by passing --multiple after the path
    if (is_multi)
        kcmd += " --multiple";

    kcmd += " 2>/dev/null";

    FILE* kpipe = popen(kcmd.c_str(), "r");
    if (!kpipe) return "";

    std::string result;
    char buf[1024];
    while (fgets(buf, sizeof(buf), kpipe)) result += buf;
    pclose(kpipe);

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result;
}

// ---------------------------------------------------------------------------
// Static helper: parse "F2", "Ctrl+R", "F10", "Escape", "Return", etc.
// into an SDL_Keycode and a required-modifier mask.
// Returns true when the shortcut was recognised.
bool Window::ParseShortcutString(const std::string& shortcut,
                                  SDL_Keycode& out_key, SDL_Keymod& out_mod)
{
    if (shortcut.empty()) return false;

    // Split on '+', collecting modifier tokens and the final key token.
    std::vector<std::string> parts;
    {
        std::istringstream ss(shortcut);
        std::string token;
        while (std::getline(ss, token, '+')) {
            if (!token.empty()) parts.push_back(token);
        }
    }
    if (parts.empty()) return false;

    SDL_Keymod mod = KMOD_NONE;
    SDL_Keycode key = SDLK_UNKNOWN;

    for (const auto& part : parts) {
        // Convert to lower-case for comparison
        std::string lc = part;
        std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);

        if (lc == "ctrl"  || lc == "control") { mod = (SDL_Keymod)(mod | KMOD_CTRL);  continue; }
        if (lc == "shift")                     { mod = (SDL_Keymod)(mod | KMOD_SHIFT); continue; }
        if (lc == "alt")                       { mod = (SDL_Keymod)(mod | KMOD_ALT);   continue; }

        // Key tokens
        if (lc == "escape" || lc == "esc")    { key = SDLK_ESCAPE;    continue; }
        if (lc == "return" || lc == "enter")  { key = SDLK_RETURN;    continue; }
        if (lc == "space")                     { key = SDLK_SPACE;     continue; }
        if (lc == "tab")                       { key = SDLK_TAB;       continue; }
        if (lc == "delete" || lc == "del")    { key = SDLK_DELETE;    continue; }
        if (lc == "backspace")                 { key = SDLK_BACKSPACE; continue; }

        // Function keys F1-F12
        if (lc.size() >= 2 && lc[0] == 'f' && std::isdigit((unsigned char)lc[1])) {
            int fnum = std::stoi(lc.substr(1));
            if (fnum >= 1 && fnum <= 12) {
                key = (SDL_Keycode)(SDLK_F1 + (fnum - 1));
                continue;
            }
        }

        // Single letter / digit
        if (lc.size() == 1) {
            key = (SDL_Keycode)(Uint8)lc[0];
            continue;
        }
    }

    if (key == SDLK_UNKNOWN) return false;
    out_key = key;
    out_mod = mod;
    return true;
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

    Uint32 flags = SDL_WINDOW_SHOWN | GetExtraWindowFlags();
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
    Uint32 my_win_id = m_sdl_window ? SDL_GetWindowID(m_sdl_window) : 0;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_quit_requested = true;
            return false;
        }

        // Route console-window events; don't let them close the game window.
        if (event.type == SDL_WINDOWEVENT) {
            if (g_console_window && g_console_window->IsOpen() &&
                event.window.windowID == g_console_window->GetWindowID()) {
                g_console_window->HandleWindowEvent(event.window);
                continue;
            }
            // Only treat close as quit if it's OUR window.
            if (my_win_id && event.window.windowID != my_win_id)
                continue;
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                m_quit_requested = true;
                return false;
            }
        }

        // Skip keyboard/mouse events that belong to other windows.
        if (my_win_id) {
            Uint32 evt_win = 0;
            if      (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
                evt_win = event.key.windowID;
            else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
                evt_win = event.button.windowID;
            else if (event.type == SDL_MOUSEMOTION)
                evt_win = event.motion.windowID;

            if (evt_win && evt_win != my_win_id)
                continue;
        }

        HandleEvent(event);
    }

    // Refresh console window display while game processes messages.
    if (g_console_window && g_console_window->IsOpen())
        g_console_window->Update();

    return !m_quit_requested;
}

bool Window::ProcessWindowMessages()
{
    return ProcessMessages();
}

bool Window::HandleEvent(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode sym = event.key.keysym.sym;
        // Build a normalised modifier mask (ignore side-specific ctrl/shift/alt bits)
        Uint16 rawmod = event.key.keysym.mod;
        SDL_Keymod pressed = KMOD_NONE;
        if (rawmod & KMOD_CTRL)  pressed = (SDL_Keymod)(pressed | KMOD_CTRL);
        if (rawmod & KMOD_SHIFT) pressed = (SDL_Keymod)(pressed | KMOD_SHIFT);
        if (rawmod & KMOD_ALT)   pressed = (SDL_Keymod)(pressed | KMOD_ALT);

        // Show keyboard-shortcut help on F1
        if (sym == SDLK_F1 && pressed == KMOD_NONE) {
            // Build a help string from all registered shortcuts
            std::string msg = "Keyboard shortcuts:\n\n";
            for (const auto& se : m_shortcuts) {
                std::string keyname;
                if (se.key >= SDLK_F1 && se.key <= SDLK_F12)
                    keyname = "F" + std::to_string(se.key - SDLK_F1 + 1);
                else
                    keyname = std::string(SDL_GetKeyName(se.key));
                if (se.mod & KMOD_CTRL)  keyname = "Ctrl+" + keyname;
                if (se.mod & KMOD_SHIFT) keyname = "Shift+" + keyname;
                if (se.mod & KMOD_ALT)   keyname = "Alt+" + keyname;

                auto it = m_menu_items.find(se.item_name);
                if (it != m_menu_items.end())
                    msg += keyname + " : " + it->second.name + "\n";
            }
            if (m_shortcuts.empty())
                msg += "(none registered)\n";
            msg += "\nF1 : Show this help";
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Help", msg.c_str(), m_sdl_window);
            return true;
        }

        // Dispatch registered shortcuts
        for (const auto& se : m_shortcuts) {
            if (se.key == sym && pressed == se.mod) {
                auto it = m_menu_items.find(se.item_name);
                if (it != m_menu_items.end() && it->second.enabled && it->second.callback) {
                    it->second.callback();
                    return true;
                }
            }
        }
    }
    return false;
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

HMENU Window::AddMenu(const std::string& name, const std::string& text)
{
    m_menu_order.push_back(name);
    m_menu_display_text[name] = text;
    return nullptr;
}

void Window::AddMenuItem(const std::string& menu_name, const std::string& item_name,
                          const std::string& text, const std::string& shortcut,
                          MenuCallback callback)
{
    MenuItemInfo info;
    info.name = text.empty() ? item_name : text;
    info.callback = callback;
    info.enabled = true;
    info.checked = false;
    m_menu_items[item_name] = info;
    m_menu_items_order[menu_name].push_back(item_name);

    // Register keyboard shortcut if one was provided
    if (!shortcut.empty() && callback) {
        SDL_Keycode key   = SDLK_UNKNOWN;
        SDL_Keymod  mod   = KMOD_NONE;
        if (ParseShortcutString(shortcut, key, mod)) {
            ShortcutEntry se;
            se.key       = key;
            se.mod       = mod;
            se.item_name = item_name;
            m_shortcuts.push_back(se);
        }
    }
}

HMENU Window::AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
                          const std::string& text)
{
    m_menu_order.push_back(submenu_name);
    m_menu_items_order[parent_name].push_back(submenu_name);
    m_menu_display_text[submenu_name] = text;
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
