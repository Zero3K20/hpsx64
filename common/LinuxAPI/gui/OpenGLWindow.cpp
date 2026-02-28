// OpenGLWindow.cpp - Linux/SDL2+OpenGL implementation

#include "OpenGLWindow.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>

// Font candidate paths shared with ConsoleWindow
static const char* s_menubar_fonts[] = {
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf",
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    nullptr
};

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

bool OpenGLWindow::Create(int width, int height, SizeMode mode)
{
    // Set OpenGL attributes before window creation (required by SDL2)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    return Window::Create(width, height, mode);
}

OpenGLWindow::~OpenGLWindow()
{
    if (m_menu_font) {
        TTF_CloseFont(m_menu_font);
        m_menu_font = nullptr;
    }
    if (m_gl_context) {
        SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
        if (m_texture_id)   { glDeleteTextures(1, &m_texture_id);   m_texture_id   = 0; }
        if (m_menubar_tex)  { glDeleteTextures(1, &m_menubar_tex);  m_menubar_tex  = 0; }
        SDL_GL_DeleteContext(m_gl_context);
        m_gl_context = nullptr;
    }
}

bool OpenGLWindow::PostCreateInitialize()
{
    if (!m_sdl_window) return false;

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
    InitMenuFont();
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

// ---------------------------------------------------------------------------
// Menu-bar rendering helpers
// ---------------------------------------------------------------------------

void OpenGLWindow::InitMenuFont()
{
    if (!TTF_WasInit()) TTF_Init();
    for (int i = 0; s_menubar_fonts[i]; ++i) {
        m_menu_font = TTF_OpenFont(s_menubar_fonts[i], 13); // 13-pt matches MENU_BAR_H
        if (m_menu_font) break;
    }
    m_menubar_dirty = true;
}

// Build an SDL_Surface representing the menu bar and upload it to m_menubar_tex.
void OpenGLWindow::BuildMenuBarTexture()
{
    if (!m_sdl_window) return;

    int win_w, win_h;
    SDL_GetWindowSize(m_sdl_window, &win_w, &win_h);
    if (win_w <= 0) return;

    const int H = MENU_BAR_H;

    // Create an RGBA surface for the menu bar.
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, win_w, H, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;

    // Dark background matching the Windows-style menu bar.
    SDL_FillRect(surf, nullptr, SDL_MapRGBA(surf->format, 45, 45, 48, 255));

    m_menu_header_rects.clear();

    // Draw bottom separator line.
    SDL_Rect sep = {0, H - 1, win_w, 1};
    SDL_FillRect(surf, &sep, SDL_MapRGBA(surf->format, 80, 80, 80, 255));

    int x = 8;
    const auto& order = GetMenuOrder();
    // Only iterate top-level menus (those NOT containing '|').
    for (const auto& name : order) {
        if (name.find('|') != std::string::npos) continue;
        std::string label = GetMenuDisplayText(name);
        if (label.empty()) continue;

        if (m_menu_font) {
            bool hovered = (name == m_hovered_menu);
            SDL_Color fg = hovered ? SDL_Color{255,255,255,255} : SDL_Color{220,220,220,255};
            SDL_Surface* txt = TTF_RenderUTF8_Blended(m_menu_font, label.c_str(), fg);
            if (txt) {
                if (hovered) {
                    // Highlight background for hovered item.
                    SDL_Rect bg = {x - 4, 1, txt->w + 8, H - 2};
                    SDL_FillRect(surf, &bg, SDL_MapRGBA(surf->format, 80, 80, 80, 255));
                }
                SDL_Rect dst = {x, (H - txt->h) / 2, txt->w, txt->h};
                SDL_BlitSurface(txt, nullptr, surf, &dst);
                SDL_Rect hr = {x - 4, 0, txt->w + 8, H};
                m_menu_header_rects.push_back({name, hr});
                x += txt->w + 16;
                SDL_FreeSurface(txt);
            }
        } else {
            // No font: draw a grey placeholder box; width estimated at 8px per char.
            static const int FALLBACK_CHAR_W = 8;
            static const int FALLBACK_PAD    = 4;
            int boxw = (int)label.size() * FALLBACK_CHAR_W + FALLBACK_PAD * 2;
            SDL_Rect box = {x, 3, boxw, H - 6};
            SDL_FillRect(surf, &box, SDL_MapRGBA(surf->format, 80, 80, 80, 255));
            m_menu_header_rects.push_back({name, box});
            x += boxw + 8;
        }
    }

    // Upload surface to OpenGL texture.
    if (!m_menubar_tex) glGenTextures(1, &m_menubar_tex);
    glBindTexture(GL_TEXTURE_2D, m_menubar_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_w, H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_menubar_tex_w = win_w;
    m_menubar_tex_h = H;
    SDL_FreeSurface(surf);
    m_menubar_dirty = false;
}

// Render the menu bar as a GL quad overlaid at the top of the window.
void OpenGLWindow::RenderMenuBarGL()
{
    if (m_menubar_dirty || !m_menubar_tex)
        BuildMenuBarTexture();
    if (!m_menubar_tex || !m_menubar_tex_w) return;

    int win_w, win_h;
    SDL_GetWindowSize(m_sdl_window, &win_w, &win_h);
    if (win_w <= 0 || win_h <= 0) return;

    // Switch to orthographic projection covering window pixels.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, win_w, win_h, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, m_menubar_tex);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0,             0);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(m_menubar_tex_w, 0);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(m_menubar_tex_w, m_menubar_tex_h);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0,             m_menubar_tex_h);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
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

    // Overlay the menu bar on top of the game frame.
    RenderMenuBarGL();

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

    // Handle mouse clicks on the rendered menu bar (top MENU_BAR_H pixels).
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        if (my < MENU_BAR_H) {
            for (const auto& hr : m_menu_header_rects) {
                if (mx >= hr.rect.x && mx < hr.rect.x + hr.rect.w) {
                    // Highlight the clicked menu header and show keyboard shortcuts.
                    m_hovered_menu = hr.name;
                    m_menubar_dirty = true;
                    // Forward to base-class F1 handler to show the help dialog.
                    SDL_Event fake;
                    fake.type = SDL_KEYDOWN;
                    fake.key.keysym.sym  = SDLK_F1;
                    fake.key.keysym.mod  = KMOD_NONE;
                    fake.key.keysym.scancode = SDL_SCANCODE_F1;
                    fake.key.windowID = event.button.windowID;
                    Window::HandleEvent(fake);
                    m_hovered_menu.clear();
                    m_menubar_dirty = true;
                    return true;
                }
            }
        }
    }

    // Rebuild menu bar texture when window is resized.
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        m_menubar_dirty = true;
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
