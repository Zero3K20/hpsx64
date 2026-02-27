// GamepadHandler.cpp

/**
 * @file GamepadHandler.cpp
 * @brief Implementation of DirectInput gamepad handling class
 * @author Your Name
 * @date 2025
 * @version 1.0
 * 
 * This file contains the implementation of the GamepadHandler class methods
 * for DirectInput gamepad support on Windows.
 */

#include "GamepadHandler.hpp"
#include <iostream>

#include <cstdlib>  // for wcstombs_s

LPDIRECTINPUT8 GamepadHandler::m_pDirectInput = nullptr;                        ///< DirectInput interface
std::vector<std::unique_ptr<GamepadHandler::GamepadDevice>> GamepadHandler::m_devices;          ///< Connected devices
std::map<GamepadHandler::GamepadInput, GamepadHandler::InputConfig> GamepadHandler::m_inputConfigs;             ///< Input configuration map
HWND GamepadHandler::m_hwnd = nullptr;                                          ///< Window handle


GamepadHandler::GamepadHandler()
{
    SetupDefaultConfiguration();
}

GamepadHandler::~GamepadHandler()
{
    Shutdown();
}

bool GamepadHandler::Initialize(HWND hwnd)
{
    m_hwnd = hwnd;
    
    // Create DirectInput object
    HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), 
                                   DIRECTINPUT_VERSION, 
                                   IID_IDirectInput8, 
                                   (VOID**)&m_pDirectInput, 
                                   nullptr);
    
    if (FAILED(hr))
        return false;
    
    RefreshDevices();
    return true;
}

void GamepadHandler::Shutdown()
{
    // Release all device interfaces
    for (auto& device : m_devices)
    {
        if (device->device)
        {
            device->device->Unacquire();
            device->device->Release();
        }
    }
    m_devices.clear();
    
    // Release DirectInput interface
    if (m_pDirectInput)
    {
        m_pDirectInput->Release();
        m_pDirectInput = nullptr;
    }
}

void GamepadHandler::RefreshDevices()
{
    if (!m_pDirectInput)
        return;
    
    // Clear existing devices
    for (auto& device : m_devices)
    {
        if (device->device)
        {
            device->device->Unacquire();
            device->device->Release();
        }
    }
    m_devices.clear();
    
    // Enumerate joysticks
    m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
}

BOOL CALLBACK GamepadHandler::EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    GamepadHandler* pThis = static_cast<GamepadHandler*>(pContext);
    pThis->InitializeDevice(pdidInstance);
    return DIENUM_CONTINUE;
}

bool GamepadHandler::InitializeDevice(const DIDEVICEINSTANCE* pdidInstance)
{
    auto device = std::make_unique<GamepadDevice>();
    device->guid = pdidInstance->guidInstance;

    // Convert TCHAR string to std::string safely
#ifdef _UNICODE
    // Unicode build - convert from wide to narrow
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, pdidInstance->tszProductName, -1, nullptr, 0, nullptr, nullptr);
    if (bufferSize > 0)
    {
        std::vector<char> buffer(bufferSize);
        int result = WideCharToMultiByte(CP_UTF8, 0, pdidInstance->tszProductName, -1, buffer.data(), bufferSize, nullptr, nullptr);
        if (result > 0)
        {
            device->name = buffer.data();
        }
        else
        {
            device->name = "Unknown Device";
        }
    }
    else
    {
        device->name = "Unknown Device";
    }
#else
    // ANSI build - direct assignment
    device->name = pdidInstance->tszProductName;
#endif

    // Create device interface
    HRESULT hr = m_pDirectInput->CreateDevice(pdidInstance->guidInstance, &device->device, nullptr);
    if (FAILED(hr))
        return false;

    // Set data format to standard joystick format
    hr = device->device->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr))
    {
        device->device->Release();
        return false;
    }

    // Set cooperative level - background and non-exclusive
    hr = device->device->SetCooperativeLevel(m_hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr))
    {
        device->device->Release();
        return false;
    }

    // Get device capabilities
    device->capabilities.dwSize = sizeof(DIDEVCAPS);
    device->device->GetCapabilities(&device->capabilities);

    // Enumerate axes to set their ranges
    device->device->EnumObjects(EnumObjectsCallback, device->device, DIDFT_AXIS);

    // Acquire the device for input
    device->device->Acquire();

    m_devices.push_back(std::move(device));
    return true;
}

