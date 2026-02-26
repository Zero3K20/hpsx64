// Example usage of GamepadConfigWindow
#include "GamepadConfigWindow.hpp"
#include <iostream>
#include <fstream>

class MyApplication {
private:
    GamepadConfigWindow::GamepadConfig m_gamepadConfig;
    GamepadHandler m_gamepadHandler; // Our main gamepad handler
    
public:
    bool Initialize(HWND hwnd) {
        // Initialize the main gamepad handler
        if (!m_gamepadHandler.Initialize(hwnd)) {
            std::cout << "Failed to initialize gamepad handler" << std::endl;
            return false;
        }
        
        // Load configuration
        LoadGamepadConfig();
        return true;
    }
    
    void LoadGamepadConfig() {
        // Load from file or use defaults
        LoadGamepadConfigFromFile();
    }
    
    void ShowGamepadConfigDialog(int deviceIndex = 0) {
        // Define custom device names to avoid encoding issues
        std::vector<std::string> deviceNames = {
            "Player 1 Controller",
            "Player 2 Controller", 
            "Player 3 Controller",
            "Player 4 Controller"
        };
        
        // Create configuration window using our existing GamepadHandler
        GamepadConfigWindow configWindow(
            "Configure Gamepad",
            m_gamepadConfig,
            [this](const GamepadConfigWindow::GamepadConfig& config, bool accepted) {
                std::cout << "Callback received: accepted = " << (accepted ? "TRUE" : "FALSE") << std::endl;
                if (accepted) {
                    std::cout << "Configuration was ACCEPTED" << std::endl;
                    OnGamepadConfigChanged(config);
                } else {
                    std::cout << "Configuration was CANCELLED" << std::endl;
                }
            },
            m_gamepadHandler, // Pass our existing gamepad handler
            deviceIndex,      // Set which device to configure
            deviceNames       // Custom device names
        );
        
        // Show the dialog modally
        bool result = configWindow.ShowModal();
        
        std::cout << "Modal dialog result: " << (result ? "true" : "false") << std::endl;
    }
    
private:
    void CreateDefaultConfig() {
        // Set up default button mappings (restored L2/R2)
        m_gamepadConfig.buttonMappings["Circle"] = 0;
        m_gamepadConfig.buttonMappings["X"] = 1;
        m_gamepadConfig.buttonMappings["Triangle"] = 2;
        m_gamepadConfig.buttonMappings["Square"] = 3;
        m_gamepadConfig.buttonMappings["L1"] = 4;  // Left shoulder
        m_gamepadConfig.buttonMappings["R1"] = 5;  // Right shoulder
        m_gamepadConfig.buttonMappings["Select"] = 6;
        m_gamepadConfig.buttonMappings["Start"] = 7;
        m_gamepadConfig.buttonMappings["L3"] = 8;  // Left stick click
        m_gamepadConfig.buttonMappings["R3"] = 9;  // Right stick click
        m_gamepadConfig.buttonMappings["L2"] = 10; // Left trigger (as button)
        m_gamepadConfig.buttonMappings["R2"] = 11; // Right trigger (as button)
        
        // Set up axis mappings
        m_gamepadConfig.leftStickXAxis = 0;
        m_gamepadConfig.leftStickYAxis = 1;
        m_gamepadConfig.rightStickXAxis = 3;
        m_gamepadConfig.rightStickYAxis = 4;
        m_gamepadConfig.leftTriggerAxis = 2;
        m_gamepadConfig.rightTriggerAxis = 5;
        
        // Default to device 0
        m_gamepadConfig.deviceIndex = 0;
        m_gamepadConfig.deviceName = "";
    }
    
    void OnGamepadConfigChanged(const GamepadConfigWindow::GamepadConfig& newConfig) {
        m_gamepadConfig = newConfig;
        
        // Save the new configuration
        SaveGamepadConfigToFile();
        
        // Apply to your game's gamepad handler
        ApplyConfigurationToGame();
        
        // Output the complete configuration to console
        std::cout << "=== GAMEPAD CONFIGURATION UPDATED ===" << std::endl;
        std::cout << "Device Index: " << m_gamepadConfig.deviceIndex << std::endl;
        std::cout << "Device Name: " << m_gamepadConfig.deviceName << std::endl;
        std::cout << std::endl;
        
        std::cout << "Button Mappings:" << std::endl;
        for (const auto& mapping : m_gamepadConfig.buttonMappings) {
            std::cout << "  " << mapping.first << " = " << mapping.second << std::endl;
        }
        std::cout << std::endl;
        
        std::cout << "Axis Mappings:" << std::endl;
        std::cout << "  Left Stick X = " << m_gamepadConfig.leftStickXAxis << std::endl;
        std::cout << "  Left Stick Y = " << m_gamepadConfig.leftStickYAxis << std::endl;
        std::cout << "  Right Stick X = " << m_gamepadConfig.rightStickXAxis << std::endl;
        std::cout << "  Right Stick Y = " << m_gamepadConfig.rightStickYAxis << std::endl;
        std::cout << "  Left Trigger = " << m_gamepadConfig.leftTriggerAxis << std::endl;
        std::cout << "  Right Trigger = " << m_gamepadConfig.rightTriggerAxis << std::endl;
        std::cout << "=======================================" << std::endl;
    }
    
