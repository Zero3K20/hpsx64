// GamepadExample.cpp

/**
 * @file GamepadExample.cpp
 * @brief Example usage of the GamepadHandler class
 * @author Your Name
 * @date 2025
 * @version 1.0
 * 
 * This file demonstrates how to use the GamepadHandler class for
 * DirectInput gamepad support in a Windows application.
 */

#include <windows.h>
#include <iostream>
#include <iomanip>
#include "GamepadHandler.hpp"

/**
 * @brief Window procedure for the example application
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


/**
 * @brief Displays current input state for a gamepad
 * @param gamepad Reference to GamepadHandler instance
 * @param deviceIndex Index of device to display
 */
void DisplayGamepadState(const GamepadHandler& gamepad, int deviceIndex)
{
    std::cout << "\r"; // Return to start of line
    std::cout << "Device " << deviceIndex << " (" << gamepad.GetDeviceName(deviceIndex) << "): ";
    
    // Check face buttons
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_A))
        std::cout << "A ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_B))
        std::cout << "B ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_X))
        std::cout << "X ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_Y))
        std::cout << "Y ";
    
    // Check shoulder buttons
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_LB))
        std::cout << "LB ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_RB))
        std::cout << "RB ";
    
    // Check system buttons
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_BACK))
        std::cout << "BACK ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_START))
        std::cout << "START ";
    
    // Check D-Pad
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::DPAD_UP))
        std::cout << "D↑ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::DPAD_DOWN))
        std::cout << "D↓ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::DPAD_LEFT))
        std::cout << "D← ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::DPAD_RIGHT))
        std::cout << "D→ ";
    
    // Check analog sticks (as digital)
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::LSTICK_UP))
        std::cout << "LS↑ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::LSTICK_DOWN))
        std::cout << "LS↓ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::LSTICK_LEFT))
        std::cout << "LS← ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::LSTICK_RIGHT))
        std::cout << "LS→ ";
    
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::RSTICK_UP))
        std::cout << "RS↑ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::RSTICK_DOWN))
        std::cout << "RS↓ ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::RSTICK_LEFT))
        std::cout << "RS← ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::RSTICK_RIGHT))
        std::cout << "RS→ ";
    
    // Check stick clicks
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_LS))
        std::cout << "LSClick ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::BUTTON_RS))
        std::cout << "RSClick ";
    
    // Check triggers (as digital)
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::LTRIGGER))
        std::cout << "LT ";
    if (gamepad.IsPressed(deviceIndex, GamepadHandler::GamepadInput::RTRIGGER))
        std::cout << "RT ";
    
    // Display analog values
    float leftX = gamepad.GetAxisValue(deviceIndex, GamepadHandler::GamepadInput::LSTICK_RIGHT);
    float leftY = gamepad.GetAxisValue(deviceIndex, GamepadHandler::GamepadInput::LSTICK_UP);
    float rightX = gamepad.GetAxisValue(deviceIndex, GamepadHandler::GamepadInput::RSTICK_RIGHT);
    float rightY = gamepad.GetAxisValue(deviceIndex, GamepadHandler::GamepadInput::RSTICK_UP);
    float leftTrigger = gamepad.GetTriggerValue(deviceIndex, GamepadHandler::GamepadInput::LTRIGGER);
    float rightTrigger = gamepad.GetTriggerValue(deviceIndex, GamepadHandler::GamepadInput::RTRIGGER);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "LS(" << leftX << "," << leftY << ") ";
    std::cout << "RS(" << rightX << "," << rightY << ") ";
    std::cout << "LT:" << leftTrigger << " RT:" << rightTrigger;
    
    // Clear rest of line
    std::cout << std::string(20, ' ');
    std::cout.flush();
}

/**
 * @brief Demonstrates input remapping functionality
 * @param gamepad Reference to GamepadHandler instance
 */
void DemonstrateRemapping(GamepadHandler& gamepad)
{
    std::cout << "\n\nDemonstrating input remapping..." << std::endl;
    std::cout << "Original A button mapping: Button " 
              << gamepad.GetInputConfig(GamepadHandler::GamepadInput::BUTTON_A).buttonMask << std::endl;
    
    // Remap A button to what was originally the B button (button index 1)
    auto newConfig = GamepadHandler::CreateButtonConfig(1);
    gamepad.ConfigureInput(GamepadHandler::GamepadInput::BUTTON_A, newConfig);
    
    std::cout << "Remapped A button to: Button " 
              << gamepad.GetInputConfig(GamepadHandler::GamepadInput::BUTTON_A).buttonMask << std::endl;
    std::cout << "Now the 'B' button will register as 'A' button input!" << std::endl;
    std::cout << "Press any key to restore original mapping..." << std::endl;
    
    std::cin.get();
    
    // Restore original mapping
    gamepad.ResetToDefaultConfiguration();
    std::cout << "Original mapping restored." << std::endl;
}


