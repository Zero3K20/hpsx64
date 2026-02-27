// OpenGLWindow.hpp - Linux/SDL2+OpenGL implementation
#pragma once

#include "Window.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <mutex>

/**
 * @brief OpenGL-enabled window class (Linux/SDL2 version)
 */
class OpenGLWindow : public Window {
public:
    enum class ScalingMode {
        NEAREST,
        LINEAR
    };

    enum class DisplayMode {
        STRETCH,
        FIT,
        CENTER
    };

    OpenGLWindow(const std::string& class_name, const std::string& title);
    virtual ~OpenGLWindow();

    bool Create(int width, int height, SizeMode mode = SizeMode::CLIENT_AREA) override;

    void DisplayPixelArray(const unsigned char* pixels, int width, int height);
    void DisplayPixelArrayScaled(const unsigned char* pixels, int width, int height,
                                  DisplayMode mode = DisplayMode::FIT);

    bool ToggleFullscreen();
    bool SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_is_fullscreen; }

    bool ToggleVSync();
    bool SetVSync(bool enabled);
    bool IsVSyncEnabled() const { return m_vsync_enabled; }

    void SetScalingMode(ScalingMode mode);
    ScalingMode GetScalingMode() const { return m_scaling_mode; }

    void SetDisplayMode(DisplayMode mode);
    DisplayMode GetDisplayMode() const { return m_display_mode; }

    void SetVerticalFlip(bool flip);
    bool GetVerticalFlip() const { return m_vertical_flip; }

    // For compatibility with Windows version
    void* GetDeviceContext() const { return nullptr; }
    void* GetRenderContext() const { return m_gl_context; }

protected:
    bool PostCreateInitialize() override;
    bool HandleEvent(const SDL_Event& event) override;
    Uint32 GetExtraWindowFlags() const override { return SDL_WINDOW_OPENGL; }

private:
    SDL_GLContext m_gl_context;
    bool m_is_fullscreen;
    bool m_vsync_enabled;
    bool m_vertical_flip;
    ScalingMode m_scaling_mode;
    DisplayMode m_display_mode;
    GLuint m_texture_id;

    // Saved windowed state for fullscreen toggle
    int m_windowed_x, m_windowed_y, m_windowed_w, m_windowed_h;

    std::mutex m_display_mutex;

    void InitTexture();
    void RenderTexture(int tex_w, int tex_h, DisplayMode mode);
    bool CreatePixelFormat();
};
