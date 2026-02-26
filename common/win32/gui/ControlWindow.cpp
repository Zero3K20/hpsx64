// ControlWindow.cpp
#include "ControlWindow.hpp"
#include <windowsx.h>
#include <tchar.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

ControlWindow::ControlWindow(const std::string& class_name, const std::string& title)
    : Window(class_name, title)
    , m_next_control_id(1000) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
}

void ControlWindow::AddButton(const std::string& name, const std::string& text,
                              int x, int y, int width, int height, ButtonCallback callback) {
    HWND hwnd_button = CreateWindowA(
        "BUTTON",
        text.c_str(),
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_button) {
        ControlInfo info;
        info.handle = hwnd_button;
        info.name = name;
        info.callback = callback;
        m_controls[name] = info;
        m_handle_to_name[hwnd_button] = name;
    }
}

void ControlWindow::AddEdit(const std::string& name, const std::string& text,
                            int x, int y, int width, int height, bool multiline) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (multiline) {
        style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
    }

    HWND hwnd_edit = CreateWindowA(
        "EDIT",
        text.c_str(),
        style,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_edit) {
        ControlInfo info;
        info.handle = hwnd_edit;
        info.name = name;
        m_controls[name] = info;
        m_handle_to_name[hwnd_edit] = name;
    }
}

void ControlWindow::SetEditText(const std::string& name, const std::string& text) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        SetWindowTextA(hwnd, text.c_str());
    }
}

std::string ControlWindow::GetEditText(const std::string& name) const {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        int length = GetWindowTextLengthA(hwnd);
        if (length > 0) {
            std::string text(length, '\0');
            GetWindowTextA(hwnd, &text[0], length + 1);
            return text;
        }
    }
    return "";
}

void ControlWindow::AddComboBox(const std::string& name, int x, int y, int width, int height) {
    HWND hwnd_combo = CreateWindowA(
        "COMBOBOX",
        "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_combo) {
        ControlInfo info;
        info.handle = hwnd_combo;
        info.name = name;
        m_controls[name] = info;
        m_handle_to_name[hwnd_combo] = name;
    }
}

void ControlWindow::AddComboItem(const std::string& name, const std::string& item) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ComboBox_AddString(hwnd, item.c_str());
    }
}

std::string ControlWindow::GetComboSelectedText(const std::string& name) const {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        int index = ComboBox_GetCurSel(hwnd);
        if (index != CB_ERR) {
            int length = ComboBox_GetLBTextLen(hwnd, index);
            if (length > 0) {
                std::string text(length, '\0');
                ComboBox_GetLBText(hwnd, index, &text[0]);
                return text;
            }
        }
    }
    return "";
}

void ControlWindow::SelectComboByText(const std::string& name, const std::string& text) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ComboBox_SelectString(hwnd, -1, text.c_str());
    }
}

void ControlWindow::SelectComboByIndex(const std::string& name, int index) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ComboBox_SetCurSel(hwnd, index);
    }
}

void ControlWindow::AddCheckbox(const std::string& name, const std::string& text,
                                int x, int y, int width, int height) {
    HWND hwnd_checkbox = CreateWindowA(
        "BUTTON",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_checkbox) {
        ControlInfo info;
        info.handle = hwnd_checkbox;
        info.name = name;
        m_controls[name] = info;
        m_handle_to_name[hwnd_checkbox] = name;
    }
}

void ControlWindow::SetCheckboxChecked(const std::string& name, bool checked) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        Button_SetCheck(hwnd, checked ? BST_CHECKED : BST_UNCHECKED);
    }
}

bool ControlWindow::GetCheckboxChecked(const std::string& name) const {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        return Button_GetCheck(hwnd) == BST_CHECKED;
    }
    return false;
}

void ControlWindow::AddListBox(const std::string& name, int x, int y, int width, int height) {
    HWND hwnd_listbox = CreateWindowA(
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_listbox) {
        ControlInfo info;
        info.handle = hwnd_listbox;
        info.name = name;
        m_controls[name] = info;
        m_handle_to_name[hwnd_listbox] = name;
    }
}

void ControlWindow::AddListBoxItem(const std::string& name, const std::string& item) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListBox_AddString(hwnd, item.c_str());
    }
}

void ControlWindow::RemoveListBoxItem(const std::string& name, int index) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListBox_DeleteString(hwnd, index);
    }
}

void ControlWindow::ClearListBox(const std::string& name) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListBox_ResetContent(hwnd);
    }
}

std::string ControlWindow::GetListBoxSelectedText(const std::string& name) const {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        int index = ListBox_GetCurSel(hwnd);
        if (index != LB_ERR) {
            int length = ListBox_GetTextLen(hwnd, index);
            if (length > 0) {
                std::string text(length, '\0');
                ListBox_GetText(hwnd, index, &text[0]);
                return text;
            }
        }
    }
    return "";
}

void ControlWindow::SelectListBoxByText(const std::string& name, const std::string& text) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListBox_SelectString(hwnd, -1, text.c_str());
    }
}

