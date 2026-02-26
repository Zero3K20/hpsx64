// GamepadConfigWindow.cpp
#include "GamepadConfigWindow.hpp"
#include <sstream>
#include <iomanip>
#include <windowsx.h>
#include <algorithm>
#include <cmath>

// Control name constants
const std::string GamepadConfigWindow::BUTTON_LIST = "button_list";
const std::string GamepadConfigWindow::AXIS_LIST = "axis_list";
const std::string GamepadConfigWindow::STATUS_TEXT = "status_text";
const std::string GamepadConfigWindow::BTN_OK = "btn_ok";
const std::string GamepadConfigWindow::BTN_CANCEL = "btn_cancel";
const std::string GamepadConfigWindow::BTN_RESET = "btn_reset";

GamepadConfigWindow::GamepadConfigWindow(const std::string& title, 
                                       const GamepadConfig& initialConfig,
                                       ConfigCallback callback,
                                       GamepadHandler& gamepadHandler,
                                       int deviceIndex,
                                       const std::vector<std::string>& deviceNames)
    : MenuWindow("GamepadConfigWindow", title)
    , m_initialConfig(initialConfig)
    , m_currentConfig(initialConfig)
    , m_callback(callback)
    , m_gamepad(gamepadHandler)
    , m_lastUpdateTime(0)
    , m_dialogAccepted(false)
    , m_customDeviceNames(deviceNames) {
    
    // Set the device index to configure
    m_currentConfig.deviceIndex = deviceIndex;
}

GamepadConfigWindow::~GamepadConfigWindow() {
    // Don't call m_gamepad.Shutdown() since we don't own the GamepadHandler
}

bool GamepadConfigWindow::ShowModal() {
    if (!Create(700, 500, SizeMode::CLIENT_AREA)) {
        return false;
    }
    
    Show(SW_SHOW);
    
    // Modal message loop
    MSG msg;
    
    while (IsWindow(m_hwnd)) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Update input detection
        UpdateInputDetection();
        
        // Small sleep to prevent excessive CPU usage
        Sleep(1);
    }
    
    return m_dialogAccepted;  // Return whether dialog was accepted
}

bool GamepadConfigWindow::PostCreateInitialize() {
    // GamepadHandler is already initialized by caller - no need to initialize here
    
    InitializeUI();
    //CreateDefaultMapping();
    UpdateDeviceList();
    UpdateButtonList();
    UpdateAxisList();
    UpdateStatusText("Hold a button/axis and double-click a mapping to configure");
    
    // Set up a timer for regular updates
    SetTimer(m_hwnd, 1, 16, nullptr); // ~60 FPS update rate
    
    return true;
}

void GamepadConfigWindow::InitializeUI() {
    // Title
    AddEdit("title_label", "Gamepad Configuration", 20, 10, 660, 25, false);
    
    CreateButtonConfigSection();
    CreateAnalogConfigSection();
    
    // Bottom buttons
    AddButton(BTN_OK, "OK", 450, 420, 80, 30, [this]() { OnOKButton(); });
    AddButton(BTN_CANCEL, "Cancel", 540, 420, 80, 30, [this]() { OnCancelButton(); });
    AddButton(BTN_RESET, "Reset Default", 20, 420, 100, 30, [this]() { OnResetButton(); });
    
    // Status bar
    AddEdit(STATUS_TEXT, "", 20, 460, 660, 25, false);
    
    // Make all label controls read-only
    MakeControlReadOnly("title_label");
    MakeControlReadOnly("device_label");
    MakeControlReadOnly("button_label");
    MakeControlReadOnly("axis_label");
}

