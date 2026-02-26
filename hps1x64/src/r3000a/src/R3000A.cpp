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




//#include <time.h>

#include <iostream>
#include <iomanip>
#include <stdio.h>

//#include <fstream>

#include "R3000A.h"
#include "R3000A_Execute.h"

// will fix this file later
#include "R3000ADebugPrint.h"
#include "ps1_system.h"


using namespace R3000A;
using namespace R3000A::Instruction;
using namespace Playstation1;

using namespace std;

//#include "GNUSignExtend_x64.h"



//using namespace x64SignExtend::Utilities;



//#define ENABLE_R3000A_IDLE


// enable level-2 recompiler for R3000A
//#define ENABLE_R3000A_RECOMPILE2

// put recompiled code behind function call for profiling
//#define ENABLE_R3000A_PROFILING


// this one goes to cout and is good for bug reports
#define INLINE_DEBUG_COUT


// force use of recompiler for profiling
//#define FORCE_ENABLE_RECOMPILER_R3000A


#define ENABLE_PS1_VIRTUAL_MACHINE_R3000A
#define ENABLE_PS1_VIRTUAL_MACHINE_DCACHE


// use exceptions with div/divu in recompiler
#define USE_EXCEPTIONS_R3000A_DIV


// check if cached instructions have been modified only on cache-reload
// this means I probably won't be checking in the recompiler if instruction is cached
//#define CHECK_MODIFY_ON_RELOAD


//#define ENABLE_LATENT_BUS_READ


//#define VERBOSE_RECOMPILE
//#define VERBOSE_CPU_DEBUG


// use callback functions
//#define USE_DELAY_CALLBACKS



#ifdef _DEBUG_VERSION_

// build this with inline debugger
#define INLINE_DEBUG_ENABLE

//#define INLINE_DEBUG_SPLIT


//#define INLINE_DEBUG_TESTING

//#define INLINE_DEBUG
//#define INLINE_DEBUG_ASYNC_INT
//#define INLINE_DEBUG_SYNC_INT
//#define INLINE_DEBUG_UPDATE_INT
//#define INLINE_DEBUG_BIOS

//#define INLINE_DEBUG_LOAD
//#define INLINE_DEBUG_STORE

//#define COUT_DELAYSLOT_CANCEL

#endif


funcVoid Cpu::UpdateInterrupts;

Debug::Log Cpu::debug;
Debug::Log Cpu::debugBios;



Playstation1::DataBus *Cpu::Bus;
u64* Cpu::_SpuCycleCount;

u64 Cpu::MemoryLatency;

volatile u64* volatile Cpu::_NextSystemEvent;


Cpu::DCache_Ptr Cpu::DCache;
char* Cpu::pDCacheVirtual;

u64 Cpu::uExceptionRWAddr64;
u64 Cpu::uExceptionInstrAddr64;


volatile Cpu::_DebugStatus Cpu::DebugStatus;
//volatile u32 Cpu::Debug_BreakPoint_Address;
//u32 Cpu::Debug_BreakPoint_Address;
//u32 Cpu::Debug_RAMDisplayStart;

Recompiler* Cpu::rs;
//Recompiler* Cpu::rm;

//char *Cpu::Rc_CodePtr;
//u32 Cpu::Rc_CodeBlock_Shift;
//u32 Cpu::Rc_CodeBlock_Mask;







u32* Cpu::_Debug_IntcStat;
u32* Cpu::_Debug_IntcMask;

u32* Cpu::_Intc_Stat;
u32* Cpu::_Intc_Mask;
u32* Cpu::_R3000A_Status;




Cpu *Cpu::_CPU;



bool Cpu::DebugWindow_Enabled;
WindowClass::Window *Cpu::DebugWindow;
DebugValueList<u32> *Cpu::GPR_ValueList;
DebugValueList<u32> *Cpu::COP0_ValueList;
DebugValueList<u32> *Cpu::COP2_CPCValueList;
DebugValueList<u32> *Cpu::COP2_CPRValueList;
Debug_DisassemblyViewer *Cpu::DisAsm_Window;
Debug_BreakpointWindow *Cpu::Breakpoint_Window;
Debug_MemoryViewer *Cpu::ScratchPad_Viewer;
Debug_BreakPoints *Cpu::Breakpoints;


u32 Cpu::TraceValue;



// *note* Bios Calls tables are ripped from MAME with parts from other sources

static const struct
{
	int address;
	int operation;
	const char *prototype;
} bioscallsA0[] =
{
	{ 0xa0, 0x00, "int open(const char *name, int mode)" },
	{ 0xa0, 0x01, "int lseek(int fd, int offset, int whence)" },
	{ 0xa0, 0x02, "int read(int fd, void *buf, int nbytes)" },
	{ 0xa0, 0x03, "int write(int fd, void *buf, int nbytes)" },
	{ 0xa0, 0x04, "int close(int fd)" },
	{ 0xa0, 0x05, "int ioctl(int fd, int cmd, int arg)" },
	{ 0xa0, 0x06, "void exit(int code)" },
	{ 0xa0, 0x07, "sys_a0_07()" },
	{ 0xa0, 0x08, "char getc(int fd)" },
	{ 0xa0, 0x09, "void putc(char c, int fd)" },
	{ 0xa0, 0x0a, "todigit()" },
	{ 0xa0, 0x0b, "double atof(const char *s)" },
	{ 0xa0, 0x0c, "long strtoul(const char *s, char **ptr, int base)" },
	{ 0xa0, 0x0d, "unsigned long strtol(const char *s, char **ptr, int base)" },
	{ 0xa0, 0x0e, "int abs(int val)" },
	{ 0xa0, 0x0f, "long labs(long lval)" },
	{ 0xa0, 0x10, "long atoi(const char *s)" },
	{ 0xa0, 0x11, "int atol(const char *s)" },
	{ 0xa0, 0x12, "atob()" },
	{ 0xa0, 0x13, "int setjmp(jmp_buf *ctx)" },
	{ 0xa0, 0x14, "void longjmp(jmp_buf *ctx, int value)" },
	{ 0xa0, 0x15, "char *strcat(char *dst, const char *src)" },
	{ 0xa0, 0x16, "char *strncat(char *dst, const char *src, size_t n)" },
	{ 0xa0, 0x17, "int strcmp(const char *dst, const char *src)" },
	{ 0xa0, 0x18, "int strncmp(const char *dst, const char *src, size_t n)" },
	{ 0xa0, 0x19, "char *strcpy(char *dst, const char *src)" },
	{ 0xa0, 0x1a, "char *strncpy(char *dst, const char *src, size_t n)" },
	{ 0xa0, 0x1b, "size_t strlen(const char *s)" },
	{ 0xa0, 0x1c, "int index(const char *s, int c)" },
	{ 0xa0, 0x1d, "int rindex(const char *s, int c)" },
	{ 0xa0, 0x1e, "char *strchr(const char *s, int c)" },
	{ 0xa0, 0x1f, "char *strrchr(const char *s, int c)" },
	{ 0xa0, 0x20, "char *strpbrk(const char *dst, const char *src)" },
	{ 0xa0, 0x21, "size_t strspn(const char *s, const char *set)" },
	{ 0xa0, 0x22, "size_t strcspn(const char *s, const char *set)" },
	{ 0xa0, 0x23, "char *strtok(char *s, const char *set)" },
	{ 0xa0, 0x24, "char *strstr(const char *s, const char *set)" },
	{ 0xa0, 0x25, "int toupper(int c)" },
	{ 0xa0, 0x26, "int tolower(int c)" },
	{ 0xa0, 0x27, "void bcopy(const void *src, void *dst, size_t len)" },
	{ 0xa0, 0x28, "void bzero(void *ptr, size_t len)" },
	{ 0xa0, 0x29, "int bcmp(const void *ptr1, const void *ptr2, int len)" },
	{ 0xa0, 0x2a, "void *memcpy(void *dst, const void *src, size_t n)" },
	{ 0xa0, 0x2b, "void *memset(void *dst, char c, size_t n)" },
	{ 0xa0, 0x2c, "void *memmove(void *dst, const void *src, size_t n)" },
	{ 0xa0, 0x2d, "int memcmp(const void *dst, const void *src, size_t n)" },
	{ 0xa0, 0x2e, "void *memchr(const void *s, int c, size_t n)" },
	{ 0xa0, 0x2f, "int rand()" },
	{ 0xa0, 0x30, "void srand(unsigned int seed)" },
	{ 0xa0, 0x31, "void qsort(void *base, int nel, int width, int (*cmp)(void *, void *))" },
	{ 0xa0, 0x32, "double strtod(const char *s, char **endptr)" },
	{ 0xa0, 0x33, "void *malloc(int size)" },
	{ 0xa0, 0x34, "void free(void *buf)" },
	{ 0xa0, 0x35, "void *lsearch(void *key, void *base, int belp, int width, int (*cmp)(void *, void *))" },
	{ 0xa0, 0x36, "void *bsearch(void *key, void *base, int nel, int size, int (*cmp)(void *, void *))" },
	{ 0xa0, 0x37, "void *calloc(int size, int n)" },
	{ 0xa0, 0x38, "void *realloc(void *buf, int n)" },
	{ 0xa0, 0x39, "InitHeap(void *block, int size)" },
	{ 0xa0, 0x3a, "void _exit(int code)" },
	{ 0xa0, 0x3b, "char getchar(void)" },
	{ 0xa0, 0x3c, "void putchar(char c)" },
	{ 0xa0, 0x3d, "char *gets(char *s)" },
	{ 0xa0, 0x3e, "void puts(const char *s)" },
	{ 0xa0, 0x3f, "int printf(const char *fmt, ...)" },
	{ 0xa0, 0x40, "sys_a0_40()" },
	{ 0xa0, 0x41, "int LoadTest(const char *name, struct EXEC *header)" },
	{ 0xa0, 0x42, "int Load(const char *name, struct EXEC *header)" },
	{ 0xa0, 0x43, "int Exec(struct EXEC *header, int argc, char **argv)" },
	{ 0xa0, 0x44, "void FlushCache()" },
	{ 0xa0, 0x45, "void InstallInterruptHandler()" },
	{ 0xa0, 0x46, "GPU_dw(int x, int y, int w, int h, long *data)" },
	{ 0xa0, 0x47, "mem2vram(int x, int y, int w, int h, long *data)" },
	{ 0xa0, 0x48, "SendGPU(int status)" },
	{ 0xa0, 0x49, "GPU_cw(long cw)" },
	{ 0xa0, 0x4a, "GPU_cwb(long *pkt, int len)" },
	{ 0xa0, 0x4b, "SendPackets(void *ptr)" },
	{ 0xa0, 0x4c, "sys_a0_4c()" },
	{ 0xa0, 0x4d, "int GetGPUStatus()" },
	{ 0xa0, 0x4e, "GPU_sync()" },
	{ 0xa0, 0x4f, "sys_a0_4f()" },
	{ 0xa0, 0x50, "sys_a0_50()" },
	{ 0xa0, 0x51, "int LoadExec(const char *name, int, int)" },
	{ 0xa0, 0x52, "GetSysSp()" },
	{ 0xa0, 0x53, "sys_a0_53()" },
	{ 0xa0, 0x54, "_96_init()" },
	{ 0xa0, 0x55, "_bu_init()" },
	{ 0xa0, 0x56, "_96_remove()" },
	{ 0xa0, 0x57, "sys_a0_57()" },
	{ 0xa0, 0x58, "sys_a0_58()" },
	{ 0xa0, 0x59, "sys_a0_59()" },
	{ 0xa0, 0x5a, "sys_a0_5a()" },
	{ 0xa0, 0x5b, "dev_tty_init()" },
	{ 0xa0, 0x5c, "dev_tty_open()" },
	{ 0xa0, 0x5d, "dev_tty_5d()" },
	{ 0xa0, 0x5e, "dev_tty_ioctl()" },
	{ 0xa0, 0x5f, "dev_cd_open()" },
	{ 0xa0, 0x60, "dev_cd_read()" },
	{ 0xa0, 0x61, "dev_cd_close()" },
	{ 0xa0, 0x62, "dev_cd_firstfile()" },
	{ 0xa0, 0x63, "dev_cd_nextfile()" },
	{ 0xa0, 0x64, "dev_cd_chdir()" },
	{ 0xa0, 0x65, "dev_card_open()" },
	{ 0xa0, 0x66, "dev_card_read()" },
	{ 0xa0, 0x67, "dev_card_write()" },
	{ 0xa0, 0x68, "dev_card_close()" },
	{ 0xa0, 0x69, "dev_card_firstfile()" },
	{ 0xa0, 0x6a, "dev_card_nextfile()" },
	{ 0xa0, 0x6b, "dev_card_erase()" },
	{ 0xa0, 0x6c, "dev_card_undelete()" },
	{ 0xa0, 0x6d, "dev_card_format()" },
	{ 0xa0, 0x6e, "dev_card_rename()" },
	{ 0xa0, 0x6f, "dev_card_6f()" },
	{ 0xa0, 0x70, "_bu_init()" },
	{ 0xa0, 0x71, "_96_init()" },
	{ 0xa0, 0x72, "_96_remove()" },
	{ 0xa0, 0x73, "sys_a0_73()" },
	{ 0xa0, 0x74, "sys_a0_74()" },
	{ 0xa0, 0x75, "sys_a0_75()" },
	{ 0xa0, 0x76, "sys_a0_76()" },
	{ 0xa0, 0x77, "sys_a0_77()" },
	{ 0xa0, 0x78, "_96_CdSeekL()" },
	{ 0xa0, 0x79, "sys_a0_79()" },
	{ 0xa0, 0x7a, "sys_a0_7a()" },
	{ 0xa0, 0x7b, "sys_a0_7b()" },
	{ 0xa0, 0x7c, "_96_CdGetStatus()" },
	{ 0xa0, 0x7d, "sys_a0_7d()" },
	{ 0xa0, 0x7e, "_96_CdRead()" },
	{ 0xa0, 0x7f, "sys_a0_7f()" },
	{ 0xa0, 0x80, "sys_a0_80()" },
	{ 0xa0, 0x81, "sys_a0_81()" },
	{ 0xa0, 0x82, "sys_a0_82()" },
	{ 0xa0, 0x83, "sys_a0_83()" },
	{ 0xa0, 0x84, "sys_a0_84()" },
	{ 0xa0, 0x85, "_96_CdStop()" },
	//{ 0xa0, 0x84, "sys_a0_84()" },
	//{ 0xa0, 0x85, "sys_a0_85()" },
	{ 0xa0, 0x86, "sys_a0_86()" },
	{ 0xa0, 0x87, "sys_a0_87()" },
	{ 0xa0, 0x88, "sys_a0_88()" },
	{ 0xa0, 0x89, "sys_a0_89()" },
	{ 0xa0, 0x8a, "sys_a0_8a()" },
	{ 0xa0, 0x8b, "sys_a0_8b()" },
	{ 0xa0, 0x8c, "sys_a0_8c()" },
	{ 0xa0, 0x8d, "sys_a0_8d()" },
	{ 0xa0, 0x8e, "sys_a0_8e()" },
	{ 0xa0, 0x8f, "sys_a0_8f()" },
	{ 0xa0, 0x90, "sys_a0_90()" },
	{ 0xa0, 0x91, "sys_a0_91()" },
	{ 0xa0, 0x92, "sys_a0_92()" },
	{ 0xa0, 0x93, "sys_a0_93()" },
	{ 0xa0, 0x94, "sys_a0_94()" },
	{ 0xa0, 0x95, "sys_a0_95()" },
	{ 0xa0, 0x96, "AddCDROMDevice()" },
	{ 0xa0, 0x97, "AddMemCardDevice()" },
	{ 0xa0, 0x98, "DisableKernelIORedirection()" },
	{ 0xa0, 0x99, "EnableKernelIORedirection()" },
	{ 0xa0, 0x9a, "sys_a0_9a()" },
	{ 0xa0, 0x9b, "sys_a0_9b()" },
	{ 0xa0, 0x9c, "void SetConf(int Event, int TCB, int Stack)" },
	{ 0xa0, 0x9d, "void GetConf(int *Event, int *TCB, int *Stack)" },
	{ 0xa0, 0x9e, "sys_a0_9e()" },
	{ 0xa0, 0x9f, "void SetMem(int size)" },
	{ 0xa0, 0xa0, "_boot()" },
	{ 0xa0, 0xa1, "SystemError()" },
	{ 0xa0, 0xa2, "EnqueueCdIntr()" },
	{ 0xa0, 0xa3, "DequeueCdIntr()" },
	{ 0xa0, 0xa4, "sys_a0_a4()" },
	{ 0xa0, 0xa5, "ReadSector(int count, int sector, void *buffer)" },
	{ 0xa0, 0xa6, "get_cd_status()" },
	{ 0xa0, 0xa7, "bufs_cb_0()" },
	{ 0xa0, 0xa8, "bufs_cb_1()" },
	{ 0xa0, 0xa9, "bufs_cb_2()" },
	{ 0xa0, 0xaa, "bufs_cb_3()" },
	{ 0xa0, 0xab, "_card_info()" },
	{ 0xa0, 0xac, "_card_load()" },
	{ 0xa0, 0xad, "_card_auto()" },
	{ 0xa0, 0xae, "bufs_cb_4()" },
	{ 0xa0, 0xaf, "sys_a0_af()" },
	{ 0xa0, 0xb0, "sys_a0_b0()" },
	{ 0xa0, 0xb1, "sys_a0_b1()" },
	{ 0xa0, 0xb2, "do_a_long_jmp()" },
	{ 0xa0, 0xb3, "sys_a0_b3()" },
	{ 0xa0, 0xb4, "GetKernelInfo(int sub_function)" }
};

