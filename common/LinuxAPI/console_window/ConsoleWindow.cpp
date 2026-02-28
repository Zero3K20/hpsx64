// ConsoleWindow.cpp - Linux/SDL2 debug console window implementation

#include "ConsoleWindow.hpp"
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------------------
// Global instance
// ---------------------------------------------------------------------------
ConsoleWindow* g_console_window = nullptr;

// ---------------------------------------------------------------------------
// Font candidate paths (tried in order on different Linux distros / Steam Deck)
// ---------------------------------------------------------------------------
const char* ConsoleWindow::s_font_candidates[] = {
    // Arch Linux / Steam Deck (SteamOS 3.x)
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    // Debian / Ubuntu
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    // Liberation Mono (Fedora / Ubuntu alternative)
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf",
    // Noto Mono
    "/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf",
    "/usr/share/fonts/noto/NotoMono-Regular.ttf",
    // Fira Mono
    "/usr/share/fonts/TTF/FiraMono-Regular.ttf",
    nullptr
};

// ---------------------------------------------------------------------------
// ConsoleWindow
// ---------------------------------------------------------------------------
ConsoleWindow::ConsoleWindow() {}

ConsoleWindow::~ConsoleWindow()
{
    Destroy();
}

bool ConsoleWindow::Create(const std::string& title)
{
    // Initialise SDL2_ttf if not already done.
    if (!TTF_WasInit()) {
        if (TTF_Init() != 0) {
            fprintf(stderr, "TTF_Init failed: %s (continuing without font rendering)\n", TTF_GetError());
            // Continue without font; the window will show a blank background
            // but text is still forwarded to the original streambuf (terminal).
        }
    }

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        700, 500,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) return false;
    m_win_id = SDL_GetWindowID(m_window);

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
    if (!m_renderer) { Destroy(); return false; }

    InitFont();
    Render();
    return true;
}

bool ConsoleWindow::InitFont()
{
    if (!TTF_WasInit()) return false;
    for (int i = 0; s_font_candidates[i]; ++i) {
        m_font = TTF_OpenFont(s_font_candidates[i], FONT_SIZE);
        if (m_font) {
            m_line_h = TTF_FontLineSkip(m_font);
            return true;
        }
    }
    return false;
}

void ConsoleWindow::AppendLine(const std::string& line)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lines.push_back(line);
    if ((int)m_lines.size() > MAX_LINES)
        m_lines.erase(m_lines.begin());
    m_dirty = true;
}

void ConsoleWindow::HandleWindowEvent(const SDL_WindowEvent& we)
{
    if (we.event == SDL_WINDOWEVENT_CLOSE)
        Destroy();
}

void ConsoleWindow::Update()
{
    if (!m_window) return;

    // Rate-limit rendering to ~10 Hz to save CPU.
    static const Uint32 RENDER_INTERVAL_MS = 100;
    Uint32 now = SDL_GetTicks();
    if (m_dirty || (now - m_last_render_ms) >= RENDER_INTERVAL_MS) {
        Render();
        m_last_render_ms = now;
        m_dirty = false;
    }
}

void ConsoleWindow::Render()
{
    if (!m_renderer) return;

    // Dark background (same look as Windows console).
    SDL_SetRenderDrawColor(m_renderer, 12, 12, 12, 255);
    SDL_RenderClear(m_renderer);

    if (m_font) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int win_w, win_h;
        SDL_GetWindowSize(m_window, &win_w, &win_h);

        int visible = win_h / m_line_h;
        int start   = (int)m_lines.size() - visible;
        if (start < 0) start = 0;

        SDL_Color col = {204, 204, 204, 255};  // light grey like a real console
        int y = PAD;
        for (int i = start; i < (int)m_lines.size(); ++i) {
            const std::string& txt = m_lines[i];
            if (txt.empty()) { y += m_line_h; continue; }
            // SDL_ttf can fail on some control characters; skip blank lines
            SDL_Surface* surf = TTF_RenderUTF8_Blended(m_font, txt.c_str(), col);
            if (!surf) { y += m_line_h; continue; }
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
            SDL_FreeSurface(surf);
            if (tex) {
                int tw, th;
                SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
                SDL_Rect dst = {PAD, y, tw, th};
                SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
            }
            y += m_line_h;
        }
    } else {
        // No font available: show a hint so the user knows output is captured.
        // (All text was still forwarded to stderr / the original streambuf.)
        SDL_SetRenderDrawColor(m_renderer, 80, 80, 80, 255);
        SDL_Rect hint = {PAD, PAD, 300, 20};
        SDL_RenderFillRect(m_renderer, &hint);
    }

    SDL_RenderPresent(m_renderer);
}

void ConsoleWindow::Destroy()
{
    if (m_font)     { TTF_CloseFont(m_font);          m_font     = nullptr; }
    if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }
    if (m_window)   { SDL_DestroyWindow(m_window);     m_window   = nullptr; }
    m_win_id = 0;
}

// ---------------------------------------------------------------------------
// ConsoleWindowBuf
// ---------------------------------------------------------------------------
ConsoleWindowBuf::ConsoleWindowBuf(ConsoleWindow* win, std::streambuf* original)
    : m_win(win), m_original(original)
{}

std::streambuf::int_type ConsoleWindowBuf::overflow(int_type c)
{
    if (c == EOF) return c;

    // Forward character to original sink (terminal if running from one).
    if (m_original) m_original->sputc((char)c);

    char ch = (char)c;
    if (ch == '\n' || ch == '\r') {
        if (ch == '\n') {
            if (m_win && m_win->IsOpen())
                m_win->AppendLine(m_line);
            m_line.clear();
        }
        // ignore bare '\r'
    } else {
        m_line += ch;
    }
    return c;
}

std::streamsize ConsoleWindowBuf::xsputn(const char* s, std::streamsize n)
{
    for (std::streamsize i = 0; i < n; ++i) overflow(s[i]);
    return n;
}
