/*
	Copyright (C) 2012-2030

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Linux/SDL2 replacement for common/WindowsAPI/GUIHandler/src/WinApiHandler.h
// Provides the same WindowClass namespace with SDL2-based implementations

//#pragma once
#ifndef __WINAPIHANDLER_H__
#define __WINAPIHANDLER_H__

#include "types.h"
#include "MultiThread.h"

#include "StringUtils.h"

// SDL2 replaces Windows headers
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <string>
#include <vector>
#include <string.h>
#include <functional>
#include <map>

#include "Debug.h"
#include "json.hpp"

using json = nlohmann::ordered_json;

// Platform-neutral type aliases
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMENU;
typedef void* HFONT;
typedef void* HGLRC;
typedef void* HDC;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned long long UINT_PTR;
typedef long LRESULT;
typedef long LPARAM;
typedef unsigned long long WPARAM;
typedef unsigned int COLORREF;
#define DT_SINGLELINE 0x20
#define DT_NOCLIP 0x100
#define DT_EXTERNALLEADING 0x200
#define DT_LEFT 0
#define DT_TOP 0
#define DT_CALCRECT 0x400
#define WM_COMMAND 0x0111
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101


namespace WindowClass
{
using namespace std;

    extern Debug::Log debug;

    // No-op registration on Linux (SDL2 doesn't need it)
    inline void Register(HINSTANCE /*hInstance*/, const char* /*lpszClassName*/ = "Default Class",
                         const char* /*lpszMenuName*/ = nullptr, void* /*lpfnWndProc*/ = nullptr,
                         UINT /*style*/ = 0, void* /*hbrBackground*/ = nullptr,
                         void* /*hIcon*/ = nullptr, void* /*hCursor*/ = nullptr,
                         int /*cbClsExtra*/ = 0, int /*cbWndExtra*/ = 0) {}

    void DoEvents();
    void DoEventsNoWait();
    void DoSingleEvent();

    static const u32 StartId = 9000;
    extern volatile u32 NextIndex;

    inline int GetCtrlIdFromHandle(HWND /*hCtrl*/) { return 0; }


    class MenuBar
    {
    public:
        typedef void (*Function)(int MenuItemId);

        static void add_map_function(const char* sName, Function pfunc_ptr);
        static Function get_map_function(const char* sName);

        class MenuBarItem
        {
        public:
            u32 MenuBarId;
            u32 ParentId;
            u32 Flags;
            string Text;
            Function pFunc;
            bool HasSubMenu;
            bool Checked;
            bool Enabled;
            vector<MenuBarItem*> SubMenuItems;

            MenuBarItem() : MenuBarId(0), ParentId(0), Flags(0), HasSubMenu(false),
                Checked(false), Enabled(true), pFunc(nullptr) {}
        };

        u32 Id;
        HWND hWnd;
        vector<MenuBarItem*> ListOfMenuBarItems_Top;

        MenuBar() : Id(0), hWnd(nullptr) {}

        void AddItem(const char* sText, u32 Id, u32 Flags = 0);
        void AddItemWithSubMenu(const char* sText, u32 Id);
        void AddSubItem(u32 ParentId, const char* sText, u32 Id, u32 Flags = 0);
        void AddSeparator(u32 ParentId);

        void CheckItem(u32 Id, bool bCheck = true);
        void EnableItem(u32 Id, bool bEnable = true);

        static vector<MenuBar*> ListOfMenuBars;
        static vector<MenuBarItem*> ListOfMenuBarItems;

        static MenuBar* CreateMenuBar(HWND _hWnd);
        static MenuBar* GetMenuBarForWindow(HWND _hWnd);
        static MenuBar* FindMenuBarById(u32 _Id);
        static MenuBarItem* FindMenuBarItemById(u32 _Id);
    };


    class Window
    {
    public:
        static const unsigned long DefaultStyle = 0;

        typedef void (*EventFunction)(HWND hCtrl, int idCtrl, unsigned int message, WPARAM wParam, LPARAM lParam);

        static volatile unsigned long Busy;
        static volatile unsigned long long LastResult;

        typedef unsigned long long (*RemoteFunction)(void* Params);

        struct RemoteCallData {
            unsigned long long Param;
            RemoteFunction FunctionToCall;
        };

        static const unsigned long c_RemoteCallBufferSize = 2048;
        static const unsigned long c_RemoteCallBufferMask = c_RemoteCallBufferSize - 1;
        static volatile unsigned long ReadIndex;
        static volatile unsigned long WriteIndex;
        static volatile RemoteCallData RemoteCall_Buffer[c_RemoteCallBufferSize];

        struct Event {
            HWND hwndParent;
            HWND hwndCtrl;
            long long id;
            unsigned int message;
            EventFunction ef;
            Event(HWND _hwndParent, HWND _hwndCtrl, int _id, unsigned int _message, EventFunction _ef)
                : hwndParent(_hwndParent), hwndCtrl(_hwndCtrl), id(_id), message(_message), ef(_ef) {}
        };

        static vector<Event*> EventList;

        void CreateMenuFromJson(json jsnMenu, const char* sLanguage);
        void CreateMenuFromYaml(const char* yaml_menu, const char* sLanguage);

        static void OutputAllDisplayModes();
        static bool AddEvent(HWND hParentWindow, HWND hControl, int CtrlId, unsigned int message, EventFunction Func);
        static bool RemoveEvent(HWND hParentWindow, long long id, unsigned int message);

        HWND CreateControl(const char* ClassName, int x, int y, int width, int height,
                           const char* Caption, int flags, HMENU hMenu = nullptr);

        HWND Create(const char* _lpWindowName, int _x, int _y, int _nWidth, int _nHeight,
                    DWORD _dwStyle = DefaultStyle, HMENU _hMenu = nullptr, HWND _hWndParent = nullptr,
                    void* _lpParam = nullptr, HINSTANCE _hInstance = nullptr,
                    const char* _lpClassName = nullptr);

        bool CreateBaseWindow(const char* title, int width, int height, bool has_menu);
        bool KillWindow();

        bool CreateOpenGLContext();
        bool DestroyOpenGLContext();
        void KillGLWindow();
        bool CreateWindowWithMenu(const char* title, int width, int height, bool has_menu, bool fullscreenflag);
        bool CreateGLWindow(const char* title, int width, int height, bool has_menu, bool fullscreenflag);
        void ToggleGLFullScreen();

        bool Redraw();

        static HFONT CreateFontObject(int fontSize, const char* fontName = "Times New Roman",
                                       bool Bold = false, bool Underline = false,
                                       bool Italic = false, bool StrikeOut = false);

        bool Destroy();

        bool Enable() { m_enabled = true; return true; }
        bool Disable() { m_enabled = false; return true; }
        bool DisableCloseButton() { return true; }

        bool SetWindowSize(long width, long height);

        HWND GetHandleToParent() { return nullptr; }
        static HWND GetHandleToParent(HWND /*hwnd*/) { return nullptr; }

        static volatile u32 LastKeyPressed;

        // Window handle
        HWND hWnd;

        // OpenGL context handle (SDL_GLContext cast to HGLRC)
        HGLRC hGLRC;
        HDC hDC;

        // SDL2 window pointer
        SDL_Window* m_sdl_window;
        SDL_GLContext m_gl_context;

        // Window state
        bool m_is_fullscreen;
        bool m_enabled;

        // Texture for pixel display
        GLuint m_texture_id;

        // For fullscreen toggle
        int m_windowed_x, m_windowed_y, m_windowed_w, m_windowed_h;

        typedef void (*Function)(int MenuItemId);

        void AddShortcutKey(Function CallbackFunc, unsigned int key, unsigned int modifier = 0);

        volatile int Show_ContextMenu(int x, int y, string ContextMenu_Options);

        bool Clear();
        bool ChangeTextColor(u32 color);
        bool ChangeTextBkColor(u32 color);
        bool ChangeBkMode(int iBkMode);
        void Set_Font(HFONT _hFont);
        int Get_TextWidth(string text, UINT uFormat = DT_SINGLELINE);
        int Get_TextHeight(string text, UINT uFormat = DT_SINGLELINE);
        bool SetText(long x, long y, const char* text, u32 fontSize = 8, UINT uFormat = DT_SINGLELINE);
        bool PrintText(long x, long LineNumber, const char* text, u32 fontSize = 8, UINT uFormat = DT_SINGLELINE);
        void DrawPixel(int x, int y, COLORREF ColorRGB24);
        bool GetWindowSize(long* width, long* height);
        bool GetViewableArea(long* width, long* height);
        inline bool SetCaption(const char* sCaption) {
            if (m_sdl_window) SDL_SetWindowTitle(m_sdl_window, sCaption);
            return true;
        }

        static bool GetRequiredWindowSize(long* width, long* height, int hasMenu = 1, long WindowStyle = DefaultStyle);

        static unsigned long long RemoteCall(RemoteFunction FunctionToCall, void* Parameters, bool WaitForReturnValue = true);

        static volatile u32 InModalMenuLoop;
        static int NextShortcutKeyID;

        struct ShortcutKey_Entry {
            Function pFunc;
            unsigned int key;
            unsigned int modifier;
        };
        static vector<ShortcutKey_Entry> ShortcutKey_Entries;

        static long GUIThread_isRunning;
        static void* GUIThread;  // was Api::Thread*

        static void StartGUIThread();
        static inline void WaitForModalMenuLoop() { while (InModalMenuLoop); }

        inline void OpenGL_MakeCurrent() {
            if (m_sdl_window && m_gl_context)
                SDL_GL_MakeCurrent(m_sdl_window, m_gl_context);
        }
        inline static void OpenGL_ReleaseWindow() { SDL_GL_MakeCurrent(nullptr, nullptr); }

        // Display pixel data via OpenGL
        void DisplayPixels(const unsigned char* pixels, int width, int height);
        void SwapBuffers();

        Window();
        ~Window();

    private:
        bool m_created;
        static int StartWindowMessageLoop(void* Param);
        static int WindowMessageLoop();
    };


    class Button
    {
    public:
        HWND hWnd;
        string Text;
        int x, y, width, height;

        Button() : hWnd(nullptr), x(0), y(0), width(0), height(0) {}

        bool Create(HWND hParent, const char* text, int _x, int _y, int _width, int _height,
                    HMENU /*hMenu*/ = nullptr) {
            x = _x; y = _y; width = _width; height = _height;
            Text = text;
            return true;
        }
        bool Destroy() { return true; }
        bool SetCaption(const char* caption) { Text = caption; return true; }
        bool Enable() { return true; }
        bool Disable() { return true; }
    };


    class Static
    {
    public:
        HWND hWnd;
        string Text;
        int x, y, width, height;

        Static() : hWnd(nullptr), x(0), y(0), width(0), height(0) {}

        bool Create(HWND /*hParent*/, const char* text, int _x, int _y, int _width, int _height,
                    HMENU /*hMenu*/ = nullptr) {
            x = _x; y = _y; width = _width; height = _height;
            Text = text;
            return true;
        }
        bool Destroy() { return true; }
        bool SetCaption(const char* caption) { Text = caption; return true; }
        bool SetText(const char* text) { Text = text; return true; }
        string GetText() { return Text; }
    };


    class Edit
    {
    public:
        HWND hWnd;
        string Text;
        int x, y, width, height;

        Edit() : hWnd(nullptr), x(0), y(0), width(0), height(0) {}

        bool Create(HWND /*hParent*/, int _x, int _y, int _width, int _height,
                    const char* text = "", HMENU /*hMenu*/ = nullptr) {
            x = _x; y = _y; width = _width; height = _height;
            Text = text;
            return true;
        }
        bool Destroy() { return true; }
        bool SetText(const char* text) { Text = text; return true; }
        string GetText() { return Text; }
        void AppendText(const char* text) { Text += text; }
        void Clear() { Text = ""; }
    };


    class ComboBox
    {
    public:
        HWND hWnd;
        vector<string> Items;
        int selection;
        int x, y, width, height;

        ComboBox() : hWnd(nullptr), selection(0), x(0), y(0), width(0), height(0) {}

        bool Create(HWND /*hParent*/, int _x, int _y, int _width, int _height,
                    HMENU /*hMenu*/ = nullptr) {
            x = _x; y = _y; width = _width; height = _height;
            return true;
        }
        bool Destroy() { return true; }
        void AddItem(const char* item) { Items.push_back(item); }
        void Clear() { Items.clear(); selection = 0; }
        int GetSelection() { return selection; }
        void SetSelection(int idx) { if (idx >= 0 && idx < (int)Items.size()) selection = idx; }
        string GetSelectedText() { return (selection < (int)Items.size()) ? Items[selection] : ""; }
    };


    class ListBox
    {
    public:
        HWND hWnd;
        vector<string> Items;
        int selection;
        int x, y, width, height;

        ListBox() : hWnd(nullptr), selection(-1), x(0), y(0), width(0), height(0) {}

        bool Create(HWND /*hParent*/, int _x, int _y, int _width, int _height,
                    HMENU /*hMenu*/ = nullptr) {
            x = _x; y = _y; width = _width; height = _height;
            return true;
        }
        bool Destroy() { return true; }
        void AddItem(const char* item) { Items.push_back(item); }
        void Clear() { Items.clear(); selection = -1; }
        int GetSelection() { return selection; }
        void SetSelection(int idx) { if (idx >= 0 && idx < (int)Items.size()) selection = idx; }
        string GetSelectedText() { return (selection >= 0 && selection < (int)Items.size()) ? Items[selection] : ""; }
        int GetCount() { return (int)Items.size(); }
    };

} // namespace WindowClass

#endif // __WINAPIHANDLER_H__
