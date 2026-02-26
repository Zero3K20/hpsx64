// ControlWindow.hpp - Linux/SDL2 stub implementation
// Provides the same API as the Windows version (controls are stubs on Linux)
#pragma once

#include "Window.hpp"
#include <string>
#include <map>
#include <vector>
#include <functional>

/**
 * @brief Window class with support for various controls (Linux stub)
 */
class ControlWindow : public Window {
public:
    using ButtonCallback = std::function<void()>;

    ControlWindow(const std::string& class_name, const std::string& title)
        : Window(class_name, title) {}

    // Button stubs
    void AddButton(const std::string& /*name*/, const std::string& /*text*/,
                   int /*x*/, int /*y*/, int /*width*/, int /*height*/,
                   ButtonCallback /*callback*/) {}

    // Edit control stubs
    void AddEdit(const std::string& name, const std::string& text,
                 int /*x*/, int /*y*/, int /*width*/, int /*height*/, bool /*multiline*/ = false) {
        m_edit_values[name] = text;
    }
    void SetEditText(const std::string& name, const std::string& text) {
        m_edit_values[name] = text;
    }
    std::string GetEditText(const std::string& name) const {
        auto it = m_edit_values.find(name);
        return (it != m_edit_values.end()) ? it->second : "";
    }

    // Combo box stubs
    void AddComboBox(const std::string& name, int /*x*/, int /*y*/, int /*width*/, int /*height*/) {
        m_combo_values[name] = {};
    }
    void AddComboBoxItem(const std::string& name, const std::string& item) {
        m_combo_values[name].push_back(item);
    }
    void SetComboBoxSelection(const std::string& /*name*/, int /*index*/) {}
    int GetComboBoxSelection(const std::string& /*name*/) const { return 0; }
    std::string GetComboBoxText(const std::string& /*name*/) const { return ""; }

    // Checkbox stubs
    void AddCheckbox(const std::string& name, const std::string& /*text*/,
                     int /*x*/, int /*y*/, int /*width*/, int /*height*/, bool checked = false) {
        m_checkbox_values[name] = checked;
    }
    void SetCheckboxState(const std::string& name, bool checked) {
        m_checkbox_values[name] = checked;
    }
    bool GetCheckboxState(const std::string& name) const {
        auto it = m_checkbox_values.find(name);
        return (it != m_checkbox_values.end()) ? it->second : false;
    }

    // List box stubs
    void AddListBox(const std::string& /*name*/, int /*x*/, int /*y*/, int /*width*/, int /*height*/) {}
    void AddListBoxItem(const std::string& /*name*/, const std::string& /*item*/) {}
    void ClearListBox(const std::string& /*name*/) {}
    int GetListBoxSelection(const std::string& /*name*/) const { return -1; }

    // Label stubs
    void AddLabel(const std::string& /*name*/, const std::string& /*text*/,
                  int /*x*/, int /*y*/, int /*width*/, int /*height*/) {}
    void SetLabelText(const std::string& /*name*/, const std::string& /*text*/) {}

private:
    std::map<std::string, std::string> m_edit_values;
    std::map<std::string, std::vector<std::string>> m_combo_values;
    std::map<std::string, bool> m_checkbox_values;
};