void GamepadConfigWindow::CreateButtonConfigSection() {
    // Show which device we're configuring
    std::string deviceText = "Configuring Device " + std::to_string(m_currentConfig.deviceIndex);
    if (m_currentConfig.deviceIndex < static_cast<int>(m_customDeviceNames.size()) && !m_customDeviceNames[m_currentConfig.deviceIndex].empty()) {
        deviceText = "Configuring " + m_customDeviceNames[m_currentConfig.deviceIndex];
    }
    AddEdit("device_label", deviceText, 20, 50, 400, 20, false);
    
    // Button configuration
    AddEdit("button_label", "Button Mappings (Double-click while holding button):", 20, 80, 400, 20, false);
    AddListView(BUTTON_LIST, 20, 100, 320, 300);
    AddListViewColumn(BUTTON_LIST, "Button", 120);
    AddListViewColumn(BUTTON_LIST, "Mapped To", 80);
    AddListViewColumn(BUTTON_LIST, "Status", 80);
}

void GamepadConfigWindow::CreateAnalogConfigSection() {
    int startX = 360;
    
    // Analog configuration
    AddEdit("axis_label", "Analog Mappings (Double-click while moving stick/trigger):", startX, 80, 320, 20, false);
    AddListView(AXIS_LIST, startX, 100, 320, 300);
    AddListViewColumn(AXIS_LIST, "Control", 120);
    AddListViewColumn(AXIS_LIST, "Axis Index", 80);
    AddListViewColumn(AXIS_LIST, "Current Value", 80);
}

void GamepadConfigWindow::UpdateDeviceList() {
    m_gamepad.RefreshDevices();
    UpdateDeviceDisplay();
}

void GamepadConfigWindow::UpdateButtonList() {
    HWND listView = FindControl(BUTTON_LIST);
    if (listView) {
        ListView_DeleteAllItems(listView);
    }
    
    // Button list - restored L2/R2 buttons
    const std::vector<std::string> buttons = {
        "Circle", "X", "Square", "Triangle", "Start", "Select", 
        "L1", "L2", "R1", "R2", "L3", "R3"
    };
    
    for (const auto& buttonName : buttons) {
        std::vector<std::string> rowData;
        rowData.push_back(buttonName);
        
        auto it = m_currentConfig.buttonMappings.find(buttonName);
        if (it != m_currentConfig.buttonMappings.end()) {
            rowData.push_back(std::to_string(it->second));
            rowData.push_back("Mapped");
        } else {
            rowData.push_back("-1");
            rowData.push_back("Not Mapped");
        }
        
        AddListViewRow(BUTTON_LIST, rowData);
    }
}

void GamepadConfigWindow::UpdateAxisList() {
    HWND listView = FindControl(AXIS_LIST);
    if (!listView) return;

    // Check if we need to rebuild the list (first time or after config change)
    int itemCount = ListView_GetItemCount(listView);
    bool needsRebuild = (itemCount != 6); // Should have 6 axis rows

    if (needsRebuild) {
        // Full rebuild - delete and recreate all items
        ListView_DeleteAllItems(listView);

        // Axis mappings - restored trigger axes
        const std::vector<std::pair<std::string, int*>> axisControls = {
            {"Left Stick X", &m_currentConfig.leftStickXAxis},
            {"Left Stick Y", &m_currentConfig.leftStickYAxis},
            {"Right Stick X", &m_currentConfig.rightStickXAxis},
            {"Right Stick Y", &m_currentConfig.rightStickYAxis},
            {"Left Trigger", &m_currentConfig.leftTriggerAxis},
            {"Right Trigger", &m_currentConfig.rightTriggerAxis}
        };

        for (const auto& axis : axisControls) {
            std::vector<std::string> rowData;
            rowData.push_back(axis.first);
            rowData.push_back(std::to_string(*axis.second));
            rowData.push_back("-");
            AddListViewRow(AXIS_LIST, rowData);
        }
    }

    // Update axis index and current value columns
    int deviceIndex = GetSelectedDevice();
    if (deviceIndex < 0) return;

    DIJOYSTATE2 state = m_gamepad.GetRawDeviceState(deviceIndex);

    // Axis mappings in same order as above
    const std::vector<int*> axisPointers = {
        &m_currentConfig.leftStickXAxis,
        &m_currentConfig.leftStickYAxis,
        &m_currentConfig.rightStickXAxis,
        &m_currentConfig.rightStickYAxis,
        &m_currentConfig.leftTriggerAxis,
        &m_currentConfig.rightTriggerAxis
    };

    for (int row = 0; row < static_cast<int>(axisPointers.size()); row++) {
        int axisIndex = *axisPointers[row];

        // Update column 1 (axis index) in case it changed
        std::string axisIndexStr = std::to_string(axisIndex);
        ListView_SetItemText(listView, row, 1, const_cast<LPSTR>(axisIndexStr.c_str()));

        LONG rawValue = 0;

        switch (axisIndex) {
        case 0: rawValue = state.lX; break;
        case 1: rawValue = state.lY; break;
        case 2: rawValue = state.lZ; break;
        case 3: rawValue = state.lRx; break;
        case 4: rawValue = state.lRy; break;
        case 5: rawValue = state.lRz; break;
        }

        // Normalize from 0-256 range (center ~127) to -1.0 to +1.0 range
        float value = (static_cast<float>(rawValue) - 127.5f) / 127.5f;
        std::string valueStr = FormatFloat(std::abs(value));

        // Update column 2 (current value) to avoid flickering
        ListView_SetItemText(listView, row, 2, const_cast<LPSTR>(valueStr.c_str()));
    }
}

