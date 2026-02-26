// GamepadHandler.hpp

/**
 * @file GamepadHandler.hpp
 * @brief DirectInput gamepad handling class for Windows applications
 * @author Your Name
 * @date 2025
 * @version 1.0
 * 
 * This file contains the GamepadHandler class which provides comprehensive
 * DirectInput gamepad support including device management, input reading,
 * and configuration.
 */

#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <vector>
#include <functional>
#include <map>
#include <string>
#include <memory>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

/**
 * @class GamepadHandler
 * @brief Handles DirectInput gamepad devices with comprehensive input management
 * 
 * This class provides a complete solution for gamepad handling in Windows applications
 * using DirectInput 8. It supports multiple controllers, input remapping, and both 
 * digital and analog input reading with automatic reconnection handling.
 * 
 * Features:
 * - Multiple gamepad support
 * - Flexible input configuration and remapping
 * - Automatic device reconnection handling
 * - Digital button state checking (current, pressed, released)
 * - Analog input reading (sticks, triggers)
 * - Default Xbox controller mapping
 * 
 * @code
 * GamepadHandler gamepad;
 * gamepad.Initialize(hwnd);
 * 
 * // In game loop
 * gamepad.Update();
 * if (gamepad.IsPressed(0, GamepadHandler::GamepadInput::BUTTON_A)) {
 *     // Handle A button press
 * }
 * @endcode
 */
class GamepadHandler
{
public:
    /**
     * @enum GamepadInput
     * @brief Enumeration of all supported gamepad inputs
     * 
     * Defines standardized input identifiers that can be mapped to physical
     * gamepad buttons, axes, or POV directions through the configuration system.
     */
    enum class GamepadInput : int
    {
        // Digital Buttons
        BUTTON_A = 0,       ///< Face button A (Xbox: A, PlayStation: X)
        BUTTON_B = 1,       ///< Face button B (Xbox: B, PlayStation: Circle)
        BUTTON_X = 2,       ///< Face button X (Xbox: X, PlayStation: Square)
        BUTTON_Y = 3,       ///< Face button Y (Xbox: Y, PlayStation: Triangle)
        BUTTON_LB = 4,      ///< Left shoulder button
        BUTTON_RB = 5,      ///< Right shoulder button
        BUTTON_BACK = 6,    ///< Back/Select button
        BUTTON_START = 7,   ///< Start/Options button
        BUTTON_LS = 8,      ///< Left stick click
        BUTTON_RS = 9,      ///< Right stick click
        
        // D-Pad directions
        DPAD_UP = 10,       ///< D-Pad up direction
        DPAD_DOWN = 11,     ///< D-Pad down direction
        DPAD_LEFT = 12,     ///< D-Pad left direction
        DPAD_RIGHT = 13,    ///< D-Pad right direction
        
        // Left analog stick (digital threshold)
        LSTICK_UP = 14,     ///< Left stick up (as digital input)
        LSTICK_DOWN = 15,   ///< Left stick down (as digital input)
        LSTICK_LEFT = 16,   ///< Left stick left (as digital input)
        LSTICK_RIGHT = 17,  ///< Left stick right (as digital input)
        
        // Right analog stick (digital threshold)
        RSTICK_UP = 18,     ///< Right stick up (as digital input)
        RSTICK_DOWN = 19,   ///< Right stick down (as digital input)
        RSTICK_LEFT = 20,   ///< Right stick left (as digital input)
        RSTICK_RIGHT = 21,  ///< Right stick right (as digital input)
        
        // Triggers (digital threshold)
        LTRIGGER = 22,      ///< Left trigger (as digital input)
        RTRIGGER = 23,      ///< Right trigger (as digital input)
        
        COUNT = 24          ///< Total number of input types
    };
    
    /**
     * @struct InputConfig
     * @brief Configuration structure for mapping logical inputs to physical controls
     * 
     * This structure defines how a logical GamepadInput maps to actual hardware.
     * An input can be mapped to a digital button, POV hat direction, or analog
     * axis with threshold detection.
     */
    struct InputConfig
    {
        DWORD buttonMask = 0;           ///< Button index for digital inputs (0-127)
        DWORD povDirection = 0xFFFFFFFF; ///< POV direction in hundredths of degrees (0-35900, 0xFFFFFFFF = center)
        bool useAxis = false;           ///< True if this input uses an analog axis
        int axisIndex = -1;             ///< Axis index: 0=X, 1=Y, 2=Z, 3=RX, 4=RY, 5=RZ
        bool axisPositive = true;       ///< True for positive axis direction, false for negative
        float threshold = 0.5f;         ///< Analog to digital conversion threshold (0.0-1.0)
        bool usePOV = false;            ///< True if this input uses POV hat switch
    };

private:
    /**
     * @struct GamepadDevice
     * @brief Internal structure representing a connected gamepad device
     */
    struct GamepadDevice
    {
        LPDIRECTINPUTDEVICE8 device = nullptr;  ///< DirectInput device interface
        DIJOYSTATE2 currentState = {};          ///< Current input state
        DIJOYSTATE2 previousState = {};         ///< Previous frame input state
        DIDEVCAPS capabilities = {};            ///< Device capabilities
        std::string name;                       ///< Human-readable device name
        bool connected = true;                  ///< Connection status
        GUID guid = {};                         ///< Device GUID
    };

