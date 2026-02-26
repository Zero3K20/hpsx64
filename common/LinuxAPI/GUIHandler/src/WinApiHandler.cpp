/*
	Copyright (C) 2012-2030

    Linux/SDL2 implementation of WinApiHandler.cpp
*/

#include "WinApiHandler.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace WindowClass
{

Debug::Log debug;
volatile u32 NextIndex = StartId;

// Static ListView buffers
WindowClass::ListView::LVCOLUMN WindowClass::ListView::lvc = {};
WindowClass::ListView::LVITEM WindowClass::ListView::lvi = {};

// Static member definitions for Window
volatile unsigned long Window::Busy = 0;
volatile unsigned long long Window::LastResult = 0;
volatile unsigned long Window::ReadIndex = 0;
volatile unsigned long Window::WriteIndex = 0;
volatile Window::RemoteCallData Window::RemoteCall_Buffer[Window::c_RemoteCallBufferSize] = {};
vector<Window::Event*> Window::EventList;
volatile u32 Window::LastKeyPressed = 0;
volatile u32 Window::InModalMenuLoop = 0;
int Window::NextShortcutKeyID = 0;
vector<Window::ShortcutKey_Entry> Window::ShortcutKey_Entries;
long Window::GUIThread_isRunning = 0;
void* Window::GUIThread = nullptr;

// Current SDL event queue
static SDL_Event s_sdl_event;

void DoEvents()
{
    while (SDL_PollEvent(&s_sdl_event)) {
        if (s_sdl_event.type == SDL_QUIT) {
            // Signal quit
            Window::LastKeyPressed = 27; // ESC
        }
        if (s_sdl_event.type == SDL_KEYDOWN) {
            Window::LastKeyPressed = (u32)s_sdl_event.key.keysym.scancode;
        }
        // Handle menu shortcuts
        if (s_sdl_event.type == SDL_KEYDOWN) {
            for (const auto& sk : Window::ShortcutKey_Entries) {
                if ((unsigned int)s_sdl_event.key.keysym.sym == sk.key) {
                    if (sk.pFunc) sk.pFunc(0);
                }
            }
        }
    }
}

void DoEventsNoWait()
{
    DoEvents();
}

void DoSingleEvent()
{
    if (SDL_PollEvent(&s_sdl_event)) {
        if (s_sdl_event.type == SDL_QUIT)
            Window::LastKeyPressed = 27;
        if (s_sdl_event.type == SDL_KEYDOWN)
            Window::LastKeyPressed = (u32)s_sdl_event.key.keysym.scancode;
    }
}


// MenuBar static members
vector<MenuBar*> MenuBar::ListOfMenuBars;
vector<MenuBar::MenuBarItem*> MenuBar::ListOfMenuBarItems;
static map<string, MenuBar::Function> s_menu_functions;

void MenuBar::add_map_function(const char* sName, Function pfunc_ptr) {
    s_menu_functions[sName] = pfunc_ptr;
}

MenuBar::Function MenuBar::get_map_function(const char* sName) {
    auto it = s_menu_functions.find(sName);
    return (it != s_menu_functions.end()) ? it->second : nullptr;
}

MenuBar* MenuBar::CreateMenuBar(HWND _hWnd) {
    MenuBar* mb = new MenuBar();
    mb->hWnd = _hWnd;
    ListOfMenuBars.push_back(mb);
    return mb;
}

MenuBar* MenuBar::GetMenuBarForWindow(HWND _hWnd) {
    for (auto* mb : ListOfMenuBars)
        if (mb->hWnd == _hWnd) return mb;
    return nullptr;
}

MenuBar* MenuBar::FindMenuBarById(u32 _Id) {
    for (auto* mb : ListOfMenuBars)
        if (mb->Id == _Id) return mb;
    return nullptr;
}

MenuBar::MenuBarItem* MenuBar::FindMenuBarItemById(u32 _Id) {
    for (auto* item : ListOfMenuBarItems)
        if (item->MenuBarId == _Id) return item;
    return nullptr;
}

void MenuBar::AddItem(const char* sText, u32 Id, u32 Flags) {
    MenuBarItem* item = new MenuBarItem();
    item->Text = sText;
    item->MenuBarId = Id;
    item->Flags = Flags;
    item->ParentId = 0;
    ListOfMenuBarItems_Top.push_back(item);
    ListOfMenuBarItems.push_back(item);
}

void MenuBar::AddItemWithSubMenu(const char* sText, u32 Id) {
    MenuBarItem* item = new MenuBarItem();
    item->Text = sText;
    item->MenuBarId = Id;
    item->HasSubMenu = true;
    ListOfMenuBarItems_Top.push_back(item);
    ListOfMenuBarItems.push_back(item);
}

void MenuBar::AddSubItem(u32 ParentId, const char* sText, u32 Id, u32 Flags) {
    MenuBarItem* item = new MenuBarItem();
    item->Text = sText;
    item->MenuBarId = Id;
    item->ParentId = ParentId;
    item->Flags = Flags;
    MenuBarItem* parent = FindMenuBarItemById(ParentId);
    if (parent) parent->SubMenuItems.push_back(item);
    ListOfMenuBarItems.push_back(item);
}

void MenuBar::AddSeparator(u32 ParentId) {
    AddSubItem(ParentId, "---", 0, 0);
}

void MenuBar::CheckItem(u32 Id, bool bCheck) {
    MenuBarItem* item = FindMenuBarItemById(Id);
    if (item) item->Checked = bCheck;
}

void MenuBar::EnableItem(u32 Id, bool bEnable) {
    MenuBarItem* item = FindMenuBarItemById(Id);
    if (item) item->Enabled = bEnable;
}


// Window implementation
Window::Window()
    : hWnd(nullptr)
    , hGLRC(nullptr)
    , hDC(nullptr)
    , m_sdl_window(nullptr)
    , m_gl_context(nullptr)
    , m_is_fullscreen(false)
    , m_enabled(true)
    , m_texture_id(0)
    , m_windowed_x(SDL_WINDOWPOS_CENTERED)
    , m_windowed_y(SDL_WINDOWPOS_CENTERED)
    , m_windowed_w(640)
    , m_windowed_h(480)
    , m_created(false)
{
}

Window::~Window()
{
    KillWindow();
}

bool Window::CreateGLWindow(const char* title, int width, int height, bool /*has_menu*/, bool fullscreenflag)
{
    if (m_sdl_window) return true;

    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
            cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
            return false;
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (fullscreenflag) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    m_sdl_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags
    );

    if (!m_sdl_window) {
        cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    hWnd = (HWND)m_sdl_window;

    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if (!m_gl_context) {
        cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
        return false;
    }

    hGLRC = (HGLRC)m_gl_context;
    SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
    SDL_GL_SetSwapInterval(0);

    // Save windowed state
    m_is_fullscreen = fullscreenflag;
    SDL_GetWindowPosition(m_sdl_window, &m_windowed_x, &m_windowed_y);
    SDL_GetWindowSize(m_sdl_window, &m_windowed_w, &m_windowed_h);

    // Init OpenGL texture
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_created = true;
    return true;
}

bool Window::CreateBaseWindow(const char* title, int width, int height, bool has_menu)
{
    return CreateGLWindow(title, width, height, has_menu, false);
}

bool Window::CreateWindowWithMenu(const char* title, int width, int height, bool has_menu, bool fullscreenflag)
{
    return CreateGLWindow(title, width, height, has_menu, fullscreenflag);
}

bool Window::KillWindow()
{
    if (m_texture_id) {
        if (m_gl_context && m_sdl_window)
            SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
    }
    if (m_gl_context) {
        SDL_GL_DeleteContext(m_gl_context);
        m_gl_context = nullptr;
        hGLRC = nullptr;
    }
    if (m_sdl_window) {
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
        hWnd = nullptr;
    }
    m_created = false;
    return true;
}

bool Window::Destroy()
{
    return KillWindow();
}

bool Window::CreateOpenGLContext()
{
    if (!m_sdl_window) return false;
    if (m_gl_context) return true;
    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if (!m_gl_context) return false;
    hGLRC = (HGLRC)m_gl_context;
    return true;
}

bool Window::DestroyOpenGLContext()
{
    if (m_gl_context) {
        SDL_GL_DeleteContext(m_gl_context);
        m_gl_context = nullptr;
        hGLRC = nullptr;
    }
    return true;
}

void Window::KillGLWindow()
{
    DestroyOpenGLContext();
}

void Window::ToggleGLFullScreen()
{
    if (!m_sdl_window) return;
    if (!m_is_fullscreen) {
        SDL_GetWindowPosition(m_sdl_window, &m_windowed_x, &m_windowed_y);
        SDL_GetWindowSize(m_sdl_window, &m_windowed_w, &m_windowed_h);
        SDL_SetWindowFullscreen(m_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        m_is_fullscreen = true;
    } else {
        SDL_SetWindowFullscreen(m_sdl_window, 0);
        SDL_SetWindowPosition(m_sdl_window, m_windowed_x, m_windowed_y);
        SDL_SetWindowSize(m_sdl_window, m_windowed_w, m_windowed_h);
        m_is_fullscreen = false;
    }
}

bool Window::SetWindowSize(long width, long height)
{
    if (!m_sdl_window) return false;
    SDL_SetWindowSize(m_sdl_window, (int)width, (int)height);
    return true;
}

bool Window::GetWindowSize(long* width, long* height)
{
    if (!m_sdl_window) return false;
    int w, h;
    SDL_GetWindowSize(m_sdl_window, &w, &h);
    *width = w;
    *height = h;
    return true;
}

bool Window::GetViewableArea(long* width, long* height)
{
    return GetWindowSize(width, height);
}

bool Window::Redraw()
{
    if (m_sdl_window && m_gl_context)
        SDL_GL_SwapWindow(m_sdl_window);
    return true;
}

void Window::DisplayPixels(const unsigned char* pixels, int width, int height)
{
    if (!m_sdl_window || !m_gl_context || !pixels) return;
    SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);

    int win_w, win_h;
    SDL_GetWindowSize(m_sdl_window, &win_w, &win_h);

    glViewport(0, 0, win_w, win_h);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Window::SwapBuffers()
{
    if (m_sdl_window)
        SDL_GL_SwapWindow(m_sdl_window);
}

HWND Window::Create(const char* _lpWindowName, int _x, int _y, int _nWidth, int _nHeight,
                     DWORD /*_dwStyle*/, HMENU /*_hMenu*/, HWND /*_hWndParent*/,
                     void* /*_lpParam*/, HINSTANCE /*_hInstance*/, const char* /*_lpClassName*/)
{
    CreateGLWindow(_lpWindowName, _nWidth, _nHeight, false, false);
    if (m_sdl_window)
        SDL_SetWindowPosition(m_sdl_window, _x, _y);
    return hWnd;
}

HWND Window::CreateControl(const char* /*ClassName*/, int /*x*/, int /*y*/, int /*width*/, int /*height*/,
                             const char* /*Caption*/, int /*flags*/, HMENU /*hMenu*/)
{
    return nullptr;
}

bool Window::Clear() { return true; }
bool Window::ChangeTextColor(u32 /*color*/) { return true; }
bool Window::ChangeTextBkColor(u32 /*color*/) { return true; }
bool Window::ChangeBkMode(int /*iBkMode*/) { return true; }
void Window::Set_Font(HFONT /*_hFont*/) {}
int Window::Get_TextWidth(string /*text*/, UINT /*uFormat*/) { return 0; }
int Window::Get_TextHeight(string /*text*/, UINT /*uFormat*/) { return 0; }
bool Window::SetText(long /*x*/, long /*y*/, const char* /*text*/, u32 /*fontSize*/, UINT /*uFormat*/) { return true; }
bool Window::PrintText(long /*x*/, long /*LineNumber*/, const char* /*text*/, u32 /*fontSize*/, UINT /*uFormat*/) { return true; }
void Window::DrawPixel(int /*x*/, int /*y*/, COLORREF /*ColorRGB24*/) {}

bool Window::GetRequiredWindowSize(long* width, long* height, int /*hasMenu*/, long /*WindowStyle*/) {
    if (width) *width = 640;
    if (height) *height = 480;
    return true;
}

HFONT Window::CreateFontObject(int /*fontSize*/, const char* /*fontName*/,
                                bool /*Bold*/, bool /*Underline*/, bool /*Italic*/, bool /*StrikeOut*/)
{
    return nullptr;
}

void Window::OutputAllDisplayModes() {}

bool Window::AddEvent(HWND /*hParentWindow*/, HWND /*hControl*/, int /*CtrlId*/,
                       unsigned int /*message*/, EventFunction /*Func*/) { return true; }

bool Window::RemoveEvent(HWND /*hParentWindow*/, long long /*id*/, unsigned int /*message*/) { return true; }

void Window::AddShortcutKey(Function CallbackFunc, unsigned int key, unsigned int modifier)
{
    ShortcutKey_Entry entry;
    entry.pFunc = CallbackFunc;
    entry.key = key;
    entry.modifier = modifier;
    ShortcutKey_Entries.push_back(entry);
}

volatile int Window::Show_ContextMenu(int /*x*/, int /*y*/, string /*ContextMenu_Options*/) { return -1; }

unsigned long long Window::RemoteCall(RemoteFunction FunctionToCall, void* Parameters, bool /*WaitForReturnValue*/)
{
    // On Linux, just call directly (no separate GUI thread)
    if (FunctionToCall) return FunctionToCall(Parameters);
    return 0;
}

void Window::StartGUIThread() {}

int Window::StartWindowMessageLoop(void* /*Param*/) { return 0; }
int Window::WindowMessageLoop() { return 0; }

void Window::CreateMenuFromJson(json jsnMenu, const char* /*sLanguage*/)
{
    // Stub: parse menu JSON and register callbacks
    (void)jsnMenu;
}

void Window::CreateMenuFromYaml(const char* /*yaml_menu*/, const char* /*sLanguage*/) {}

} // namespace WindowClass
