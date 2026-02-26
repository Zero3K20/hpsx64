// Window.cpp
#include "Window.hpp"
#include <cstdio>
#include <sstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "comdlg32.lib")

Window::Window(const std::string& class_name, const std::string& title)
    : m_hwnd(nullptr)
    , m_hinstance(GetModuleHandle(nullptr))
    , m_class_name(class_name)
    , m_title(title)
    , m_class_registered(false)
    , m_user_resizable(true)  // Default to allowing user resizing
    , m_menu_bar(nullptr)
    , m_accelerators(nullptr)
    , m_next_menu_id(2000) {
}

Window::~Window() {
    if (m_accelerators) {
        DestroyAcceleratorTable(m_accelerators);
    }
    if (m_menu_bar) {
        DestroyMenu(m_menu_bar);
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    if (m_class_registered) {
        UnregisterClassA(m_class_name.c_str(), m_hinstance);
    }
}

bool Window::Create(int width, int height, SizeMode mode) {
    if (m_hwnd) {
        // Window already created
        return true;
    }

    if (!RegisterWindowClass()) {
        OutputDebugStringA("Failed to register window class\n");
        return false;
    }

    DWORD style = GetBaseWindowStyle();
    DWORD ex_style = WS_EX_APPWINDOW;
    
    RECT rect = { 0, 0, width, height };
    if (mode == SizeMode::CLIENT_AREA) {
        if (!AdjustWindowRectEx(&rect, style, FALSE, ex_style)) {
            OutputDebugStringA("AdjustWindowRectEx failed\n");
            return false;
        }
    }

    int window_width = rect.right - rect.left;
    int window_height = rect.bottom - rect.top;

    // Ensure reasonable window size
    if (window_width <= 0) window_width = width;
    if (window_height <= 0) window_height = height;

    OutputDebugStringA("Creating window...\n");

    m_hwnd = CreateWindowExA(
        ex_style,               // Extended window style
        m_class_name.c_str(),   // Window class name
        m_title.c_str(),        // Window title
        style,                  // Window style
        CW_USEDEFAULT,         // X position
        CW_USEDEFAULT,         // Y position
        window_width,          // Width
        window_height,         // Height
        nullptr,               // Parent window
        nullptr,               // Menu
        m_hinstance,           // Instance handle
        this                   // Additional application data
    );

    if (!m_hwnd) {
        DWORD error = GetLastError();
        char error_msg[512];
        sprintf_s(error_msg, "CreateWindowEx failed with error %lu\n", error);
        OutputDebugStringA(error_msg);
        return false;
    }

    OutputDebugStringA("Window created successfully\n");

    // Call post-creation initialization hook
    if (!PostCreateInitialize()) {
        OutputDebugStringA("PostCreateInitialize failed\n");
        return false;
    }

    return true;
}

void Window::Show(int show_command) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show_command);
        UpdateWindow(m_hwnd);
    }
}