static const struct
{
	int address;
	int operation;
	const char *prototype;
} bioscallsB0[] =
{
	{ 0xb0, 0x00, "SysMalloc()" },
	{ 0xb0, 0x01, "sys_b0_01()" },
	{ 0xb0, 0x02, "sys_b0_02()" },
	{ 0xb0, 0x03, "sys_b0_03()" },
	{ 0xb0, 0x04, "sys_b0_04()" },
	{ 0xb0, 0x05, "sys_b0_05()" },
	{ 0xb0, 0x06, "sys_b0_06()" },
	{ 0xb0, 0x07, "void DeliverEvent(u_long class, u_long event)" },
	{ 0xb0, 0x08, "long OpenEvent(u_long class, long spec, long mode, long (*func)())" },
	{ 0xb0, 0x09, "long CloseEvent(long event)" },
	{ 0xb0, 0x0a, "long WaitEvent(long event)" },
	{ 0xb0, 0x0b, "long TestEvent(long event)" },
	{ 0xb0, 0x0c, "long EnableEvent(long event)" },
	{ 0xb0, 0x0d, "long DisableEvent(long event)" },
	{ 0xb0, 0x0e, "OpenTh()" },
	{ 0xb0, 0x0f, "CloseTh()" },
	{ 0xb0, 0x10, "ChangeTh()" },
	{ 0xb0, 0x11, "sys_b0_11()" },
	{ 0xb0, 0x12, "int InitPAD(char *buf1, int len1, char *buf2, int len2)" },
	{ 0xb0, 0x13, "int StartPAD(void)" },
	{ 0xb0, 0x14, "int StopPAD(void)" },
	{ 0xb0, 0x15, "PAD_init(u_long nazo, u_long *pad_buf)" },
	{ 0xb0, 0x16, "u_long PAD_dr()" },
	{ 0xb0, 0x17, "void ReturnFromException(void)" },
	{ 0xb0, 0x18, "ResetEntryInt()" },
	{ 0xb0, 0x19, "HookEntryInt()" },
	{ 0xb0, 0x1a, "sys_b0_1a()" },
	{ 0xb0, 0x1b, "sys_b0_1b()" },
	{ 0xb0, 0x1c, "sys_b0_1c()" },
	{ 0xb0, 0x1d, "sys_b0_1d()" },
	{ 0xb0, 0x1e, "sys_b0_1e()" },
	{ 0xb0, 0x1f, "sys_b0_1f()" },
	{ 0xb0, 0x20, "UnDeliverEvent(int class, int event)" },
	{ 0xb0, 0x21, "sys_b0_21()" },
	{ 0xb0, 0x22, "sys_b0_22()" },
	{ 0xb0, 0x23, "sys_b0_23()" },
	{ 0xb0, 0x24, "sys_b0_24()" },
	{ 0xb0, 0x25, "sys_b0_25()" },
	{ 0xb0, 0x26, "sys_b0_26()" },
	{ 0xb0, 0x27, "sys_b0_27()" },
	{ 0xb0, 0x28, "sys_b0_28()" },
	{ 0xb0, 0x29, "sys_b0_29()" },
	{ 0xb0, 0x2a, "sys_b0_2a()" },
	{ 0xb0, 0x2b, "sys_b0_2b()" },
	{ 0xb0, 0x2c, "sys_b0_2c()" },
	{ 0xb0, 0x2d, "sys_b0_2d()" },
	{ 0xb0, 0x2e, "sys_b0_2e()" },
	{ 0xb0, 0x2f, "sys_b0_2f()" },
	// correction from MAME v0145 -> changed 0x2f to 0x30 just below
	{ 0xb0, 0x30, "sys_b0_30()" },
	{ 0xb0, 0x31, "sys_b0_31()" },
	{ 0xb0, 0x32, "int open(const char *name, int access)" },
	{ 0xb0, 0x33, "int lseek(int fd, long pos, int seektype)" },
	{ 0xb0, 0x34, "int read(int fd, void *buf, int nbytes)" },
	{ 0xb0, 0x35, "int write(int fd, void *buf, int nbytes)" },
	{ 0xb0, 0x36, "close(int fd)" },
	{ 0xb0, 0x37, "int ioctl(int fd, int cmd, int arg)" },
	{ 0xb0, 0x38, "exit(int exitcode)" },
	{ 0xb0, 0x39, "sys_b0_39()" },
	{ 0xb0, 0x3a, "char getc(int fd)" },
	{ 0xb0, 0x3b, "putc(int fd, char ch)" },
	{ 0xb0, 0x3c, "char getchar(void)" },
	{ 0xb0, 0x3d, "putchar(char ch)" },
	{ 0xb0, 0x3e, "char *gets(char *s)" },
	{ 0xb0, 0x3f, "puts(const char *s)" },
	{ 0xb0, 0x40, "int cd(const char *path)" },
	{ 0xb0, 0x41, "int format(const char *fs)" },
	{ 0xb0, 0x42, "struct DIRENTRY* firstfile(const char *name, struct DIRENTRY *dir)" },
	{ 0xb0, 0x43, "struct DIRENTRY* nextfile(struct DIRENTRY *dir)" },
	{ 0xb0, 0x44, "int rename(const char *oldname, const char *newname)" },
	{ 0xb0, 0x45, "int delete(const char *name)" },
	{ 0xb0, 0x46, "undelete()" },
	{ 0xb0, 0x47, "AddDevice()" },
	{ 0xb0, 0x48, "RemoveDevice()" },
	{ 0xb0, 0x49, "PrintInstalledDevices()" },
	{ 0xb0, 0x4a, "InitCARD()" },
	{ 0xb0, 0x4b, "StartCARD()" },
	{ 0xb0, 0x4c, "StopCARD()" },
	{ 0xb0, 0x4d, "sys_b0_4d()" },
	{ 0xb0, 0x4e, "_card_write()" },
	{ 0xb0, 0x4f, "_card_read()" },
	{ 0xb0, 0x50, "_new_card()" },
	{ 0xb0, 0x51, "void *Krom2RawAdd(int code)" },
	{ 0xb0, 0x52, "sys_b0_52()" },
	{ 0xb0, 0x53, "sys_b0_53()" },
	{ 0xb0, 0x54, "long _get_errno(void)" },
	{ 0xb0, 0x55, "long _get_error(long fd)" },
	{ 0xb0, 0x56, "GetC0Table()" },
	{ 0xb0, 0x57, "GetB0Table()" },
	{ 0xb0, 0x58, "_card_chan()" },
	{ 0xb0, 0x59, "sys_b0_59()" },
	{ 0xb0, 0x5a, "sys_b0_5a()" },
	{ 0xb0, 0x5b, "ChangeClearPAD(int, int)" },
	{ 0xb0, 0x5c, "_card_status()" },
	{ 0xb0, 0x5d, "_card_wait()" },
	/*{ 0x00, 0x00, NULL }*/	};

	
static const struct
{
	int address;
	int operation;
	const char *prototype;
} bioscallsC0[] =
{
	{ 0xc0, 0x00, "InitRCnt(int r)" },
	{ 0xc0, 0x01, "InitException(int e)" },
	{ 0xc0, 0x02, "SysEnqIntRP(int index, long *queue)" },
	{ 0xc0, 0x03, "SysDeqIntRP(int index, long *queue)" },
	{ 0xc0, 0x04, "int get_free_EvCB_slot(void)" },
	{ 0xc0, 0x05, "get_free_TCB_slot()" },
	{ 0xc0, 0x06, "ExceptionHandler()" },
	{ 0xc0, 0x07, "InstallExceptionHandlers()" },
	{ 0xc0, 0x08, "SysInitMemory(void* pointer, int size)" },
	{ 0xc0, 0x09, "SysInitKMem()" },
	{ 0xc0, 0x0a, "ChangeClearRCnt()" },
	{ 0xc0, 0x0b, "SystemError()" },
	{ 0xc0, 0x0c, "InitDefInt(int i)" },
	{ 0xc0, 0x0d, "sys_c0_0d()" },
	{ 0xc0, 0x0e, "sys_c0_0e()" },
	{ 0xc0, 0x0f, "sys_c0_0f()" },
	{ 0xc0, 0x10, "sys_c0_10()" },
	{ 0xc0, 0x11, "sys_c0_11()" },
	{ 0xc0, 0x12, "InstallDevices()" },
	{ 0xc0, 0x13, "FlushStdInOutPut()" },
	{ 0xc0, 0x14, "sys_c0_14()" },
	{ 0xc0, 0x15, "_cdevinput()" },
	{ 0xc0, 0x16, "_cdevscan()" },
	{ 0xc0, 0x17, "char _circgetc(struct device_buf *circ)" },
	{ 0xc0, 0x18, "_circputc(char c, struct device_buf *circ)" },
	{ 0xc0, 0x19, "ioabort(const char *str)" },
	{ 0xc0, 0x1a, "sys_c0_1a()" },
	{ 0xc0, 0x1b, "KernelRedirect(int flag)" },
	{ 0xc0, 0x1c, "PatchA0Table()" }
};



// for IOP
// INTRMAN - Interrupt manager functions
// IOMAN - File driver functions
// LIBSD - Sound processor functions
// LOADCORE - Core misc functions
// SIFCMD - Serial interface functions
// SIFMAN - Serial interface manager functions
// STDIO - Printf function
// SYSCLIB - Standard C library functions
// SYSMEM - Memory management functions
// THBASE - Threading functions
// THSEMAP - Semaphore functions


/*
Cpu::Cpu ()
{
	cout << "Running R3000A constructor...\n";


}
*/

void Cpu::ConnectDevices ( Playstation1::DataBus* db, u64* _SpuCC )
{
	Bus = db;
	_SpuCycleCount = _SpuCC;
}