    static LPDIRECTINPUT8 m_pDirectInput;                        ///< DirectInput interface
    static std::vector<std::unique_ptr<GamepadDevice>> m_devices;          ///< Connected devices
    static std::map<GamepadInput, InputConfig> m_inputConfigs;             ///< Input configuration map
    static HWND m_hwnd;                                          ///< Window handle
    
    /**
     * @brief Sets up default Xbox controller configuration
     */
    void SetupDefaultConfiguration();
    
    /**
     * @brief DirectInput callback for enumerating joystick devices
     * @param pdidInstance Device instance information
     * @param pContext Pointer to GamepadHandler instance
     * @return DIENUM_CONTINUE to continue enumeration
     */
    static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);
    
    /**
     * @brief DirectInput callback for enumerating device objects (axes, buttons)
     * @param pdidoi Device object information
     * @param pContext Pointer to DirectInput device
     * @return DIENUM_CONTINUE to continue enumeration
     */
    static BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
    
    /**
     * @brief Initializes a single gamepad device
     * @param pdidInstance Device instance to initialize
     * @return True if initialization succeeded
     */
    bool InitializeDevice(const DIDEVICEINSTANCE* pdidInstance);
    
    /**
     * @brief Updates input states for all connected devices
     */
    void UpdateDeviceStates();
    
    /**
     * @brief Checks if an input condition is met based on configuration
     * @param device Device to check
     * @param config Input configuration
     * @return True if input condition is satisfied
     */
    bool CheckInputCondition(const GamepadDevice& device, const InputConfig& config) const;
    
    /**
     * @brief Gets normalized axis value from device
     * @param device Device to read from
     * @param axisIndex Axis index (0-5)
     * @return Normalized axis value (-1.0 to 1.0)
     */
    float GetAxisValue(const GamepadDevice& device, int axisIndex) const;
    
    /**
     * @brief Checks device connections and attempts reconnection
     */
    void CheckDeviceConnections();

