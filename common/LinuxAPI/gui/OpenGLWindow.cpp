// OpenGLWindow.cpp - Linux/SDL2+OpenGL implementation

#include "OpenGLWindow.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

OpenGLWindow::OpenGLWindow(const std::string& class_name, const std::string& title)
    : Window(class_name, title)
    , m_gl_context(nullptr)
    , m_is_fullscreen(false)
    , m_vsync_enabled(false)
    , m_vertical_flip(false)
    , m_scaling_mode(ScalingMode::NEAREST)
    , m_display_mode(DisplayMode::STRETCH)
    , m_texture_id(0)
    , m_windowed_x(SDL_WINDOWPOS_CENTERED)
    , m_windowed_y(SDL_WINDOWPOS_CENTERED)
    , m_windowed_w(640)
    , m_windowed_h(480)
{
}

OpenGLWindow::~OpenGLWindow()
{
    if (m_gl_context) {
        if (m_texture_id) {
            // Make context current to delete texture
            SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        }
        SDL_GL_DeleteContext(m_gl_context);
        m_gl_context = nullptr;
    }
}

bool OpenGLWindow::PostCreateInitialize()
{
    if (!m_sdl_window) return false;

    // Set OpenGL attributes before creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if (!m_gl_context) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
    SDL_GL_SetSwapInterval(0); // VSync off by default

    // Save windowed state
    SDL_GetWindowPosition(m_sdl_window, &m_windowed_x, &m_windowed_y);
    SDL_GetWindowSize(m_sdl_window, &m_windowed_w, &m_windowed_h);

    InitTexture();
    return true;
}

void OpenGLWindow::InitTexture()
{
    if (m_texture_id) {
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    m_scaling_mode == ScalingMode::NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    m_scaling_mode == ScalingMode::NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLWindow::DisplayPixelArray(const unsigned char* pixels, int width, int height)
{
    DisplayPixelArrayScaled(pixels, width, height, m_display_mode);
}

void OpenGLWindow::DisplayPixelArrayScaled(const unsigned char* pixels, int width, int height, DisplayMode mode)
{
    if (!m_gl_context || !m_sdl_window || !pixels) return;

    std::lock_guard<std::mutex> lock(m_display_mutex);

    SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);

    // Upload texture
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    RenderTexture(width, height, mode);

    SDL_GL_SwapWindow(m_sdl_window);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLWindow::RenderTexture(int tex_w, int tex_h, DisplayMode mode)
{
    int win_w, win_h;
    SDL_GetWindowSize(m_sdl_window, &win_w, &win_h);

    glViewport(0, 0, win_w, win_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Calculate destination rect
    float x0 = -1.0f, y0 = -1.0f, x1 = 1.0f, y1 = 1.0f;

    if (mode == DisplayMode::FIT) {
        float scale_x = (float)win_w / tex_w;
        float scale_y = (float)win_h / tex_h;
        float scale = std::min(scale_x, scale_y);
        float dst_w = tex_w * scale;
        float dst_h = tex_h * scale;
        x0 = -dst_w / win_w;
        x1 =  dst_w / win_w;
        y0 = -dst_h / win_h;
        y1 =  dst_h / win_h;
    } else if (mode == DisplayMode::CENTER) {
        float dst_w = (float)tex_w / win_w;
        float dst_h = (float)tex_h / win_h;
        x0 = -dst_w;
        x1 =  dst_w;
        y0 = -dst_h;
        y1 =  dst_h;
    }

    // Vertical flip if needed
    float ty0 = m_vertical_flip ? 1.0f : 0.0f;
    float ty1 = m_vertical_flip ? 0.0f : 1.0f;

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, ty0); glVertex2f(x0, y1);
        glTexCoord2f(1.0f, ty0); glVertex2f(x1, y1);
        glTexCoord2f(1.0f, ty1); glVertex2f(x1, y0);
        glTexCoord2f(0.0f, ty1); glVertex2f(x0, y0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

bool OpenGLWindow::HandleEvent(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_f && (event.key.keysym.mod & KMOD_CTRL)) {
            ToggleFullscreen();
            return true;
        }
    }
    return Window::HandleEvent(event);
}

bool OpenGLWindow::ToggleFullscreen()
{
    return SetFullscreen(!m_is_fullscreen);
}

bool OpenGLWindow::SetFullscreen(bool fullscreen)
{
    if (!m_sdl_window) return false;

    if (fullscreen && !m_is_fullscreen) {
        SDL_GetWindowPosition(m_sdl_window, &m_windowed_x, &m_windowed_y);
        SDL_GetWindowSize(m_sdl_window, &m_windowed_w, &m_windowed_h);
        SDL_SetWindowFullscreen(m_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        m_is_fullscreen = true;
    } else if (!fullscreen && m_is_fullscreen) {
        SDL_SetWindowFullscreen(m_sdl_window, 0);
        SDL_SetWindowPosition(m_sdl_window, m_windowed_x, m_windowed_y);
        SDL_SetWindowSize(m_sdl_window, m_windowed_w, m_windowed_h);
        m_is_fullscreen = false;
    }
    return true;
}

bool OpenGLWindow::ToggleVSync()
{
    return SetVSync(!m_vsync_enabled);
}

bool OpenGLWindow::SetVSync(bool enabled)
{
    if (!m_gl_context) return false;
    SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
    int result = SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    if (result == 0) {
        m_vsync_enabled = enabled;
        return true;
    }
    return false;
}

void OpenGLWindow::SetScalingMode(ScalingMode mode)
{
    m_scaling_mode = mode;
    if (m_texture_id) {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        mode == ScalingMode::NEAREST ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        mode == ScalingMode::NEAREST ? GL_NEAREST : GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void OpenGLWindow::SetDisplayMode(DisplayMode mode)
{
    m_display_mode = mode;
}

void OpenGLWindow::SetVerticalFlip(bool flip)
{
    m_vertical_flip = flip;
}

bool OpenGLWindow::CreatePixelFormat()
{
    return true; // handled in PostCreateInitialize
}
