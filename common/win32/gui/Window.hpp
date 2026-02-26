// Window.hpp
#pragma once

#include <windows.h>
#include <commdlg.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <map>
#include <vector>

/**
 * @brief Base window class providing Win32 window functionality with menu support
 * 
 * This class encapsulates basic window creation, message handling, and menu management.
 * It supports setting window size either by total window dimensions or client area dimensions,
 * and allows controlling whether the user can resize the window.
 */
class Window {
public:
    /**
     * @brief Window size specification mode
     */
    enum class SizeMode {
        TOTAL_WINDOW,   ///< Size includes window borders and title bar
        CLIENT_AREA     ///< Size is only the client area
    };

    /**
     * @brief Menu callback function type
     */
    using MenuCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param class_name The window class name to register
     * @param title The window title
     */
    Window(const std::string& class_name, const std::string& title);
    
    /**
     * @brief Destructor
     */
    virtual ~Window();

    /**
     * @brief Create the window
     * @param width Window width
     * @param height Window height
     * @param mode Size mode (total window or client area)
     * @return True if successful
     */
    bool Create(int width, int height, SizeMode mode = SizeMode::CLIENT_AREA);

    /**
     * @brief Show the window
     * @param show_command Show command (SW_SHOW, SW_HIDE, etc.)
     */
    void Show(int show_command = SW_SHOW);

    /**
     * @brief Process all queued messages once (non-blocking)
     * @return True if messages were processed, false if WM_QUIT received
     */
    bool ProcessMessages();

    /**
     * @brief Process all queued messages for this window only (non-blocking)
     * @return True if messages were processed, false if WM_QUIT received
     */
    bool ProcessWindowMessages();

    /**
     * @brief Set the window caption/title
     * @param caption New window caption
     * @return True if successful
     */
    bool SetCaption(const std::string& caption);

    /**
     * @brief Get the current window caption/title
     * @return Current window caption
     */
    std::string GetCaption() const;

    /**
     * @brief Get the window handle
     * @return HWND handle
     */
    HWND GetHandle() const { return m_hwnd; }

    /**
     * @brief Get the window instance
     * @return HINSTANCE handle
     */
    HINSTANCE GetInstance() const { return m_hinstance; }

    /**
     * @brief Enable or disable the window
     * @param enabled Whether to enable the window
     * @return True if successful
     */
    bool SetEnabled(bool enabled);

    /**
     * @brief Get the current enabled state of the window
     * @return True if window is enabled
     */
    bool IsEnabled() const;

    // Window sizing functions

    /**
     * @brief Set the window size (total window including borders and title bar)
     * @param width New window width
     * @param height New window height
     * @return True if successful
     */
    bool SetWindowSize(int width, int height);

    /**
     * @brief Set the client area size (viewable area inside window borders)
     * @param width New client area width
     * @param height New client area height
     * @return True if successful
     */
    bool SetClientSize(int width, int height);

    /**
     * @brief Get the current window size (total window including borders)
     * @param width Output parameter for window width
     * @param height Output parameter for window height
     * @return True if successful
     */
    bool GetWindowSize(int& width, int& height) const;

    /**
     * @brief Get the current client area size (viewable area)
     * @param width Output parameter for client area width
     * @param height Output parameter for client area height
     * @return True if successful
     */
    bool GetClientSize(int& width, int& height) const;

    /**
     * @brief Set window position
     * @param x New X position
     * @param y New Y position
     * @return True if successful
     */
    bool SetWindowPosition(int x, int y);

    /**
     * @brief Get window position
     * @param x Output parameter for X position
     * @param y Output parameter for Y position
     * @return True if successful
     */
    bool GetWindowPosition(int& x, int& y) const;

    /**
     * @brief Set both window position and size
     * @param x New X position
     * @param y New Y position
     * @param width New window width
     * @param height New window height
     * @param mode Size mode (total window or client area)
     * @return True if successful
     */
    bool SetWindowRect(int x, int y, int width, int height, SizeMode mode = SizeMode::CLIENT_AREA);

    // Window resizing control

    /**
     * @brief Enable or disable user resizing of the window
     * @param resizable Whether the user can resize the window
     * @return True if successful
     */
    bool SetUserResizable(bool resizable);

    /**
     * @brief Get whether the user can resize the window
     * @return True if user can resize the window
     */
    bool IsUserResizable() const { return m_user_resizable; }

    /**
     * @brief Set whether the window should be user resizable when created
     * @param resizable Whether to allow user resizing (must be called before Create())
     */
    void SetInitialUserResizable(bool resizable) { m_user_resizable = resizable; }

    // Menu functionality

    /**
     * @brief Create the menu bar
     */
    void CreateMenuBar();

    /**
     * @brief Add a top-level menu
     * @param name Menu name for later reference
     * @param text Menu text
     * @return Menu handle for adding items
     */
    HMENU AddMenu(const std::string& name, const std::string& text);