Cpu::~Cpu ( void )
{
	//if ( debug_enabled ) delete DebugWindow;
	
	delete rs;

#ifdef ENABLE_PS1_VIRTUAL_MACHINE_DCACHE

	VirtualUnlock((LPVOID)pDCacheVirtual, DCache_Size);
	if (!VirtualFree((LPVOID)pDCacheVirtual, NULL, MEM_RELEASE))
	{
		cout << "\nhpsx64: VIRTUAL MACHINE: PROBLEM freeing Virtual R3000A DCACHE.";
	}

#endif

}


void Cpu::Start ()
{
	cout << "Running Cpu::Start...\n";
	
#ifdef INLINE_DEBUG_ENABLE

#ifdef INLINE_DEBUG_SPLIT
	// put debug output into a separate file
	debug.SetSplit ( true );
	debug.SetCombine ( false );
#endif

#ifdef INLINE_DEBUG_SPLIT_BIOS
	// put debug output into a separate file
	debugBios.SetSplit ( true );
	debugBios.SetCombine ( false );
#endif

	// start the inline debugger
	debug.Create ( "R3000ALog.txt" );
	debugBios.Create ( "BiosCallsLog.txt" );
#endif

	
	// not yet debugging
	debug_enabled = false;


#ifdef ENABLE_PS1_VIRTUAL_MACHINE_DCACHE
	// dcache on R3000A is 1KB
	pDCacheVirtual = (char*)VirtualAlloc((LPVOID)(0x1f800000ull | Playstation1::DataBus::BUS_R3000A_TRANSFER_BIT), DCache_Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (pDCacheVirtual)
	{
		cout << "\nhpsx64: VIRTUAL MACHINE: R3000A DCACHE Allocated successfully.";
	}

	DCache.b32 = (u32*)pDCacheVirtual;
#else

	// set the internal dcache pointer to point to the physical one for now
	DCache.b32 = PhyDCache.b32;
#endif

	// initialize/enable printing of instructions
	Print::Start ();

	// Connect CPU with System Bus
	//Bus = db;
	
	
	// create execution unit to execute instructions
	//e = new Instruction::Execute ();
	//Instruction::Execute::Execute ( this );
	Instruction::Execute::r = this;
	
	// create an instance of ICache and connect with CPU
	// *note* this is no longer a pointer to the object
	//ICache = new ICache_Device ();
	
	// create an instance of COP2 co-processor and connect with CPU
	// *note* this is no longer a pointer to the object
	//COP2 = new COP2_Device ();
	
	// set as the current R3000A cpu object
	_CPU = this;
	
	// create object to handle breakpoints
	Breakpoints = new Debug_BreakPoints ( Bus->BIOS.b8, Bus->MainMemory.b8, DCache.b8 );
	
	// reset the cpu object
	Reset ();
	
	// start COP2 object
	COP2.Start ();
	
	// start the instruction execution object
	Instruction::Execute::Start ();
	

#ifdef ENABLE_R3000A_RECOMPILE2
	//rs = new Recompiler ( this, 19 - DataBus::c_iInvalidate_Shift, DataBus::c_iInvalidate_Shift + 8, DataBus::c_iInvalidate_Shift );
	//rs = new Recompiler ( this, 15 - DataBus::c_iInvalidate_Shift, DataBus::c_iInvalidate_Shift + 12, DataBus::c_iInvalidate_Shift );
	rs = new Recompiler ( this, 15 - DataBus::c_iInvalidate_Shift, DataBus::c_iInvalidate_Shift + 9, DataBus::c_iInvalidate_Shift );
	
	
	rs->SetOptimizationLevel ( 2 );
	//rs->SetOptimizationLevel ( 1 );
	//rs->SetOptimizationLevel ( 0 );
#else
	//rs = new Recompiler ( this, 16 - DataBus::c_iInvalidate_Shift, DataBus::c_iInvalidate_Shift + 8, DataBus::c_iInvalidate_Shift );
	rs = new Recompiler(this, R3000A_CODE_BLOCK_COUNT_SHIFT, R3000A_CODE_BLOCK_SIZE_SHIFT, R3000A_INSTRUCTIONS_PER_CACHE_ENTRY_SHIFT);

	rs->SetOptimizationLevel ( 1 );
#endif
	
	// enable recompiler by default
	bEnableRecompiler = true;
	Status.uDisableRecompiler = false;

}

void Cpu::Reset ( void )
{
	u32 i;
	
	// make sure recompiler instance(s) is deleted
	//delete rs;
	//delete rm;
	
	// zero object
	memset ( this, 0, sizeof( Cpu ) );

	// this is the start address for the program counter when a R3000A is reset
	PC = 0xbfc00000;
	
	// start in kernel mode?
	CPR0.Status.KUc = 1;

	// must set this as already having been executed
	CurInstExecuted = true;
	
	// set processor id implementation and revision for R3000A
	CPR0.PRId.Rev = c_ProcessorRevisionNumber;
	CPR0.PRId.Imp = c_ProcessorImplementationNumber;
	
	
	// need to redo reset of COP2 - must call reset and not Start, cuz if you call start again then inline debugging does not work anymore
	COP2.Reset ();
	
	// reset i-cache again
	ICache.Reset ();
	
}



void Cpu::Refresh ( void )
{
	Refresh_AllDelaySlotPointers ();
}


void Cpu::InvalidateCache ( u32 Address )
{
	ICache.Invalidate ( Address );
}



// ---------------------------------------------------------------------------------


// before saving state, need to call this to copy virtual memory back into physical memory if using virtual machine
void Cpu::SaveVirtualState()
{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_DCACHE

	memcpy((void*)PhyDCache.b8, (const void*)DCache.b8, DCache_Size);

#endif

}

// after loading state, call this to restore physical memory to virtual memory
void Cpu::RestoreVirtualState()
{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_DCACHE

	memcpy((void*)DCache.b8, (const void*)PhyDCache.b8, DCache_Size);

#endif

}


int Cpu::ExceptionFilter(LPEXCEPTION_POINTERS pEp
	//void (*pMonitorFxn)(LPEXCEPTION_POINTERS, void*)
)
{
	CONTEXT* ctx = pEp->ContextRecord;
	ULONG_PTR* info = pEp->ExceptionRecord->ExceptionInformation;
	UINT_PTR addr = info[1];
	DWORD dummy;

	long lTemp;

	static constexpr int IDIV_INSTRUCTION_SIZE_BYTES = 2;

	switch (pEp->ExceptionRecord->ExceptionCode)
	{
#ifdef USE_EXCEPTIONS_R3000A_DIV

	case EXCEPTION_INT_DIVIDE_BY_ZERO:

		//cout << "\nR3000A: EXCEPTION_INT_DIVIDE_BY_ZERO";

		// remainder -> hi -> rs (rax)
		// quotient -> lo -> 1 if negative or -1 if positive -> (rax)
		lTemp = pEp->ContextRecord->Rdx | 0x1;
		pEp->ContextRecord->Rdx = pEp->ContextRecord->Rax;
		//pEp->ContextRecord->Rax = (((long)pEp->ContextRecord->Rax) < 0) ? 1 : -1;
		pEp->ContextRecord->Rax = -lTemp;

		// skip the 32-bit idiv with memory
		pEp->ContextRecord->Rip += IDIV_INSTRUCTION_SIZE_BYTES;

		// continue execution
		return EXCEPTION_CONTINUE_EXECUTION;
		break;

	case EXCEPTION_INT_OVERFLOW:

		//cout << "\nR3000A: EXCEPTION_INT_OVERFLOW";

		pEp->ContextRecord->Rax = 0x80000000;
		pEp->ContextRecord->Rdx = 0;

		// skip the 32-bit idiv with memory
		pEp->ContextRecord->Rip += IDIV_INSTRUCTION_SIZE_BYTES;

		// continue execution
		return EXCEPTION_CONTINUE_EXECUTION;
		break;

#endif	// end #ifdef USE_EXCEPTIONS_R3000A_DIV

	case STATUS_ACCESS_VIOLATION:

		// only interested if address is larger than 32-bits for now
		if (((u64)addr) >> 32)
		{
			// get the address that it was trying to read/write to
			uExceptionRWAddr64 = addr;

			// get the address of the instruction that caused the exception
			uExceptionInstrAddr64 = ctx->Rip;

			return EXCEPTION_EXECUTE_HANDLER;
		}

		break;

	default:
		break;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}


void Cpu::ExecuteRecompiledCode()
{
	long Index;

	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	GPR[0].u = 0;

	// get the block index
	//Block = Get_Block ( PC );
	Index = rs->Get_Index(PC);

	// offset cycles before the run, so that it updates to the correct value
	CycleCount -= rs->CycleCount[Index];


	((func2)(rs->pCodeStart[Index])) ();
}


void Cpu::ExecuteSomethingBusy()
{

	if (Status.DelaySlot_Valid)
	{

		if (Status.DelaySlot_Valid & 1)
		{

			DelaySlots[NextDelaySlotIndex].cb();
			DelaySlots[NextDelaySlotIndex].Value = 0;
		}

		///////////////////////////////////
		// move delay slot
		NextDelaySlotIndex ^= 1;

		///////////////////////////////////////////////////
		// Advance status bits for checking delay slot
		Status.DelaySlot_Valid >>= 1;

	} // end if ( Status.DelaySlot_Valid )


	//////////////////////////////////////////////
	// check for Asynchronous Interrupts
	// also make sure interrupts are enabled
	if (Status.isExternalInterrupt)
	{
		//Status.isExternalInterrupt >>= 1;

		//if (!Status.isExternalInterrupt)
		{

			///////////////////////
			// *** IMPORTANT *** /////////////////////////////////////////////////////////////////////
			// *** PS1 must NOT be triggering Async Interrupt when GTE opcode is next to execute ** //
			if (((Bus->Read(NextPC) >> 24) & 0xfe) != 0x4a)
			{

				// interrupt handled
				Status.isExternalInterrupt = 0;

				if ((CPR0.Status.IEc) && (CPR0.Status.Value & CPR0.Cause.l & 0xff00))
				{
					CycleCount++;


					LastPC = PC;
					PC = NextPC;

					// interrupt has been triggered
					ProcessAsynchronousInterrupt();

					return;
				}

			}	// end if ( ( ( Bus->Read ( NextPC ) >> 24 ) & 0xfe ) != 0x4a )

		}	// end if ( !Status.isExternalInterrupt )

	}	// end if (Status.isExternalInterrupt)

}

void Cpu::ExecuteNotCached()
{
	// the bus is free and there are no pending load/store operations
	// Important: When reading from the bus, it has already been determined whether address is in I-Cache or not, so clear top 3 bits
	//CurInst.Value = Bus->Read ( PC & 0x1fffffff );
	CurInst.Value = Bus->Read_t<0xffffffff>(PC & 0x1fffffff);

	// cpu should be suspended while loading from memory //
	CycleCount += Bus->GetLatency();


	if (bEnableRecompiler)
	{

		if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
		{

			// read from pc in main memory, compare instructions

			// make sure that
			u64 xcompare64;

			// get pointer into main memory
			u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0xf]);

			// get pointer into the source code that was recompiled
			u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0xf);

			xcompare64 = *pmemory64++ ^ *psource64++;
			xcompare64 |= *pmemory64 ^ *psource64;

			if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0xf)))
			{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_R3000A
				rs->clear_hwrw_bitmap(PC);
#else
				rs->set_hwrw_bitmap(PC);
#endif

				rs->Recompile(PC);
			}

		} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

	} // if ( bEnableRecompiler )
}

void Cpu::ExecuteCacheMiss()
{
	// cache miss //

	u32 StartPC;
	u32* pCacheLine;
	u32* pMemPtr32;

	StartPC = PC & 0x1ffffff0;
	pCacheLine = ICache.GetCacheLinePtr(StartPC);

	// get pointer to instructions in memory
	pMemPtr32 = (u32*)Bus->GetIMemPtr(StartPC);
	pCacheLine[0] = pMemPtr32[0];
	pCacheLine[1] = pMemPtr32[1];
	pCacheLine[2] = pMemPtr32[2];
	pCacheLine[3] = pMemPtr32[3];

	// validate cache lines

	ICache.ValidateCacheLine(PC);

	// update cycles (this would update to the Bus->BusyUntil_Cycle since the bus was free, so no other action needed)
	CycleCount += Bus->GetLatency();


	if (bEnableRecompiler)
	{

		if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
		{

			// read from pc in main memory, compare instructions

			// make sure that
			u64 xcompare64;

			// get pointer into main memory
			u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0xf]);

			// get pointer into the source code that was recompiled
			u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0xf);

			xcompare64 = *pmemory64++ ^ *psource64++;
			xcompare64 |= *pmemory64 ^ *psource64;

			if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0xf)))
			{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_R3000A
				rs->clear_hwrw_bitmap(PC);
#else
				rs->set_hwrw_bitmap(PC);
#endif

				rs->Recompile(PC);
			}


		} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

	} // if ( bEnableRecompiler )

	// get the memory area (ram/bios)

	// *** testing *** load the instruction from cache //
	CurInst.Value = pMemPtr32[(PC >> 2) & 0x3];
}


