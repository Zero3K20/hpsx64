// GamepadConfigWindow.hpp - Linux version
#pragma once

#include "../gamepad/GamepadHandler.hpp"
#include <map>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief Configuration window for gamepad input mapping (Linux stub)
 */
class GamepadConfigWindow {
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

    GamepadConfigWindow(const char* title,
                        const GamepadConfig& initialConfig,
                        ConfigCallback callback,
                        GamepadHandler& gamepadHandler,
                        int deviceIndex = 0,
                        const std::vector<std::string>& deviceNames = {})
        : m_config(initialConfig)
        , m_callback(callback)
        , m_gamepadHandler(gamepadHandler)
        , m_title(title ? title : "")
        , m_deviceIndex(deviceIndex)
        , m_deviceNames(deviceNames)
    {}

    virtual ~GamepadConfigWindow() {}

    // Show the configuration dialog (stub: immediately calls callback with accepted=false)
    bool ShowModal() {
        if (m_callback) m_callback(m_config, false);
        return false;
    }

    const GamepadConfig& GetConfig() const { return m_config; }

    void SetEnabled(bool /*enabled*/) {}

private:
    GamepadConfig m_config;
    ConfigCallback m_callback;
    GamepadHandler& m_gamepadHandler;
    std::string m_title;
    int m_deviceIndex;
    std::vector<std::string> m_deviceNames;
};