void GamepadConfigWindow::ChangeDevice(int offset) {
    int deviceCount = m_gamepad.GetDeviceCount();
    if (deviceCount == 0) return;
    
    int newIndex = m_currentConfig.deviceIndex + offset;
    if (newIndex < 0) newIndex = deviceCount - 1;
    if (newIndex >= deviceCount) newIndex = 0;
    
    m_currentConfig.deviceIndex = newIndex;
    UpdateDeviceDisplay();
    UpdateStatusText("Changed to device " + std::to_string(newIndex));
}

void GamepadConfigWindow::UpdateDeviceDisplay() {
    int deviceCount = m_gamepad.GetDeviceCount();
    
    if (deviceCount == 0) {
        SetEditText("device_display", "No devices");
        return;
    }
    
    std::string displayText;
    int deviceIndex = m_currentConfig.deviceIndex;
    
    // Use custom name if provided, otherwise use generic format
    if (deviceIndex < static_cast<int>(m_customDeviceNames.size()) && !m_customDeviceNames[deviceIndex].empty()) {
        displayText = m_customDeviceNames[deviceIndex];
    } else {
        displayText = "Device " + std::to_string(deviceIndex);
    }
    
    // Store the display name
    m_currentConfig.deviceName = displayText;
    
    // Update the display
    SetEditText("device_display", displayText);
}

void GamepadConfigWindow::OnResetButton() {
    CreateDefaultMapping();
    UpdateButtonList();
    UpdateAxisList();
    UpdateStatusText("Reset to default configuration");
}

void GamepadConfigWindow::OnOKButton() {
    UpdateStatusText("OK button clicked - validating configuration...");
    
    if (ValidateConfiguration()) {
        UpdateStatusText("Configuration valid - calling callback with TRUE");
        m_dialogAccepted = true;  // Set accepted flag
        if (m_callback) {
            m_callback(m_currentConfig, true);  // Explicitly passing true for accepted
        }
        DestroyWindow(m_hwnd);
    } else {
        UpdateStatusText("Configuration validation failed");
    }
}

void GamepadConfigWindow::OnCancelButton() {
    m_dialogAccepted = false;  // Set cancelled flag
    if (m_callback) {
        m_callback(m_initialConfig, false);
    }
    DestroyWindow(m_hwnd);
}