void Cpu::ExecuteExceptionBlock()
{
	u32 uSrcAddr;

	u32 uBlockStartAddr;
	u64 uSrcCycles;

	// get addr that block starts at
	uBlockStartAddr = PC & ~0xf;

	// search for instruction that caused exception in pCodeStart //

	// get pointer to list of code starts
	u8** pCurCodeStart = rs->get_code_start_ptr(uBlockStartAddr);

	// check which source instr caused the exception
	int i;
	for (i = 1; i < 4; i++)
	{

		if (pCurCodeStart[i] > ((u8*)uExceptionInstrAddr64))
		{
			break;
		}
	}

	// get the source ps1 address that affected R3000A instruction is at
	uSrcAddr = uBlockStartAddr + ((i - 1) << 2);

	// get the cycles to update after exception
	//uSrcCycles = uExceptionCycles64;
	uSrcCycles = rs->get_cycle_count(uSrcAddr);

	if (PC == uSrcAddr)
	{
		// no instruction was executed yet //

		// interpret the instruction
		//CurInst.Value = Bus->Read_t<0xffffffff>(PC);
		//NextPC = PC + 4;
		Instruction::Execute::ExecuteInstruction(CurInst);

		// instruction that was interrupted on was executed //
		//PC = NextPC;
		//NextPC = PC + 4;

		CycleCount += uSrcCycles;
	}
	else
	{
		// instruction that was interrupted on was NOT executed //

		// update PC/NextPC to the instruction that had the exception
		PC = uSrcAddr;
		NextPC = uSrcAddr;

		CycleCount += uSrcCycles - 1;
	}

	// mark the problem instruction in the bitmap //
	rs->bitset_hwrw_bitmap(uSrcAddr);


	// recompile the block while the instruction is marked //
	rs->Recompile(uBlockStartAddr);
}

// use for running cpu
void Cpu::Run ()
{
#ifdef INLINE_DEBUG_TESTING
	debug << "\r\nCpu::Run";
#endif

	static u32* pInstrPtr;
	static u32* pCacheLine;
	u64 InstrLatency;
	u32 Index;
	u32 StartPC;
	unsigned long ret;

	u32* pMemPtr32;

#ifdef INLINE_DEBUG
	debug << "\r\n->PC = " << hex << setw( 8 ) << PC << dec;
#endif

#ifdef VERBOSE_RECOMPILE
	cout << "\nR3000A";
#endif


	//////////////////////
	// Load Instruction //
	//////////////////////
	
	// load next instruction to be executed
	// step 0: check if instruction is in a cached location
	if (ICache_Device::isCached(PC))
	{
		// instruction is in a cached location //

#ifdef INLINE_DEBUG
		debug << ";isCached";
#endif

#ifdef VERBOSE_RECOMPILE
		cout << "-CACHED";
#endif

		// check if there is a cache hit
		if (!ICache.isCacheHit(PC))
		{
			// cache miss //

#ifdef INLINE_DEBUG
			debug << ";MISS";
#endif

#ifdef ENABLE_R3000A_PROFILING

			ExecuteCacheMiss();

#else

			StartPC = PC & 0x1ffffff0;
			pCacheLine = ICache.GetCacheLinePtr(StartPC);
			//pCacheLine[0] = Bus->Read_t<0xffffffff>(StartPC + 0);
			//pCacheLine[1] = Bus->Read_t<0xffffffff>(StartPC + 4);
			//pCacheLine[2] = Bus->Read_t<0xffffffff>(StartPC + 8);
			//pCacheLine[3] = Bus->Read_t<0xffffffff>(StartPC + 12);

			// get pointer to instructions in memory
			pMemPtr32 = (u32*)Bus->GetIMemPtr(StartPC);
			pCacheLine[0] = pMemPtr32[0];
			pCacheLine[1] = pMemPtr32[1];
			pCacheLine[2] = pMemPtr32[2];
			pCacheLine[3] = pMemPtr32[3];

			// validate cache lines

#ifdef INLINE_DEBUG
			debug << ";ICache Validate;HIT";
#endif

			ICache.ValidateCacheLine(PC);

			// update cycles (this would update to the Bus->BusyUntil_Cycle since the bus was free, so no other action needed)
			CycleCount += Bus->GetLatency();

#ifndef FORCE_ENABLE_RECOMPILER_R3000A
			if (bEnableRecompiler)
#endif
			{

				if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
				{

					// read from pc in main memory, compare instructions

					// make sure that
					u64 xcompare64;

					// get pointer into main memory
					u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0xf]);

					// get pointer into the source code that was recompiled
					u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0xf);

					xcompare64 = *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64 ^ *psource64;

					if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0xf)))
					{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_R3000A
						rs->clear_hwrw_bitmap(PC);
#else
						rs->set_hwrw_bitmap(PC);
#endif

						rs->Recompile(PC);
					}


				} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

			} // if ( bEnableRecompiler )

			// get the memory area (ram/bios)

			// *** testing *** load the instruction from cache //
			//CurInst.Value = ICache.Read(PC);
			CurInst.Value = pMemPtr32[(PC >> 2) & 0x3];

#endif
		}
		else
		{
			// cache hit //

#ifdef INLINE_DEBUG
			debug << ";HIT";
#endif

			CurInst.Value = ICache.Read(PC);

		}

	}
	else
	{
		///////////////////////////////////////////////
		// instruction is not in cached location

#ifdef INLINE_DEBUG
		debug << ";!isCached";
#endif

		// attempt to load instruction from bus

		// handle load/store operations first

#ifdef ENABLE_R3000A_PROFILING

		ExecuteNotCached();

#else
		// the bus is free and there are no pending load/store operations
		// Important: When reading from the bus, it has already been determined whether address is in I-Cache or not, so clear top 3 bits
		//CurInst.Value = Bus->Read ( PC & 0x1fffffff );
		CurInst.Value = Bus->Read_t<0xffffffff>(PC & 0x1fffffff);

		// cpu should be suspended while loading from memory //
		CycleCount += Bus->GetLatency();


#ifndef FORCE_ENABLE_RECOMPILER_R3000A
		if (bEnableRecompiler)
#endif
		{

			if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
			{

				// read from pc in main memory, compare instructions

				// make sure that
				u64 xcompare64;

				// get pointer into main memory
				u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0xf]);

				// get pointer into the source code that was recompiled
				u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0xf);

				xcompare64 = *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64 ^ *psource64;

				if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0xf)))
				{
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_R3000A
					rs->clear_hwrw_bitmap(PC);
#else
					rs->set_hwrw_bitmap(PC);
#endif

					rs->Recompile(PC);
				}

			} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

		} // if ( bEnableRecompiler )

#endif

	}
	
	
	/////////////////////////
	// Execute Instruction //
	/////////////////////////

#ifdef INLINE_DEBUG
	debug << ";Execute";
#endif

	// execute instruction
	NextPC = PC + 4;

	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	GPR[0].u = 0;

#ifdef DISABLE_RECOMPILER_R3000A

	if (CurInst.Value)
	{

		// interpret the instruction
		Instruction::Execute::ExecuteInstruction(CurInst);
	}

#else

#ifndef FORCE_ENABLE_RECOMPILER_R3000A
	//if (!bEnableRecompiler)
	if (Status.Value)
	{

		if (CurInst.Value)
		{

			// interpret the instruction
			Instruction::Execute::ExecuteInstruction(CurInst);
		}

	}
	else
#endif
	{


#ifdef VERBOSE_RECOMPILE
		cout << "-CHECK";
#endif


		// check that address block is encoded
		if (!rs->isRecompiled(PC))
		{
			// address is NOT encoded //
#ifdef VERBOSE_RECOMPILE
			cout << "-AddressNOTEncoded: Recompile: PC=" << hex << PC;
#endif
#ifdef INLINE_DEBUG
			debug << ";Recompile(PC=" << hex << PC << ")";
#endif

			// recompile block
#ifdef ENABLE_PS1_VIRTUAL_MACHINE_R3000A
			rs->clear_hwrw_bitmap(PC);
#else
			rs->set_hwrw_bitmap(PC);
#endif

			rs->Recompile(PC);
		}


#ifdef VERBOSE_RECOMPILE
		cout << "-Execute: PC=" << hex << PC;
#endif
#ifdef INLINE_DEBUG
		debug << ";Execute(PC=" << hex << PC << ")";
#endif



		__try
		{

#ifdef ENABLE_R3000A_PROFILING

			ExecuteRecompiledCode();

#else

			// get the block index
			//Block = Get_Block ( PC );
			Index = rs->Get_Index(PC);

			// offset cycles before the run, so that it updates to the correct value
			CycleCount -= rs->CycleCount[Index];

			// already checked that is was either in cache, or etc
			// execute from address
			((func2)(rs->pCodeStart[Index])) ();
#endif
		}
		__except (ExceptionFilter(GetExceptionInformation()))
		{
#ifdef ENABLE_R3000A_PROFILING

			ExecuteExceptionBlock();

#else

			u32 uSrcAddr;

			u32 uBlockStartAddr;
			u64 uSrcCycles;

			// get addr that block starts at
			uBlockStartAddr = PC & ~0xf;

			//#define VERBOSE_VIRTUAL_MACHINE
#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\nhpsx64: EXCEPTION: Exception code: " << hex << GetExceptionCode();
			cout << "\nException read/write address: " << hex << uExceptionRWAddr64 << " from code at address: " << uExceptionInstrAddr64;
			cout << "\nR3000A PC: " << hex << PC << " NextPC: " << NextPC << " LastPC: " << LastPC;
			cout << "\nCycle#" << dec << CycleCount;
			//cout << "\nhpsx64: EXCEPTION: Accessing address: " << hex << addr;
#endif

			// search for instruction that caused exception in pCodeStart //

			// get pointer to list of code starts
			u8** pCurCodeStart = rs->get_code_start_ptr(uBlockStartAddr);

			// check which source instr caused the exception
			int i;
			for (i = 1; i < 4; i++)
			{
#ifdef VERBOSE_VIRTUAL_MACHINE
				cout << "\n#" << i << hex << " Comparing code start: " << (u64)(pCurCodeStart[i]) << " with exception rip: " << uExceptionInstrAddr64;
#endif

				if (pCurCodeStart[i] > ((u8*)uExceptionInstrAddr64))
				{
					break;
				}
			}

			// get the source ps1 address that affected R3000A instruction is at
			uSrcAddr = uBlockStartAddr + ((i - 1) << 2);

#ifdef VERBOSE_VIRTUAL_MACHINE
			//u32 Instr0 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) - 1];
			//u32 Instr1 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) + 0];
			//u32 Instr2 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) + 1];
			//u32 Instr0 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) - 1];
			//u32 Instr1 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 0];
			//u32 Instr2 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 1];

			u32 Instr0 = Bus->MainMemory.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) - 1];
			u32 Instr1 = Bus->MainMemory.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 0];
			u32 Instr2 = Bus->MainMemory.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 1];

			cout << "\nthe instructions are:";
			cout << "\n#0: " << hex << Instr0;
			cout << "\n#1: " << hex << Instr1;
			cout << "\n#2: " << hex << Instr2;

#endif

			// get the cycles to update after exception
			//uSrcCycles = uExceptionCycles64;
			uSrcCycles = rs->get_cycle_count(uSrcAddr);

			if (PC == uSrcAddr)
			{
				// no instruction was executed yet //

				// interpret the instruction
				//CurInst.Value = Bus->Read_t<0xffffffff>(PC);
				//NextPC = PC + 4;
				Instruction::Execute::ExecuteInstruction(CurInst);

				// instruction that was interrupted on was executed //
				//PC = NextPC;
				//NextPC = PC + 4;

				CycleCount += uSrcCycles;
			}
			else
			{
				// instruction that was interrupted on was NOT executed //

				// update PC/NextPC to the instruction that had the exception
				PC = uSrcAddr;
				NextPC = uSrcAddr;

				CycleCount += uSrcCycles - 1;
			}


#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\nStart addr for the block: " << hex << uBlockStartAddr;
			cout << "\nPS1 Source Address for affected instruction: " << uSrcAddr;
#endif

#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\n*OLD* Bitmap for recompile block: " << rs->get_hwrw_bitmap(uSrcAddr);
			cout << "\n*OLD* Bit check for exception instruction: " << rs->bitget_hwrw_bitmap(uSrcAddr);
#endif


			// mark the problem instruction in the bitmap //
			rs->bitset_hwrw_bitmap(uSrcAddr);

#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\n*NEW* Bitmap for recompile block: " << rs->get_hwrw_bitmap(uSrcAddr);
			cout << "\n*NEW* Bit check for exception instruction: " << rs->bitget_hwrw_bitmap(uSrcAddr);
#endif

			// recompile the block while the instruction is marked //
			rs->Recompile(uBlockStartAddr);

#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\n*NEW* R3000A PC: " << hex << PC << " NextPC: " << NextPC << " LastPC: " << LastPC;
			cout << "\nCycle#" << dec << CycleCount;
			//cout << "\nhpsx64: EXCEPTION: Accessing address: " << hex << addr;
#endif

#endif	// end #ifdef ENABLE_R3000A_PROFILING
		}


#ifdef INLINE_DEBUG
		debug << "->ExecuteDone(NextPC=" << hex << NextPC << ")";
#endif


	} // end if (Status.Value) else