void ControlWindow::SelectListBoxByIndex(const std::string& name, int index) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListBox_SetCurSel(hwnd, index);
    }
}

void ControlWindow::AddListView(const std::string& name, int x, int y, int width, int height) {
    HWND hwnd_listview = CreateWindowA(
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        x, y, width, height,
        m_hwnd,
        reinterpret_cast<HMENU>(GetControlId(name)),
        m_hinstance,
        nullptr
    );

    if (hwnd_listview) {
        ListView_SetExtendedListViewStyle(hwnd_listview, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        ControlInfo info;
        info.handle = hwnd_listview;
        info.name = name;
        m_controls[name] = info;
        m_handle_to_name[hwnd_listview] = name;
    }
}

void ControlWindow::AddListViewColumn(const std::string& name, const std::string& column_text, int width) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        
        // Convert std::string to TCHAR array for compatibility
        std::vector<TCHAR> text_buffer(column_text.begin(), column_text.end());
        text_buffer.push_back(0); // null terminator
        column.pszText = text_buffer.data();
        column.cx = width;
        
        int column_count = Header_GetItemCount(ListView_GetHeader(hwnd));
        ListView_InsertColumn(hwnd, column_count, &column);
    }
}

int ControlWindow::AddListViewRow(const std::string& name, const std::vector<std::string>& items) {
    HWND hwnd = FindControl(name);
    if (hwnd && !items.empty()) {
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = ListView_GetItemCount(hwnd);
        
        // Convert first item to TCHAR array
        std::vector<TCHAR> text_buffer(items[0].begin(), items[0].end());
        text_buffer.push_back(0); // null terminator
        item.pszText = text_buffer.data();
        
        int row = ListView_InsertItem(hwnd, &item);
        
        // Add subsequent columns
        for (size_t i = 1; i < items.size(); ++i) {
            std::vector<TCHAR> col_buffer(items[i].begin(), items[i].end());
            col_buffer.push_back(0);
            ListView_SetItemText(hwnd, row, static_cast<int>(i), col_buffer.data());
        }
        
        return row;
    }
    return -1;
}

void ControlWindow::RemoveListViewRow(const std::string& name, int row) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListView_DeleteItem(hwnd, row);
    }
}

void ControlWindow::SetListViewCellText(const std::string& name, int row, int column, const std::string& text) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        std::vector<TCHAR> text_buffer(text.begin(), text.end());
        text_buffer.push_back(0);
        ListView_SetItemText(hwnd, row, column, text_buffer.data());
    }
}

std::string ControlWindow::GetListViewCellText(const std::string& name, int row, int column) const {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        TCHAR buffer[256] = {};
        ListView_GetItemText(hwnd, row, column, buffer, sizeof(buffer) / sizeof(TCHAR));
        
        // Convert TCHAR array back to std::string
        std::string result;
        for (int i = 0; buffer[i] != 0 && i < 256; ++i) {
            result += static_cast<char>(buffer[i]);
        }
        return result;
    }
    return "";
}

void ControlWindow::SelectListViewRow(const std::string& name, int row) {
    HWND hwnd = FindControl(name);
    if (hwnd) {
        ListView_SetItemState(hwnd, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

std::vector<std::string> ControlWindow::GetListViewSelectedTexts(const std::string& name) const {
    std::vector<std::string> selected_texts;
    HWND hwnd = FindControl(name);
    if (hwnd) {
        int item = -1;
        while ((item = ListView_GetNextItem(hwnd, item, LVNI_SELECTED)) != -1) {
            TCHAR buffer[256] = {};
            ListView_GetItemText(hwnd, item, 0, buffer, sizeof(buffer) / sizeof(TCHAR));
            
            // Convert TCHAR array to std::string
            std::string text;
            for (int i = 0; buffer[i] != 0 && i < 256; ++i) {
                text += static_cast<char>(buffer[i]);
            }
            selected_texts.push_back(text);
        }
    }
    return selected_texts;
}

LRESULT ControlWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_COMMAND:
            {
                HWND hwnd_control = reinterpret_cast<HWND>(lparam);
                auto it = m_handle_to_name.find(hwnd_control);
                if (it != m_handle_to_name.end()) {
                    auto control_it = m_controls.find(it->second);
                    if (control_it != m_controls.end() && control_it->second.callback) {
                        control_it->second.callback();
                    }
                }
            }
            break;
        default:
            return Window::HandleMessage(msg, wparam, lparam);
    }
    return 0;
}

HWND ControlWindow::FindControl(const std::string& name) const {
    auto it = m_controls.find(name);
    if (it != m_controls.end()) {
        return it->second.handle;
    }
    return nullptr;
}

int ControlWindow::GetControlId(const std::string& name) {
    auto it = m_controls.find(name);
    if (it != m_controls.end()) {
        return static_cast<int>(reinterpret_cast<intptr_t>(GetMenu(it->second.handle)));
    }
    return m_next_control_id++;
}