// Window.hpp - Linux/SDL2 implementation
// Provides the same public API as the Windows Win32 version

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <map>
#include <vector>
#include <cstdint>

// Compatibility types replacing Windows types
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMENU;
typedef void* HACCEL;
typedef uint32_t UINT;
typedef uint32_t DWORD;

namespace WindowClass {

// Forward declaration for compatibility
class Window;

} // namespace WindowClass

/**
 * @brief Base window class providing SDL2 window functionality with menu support
 */
class Window {
public:
    enum class SizeMode {
        TOTAL_WINDOW,
        CLIENT_AREA
    };

    using MenuCallback = std::function<void()>;

    Window(const std::string& class_name, const std::string& title);
    virtual ~Window();

    bool Create(int width, int height, SizeMode mode = SizeMode::CLIENT_AREA);
    void Show(int show_command = 1);
    bool ProcessMessages();
    bool ProcessWindowMessages();
    bool SetCaption(const std::string& caption);
    std::string GetCaption() const;

    // Returns the SDL window pointer cast to HWND for compatibility
    HWND GetHandle() const { return (HWND)m_sdl_window; }
    HINSTANCE GetInstance() const { return nullptr; }

    bool SetEnabled(bool enabled);
    bool IsEnabled() const;

    bool SetWindowSize(int width, int height);
    bool SetClientSize(int width, int height);
    bool GetWindowSize(int& width, int& height) const;
    bool GetClientSize(int& width, int& height) const;
    bool SetWindowPosition(int x, int y);
    bool GetWindowPosition(int& x, int& y) const;
    bool SetWindowRect(int x, int y, int width, int height, SizeMode mode = SizeMode::CLIENT_AREA);

    bool SetUserResizable(bool resizable);
    bool IsUserResizable() const { return m_user_resizable; }
    void SetInitialUserResizable(bool resizable) { m_user_resizable = resizable; }

    void CreateMenuBar();
    HMENU AddMenu(const std::string& name, const std::string& text);
    void AddMenuItem(const std::string& menu_name, const std::string& item_name,
                     const std::string& text, const std::string& shortcut = "",
                     MenuCallback callback = nullptr);
    HMENU AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
                     const std::string& text);
    void AddMenuSeparator(const std::string& menu_name);
    void SetMenuItemEnabled(const std::string& item_name, bool enabled);
    void SetMenuItemChecked(const std::string& item_name, bool checked);
    bool GetMenuItemChecked(const std::string& item_name) const;

    std::string ShowOpenFileDialog(const std::string& title = "Open File",
                                   const std::string& filter = "",
                                   const std::string& default_ext = "",
                                   const std::string& initial_dir = "");
    std::string ShowSaveFileDialog(const std::string& title = "Save File",
                                   const std::string& filter = "",
                                   const std::string& default_ext = "",
                                   const std::string& initial_dir = "",
                                   const std::string& default_name = "");
    std::vector<std::string> ShowOpenMultipleFilesDialog(const std::string& title = "Open Files",
                                                          const std::string& filter = "",
                                                          const std::string& initial_dir = "");

protected:
    virtual bool HandleEvent(const SDL_Event& event);
    virtual bool PostCreateInitialize() { return true; }

    SDL_Window* m_sdl_window;
    std::string m_class_name;
    std::string m_title;
    bool m_user_resizable;
    bool m_enabled;
    bool m_quit_requested;

private:
    struct MenuItemInfo {
        std::string name;
        MenuCallback callback;
        bool enabled;
        bool checked;
    };

    // Simple text-based menu tracking (rendered on key press on Linux)
    std::map<std::string, std::vector<std::string>> m_menu_items_order;
    std::map<std::string, MenuItemInfo> m_menu_items;
    std::vector<std::string> m_menu_order;
};
