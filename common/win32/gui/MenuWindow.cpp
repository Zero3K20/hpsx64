// MenuWindow.cpp
#include "MenuWindow.hpp"
#include <sstream>

MenuWindow::MenuWindow(const std::string& class_name, const std::string& title)
    : ControlWindow(class_name, title)
    , m_menu_bar(nullptr)
    , m_accelerators(nullptr)
    , m_next_menu_id(2000) {
}

MenuWindow::~MenuWindow() {
    if (m_accelerators) {
        DestroyAcceleratorTable(m_accelerators);
    }
    if (m_menu_bar) {
        DestroyMenu(m_menu_bar);
    }
}

void MenuWindow::CreateMenuBar() {
    if (!m_menu_bar) {
        m_menu_bar = CreateMenu();
        if (m_hwnd && m_menu_bar) {
            SetMenu(m_hwnd, m_menu_bar);
        }
    }
}

HMENU MenuWindow::AddMenu(const std::string& name, const std::string& text) {
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

void MenuWindow::AddMenuItem(const std::string& menu_name, const std::string& item_name,
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

HMENU MenuWindow::AddSubmenu(const std::string& parent_name, const std::string& submenu_name,
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

void MenuWindow::AddMenuSeparator(const std::string& menu_name) {
    auto menu_it = m_menus.find(menu_name);
    if (menu_it != m_menus.end()) {
        AppendMenuA(menu_it->second, MF_SEPARATOR, 0, nullptr);
    }
}

void MenuWindow::SetMenuItemEnabled(const std::string& item_name, bool enabled) {
    auto item_it = m_menu_items.find(item_name);
    if (item_it != m_menu_items.end()) {
        UINT flags = enabled ? MF_ENABLED : MF_GRAYED;
        EnableMenuItem(item_it->second.parent_menu, item_it->second.id, MF_BYCOMMAND | flags);
    }
}

void MenuWindow::SetMenuItemChecked(const std::string& item_name, bool checked) {
    auto item_it = m_menu_items.find(item_name);
    if (item_it != m_menu_items.end()) {
        UINT flags = checked ? MF_CHECKED : MF_UNCHECKED;
        CheckMenuItem(item_it->second.parent_menu, item_it->second.id, MF_BYCOMMAND | flags);
    }
}

bool MenuWindow::GetMenuItemChecked(const std::string& item_name) const {
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

LRESULT MenuWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
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
        default:
            return ControlWindow::HandleMessage(msg, wparam, lparam);
    }
    return 0;
}

UINT MenuWindow::ParseShortcut(const std::string& shortcut, UINT& key, UINT& modifiers) {
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

void MenuWindow::UpdateAcceleratorTable() {
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

std::string MenuWindow::FormatMenuText(const std::string& text, const std::string& shortcut) {
    if (shortcut.empty()) {
        return text;
    }
    return text + "\t" + shortcut;
}