    /**
     * @brief Add a menu item
     * @param menu_name Parent menu name
     * @param item_name Item name for later reference
     * @param text Item text
     * @param shortcut Keyboard shortcut (e.g., "Ctrl+N")
     * @param callback Callback function when selected
     */
    void AddMenuItem(const std::string& menu_name, const std::string& item_name,
                     const std::string& text, const std::string& shortcut = "",
                     MenuCallback callback = nullptr);

    /**
     * @brief Add a submenu
     * @param parent_name Parent menu name
     * @param submenu_name Submenu name for later reference
     * @param text Submenu text
     * @return Submenu handle for adding items
     */
    HMENU AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
                     const std::string& text);

    /**
     * @brief Add a menu separator
     * @param menu_name Menu name to add separator to
     */
    void AddMenuSeparator(const std::string& menu_name);

    /**
     * @brief Enable or disable a menu item
     * @param item_name Menu item name
     * @param enabled Whether to enable the item
     */
    void SetMenuItemEnabled(const std::string& item_name, bool enabled);

    /**
     * @brief Check or uncheck a menu item
     * @param item_name Menu item name
     * @param checked Whether to check the item
     */
    void SetMenuItemChecked(const std::string& item_name, bool checked);

    /**
     * @brief Get menu item checked state
     * @param item_name Menu item name
     * @return Whether the item is checked
     */
    bool GetMenuItemChecked(const std::string& item_name) const;

    // File dialog functionality

    /**
     * @brief Show modal file open dialog
     * @param title Dialog title
     * @param filter File filter string (e.g., "Text Files\0*.txt\0All Files\0*.*\0")
     * @param default_ext Default file extension (e.g., "txt")
     * @param initial_dir Initial directory (empty for current directory)
     * @return Selected file path, empty string if cancelled
     */
    std::string ShowOpenFileDialog(const std::string& title = "Open File",
                                   const std::string& filter = "All Files\0*.*\0",
                                   const std::string& default_ext = "",
                                   const std::string& initial_dir = "");

    /**
     * @brief Show modal file save dialog
     * @param title Dialog title
     * @param filter File filter string (e.g., "Text Files\0*.txt\0All Files\0*.*\0")
     * @param default_ext Default file extension (e.g., "txt")
     * @param initial_dir Initial directory (empty for current directory)
     * @param default_name Default filename
     * @return Selected file path, empty string if cancelled
     */
    std::string ShowSaveFileDialog(const std::string& title = "Save File",
                                   const std::string& filter = "All Files\0*.*\0",
                                   const std::string& default_ext = "",
                                   const std::string& initial_dir = "",
                                   const std::string& default_name = "");

    /**
     * @brief Show modal file open dialog with multiple file selection
     * @param title Dialog title
     * @param filter File filter string (e.g., "Text Files\0*.txt\0All Files\0*.*\0")
     * @param initial_dir Initial directory (empty for current directory)
     * @return Vector of selected file paths, empty if cancelled
     */
    std::vector<std::string> ShowOpenMultipleFilesDialog(const std::string& title = "Open Files",
                                                          const std::string& filter = "All Files\0*.*\0",
                                                          const std::string& initial_dir = "");

protected:
    /**
     * @brief Virtual message handler
     * @param msg Message ID
     * @param wparam WPARAM parameter
     * @param lparam LPARAM parameter
     * @return Message result
     */
    virtual LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    /**
     * @brief Get window class style flags
     * @return Window class style flags
     */
    virtual UINT GetWindowClassStyle() const { return CS_HREDRAW | CS_VREDRAW; }

    /**
     * @brief Post-creation initialization hook
     * @return True if successful
     */
    virtual bool PostCreateInitialize() { return true; }

    /**
     * @brief Register window class
     * @return True if successful
     */
    bool RegisterWindowClass();

    /**
     * @brief Get the base window style for window creation
     * @return Base window style flags
     */
    virtual DWORD GetBaseWindowStyle() const;

    HWND m_hwnd;                        ///< Window handle
    HINSTANCE m_hinstance;              ///< Application instance
    std::string m_class_name;           ///< Window class name
    std::string m_title;                ///< Window title
    bool m_class_registered;            ///< Whether window class is registered
    bool m_user_resizable;              ///< Whether user can resize the window

private:
    struct MenuItemInfo {
        UINT id;
        std::string name;
        HMENU parent_menu;
        MenuCallback callback;
        UINT accelerator_key;
        UINT accelerator_modifiers;
    };

    // Menu-related members
    HMENU m_menu_bar;
    std::map<std::string, HMENU> m_menus;
    std::map<std::string, MenuItemInfo> m_menu_items;
    std::map<UINT, std::string> m_id_to_name;
    HACCEL m_accelerators;
    std::vector<ACCEL> m_accel_table;
    UINT m_next_menu_id;

    // Menu helper methods
    UINT ParseShortcut(const std::string& shortcut, UINT& key, UINT& modifiers);
    void UpdateAcceleratorTable();
    std::string FormatMenuText(const std::string& text, const std::string& shortcut);

    // File dialog helper methods
    std::string PrepareFilterString(const std::string& filter);
    std::vector<std::string> ParseMultipleFileNames(const std::string& buffer);

    // Window style helper methods
    void UpdateWindowStyle();

    /**
     * @brief Static window procedure
     */
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};