void GamepadConfigWindow::OnButtonListDoubleClick() {
    int deviceIndex = GetSelectedDevice();
    if (deviceIndex < 0) {
        UpdateStatusText("No valid device selected");
        return;
    }
    
    // Get selected button
    std::vector<std::string> selectedTexts = GetListViewSelectedTexts(BUTTON_LIST);
    if (selectedTexts.empty()) {
        UpdateStatusText("No button selected");
        return;
    }
    
    std::string buttonName = selectedTexts[0];
    
    // Check for held button
    int heldButton = GetHeldButton();
    if (heldButton >= 0) {
        // Map the button
        m_currentConfig.buttonMappings[buttonName] = heldButton;
        UpdateButtonList();
        UpdateStatusText("Mapped " + buttonName + " to button " + std::to_string(heldButton));
    } else {
        UpdateStatusText("Hold a button on the gamepad and try again");
    }
}

void GamepadConfigWindow::OnAxisListDoubleClick() {
    UpdateStatusText("Axis double-click detected!");
    
    int deviceIndex = GetSelectedDevice();
    if (deviceIndex < 0) {
        UpdateStatusText("No valid device selected");
        return;
    }
    
    // Get selected axis from the ListView
    std::vector<std::string> selectedTexts = GetListViewSelectedTexts(AXIS_LIST);
    std::string axisName;
    
    if (!selectedTexts.empty()) {
        axisName = selectedTexts[0];
    } else {
        // If nothing selected, try to determine from mouse position or just use first item
        HWND listView = FindControl(AXIS_LIST);
        if (listView && ListView_GetItemCount(listView) > 0) {
            // Get the first item as fallback
            TCHAR buffer[256];
            ListView_GetItemText(listView, 0, 0, buffer, sizeof(buffer) / sizeof(TCHAR));
            
            // Convert TCHAR to std::string
#ifdef _UNICODE
            // Unicode build - convert from wide to narrow
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
            if (bufferSize > 0) {
                std::vector<char> narrowBuffer(bufferSize);
                int result = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, narrowBuffer.data(), bufferSize, nullptr, nullptr);
                if (result > 0) {
                    axisName = narrowBuffer.data();
                }
            }
#else
            // ANSI build - direct assignment
            axisName = buffer;
#endif
            UpdateStatusText("No selection, using first axis: " + axisName);
        }
    }
    
    if (axisName.empty()) {
        UpdateStatusText("Could not determine which axis to configure");
        return;
    }
    
    UpdateStatusText("Configuring axis: " + axisName);
    
    // Find the axis with the largest absolute value (most movement)
    int bestAxis = -1;
    float largestValue = 0.0f;
    
    for (const auto& axis : m_lastAxisValues) {
        float absValue = std::abs(axis.second);
        if (absValue > largestValue) {
            largestValue = absValue;
            bestAxis = axis.first;
        }
    }
    
    // Debug output - show all current axis values
    std::string debugMsg = "Axis values: ";
    for (const auto& axis : m_lastAxisValues) {
        debugMsg += "A" + std::to_string(axis.first) + "=" + FormatFloat(axis.second, 2) + " ";
    }
    debugMsg += " | Largest: A" + std::to_string(bestAxis) + "(" + FormatFloat(largestValue, 2) + ")";
    
    if (bestAxis >= 0 && largestValue > AXIS_MOVEMENT_THRESHOLD) {
        // Map the axis - restored trigger axis handling
        if (axisName == "Left Stick X") m_currentConfig.leftStickXAxis = bestAxis;
        else if (axisName == "Left Stick Y") m_currentConfig.leftStickYAxis = bestAxis;
        else if (axisName == "Right Stick X") m_currentConfig.rightStickXAxis = bestAxis;
        else if (axisName == "Right Stick Y") m_currentConfig.rightStickYAxis = bestAxis;
        else if (axisName == "Left Trigger") m_currentConfig.leftTriggerAxis = bestAxis;
        else if (axisName == "Right Trigger") m_currentConfig.rightTriggerAxis = bestAxis;
        
        UpdateAxisList();
        UpdateStatusText("SUCCESS: Mapped " + axisName + " to axis " + std::to_string(bestAxis) + 
                        " (value: " + FormatFloat(m_lastAxisValues[bestAxis], 2) + ")");
    } else {
        UpdateStatusText("FAILED: " + debugMsg + " - Move stick/trigger more (need >" + 
                        FormatFloat(AXIS_MOVEMENT_THRESHOLD, 1) + ")!");
    }
}

