// OpenGLWindow.cpp
#include "OpenGLWindow.hpp"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")

OpenGLWindow::OpenGLWindow(const std::string& class_name, const std::string& title)
    : Window(class_name, title)
    , m_device_context(nullptr)
    , m_render_context(nullptr)
    , m_is_fullscreen(false)
    , m_vsync_enabled(false)
    , m_vertical_flip(false)
    , m_scaling_mode(ScalingMode::NEAREST)
    , m_display_mode(DisplayMode::STRETCH)
    , m_texture_id(0)
    , m_windowed_rect{}
    , m_windowed_style(0)
    , m_windowed_ex_style(0)
    , m_wglSwapIntervalEXT(nullptr) {
}

OpenGLWindow::~OpenGLWindow() {
    if (m_render_context) {
        // Make context current before deleting texture
        wglMakeCurrent(m_device_context, m_render_context);
        
        if (m_texture_id) {
            glDeleteTextures(1, &m_texture_id);
        }
        
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_render_context);
    }
    if (m_device_context) {
        ReleaseDC(m_hwnd, m_device_context);
    }
}

bool OpenGLWindow::PostCreateInitialize() {
    if (!m_hwnd) {
        return false;
    }

    m_device_context = GetDC(m_hwnd);
    if (!m_device_context) {
        return false;
    }

    if (!CreatePixelFormat()) {
        return false;
    }

    m_render_context = wglCreateContext(m_device_context);
    if (!m_render_context) {
        return false;
    }

    if (!wglMakeCurrent(m_device_context, m_render_context)) {
        return false;
    }

    // Load WGL extensions
    m_wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
        wglGetProcAddress("wglSwapIntervalEXT"));

    // Generate texture for pixel array display
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    // Set initial OpenGL state
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    UpdateViewport();

    SetScalingMode(m_scaling_mode);

    ReleaseCurrent();

    return true;
}

bool OpenGLWindow::ReleaseCurrent() {
#ifdef ENABLE_USE_GUI_MUTEX
    std::lock_guard<std::mutex> lock(m_context_mutex);
#endif
    return wglMakeCurrent(nullptr, nullptr) != FALSE;
}

bool OpenGLWindow::MakeCurrent() {
    if (!m_device_context || !m_render_context) {
        return false;
    }

#ifdef ENABLE_USE_GUI_MUTEX
    std::lock_guard<std::mutex> lock(m_context_mutex);
#endif
    return wglMakeCurrent(m_device_context, m_render_context) != FALSE;
}

bool OpenGLWindow::IsCurrentOnThisThread() const {
    return wglGetCurrentContext() == m_render_context;
}

void OpenGLWindow::DisplayPixelArray(const unsigned char* pixels, int width, int height) {
    DisplayPixelArrayScaled(pixels, width, height, m_display_mode);
}

void OpenGLWindow::DisplayPixelArrayScaled(const unsigned char* pixels, int width, int height, DisplayMode mode) {
    if (!pixels || !m_render_context) {
        return;
    }

    // Make this window's context current before rendering
    if (!MakeCurrent()) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    Clear();
    RenderScaledTexturedQuad(width, height, mode);
    SwapBuffers();

    // Optionally release context if called from worker thread
    ReleaseCurrent();
}

bool OpenGLWindow::ToggleFullscreen() {
    return SetFullscreen(!m_is_fullscreen);
}

bool OpenGLWindow::SetFullscreen(bool fullscreen) {
    if (m_is_fullscreen == fullscreen) {
        return true;
    }

    if (fullscreen) {
        // Save windowed mode properties
        GetWindowRect(m_hwnd, &m_windowed_rect);
        m_windowed_style = GetWindowLong(m_hwnd, GWL_STYLE);
        m_windowed_ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);

        // Get monitor info
        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &monitor_info);

        // Set fullscreen style
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLong(m_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);

        // Set fullscreen position and size
        SetWindowPos(m_hwnd, HWND_TOP,
                     monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.top,
                     monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        m_is_fullscreen = true;
    } else {
        // Restore windowed mode
        SetWindowLong(m_hwnd, GWL_STYLE, m_windowed_style);
        SetWindowLong(m_hwnd, GWL_EXSTYLE, m_windowed_ex_style);

        SetWindowPos(m_hwnd, HWND_NOTOPMOST,
                     m_windowed_rect.left,
                     m_windowed_rect.top,
                     m_windowed_rect.right - m_windowed_rect.left,
                     m_windowed_rect.bottom - m_windowed_rect.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        m_is_fullscreen = false;
    }

    // Make context current before updating viewport
    MakeCurrent();
    UpdateViewport();
    ReleaseCurrent();
    return true;
}

bool OpenGLWindow::ToggleVSync() {
    return SetVSync(!m_vsync_enabled);
}

bool OpenGLWindow::SetVSync(bool enabled) {
    if (!MakeCurrent()) {
        return false;
    }
    
    if (m_wglSwapIntervalEXT) {
        m_wglSwapIntervalEXT(enabled ? 1 : 0);
        m_vsync_enabled = enabled;
        ReleaseCurrent();
        return true;
    }
    ReleaseCurrent();
    return false;
}