BOOL CALLBACK GamepadHandler::EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
    LPDIRECTINPUTDEVICE8 device = static_cast<LPDIRECTINPUTDEVICE8>(pContext);
    
    // Set axis ranges if this is an axis object
    if (pdidoi->dwType & DIDFT_AXIS)
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = pdidoi->dwType;
        diprg.lMin = 1;
        diprg.lMax = 256;
        
        device->SetProperty(DIPROP_RANGE, &diprg.diph);
    }
    
    return DIENUM_CONTINUE;
}

void GamepadHandler::Update()
{
    UpdateDeviceStates();
    CheckDeviceConnections();
}

void GamepadHandler::UpdateDeviceStates()
{
    for (auto& device : m_devices)
    {
        if (!device->device || !device->connected)
            continue;
        
        // Store previous state for edge detection
        device->previousState = device->currentState;
        
        // Poll the device to update its state
        device->device->Poll();
        
        // Get current device state
        HRESULT hr = device->device->GetDeviceState(sizeof(DIJOYSTATE2), &device->currentState);
        
        if (FAILED(hr))
        {
            // Device may have been disconnected, try to reacquire
            hr = device->device->Acquire();
            if (FAILED(hr))
            {
                device->connected = false;
            }
        }
    }
}

void GamepadHandler::CheckDeviceConnections()
{
    // Check each device's connection status and attempt reconnection
    for (size_t i = 0; i < m_devices.size(); ++i)
    {
        bool wasConnected = m_devices[i]->connected;
        
        if (!wasConnected)
        {
            // Try to reacquire disconnected device
            HRESULT hr = m_devices[i]->device->Acquire();
            if (SUCCEEDED(hr))
            {
                // Device successfully reacquired
                m_devices[i]->connected = true;
            }
        }
        else
        {
            // Test connection by polling the device
            HRESULT hr = m_devices[i]->device->Poll();
            bool isConnected = SUCCEEDED(hr) || hr == DI_NOEFFECT;
            
            // Update connection status if changed
            if (!isConnected)
            {
                m_devices[i]->connected = false;
            }
        }
    }
}

bool GamepadHandler::IsPressed(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return false;
    
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end())
        return false;
    
    return CheckInputCondition(*m_devices[deviceIndex], it->second);
}

bool GamepadHandler::WasPressed(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return false;
    
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end())
        return false;
    
    bool currentPressed = CheckInputCondition(*m_devices[deviceIndex], it->second);
    
    // Temporarily set current state to previous for comparison
    DIJOYSTATE2 tempCurrent = m_devices[deviceIndex]->currentState;
    const_cast<GamepadDevice*>(m_devices[deviceIndex].get())->currentState = m_devices[deviceIndex]->previousState;
    bool previousPressed = CheckInputCondition(*m_devices[deviceIndex], it->second);
    const_cast<GamepadDevice*>(m_devices[deviceIndex].get())->currentState = tempCurrent;
    
    return currentPressed && !previousPressed;
}

bool GamepadHandler::WasReleased(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return false;
    
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end())
        return false;
    
    bool currentPressed = CheckInputCondition(*m_devices[deviceIndex], it->second);
    
    // Temporarily set current state to previous for comparison
    DIJOYSTATE2 tempCurrent = m_devices[deviceIndex]->currentState;
    const_cast<GamepadDevice*>(m_devices[deviceIndex].get())->currentState = m_devices[deviceIndex]->previousState;
    bool previousPressed = CheckInputCondition(*m_devices[deviceIndex], it->second);
    const_cast<GamepadDevice*>(m_devices[deviceIndex].get())->currentState = tempCurrent;
    
    return !currentPressed && previousPressed;
}

bool GamepadHandler::CheckInputCondition(const GamepadDevice& device, const InputConfig& config) const
{
    if (config.useAxis)
    {
        // Check analog axis with threshold
        float value = GetAxisValue(device, config.axisIndex);
        if (config.axisPositive)
            return value >= config.threshold;
        else
            return value <= -config.threshold;
    }
    else if (config.usePOV)
    {
        // Check POV hat direction
        DWORD pov = device.currentState.rgdwPOV[0];
        if (pov == 0xFFFFFFFF) // Centered
            return config.povDirection == 0xFFFFFFFF;
        return pov == config.povDirection;
    }
    else
    {
        // Check digital button
        return (device.currentState.rgbButtons[config.buttonMask] & 0x80) != 0;
    }
}

