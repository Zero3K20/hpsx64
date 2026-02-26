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
#include <cstdio>
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
typedef uint32_t        DWORD;
typedef long            LONG;
typedef long long       LONGLONG;
typedef uint64_t        UINT_PTR;
typedef uint64_t        DWORD_PTR;
typedef long            LRESULT;
typedef long            LPARAM;
typedef unsigned long long WPARAM;
typedef unsigned int    COLORREF;
typedef int             BOOL;
typedef long long       LARGE_INTEGER;

// Windows UINT types
typedef uint32_t        UINT32;
typedef uint16_t        UINT16;
typedef uint8_t         UINT8;
typedef uint64_t        UINT64;
typedef int32_t         INT32;
typedef int16_t         INT16;
typedef int8_t          INT8;
typedef int64_t         INT64;
typedef uint64_t        ULONG_PTR;
typedef uint64_t        UINT64_PTR;

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

// ---- Windows SEH (Structured Exception Handling) stubs for Linux ----
// These types allow Windows exception-handling code to compile on Linux
// The actual SEH (__try/__except) blocks must be guarded with #ifdef _WIN32

// CPU context stub (simplified - only fields used by R3000A)
struct CONTEXT {
    uint64_t Rax;
    uint64_t Rbx;
    uint64_t Rcx;
    uint64_t Rdx;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t Rbp;
    uint64_t Rsp;
    uint64_t R8;
    uint64_t R9;
    uint64_t R10;
    uint64_t R11;
    uint64_t R12;
    uint64_t R13;
    uint64_t R14;
    uint64_t R15;
    uint64_t Rip;
    uint64_t EFlags;
};
typedef CONTEXT* PCONTEXT;
typedef CONTEXT* LPCONTEXT;

struct _EXCEPTION_RECORD {
    unsigned int ExceptionCode;
    unsigned int ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    void* ExceptionAddress;
    uint32_t NumberParameters;
    ULONG_PTR ExceptionInformation[15];
};
typedef _EXCEPTION_RECORD EXCEPTION_RECORD;
typedef _EXCEPTION_RECORD* PEXCEPTION_RECORD;

struct _EXCEPTION_POINTERS {
    _EXCEPTION_RECORD* ExceptionRecord;
    CONTEXT* ContextRecord;
};
typedef _EXCEPTION_POINTERS EXCEPTION_POINTERS;
typedef _EXCEPTION_POINTERS* LPEXCEPTION_POINTERS;

// Exception codes
#define EXCEPTION_CONTINUE_EXECUTION (-1)
#define EXCEPTION_CONTINUE_SEARCH    (0)
#define EXCEPTION_EXECUTE_HANDLER    (1)
#define STATUS_ACCESS_VIOLATION      ((unsigned int)0xC0000005)
#define EXCEPTION_INT_DIVIDE_BY_ZERO ((unsigned int)0xC0000094)
#define EXCEPTION_INT_OVERFLOW       ((unsigned int)0xC0000095)

// __try/__except is a MSVC-specific feature; on Linux, use a no-op wrapper
// The code using __try/__except MUST be guarded with #ifdef _WIN32
// If it's not guarded, define these to be no-ops (they won't do SEH but will compile)
#ifndef _WIN32
#define __try        if(1)
#define __except(x)  else if(0)
#define __finally    if(1)

inline LPEXCEPTION_POINTERS GetExceptionInformation() { return nullptr; }
inline unsigned int GetExceptionCode() { return 0; }
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

// ---- Windows Event/Synchronization API stubs ----
#define WAIT_OBJECT_0       0x00000000
#define WAIT_TIMEOUT        0x00000102
#define WAIT_ABANDONED      0x00000080
#define WAIT_FAILED         0xFFFFFFFF
#define QS_ALLINPUT         0x04FF
#define MWMO_ALERTABLE      0x0002
#define SYNCHRONIZE         0x00100000
#define EVENT_ALL_ACCESS    0x001F0003

inline HANDLE CreateEvent(void* /*attrs*/, BOOL /*manual*/, BOOL /*initial*/, const char* /*name*/) {
    // Use a pipe fd pair as a simple event
    return nullptr; // stub - not used on Linux path
}
inline BOOL SetEvent(HANDLE /*h*/) { return TRUE; }
inline BOOL ResetEvent(HANDLE /*h*/) { return TRUE; }
inline BOOL CloseHandle(HANDLE /*h*/) { return TRUE; }
inline DWORD WaitForSingleObject(HANDLE /*h*/, DWORD /*timeout*/) { return WAIT_OBJECT_0; }
inline DWORD WaitForMultipleObjects(DWORD /*count*/, const HANDLE* /*handles*/, BOOL /*waitAll*/, DWORD /*timeout*/) {
    return WAIT_OBJECT_0;
}
inline DWORD MsgWaitForMultipleObjectsEx(DWORD /*nCount*/, const HANDLE* /*pHandles*/, DWORD /*dwMilliseconds*/, DWORD /*dwWakeMask*/, DWORD /*dwFlags*/) {
    return WAIT_OBJECT_0;
}
inline BOOL PeekMessage(void* /*msg*/, HWND /*hwnd*/, UINT /*min*/, UINT /*max*/, UINT /*remove*/) { return FALSE; }
inline BOOL GetMessage(void* /*msg*/, HWND /*hwnd*/, UINT /*min*/, UINT /*max*/) { return FALSE; }
inline BOOL TranslateMessage(const void* /*msg*/) { return TRUE; }
inline BOOL DispatchMessage(const void* /*msg*/) { return TRUE; }

