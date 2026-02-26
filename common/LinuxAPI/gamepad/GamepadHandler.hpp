// GamepadHandler.hpp - Linux/SDL2 implementation
// Provides the same public API as the Windows DirectInput version

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <functional>
#include <map>
#include <string>
#include <memory>
#include <cstdint>

// These type aliases are provided by windows_compat.h when included before this
// Only define them if not already defined (i.e. when used standalone)
#if !defined(_WIN32) && !defined(DWORD)
   typedef uint32_t DWORD;
#endif
#if !defined(_WIN32) && !defined(BYTE)
   typedef uint8_t BYTE;
#endif
#if !defined(_WIN32) && !defined(HWND)
   typedef void* HWND;
#endif

// Simplified DIJOYSTATE2-compatible struct for Linux
struct DIJOYSTATE2 {
    long lX;    // X axis
    long lY;    // Y axis
    long lZ;    // Z axis
    long lRx;   // Rx axis
    long lRy;   // Ry axis
    long lRz;   // Rz axis
    BYTE rgbButtons[128];   // buttons
    DWORD rgdwPOV[4];       // POV hats
};

class GamepadHandler
{
public:
    enum class GamepadInput : int
    {
        BUTTON_A = 0,
        BUTTON_B = 1,
        BUTTON_X = 2,
        BUTTON_Y = 3,
        BUTTON_LB = 4,
        BUTTON_RB = 5,
        BUTTON_BACK = 6,
        BUTTON_START = 7,
        BUTTON_LS = 8,
        BUTTON_RS = 9,
        DPAD_UP = 10,
        DPAD_DOWN = 11,
        DPAD_LEFT = 12,
        DPAD_RIGHT = 13,
        LSTICK_UP = 14,
        LSTICK_DOWN = 15,
        LSTICK_LEFT = 16,
        LSTICK_RIGHT = 17,
        RSTICK_UP = 18,
        RSTICK_DOWN = 19,
        RSTICK_LEFT = 20,
        RSTICK_RIGHT = 21,
        LTRIGGER = 22,
        RTRIGGER = 23,
        COUNT = 24
    };

    struct InputConfig
    {
        DWORD buttonMask = 0;
        DWORD povDirection = 0xFFFFFFFF;
        bool useAxis = false;
        int axisIndex = -1;
        bool axisPositive = true;
        float threshold = 0.5f;
        bool usePOV = false;
    };

private:
    struct GamepadDevice
    {
        SDL_GameController* controller = nullptr;
        SDL_Joystick* joystick = nullptr;
        DIJOYSTATE2 currentState = {};
        DIJOYSTATE2 previousState = {};
        std::string name;
        bool connected = true;
        int instanceId = -1;
    };

    static std::vector<std::unique_ptr<GamepadDevice>> m_devices;
    static std::map<GamepadInput, InputConfig> m_inputConfigs;
    static HWND m_hwnd;

    void SetupDefaultConfiguration();
    void UpdateDeviceStates();
    bool CheckInputCondition(const GamepadDevice& device, const InputConfig& config) const;
    float GetAxisValueInternal(const GamepadDevice& device, int axisIndex) const;

public:
    GamepadHandler();
    ~GamepadHandler();

    bool Initialize(HWND hwnd);
    void Shutdown();
    void RefreshDevices();

    int GetDeviceCount() const { return static_cast<int>(m_devices.size()); }
    std::string GetDeviceName(int deviceIndex) const;

    DIJOYSTATE2 GetRawDeviceState(int deviceIndex) const;
    bool IsPressed(int deviceIndex, GamepadInput input) const;
    bool WasPressed(int deviceIndex, GamepadInput input) const;
    bool WasReleased(int deviceIndex, GamepadInput input) const;
    float GetAxisValue(int deviceIndex, GamepadInput axis) const;
    float GetTriggerValue(int deviceIndex, GamepadInput trigger) const;

    void SetInputConfig(GamepadInput input, const InputConfig& config);
    InputConfig GetInputConfig(GamepadInput input) const;
    void ResetToDefaultConfiguration();
    void Update();

    // Get any button currently held (for remapping)
    bool GetAnyPressedButton(int deviceIndex, int& buttonIndex) const;
    bool GetAnyActiveAxis(int deviceIndex, int& axisIndex, bool& positive) const;
};
