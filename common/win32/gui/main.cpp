// main.cpp - Updated to use Window base class menu functionality and file dialogs
#include "MenuWindow.hpp"
#include "OpenGLWindow.hpp"
#include <iostream>
#include <memory>
#include <cmath>
#include <cstring>
#include <fstream>

/**
 * @brief Demo application showing window functionality
 * 
 * Demonstrates normal window with controls and menus, plus OpenGL window
 * with pixel array display, fullscreen/vsync toggle, scaling modes, menus,
 * and file dialog functionality.
 */
class DemoApplication {
public:
    /**
     * @brief Constructor
     */
    DemoApplication() 
        : m_running(true)
        , m_pixel_data(nullptr)
        , m_pixel_width(256)
        , m_pixel_height(256)
        , m_frame_counter(0) {
        
        // Initialize pixel data
        m_pixel_data = new unsigned char[m_pixel_width * m_pixel_height * 4];
        GenerateTestPattern();
    }

    /**
     * @brief Destructor
     */
    ~DemoApplication() {
        delete[] m_pixel_data;
    }

    /**
     * @brief Initialize and run the application
     * @return Exit code
     */
    int Run() {
        if (!Initialize()) {
            return -1;
        }

        // Main loop using non-blocking message processing
        while (m_running) {
            bool control_msgs = m_control_window->ProcessMessages();
            bool opengl_msgs = m_opengl_window->ProcessMessages();
            
            if (!control_msgs || !opengl_msgs) {
                std::cout << "Message processing returned false, exiting..." << std::endl;
                m_running = false;
                break;
            }

            // Update and render OpenGL window
            UpdatePixelData();
            m_opengl_window->DisplayPixelArray(m_pixel_data, m_pixel_width, m_pixel_height);

            // Small delay to prevent 100% CPU usage
            Sleep(16); // ~60 FPS
        }

        return 0;
    }

private:
    bool m_running;
    std::unique_ptr<MenuWindow> m_control_window;
    std::unique_ptr<OpenGLWindow> m_opengl_window; // Using base OpenGLWindow with menu support

    unsigned char* m_pixel_data;
    int m_pixel_width;
    int m_pixel_height;
    int m_frame_counter;
    std::string m_current_config_file; // Track current configuration file

    /**
     * @brief Initialize windows and controls
     * @return True if successful
     */
    bool Initialize() {
        std::cout << "Initializing control window..." << std::endl;
        
        // Create control window with menu
        m_control_window = std::make_unique<MenuWindow>("DemoControlWindow", "Demo Control Panel");
        if (!m_control_window->Create(600, 500)) {
            std::cout << "Failed to create control window!" << std::endl;
            return false;
        }
        std::cout << "Control window created successfully." << std::endl;

        SetupControlWindow();
        SetupControlWindowMenus();

        std::cout << "Initializing OpenGL window..." << std::endl;
        
        // Create OpenGL window - now has menu support from base Window class
        m_opengl_window = std::make_unique<OpenGLWindow>("DemoOpenGLWindow", "OpenGL Demo - Press F11 for fullscreen");
        if (!m_opengl_window->Create(800, 600)) {
            std::cout << "Failed to create OpenGL window!" << std::endl;
            return false;
        }
        std::cout << "OpenGL window created successfully." << std::endl;

        SetupOpenGLWindowMenus(); // Setup menus directly on the OpenGL window

        // Show windows
        std::cout << "Showing windows..." << std::endl;
        m_control_window->Show();
        m_opengl_window->Show();
        std::cout << "Windows should now be visible." << std::endl;

        return true;
    }

