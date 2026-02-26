// GamepadHandler.cpp - Linux/SDL2 implementation

#include "GamepadHandler.hpp"
#include <iostream>
#include <algorithm>

std::vector<std::unique_ptr<GamepadHandler::GamepadDevice>> GamepadHandler::m_devices;
std::map<GamepadHandler::GamepadInput, GamepadHandler::InputConfig> GamepadHandler::m_inputConfigs;
HWND GamepadHandler::m_hwnd = nullptr;

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

    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
            std::cerr << "SDL_Init(gamecontroller) failed: " << SDL_GetError() << "\n";
            return false;
        }
    }

    RefreshDevices();
    return true;
}

void GamepadHandler::Shutdown()
{
    for (auto& dev : m_devices) {
        if (dev->controller) {
            SDL_GameControllerClose(dev->controller);
            dev->controller = nullptr;
        }
    }
    m_devices.clear();
}

void GamepadHandler::RefreshDevices()
{
    Shutdown();

    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; i++) {
        auto dev = std::make_unique<GamepadDevice>();
        if (SDL_IsGameController(i)) {
            dev->controller = SDL_GameControllerOpen(i);
            if (dev->controller) {
                dev->joystick = SDL_GameControllerGetJoystick(dev->controller);
                dev->name = SDL_GameControllerName(dev->controller);
                dev->instanceId = SDL_JoystickInstanceID(dev->joystick);
                dev->connected = true;
                m_devices.push_back(std::move(dev));
            }
        } else {
            dev->joystick = SDL_JoystickOpen(i);
            if (dev->joystick) {
                dev->name = SDL_JoystickName(dev->joystick);
                dev->instanceId = SDL_JoystickInstanceID(dev->joystick);
                dev->connected = true;
                m_devices.push_back(std::move(dev));
            }
        }
    }
}

std::string GamepadHandler::GetDeviceName(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return "";
    return m_devices[deviceIndex]->name;
}

void GamepadHandler::Update()
{
    SDL_GameControllerUpdate();
    UpdateDeviceStates();
}