#endif
		
	///////////////////////////////////////////////////
	// Check if there is anything else going on
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Check delay slots
	// need to do this before executing instruction because load delay slot might stall pipeline

	// check if anything is going on with the delay slots
	// *note* by doing this before execution of instruction, pipeline can be stalled as needed
	if (Status.isSomethingBusy)
	{

#ifdef ENABLE_R3000A_PROFILING

		ExecuteSomethingBusy();

#else
		/////////////////////////////////////////////
		// check for anything in delay slots

		if (Status.DelaySlot_Valid)
		{
#ifdef INLINE_DEBUG
			debug << ";Delay1.Value";
#endif


			if (Status.DelaySlot_Valid & 1)
			{
#ifdef INLINE_DEBUG
				debug << ";Delay1.Value";
#endif

				DelaySlots[NextDelaySlotIndex].cb();
				DelaySlots[NextDelaySlotIndex].Value = 0;
			}

			///////////////////////////////////
			// move delay slot
			NextDelaySlotIndex ^= 1;

			///////////////////////////////////////////////////
			// Advance status bits for checking delay slot
			Status.DelaySlot_Valid >>= 1;

		} // end if ( Status.DelaySlot_Valid )


		//////////////////////////////////////////////
		// check for Asynchronous Interrupts
		// also make sure interrupts are enabled
		if (Status.isExternalInterrupt)
		{
			//Status.isExternalInterrupt >>= 1;

			//if (!Status.isExternalInterrupt)
			{

				///////////////////////
				// *** IMPORTANT *** /////////////////////////////////////////////////////////////////////
				// *** PS1 must NOT be triggering Async Interrupt when GTE opcode is next to execute ** //
				if (
					// don't interrupt when it is a gte instruction
					(((Bus->Read(NextPC) >> 24) & 0xfe) != 0x4a)
					&&
					// todo: what if in load delay slot ??
					!(
						(DelaySlots[NextDelaySlotIndex].Instruction.Value >= 0x40000000)
						&&
						(Status.DelaySlot_Valid & 1)
					)
				)
				{
#ifdef PS2_COMPILE
#ifdef ENABLE_R3000A_IDLE
					// cpu not idle if there is an interrupt
					ulWaitingForInterrupt = 0;
#endif
#endif

					// interrupt handled
					Status.isExternalInterrupt = 0;

					if ((CPR0.Status.IEc) && (CPR0.Status.Value & CPR0.Cause.l & 0xff00))
					{
						CycleCount++;


						LastPC = PC;
						PC = NextPC;

						// interrupt has been triggered
						ProcessAsynchronousInterrupt();


#ifdef INLINE_DEBUG_TESTING
						debug << "\r\nCpu::Run->AsyncInt";
						//debug << "\r\nBus->MainMemory.b32 [ 0x269f0 >> 2 ]=" << hex << Bus->MainMemory.b32 [ 0x269f0 >> 2 ];
#endif

						return;
					}

				}	// end if ( ( ( Bus->Read ( NextPC ) >> 24 ) & 0xfe ) != 0x4a )

			}	// end if ( !Status.isExternalInterrupt )

		}	// end if (Status.isExternalInterrupt)

#endif	// end #ifdef ENABLE_R3000A_PROFILING

	}	// end if (Status.isSomethingBusy)

	
	/////////////////////////////////////
	// Update Program Counter

	LastPC = PC;
	PC = NextPC;

	// update cpu cycle#	
	CycleCount++;

}




// ------------------------------------------------------------------------------------


void Cpu::ProcessBranchDelaySlot ()
{
	Instruction::Format i;
	u32 Address;
	
	//i = DelaySlot1.Instruction;
	i = DelaySlots [ NextDelaySlotIndex ].Instruction;
	
	switch ( i.Opcode )
	{
		case OPREGIMM:
		
			switch ( i.Rt )
			{
				case RTBLTZ:	// regimm
				case RTBGEZ:	// regimm
					NextPC = PC + ( i.sImmediate << 2 );
					break;

				case RTBLTZAL:	// regimm
				case RTBGEZAL:	// regimm
					// *** TESTING ***
					//if ( GPR [ 31 ].u != ( PC + 4 ) ) Cpu::DebugStatus.Stop = true;
					
					//GPR [ 31 ].u = PC + 4; // *note* this is already handled properly when instruction is first encountered
					NextPC = PC + ( i.sImmediate << 2 );
					break;
			}
			
			break;
		
		case OPSPECIAL:
		
			switch ( i.Funct )
			{
				case SPJR:
				case SPJALR:
					//NextPC = DelaySlot1.Data;
					NextPC = DelaySlots [ NextDelaySlotIndex ].Data;
					break;
			}
			
			break;
			
	
		case OPJ:	// ok
			NextPC = ( 0xf0000000 & PC ) | ( i.JumpAddress << 2 );
			break;
			
			
		case OPJAL:	// ok
			// *** TESTING ***
			//if ( GPR [ 31 ].u != ( PC + 4 ) ) Cpu::DebugStatus.Stop = true;
			
			//GPR [ 31 ].u = PC + 4; // *note* this is already handled properly where it is supposed to be handled
			NextPC = ( 0xf0000000 & PC ) | ( i.JumpAddress << 2 );
			break;
			
		case OPBEQ:
		case OPBNE:
		case OPBLEZ:
		case OPBGTZ:
			NextPC = PC + ( i.sImmediate << 2 );
			break;
			
	}
	
	// testing
	DelaySlots [ NextDelaySlotIndex ].Value = 0;
	
#ifdef INLINE_DEBUG_COUT
	if ( NextPC == 0x000000a0 ) 
	{
		if ( GPR [ 9 ].u < ( sizeof(bioscallsA0) / sizeof(bioscallsA0[0]) ) )
		{
			Debug_CallStack_Address [ CallStackDepth ] = NextPC;
			Debug_CallStack_Function [ CallStackDepth ] = GPR [ 9 ].u;
			Debug_CallStack_ReturnAddress [ CallStackDepth ] = GPR [ 31 ].u;
			CallStackDepth++;
			CallStackDepth &= ( CallStack_Size - 1 );
			
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nDepth: " << dec << CallStackDepth << ";BiosCallA0; " << hex << setw(8) << PC << " " << dec << CycleCount << " r31=" << hex << GPR [ 31 ].u << " " << bioscallsA0 [ GPR [ 9 ].u ].prototype;
	#endif
			
			switch ( GPR [ 9 ].u )
			{
				case 0x17:
					//int strcmp(const char *dst, const char *src)
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\ndst=" << hex << setw(8) << GPR [ 4 ].u << "; src=" << GPR [ 5 ].u;
	#endif
					break;
				
				case 0x25:
					//int toupper(int c)
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nc=" << (char) GPR [ 4 ].u;
	#endif
					break;
					
				case 0x2a:
					//void *memcpy(void *dst, const void *src, size_t n)
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\ndst=" << hex << setw(8) << GPR [ 4 ].u << "; src=" << setw(8) << GPR [ 5 ].u << "; n=" << GPR [ 6 ].u;
	#endif
					break;
					
				case 0x3f:
					//int printf(const char *fmt, ...)
					
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nfmt=" << hex << setw(8) << GPR [ 4 ].u << " string is: " << ( (const char*) (Bus->GetPointer( GPR [ 4 ].u )) );
					debugBios << " a1=" << GPR [ 5 ].u << " a2=" << GPR [ 6 ].u << " a3=" << GPR [ 7 ].u;
	#endif
					
					break;
			
				default:
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nt1=" << hex << GPR [ 9 ].u << "; a0=" << hex << GPR [ 4 ].u << "; a1=" << GPR [ 5 ].u << "; a2=" << GPR [ 6 ].u << "; a3=" << GPR [ 7 ].u;
	#endif
					break;
			}
		}
		else
		{
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nBiosCallA0; Unknown; t1 = " << hex << GPR [ 9 ].u;
	#endif
		}
	}
	else if ( NextPC == 0x000000b0 )
	{
		if ( GPR [ 9 ].u < ( sizeof(bioscallsB0) / sizeof(bioscallsB0[0]) ) )
		{
			Debug_CallStack_Address [ CallStackDepth ] = NextPC;
			Debug_CallStack_Function [ CallStackDepth ] = GPR [ 9 ].u;
			Debug_CallStack_ReturnAddress [ CallStackDepth ] = GPR [ 31 ].u;
			CallStackDepth++;
			CallStackDepth &= ( CallStack_Size - 1 );
			
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nDepth: " << dec << CallStackDepth << ";BiosCallB0; " << hex << setw(8) << PC << " " << dec << CycleCount << " r31=" << hex << GPR [ 31 ].u << " " << bioscallsB0 [ GPR [ 9 ].u ].prototype;
	#endif
			
			switch ( GPR [ 9 ].u )
			{
				case 0x3d:
					//putchar(char ch)
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nch=" << (char) GPR [ 4 ].u;
	#endif
					// if newline, then output cycle
					if ( GPR [ 4 ].u == '\n' )
					{
						// *** output cycle ***
						// disabling display of cycle count
						//out << " CycleCount=" << dec << CycleCount;
					}
					
					putchar ( GPR [ 4 ].u );
					
					break;
					
				case 0x3f:
					//puts(const char *s)
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\ns=" << hex << setw(8) << GPR [ 4 ].u << " string is: " << ( (const char*) (Bus->GetPointer( GPR [ 4 ].u )) );
	#endif
					
					puts ( (const char*) (Bus->GetPointer( GPR [ 4 ].u )) );
					
					// *** output cycle ***
					// disabling display of cycle count
					//cout << " CycleCount=" << dec << CycleCount << "\n";
					
					break;
							
				default:
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nt1=" << hex << GPR [ 9 ].u << "; a0=" << hex << GPR [ 4 ].u << "; a1=" << GPR [ 5 ].u << "; a2=" << GPR [ 6 ].u << "; a3=" << GPR [ 7 ].u;
	#endif
					break;
			}
		}
		else
		{
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nBiosCallB0; Unknown; t1 = " << hex << GPR [ 9 ].u;
	#endif
		}
	}
	else if ( NextPC == 0x000000c0 )
	{
		if ( GPR [ 9 ].u < ( sizeof(bioscallsC0) / sizeof(bioscallsC0[0]) ) )
		{
			Debug_CallStack_Address [ CallStackDepth ] = NextPC;
			Debug_CallStack_Function [ CallStackDepth ] = GPR [ 9 ].u;
			Debug_CallStack_ReturnAddress [ CallStackDepth ] = GPR [ 31 ].u;
			CallStackDepth++;
			CallStackDepth &= ( CallStack_Size - 1 );
			
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nDepth: " << dec << CallStackDepth << ";BiosCallC0; " << hex << setw(8) << PC << " " << dec << CycleCount << " r31=" << hex << GPR [ 31 ].u << " " << bioscallsC0 [ GPR [ 9 ].u ].prototype;
			debugBios << "\r\nt1=" << hex << GPR [ 9 ].u << "; a0=" << hex << GPR [ 4 ].u << "; a1=" << GPR [ 5 ].u << "; a2=" << GPR [ 6 ].u << "; a3=" << GPR [ 7 ].u;
	#endif
		}
		else
		{
	#ifdef INLINE_DEBUG_BIOS
			debugBios << "\r\nBiosCallC0; Unknown; t1 = " << hex << GPR [ 9 ].u;
	#endif
		}
	}
	
	// also check if returning from a function on call stack
	if ( CallStackDepth > 0 )
	{
		if ( NextPC == Debug_CallStack_ReturnAddress [ CallStackDepth - 1 ] )
		{
			CallStackDepth--;
			// show that we are returning
			switch ( Debug_CallStack_Address [ CallStackDepth ] )
			{
				case 0xa0:
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nreturn: " << bioscallsA0 [ Debug_CallStack_Function [ CallStackDepth ] ].prototype;
					debugBios << ";v0=" << hex << GPR [ 2 ].u << " " << hex << setw(8) << NextPC << " " << dec << CycleCount;
	#endif
					break;
				
				case 0xb0:
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nreturn: " << bioscallsB0 [ Debug_CallStack_Function [ CallStackDepth ] ].prototype;
					debugBios << ";v0=" << hex << GPR [ 2 ].u << " " << hex << setw(8) << NextPC << " " << dec << CycleCount;
	#endif
					break;

				case 0xc0:
	#ifdef INLINE_DEBUG_BIOS
					debugBios << "\r\nreturn: " << bioscallsC0 [ Debug_CallStack_Function [ CallStackDepth ] ].prototype;
					debugBios << ";v0=" << hex << GPR [ 2 ].u << " " << hex << setw(8) << NextPC << " " << dec << CycleCount;
	#endif
					break;
			}
			
		}
	}
#endif
}





void Cpu::Write_MTC0 ( u32 Register, u32 Value )
{
	/////////////////////////////////////////////////////////////////////
	// Process storing to COP0 registers
	switch ( Register )
	{
		case 12:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Status" << ";Before=" << CPR0.Regs [ 12 ] << ";Writing=" << hex << Value;
#endif
		
			///////////////////////////////////////////////////
			// Status register
			
			// software can't modify bits 1-7,10-15,19-21,23-24,26-27
			// *note* Asynchronous interrupt will be checked for in main processor loop
			CPR0.Regs [ 12 ] = ( CPR0.Regs [ 12 ] & StatusReg_WriteMask ) | ( Value & (~StatusReg_WriteMask) );
			
			/*
			if ( CPR0.Status.SwC )
			{
				cout << "\nhps1x64 ALERT: SwC -> it is invalidting I-Cache entries\n";
			}
			*/
			
			// whenever interrupt related stuff is messed with, must update the other interrupt stuff
			UpdateInterrupt ();
			
#ifdef INLINE_DEBUG_LOAD
			debug << ";After=" << hex << CPR0.Regs [ 12 ];
#endif

			break;
								
		case 13:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Cause";
#endif
		
			////////////////////////////////////////////////////
			// Cause register
			// ** note ** this register is READ-ONLY except for 2 software writable interrupt bits
			
			// software can't modify bits 0-1,7,10-31
			// software can only modify bits 8 and 9
			// *note* Asynchronous interrupt will be checked for in main processor loop
			CPR0.Regs [ 13 ] = ( CPR0.Regs [ 13 ] & CauseReg_WriteMask ) | ( Value & (~CauseReg_WriteMask) );
			
			// whenever interrupt related stuff is messed with, must update the other interrupt stuff
			UpdateInterrupt ();
			
			break;
			
		case 15:
#ifdef INLINE_DEBUG_LOAD
			debug << ";PRId";
#endif
							
			///////////////////////////////////////////////////
			// PRId register
			// this is register is READ-ONLY
			break;
			
		default:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Other";
#endif
		
			CPR0.Regs [ Register ] = Value;
			break;
	}
}


