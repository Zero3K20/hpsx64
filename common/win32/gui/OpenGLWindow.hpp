// OpenGLWindow.hpp
#pragma once

#include "Window.hpp"
#include <GL/gl.h>
#include <vector>

#include <mutex>

/**
 * @brief OpenGL-enabled window class
 * 
 * Provides OpenGL rendering context with support for pixel array display,
 * fullscreen toggle, VSync toggle, pixel scaling modes, and vertical flip.
 */
class OpenGLWindow : public Window {
public:
    /**
     * @brief Pixel scaling mode
     */
    enum class ScalingMode {
        NEAREST,    ///< Nearest neighbor (pixelated)
        LINEAR      ///< Linear interpolation (smooth)
    };

    /**
     * @brief Pixel display scaling behavior
     */
    enum class DisplayMode {
        STRETCH,        ///< Stretch to fill entire viewport (may distort aspect ratio)
        FIT,           ///< Scale to fit viewport while maintaining aspect ratio
        CENTER         ///< Display at original size, centered in viewport
    };

    /**
     * @brief Constructor
     * @param class_name The window class name
     * @param title The window title
     */
    OpenGLWindow(const std::string& class_name, const std::string& title);

    /**
     * @brief Destructor
     */
    virtual ~OpenGLWindow();

    /**
     * @brief Display a pixel array (RGBA8 format)
     * @param pixels Pointer to pixel data (width * height * 4 bytes)
     * @param width Pixel array width
     * @param height Pixel array height
     */
    void DisplayPixelArray(const unsigned char* pixels, int width, int height);

    /**
     * @brief Display pixel array scaled to viewable area with specified mode
     * @param pixels Pointer to pixel data (width * height * 4 bytes)
     * @param width Pixel array width
     * @param height Pixel array height
     * @param mode Display scaling mode
     */
    void DisplayPixelArrayScaled(const unsigned char* pixels, int width, int height, DisplayMode mode = DisplayMode::FIT);

    /**
     * @brief Toggle fullscreen mode
     * @return True if now fullscreen, false if windowed
     */
    bool ToggleFullscreen();

    /**
     * @brief Set fullscreen mode
     * @param fullscreen Whether to enable fullscreen
     * @return True if successful
     */
    bool SetFullscreen(bool fullscreen);

    /**
     * @brief Get current fullscreen state
     * @return True if fullscreen
     */
    bool IsFullscreen() const { return m_is_fullscreen; }

    /**
     * @brief Toggle VSync
     * @return True if VSync is now enabled
     */
    bool ToggleVSync();

    /**
     * @brief Set VSync enabled state
     * @param enabled Whether to enable VSync
     * @return True if successful
     */
    bool SetVSync(bool enabled);

    /**
     * @brief Get current VSync state
     * @return True if VSync is enabled
     */
    bool IsVSyncEnabled() const { return m_vsync_enabled; }

    /**
     * @brief Set pixel scaling mode
     * @param mode Scaling mode (nearest or linear)
     */
    void SetScalingMode(ScalingMode mode);

    /**
     * @brief Get current scaling mode
     * @return Current scaling mode
     */
    ScalingMode GetScalingMode() const { return m_scaling_mode; }

    /**
     * @brief Set display mode
     * @param mode Display mode (stretch, fit, or center)
     */
    void SetDisplayMode(DisplayMode mode) { m_display_mode = mode; }

    /**
     * @brief Get current display mode
     * @return Current display mode
     */
    DisplayMode GetDisplayMode() const { return m_display_mode; }

    /**
     * @brief Set vertical flip mode
     * @param flipped Whether to flip the display vertically
     */
    void SetVerticalFlip(bool flipped) { m_vertical_flip = flipped; }

    /**
     * @brief Toggle vertical flip
     * @return True if now flipped vertically
     */
    bool ToggleVerticalFlip() { 
        m_vertical_flip = !m_vertical_flip; 
        return m_vertical_flip;
    }

    /**
     * @brief Get current vertical flip state
     * @return True if display is flipped vertically
     */
    bool IsVerticallyFlipped() const { return m_vertical_flip; }

    /**
     * @brief Swap front and back buffers
     */
    void SwapBuffers();

    /**
     * @brief Clear the back buffer
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha component (0.0-1.0)
     */
    void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    /**
     * @brief Make this window's OpenGL context current
     * @return True if successful
     */
    bool MakeCurrent();

    /**
     * @brief Release the OpenGL context from current thread
     * @return True if successful
     */
    bool ReleaseCurrent();

    /**
     * @brief Check if context is current on calling thread
     * @return True if current
     */
    bool IsCurrentOnThisThread() const;

protected:
    /**
     * @brief Initialize OpenGL context after window creation
     * @return True if successful
     */
    bool PostCreateInitialize() override;

    /**
     * @brief Get window class style flags for OpenGL
     * @return Window class style flags including CS_OWNDC for OpenGL
     */
    UINT GetWindowClassStyle() const override { return CS_HREDRAW | CS_VREDRAW | CS_OWNDC; }

    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
    HDC m_device_context;
    HGLRC m_render_context;
    bool m_is_fullscreen;
    bool m_vsync_enabled;
    bool m_vertical_flip;
    ScalingMode m_scaling_mode;
    DisplayMode m_display_mode;
    GLuint m_texture_id;
    
    // Windowed mode properties
    RECT m_windowed_rect;
    DWORD m_windowed_style;
    DWORD m_windowed_ex_style;

#ifdef ENABLE_USE_GUI_MUTEX
    mutable std::mutex m_context_mutex;
#endif

    bool CreatePixelFormat();
    void UpdateViewport();
    void RenderTexturedQuad();
    void RenderScaledTexturedQuad(int pixel_width, int pixel_height, DisplayMode mode);
    
    // WGL extensions
    typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC m_wglSwapIntervalEXT;
};