    /**
     * @brief Setup control window with various controls
     */
    void SetupControlWindow() {
        int y_pos = 20;
        const int control_height = 25;
        const int spacing = 35;

        // Add buttons
        m_control_window->AddButton("test_button", "Test Button", 20, y_pos, 120, control_height,
            [this]() { 
                std::cout << "Test button clicked!" << std::endl;
                m_control_window->SetEditText("status_edit", "Button clicked!");
            });
        y_pos += spacing;

        m_control_window->AddButton("toggle_fullscreen", "Toggle Fullscreen", 20, y_pos, 120, control_height,
            [this]() {
                bool is_fullscreen = m_opengl_window->ToggleFullscreen();
                std::string status = is_fullscreen ? "Fullscreen ON" : "Fullscreen OFF";
                m_control_window->SetEditText("status_edit", status);
            });
        y_pos += spacing;

        m_control_window->AddButton("toggle_vsync", "Toggle VSync", 20, y_pos, 120, control_height,
            [this]() {
                bool vsync = m_opengl_window->ToggleVSync();
                std::string status = vsync ? "VSync ON" : "VSync OFF";
                m_control_window->SetEditText("status_edit", status);
            });
        y_pos += spacing;

        // Add file dialog buttons
        m_control_window->AddButton("load_config", "Load Config", 20, y_pos, 120, control_height,
            [this]() {
                LoadConfiguration();
            });
        
        m_control_window->AddButton("save_config", "Save Config", 150, y_pos, 120, control_height,
            [this]() {
                SaveConfiguration();
            });
        y_pos += spacing;

        // Add edit control
        m_control_window->AddEdit("status_edit", "Ready", 20, y_pos, 300, control_height);
        y_pos += spacing;

        // Add multi-line edit
        m_control_window->AddEdit("notes", "Multi-line notes...", 20, y_pos, 300, 80, true);
        y_pos += 90;

        // Add combo box
        m_control_window->AddComboBox("scaling_combo", 20, y_pos, 150, 100);
        m_control_window->AddComboItem("scaling_combo", "Nearest (Pixelated)");
        m_control_window->AddComboItem("scaling_combo", "Linear (Smooth)");
        m_control_window->SelectComboByIndex("scaling_combo", 0);
        y_pos += spacing;

        m_control_window->AddButton("apply_scaling", "Apply Scaling", 180, y_pos - spacing, 100, control_height,
            [this]() {
                std::string selected = m_control_window->GetComboSelectedText("scaling_combo");
                if (selected == "Nearest (Pixelated)") {
                    m_opengl_window->SetScalingMode(OpenGLWindow::ScalingMode::NEAREST);
                    m_control_window->SetEditText("status_edit", "Scaling: Nearest");
                } else if (selected == "Linear (Smooth)") {
                    m_opengl_window->SetScalingMode(OpenGLWindow::ScalingMode::LINEAR);
                    m_control_window->SetEditText("status_edit", "Scaling: Linear");
                }
            });

        // Add checkbox
        m_control_window->AddCheckbox("animate_check", "Animate Pattern", 20, y_pos, 150, control_height);
        m_control_window->SetCheckboxChecked("animate_check", true);
        y_pos += spacing;

        // Add list box
        m_control_window->AddListBox("pattern_list", 20, y_pos, 150, 80);
        m_control_window->AddListBoxItem("pattern_list", "Checkerboard");
        m_control_window->AddListBoxItem("pattern_list", "Gradient");
        m_control_window->AddListBoxItem("pattern_list", "Plasma");
        m_control_window->SelectListBoxByIndex("pattern_list", 0);
        y_pos += 90;

        // Add list view
        m_control_window->AddListView("info_view", 350, 20, 220, 200);
        m_control_window->AddListViewColumn("info_view", "Property", 100);
        m_control_window->AddListViewColumn("info_view", "Value", 100);
        
        m_control_window->AddListViewRow("info_view", {"Resolution", "256x256"});
        m_control_window->AddListViewRow("info_view", {"Format", "RGBA8"});
        m_control_window->AddListViewRow("info_view", {"FPS", "~60"});
        m_control_window->AddListViewRow("info_view", {"VSync", "OFF"});
    }