void OpenGLWindow::SetScalingMode(ScalingMode mode) {
    m_scaling_mode = mode;
    
    if (m_texture_id) {
        // Make context current before modifying texture parameters
        if (MakeCurrent()) {
            glBindTexture(GL_TEXTURE_2D, m_texture_id);
            GLenum filter = (mode == ScalingMode::LINEAR) ? GL_LINEAR : GL_NEAREST;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

            ReleaseCurrent();
        }
    }
}

void OpenGLWindow::SwapBuffers() {
    if (m_device_context) {
        ::SwapBuffers(m_device_context);
    }
}

void OpenGLWindow::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

LRESULT OpenGLWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE:
            // Make context current before updating viewport
            if (MakeCurrent()) {
                UpdateViewport();
                ReleaseCurrent();
            }
            break;
        case WM_KEYDOWN:
            if (wparam == VK_F11) {
                ToggleFullscreen();
                return 0;
            }
            break;
        default:
            return Window::HandleMessage(msg, wparam, lparam);
    }
    return 0;
}

bool OpenGLWindow::CreatePixelFormat() {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(m_device_context, &pfd);
    if (!pixel_format) {
        return false;
    }

    return SetPixelFormat(m_device_context, pixel_format, &pfd) != FALSE;
}

void OpenGLWindow::UpdateViewport() {
    if (!m_render_context) {
        return;
    }

    RECT client_rect;
    GetClientRect(m_hwnd, &client_rect);
    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLWindow::RenderTexturedQuad() {
    // Determine texture coordinates based on vertical flip setting
    float tex_top = m_vertical_flip ? 1.0f : 0.0f;
    float tex_bottom = m_vertical_flip ? 0.0f : 1.0f;
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, tex_bottom); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, tex_bottom); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, tex_top); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, tex_top); glVertex2f(0.0f, 1.0f);
    glEnd();
}

void OpenGLWindow::RenderScaledTexturedQuad(int pixel_width, int pixel_height, DisplayMode mode) {
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float viewport_width = static_cast<float>(viewport[2]);
    float viewport_height = static_cast<float>(viewport[3]);

    if (viewport_width <= 0 || viewport_height <= 0) {
        return;
    }

    float quad_left, quad_right, quad_bottom, quad_top;

    switch (mode) {
        case DisplayMode::STRETCH:
            // Fill entire viewport (original behavior)
            quad_left = 0.0f;
            quad_right = 1.0f;
            quad_bottom = 0.0f;
            quad_top = 1.0f;
            break;

        case DisplayMode::FIT:
            {
                // Scale to fit while maintaining aspect ratio
                float pixel_aspect = static_cast<float>(pixel_width) / static_cast<float>(pixel_height);
                float viewport_aspect = viewport_width / viewport_height;

                if (pixel_aspect > viewport_aspect) {
                    // Pixel array is wider relative to its height than viewport
                    // Fit to width, center vertically
                    quad_left = 0.0f;
                    quad_right = 1.0f;
                    float scale_y = viewport_aspect / pixel_aspect;
                    float center_y = 0.5f;
                    quad_bottom = center_y - scale_y * 0.5f;
                    quad_top = center_y + scale_y * 0.5f;
                } else {
                    // Pixel array is taller relative to its width than viewport
                    // Fit to height, center horizontally
                    quad_bottom = 0.0f;
                    quad_top = 1.0f;
                    float scale_x = pixel_aspect / viewport_aspect;
                    float center_x = 0.5f;
                    quad_left = center_x - scale_x * 0.5f;
                    quad_right = center_x + scale_x * 0.5f;
                }
            }
            break;

        case DisplayMode::CENTER:
            {
                // Display at original size, centered
                float scale_x = static_cast<float>(pixel_width) / viewport_width;
                float scale_y = static_cast<float>(pixel_height) / viewport_height;
                
                if (scale_x > 1.0f || scale_y > 1.0f) {
                    // If pixel array is larger than viewport, fall back to FIT mode
                    RenderScaledTexturedQuad(pixel_width, pixel_height, DisplayMode::FIT);
                    return;
                }

                float center_x = 0.5f;
                float center_y = 0.5f;
                quad_left = center_x - scale_x * 0.5f;
                quad_right = center_x + scale_x * 0.5f;
                quad_bottom = center_y - scale_y * 0.5f;
                quad_top = center_y + scale_y * 0.5f;
            }
            break;
    }

    // Determine texture coordinates based on vertical flip setting
    float tex_top = m_vertical_flip ? 1.0f : 0.0f;
    float tex_bottom = m_vertical_flip ? 0.0f : 1.0f;

    // Render textured quad with calculated coordinates
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, tex_bottom); glVertex2f(quad_left, quad_bottom);
        glTexCoord2f(1.0f, tex_bottom); glVertex2f(quad_right, quad_bottom);
        glTexCoord2f(1.0f, tex_top); glVertex2f(quad_right, quad_top);
        glTexCoord2f(0.0f, tex_top); glVertex2f(quad_left, quad_top);
    glEnd();
}