// GetModuleFileName stub - returns empty string on Linux
inline DWORD GetModuleFileName(HANDLE /*module*/, char* buf, DWORD /*size*/) {
    if (buf) buf[0] = 0;
    return 0;
}
inline DWORD GetModuleFileNameA(HANDLE module, char* buf, DWORD size) {
    return GetModuleFileName(module, buf, size);
}

// Windows handle types for file mapping (stubs for header compatibility)
inline HANDLE CreateFileMapping(HANDLE /*hFile*/, void* /*attrs*/, DWORD /*protect*/,
                                 DWORD /*maxSizeHigh*/, DWORD /*maxSizeLow*/, const char* /*name*/) {
    return nullptr;
}
inline void* MapViewOfFileEx(HANDLE /*hMapping*/, DWORD /*access*/, DWORD /*offHigh*/,
                              DWORD /*offLow*/, size_t /*bytesToMap*/, void* /*baseAddr*/) {
    return nullptr;
}
inline BOOL UnmapViewOfFile(void* /*lpBaseAddress*/) { return TRUE; }

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

// ---- Windows Memory API (VirtualAlloc/VirtualFree etc.) ----
#include <sys/mman.h>
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define MEM_DECOMMIT    0x00004000
#define PAGE_READWRITE          (PROT_READ | PROT_WRITE)
#define PAGE_EXECUTE_READWRITE  (PROT_READ | PROT_WRITE | PROT_EXEC)
#define PAGE_READONLY           PROT_READ
#define PAGE_NOACCESS           PROT_NONE
#define FILE_MAP_ALL_ACCESS     (PROT_READ | PROT_WRITE)
#define FILE_MAP_READ           PROT_READ
#define FILE_MAP_WRITE          PROT_WRITE
#define INVALID_HANDLE_VALUE    ((HANDLE)(intptr_t)(-1))

inline void* VirtualAlloc(void* lpAddress, size_t dwSize, DWORD /*flAllocationType*/, DWORD flProtect) {
    void* result = mmap(lpAddress, dwSize, (int)flProtect,
                        MAP_PRIVATE | MAP_ANONYMOUS | (lpAddress ? MAP_FIXED : 0), -1, 0);
    if (result == MAP_FAILED) return nullptr;
    return result;
}
inline BOOL VirtualFree(void* lpAddress, size_t /*dwSize*/, DWORD /*dwFreeType*/) {
    // For MAP_FIXED mmap, we can't easily get size, so use a reasonable unmap
    // Caller should track the original size; we use PAGE_SIZE as minimum
    return munmap(lpAddress, 4096) == 0 ? TRUE : FALSE;
}
inline BOOL VirtualLock(void* lpAddress, size_t dwSize) {
    return mlock(lpAddress, dwSize) == 0 ? TRUE : FALSE;
}
inline BOOL VirtualUnlock(void* lpAddress, size_t dwSize) {
    return munlock(lpAddress, dwSize) == 0 ? TRUE : FALSE;
}
inline BOOL VirtualProtect(void* lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* /*lpflOldProtect*/) {
    return mprotect(lpAddress, dwSize, (int)flNewProtect) == 0 ? TRUE : FALSE;
}

// Windows file mapping stubs (CreateFileMapping/MapViewOfFileEx not used directly in Linux build)
// These are provided for header compatibility but the actual implementation is in memmgmt.cpp
struct _SECURITY_ATTRIBUTES { int nLength; void* lpSecurityDescriptor; int bInheritHandle; };
typedef _SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

// Stubs for functions not available on Linux that may appear in header-only code
inline void DeleteObject(void* /*obj*/) {}
inline int MessageBox(HWND /*hwnd*/, const char* text, const char* title, unsigned int /*flags*/) {
    fprintf(stderr, "MessageBox: [%s] %s\n", title ? title : "", text ? text : "");
    return 1; // IDOK
}
inline int MessageBoxA(HWND hwnd, const char* text, const char* title, unsigned int flags) {
    return MessageBox(hwnd, text, title, flags);
}

// NM notification codes
#define NM_DBLCLK    (-3)
#define NM_CLICK     (-2)
#define NM_FIRST     (0u - 0u)
#define NM_SETFOCUS  (-7)

// ListView macros (stubs)
#define LVSCW_AUTOSIZE    (-1)
#define LVSCW_AUTOSIZE_USEHEADER (-2)
#define LVNI_SELECTED     0x0002
#define LVN_FIRST         (0u - 100u)
#define LVN_GETDISPINFO   (LVN_FIRST - 77)

// Common controls (stub)
inline void InitCommonControls() {}

// Window/GDI stubs
typedef void* HFONT_WIN;
inline void* CreateFont(int /*h*/, int /*w*/, int /*e*/, int /*o*/, int /*wt*/, DWORD /*i*/,
                         DWORD /*u*/, DWORD /*s*/, DWORD /*cs*/, DWORD /*op*/, DWORD /*cp*/,
                         DWORD /*q*/, DWORD /*pf*/, const char* /*face*/) { return nullptr; }

// ---- GDI/OpenGL compatibility ----
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

// ---- Guicon stub ----
inline void RedirectIOToConsole() {}

#endif // _WIN32