    /**
     * @brief Setup menus for control window
     */
    void SetupControlWindowMenus() {
        m_control_window->CreateMenuBar();

        // File menu
        auto file_menu = m_control_window->AddMenu("file", "File");
        m_control_window->AddMenuItem("file", "new", "New", "Ctrl+N", 
            [this]() {
                // Reset to default configuration
                m_control_window->SetEditText("status_edit", "New configuration");
                m_control_window->SetEditText("notes", "Multi-line notes...");
                m_control_window->SelectComboByIndex("scaling_combo", 0);
                m_control_window->SetCheckboxChecked("animate_check", true);
                m_control_window->SelectListBoxByIndex("pattern_list", 0);
                m_current_config_file.clear();
            });

        m_control_window->AddMenuItem("file", "open", "Open Configuration...", "Ctrl+O",
            [this]() {
                LoadConfiguration();
            });

        m_control_window->AddMenuItem("file", "save", "Save Configuration", "Ctrl+S",
            [this]() {
                if (m_current_config_file.empty()) {
                    SaveConfiguration();
                } else {
                    SaveConfigurationToFile(m_current_config_file);
                }
            });

        m_control_window->AddMenuItem("file", "saveas", "Save Configuration As...", "Ctrl+Shift+S",
            [this]() {
                SaveConfiguration();
            });

        m_control_window->AddMenuSeparator("file");

        m_control_window->AddMenuItem("file", "export_image", "Export Test Images...", "",
            [this]() {
                ExportTestImages();
            });

        m_control_window->AddMenuSeparator("file");

        m_control_window->AddMenuItem("file", "exit", "Exit", "Alt+F4",
            [this]() {
                m_running = false;
            });

        // View menu
        auto view_menu = m_control_window->AddMenu("view", "View");
        m_control_window->AddMenuItem("view", "fullscreen", "Fullscreen", "F11",
            [this]() {
                m_opengl_window->ToggleFullscreen();
            });
        m_control_window->AddMenuItem("view", "vsync", "VSync", "F12",
            [this]() {
                bool vsync = m_opengl_window->ToggleVSync();
                m_control_window->SetMenuItemChecked("vsync", vsync);
            });

        // Help menu
        auto help_menu = m_control_window->AddMenu("help", "Help");
        m_control_window->AddMenuItem("help", "about", "About", "",
            [this]() {
                m_control_window->SetEditText("status_edit", "Demo Application v1.0 - File Dialog Support");
            });
    }