void GamepadConfigWindow::UpdateInputDetection() {
    int deviceIndex = GetSelectedDevice();
    if (deviceIndex < 0) {
        UpdateStatusText("Invalid device index: " + std::to_string(deviceIndex));
        return;
    }

    // Make sure to update the gamepad state
    m_gamepad.Update();

    // Check device count
    if (deviceIndex >= m_gamepad.GetDeviceCount()) {
        UpdateStatusText("Device index out of range: " + std::to_string(deviceIndex) + " >= " + std::to_string(m_gamepad.GetDeviceCount()));
        return;
    }

    // Store current button states for comparison
    std::vector<int> currentPressed = m_gamepad.GetAllPressedButtonIndices(deviceIndex);
    m_lastPressedButtons = currentPressed;

    // Debug: Show if any buttons are pressed
    if (!currentPressed.empty()) {
        std::string buttonList = "Buttons pressed: ";
        for (int btn : currentPressed) {
            buttonList += std::to_string(btn) + " ";
        }
        UpdateStatusText(buttonList);
    }

    // Get raw axis values - they're in 0-256 range with center around 127-128
    // Normalize to -1.0 to +1.0 range for proper bidirectional detection
    DIJOYSTATE2 state = m_gamepad.GetRawDeviceState(deviceIndex);

    auto normalizeAxis = [](LONG value) -> float {
        // Normalize from 0-256 range (center ~127) to -1.0 to +1.0 range
        return (static_cast<float>(value) - 127.5f) / 127.5f;
        };

    m_lastAxisValues[0] = normalizeAxis(state.lX);
    m_lastAxisValues[1] = normalizeAxis(state.lY);
    m_lastAxisValues[2] = normalizeAxis(state.lZ);
    m_lastAxisValues[3] = normalizeAxis(state.lRx);
    m_lastAxisValues[4] = normalizeAxis(state.lRy);
    m_lastAxisValues[5] = normalizeAxis(state.lRz);
}

int GamepadConfigWindow::GetHeldButton() {
    if (!m_lastPressedButtons.empty()) {
        return m_lastPressedButtons[0]; // Return first held button
    }
    return -1;
}

int GamepadConfigWindow::GetActiveAxis() {
    // Find axis with significant movement (>0.7 from center for better detection)
    for (const auto& axis : m_lastAxisValues) {
        if (std::abs(axis.second) > 0.7f) {
            return axis.first;
        }
    }
    
    // Also check for trigger-like behavior (positive values above threshold)
    for (const auto& axis : m_lastAxisValues) {
        if (axis.second > 0.8f) { // Triggers often go from -1 to 1, with 1 being fully pressed
            return axis.first;
        }
    }
    
    return -1;
}