void GamepadHandler::UpdateDeviceStates()
{
    for (auto& dev : m_devices) {
        dev->previousState = dev->currentState;
        memset(&dev->currentState, 0, sizeof(DIJOYSTATE2));

        if (dev->controller) {
            // Map SDL game controller buttons to DIJOYSTATE2
            auto mapBtn = [&](SDL_GameControllerButton sdlBtn, int idx) {
                if (SDL_GameControllerGetButton(dev->controller, sdlBtn))
                    dev->currentState.rgbButtons[idx] = 0x80;
            };
            mapBtn(SDL_CONTROLLER_BUTTON_A, 0);
            mapBtn(SDL_CONTROLLER_BUTTON_B, 1);
            mapBtn(SDL_CONTROLLER_BUTTON_X, 2);
            mapBtn(SDL_CONTROLLER_BUTTON_Y, 3);
            mapBtn(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 4);
            mapBtn(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 5);
            mapBtn(SDL_CONTROLLER_BUTTON_BACK, 6);
            mapBtn(SDL_CONTROLLER_BUTTON_START, 7);
            mapBtn(SDL_CONTROLLER_BUTTON_LEFTSTICK, 8);
            mapBtn(SDL_CONTROLLER_BUTTON_RIGHTSTICK, 9);

            // Map axes
            auto axis = [&](SDL_GameControllerAxis a) -> long {
                Sint16 v = SDL_GameControllerGetAxis(dev->controller, a);
                return (long)v;
            };
            dev->currentState.lX  = axis(SDL_CONTROLLER_AXIS_LEFTX);
            dev->currentState.lY  = axis(SDL_CONTROLLER_AXIS_LEFTY);
            dev->currentState.lRx = axis(SDL_CONTROLLER_AXIS_RIGHTX);
            dev->currentState.lRy = axis(SDL_CONTROLLER_AXIS_RIGHTY);
            dev->currentState.lZ  = axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            dev->currentState.lRz = axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

            // D-pad via buttons
            if (SDL_GameControllerGetButton(dev->controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
                dev->currentState.rgdwPOV[0] = 0;
            else if (SDL_GameControllerGetButton(dev->controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
                dev->currentState.rgdwPOV[0] = 18000;
            else if (SDL_GameControllerGetButton(dev->controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
                dev->currentState.rgdwPOV[0] = 27000;
            else if (SDL_GameControllerGetButton(dev->controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
                dev->currentState.rgdwPOV[0] = 9000;
            else
                dev->currentState.rgdwPOV[0] = 0xFFFFFFFF; // centered
        } else if (dev->joystick) {
            // Raw joystick fallback
            int numButtons = std::min(SDL_JoystickNumButtons(dev->joystick), 128);
            for (int b = 0; b < numButtons; b++) {
                if (SDL_JoystickGetButton(dev->joystick, b))
                    dev->currentState.rgbButtons[b] = 0x80;
            }
            // axes
            if (SDL_JoystickNumAxes(dev->joystick) > 0)
                dev->currentState.lX  = SDL_JoystickGetAxis(dev->joystick, 0);
            if (SDL_JoystickNumAxes(dev->joystick) > 1)
                dev->currentState.lY  = SDL_JoystickGetAxis(dev->joystick, 1);
            if (SDL_JoystickNumAxes(dev->joystick) > 2)
                dev->currentState.lZ  = SDL_JoystickGetAxis(dev->joystick, 2);
            if (SDL_JoystickNumAxes(dev->joystick) > 3)
                dev->currentState.lRx = SDL_JoystickGetAxis(dev->joystick, 3);
            if (SDL_JoystickNumAxes(dev->joystick) > 4)
                dev->currentState.lRy = SDL_JoystickGetAxis(dev->joystick, 4);
            if (SDL_JoystickNumAxes(dev->joystick) > 5)
                dev->currentState.lRz = SDL_JoystickGetAxis(dev->joystick, 5);
        }
    }
}

float GamepadHandler::GetAxisValueInternal(const GamepadDevice& device, int axisIndex) const
{
    long raw = 0;
    switch (axisIndex) {
        case 0: raw = device.currentState.lX;  break;
        case 1: raw = device.currentState.lY;  break;
        case 2: raw = device.currentState.lZ;  break;
        case 3: raw = device.currentState.lRx; break;
        case 4: raw = device.currentState.lRy; break;
        case 5: raw = device.currentState.lRz; break;
        default: return 0.0f;
    }
    return (float)raw / 32767.0f;
}

bool GamepadHandler::CheckInputCondition(const GamepadDevice& device, const InputConfig& config) const
{
    if (config.usePOV) {
        if (device.currentState.rgdwPOV[0] == 0xFFFFFFFF) return false;
        DWORD pov = device.currentState.rgdwPOV[0];
        // Check within 4500 centidegrees of target direction
        int diff = (int)pov - (int)config.povDirection;
        if (diff < 0) diff += 36000;
        return (diff <= 4500 || diff >= 31500);
    }
    if (config.useAxis) {
        float val = GetAxisValueInternal(device, config.axisIndex);
        return config.axisPositive ? (val > config.threshold) : (val < -config.threshold);
    }
    // Button
    if (config.buttonMask < 128)
        return (device.currentState.rgbButtons[config.buttonMask] & 0x80) != 0;
    return false;
}

DIJOYSTATE2 GamepadHandler::GetRawDeviceState(int deviceIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size())
        return DIJOYSTATE2{};
    return m_devices[deviceIndex]->currentState;
}

bool GamepadHandler::IsPressed(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return false;
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end()) return false;
    return CheckInputCondition(*m_devices[deviceIndex], it->second);
}

bool GamepadHandler::WasPressed(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return false;
    // Create a temp device with previous state to check
    GamepadDevice prevDev = *m_devices[deviceIndex];
    prevDev.currentState = m_devices[deviceIndex]->previousState;
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end()) return false;
    bool wasActive = CheckInputCondition(prevDev, it->second);
    bool isActive = CheckInputCondition(*m_devices[deviceIndex], it->second);
    return isActive && !wasActive;
}

bool GamepadHandler::WasReleased(int deviceIndex, GamepadInput input) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return false;
    GamepadDevice prevDev = *m_devices[deviceIndex];
    prevDev.currentState = m_devices[deviceIndex]->previousState;
    auto it = m_inputConfigs.find(input);
    if (it == m_inputConfigs.end()) return false;
    bool wasActive = CheckInputCondition(prevDev, it->second);
    bool isActive = CheckInputCondition(*m_devices[deviceIndex], it->second);
    return !isActive && wasActive;
}

float GamepadHandler::GetAxisValue(int deviceIndex, GamepadInput axis) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return 0.0f;
    auto it = m_inputConfigs.find(axis);
    if (it == m_inputConfigs.end() || !it->second.useAxis) return 0.0f;
    return GetAxisValueInternal(*m_devices[deviceIndex], it->second.axisIndex);
}

float GamepadHandler::GetTriggerValue(int deviceIndex, GamepadInput trigger) const
{
    return GetAxisValue(deviceIndex, trigger);
}

void GamepadHandler::SetInputConfig(GamepadInput input, const InputConfig& config)
{
    m_inputConfigs[input] = config;
}

GamepadHandler::InputConfig GamepadHandler::GetInputConfig(GamepadInput input) const
{
    auto it = m_inputConfigs.find(input);
    if (it != m_inputConfigs.end()) return it->second;
    return InputConfig{};
}

void GamepadHandler::ResetToDefaultConfiguration()
{
    m_inputConfigs.clear();
    SetupDefaultConfiguration();
}

void GamepadHandler::SetupDefaultConfiguration()
{
    // Standard SDL2 game controller button mapping
    auto setBtn = [&](GamepadInput gi, int btnIdx) {
        InputConfig c;
        c.buttonMask = (DWORD)btnIdx;
        m_inputConfigs[gi] = c;
    };
    setBtn(GamepadInput::BUTTON_A, 0);
    setBtn(GamepadInput::BUTTON_B, 1);
    setBtn(GamepadInput::BUTTON_X, 2);
    setBtn(GamepadInput::BUTTON_Y, 3);
    setBtn(GamepadInput::BUTTON_LB, 4);
    setBtn(GamepadInput::BUTTON_RB, 5);
    setBtn(GamepadInput::BUTTON_BACK, 6);
    setBtn(GamepadInput::BUTTON_START, 7);
    setBtn(GamepadInput::BUTTON_LS, 8);
    setBtn(GamepadInput::BUTTON_RS, 9);

    // D-pad via POV
    auto setPOV = [&](GamepadInput gi, DWORD dir) {
        InputConfig c;
        c.usePOV = true;
        c.povDirection = dir;
        m_inputConfigs[gi] = c;
    };
    setPOV(GamepadInput::DPAD_UP, 0);
    setPOV(GamepadInput::DPAD_RIGHT, 9000);
    setPOV(GamepadInput::DPAD_DOWN, 18000);
    setPOV(GamepadInput::DPAD_LEFT, 27000);

    // Left stick (axis 0=X, 1=Y)
    auto setAxis = [&](GamepadInput gi, int ax, bool pos) {
        InputConfig c;
        c.useAxis = true;
        c.axisIndex = ax;
        c.axisPositive = pos;
        c.threshold = 0.5f;
        m_inputConfigs[gi] = c;
    };
    setAxis(GamepadInput::LSTICK_RIGHT, 0, true);
    setAxis(GamepadInput::LSTICK_LEFT,  0, false);
    setAxis(GamepadInput::LSTICK_DOWN,  1, true);
    setAxis(GamepadInput::LSTICK_UP,    1, false);
    setAxis(GamepadInput::RSTICK_RIGHT, 3, true);
    setAxis(GamepadInput::RSTICK_LEFT,  3, false);
    setAxis(GamepadInput::RSTICK_DOWN,  4, true);
    setAxis(GamepadInput::RSTICK_UP,    4, false);
    setAxis(GamepadInput::LTRIGGER,     2, true);
    setAxis(GamepadInput::RTRIGGER,     5, true);
}

bool GamepadHandler::GetAnyPressedButton(int deviceIndex, int& buttonIndex) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return false;
    const auto& state = m_devices[deviceIndex]->currentState;
    for (int i = 0; i < 128; i++) {
        if (state.rgbButtons[i] & 0x80) {
            buttonIndex = i;
            return true;
        }
    }
    return false;
}

bool GamepadHandler::GetAnyActiveAxis(int deviceIndex, int& axisIndex, bool& positive) const
{
    if (deviceIndex < 0 || deviceIndex >= (int)m_devices.size()) return false;
    const auto& state = m_devices[deviceIndex]->currentState;
    long axes[6] = { state.lX, state.lY, state.lZ, state.lRx, state.lRy, state.lRz };
    for (int i = 0; i < 6; i++) {
        float v = (float)axes[i] / 32767.0f;
        if (v > 0.7f) { axisIndex = i; positive = true; return true; }
        if (v < -0.7f) { axisIndex = i; positive = false; return true; }
    }
    return false;
}