    /**
     * @brief Setup menus for OpenGL window using base Window class functionality
     */
    void SetupOpenGLWindowMenus() {
        m_opengl_window->CreateMenuBar();

        // File menu
        auto file_menu = m_opengl_window->AddMenu("file", "File");
        m_opengl_window->AddMenuItem("file", "save_image", "Save Image As...", "Ctrl+S",
            [this]() {
                SaveScreenshot();
            });

        m_opengl_window->AddMenuItem("file", "export_multiple", "Export Multiple Images...", "",
            [this]() {
                ExportMultipleImages();
            });

        m_opengl_window->AddMenuSeparator("file");

        m_opengl_window->AddMenuItem("file", "load_images", "Load Reference Images...", "Ctrl+O",
            [this]() {
                LoadReferenceImages();
            });

        // View menu
        auto view_menu = m_opengl_window->AddMenu("view", "View");
        m_opengl_window->AddMenuItem("view", "fullscreen", "Toggle Fullscreen", "F11",
            [this]() {
                bool is_fullscreen = m_opengl_window->ToggleFullscreen();
                m_opengl_window->SetMenuItemChecked("fullscreen", is_fullscreen);
            });
        
        m_opengl_window->AddMenuItem("view", "vsync", "Toggle VSync", "F12",
            [this]() {
                bool vsync = m_opengl_window->ToggleVSync();
                m_opengl_window->SetMenuItemChecked("vsync", vsync);
            });

        m_opengl_window->AddMenuSeparator("view");

        // Display mode submenu
        auto display_submenu = m_opengl_window->AddSubmenu("view", "display", "Display Mode");
        m_opengl_window->AddMenuItem("display", "stretch", "Stretch", "",
            [this]() {
                m_opengl_window->SetDisplayMode(OpenGLWindow::DisplayMode::STRETCH);
                m_opengl_window->SetMenuItemChecked("stretch", true);
                m_opengl_window->SetMenuItemChecked("fit", false);
                m_opengl_window->SetMenuItemChecked("center", false);
            });
        m_opengl_window->AddMenuItem("display", "fit", "Fit", "",
            [this]() {
                m_opengl_window->SetDisplayMode(OpenGLWindow::DisplayMode::FIT);
                m_opengl_window->SetMenuItemChecked("stretch", false);
                m_opengl_window->SetMenuItemChecked("fit", true);
                m_opengl_window->SetMenuItemChecked("center", false);
            });
        m_opengl_window->AddMenuItem("display", "center", "Center", "",
            [this]() {
                m_opengl_window->SetDisplayMode(OpenGLWindow::DisplayMode::CENTER);
                m_opengl_window->SetMenuItemChecked("stretch", false);
                m_opengl_window->SetMenuItemChecked("fit", false);
                m_opengl_window->SetMenuItemChecked("center", true);
            });

        // Scaling submenu
        auto scaling_submenu = m_opengl_window->AddSubmenu("view", "scaling", "Scaling Mode");
        m_opengl_window->AddMenuItem("scaling", "nearest", "Nearest (Pixelated)", "",
            [this]() {
                m_opengl_window->SetScalingMode(OpenGLWindow::ScalingMode::NEAREST);
                m_opengl_window->SetMenuItemChecked("nearest", true);
                m_opengl_window->SetMenuItemChecked("linear", false);
            });
        m_opengl_window->AddMenuItem("scaling", "linear", "Linear (Smooth)", "",
            [this]() {
                m_opengl_window->SetScalingMode(OpenGLWindow::ScalingMode::LINEAR);
                m_opengl_window->SetMenuItemChecked("nearest", false);
                m_opengl_window->SetMenuItemChecked("linear", true);
            });

        // Tools menu
        auto tools_menu = m_opengl_window->AddMenu("tools", "Tools");
        m_opengl_window->AddMenuItem("tools", "reset_view", "Reset View", "Home",
            [this]() {
                m_opengl_window->SetDisplayMode(OpenGLWindow::DisplayMode::FIT);
                m_opengl_window->SetScalingMode(OpenGLWindow::ScalingMode::NEAREST);
                // Update checkmarks
                m_opengl_window->SetMenuItemChecked("fit", true);
                m_opengl_window->SetMenuItemChecked("stretch", false);
                m_opengl_window->SetMenuItemChecked("center", false);
                m_opengl_window->SetMenuItemChecked("nearest", true);
                m_opengl_window->SetMenuItemChecked("linear", false);
            });

        m_opengl_window->AddMenuSeparator("tools");
        
        m_opengl_window->AddMenuItem("tools", "screenshot", "Take Screenshot", "F9",
            [this]() {
                SaveScreenshot();
            });

        // Help menu
        auto help_menu = m_opengl_window->AddMenu("help", "Help");
        m_opengl_window->AddMenuItem("help", "controls", "Controls", "F1",
            [this]() {
                std::cout << "OpenGL Window Controls:" << std::endl;
                std::cout << "- F11: Toggle Fullscreen" << std::endl;
                std::cout << "- F12: Toggle VSync" << std::endl;
                std::cout << "- Home: Reset View" << std::endl;
                std::cout << "- F9: Take Screenshot" << std::endl;
                std::cout << "- Ctrl+S: Save Image" << std::endl;
                std::cout << "- Ctrl+O: Load Reference Images" << std::endl;
            });

        m_opengl_window->AddMenuItem("help", "about_gl", "About", "",
            [this]() {
                std::cout << "OpenGL Demo Window v1.0 - File Dialog Support" << std::endl;
                std::cout << "Pixel array rendering with scaling options and file I/O" << std::endl;
            });

        // Set initial menu states
        m_opengl_window->SetMenuItemChecked("fit", true);
        m_opengl_window->SetMenuItemChecked("nearest", true);
    }