// returns false if load buffer is full
void Cpu::ProcessLoadDelaySlot ()
{
#ifdef INLINE_DEBUG_LOAD
		debug << "\r\nCPU::ProcessLoadDelaySlot " << hex << PC << " " << dec << CycleCount;
#endif

	Instruction::Format i;
	u32 Data;
	u32 Temp;

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;

	// get the instruction
	//i = DelaySlot1.Instruction;
	i = DelaySlots [ NextDelaySlotIndex ].Instruction;
	
	Data = DelaySlots [ NextDelaySlotIndex ].Data;
	
	// check if this is a load OP
	if ( i.Opcode >= 0x20 )
	{
		/////////////////////////
		// load instruction
		
#ifdef INLINE_DEBUG_LOAD
		debug << ";Load";
#endif
		
		//DelaySlot1.cb ();
		//return;
		
		// check if address is for data cache
		//LoadAddress = DelaySlot1.Data;
		
		// !!! Important !!!
		// there is no data cache, just scratch pad ram on PS1, so clear top 3 bits of address
		//LoadAddress &= 0x1fffffff;
		Data &= 0x1fffffff;
		
#ifdef INLINE_DEBUG_LOAD
		//debug << ";LoadAddress=" << hex << LoadAddress;
		debug << ";LoadAddress=" << hex << Data;
#endif

		
		
		if ( isDCache ( Data ) )
		{
#ifdef INLINE_DEBUG_LOAD
			debug << ";D$";
#endif

			// load data from d-cache
			switch ( i.Opcode )
			{
				case OPLB:
				
					// if register was modified in delay slot, then cancel load
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LB Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
		
					GPR [ i.Rt ].s = (s32) ((s8) (DCache.b8 [ Data & ( c_ScratchPadRam_Size - 1 ) ]));
					break;
					
				case OPLH:
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LH Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
		
					GPR [ i.Rt ].s = (s32) ((s16) (DCache.b16 [ ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 1 ]));
					break;
					
				case OPLW:
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LW Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
		
					GPR [ i.Rt ].u = DCache.b32 [ ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 2 ];
					break;
					
				case OPLBU:
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LBU Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
		
					GPR [ i.Rt ].u = (u32) DCache.b8 [ Data & ( c_ScratchPadRam_Size - 1 ) ];
					break;
					
				case OPLHU:
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LHU Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
		
					GPR [ i.Rt ].u = (u32) DCache.b16 [ ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 1 ];
					break;
					
				// no LWL/LWR since this is done during first execution of instruction
				// actually, a PS1 seems to have an LWL/LWR delay slot, even though it is not supposed to
				case OPLWL:
					// don't break if LWL or LWR
					// no, actually this should happen
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LWL Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
					
					Type = 3 - ( Data & 3 );
					Offset = ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 2;
					GPR [ i.Rt ].u = ( DCache.b32 [ Offset ] << ( Type << 3 ) ) | ( GPR [ i.Rt ].u & ~( c_Mask << ( Type << 3 ) ) );
				
					break;
					
				case OPLWR:
					//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister )
					if ( i.Rt == LastModifiedRegister )
					{
#ifdef COUT_DELAYSLOT_CANCEL
						cout << "\nhps1x64 ALERT: LWR Reg#" << dec << i.Rt << " was modified in load (from D$) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif

						break;
					}
					
					Type = Data & 3;
					Offset = ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 2;
					GPR [ i.Rt ].u = ( DCache.b32 [ Offset ] >> ( Type << 3 ) ) | ( GPR [ i.Rt ].u & ~( c_Mask >> ( Type << 3 ) ) );
				
					break;
				
				case OPLWC2:
					//COP2.CPR2.Regs [ i.Rt ] = DCache.b32 [ ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 2 ];
					COP2.Write_MTC ( i.Rt, DCache.b32 [ ( Data & ( c_ScratchPadRam_Size - 1 ) ) >> 2 ] );
					
					// mark register as ready for use in COP2
					//COP2.CPRLoading_Bitmap &= ~( 1 << i.Rt );
					
					break;
			}
			
			if ( !i.Rt ) GPR [ 0 ].u = 0;
		}
		else
		{
		
#ifdef INLINE_DEBUG_LOAD
			debug << ";BUS";
#endif			
			
			// perform the load
			ProcessExternalLoad ();
			
		}
		
#ifdef INLINE_DEBUG_LOAD
			debug << ";CLEARING_DELAY_SLOT1";
#endif

		// must do this in case I need to call this while executing an instruction (JAL,JALR,ETC)
		//DelaySlot1.Value = 0;
		DelaySlots [ NextDelaySlotIndex ].Value = 0;
		
		//Status.DelaySlot_Valid &= 1;
		Status.DelaySlot_Valid &= 2;
	}
	else if ( i.Opcode >= 0x10 )
	{
		//////////////////////////////////////////////////////
		// COP instruction
	
#ifdef INLINE_DEBUG_LOAD
		debug << ";COP";
		debug << "; Instr=" << Print::PrintInstruction ( i.Value ).c_str ();
		debug << "; (before) Rt=" << hex << GPR [ i.Rt ].u;
#endif

#ifdef USE_DELAY_CALLBACKS

		//DelaySlot1.cb ();
		DelaySlots [ NextDelaySlotIndex ].cb ();
		
#else
		
#ifdef INLINE_DEBUG_LOAD
		debug << "; (after) Rt(CPU)=" << hex << GPR [ i.Rt ].u << " Rd(COP)=" << CPR0.Regs [ i.Rd ] << " LMR=" << dec << LastModifiedRegister << " Fnc=" << hex << ((u64)DelaySlots [ NextDelaySlotIndex ].cb);
#endif

		// CFC/CTC/MFC/MTC
		switch ( i.Rs )
		{
			case RSCFC2:
			case RSMFC0:
				// if register was modified in delay slot, then cancel load
				//if ( DelaySlot1.Instruction.Rt == LastModifiedRegister ) break;
				
				//GPR [ i.Rt ].u = DelaySlot1.Data;
				GPR [ i.Rt ].u = Data;
				
				if ( !i.Rt ) GPR [ 0 ].u = 0;
				break;

			case RSCTC2:
				
				//COP2.Write_CTC ( i.Rd, DelaySlot1.Data );
				COP2.Write_CTC ( i.Rd, Data );
				break;
				
			case RSMTC0:
			
				switch ( i.Opcode & 0x3 )
				{
					case 0:
#ifdef INLINE_DEBUG_LOAD
						debug << ";MTC0";
#endif
					
						//Write_MTC0 ( i.Rd, DelaySlot1.Data );
						Write_MTC0 ( i.Rd, Data );
						
						break;
						
					case 2:
					
						// MTC2

						/////////////////////////////////////////////////////////////////////
						// Process storing to COP2 registers
						//COP2.Write_MTC ( i.Rd, DelaySlot1.Data );
						COP2.Write_MTC ( i.Rd, Data );
						break;

				}
				
				break;
		}	// end switch ( i.Rs )
		
		
		// must do this in case I need to call this while executing an instruction (JAL,JALR,ETC)
		//DelaySlot1.Value = 0;
		DelaySlots [ NextDelaySlotIndex ].Value = 0;
		
		//Status.DelaySlot_Valid &= 1;
		Status.DelaySlot_Valid &= 2;
		
#endif

	}	// end else if ( i.Opcode >= 0x10 )

}


void Cpu::ProcessExternalLoad ()
{
#ifdef INLINE_DEBUG_LOAD
	debug << "\r\nCPU::ProcessExternalLoad; " << hex << PC << " " << dec << CycleCount;
#endif

	static const u32 c_CycleTime = 5;

	Instruction::Format i;
	u32 Address;
	u32 Value;
	u32 Temp;
	u32 Index;
	
	/////////////////////////////////////////
	// process load
	
	
	//i = DelaySlot1.Instruction;
	//Address = DelaySlot1.Data;
	i = DelaySlots [ NextDelaySlotIndex ].Instruction;
	Address = DelaySlots [ NextDelaySlotIndex ].Data;
	
	// clear top 3 bits of address
	Address &= 0x1fffffff;
	
#ifdef INLINE_DEBUG_LOAD
	debug << "; Inst=" << hex << i.Value << "; Address=" << Address << "; Index=" << Index << "; NextIndex=" << NextIndex;
#endif

	// catch #1 - if register was modified in delay slot, then it gets overwritten later in the pipeline
	if ( i.Opcode < 50 )
	{
		// if register was modified in load delay slot, then cancel load
		if ( i.Rt == LastModifiedRegister )
		{
#ifdef COUT_DELAYSLOT_CANCEL
			cout << "\nhps1x64 ALERT: Reg#" << dec << i.Rt << " was modified in load (from bus) delay slot @ Cycle#" << CycleCount << hex << " PC=" << PC;
#endif
			
			return;
		}
	}

	switch ( i.Opcode )
	{
		case OPLB:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LB";
#endif
			
			GPR [ i.Rt ].s = (s8) Bus->Read_t<0xff> ( Address );
			
			break;
		
		case OPLH:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LH";
#endif
			
			GPR [ i.Rt ].s = (s16) Bus->Read_t<0xffff> ( Address );
			
			break;
		
		case OPLW:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LW";
#endif
			
			GPR [ i.Rt ].u = Bus->Read_t<0xffffffff> ( Address );
			
			break;
		
		case OPLBU:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LBU";
#endif
			
			GPR [ i.Rt ].u = (u8) Bus->Read_t<0xff> ( Address );
			
			break;
		
		case OPLHU:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LHU";
#endif
			
			GPR [ i.Rt ].u = (u16) Bus->Read_t<0xffff> ( Address );

			break;
		
		case OPLWL:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LWL";
#endif
			
			Value = Bus->Read_t<0xffffffff> ( Address & ~3 );
			
			Value <<= ( ( 3 - ( Address & 3 ) ) << 3 );
			Temp = GPR [ i.Rt ].u;
			Temp <<= ( ( ( Address & 3 ) + 1 ) << 3 );
			if ( ( Address & 3 ) == 3 ) Temp = 0;
			Temp >>= ( ( ( Address & 3 ) + 1 ) << 3 );
			GPR [ i.Rt ].u = Value | Temp;

			break;
			
		case OPLWR:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LWR";
#endif
			
			Value = Bus->Read_t<0xffffffff> ( Address & ~3 );
			
			Value >>= ( ( Address & 3 ) << 3 );
			Temp = GPR [ i.Rt ].u;
			Temp >>= ( ( 4 - ( Address & 3 ) ) << 3 );
			if ( ( Address & 3 ) == 0 ) Temp = 0;
			Temp <<= ( ( 4 - ( Address & 3 ) ) << 3 );
			GPR [ i.Rt ].u = Value | Temp;
			
			break;
			
		case OPLWC2:
#ifdef INLINE_DEBUG_LOAD
			debug << ";LWC2";
#endif
			
			Value = Bus->Read_t<0xffffffff> ( Address );
			
			//COP2.CPR2.Regs [ i.Rt ] = Value;
			COP2.Write_MTC ( i.Rt, Value );

			break;
	}
	
	// for recompiler for now
	if ( !i.Rt ) GPR [ 0 ].u = 0;
	
	// cpu should be suspended while value is loaded //
	CycleCount += Bus->GetLatency ();
	
#ifdef INLINE_DEBUG_LOAD
	debug << "; ValueLoaded=" << Value;
#endif

}


