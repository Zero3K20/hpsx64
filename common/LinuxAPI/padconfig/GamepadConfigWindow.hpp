// GamepadConfigWindow.hpp - Linux version
#pragma once

#include "MenuWindow.hpp"
#include "../gamepad/GamepadHandler.hpp"
#include <map>
#include <functional>

/**
 * @brief Configuration window for gamepad input mapping (Linux stub)
 */
class GamepadConfigWindow : public MenuWindow {
public:
    struct GamepadConfig {
        std::map<std::string, int> buttonMappings;
        int leftStickXAxis = 0;
        int leftStickYAxis = 1;
        int rightStickXAxis = 2;
        int rightStickYAxis = 5;
        int leftTriggerAxis = 3;
        int rightTriggerAxis = 4;
        int deviceIndex = 0;
        std::string deviceName = "";
    };

    using ConfigCallback = std::function<void(const GamepadConfig& config, bool accepted)>;

    GamepadConfigWindow(const std::string& title,
                        const GamepadConfig& initialConfig,
                        ConfigCallback callback,
                        GamepadHandler& gamepadHandler)
        : MenuWindow("GamepadConfig", title)
        , m_config(initialConfig)
        , m_callback(callback)
        , m_gamepadHandler(gamepadHandler)
    {}

    virtual ~GamepadConfigWindow() {}

    const GamepadConfig& GetConfig() const { return m_config; }

private:
    GamepadConfig m_config;
    ConfigCallback m_callback;
    GamepadHandler& m_gamepadHandler;
};