    /**
     * @brief Load configuration using file dialog
     */
    void LoadConfiguration() {
        std::string filename = m_control_window->ShowOpenFileDialog(
            "Load Configuration",
            "Configuration Files\0*.cfg\0Text Files\0*.txt\0All Files\0*.*\0",
            "cfg"
        );

        if (!filename.empty()) {
            if (LoadConfigurationFromFile(filename)) {
                m_current_config_file = filename;
                m_control_window->SetEditText("status_edit", "Configuration loaded: " + filename);
                std::cout << "Configuration loaded from: " << filename << std::endl;
            } else {
                m_control_window->SetEditText("status_edit", "Failed to load configuration");
                std::cout << "Failed to load configuration from: " << filename << std::endl;
            }
        }
    }

    /**
     * @brief Save configuration using file dialog
     */
    void SaveConfiguration() {
        std::string filename = m_control_window->ShowSaveFileDialog(
            "Save Configuration",
            "Configuration Files\0*.cfg\0Text Files\0*.txt\0All Files\0*.*\0",
            "cfg",
            "",
            "demo_config.cfg"
        );

        if (!filename.empty()) {
            if (SaveConfigurationToFile(filename)) {
                m_current_config_file = filename;
                m_control_window->SetEditText("status_edit", "Configuration saved: " + filename);
                std::cout << "Configuration saved to: " << filename << std::endl;
            } else {
                m_control_window->SetEditText("status_edit", "Failed to save configuration");
                std::cout << "Failed to save configuration to: " << filename << std::endl;
            }
        }
    }

    /**
     * @brief Save screenshot using file dialog
     */
    void SaveScreenshot() {
        std::string filename = m_opengl_window->ShowSaveFileDialog(
            "Save Screenshot",
            "PNG Files\0*.png\0BMP Files\0*.bmp\0TGA Files\0*.tga\0All Files\0*.*\0",
            "png",
            "",
            "screenshot.png"
        );

        if (!filename.empty()) {
            // In a real application, you would implement actual screenshot saving
            std::cout << "Screenshot would be saved to: " << filename << std::endl;
            
            // For demonstration, create a simple text file with pixel data info
            std::ofstream file(filename + ".info");
            if (file.is_open()) {
                file << "Screenshot Info\n";
                file << "Dimensions: " << m_pixel_width << "x" << m_pixel_height << "\n";
                file << "Format: RGBA8\n";
                file << "Frame: " << m_frame_counter << "\n";
                file << "Pattern: " << m_control_window->GetListBoxSelectedText("pattern_list") << "\n";
                file.close();
                std::cout << "Screenshot info saved to: " << filename << ".info" << std::endl;
            }
        }
    }

    /**
     * @brief Export multiple test images
     */
    void ExportMultipleImages() {
        std::string directory = m_opengl_window->ShowSaveFileDialog(
            "Select Export Directory (filename will be ignored)",
            "Directory\0*\0",
            "",
            "",
            "export_directory"
        );

        if (!directory.empty()) {
            // Extract directory from the selected path
            size_t last_slash = directory.find_last_of("\\/");
            if (last_slash != std::string::npos) {
                directory = directory.substr(0, last_slash);
            }
            
            std::cout << "Would export multiple test images to directory: " << directory << std::endl;
            
            // Generate some example files
            const std::vector<std::string> patterns = {"checkerboard", "gradient", "plasma"};
            for (const auto& pattern : patterns) {
                std::string filename = directory + "\\" + pattern + "_test.info";
                std::ofstream file(filename);
                if (file.is_open()) {
                    file << "Test Image: " << pattern << "\n";
                    file << "Dimensions: " << m_pixel_width << "x" << m_pixel_height << "\n";
                    file << "Generated by Demo Application\n";
                    file.close();
                    std::cout << "Created: " << filename << std::endl;
                }
            }
        }
    }