void Cpu::ProcessSynchronousInterrupt ( u32 ExceptionType )
{
	// *** todo *** Synchronous Interrupts can also be triggered by setting bits 8 or 9 (IP0 or IP1) of Cause register

	//////////////////////////////////////////////////////////////////////////////////
	// At this point:
	// PC points to instruction currently in the process of being executed
	// LastPC points to instruction before the one in the process of being executed
	// NextPC DOES NOT point to the next instruction to execute
	
#ifdef INLINE_DEBUG_SYNC_INT
	debug << "\r\nSynchronous Interrupt PC=" << hex << PC << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
#endif


	// step 3: shift left first 8 bits of status register by 2 and clear top 2 bits of byte
	CPR0.Status.b0 = ( CPR0.Status.b0 << 2 ) & 0x3f;
	
	// step 4: set to kernel mode with interrupts disabled
	CPR0.Status.IEc = 0;
	CPR0.Status.KUc = 1;
	
	// step 5: Store current address (has not been executed yet) into COP0 register "EPC" unless in branch delay slot
	// if in branch delay slot, then set field "BD" in Cause register and store address of previous instruction in COP0 register "EPC"
	// Branch Delay Slot 1 is correct since instruction is still in the process of being executed
	//if ( DelaySlot1.Value && ( DelaySlot1.Instruction.Value < 0x40000000 ) )
	if ( DelaySlots [ NextDelaySlotIndex ].Value
		&&
		( DelaySlots [ NextDelaySlotIndex ].Instruction.Value < 0x40000000 )
		&&
		(Status.DelaySlot_Valid & 1)
		)
	{
		// we are in branch delay slot - instruction in branch delay slot has not been executed yet, since it was "interrupted"
		CPR0.Cause.BD = 1;
		
		// this is actually not the previous instruction, but it is the previous instruction executed
		// this allows for branches in branch delay slots
		//CPR0.EPC = LastPC;
		CPR0.EPC = PC - 4;
		
		// no longer want to execute the branch that is in branch delay slot
		//DelaySlot1.Value = 0;
		DelaySlots [ NextDelaySlotIndex ].Value = 0;
		
		// ***testing*** may need to preserve the interrupts
		//Status.DelaySlot_Valid &= 0xfc;
		Status.DelaySlot_Valid = 0;
	}
	else
	{
		// we are not in branch delay slot
		CPR0.Cause.BD = 0;
		
		// this is synchronous interrupt, so EPC gets set to the instruction that caused the interrupt
		CPR0.EPC = PC;
	}

	// step 6: Set correct interrupt pending bit in "IP" (Interrupt Pending) field of Cause register
	// actually, we need to send an interrupt acknowledge signal back to the interrupting device probably
	
	// step 7: set PC to interrupt vector address
	if ( CPR0.Status.BEV == 0 )
	{
		// check if tlb miss exception - skip for R3000A for now
		
		// set PC with interrupt vector address
		// kseg1 - 0xa000 0000 - 0xbfff ffff : translated to physical address by removing top three bits
		// this region is not cached
		//NextPC = 0x80000080;
		NextPC = c_GeneralInterruptVector;
	}
	else
	{
		// check if tlb miss exception - skip for R3000A for now
		
		// set PC with interrupt vector address
		// kseg0 - 0x8000 0000 - 0x9fff ffff : translated to physical address by removing top bit
		// this region is cached
		//NextPC = 0xbfc00180;
		NextPC = c_BootInterruptVector;
	}
	
	// step 8: set Cause register so that software can see the reason for the exception
	// we know that this is an synchronous interrupt
	//CPR0.Cause.ExcCode = Status.ExceptionType;
	CPR0.Cause.ExcCode = ExceptionType;
	
	// *** todo *** instruction needs to be passed above because lower 2 bits of opcode go into Cause Reg bits 28 and 29


	// step 9: continue execution
	
	// when returning from interrupt you should
	// Step 1: enable interrupts globally
	// Step 2: shift right first 8 bits of status register
	// Step 3: set ExceptionCode to EXC_Unknown
		

#ifdef INLINE_DEBUG_SYNC_INT
	debug << "\r\n->PC=" << hex << PC << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << Print::PrintInstruction ( DelaySlots [ NextDelaySlotIndex ].Instruction.Value ).c_str () << "  " << hex << DelaySlots [ NextDelaySlotIndex ].Instruction.Value;
#endif

#ifdef INLINE_DEBUG_BIOS
	if ( CPR0.Cause.ExcCode == EXC_SYSCALL )
	{
		debugBios << "\r\nBiosCallSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount;
#ifdef PS2_COMPILE
		if ( GPR [ 4 ].u == 0 ) debugBios << "; get_ver ()";
		if ( GPR [ 4 ].u == 1 ) debugBios << "; halt (int mode)";
		if ( GPR [ 4 ].u == 2 ) debugBios << "; setdve(int mode)";
		if ( GPR [ 4 ].u == 3 )
		{
			debugBios << "; putchar(int ch)";
			// char in r5??
			cout << "\niop: " << (char) GPR [ 5 ].u << "\n";
		}
		
		if ( GPR [ 4 ].u == 4 ) debugBios << "; getchar()";
		
		if ( GPR [ 4 ].u == 16 ) debugBios << "; dma_init()";
		if ( GPR [ 4 ].u == 17 ) debugBios << "; dma_exit()";
		
		if ( GPR [ 4 ].u == 32 ) debugBios << "; cmd_init()";
		if ( GPR [ 4 ].u == 33 ) debugBios << "; cmd_exit()";
		
		if ( GPR [ 4 ].u == 48 ) debugBios << "; rpc_init()";
		if ( GPR [ 4 ].u == 49 ) debugBios << "; rpc_exit()";
		
		if ( GPR [ 4 ].u == 51 ) debugBios << "; rpc_bind()";
		
		if ( GPR [ 4 ].u == 64 ) debugBios << "; SBR_IOPH_INIT()";
		if ( GPR [ 4 ].u == 65 ) debugBios << "; alloc_iop_heap()";
		if ( GPR [ 4 ].u == 66 ) debugBios << "; iopmem_free()";
#else
		if ( GPR [ 4 ].u == 0 ) debugBios << "; Exception()";
		if ( GPR [ 4 ].u == 1 ) debugBios << "; EnterCriticalSection()";
		if ( GPR [ 4 ].u == 2 ) debugBios << "; ExitCriticalSection()";
#endif

		debugBios << "; r1 = " << GPR [ 1 ].u;
		debugBios << "; r2 = " << GPR [ 2 ].u;
		debugBios << "; r3 = " << GPR [ 3 ].u;
		debugBios << "; a0/r4 = " << GPR [ 4 ].u;
	}
#endif

}

void Cpu::ProcessAsynchronousInterrupt ()
{

	///////////////////////////////////////////////////////////////////
	// At this point:
	// PC points to instruction just executed
	// LastPC points to instruction before the one just executed
	// NextPC points to the next instruction to execute

#ifdef INLINE_DEBUG_ASYNC_INT
	debug << "\r\nAsynchronous Interrupt PC=" << hex << PC << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
	//debug << "\r\nBus->MainMemory.b32 [ 0x269f0 >> 2 ]=" << hex << Bus->MainMemory.b32 [ 0x269f0 >> 2 ];
#endif
	
	// step 1: make sure interrupts are enabled globally (Status register field "IEc" equal to 1) and not masked
	// and that external interrupts are enabled (Status register bit 10 equal to 1)
	// *** note *** already checked for this stuff
//	if ( ( CPR0.Status.IEc == 1 ) && ( CPR0.Regs [ 12 ] & CPR0.Regs [ 13 ] & 0x4 ) )
//	{
		// step 2: make sure corresponding interrupt bit is set (enabled) in 8-bit Status register field "Im" (Interrupt Mask)
		
		// step 3: shift left first 8 bits of status register by 2 and clear top 2 bits of byte
		CPR0.Status.b0 = ( CPR0.Status.b0 << 2 ) & 0x3f;
		
		// step 4: set to kernel mode with interrupts disabled
		CPR0.Status.IEc = 0;
		CPR0.Status.KUc = 1;

		// step 5: Store current address (has not been executed yet) into COP0 register "EPC" unless in branch delay slot
		// if in branch delay slot, then set field "BD" in Cause register and store address of previous instruction in COP0 register "EPC"
		//if ( DelaySlot1.Value && ( DelaySlot1.Instruction.Value < 0x40000000 ) )
		if ( DelaySlots [ NextDelaySlotIndex ].Value
			&&
			( DelaySlots [ NextDelaySlotIndex ].Instruction.Value < 0x40000000 )
			&&
			(Status.DelaySlot_Valid & 1)
			)
		{
			// we are in branch delay slot - instruction in branch delay slot has not been executed yet, since it was "interrupted"
			CPR0.Cause.BD = 1;
			
			// this is actually not the previous instruction, but it is the previous instruction executed
			// this allows for branches in branch delay slots
			//CPR0.EPC = LastPC;
			// if we are in branch delay slot, then delay slot has branch at this point, so the instruction just executed is the branch
			//CPR0.EPC = PC;
			//CPR0.EPC = LastPC;
			CPR0.EPC = PC - 4;
			
			// no longer want to execute the branch that is in branch delay slot
			//DelaySlot1.Value = 0;
			DelaySlots [ NextDelaySlotIndex ].Value = 0;
			
			// ***testing*** may need to preserve interrupts
			//Status.DelaySlot_Valid &= 0xfc;
			Status.DelaySlot_Valid = 0;
		}
		else
		{
			// we are not in branch delay slot
			CPR0.Cause.BD = 0;
			
			// this is actually not the next instruction, but rather the next instruction to be executed
			//CPR0.EPC = NextPC;
			CPR0.EPC = PC;
		}

		// step 6: Set correct interrupt pending bit in "IP" (Interrupt Pending) field of Cause register
		// actually, we need to send an interrupt acknowledge signal back to the interrupting device probably
		
		// step 7: set PC to interrupt vector address
		if ( CPR0.Status.BEV == 0 )
		{
			// check if tlb miss exception - skip for R3000A for now
			
			// set PC with interrupt vector address
			// kseg0 - 0x8000 0000 - 0x9fff ffff : translated to physical address by removing top bit
			// this region is cached
			//NextPC = 0x80000080;
			PC = c_GeneralInterruptVector;
		}
		else
		{
			// check if tlb miss exception - skip for R3000A for now
			
			// set PC with interrupt vector address
			// kseg1 - 0xa000 0000 - 0xbfff ffff : translated to physical address by removing top three bits
			// this region is not cached
			//NextPC = 0xbfc00180;	// this must be pointing to RAM
			PC = c_BootInterruptVector;
		}
		
		// step 8: set Cause register so that software can see the reason for the exception
		// we know that this is an asynchronous interrupt
		CPR0.Cause.ExcCode = EXC_INT;
	
		// step 9: continue execution
		
		// when returning from interrupt you should
		// Step 1: enable interrupts globally
		// Step 2: shift right first 8 bits of status register
		// - this stuff is done automatically by RFE instruction
//	}

	// interrupt related stuff was modified, so update interrupts
	UpdateInterrupt ();


#ifdef INLINE_DEBUG_ASYNC_INT
	debug << "\r\n->PC=" << hex << PC << " " << dec << CycleCount << hex << " ISTAT=" << *_Debug_IntcStat << " IMASK=" << *_Debug_IntcMask << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ] << " EPC=" << CPR0.EPC;
	debug << "\r\nBranch Delay Instruction: " << Print::PrintInstruction ( DelaySlots [ NextDelaySlotIndex ].Instruction.Value ).c_str () << "  " << hex << DelaySlots [ NextDelaySlotIndex ].Instruction.Value;
	//debug << "\r\nBus->MainMemory.b32 [ 0x269f0 >> 2 ]=" << hex << Bus->MainMemory.b32 [ 0x269f0 >> 2 ];
#endif
}


// when re-loading a state, need to reset the function pointers to the proper locations
Cpu::cbFunction Cpu::Refresh_BranchDelay ( Instruction::Format i )
{
	//Instruction::Format i;
	//u32 Address;
	//i = DelaySlot1.Instruction;
	
	switch ( i.Opcode )
	{
		case OPSPECIAL:
			//case SPJR:
			//case SPJALR:
			
			//_CPU->NextPC = _CPU->DelaySlot1.Data;
			return ProcessBranchDelaySlot_t<OPJR>;
			break;
			
	
		case OPJ:	// ok
		case OPJAL:	// ok
			//_CPU->NextPC = ( 0xf0000000 & _CPU->PC ) | ( _CPU->DelaySlot1.Instruction.JumpAddress << 2 );
			
			return ProcessBranchDelaySlot_t<OPJ>;
			break;
			
		case OPREGIMM:
			//case RTBLTZ:	// regimm
			//case RTBGEZ:	// regimm
			//case RTBLTZAL:	// regimm
			//case RTBGEZAL:	// regimm
		case OPBEQ:
		case OPBNE:
		case OPBLEZ:
		case OPBGTZ:
			//_CPU->NextPC = _CPU->PC + ( _CPU->DelaySlot1.Instruction.sImmediate << 2 );
			
			return ProcessBranchDelaySlot_t<OPBEQ>;
			break;
			
	}

	return NULL;
}



// when re-loading a state, need to reset the function pointers to the proper locations
Cpu::cbFunction Cpu::Refresh_LoadDelay ( Instruction::Format i )
{
	//static const u32 c_Mask = 0xffffffff;
	//u32 Type, Offset;

	//Instruction::Format i;
	//u32 LoadAddress;
	//u32 Temp;
	
	
	// check if this is a load OP
	if ( i.Opcode >= 0x20 )
	{
		/////////////////////////
		// load instruction
		

		switch ( i.Opcode )
		{
			case OPLB:
			
				//_CPU->GPR [ i.Rt ].s = (s32) ((s8) (_CPU->DCache.b8 [ LoadAddress & ( c_ScratchPadRam_Size - 1 ) ]));
				
				return Cpu::ProcessLoadDelaySlot_t<OPLB,0>;
				break;
				
			case OPLH:
			
				//_CPU->GPR [ i.Rt ].s = (s32) ((s16) (_CPU->DCache.b16 [ ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 1 ]));
				
				return Cpu::ProcessLoadDelaySlot_t<OPLH,0>;
				break;
				
			case OPLW:
	
				//_CPU->GPR [ i.Rt ].u = _CPU->DCache.b32 [ ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 2 ];
				
				return Cpu::ProcessLoadDelaySlot_t<OPLW,0>;
				break;
				
			case OPLBU:
	
				//_CPU->GPR [ i.Rt ].u = (u32) _CPU->DCache.b8 [ LoadAddress & ( c_ScratchPadRam_Size - 1 ) ];
				
				return Cpu::ProcessLoadDelaySlot_t<OPLBU,0>;
				break;
				
			case OPLHU:
	
				//_CPU->GPR [ i.Rt ].u = (u32) _CPU->DCache.b16 [ ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 1 ];
				
				return Cpu::ProcessLoadDelaySlot_t<OPLHU,0>;
				break;
				
			// no LWL/LWR since this is done during first execution of instruction
			// actually, a PS1 seems to have an LWL/LWR delay slot, even though it is not supposed to
			case OPLWL:
				
				//Type = 3 - ( LoadAddress & 3 );
				//Offset = ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 2;
				//_CPU->GPR [ i.Rt ].u = ( _CPU->DCache.b32 [ Offset ] << ( Type << 3 ) ) | ( _CPU->GPR [ i.Rt ].u & ~( c_Mask << ( Type << 3 ) ) );
				
				return Cpu::ProcessLoadDelaySlot_t<OPLWL,0>;
				break;
				
			case OPLWR:
				
				//Type = LoadAddress & 3;
				//Offset = ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 2;
				//_CPU->GPR [ i.Rt ].u = ( _CPU->DCache.b32 [ Offset ] >> ( Type << 3 ) ) | ( _CPU->GPR [ i.Rt ].u & ~( c_Mask >> ( Type << 3 ) ) );
				
				return Cpu::ProcessLoadDelaySlot_t<OPLWR,0>;
				break;
			
			case OPLWC2:
				//_CPU->COP2.Write_MTC ( i.Rt, _CPU->DCache.b32 [ ( LoadAddress & ( c_ScratchPadRam_Size - 1 ) ) >> 2 ] );
				
				return Cpu::ProcessLoadDelaySlot_t<OPLWC2,0>;
				break;
		}
		
	}	// end if ( i.Opcode >= 0x20 )
	else if ( i.Opcode >= 0x10 )
	{
		//////////////////////////////////////////////////////
		// COP instruction
	
		
		// CFC/CTC/MFC/MTC
		switch ( i.Rs )
		{
			case RSCFC2:
			case RSMFC0:
				// if register was modified in delay slot, then cancel load
				//if ( i.Rt == _CPU->LastModifiedRegister ) break;
				//_CPU->GPR [ i.Rt ].u = _CPU->DelaySlot1.Data;
				
				return Cpu::ProcessLoadDelaySlot_t<OPCFC2,RSCFC2>;
				break;

			case RSCTC2:
				//_CPU->COP2.Write_CTC ( i.Rd, _CPU->DelaySlot1.Data );
				
				return Cpu::ProcessLoadDelaySlot_t<OPCTC2,RSCTC2>;
				break;
				
			case RSMTC0:
			
				switch ( i.Opcode & 0x3 )
				{
					// MTC0
					case 0:
						//_CPU->Write_MTC0 ( i.Rd, _CPU->DelaySlot1.Data );
						
						return Cpu::ProcessLoadDelaySlot_t<OPMTC0,RSMTC0>;
						break;
						
					// MTC2
					case 2:
						//_CPU->COP2.Write_MTC ( i.Rd, _CPU->DelaySlot1.Data );
						
						return Cpu::ProcessLoadDelaySlot_t<OPMTC2,RSMTC2>;
						break;

				}	// end switch ( OPCODE & 0x3 )
				
				break;
				
		}	// end switch ( i.Rs )
		
	}	// end else if ( OPCODE >= 0x10 )

	return NULL;
}