    void SaveGamepadConfigToFile() {
        std::ofstream file("gamepad_config.txt");
        if (file.is_open()) {
            // Save device info
            file << "device_index=" << m_gamepadConfig.deviceIndex << std::endl;
            file << "device_name=" << m_gamepadConfig.deviceName << std::endl;
            
            // Save button mappings
            for (const auto& mapping : m_gamepadConfig.buttonMappings) {
                file << "button_" << mapping.first << "=" << mapping.second << std::endl;
            }
            
            // Save axis mappings
            file << "left_stick_x_axis=" << m_gamepadConfig.leftStickXAxis << std::endl;
            file << "left_stick_y_axis=" << m_gamepadConfig.leftStickYAxis << std::endl;
            file << "right_stick_x_axis=" << m_gamepadConfig.rightStickXAxis << std::endl;
            file << "right_stick_y_axis=" << m_gamepadConfig.rightStickYAxis << std::endl;
            file << "left_trigger_axis=" << m_gamepadConfig.leftTriggerAxis << std::endl;
            file << "right_trigger_axis=" << m_gamepadConfig.rightTriggerAxis << std::endl;
            
            file.close();
        }
    }
    
    void LoadGamepadConfigFromFile() {
        std::ifstream file("gamepad_config.txt");
        if (!file.is_open()) {
            CreateDefaultConfig(); // Use defaults if file doesn't exist
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) continue;
            
            std::string key = line.substr(0, equalPos);
            std::string valueStr = line.substr(equalPos + 1);
            
            try {
                if (key == "device_index") {
                    m_gamepadConfig.deviceIndex = std::stoi(valueStr);
                } else if (key == "device_name") {
                    m_gamepadConfig.deviceName = valueStr;
                } else if (key.find("button_") == 0) {
                    std::string buttonName = key.substr(7);
                    int buttonIndex = std::stoi(valueStr);
                    m_gamepadConfig.buttonMappings[buttonName] = buttonIndex;
                } else if (key == "left_stick_x_axis") {
                    m_gamepadConfig.leftStickXAxis = std::stoi(valueStr);
                } else if (key == "left_stick_y_axis") {
                    m_gamepadConfig.leftStickYAxis = std::stoi(valueStr);
                } else if (key == "right_stick_x_axis") {
                    m_gamepadConfig.rightStickXAxis = std::stoi(valueStr);
                } else if (key == "right_stick_y_axis") {
                    m_gamepadConfig.rightStickYAxis = std::stoi(valueStr);
                } else if (key == "left_trigger_axis") {
                    m_gamepadConfig.leftTriggerAxis = std::stoi(valueStr);
                } else if (key == "right_trigger_axis") {
                    m_gamepadConfig.rightTriggerAxis = std::stoi(valueStr);
                }
            } catch (const std::exception& e) {
                std::cout << "Error parsing config line: " << line << " - " << e.what() << std::endl;
            }
        }
        
        file.close();
    }
    
    void ApplyConfigurationToGame() {
        // Apply the configuration to your actual GamepadHandler instance
        // This would be your main game's gamepad handler, not the config window's
        
        std::cout << "Applied configuration:" << std::endl;
        std::cout << "Device: " << m_gamepadConfig.deviceIndex << std::endl;
        std::cout << "Button mappings:" << std::endl;
        
        for (const auto& mapping : m_gamepadConfig.buttonMappings) {
            std::cout << "  " << mapping.first << " -> Button " << mapping.second << std::endl;
        }
        
        std::cout << "Axis mappings:" << std::endl;
        std::cout << "  Left Stick X -> Axis " << m_gamepadConfig.leftStickXAxis << std::endl;
        std::cout << "  Left Stick Y -> Axis " << m_gamepadConfig.leftStickYAxis << std::endl;
        std::cout << "  Right Stick X -> Axis " << m_gamepadConfig.rightStickXAxis << std::endl;
        std::cout << "  Right Stick Y -> Axis " << m_gamepadConfig.rightStickYAxis << std::endl;
        std::cout << "  Left Trigger -> Axis " << m_gamepadConfig.leftTriggerAxis << std::endl;
        std::cout << "  Right Trigger -> Axis " << m_gamepadConfig.rightTriggerAxis << std::endl;
    }
};

// Example main function showing how to use the configuration system
int main() {
    MyApplication app;
    
    // You would need to get a window handle from your actual window creation
    // For this example, we'll assume you have a window handle
    HWND hwnd = GetConsoleWindow(); // Or your actual window handle
    
    // IMPORTANT: Initialize the gamepad handler first!
    if (!app.Initialize(hwnd)) {
        std::cout << "Failed to initialize application" << std::endl;
        return -1;
    }
    
    // Load existing configuration
    app.LoadGamepadConfig();
    
    // Show configuration dialog (maybe from a menu option)
    app.ShowGamepadConfigDialog(0); // Configure device 0
    
    return 0;
}