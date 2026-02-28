// ConsoleWindow.hpp - Linux/SDL2 debug console window
//
// On Windows, WinMain calls AllocConsole() + RedirectIOToConsole() which opens
// a second console window that shows all cout/cerr debug output.  This class
// replicates that behaviour on Linux/Steam Deck: it creates a second SDL2
// window (software renderer + SDL2_ttf text) and intercepts std::cout and
// std::cerr so that all logging is visible even when launched from Steam or a
// file manager with no attached terminal.

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <streambuf>
#include <mutex>

// Opaque forward declaration to avoid pulling SDL_ttf.h into every TU that
// includes this header.
typedef struct _TTF_Font TTF_Font;

// ---------------------------------------------------------------------------
// ConsoleWindow - second SDL2 window showing captured debug output
// ---------------------------------------------------------------------------
class ConsoleWindow {
public:
    ConsoleWindow();
    ~ConsoleWindow();

    // Create the window (call after SDL_Init).  Returns true on success.
    bool Create(const std::string& title = "Debug Console");

    // Process console-window-specific events and redraw if needed.
    // Call this once per frame from the main game loop / DoEvents().
    void Update();

    // Close and release all SDL resources.
    void Destroy();

    bool IsOpen()    const { return m_window != nullptr; }
    Uint32 GetWindowID() const { return m_win_id; }

    // Thread-safe: append a line of text to the console display.
    void AppendLine(const std::string& line);

    // Handle a raw SDL event that belongs to this window (called by the
    // global event dispatcher when a WINDOWEVENT is received for m_win_id).
    void HandleWindowEvent(const SDL_WindowEvent& we);

private:
    static const int MAX_LINES = 2000;
    static const int FONT_SIZE = 13;
    static const int PAD       = 4;     // pixels of padding

    SDL_Window*   m_window   = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font*     m_font     = nullptr;
    Uint32        m_win_id   = 0;
    int           m_line_h   = 16;

    std::vector<std::string> m_lines;
    std::mutex               m_mutex;
    bool                     m_dirty   = false;
    Uint32                   m_last_render_ms = 0;

    bool InitFont();
    void Render();

    // Ordered list of paths to try for a monospace TTF font.
    static const char* s_font_candidates[];
};

// ---------------------------------------------------------------------------
// ConsoleWindowBuf - custom std::streambuf that routes to ConsoleWindow
//
// Usage:
//   ConsoleWindow* win = ...;
//   ConsoleWindowBuf buf(win, std::cout.rdbuf()); // replaces cout's buffer
//   std::cout.rdbuf(&buf);                        // now cout → console window
// ---------------------------------------------------------------------------
class ConsoleWindowBuf : public std::streambuf {
public:
    // Takes a pointer to the ConsoleWindow AND the original rdbuf so that
    // output also reaches the original destination (terminal if any).
    ConsoleWindowBuf(ConsoleWindow* win, std::streambuf* original);

protected:
    int_type        overflow(int_type c) override;
    std::streamsize xsputn(const char* s, std::streamsize n) override;

private:
    ConsoleWindow* m_win;
    std::streambuf* m_original;   // also forward output to original sink
    std::string     m_line;
};

// ---------------------------------------------------------------------------
// Global console window instance (defined in ConsoleWindow.cpp).
// Set by hps1x64/hps2x64 WinMain; read by WinApiHandler.cpp DoEvents().
// ---------------------------------------------------------------------------
extern ConsoleWindow* g_console_window;
