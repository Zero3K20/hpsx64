// GamepadConfigWindow.hpp
#pragma once

#include "MenuWindow.hpp"
#include "GamepadHandler.hpp"
#include <map>
#include <functional>

/**
 * @brief Configuration window for gamepad input mapping
 * 
 * This class provides a user-friendly interface for configuring gamepad inputs.
 * Users can map buttons and analog axes by holding the desired input and double-clicking
 * the mapping they want to set.
 */
class GamepadConfigWindow : public MenuWindow {
public:
    /**
     * @brief Configuration data structure for gamepad settings
     */
    struct GamepadConfig {
        // Button mappings - maps logical button to physical button index
        std::map<std::string, int> buttonMappings;
        
        // Axis mappings for analog sticks and triggers
        int leftStickXAxis = 0;     ///< X-axis index for left stick
        int leftStickYAxis = 1;     ///< Y-axis index for left stick
        int rightStickXAxis = 2;    ///< X-axis index for right stick (RX)
        int rightStickYAxis = 5;    ///< Y-axis index for right stick (RY)
        int leftTriggerAxis = 3;    ///< Axis index for left trigger
        int rightTriggerAxis = 4;   ///< Axis index for right trigger
        
        // Selected device
        int deviceIndex = 0;        ///< Currently configured device index
        std::string deviceName = "";  ///< Name of the selected device
    };

    /**
     * @brief Callback function type for configuration completion
     * @param config The final configuration after user changes
     * @param accepted True if user accepted changes, false if cancelled
     */
    using ConfigCallback = std::function<void(const GamepadConfig& config, bool accepted)>;

    /**
     * @brief Constructor
     * @param title Window title
     * @param initialConfig Initial configuration to display
     * @param callback Function to call when configuration is complete
     * @param gamepadHandler Reference to already-initialized GamepadHandler
     * @param deviceIndex Initial device index to configure (default: 0)
     * @param deviceNames Optional custom device names (if empty, uses "Device X" format)
     */
    GamepadConfigWindow(const std::string& title, 
                       const GamepadConfig& initialConfig,
                       ConfigCallback callback,
                       GamepadHandler& gamepadHandler,
                       int deviceIndex = 0,
                       const std::vector<std::string>& deviceNames = {});

    /**
     * @brief Destructor
     */
    virtual ~GamepadConfigWindow();

    /**
     * @brief Shows the configuration window modally
     * @return True if configuration was accepted, false if cancelled
     */
    bool ShowModal();

    /**
     * @brief Gets the current configuration
     * @return Current gamepad configuration
     */
    const GamepadConfig& GetCurrentConfig() const { return m_currentConfig; }

protected:
    /**
     * @brief Handle window messages
     */
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) override;

    /**
     * @brief Post-creation initialization
     */
    bool PostCreateInitialize() override;

private:
    // Configuration data
    GamepadConfig m_initialConfig;      ///< Original configuration
    GamepadConfig m_currentConfig;      ///< Current working configuration
    ConfigCallback m_callback;          ///< Completion callback
    
    // Gamepad handler for testing
    GamepadHandler& m_gamepad;         ///< Reference to already-initialized GamepadHandler
    
    // Custom device names
    std::vector<std::string> m_customDeviceNames; ///< Custom device names provided by caller
    
    // Constants
    static constexpr float AXIS_MOVEMENT_THRESHOLD = 0.2f; ///< Minimum axis movement to detect for configuration
    
    // UI state for input detection
    DWORD m_lastUpdateTime;             ///< Last update time for input detection
    std::vector<int> m_lastPressedButtons; ///< Previously pressed buttons
    std::map<int, float> m_lastAxisValues; ///< Previous axis values
    bool m_dialogAccepted;              ///< Whether dialog was accepted (OK clicked)
    
    // Control names for easy reference
    static const std::string BUTTON_LIST;
    static const std::string AXIS_LIST;
    static const std::string STATUS_TEXT;
    static const std::string BTN_OK;
    static const std::string BTN_CANCEL;
    static const std::string BTN_RESET;

    /**
     * @brief Initialize the user interface
     */
    void InitializeUI();
    
    /**
     * @brief Create the button configuration section
     */
    void CreateButtonConfigSection();
    
    /**
     * @brief Create the analog configuration section
     */
    void CreateAnalogConfigSection();
    
    /**
     * @brief Update the device list
     */
    void UpdateDeviceList();
    
    /**
     * @brief Update the device display
     */
    void UpdateDeviceDisplay();
    
    /**
     * @brief Update the button mapping list
     */
    void UpdateButtonList();
    
    /**
     * @brief Update the axis configuration list
     */
    void UpdateAxisList();
    
    /**
     * @brief Handle device selection change
     */
    void OnDeviceChanged();
    
    /**
     * @brief Change device by offset
     */
    void ChangeDevice(int offset);
    
    /**
     * @brief Handle reset button click
     */
    void OnResetButton();
    
    /**
     * @brief Handle OK button click
     */
    void OnOKButton();
    
    /**
     * @brief Handle Cancel button click
     */
    void OnCancelButton();
    
    /**
     * @brief Handle double-click on button list
     */
    void OnButtonListDoubleClick();
    
    /**
     * @brief Handle double-click on axis list
     */
    void OnAxisListDoubleClick();
    
    /**
     * @brief Update input detection
     */
    void UpdateInputDetection();
    
    /**
     * @brief Check for held button input
     */
    int GetHeldButton();
    
    /**
     * @brief Check for active axis movement
     */
    int GetActiveAxis();
    
    /**
     * @brief Create default button mapping
     */
    void CreateDefaultMapping();
    
    /**
     * @brief Validate configuration
     */
    bool ValidateConfiguration();
    
    /**
     * @brief Update status text
     */
    void UpdateStatusText(const std::string& text);
    
    /**
     * @brief Get axis name from index
     */
    std::string GetAxisName(int axisIndex);
    
    /**
     * @brief Format floating point value
     */
    std::string FormatFloat(float value, int precision = 2) const;
    
    /**
     * @brief Get currently selected device index
     */
    int GetSelectedDevice() const;
    
    /**
     * @brief Make a control read-only
     */
    void MakeControlReadOnly(const std::string& controlName);
};