public:
    /**
     * @brief Constructor - initializes with default configuration
     */
    GamepadHandler();
    
    /**
     * @brief Destructor - cleans up DirectInput resources
     */
    ~GamepadHandler();
    
    // Initialization
    
    /**
     * @brief Initializes DirectInput and enumerates devices
     * @param hwnd Window handle for cooperative level setting
     * @return True if initialization succeeded
     * 
     * This must be called before any other operations. The window handle is used
     * to set the DirectInput cooperative level and should remain valid for the
     * lifetime of this object.
     */
    bool Initialize(HWND hwnd);
    
    /**
     * @brief Shuts down DirectInput and releases all resources
     * 
     * This is called automatically by the destructor but can be called
     * explicitly to clean up resources earlier.
     */
    void Shutdown();
    
    // Device management
    
    /**
     * @brief Re-enumerates all gamepad devices
     * 
     * This will detect newly connected devices and remove disconnected ones.
     * Existing device indices may change after this call.
     */
    void RefreshDevices();
    
    /**
     * @brief Gets the number of currently connected devices
     * @return Number of connected gamepad devices
     */
    int GetDeviceCount() const { return static_cast<int>(m_devices.size()); }
    
    /**
     * @brief Gets the human-readable name of a device
     * @param deviceIndex Zero-based device index
     * @return Device name string, empty if index invalid
     */
    std::string GetDeviceName(int deviceIndex) const;
    
    // Input state checking
    
    /**
     * @brief Gets the raw device state
     * @return Raw current device state
     */
    DIJOYSTATE2 GetRawDeviceState(int deviceIndex) const;
     
     /**
     * @brief Checks if an input is currently pressed
     * @param deviceIndex Zero-based device index
     * @param input Input to check
     * @return True if input is currently active
     * 
     * For digital inputs, returns true while pressed.
     * For analog inputs, returns true if above threshold.
     */
    bool IsPressed(int deviceIndex, GamepadInput input) const;
    
    /**
     * @brief Checks if an input was just pressed this frame
     * @param deviceIndex Zero-based device index  
     * @param input Input to check
     * @return True if input transitioned from inactive to active this frame
     * 
     * Useful for detecting button press events rather than held states.
     */
    bool WasPressed(int deviceIndex, GamepadInput input) const;
    
    /**
     * @brief Checks if an input was just released this frame
     * @param deviceIndex Zero-based device index
     * @param input Input to check  
     * @return True if input transitioned from active to inactive this frame
     * 
     * Useful for detecting button release events.
     */
    bool WasReleased(int deviceIndex, GamepadInput input) const;
    
    // Analog input values
    
    /**
     * @brief Gets analog axis value for stick inputs
     * @param deviceIndex Zero-based device index
     * @param axis Axis input (LSTICK_*, RSTICK_*)
     * @return Normalized axis value (-1.0 to 1.0)
     * 
     * For stick inputs:
     * - Left/Right: -1.0 = full left, 1.0 = full right
     * - Up/Down: -1.0 = full up, 1.0 = full down
     */
    float GetAxisValue(int deviceIndex, GamepadInput axis) const;
    
    /**
     * @brief Gets analog trigger value
     * @param deviceIndex Zero-based device index
     * @param trigger Trigger input (LTRIGGER or RTRIGGER)
     * @return Normalized trigger value (0.0 to 1.0)
     * 
     * 0.0 = not pressed, 1.0 = fully pressed
     */
    float GetTriggerValue(int deviceIndex, GamepadInput trigger) const;
    
    // Input configuration
    
    /**
     * @brief Configures how a logical input maps to physical hardware
     * @param input Logical input to configure
     * @param config Configuration specifying the physical mapping
     * 
     * This allows remapping inputs at runtime. Use the helper functions
     * CreateButtonConfig(), CreatePOVConfig(), and CreateAxisConfig()
     * to easily create configurations.
     */
    void ConfigureInput(GamepadInput input, const InputConfig& config);
    
    /**
     * @brief Gets current configuration for an input
     * @param input Input to query
     * @return Current input configuration
     */
    InputConfig GetInputConfig(GamepadInput input) const;
    
    /**
     * @brief Resets all inputs to default Xbox controller configuration
     * 
     * This restores the standard mapping suitable for Xbox controllers.
     */
    void ResetToDefaultConfiguration();
    
    // Update function
    
    /**
     * @brief Updates all device states and checks for connection changes
     * 
     * This must be called once per frame, typically at the beginning of
     * your game loop, to update input states and handle device reconnections.
     */
    void Update();
    
    // Utility functions for configuration
    
    /**
     * @brief Creates configuration for a digital button
     * @param buttonIndex Physical button index (0-127)
     * @return InputConfig for the specified button
     * 
     * Helper function to create button mapping configurations.
     */
    static InputConfig CreateButtonConfig(int buttonIndex);
    
    /**
     * @brief Creates configuration for a POV hat direction
     * @param povDirection Direction in hundredths of degrees (0-35900)
     * @return InputConfig for the specified POV direction
     * 
     * Common values:
     * - 0 = Up
     * - 9000 = Right  
     * - 18000 = Down
     * - 27000 = Left
     * - 0xFFFFFFFF = Center
     */
    static InputConfig CreatePOVConfig(DWORD povDirection);
    
    /**
     * @brief Gets raw button state for debugging purposes
     * @param deviceIndex Zero-based device index
     * @param buttonIndex Physical button index to check
     * @return True if the specified button index is pressed
     * 
     * This function is useful for debugging controller mappings by allowing
     * you to check individual button indices directly.
     */
    bool IsRawButtonPressed(int deviceIndex, int buttonIndex) const;
    
    /**
    * @brief Prints current button states for debugging
    * @param deviceIndex Zero-based device index
    *
    * This function prints the state of all buttons (0-31) to help identify
    * which physical button indices correspond to which controller buttons.
    */
    void DebugPrintButtonStates(int deviceIndex) const;

    /**
     * @brief Creates configuration for an analog axis
     * @param axisIndex Axis index (0=X, 1=Y, 2=Z, 3=RX, 4=RY, 5=RZ)
     * @param positive True for positive direction, false for negative
     * @param threshold Activation threshold (0.0-1.0, default 0.5)
     * @return InputConfig for the specified axis
     * 
     * Creates axis mapping for analog-to-digital conversion.
     */
    static InputConfig CreateAxisConfig(int axisIndex, bool positive, float threshold = 0.5f);

    /**
     * @brief Gets the index of the first button currently being pressed
     * @param deviceIndex Zero-based device index
     * @return Button index (0-127) if a button is pressed, -1 if none pressed
     *
     * This function is useful for identifying button indices when setting up
     * custom input mappings. It returns the index of the first pressed button
     * it finds, scanning from button 0 to 127.
     */
    int GetPressedButtonIndex(int deviceIndex) const;

    /**
     * @brief Gets all currently pressed button indices
     * @param deviceIndex Zero-based device index
     * @return Vector containing all pressed button indices
     *
     * This function returns all buttons that are currently pressed, which is
     * useful if multiple buttons might be pressed simultaneously during mapping.
     */
    std::vector<int> GetAllPressedButtonIndices(int deviceIndex) const;
};