void GamepadConfigWindow::CreateDefaultMapping() {
    m_currentConfig.buttonMappings.clear();
    
    // Default mapping for common controllers (restored L2/R2)
    m_currentConfig.buttonMappings["Circle"] = 2;
    m_currentConfig.buttonMappings["X"] = 1;
    m_currentConfig.buttonMappings["Triangle"] = 3;
    m_currentConfig.buttonMappings["Square"] = 0;
    m_currentConfig.buttonMappings["L1"] = 4;
    m_currentConfig.buttonMappings["R1"] = 5;
    m_currentConfig.buttonMappings["Select"] = 8;
    m_currentConfig.buttonMappings["Start"] = 9;
    m_currentConfig.buttonMappings["L3"] = 10;
    m_currentConfig.buttonMappings["R3"] = 11;
    m_currentConfig.buttonMappings["L2"] = 6; // May vary by controller
    m_currentConfig.buttonMappings["R2"] = 7; // May vary by controller
    
    // Default axis mapping (keep triggers for internal use)
    m_currentConfig.leftStickXAxis = 0;
    m_currentConfig.leftStickYAxis = 1;
    m_currentConfig.rightStickXAxis = 2;
    m_currentConfig.rightStickYAxis = 5;
    m_currentConfig.leftTriggerAxis = 3;
    m_currentConfig.rightTriggerAxis = 4;
}

bool GamepadConfigWindow::ValidateConfiguration() {
    // Just check that we have some mappings
    return !m_currentConfig.buttonMappings.empty();
}

void GamepadConfigWindow::UpdateStatusText(const std::string& text) {
    SetEditText(STATUS_TEXT, text);
}

std::string GamepadConfigWindow::GetAxisName(int axisIndex) {
    switch (axisIndex) {
        case 0: return "X";
        case 1: return "Y";
        case 2: return "Z";
        case 3: return "RX";
        case 4: return "RY";
        case 5: return "RZ";
        default: return "?";
    }
}

std::string GamepadConfigWindow::FormatFloat(float value, int precision) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

int GamepadConfigWindow::GetSelectedDevice() const {
    return m_currentConfig.deviceIndex;
}

void GamepadConfigWindow::MakeControlReadOnly(const std::string& controlName) {
    HWND hwnd = FindControl(controlName);
    if (hwnd) {
        // Add ES_READONLY style to make the edit control read-only
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style |= ES_READONLY;
        SetWindowLong(hwnd, GWL_STYLE, style);
        
        // Optional: Change background color to indicate it's read-only
        // This makes it visually clear that the control is not editable
        SendMessage(hwnd, EM_SETREADONLY, TRUE, 0);
    }
}

LRESULT GamepadConfigWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_COMMAND:
            {
                UINT id = LOWORD(wparam);
                HWND hwnd_control = reinterpret_cast<HWND>(lparam);
                
                // Handle button clicks by checking control handles directly
                if (hwnd_control == FindControl(BTN_OK)) {
                    OnOKButton();
                    return 0;
                }
                else if (hwnd_control == FindControl(BTN_CANCEL)) {
                    OnCancelButton();
                    return 0;
                }
                else if (hwnd_control == FindControl(BTN_RESET)) {
                    OnResetButton();
                    return 0;
                }
            }
            break;
        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lparam;
                if (pnmh->code == NM_DBLCLK || pnmh->code == LVN_ITEMACTIVATE) {
                    if (pnmh->hwndFrom == FindControl(BUTTON_LIST)) {
                        OnButtonListDoubleClick();
                        return 0;
                    } else if (pnmh->hwndFrom == FindControl(AXIS_LIST)) {
                        // For axis list, we'll determine the axis based on the clicked item
                        LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lparam;
                        if (pnmia->iItem >= 0) {
                            // Select the clicked item first
                            ListView_SetItemState(FindControl(AXIS_LIST), pnmia->iItem, 
                                                LVIS_SELECTED | LVIS_FOCUSED, 
                                                LVIS_SELECTED | LVIS_FOCUSED);
                        }
                        OnAxisListDoubleClick();
                        return 0;
                    }
                }
            }
            break;
        case WM_TIMER:
            if (wparam == 1) {
                UpdateInputDetection();
                // Update axis display values
                UpdateAxisList();
            }
            break;
        case WM_DESTROY:
            KillTimer(m_hwnd, 1);
            break;
        case WM_CLOSE:
            OnCancelButton();
            return 0;
    }
    
    return MenuWindow::HandleMessage(msg, wparam, lparam);
}