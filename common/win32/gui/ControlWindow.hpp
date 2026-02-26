// ControlWindow.hpp
#pragma once

#include "Window.hpp"
#include <commctrl.h>
#include <vector>

/**
 * @brief Window class with support for various Win32 controls
 * 
 * Provides name-based API for managing buttons, edit controls, combo boxes,
 * checkboxes, list boxes, and list views with callback support.
 */
class ControlWindow : public Window {
public:
    /**
     * @brief Button callback function type
     */
    using ButtonCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param class_name The window class name
     * @param title The window title
     */
    ControlWindow(const std::string& class_name, const std::string& title);

    // Button methods
    /**
     * @brief Add a button control
     * @param name Button name for later reference
     * @param text Button text
     * @param x X position
     * @param y Y position
     * @param width Button width
     * @param height Button height
     * @param callback Callback function when clicked
     */
    void AddButton(const std::string& name, const std::string& text, 
                   int x, int y, int width, int height, ButtonCallback callback);

    // Edit control methods
    /**
     * @brief Add an edit control
     * @param name Edit name for later reference
     * @param text Initial text
     * @param x X position
     * @param y Y position
     * @param width Edit width
     * @param height Edit height
     * @param multiline Whether to support multiple lines
     */
    void AddEdit(const std::string& name, const std::string& text,
                 int x, int y, int width, int height, bool multiline = false);

    /**
     * @brief Set edit control text
     * @param name Edit name
     * @param text New text
     */
    void SetEditText(const std::string& name, const std::string& text);

    /**
     * @brief Get edit control text
     * @param name Edit name
     * @return Current text
     */
    std::string GetEditText(const std::string& name) const;

    // Combo box methods
    /**
     * @brief Add a combo box control
     * @param name Combo name for later reference
     * @param x X position
     * @param y Y position
     * @param width Combo width
     * @param height Combo height
     */
    void AddComboBox(const std::string& name, int x, int y, int width, int height);

    /**
     * @brief Add item to combo box
     * @param name Combo name
     * @param item Item text
     */
    void AddComboItem(const std::string& name, const std::string& item);

    /**
     * @brief Get selected combo box text
     * @param name Combo name
     * @return Selected text
     */
    std::string GetComboSelectedText(const std::string& name) const;

    /**
     * @brief Select combo box item by text
     * @param name Combo name
     * @param text Text to select
     */
    void SelectComboByText(const std::string& name, const std::string& text);

    /**
     * @brief Select combo box item by index
     * @param name Combo name
     * @param index Index to select
     */
    void SelectComboByIndex(const std::string& name, int index);

    // Checkbox methods
    /**
     * @brief Add a checkbox control
     * @param name Checkbox name for later reference
     * @param text Checkbox text
     * @param x X position
     * @param y Y position
     * @param width Checkbox width
     * @param height Checkbox height
     */
    void AddCheckbox(const std::string& name, const std::string& text,
                     int x, int y, int width, int height);

    /**
     * @brief Set checkbox checked state
     * @param name Checkbox name
     * @param checked Whether checked
     */
    void SetCheckboxChecked(const std::string& name, bool checked);

    /**
     * @brief Get checkbox checked state
     * @param name Checkbox name
     * @return Whether checked
     */
    bool GetCheckboxChecked(const std::string& name) const;

    // List box methods
    /**
     * @brief Add a list box control
     * @param name List box name for later reference
     * @param x X position
     * @param y Y position
     * @param width List box width
     * @param height List box height
     */
    void AddListBox(const std::string& name, int x, int y, int width, int height);

    /**
     * @brief Add item to list box
     * @param name List box name
     * @param item Item text
     */
    void AddListBoxItem(const std::string& name, const std::string& item);

    /**
     * @brief Remove item from list box
     * @param name List box name
     * @param index Item index to remove
     */
    void RemoveListBoxItem(const std::string& name, int index);

    /**
     * @brief Clear all list box items
     * @param name List box name
     */
    void ClearListBox(const std::string& name);

    /**
     * @brief Get selected list box text
     * @param name List box name
     * @return Selected text
     */
    std::string GetListBoxSelectedText(const std::string& name) const;

    /**
     * @brief Select list box item by text
     * @param name List box name
     * @param text Text to select
     */
    void SelectListBoxByText(const std::string& name, const std::string& text);

    /**
     * @brief Select list box item by index
     * @param name List box name
     * @param index Index to select
     */
    void SelectListBoxByIndex(const std::string& name, int index);

    // List view methods
    /**
     * @brief Add a list view control
     * @param name List view name for later reference
     * @param x X position
     * @param y Y position
     * @param width List view width
     * @param height List view height
     */
    void AddListView(const std::string& name, int x, int y, int width, int height);

    /**
     * @brief Add column to list view
     * @param name List view name
     * @param column_text Column header text
     * @param width Column width
     */
    void AddListViewColumn(const std::string& name, const std::string& column_text, int width);

    /**
     * @brief Add row to list view
     * @param name List view name
     * @param items Vector of column texts for the row
     * @return Row index
     */
    int AddListViewRow(const std::string& name, const std::vector<std::string>& items);

    /**
     * @brief Remove row from list view
     * @param name List view name
     * @param row Row index to remove
     */
    void RemoveListViewRow(const std::string& name, int row);

    /**
     * @brief Set text in list view cell
     * @param name List view name
     * @param row Row index
     * @param column Column index
     * @param text New text
     */
    void SetListViewCellText(const std::string& name, int row, int column, const std::string& text);

    /**
     * @brief Get text from list view cell
     * @param name List view name
     * @param row Row index
     * @param column Column index
     * @return Cell text
     */
    std::string GetListViewCellText(const std::string& name, int row, int column) const;

    /**
     * @brief Select list view row
     * @param name List view name
     * @param row Row index to select
     */
    void SelectListViewRow(const std::string& name, int row);

    /**
     * @brief Get selected list view row texts
     * @param name List view name
     * @return Vector of selected row texts (first column)
     */
    std::vector<std::string> GetListViewSelectedTexts(const std::string& name) const;

    HWND FindControl(const std::string& name) const;

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
    struct ControlInfo {
        HWND handle;
        std::string name;
        ButtonCallback callback;
    };

    std::unordered_map<std::string, ControlInfo> m_controls;
    std::unordered_map<HWND, std::string> m_handle_to_name;
    int m_next_control_id;

    int GetControlId(const std::string& name);
};
