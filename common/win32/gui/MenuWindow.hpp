// MenuWindow.hpp
#pragma once

#include "ControlWindow.hpp"
#include <map>

/**
 * @brief Window class with menu support
 * 
 * Extends ControlWindow to provide menu functionality including submenus,
 * enable/disable, check/uncheck, keyboard shortcuts, and callbacks by name.
 */
class MenuWindow : public ControlWindow {
public:
    /**
     * @brief Menu callback function type
     */
    using MenuCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param class_name The window class name
     * @param title The window title
     */
    MenuWindow(const std::string& class_name, const std::string& title);

    /**
     * @brief Destructor
     */
    virtual ~MenuWindow();

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

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
    struct MenuItemInfo {
        UINT id;
        std::string name;
        HMENU parent_menu;
        MenuCallback callback;
        UINT accelerator_key;
        UINT accelerator_modifiers;
    };

    HMENU m_menu_bar;
    std::map<std::string, HMENU> m_menus;
    std::map<std::string, MenuItemInfo> m_menu_items;
    std::map<UINT, std::string> m_id_to_name;
    HACCEL m_accelerators;
    std::vector<ACCEL> m_accel_table;
    UINT m_next_menu_id;

    UINT ParseShortcut(const std::string& shortcut, UINT& key, UINT& modifiers);
    void UpdateAcceleratorTable();
    std::string FormatMenuText(const std::string& text, const std::string& shortcut);
};