    /**
     * @brief Export test images from control window
     */
    void ExportTestImages() {
        std::string filename = m_control_window->ShowSaveFileDialog(
            "Export Test Images",
            "Batch Files\0*.bat\0Text Files\0*.txt\0All Files\0*.*\0",
            "txt",
            "",
            "test_images_export.txt"
        );

        if (!filename.empty()) {
            std::ofstream file(filename);
            if (file.is_open()) {
                file << "Test Images Export Configuration\n";
                file << "====================================\n\n";
                file << "Current Settings:\n";
                file << "- Status: " << m_control_window->GetEditText("status_edit") << "\n";
                file << "- Scaling: " << m_control_window->GetComboSelectedText("scaling_combo") << "\n";
                file << "- Animation: " << (m_control_window->GetCheckboxChecked("animate_check") ? "Enabled" : "Disabled") << "\n";
                file << "- Pattern: " << m_control_window->GetListBoxSelectedText("pattern_list") << "\n";
                file << "- Notes: " << m_control_window->GetEditText("notes") << "\n";
                file << "\nFrame: " << m_frame_counter << "\n";
                file.close();
                
                m_control_window->SetEditText("status_edit", "Test images config exported");
                std::cout << "Test images configuration exported to: " << filename << std::endl;
            }
        }
    }

    /**
     * @brief Load reference images using multiple file selection
     */
    void LoadReferenceImages() {
        std::vector<std::string> filenames = m_opengl_window->ShowOpenMultipleFilesDialog(
            "Load Reference Images",
            "Image Files\0*.png;*.jpg;*.bmp;*.tga\0PNG Files\0*.png\0JPEG Files\0*.jpg\0BMP Files\0*.bmp\0All Files\0*.*\0"
        );

        if (!filenames.empty()) {
            std::cout << "Reference images selected:" << std::endl;
            for (size_t i = 0; i < filenames.size(); ++i) {
                std::cout << "- " << filenames[i] << std::endl;
                
                // In a real application, you would load and process these images
                // For demonstration, just show the filenames
            }
            
            std::cout << "Total files selected: " << filenames.size() << std::endl;
        } else {
            std::cout << "No reference images selected." << std::endl;
        }
    }

