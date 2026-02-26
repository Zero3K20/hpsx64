/*
 * windows_compat.h - Linux compatibility layer for Windows-specific code
 * This file is force-included when building on Linux/Steam Deck.
 * It provides stubs and mappings for Windows APIs used in hpsx64.
 */

#pragma once

#ifndef _WIN32

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

// ---- Basic Windows type aliases ----
typedef void*           HWND;
typedef void*           HINSTANCE;
typedef void*           HANDLE;
typedef void*           HGLRC;
typedef void*           HDC;
typedef void*           HFONT;
typedef void*           HMENU;
typedef void*           HACCEL;
typedef void*           HBRUSH;
typedef void*           HICON;
typedef void*           HCURSOR;
typedef void*           LPVOID;
typedef char*           LPSTR;
typedef const char*     LPCSTR;
typedef const char*     LPCTSTR;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    UINT;
typedef unsigned long   DWORD;
typedef long            LONG;
typedef long long       LONGLONG;
typedef unsigned long long UINT_PTR;
typedef unsigned long long DWORD_PTR;
typedef long            LRESULT;
typedef long            LPARAM;
typedef unsigned long long WPARAM;
typedef unsigned int    COLORREF;
typedef int             BOOL;
typedef long long       LARGE_INTEGER;

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

#ifndef NULL
#define NULL nullptr
#endif

// ---- WinMain support ----
// On Linux, WinMain is mapped to main via this stub
#define WINAPI
#define CALLBACK

// Declare the WinMain function signature
extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                           LPSTR lpCmdLine, int iCmdShow);

// main() that calls WinMain()
// This will be provided by the Linux entry wrapper
// (defined in common/LinuxAPI/linux_main.cpp)

// ---- Console API stubs ----
#define STD_OUTPUT_HANDLE   ((DWORD)(-11))
#define STD_INPUT_HANDLE    ((DWORD)(-10))
#define STD_ERROR_HANDLE    ((DWORD)(-12))

struct COORD {
    short X;
    short Y;
};

struct SMALL_RECT {
    short Left, Top, Right, Bottom;
};

struct CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
};

inline HANDLE GetStdHandle(DWORD /*nStdHandle*/) { return nullptr; }
inline BOOL SetConsoleScreenBufferSize(HANDLE /*h*/, COORD /*s*/) { return TRUE; }
inline COORD GetLargestConsoleWindowSize(HANDLE /*h*/) { COORD c{300, 50}; return c; }
inline BOOL GetConsoleScreenBufferInfo(HANDLE /*h*/, CONSOLE_SCREEN_BUFFER_INFO* info) {
    if (info) { memset(info, 0, sizeof(*info)); info->dwMaximumWindowSize = {300, 50}; }
    return TRUE;
}
inline DWORD GetLastError() { return 0; }

// ---- Timer API stubs (winmm.lib equivalents) ----
#define TIMERR_NOERROR  0
#define TIMERR_NOCANDO  97

inline DWORD timeBeginPeriod(UINT /*uPeriod*/) { return TIMERR_NOERROR; }
inline DWORD timeEndPeriod(UINT /*uPeriod*/) { return TIMERR_NOERROR; }

inline DWORD timeGetTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// ---- Sleep ----
inline void Sleep(DWORD dwMilliseconds) {
    usleep(dwMilliseconds * 1000);
}

// ---- QueryPerformanceCounter / Frequency ----
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *lpPerformanceCount = (LARGE_INTEGER)(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    return TRUE;
}
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) {
    *lpFrequency = 1000000000LL; // nanoseconds
    return TRUE;
}

// ---- GetTickCount ----
inline DWORD GetTickCount() { return timeGetTime(); }

// ---- IsGUIThread (no-op on Linux) ----
inline BOOL IsGUIThread(BOOL /*bConvert*/) { return FALSE; }

// ---- Thread functions stubs (for GetThreadId) ----
inline DWORD GetThreadId(pthread_t /*thread*/) { return 0; }

// ---- InterlockedExchange / InterlockedIncrement ----
inline LONG InterlockedExchange(volatile LONG* Target, LONG Value) {
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
inline LONG InterlockedIncrement(volatile LONG* Addend) {
    return __atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}
inline LONG InterlockedDecrement(volatile LONG* Addend) {
    return __atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}
inline LONG InterlockedCompareExchange(volatile LONG* Destination, LONG Exchange, LONG Comperand) {
    LONG old = Comperand;
    __atomic_compare_exchange_n(Destination, &old, Exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return old;
}

// ---- Memory/string functions ----
#ifndef ZeroMemory
#define ZeroMemory(ptr, size) memset(ptr, 0, size)
#endif
#ifndef CopyMemory
#define CopyMemory(dst, src, size) memcpy(dst, src, size)
#endif

// ---- Math utilities ----
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

// ---- GDI/OpenGL compatibility ----
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

// ---- Guicon stub ----
inline void RedirectIOToConsole() {}

#endif // _WIN32