bool Window::ProcessMessages() {
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        
        // Handle accelerator keys if we have them
        if (m_accelerators && TranslateAcceleratorA(m_hwnd, m_accelerators, &msg)) {
            continue;
        }
        
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

bool Window::ProcessWindowMessages() {
    if (!m_hwnd) {
        return false;
    }

    MSG msg;
    while (PeekMessageA(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        
        // Handle accelerator keys if we have them
        if (m_accelerators && TranslateAcceleratorA(m_hwnd, m_accelerators, &msg)) {
            continue;
        }
        
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

bool Window::SetCaption(const std::string& caption) {
    if (!m_hwnd) {
        // Store the caption for later use when window is created
        m_title = caption;
        return true;
    }
    
    if (SetWindowTextA(m_hwnd, caption.c_str())) {
        m_title = caption;
        return true;
    }
    return false;
}

std::string Window::GetCaption() const {
    if (!m_hwnd) {
        return m_title;
    }
    
    int length = GetWindowTextLengthA(m_hwnd);
    if (length == 0) {
        return "";
    }
    
    std::string caption(length, '\0');
    int actual_length = GetWindowTextA(m_hwnd, &caption[0], length + 1);
    
    if (actual_length > 0) {
        caption.resize(actual_length);
        return caption;
    }
    
    return "";
}

bool Window::SetEnabled(bool enabled) {
    if (!m_hwnd) {
        return false;
    }
    
    return EnableWindow(m_hwnd, enabled ? TRUE : FALSE) != 0;
}

bool Window::IsEnabled() const {
    if (!m_hwnd) {
        return false;
    }
    
    return IsWindowEnabled(m_hwnd) != FALSE;
}

// Window sizing function implementations

bool Window::SetWindowSize(int width, int height) {
    if (!m_hwnd) {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(m_hwnd, &rect)) {
        return false;
    }

    return SetWindowPos(m_hwnd, nullptr, rect.left, rect.top, width, height,
                        SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool Window::SetClientSize(int width, int height) {
    if (!m_hwnd) {
        return false;
    }

    // Get current window style
    DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
    DWORD ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    BOOL has_menu = GetMenu(m_hwnd) != nullptr;

    // Calculate required window size for the desired client area
    RECT rect = { 0, 0, width, height };
    if (!AdjustWindowRectEx(&rect, style, has_menu, ex_style)) {
        return false;
    }

    int window_width = rect.right - rect.left;
    int window_height = rect.bottom - rect.top;

    // Get current window position
    RECT current_rect;
    if (!GetWindowRect(m_hwnd, &current_rect)) {
        return false;
    }

    return SetWindowPos(m_hwnd, nullptr, current_rect.left, current_rect.top,
                        window_width, window_height, SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool Window::GetWindowSize(int& width, int& height) const {
    if (!m_hwnd) {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(m_hwnd, &rect)) {
        return false;
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    return true;
}

bool Window::GetClientSize(int& width, int& height) const {
    if (!m_hwnd) {
        return false;
    }

    RECT rect;
    if (!GetClientRect(m_hwnd, &rect)) {
        return false;
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    return true;
}

bool Window::SetWindowPosition(int x, int y) {
    if (!m_hwnd) {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(m_hwnd, &rect)) {
        return false;
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    return SetWindowPos(m_hwnd, nullptr, x, y, width, height,
                        SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool Window::GetWindowPosition(int& x, int& y) const {
    if (!m_hwnd) {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(m_hwnd, &rect)) {
        return false;
    }

    x = rect.left;
    y = rect.top;
    return true;
}

bool Window::SetWindowRect(int x, int y, int width, int height, SizeMode mode) {
    if (!m_hwnd) {
        return false;
    }

    int final_width = width;
    int final_height = height;

    if (mode == SizeMode::CLIENT_AREA) {
        // Calculate required window size for the desired client area
        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        DWORD ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        BOOL has_menu = GetMenu(m_hwnd) != nullptr;

        RECT rect = { 0, 0, width, height };
        if (!AdjustWindowRectEx(&rect, style, has_menu, ex_style)) {
            return false;
        }

        final_width = rect.right - rect.left;
        final_height = rect.bottom - rect.top;
    }

    return SetWindowPos(m_hwnd, nullptr, x, y, final_width, final_height,
                        SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

// Window resizing control implementation

bool Window::SetUserResizable(bool resizable) {
    if (!m_hwnd) {
        // Store the setting for when the window is created
        m_user_resizable = resizable;
        return true;
    }

    m_user_resizable = resizable;
    UpdateWindowStyle();
    return true;
}

DWORD Window::GetBaseWindowStyle() const {
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    
    if (m_user_resizable) {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
    
    style |= WS_VISIBLE;
    return style;
}

void Window::UpdateWindowStyle() {
    if (!m_hwnd) {
        return;
    }

    DWORD current_style = GetWindowLong(m_hwnd, GWL_STYLE);
    DWORD new_style = current_style;

    if (m_user_resizable) {
        // Add resizing styles
        new_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    } else {
        // Remove resizing styles
        new_style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    if (new_style != current_style) {
        SetWindowLong(m_hwnd, GWL_STYLE, new_style);
        
        // Force window to redraw with new style
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

// File dialog functionality implementation

std::string Window::ShowOpenFileDialog(const std::string& title, const std::string& filter,
                                        const std::string& default_ext, const std::string& initial_dir) {
    char file_name[MAX_PATH] = {};
    
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name);
    
    std::string prepared_filter = PrepareFilterString(filter);
    ofn.lpstrFilter = prepared_filter.c_str();
    ofn.nFilterIndex = 1;
    
    if (!title.empty()) {
        ofn.lpstrTitle = title.c_str();
    }
    
    if (!default_ext.empty()) {
        ofn.lpstrDefExt = default_ext.c_str();
    }
    
    if (!initial_dir.empty()) {
        ofn.lpstrInitialDir = initial_dir.c_str();
    }
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(file_name);
    }
    
    return "";
}

std::string Window::ShowSaveFileDialog(const std::string& title, const std::string& filter,
                                        const std::string& default_ext, const std::string& initial_dir,
                                        const std::string& default_name) {
    char file_name[MAX_PATH] = {};
    
    // Set default filename if provided
    if (!default_name.empty() && default_name.length() < MAX_PATH) {
        strcpy_s(file_name, default_name.c_str());
    }
    
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name);
    
    std::string prepared_filter = PrepareFilterString(filter);
    ofn.lpstrFilter = prepared_filter.c_str();
    ofn.nFilterIndex = 1;
    
    if (!title.empty()) {
        ofn.lpstrTitle = title.c_str();
    }
    
    if (!default_ext.empty()) {
        ofn.lpstrDefExt = default_ext.c_str();
    }
    
    if (!initial_dir.empty()) {
        ofn.lpstrInitialDir = initial_dir.c_str();
    }
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER;
    
    if (GetSaveFileNameA(&ofn)) {
        return std::string(file_name);
    }
    
    return "";
}

std::vector<std::string> Window::ShowOpenMultipleFilesDialog(const std::string& title, const std::string& filter,
                                                              const std::string& initial_dir) {
    const int buffer_size = 32768; // Large buffer for multiple files
    char* file_buffer = new char[buffer_size];
    file_buffer[0] = '\0';
    
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = file_buffer;
    ofn.nMaxFile = buffer_size;
    
    std::string prepared_filter = PrepareFilterString(filter);
    ofn.lpstrFilter = prepared_filter.c_str();
    ofn.nFilterIndex = 1;
    
    if (!title.empty()) {
        ofn.lpstrTitle = title.c_str();
    }
    
    if (!initial_dir.empty()) {
        ofn.lpstrInitialDir = initial_dir.c_str();
    }
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
    
    std::vector<std::string> selected_files;
    
    if (GetOpenFileNameA(&ofn)) {
        selected_files = ParseMultipleFileNames(std::string(file_buffer, buffer_size));
    }
    
    delete[] file_buffer;
    return selected_files;
}

// File dialog helper methods

std::string Window::PrepareFilterString(const std::string& filter) {
    // The filter string needs to have null characters between entries
    // Input format: "Text Files\0*.txt\0All Files\0*.*\0"
    // We need to ensure it's properly null-terminated
    std::string result = filter;
    
    // If filter doesn't end with double null, add it
    if (result.length() < 2 || result.substr(result.length() - 2) != std::string("\0\0", 2)) {
        if (result.length() == 0 || result.back() != '\0') {
            result += '\0';
        }
        result += '\0';
    }
    
    return result;
}

std::vector<std::string> Window::ParseMultipleFileNames(const std::string& buffer) {
    std::vector<std::string> files;
    
    // Find the first null terminator
    size_t first_null = buffer.find('\0');
    if (first_null == std::string::npos) {
        // Single file selected
        if (!buffer.empty()) {
            files.push_back(buffer);
        }
        return files;
    }
    
    std::string directory = buffer.substr(0, first_null);
    
    // Check if there are more files after the directory
    size_t pos = first_null + 1;
    if (pos >= buffer.length() || buffer[pos] == '\0') {
        // Only one file selected
        files.push_back(directory);
        return files;
    }
    
    // Multiple files selected
    while (pos < buffer.length() && buffer[pos] != '\0') {
        size_t next_null = buffer.find('\0', pos);
        if (next_null == std::string::npos) {
            break;
        }
        
        std::string filename = buffer.substr(pos, next_null - pos);
        if (!filename.empty()) {
            std::string full_path = directory + "\\" + filename;
            files.push_back(full_path);
        }
        
        pos = next_null + 1;
    }
    
    return files;
}

// Menu functionality implementation

void Window::CreateMenuBar() {
    if (!m_menu_bar) {
        m_menu_bar = CreateMenu();
        if (m_hwnd && m_menu_bar) {
            SetMenu(m_hwnd, m_menu_bar);
        }
    }
}

HMENU Window::AddMenu(const std::string& name, const std::string& text) {
    if (!m_menu_bar) {
        CreateMenuBar();
    }

    HMENU menu = CreatePopupMenu();
    if (menu) {
        AppendMenuA(m_menu_bar, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(menu), text.c_str());
        m_menus[name] = menu;
        if (m_hwnd) {
            DrawMenuBar(m_hwnd);
        }
    }
    return menu;
}

void Window::AddMenuItem(const std::string& menu_name, const std::string& item_name,
                         const std::string& text, const std::string& shortcut,
                         MenuCallback callback) {
    auto menu_it = m_menus.find(menu_name);
    if (menu_it == m_menus.end()) {
        return;
    }

    UINT id = m_next_menu_id++;
    std::string formatted_text = FormatMenuText(text, shortcut);

    AppendMenuA(menu_it->second, MF_STRING, id, formatted_text.c_str());

    MenuItemInfo info;
    info.id = id;
    info.name = item_name;
    info.parent_menu = menu_it->second;
    info.callback = callback;
    info.accelerator_key = 0;
    info.accelerator_modifiers = 0;

    if (!shortcut.empty()) {
        ParseShortcut(shortcut, info.accelerator_key, info.accelerator_modifiers);
    }

    m_menu_items[item_name] = info;
    m_id_to_name[id] = item_name;

    UpdateAcceleratorTable();
}

HMENU Window::AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
                         const std::string& text) {
    auto parent_it = m_menus.find(parent_name);
    if (parent_it == m_menus.end()) {
        return nullptr;
    }

    HMENU submenu = CreatePopupMenu();
    if (submenu) {
        AppendMenuA(parent_it->second, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), text.c_str());
        m_menus[submenu_name] = submenu;
    }
    return submenu;
}

void Window::AddMenuSeparator(const std::string& menu_name) {
    auto menu_it = m_menus.find(menu_name);
    if (menu_it != m_menus.end()) {
        AppendMenuA(menu_it->second, MF_SEPARATOR, 0, nullptr);
    }
}

void Window::SetMenuItemEnabled(const std::string& item_name, bool enabled) {
    auto item_it = m_menu_items.find(item_name);
    if (item_it != m_menu_items.end()) {
        UINT flags = enabled ? MF_ENABLED : MF_GRAYED;
        EnableMenuItem(item_it->second.parent_menu, item_it->second.id, MF_BYCOMMAND | flags);
    }
}

void Window::SetMenuItemChecked(const std::string& item_name, bool checked) {
    auto item_it = m_menu_items.find(item_name);
    if (item_it != m_menu_items.end()) {
        UINT flags = checked ? MF_CHECKED : MF_UNCHECKED;
        CheckMenuItem(item_it->second.parent_menu, item_it->second.id, MF_BYCOMMAND | flags);
    }
}

bool Window::GetMenuItemChecked(const std::string& item_name) const {
    auto item_it = m_menu_items.find(item_name);
    if (item_it != m_menu_items.end()) {
        MENUITEMINFOA info = {};
        info.cbSize = sizeof(MENUITEMINFOA);
        info.fMask = MIIM_STATE;
        if (GetMenuItemInfoA(item_it->second.parent_menu, item_it->second.id, FALSE, &info)) {
            return (info.fState & MFS_CHECKED) != 0;
        }
    }
    return false;
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_COMMAND:
            {
                UINT id = LOWORD(wparam);
                auto id_it = m_id_to_name.find(id);
                if (id_it != m_id_to_name.end()) {
                    auto item_it = m_menu_items.find(id_it->second);
                    if (item_it != m_menu_items.end() && item_it->second.callback) {
                        item_it->second.callback();
                        return 0;
                    }
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            DestroyWindow(m_hwnd);
            return 0;
        default:
            return DefWindowProcA(m_hwnd, msg, wparam, lparam);
    }
    return 0;
}

bool Window::RegisterWindowClass() {
    if (m_class_registered) {
        return true;
    }

    // Check if class already exists
    WNDCLASSEXA existing_wc = {};
    if (GetClassInfoExA(m_hinstance, m_class_name.c_str(), &existing_wc)) {
        m_class_registered = true;
        return true;
    }

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = GetWindowClassStyle(); // Use virtual method
    wc.lpfnWndProc = StaticWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hinstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = m_class_name.c_str();
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    ATOM class_atom = RegisterClassExA(&wc);
    if (class_atom == 0) {
        DWORD error = GetLastError();
        char error_msg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, error_msg, sizeof(error_msg), nullptr);
        OutputDebugStringA("RegisterClassEx failed: ");
        OutputDebugStringA(error_msg);
        return false;
    }

    m_class_registered = true;
    return true;
}

// Menu helper methods implementation

UINT Window::ParseShortcut(const std::string& shortcut, UINT& key, UINT& modifiers) {
    key = 0;
    modifiers = 0;

    if (shortcut.empty()) {
        return 0;
    }

    std::istringstream iss(shortcut);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '+')) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return 0;
    }

    // Parse modifiers
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        const std::string& mod = tokens[i];
        if (mod == "Ctrl") {
            modifiers |= FCONTROL;
        } else if (mod == "Alt") {
            modifiers |= FALT;
        } else if (mod == "Shift") {
            modifiers |= FSHIFT;
        }
    }

    // Parse key
    const std::string& key_str = tokens.back();
    if (key_str.length() == 1) {
        char c = key_str[0];
        if (c >= 'A' && c <= 'Z') {
            key = c;
        } else if (c >= 'a' && c <= 'z') {
            key = c - 'a' + 'A';
        } else if (c >= '0' && c <= '9') {
            key = c;
        }
    } else {
        // Special keys
        if (key_str == "F1") key = VK_F1;
        else if (key_str == "F2") key = VK_F2;
        else if (key_str == "F3") key = VK_F3;
        else if (key_str == "F4") key = VK_F4;
        else if (key_str == "F5") key = VK_F5;
        else if (key_str == "F6") key = VK_F6;
        else if (key_str == "F7") key = VK_F7;
        else if (key_str == "F8") key = VK_F8;
        else if (key_str == "F9") key = VK_F9;
        else if (key_str == "F10") key = VK_F10;
        else if (key_str == "F11") key = VK_F11;
        else if (key_str == "F12") key = VK_F12;
        else if (key_str == "Enter") key = VK_RETURN;
        else if (key_str == "Space") key = VK_SPACE;
        else if (key_str == "Tab") key = VK_TAB;
        else if (key_str == "Esc") key = VK_ESCAPE;
        else if (key_str == "Delete") key = VK_DELETE;
        else if (key_str == "Insert") key = VK_INSERT;
        else if (key_str == "Home") key = VK_HOME;
        else if (key_str == "End") key = VK_END;
        else if (key_str == "PageUp") key = VK_PRIOR;
        else if (key_str == "PageDown") key = VK_NEXT;
    }

    return key;
}

void Window::UpdateAcceleratorTable() {
    if (m_accelerators) {
        DestroyAcceleratorTable(m_accelerators);
        m_accelerators = nullptr;
    }

    m_accel_table.clear();
    for (const auto& item : m_menu_items) {
        if (item.second.accelerator_key != 0) {
            ACCEL accel = {};
            accel.fVirt = FVIRTKEY | item.second.accelerator_modifiers;
            accel.key = item.second.accelerator_key;
            accel.cmd = item.second.id;
            m_accel_table.push_back(accel);
        }
    }

    if (!m_accel_table.empty()) {
        m_accelerators = CreateAcceleratorTableA(m_accel_table.data(), static_cast<int>(m_accel_table.size()));
    }
}

std::string Window::FormatMenuText(const std::string& text, const std::string& shortcut) {
    if (shortcut.empty()) {
        return text;
    }
    return text + "\t" + shortcut;
}

LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window* window = nullptr;

    if (msg == WM_NCCREATE) {
        // Get the Window object from the creation parameters
        CREATESTRUCTA* create_struct = reinterpret_cast<CREATESTRUCTA*>(lparam);
        window = static_cast<Window*>(create_struct->lpCreateParams);
        
        if (window) {
            // Store the Window object pointer in the window's user data
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            
            // Store the hwnd in the Window object
            window->m_hwnd = hwnd;
        }
    } else {
        // Retrieve the Window object from the window's user data
        window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(msg, wparam, lparam);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}