    /**
     * @brief Load configuration from file
     */
    bool LoadConfigurationFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "status") {
                    m_control_window->SetEditText("status_edit", value);
                } else if (key == "notes") {
                    m_control_window->SetEditText("notes", value);
                } else if (key == "scaling") {
                    if (value == "0") {
                        m_control_window->SelectComboByIndex("scaling_combo", 0);
                    } else if (value == "1") {
                        m_control_window->SelectComboByIndex("scaling_combo", 1);
                    }
                } else if (key == "animate") {
                    m_control_window->SetCheckboxChecked("animate_check", value == "1");
                } else if (key == "pattern") {
                    int pattern_index = std::stoi(value);
                    if (pattern_index >= 0 && pattern_index < 3) {
                        m_control_window->SelectListBoxByIndex("pattern_list", pattern_index);
                    }
                }
            }
        }
        
        file.close();
        return true;
    }

    /**
     * @brief Save configuration to file
     */
    bool SaveConfigurationToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << "status=" << m_control_window->GetEditText("status_edit") << "\n";
        file << "notes=" << m_control_window->GetEditText("notes") << "\n";
        
        std::string scaling = m_control_window->GetComboSelectedText("scaling_combo");
        file << "scaling=" << (scaling == "Nearest (Pixelated)" ? "0" : "1") << "\n";
        
        file << "animate=" << (m_control_window->GetCheckboxChecked("animate_check") ? "1" : "0") << "\n";
        
        std::string pattern = m_control_window->GetListBoxSelectedText("pattern_list");
        int pattern_index = 0;
        if (pattern == "Gradient") pattern_index = 1;
        else if (pattern == "Plasma") pattern_index = 2;
        file << "pattern=" << pattern_index << "\n";
        
        file.close();
        return true;
    }

    /**
     * @brief Generate initial test pattern
     */
    void GenerateTestPattern() {
        for (int y = 0; y < m_pixel_height; ++y) {
            for (int x = 0; x < m_pixel_width; ++x) {
                int index = (y * m_pixel_width + x) * 4;
                
                // Checkerboard pattern
                bool checker = ((x / 32) + (y / 32)) % 2 == 0;
                
                m_pixel_data[index + 0] = checker ? 255 : 64;  // Red
                m_pixel_data[index + 1] = checker ? 128 : 192; // Green
                m_pixel_data[index + 2] = checker ? 64 : 255;  // Blue
                m_pixel_data[index + 3] = 255;                 // Alpha
            }
        }
    }

    /**
     * @brief Update pixel data for animation
     */
    void UpdatePixelData() {
        if (!m_control_window->GetCheckboxChecked("animate_check")) {
            return;
        }

        std::string pattern = m_control_window->GetListBoxSelectedText("pattern_list");
        float time = m_frame_counter * 0.05f;
        
        for (int y = 0; y < m_pixel_height; ++y) {
            for (int x = 0; x < m_pixel_width; ++x) {
                int index = (y * m_pixel_width + x) * 4;
                
                if (pattern == "Gradient") {
                    float fx = static_cast<float>(x) / m_pixel_width;
                    float fy = static_cast<float>(y) / m_pixel_height;
                    
                    m_pixel_data[index + 0] = static_cast<unsigned char>(fx * 255);
                    m_pixel_data[index + 1] = static_cast<unsigned char>(fy * 255);
                    m_pixel_data[index + 2] = static_cast<unsigned char>((fx + fy) * 127);
                    m_pixel_data[index + 3] = 255;
                } else if (pattern == "Plasma") {
                    float fx = static_cast<float>(x) / m_pixel_width;
                    float fy = static_cast<float>(y) / m_pixel_height;
                    
                    float v1 = sin(fx * 10.0f + time);
                    float v2 = sin(fy * 10.0f + time);
                    float v3 = sin((fx + fy) * 10.0f + time);
                    float plasma = (v1 + v2 + v3) / 3.0f;
                    
                    unsigned char intensity = static_cast<unsigned char>((plasma + 1.0f) * 127.5f);
                    m_pixel_data[index + 0] = intensity;
                    m_pixel_data[index + 1] = static_cast<unsigned char>(intensity * 0.7f);
                    m_pixel_data[index + 2] = static_cast<unsigned char>(255 - intensity);
                    m_pixel_data[index + 3] = 255;
                } else {
                    // Animated checkerboard
                    int offset = static_cast<int>(time * 10) % 64;
                    bool checker = (((x + offset) / 32) + ((y + offset) / 32)) % 2 == 0;
                    
                    m_pixel_data[index + 0] = checker ? 255 : 64;
                    m_pixel_data[index + 1] = checker ? 128 : 192;
                    m_pixel_data[index + 2] = checker ? 64 : 255;
                    m_pixel_data[index + 3] = 255;
                }
            }
        }
        
        ++m_frame_counter;
    }
};

/**
 * @brief Application entry point
 * @param hInstance Application instance
 * @param hPrevInstance Previous instance (unused)
 * @param lpCmdLine Command line arguments
 * @param nCmdShow Show command
 * @return Exit code
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Enable console for debugging
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    
    std::cout << "Demo Application Starting..." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "- F11: Toggle fullscreen in OpenGL window" << std::endl;
    std::cout << "- F12: Toggle VSync in OpenGL window" << std::endl;
    std::cout << "- F9: Take screenshot (OpenGL window)" << std::endl;
    std::cout << "- Home: Reset view (OpenGL window)" << std::endl;
    std::cout << "- Ctrl+O: Open files (different in each window)" << std::endl;
    std::cout << "- Ctrl+S: Save files (different in each window)" << std::endl;
    std::cout << "- Use menus in both windows to access file dialogs" << std::endl;
    
    DemoApplication app;
    int result = app.Run();
    
    std::cout << "Demo Application Exiting..." << std::endl;
    FreeConsole();
    
    return result;
}