float GamepadHandler::GetAxisValue(const GamepadDevice& device, int axisIndex) const
{
    LONG value = 0;
    
    // Get raw axis value based on index
    switch (axisIndex)
    {
        case 0: value = device.currentState.lX; break;      // X axis (left stick horizontal)
        case 1: value = device.currentState.lY; break;      // Y axis (left stick vertical)
        case 2: value = device.currentState.lZ; break;      // Z axis (often left trigger)
        case 3: value = device.currentState.lRx; break;     // RX axis (right stick horizontal)
        case 4: value = device.currentState.lRy; break;     // RY axis (right stick vertical)
        case 5: value = device.currentState.lRz; break;     // RZ axis (often right trigger)
        default: return 0.0f;
    }
    
    // Normalize to -1.0 to 1.0 range
    return static_cast<float>(value) / 1000.0f;
}

float GamepadHandler::GetAxisValue(int deviceIndex, GamepadInput axis) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return 0.0f;
    
    switch (axis)
    {
        case GamepadInput::LSTICK_LEFT:
        case GamepadInput::LSTICK_RIGHT:
            return GetAxisValue(*m_devices[deviceIndex], 0); // X axis
        case GamepadInput::LSTICK_UP:
        case GamepadInput::LSTICK_DOWN:
            return -GetAxisValue(*m_devices[deviceIndex], 1); // Y axis (inverted for up/down)
        case GamepadInput::RSTICK_LEFT:
        case GamepadInput::RSTICK_RIGHT:
            return GetAxisValue(*m_devices[deviceIndex], 3); // RX axis
        case GamepadInput::RSTICK_UP:
        case GamepadInput::RSTICK_DOWN:
            return -GetAxisValue(*m_devices[deviceIndex], 4); // RY axis (inverted for up/down)
        default:
            return 0.0f;
    }
}

float GamepadHandler::GetTriggerValue(int deviceIndex, GamepadInput trigger) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return 0.0f;
    
    switch (trigger)
    {
        case GamepadInput::LTRIGGER:
            // Convert from -1..1 range to 0..1 range for trigger
            return (GetAxisValue(*m_devices[deviceIndex], 2) + 1.0f) * 0.5f;
        case GamepadInput::RTRIGGER:
            // Convert from -1..1 range to 0..1 range for trigger
            return (GetAxisValue(*m_devices[deviceIndex], 5) + 1.0f) * 0.5f;
        default:
            return 0.0f;
    }
}

std::string GamepadHandler::GetDeviceName(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()))
        return "";
    return m_devices[deviceIndex]->name;
}

DIJOYSTATE2 GamepadHandler::GetRawDeviceState(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()))
        return {};
    return m_devices[deviceIndex]->currentState;
}

void GamepadHandler::ConfigureInput(GamepadInput input, const InputConfig& config)
{
    m_inputConfigs[input] = config;
}

GamepadHandler::InputConfig GamepadHandler::GetInputConfig(GamepadInput input) const
{
    auto it = m_inputConfigs.find(input);
    return it != m_inputConfigs.end() ? it->second : InputConfig{};
}

void GamepadHandler::ResetToDefaultConfiguration()
{
    m_inputConfigs.clear();
    SetupDefaultConfiguration();
}

void GamepadHandler::SetupDefaultConfiguration()
{
    // Default Xbox controller button mapping
    // Note: Button indices may vary by controller. Common mappings:
    // Xbox 360/One: A=0, B=1, X=2, Y=3, LB=4, RB=5, Back=6, Start=7, LS=8, RS=9
    // Some controllers: A=0, B=1, X=2, Y=3, LB=4, RB=5, Back=6, Start=7, LS=9, RS=10
    m_inputConfigs[GamepadInput::BUTTON_A] = CreateButtonConfig(0);
    m_inputConfigs[GamepadInput::BUTTON_B] = CreateButtonConfig(1);
    m_inputConfigs[GamepadInput::BUTTON_X] = CreateButtonConfig(2);
    m_inputConfigs[GamepadInput::BUTTON_Y] = CreateButtonConfig(3);
    m_inputConfigs[GamepadInput::BUTTON_LB] = CreateButtonConfig(4);
    m_inputConfigs[GamepadInput::BUTTON_RB] = CreateButtonConfig(5);
    m_inputConfigs[GamepadInput::BUTTON_BACK] = CreateButtonConfig(6);
    m_inputConfigs[GamepadInput::BUTTON_START] = CreateButtonConfig(7);
    m_inputConfigs[GamepadInput::BUTTON_LS] = CreateButtonConfig(8);
    m_inputConfigs[GamepadInput::BUTTON_RS] = CreateButtonConfig(9);
    
    // D-Pad mapping using POV hat
    m_inputConfigs[GamepadInput::DPAD_UP] = CreatePOVConfig(0);
    m_inputConfigs[GamepadInput::DPAD_RIGHT] = CreatePOVConfig(9000);
    m_inputConfigs[GamepadInput::DPAD_DOWN] = CreatePOVConfig(18000);
    m_inputConfigs[GamepadInput::DPAD_LEFT] = CreatePOVConfig(27000);
    
    // Left analog stick as digital inputs with threshold
    m_inputConfigs[GamepadInput::LSTICK_LEFT] = CreateAxisConfig(0, false, 0.5f);
    m_inputConfigs[GamepadInput::LSTICK_RIGHT] = CreateAxisConfig(0, true, 0.5f);
    m_inputConfigs[GamepadInput::LSTICK_UP] = CreateAxisConfig(1, false, 0.5f);
    m_inputConfigs[GamepadInput::LSTICK_DOWN] = CreateAxisConfig(1, true, 0.5f);
    
    // Right analog stick as digital inputs with threshold
    m_inputConfigs[GamepadInput::RSTICK_LEFT] = CreateAxisConfig(3, false, 0.5f);
    m_inputConfigs[GamepadInput::RSTICK_RIGHT] = CreateAxisConfig(3, true, 0.5f);
    m_inputConfigs[GamepadInput::RSTICK_UP] = CreateAxisConfig(4, false, 0.5f);
    m_inputConfigs[GamepadInput::RSTICK_DOWN] = CreateAxisConfig(4, true, 0.5f);
    
    // Triggers as digital inputs with low threshold
    m_inputConfigs[GamepadInput::LTRIGGER] = CreateAxisConfig(2, true, 0.1f);
    m_inputConfigs[GamepadInput::RTRIGGER] = CreateAxisConfig(5, true, 0.1f);
}

