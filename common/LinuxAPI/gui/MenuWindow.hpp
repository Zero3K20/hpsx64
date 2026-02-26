// MenuWindow.hpp - Linux/SDL2 stub (extends ControlWindow)
#pragma once

#include "ControlWindow.hpp"

/**
 * @brief Window class with menu support (Linux version, inherits from ControlWindow)
 * Menu functionality is handled in the base Window class on Linux.
 */
class MenuWindow : public ControlWindow {
public:
    MenuWindow(const std::string& class_name, const std::string& title)
        : ControlWindow(class_name, title) {}
    virtual ~MenuWindow() {}
};