/**
 * @brief Interactive button remapping utility
 * @param gamepad Reference to GamepadHandler instance
 */
void InteractiveButtonMapping(GamepadHandler& gamepad)
{
    if (gamepad.GetDeviceCount() == 0)
    {
        std::cout << "No gamepads connected for button mapping." << std::endl;
        return;
    }

    std::cout << "\n=== Interactive Button Mapping ===" << std::endl;
    std::cout << "This will help you remap any GamepadInput to a physical button." << std::endl;

    // List available inputs to remap
    std::cout << "\nAvailable inputs to remap:" << std::endl;
    std::cout << "1. BUTTON_A" << std::endl;
    std::cout << "2. BUTTON_B" << std::endl;
    std::cout << "3. BUTTON_X" << std::endl;
    std::cout << "4. BUTTON_Y" << std::endl;
    std::cout << "5. BUTTON_LB" << std::endl;
    std::cout << "6. BUTTON_RB" << std::endl;
    std::cout << "7. BUTTON_START" << std::endl;
    std::cout << "8. BUTTON_BACK" << std::endl;
    std::cout << "9. BUTTON_LS (Left stick click)" << std::endl;
    std::cout << "10. BUTTON_RS (Right stick click)" << std::endl;

    std::cout << "\nEnter the number of the input you want to remap (1-10): ";
    int choice;
    std::cin >> choice;

    GamepadHandler::GamepadInput inputToRemap;
    std::string inputName;

    switch (choice)
    {
    case 1: inputToRemap = GamepadHandler::GamepadInput::BUTTON_A; inputName = "BUTTON_A"; break;
    case 2: inputToRemap = GamepadHandler::GamepadInput::BUTTON_B; inputName = "BUTTON_B"; break;
    case 3: inputToRemap = GamepadHandler::GamepadInput::BUTTON_X; inputName = "BUTTON_X"; break;
    case 4: inputToRemap = GamepadHandler::GamepadInput::BUTTON_Y; inputName = "BUTTON_Y"; break;
    case 5: inputToRemap = GamepadHandler::GamepadInput::BUTTON_LB; inputName = "BUTTON_LB"; break;
    case 6: inputToRemap = GamepadHandler::GamepadInput::BUTTON_RB; inputName = "BUTTON_RB"; break;
    case 7: inputToRemap = GamepadHandler::GamepadInput::BUTTON_START; inputName = "BUTTON_START"; break;
    case 8: inputToRemap = GamepadHandler::GamepadInput::BUTTON_BACK; inputName = "BUTTON_BACK"; break;
    case 9: inputToRemap = GamepadHandler::GamepadInput::BUTTON_LS; inputName = "BUTTON_LS"; break;
    case 10: inputToRemap = GamepadHandler::GamepadInput::BUTTON_RS; inputName = "BUTTON_RS"; break;
    default:
        std::cout << "Invalid choice." << std::endl;
        return;
    }

    std::cout << "\nNow press the physical button you want to map " << inputName << " to..." << std::endl;
    std::cout << "(Press ESC to cancel)" << std::endl;

    int deviceIndex = 0; // Use first gamepad
    int lastPressedButton = -1;
    bool buttonDetected = false;

    // Wait for button press
    while (!buttonDetected)
    {
        // Check for ESC key to cancel
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            std::cout << "Button mapping cancelled." << std::endl;
            return;
        }

        gamepad.Update();

        // Get currently pressed button
        int pressedButton = gamepad.GetPressedButtonIndex(deviceIndex);

        if (pressedButton != -1)
        {
            if (pressedButton != lastPressedButton)
            {
                std::cout << "Detected button press: Index " << pressedButton << std::endl;
                std::cout << "Press this button again to confirm mapping, or press a different button to try another..." << std::endl;
                lastPressedButton = pressedButton;
            }
            else
            {
                // Same button pressed twice, confirm mapping
                std::cout << "Confirmed! Mapping " << inputName << " to button index " << pressedButton << std::endl;

                // Create and apply the new configuration
                auto newConfig = GamepadHandler::CreateButtonConfig(pressedButton);
                gamepad.ConfigureInput(inputToRemap, newConfig);

                std::cout << "Mapping applied successfully!" << std::endl;
                std::cout << "Test the mapping by pressing the button you just configured..." << std::endl;

                buttonDetected = true;

                // Brief test period
                for (int i = 0; i < 100; ++i) // 10 seconds at ~100ms per iteration
                {
                    gamepad.Update();

                    if (gamepad.IsPressed(deviceIndex, inputToRemap))
                    {
                        std::cout << "\rSUCCESS: " << inputName << " is working!";
                        std::cout.flush();
                    }
                    else
                    {
                        std::cout << "\rWaiting for " << inputName << " test...";
                        std::cout.flush();
                    }

                    Sleep(100);
                }
                std::cout << std::endl;
            }
        }
        else
        {
            lastPressedButton = -1;
        }

        Sleep(50);
    }

    std::cout << "Button mapping complete! Press any key to return to main menu..." << std::endl;
    std::cin.ignore();
    std::cin.get();
}