void Cpu::Refresh_DelaySlotPointer ( DelaySlot *DelaySlotPtr )
{
	if ( DelaySlotPtr->Instruction.Value < 0x40000000 )
	{
		///////////////////////////////////////////////////////////
		// instruction just executed was in branch delay slot
	
		//ProcessBranchDelaySlot ();
		DelaySlotPtr->cb = Refresh_BranchDelay ( DelaySlotPtr->Instruction );
	}
	else
	{
		// otherwise it is a load or COP instruction
		
		//ProcessLoadDelaySlot ();
		DelaySlotPtr->cb = Refresh_LoadDelay ( DelaySlotPtr->Instruction );
	}
}


// call this when refreshing the CPU state
void Cpu::Refresh_AllDelaySlotPointers ()
{
	//Refresh_DelaySlotPointer ( & DelaySlot0 );
	//Refresh_DelaySlotPointer ( & DelaySlot1 );
	Refresh_DelaySlotPointer ( & DelaySlots [ 0 ] );
	Refresh_DelaySlotPointer ( & DelaySlots [ 1 ] );
}


void Cpu::SetPC ( u32 Value )
{
	PC = Value;
}


void Cpu::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* COP0_Names [] = { "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "Reserved",
								"BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause", "EPC", "PRId",
								"Config", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "BadPAddr",
								"Debug", "Perf", "Reserved", "Reserved", "TagLo", "TagHi", "ErrorEPC", "Reserved" };
								
	static const char* DisAsm_Window_ColumnHeadings [] = { "Address", "@", ">", "Instruction", "Inst (hex)" };
								
	static const char* FontName = "Courier New";
	static const int FontSize = 6;
	
	static const char* DebugWindow_Caption = "R3000A Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 995;
	static const int DebugWindow_Height = 420;
	
	static const int GPRList_X = 0;
	static const int GPRList_Y = 0;
	static const int GPRList_Width = 190;
	static const int GPRList_Height = 380;

	static const int COP1List_X = GPRList_X + GPRList_Width;
	static const int COP1List_Y = 0;
	static const int COP1List_Width = 175;
	static const int COP1List_Height = 300;
	
	static const int COP2_CPCList_X = COP1List_X + COP1List_Width;
	static const int COP2_CPCList_Y = 0;
	static const int COP2_CPCList_Width = 175;
	static const int COP2_CPCList_Height = 300;
	
	static const int COP2_CPRList_X = COP2_CPCList_X + COP2_CPCList_Width;
	static const int COP2_CPRList_Y = 0;
	static const int COP2_CPRList_Width = 175;
	static const int COP2_CPRList_Height = 300;
	
	static const int DisAsm_X = COP2_CPRList_X + COP2_CPRList_Width;
	static const int DisAsm_Y = 0;
	static const int DisAsm_Width = 270;
	static const int DisAsm_Height = 380;
	
	static const int MemoryViewer_Columns = 8;
	static const int MemoryViewer_X = GPRList_X + GPRList_Width;
	static const int MemoryViewer_Y = 300;
	static const int MemoryViewer_Width = 250;
	static const int MemoryViewer_Height = 80;
	
	static const int BkptViewer_X = MemoryViewer_X + MemoryViewer_Width;
	static const int BkptViewer_Y = 300;
	static const int BkptViewer_Width = 275;
	static const int BkptViewer_Height = 80;
	
	int i;
	stringstream ss;
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( (char*) DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->Set_Font ( DebugWindow->CreateFontObject ( FontSize, (char*) FontName ) );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		GPR_ValueList = new DebugValueList<u32> ();
		COP0_ValueList = new DebugValueList<u32> ();
		COP2_CPCValueList = new DebugValueList<u32> ();
		COP2_CPRValueList = new DebugValueList<u32> ();
		
		// create the value lists
		GPR_ValueList->Create ( DebugWindow, GPRList_X, GPRList_Y, GPRList_Width, GPRList_Height );
		COP0_ValueList->Create ( DebugWindow, COP1List_X, COP1List_Y, COP1List_Width, COP1List_Height );
		COP2_CPCValueList->Create ( DebugWindow, COP2_CPCList_X, COP2_CPCList_Y, COP2_CPCList_Width, COP2_CPCList_Height );
		COP2_CPRValueList->Create ( DebugWindow, COP2_CPRList_X, COP2_CPRList_Y, COP2_CPRList_Width, COP2_CPRList_Height );
		
		GPR_ValueList->EnableVariableEdits ();
		COP0_ValueList->EnableVariableEdits ();
		COP2_CPCValueList->EnableVariableEdits ();
		COP2_CPRValueList->EnableVariableEdits ();
	
		// add variables into lists
		for ( i = 0; i < 32; i++ )
		{
			ss.str ("");
			ss << "R" << dec << i;
			GPR_ValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->GPR [ i ].u) );
			
			ss.str ("");
			ss << COP0_Names [ i ];
			
#ifdef PS2_COMPILE
			ss << "X";
#endif

			COP0_ValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->CPR0.Regs [ i ]) );

			if ( i < 16 )
			{
				ss.str("");
				ss << "CPC2_" << dec << ( i << 1 );
				COP2_CPCValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ i << 1 ]) );
				
				ss.str("");
				ss << "CPC2_" << dec << ( ( i << 1 ) + 1 );
				COP2_CPCValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ ( i << 1 ) + 1 ]) );
			}
			else
			{
				ss.str("");
				ss << "CPR2_" << dec << ( ( i - 16 ) << 1 );
				COP2_CPRValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( i - 16 ) << 1 ]) );
				
				ss.str("");
				ss << "CPR2_" << dec << ( ( ( i - 16 ) << 1 ) + 1 );
				COP2_CPRValueList->AddVariable ( (char*) ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( ( i - 16 ) << 1 ) + 1 ]) );
			}
		}
		
		// Don't forget to show the HI and LO registers
		GPR_ValueList->AddVariable ( "LO", &(_CPU->HiLo.uLo) );
		GPR_ValueList->AddVariable ( "HI", &(_CPU->HiLo.uHi) );
		
#ifdef PS2_COMPILE
		// also add PC and CycleCount for a PS2 compile
		GPR_ValueList->AddVariable ( "PCX", &(_CPU->PC) );
		GPR_ValueList->AddVariable ( "NextPCX", &(_CPU->NextPC) );
		GPR_ValueList->AddVariable ( "LastPCX", &(_CPU->LastPC) );
		GPR_ValueList->AddVariable ( "CycleLOX", (u32*) &(_CPU->CycleCount) );
		
		GPR_ValueList->AddVariable ( "LastReadAddressX", &(_CPU->Last_ReadAddress) );
		GPR_ValueList->AddVariable ( "LastWriteAddressX", &(_CPU->Last_WriteAddress) );
		GPR_ValueList->AddVariable ( "LastReadWriteAddressX", &(_CPU->Last_ReadWriteAddress) );

#else
		// also add PC and CycleCount
		GPR_ValueList->AddVariable ( "PC", &(_CPU->PC) );
		GPR_ValueList->AddVariable ( "NextPC", &(_CPU->NextPC) );
		GPR_ValueList->AddVariable ( "LastPC", &(_CPU->LastPC) );
		GPR_ValueList->AddVariable ( "CycleLO", (u32*) &(_CPU->CycleCount) );
		GPR_ValueList->AddVariable ( "CycleSYS", (u32*) &(System::_SYSTEM->CycleCount) );
		GPR_ValueList->AddVariable ( "NextEvent", (u32*) &(System::_SYSTEM->NextEvent_Cycle) );
		
		GPR_ValueList->AddVariable ( "LastReadAddress", &(_CPU->Last_ReadAddress) );
		GPR_ValueList->AddVariable ( "LastWriteAddress", &(_CPU->Last_WriteAddress) );
		GPR_ValueList->AddVariable ( "LastReadWriteAddress", &(_CPU->Last_ReadWriteAddress) );
#endif
		
		GPR_ValueList->AddVariable ( "IsBusy", (u32*) & _CPU->Status.isSomethingBusy );
		
		
		// need to add in load delay slot values
		//GPR_ValueList->AddVariable ( "D0_INST", &(_CPU->DelaySlot0.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D0_VAL", &(_CPU->DelaySlot0.Data) );
		//GPR_ValueList->AddVariable ( "D1_INST", &(_CPU->DelaySlot1.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D1_VAL", &(_CPU->DelaySlot1.Data) );
		GPR_ValueList->AddVariable ( "D0_INST", &(_CPU->DelaySlots [ _CPU->NextDelaySlotIndex ].Instruction.Value) );
		GPR_ValueList->AddVariable ( "D0_VAL", &(_CPU->DelaySlots [ _CPU->NextDelaySlotIndex ].Data) );
		GPR_ValueList->AddVariable ( "D1_INST", &(_CPU->DelaySlots [ _CPU->NextDelaySlotIndex ^ 1 ].Instruction.Value) );
		GPR_ValueList->AddVariable ( "D1_VAL", &(_CPU->DelaySlots [ _CPU->NextDelaySlotIndex ^ 1 ].Data) );

		
		GPR_ValueList->AddVariable ( "SPUCC", (u32*) _SpuCycleCount );
		
		GPR_ValueList->AddVariable ( "Trace", &TraceValue );

		GPR_ValueList->AddVariable ( "addrx", & ( Bus->MainMemory.b32 [ 0x269f0 >> 2 ] ) );
		

		// temporary space
		//GPR_ValueList->AddVariable ( "0x1F8001A8", (u32*) &(_CPU->DCache.b8 [ 0x1A8 ]) );

		// create the disassembly window
		DisAsm_Window = new Debug_DisassemblyViewer ( Breakpoints );
		DisAsm_Window->Create ( DebugWindow, DisAsm_X, DisAsm_Y, DisAsm_Width, DisAsm_Height, (Debug_DisassemblyViewer::DisAsmFunction) Print::PrintInstruction );
		
		DisAsm_Window->Add_MemoryDevice ( "RAM", Playstation1::DataBus::MainMemory_Start, Playstation1::DataBus::MainMemory_Size, (u8*) Bus->MainMemory.b8 );
		DisAsm_Window->Add_MemoryDevice ( "BIOS", Playstation1::DataBus::BIOS_Start, Playstation1::DataBus::BIOS_Size, (u8*) Bus->BIOS.b8 );
		
		DisAsm_Window->SetProgramCounter ( &_CPU->PC );
		
		
		// create window area for breakpoints
		Breakpoint_Window = new Debug_BreakpointWindow ( Breakpoints );
		Breakpoint_Window->Create ( DebugWindow, BkptViewer_X, BkptViewer_Y, BkptViewer_Width, BkptViewer_Height );
		
		// create the viewer for D-Cache scratch pad
		ScratchPad_Viewer = new Debug_MemoryViewer ();
		
		ScratchPad_Viewer->Create ( DebugWindow, MemoryViewer_X, MemoryViewer_Y, MemoryViewer_Width, MemoryViewer_Height, MemoryViewer_Columns );
		ScratchPad_Viewer->Add_MemoryDevice ( "ScratchPad", c_ScratchPadRam_Addr, c_ScratchPadRam_Size, _CPU->DCache.b8 );
		
		
		// mark debug as enabled now
		DebugWindow_Enabled = true;
		
		// update the value lists
		DebugWindow_Update ();
	}
	
#endif
	
}

void Cpu::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		delete DebugWindow;
		delete GPR_ValueList;
		delete COP0_ValueList;
		delete COP2_CPCValueList;
		delete COP2_CPRValueList;

		delete DisAsm_Window;
		
		delete Breakpoint_Window;
		delete ScratchPad_Viewer;
		
		// disable debug window
		DebugWindow_Enabled = false;
	}
	
#endif

}


void Cpu::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		GPR_ValueList->Update();
		COP0_ValueList->Update();
		COP2_CPCValueList->Update();
		COP2_CPRValueList->Update();
		DisAsm_Window->GoTo_Address ( _CPU->PC );
		DisAsm_Window->Update ();
		Breakpoint_Window->Update ();
		ScratchPad_Viewer->Update ();
	}
	
#endif

}