GamepadHandler::InputConfig GamepadHandler::CreateButtonConfig(int buttonIndex)
{
    InputConfig config;
    config.buttonMask = buttonIndex;
    config.useAxis = false;
    config.usePOV = false;
    return config;
}

GamepadHandler::InputConfig GamepadHandler::CreatePOVConfig(DWORD povDirection)
{
    InputConfig config;
    config.povDirection = povDirection;
    config.useAxis = false;
    config.usePOV = true;
    return config;
}

GamepadHandler::InputConfig GamepadHandler::CreateAxisConfig(int axisIndex, bool positive, float threshold)
{
    InputConfig config;
    config.axisIndex = axisIndex;
    config.axisPositive = positive;
    config.threshold = threshold;
    config.useAxis = true;
    config.usePOV = false;
    return config;
}

bool GamepadHandler::IsRawButtonPressed(int deviceIndex, int buttonIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return false;
    
    if (buttonIndex < 0 || buttonIndex >= 128) // DirectInput supports up to 128 buttons
        return false;
    
    return (m_devices[deviceIndex]->currentState.rgbButtons[buttonIndex] & 0x80) != 0;
}

void GamepadHandler::DebugPrintButtonStates(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
    {
        std::cout << "Invalid device index or device not connected." << std::endl;
        return;
    }
    
    std::cout << "Button states for device " << deviceIndex << " (" << m_devices[deviceIndex]->name << "):" << std::endl;
    
    bool anyPressed = false;
    for (int i = 0; i < 32; ++i) // Check first 32 buttons (most common range)
    {
        if (m_devices[deviceIndex]->currentState.rgbButtons[i] & 0x80)
        {
            std::cout << "Button " << i << " is pressed" << std::endl;
            anyPressed = true;
        }
    }
    
    if (!anyPressed)
    {
        std::cout << "No buttons currently pressed." << std::endl;
    }
    
    // Also show POV state
    DWORD pov = m_devices[deviceIndex]->currentState.rgdwPOV[0];
    if (pov != 0xFFFFFFFF)
    {
        std::cout << "POV: " << pov << " degrees" << std::endl;
    }
}

int GamepadHandler::GetPressedButtonIndex(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return -1;

    // Check all buttons from 0 to 127 (DirectInput max)
    for (int i = 0; i < 128; ++i)
    {
        if (m_devices[deviceIndex]->currentState.rgbButtons[i] & 0x80)
        {
            return i; // Return the first pressed button found
        }
    }

    return -1; // No button currently pressed
}

std::vector<int> GamepadHandler::GetAllPressedButtonIndices(int deviceIndex) const
{
    std::vector<int> pressedButtons;

    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devices.size()) || !m_devices[deviceIndex]->connected)
        return pressedButtons;

    // Check all buttons from 0 to 127 (DirectInput max)
    for (int i = 0; i < 128; ++i)
    {
        if (m_devices[deviceIndex]->currentState.rgbButtons[i] & 0x80)
        {
            pressedButtons.push_back(i);
        }
    }

    return pressedButtons;
}