/**
 * @brief Main application entry point
 */
int main()
{
    std::cout << "DirectInput Gamepad Handler Example" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Create a minimal window for DirectInput
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"GamepadExample";
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(
        0,
        L"GamepadExample",
        L"Gamepad Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!hwnd)
    {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }
    
    // Initialize gamepad handler
    GamepadHandler gamepad;
    if (!gamepad.Initialize(hwnd))
    {
        std::cerr << "Failed to initialize DirectInput!" << std::endl;
        return -1;
    }
    
    
    std::cout << "Gamepad handler initialized." << std::endl;
    std::cout << "Found " << gamepad.GetDeviceCount() << " gamepad(s)." << std::endl;
    
    // List connected devices
    for (int i = 0; i < gamepad.GetDeviceCount(); ++i)
    {
        std::cout << "Device " << i << ": " << gamepad.GetDeviceName(i) << std::endl;
    }
    
    if (gamepad.GetDeviceCount() == 0)
    {
        std::cout << "No gamepads detected. Connect a gamepad and press R to refresh, or Q to quit." << std::endl;
    }
    else
    {
        std::cout << "\nPress M to test input remapping, R to refresh devices, D to debug button mapping, Q to quit." << std::endl;
        std::cout << "Current input state:" << std::endl;
    }
    
    // Main loop
    bool running = true;
    while (running)
    {
        // Process Windows messages
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!running)
            break;
        
        // Update gamepad states
        gamepad.Update();
        
        // Check for keyboard input
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            running = false;
        }
        if (GetAsyncKeyState('R') & 0x8000)
        {
            gamepad.RefreshDevices();
            std::cout << "\nRefreshed devices. Found " << gamepad.GetDeviceCount() << " gamepad(s)." << std::endl;
            Sleep(500); // Prevent rapid refresh
        }
        if (GetAsyncKeyState('M') & 0x8000)
        {
            DemonstrateRemapping(gamepad);
            Sleep(500); // Prevent rapid triggering
        }
        if (GetAsyncKeyState('N') & 0x8000)
        {
            InteractiveButtonMapping(gamepad);
            Sleep(500); // Prevent rapid triggering
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            std::cout << "\n\nDEBUG MODE: Press any button on controller 0 to see its button index..." << std::endl;
            std::cout << "This will help you identify the correct mapping for stick clicks." << std::endl;
            std::cout << "Press ESC to exit debug mode." << std::endl;
            
            bool debugMode = true;
            while (debugMode && gamepad.GetDeviceCount() > 0)
            {
                gamepad.Update();
                gamepad.DebugPrintButtonStates(0);
                
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
                {
                    debugMode = false;
                }
                
                Sleep(100);
            }
            
            std::cout << "Exited debug mode." << std::endl;
            Sleep(500); // Prevent rapid triggering
        }
        
        // Display gamepad states
        for (int i = 0; i < gamepad.GetDeviceCount(); ++i)
        {
            DisplayGamepadState(gamepad, i);
            
            // Check for button press events
            if (gamepad.WasPressed(i, GamepadHandler::GamepadInput::BUTTON_A))
                std::cout << "\nA button pressed on device " << i << "!" << std::endl;
            if (gamepad.WasPressed(i, GamepadHandler::GamepadInput::BUTTON_START))
                std::cout << "\nStart button pressed on device " << i << "!" << std::endl;
        }
        
        Sleep(16); // ~60 FPS update rate
    }
    
    std::cout << "\n\nShutting down..." << std::endl;
    
    // Cleanup is automatic through destructor
    DestroyWindow(hwnd);
    UnregisterClass(L"GamepadExample", GetModuleHandle(nullptr));
    
    return 0;
}