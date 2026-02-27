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





#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string.h>

#include <float.h>

//#include <fenv.h>
//#pragma STDC FENV_ACCESS ON

//#include <fstream>

#include "MipsOpcode.h"
#include "R5900_Print.h"

#include "R5900_Execute.h"
#include "R5900_Recompiler.h"

//#include "R5900.h"

// will fix this file later
//#include "ps2_system.h"


// *** I'll put this here temporarily for debugging *** //
#include "PS2_Gpu.h"
#include "VU.h"
#include "PS1_SPU2.h"


using namespace R5900;
using namespace R5900::Instruction;
using namespace Playstation2;

using namespace std;

//#include "GNUSignExtend_x64.h"
//using namespace x64SignExtend::Utilities;

#if defined(__GNUC__) && !defined(_MSC_VER)
// GCC on Windows does not support __try/__except in C++ mode.
// Map __except to catch(...) so the code compiles; __try is already an alias for try in GCC.
#ifndef __except
#define __except(x) catch(...)
#endif
#endif

// enable recompiler option
#define ENABLE_RECOMPILER_R5900

// enables the level-2 recompiler
//#define ENABLE_R5900_RECOMPILE2


// force enable recompiler for profiling
//#define FORCE_ENABLE_RECOMPILER_R5900


// put recompiled code behind function call for profiling
//#define ENABLE_R5900_PROFILING


// use virtual machine to manage more vu0 macro mode
// this define exists in: R5900.cpp, R5900_Recompiler.cpp
//#define ENABLE_VIRTUAL_MACHINE_MACRO_RUN

// use exceptions for div/divu/pdivbw/pdivw/pdivuw in recompiler
#define USE_EXCEPTIONS_R5900_DIV

// enable branch prediction timing
#define ENABLE_R5900_BRANCH_PREDICTION



#define ENABLE_BUS_EMULATION


// enables i-cache on R5900
// must be enabled here and in R5900_Execute.cpp
#define ENABLE_R5900_ICACHE

// do not interrupt for CPX 0/1/2/3 instructions
#define DISABLE_INTERRUPT_FOR_CPX_INSTRUCTIONS


// interrupt testing
//#define VERBOSE_MULTIPLE_INT_CAUSE


// this one goes to cout and is good for bug reports
#define INLINE_DEBUG_COUT


#define ENABLE_PS2_VIRTUAL_MACHINE_R5900


// exception info should come from recompiler
//#define ENABLE_EXCEPTION_INFO_VIRTUAL


#ifdef _DEBUG_VERSION_

// build this with inline debugger
#define INLINE_DEBUG_ENABLE


//#define INLINE_DEBUG
//#define INLINE_DEBUG2
//#define INLINE_DEBUG_ASYNC_INT
//#define INLINE_DEBUG_SYNC_INT
//#define INLINE_DEBUG_UPDATE_INT
//#define INLINE_DEBUG_SYSCALL

//#define INLINE_DEBUG_LOAD
//#define INLINE_DEBUG_STORE

//#define COUT_DELAYSLOT_CANCEL


#endif


Playstation2::DataBus *Cpu::Bus;

namespace R5900
{
	class Recompiler;

u64* Cpu::_SpuCycleCount;


u64* Cpu::_NextSystemEvent;


volatile Cpu::_DebugStatus Cpu::DebugStatus;
//volatile u32 Cpu::Debug_BreakPoint_Address;
//u32 Cpu::Debug_BreakPoint_Address;
//u32 Cpu::Debug_RAMDisplayStart;


u32* Cpu::_Debug_IntcStat;
u32* Cpu::_Debug_IntcMask;

u32* Cpu::_Intc_Stat;
u32* Cpu::_Intc_Mask;
u32* Cpu::_R5900_Status;


Cpu *Cpu::_CPU;


R5900::Recompiler* R5900::Cpu::rs;

u64 Cpu::uExceptionRWAddr64;
u64 Cpu::uExceptionInstrAddr64;
u64 Cpu::uExceptionPC64;
u64 Cpu::uExceptionCycles64;

bool Cpu::bIsFloatException;
bool Cpu::bIsInvalidException;

bool Cpu::DebugWindow_Enabled;

#ifdef ENABLE_GUI_DEBUGGER
Debug::Log Cpu::debug;
Debug::Log Cpu::debugBios;
WindowClass::Window *Cpu::DebugWindow;
DebugValueList<u32> *Cpu::GPR_ValueList;
DebugValueList<u32> *Cpu::COP0_ValueList;
DebugValueList<float> *Cpu::COP2_CPCValueList;
DebugValueList<u32> *Cpu::COP2_CPRValueList;
Debug_DisassemblyViewer *Cpu::DisAsm_Window;
Debug_BreakpointWindow *Cpu::Breakpoint_Window;
Debug_MemoryViewer *Cpu::ScratchPad_Viewer;
Debug_BreakPoints *Cpu::Breakpoints;
#endif


u32 Cpu::TraceValue;







Cpu::Cpu ()
{
	cout << "Running R5900 constructor...\n";

/*
#ifdef INLINE_DEBUG_ENABLE
	// start the inline debugger
	debug.Create ( "R3000ALog.txt" );
	debugBios.Create ( "BiosCallsLog.txt" );
#endif


	// not yet debugging
	debug_enabled = false;

	// Connect CPU with System Bus
	//Bus = db;
	
	// set cpu for buffers
	LoadBuffer.ConnectDevices ( this );
	StoreBuffer.ConnectDevices ( this );
	
	// create execution unit to execute instructions
	//e = new Instruction::Execute ();
	Instruction::Execute::Execute ();
	
	// create an instance of ICache and connect with CPU
	// *note* this is no longer a pointer to the object
	//ICache = new ICache_Device ();
	
	// create an instance of COP2 co-processor and connect with CPU
	// *note* this is no longer a pointer to the object
	//COP2 = new COP2_Device ();
	
	// set as the current R3000A cpu object
	_CPU = this;
	
	// create object to handle breakpoints
	Breakpoints = new Debug_BreakPoints ();
	
	// reset the cpu object
	Reset ();
*/

}


void Cpu::ConnectDevices ( Playstation2::DataBus* db )
{
	Bus = db;
	//_SpuCycleCount = _SpuCC;
}


Cpu::~Cpu ( void )
{
	//if ( debug_enabled ) delete DebugWindow;
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

	// start the inline debugger
	debug.Create ( "R5900Log.txt" );
	
#ifdef INLINE_DEBUG_SPLIT_BIOS
	// put debug output into a separate file
	debugBios.SetSplit ( true );
	debugBios.SetCombine ( false );
#endif

	debugBios.Create ( "PS2SysCallsLog.txt" );
#endif

	// set float rounding mode
	//fesetround(FE_TOWARDZERO);

	// round toward zero and flush denormals ?
	_controlfp(_DN_FLUSH, _MCW_DN);
	_controlfp(_RC_CHOP, _MCW_RC);

	// enable floating point exceptions
	//_controlfp(_EM_INEXACT | _EM_ZERODIVIDE | _EM_DENORMAL, _MCW_EM);
	//_clearfp();

	// not yet debugging
	debug_enabled = false;
	
	// initialize/enable printing of instructions
	R5900::Instruction::Print::Start ();

	
	// create execution unit to execute instructions
	//e = new Instruction::Execute ();
	//Instruction::Execute::Execute ( this );
	Instruction::Execute::r = this;
	
	
	// set as the current R3000A cpu object
	_CPU = this;
	
	// create object to handle breakpoints
#ifdef ENABLE_GUI_DEBUGGER
	//Breakpoints = new Debug_BreakPoints ( Bus->BIOS.b8, Bus->MainMemory.b8, DCache.b8 );
	Breakpoints = new Debug_BreakPoints ( Bus->BIOS.b8, Bus->MainMemory.b8, Bus->ScratchPad.b8 );
#endif

	
	// reset the cpu object
	Reset ();
	
	// start COP2 object
	//COP2.Start ();
	
	// start the instruction execution object
	Instruction::Execute::Start ();


	// set vu recompiler to use either sse/avx2/avx512
	//Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_SSE);
	//Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_SSEX1);

	// if using avx2 encoding max of 1 instruction at a time
	R5900::Recompiler::SetVectorType(R5900::Recompiler::VECTOR_TYPE_AVX2);
	R5900::Recompiler::SetRecompilerWidth(R5900::Recompiler::RECOMPILER_WIDTH_AVX2X1);



#ifdef ENABLE_RECOMPILER_R5900
	
#ifdef ENABLE_R5900_RECOMPILE2
	rs = new Recompiler ( this, 18 - Playstation2::DataBus::c_iInvalidate_Shift, Playstation2::DataBus::c_iInvalidate_Shift + 11, Playstation2::DataBus::c_iInvalidate_Shift );
	
	rs->SetOptimizationLevel ( 2 );
	//rs->SetOptimizationLevel ( 1 );
	//rs->SetOptimizationLevel ( 0 );
#else
	//rs = new Recompiler ( this, 19 - Playstation2::DataBus::c_iInvalidate_Shift, Playstation2::DataBus::c_iInvalidate_Shift + 11, Playstation2::DataBus::c_iInvalidate_Shift );
	//rs = new Recompiler(this, 19 - Playstation2::DataBus::c_iInvalidate_Shift, Playstation2::DataBus::c_iInvalidate_Shift + 10, Playstation2::DataBus::c_iInvalidate_Shift);
	rs = new Recompiler(this, R5900_CODE_BLOCK_COUNT_SHIFT, R5900_CODE_BLOCK_SIZE_SHIFT, R5900_INSTRUCTIONS_PER_CACHE_ENTRY_SHIFT);

	rs->SetOptimizationLevel(1);
	//rs->SetOptimizationLevel(0);
#endif
	
	// enable recompiler by default
	bEnableRecompiler = true;
	Status.uDisableRecompiler = false;

#else

	bEnableRecompiler = false;
	Status.uDisableRecompiler = true;
#endif	// end #ifdef ENABLE_RECOMPILER_R5900

}

void Cpu::Reset ( void )
{
	u32 i;
	
	// zero object
	memset ( this, 0, sizeof( Cpu ) );

	// this is the start address for the program counter when a R5900 is reset
	PC = 0xbfc00000;
	
	// start in kernel mode?
	//CPR0.Status.KUc = 1;

	// must set this as already having been executed
	//CurInstExecuted = true;
	
	// set processor id implementation and revision for R3000A
	CPR0.PRId.Rev = c_ProcessorRevisionNumber;
	CPR0.PRId.Imp = c_ProcessorImplementationNumber;
	
	CPR0.Config.l = c_iConfigReg_StartValue;
	CPR0.Status.l = c_iStatusReg_StartValue;
	
	// fpu revision
	CPC1 [ 0 ] = c_lFCR0;
	
	// initialize the set bits for fpu status register 31
	CPC1 [ 31 ] = c_lFCR31_SetMask;
	
	// need to redo reset of COP2 - must call reset and not Start, cuz if you call start again then inline debugging does not work anymore
	//COP2.Reset ();
	
	// reset i-cache again
	ICache.Reset ();
	
	// reset dcache
	DCache.Reset ();
}


void Cpu::Refresh ( void )
{
	Refresh_AllBranchDelaySlots ();
}



void Cpu::InvalidateCache ( u32 Address )
{
	//ICache.Invalidate ( Address );
}



// ---------------------------------------------------------------------------------


int Cpu::ExceptionFilter(LPEXCEPTION_POINTERS pEp
	//void (*pMonitorFxn)(LPEXCEPTION_POINTERS, void*)
)
{
	CONTEXT* ctx = pEp->ContextRecord;
	ULONG_PTR* info = pEp->ExceptionRecord->ExceptionInformation;
	UINT_PTR addr = info[1];
	DWORD dummy;

	u64 ullFunctionCode;
	u64 ullInstrPtr;

	u32 op1, op2, op3, result;
	u32 instr_opcode, instr_func, instr_func2;

	long lTemp;

	static constexpr int IDIV_INSTRUCTION_SIZE_BYTES = 2;

	R5900::Instruction::Format instr;

	bIsFloatException = false;
	bIsInvalidException = false;

	switch (pEp->ExceptionRecord->ExceptionCode)
	{
#ifdef USE_EXCEPTIONS_R5900_DIV

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

			// get the pc and cycles offset the exception happened at
			uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
			uExceptionCycles64 = ((u64)ctx->R8) >> 32;

			return EXCEPTION_EXECUTE_HANDLER;
		}

		break;

	case STATUS_FLOAT_INVALID_OPERATION:
#define VERBOSE_EXCEPTION_FILTER_INVALID
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID
		printf("\nhps2x64: status r5900 float invalid operation: pc=%x op1=%016llx %016llx op2=%016llx %016llx res=%016llx %016llx",
			uExceptionPC64, ctx->Xmm0.High, ctx->Xmm0.Low, ctx->Xmm1.High, ctx->Xmm1.Low, ctx->Xmm3.High, ctx->Xmm3.Low);
#endif
		
		bIsFloatException = true;
		bIsInvalidException = true;
		uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		//return EXCEPTION_EXECUTE_HANDLER;
		break;

	case STATUS_FLOAT_OVERFLOW:
#define VERBOSE_EXCEPTION_FILTER_INVALID_OVERFLOW
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID_OVERFLOW
		printf("\nhps2x64: status r5900 float overflow");
#endif

		bIsFloatException = true;
		uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		return EXCEPTION_EXECUTE_HANDLER;
		break;

	case STATUS_FLOAT_UNDERFLOW:
#define VERBOSE_EXCEPTION_FILTER_INVALID_UNDERFLOW
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID_UNDERFLOW
		printf("\nhps2x64: status r5900 float underflow");
#endif

		bIsFloatException = true;
		uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		return EXCEPTION_EXECUTE_HANDLER;
		break;

	default:
		break;
	}


	if (bIsFloatException)
	{
		ullFunctionCode = ((u64)ctx->R8) >> 32;
		uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;

		instr_opcode = ullFunctionCode >> 26;
		instr_func = ullFunctionCode & 0x3f;

		instr_func2 = ullFunctionCode & 0x7ff;

		ullInstrPtr = ctx->Rip;
		instr.Value = ullFunctionCode;


		// op1 is in vector RAX reg
		op1 = (u32)ctx->Xmm0.Low;
		// op2 is in vector RCX
		op2 = (u32)ctx->Xmm1.Low;
		op3 = (u32)ctx->Xmm2.Low;

//#define VERBOSE_EXCEPTION_FILTER_INVALID_FLOAT_EXCEPTION
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID_FLOAT_EXCEPTION
		printf("\nhps2x64: FLOAT EXCEPTION: func=%016llx ret=%016llx rip=%016llx", ullFunctionCode, uExceptionPC64, ullInstrPtr);
#endif

		Instruction::Execute::ExecuteInstruction(instr);

		// resume from the next instruction
		ctx->Rip = uExceptionPC64;

		// resume from the next instruction
		return EXCEPTION_CONTINUE_EXECUTION;

	}	// end if (bIsFloatException)

	return EXCEPTION_CONTINUE_SEARCH;
}

void Cpu::ExecuteRecompiledCode(u32 Index)
{
	((func2)(rs->pCodeStart[Index])) ();
}

// use for testing execution unit
void Cpu::Run ()
{
	u32 Index;
	u32 *pCacheLine32;
	u64 *pMemPtr64, *pCacheLine64;

	u64 ullChecksum64;
	

#ifdef INLINE_DEBUG
	debug << "\r\n->PC = " << hex << setw( 8 ) << PC << dec;
	debug << " Cycle#" << CycleCount;
#endif

		
	//////////////////////
	// Load Instruction //
	//////////////////////

	
#ifdef ENABLE_R5900_ICACHE
			
	// load next instruction to be executed
	// step 0: check if instruction is in a cached location
	if (ICache_Device::isCached(PC))
	{
		// instruction is in a cached location //

#ifdef INLINE_DEBUG
		debug << ";isCached";
#endif

		// bus might be free //

		// check if there is a cache hit
		pCacheLine32 = ICache.isCacheHit(PC);

		// check if there is a cache hit
		if (!pCacheLine32)
		{
			// cache miss //

#ifdef INLINE_DEBUG
			debug << ";MISS";
#endif

			////////////////////////////////////////////
			// check if we can access data bus

				// since it is pipelined, we can read all 4 instructions to refill cache all at once and then wait until setting cache as valid
				// I'll have bus decide how many cycles it is busy for

#ifdef INLINE_DEBUG
			debug << ";IREAD16";
#endif

			// get pointer to instructions in memory
			pMemPtr64 = (u64*)Bus->GetIMemPtr(PC & 0xffffffc0);

#ifdef INLINE_DEBUG
			debug << " LATENCY=" << dec << Bus->GetLatency();
			debug << " Inst(Mem): ";
			for (int i = 0; i < 16; i++) { debug << " " << R5900::Instruction::Print::PrintInstruction(((u32*)pMemPtr64)[i]).c_str(); }
#endif

			// load the instructions into the cache line
			ICache.ReloadCache(PC, pMemPtr64);

#ifdef INLINE_DEBUG
			debug << " Inst(ICache): ";
			pCacheLine32 = ICache.isCacheHit_Line(PC);
			for (int i = 0; i < 16; i++) { debug << " " << R5900::Instruction::Print::PrintInstruction(pCacheLine32[i]).c_str(); }
#endif

			CycleCount += Bus->GetLatency();

			// also add in the time to refill the cache line in addition to the latency
			CycleCount += c_ullCacheRefill_CycleTime;


#ifdef ENABLE_BUS_EMULATION
#ifdef INLINE_DEBUG
			debug << " BUSY-UNTIL=" << dec << Bus->BusyUntil_Cycle;
#endif
			//if ( Bus->GetLatency () < 20 )
			//{
			// now check if bus is free after memory device latency period
			if (CycleCount < Bus->BusyUntil_Cycle)
			{
				// bus is not free yet, so must wait until it is free
				CycleCount = Bus->BusyUntil_Cycle;
			}

			// now set the time that the bus will be busy for
			// loading 4 quadwords, should take like 4 cycles?
			Bus->BusyUntil_Cycle = CycleCount + 4;
#endif


			// validate cache lines


			// *** testing *** load the instruction from cache //
			//CurInst.Value = ICache.Read ( PC );
			CurInst.Value = ((u32*)pMemPtr64)[(PC >> 2) & 0xf];

#ifndef FORCE_ENABLE_RECOMPILER_R5900
			if (bEnableRecompiler)
#endif
			{

				if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
				{
					// read from pc in main memory, compare instructions

					// make sure that
					u64 xcompare64;

					// get pointer into main memory
					u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0x3f]);

					// get pointer into the source code that was recompiled
					u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0x3f);

					xcompare64 = *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64++ ^ *psource64++;
					xcompare64 |= *pmemory64 ^ *psource64;

					if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0x3f)))
					{
#ifdef ENABLE_PS2_VIRTUAL_MACHINE_R5900
						rs->clear_hwrw_bitmap(PC);
						rs->clear_macro_bitmap(PC);

#else
						rs->set_hwrw_bitmap(PC);
						rs->set_macro_bitmap(PC);
#endif

						rs->Recompile(PC);
					}


#ifdef ENABLE_MEMORY_INVALIDATE
					// check for data-modify if using recompiler //
					if (Bus->InvalidArray.b8[((PC & DataBus::MainMemory_Mask) >> (2 + DataBus::c_iInvalidate_Shift)) & DataBus::c_iInvalidate_Mask])
					{
#ifdef VERBOSE_RECOMPILE
						cout << "\nR3000A: CacheMiss: Recompile: PC=" << hex << PC;
#endif

						rs->Recompile(PC);
						Bus->InvalidArray.b8[((PC & DataBus::MainMemory_Mask) >> (2 + DataBus::c_iInvalidate_Shift)) & DataBus::c_iInvalidate_Mask] = 0;

					}
#endif

				} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

			} // if ( bEnableRecompiler )

		}
		else
		{
			// cache hit //

#ifdef INLINE_DEBUG
			debug << ";HIT";
#endif

			//CurInst.Value = ICache.Read ( PC );
			CurInst.Value = *pCacheLine32;
		}


		/////////////
		// Execute //
		/////////////

		// execute instruction
		// process all CPU events

	}
	else
	{
		///////////////////////////////////////////////
		// instruction is not in cached location

#ifdef INLINE_DEBUG
		debug << ";!isCached";
#endif

		// attempt to load instruction from bus


#ifdef INLINE_DEBUG
		debug << ";IREAD1";
#endif

		// the bus is free and there are no pending load/store operations
		// Important: When reading from the bus, it has already been determined whether address is in I-Cache or not, so clear top 3 bits
		//CurInst.Value = Bus->Read ( PC & 0x1fffffff );
		CurInst.Value = Bus->Read_t<0xffffffff>(PC);

		CycleCount += Bus->GetLatency();


#ifndef FORCE_ENABLE_RECOMPILER_R5900
		if (bEnableRecompiler)
#endif
		{

			if (Bus->GetLatency() <= DataBus::c_iRAM_Read_Latency)
			{
				// read from pc in main memory, compare instructions

				// make sure that
				u64 xcompare64;

				// get pointer into main memory
				u64* pmemory64 = (u64*)&(Bus->MainMemory.b8[PC & DataBus::MainMemory_Mask & ~0x3f]);

				// get pointer into the source code that was recompiled
				u64* psource64 = (u64*)rs->get_source_code_ptr(PC & ~0x3f);

				xcompare64 = *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64++ ^ *psource64++;
				xcompare64 |= *pmemory64 ^ *psource64;

				if ((xcompare64) || (rs->get_start_address(PC) != (PC & ~0x3f)))
				{
#ifdef ENABLE_PS2_VIRTUAL_MACHINE_R5900
					rs->clear_hwrw_bitmap(PC);
					rs->clear_macro_bitmap(PC);

#else
					rs->set_hwrw_bitmap(PC);
					rs->set_macro_bitmap(PC);
#endif

					rs->Recompile(PC);
				}


#ifdef ENABLE_MEMORY_INVALIDATE
				// check for data-modify if using recompiler //
				if (Bus->InvalidArray.b8[((PC & DataBus::MainMemory_Mask) >> (2 + DataBus::c_iInvalidate_Shift)) & DataBus::c_iInvalidate_Mask])
				{
#ifdef VERBOSE_RECOMPILE
					cout << "\nR3000A: CacheMiss: Recompile: PC=" << hex << PC;
#endif

					rs->Recompile(PC);
					Bus->InvalidArray.b8[((PC & DataBus::MainMemory_Mask) >> (2 + DataBus::c_iInvalidate_Shift)) & DataBus::c_iInvalidate_Mask] = 0;

				}
#endif

			} // if ( Bus->GetLatency () <= c_iRAM_Read_Latency )

		} // if ( bEnableRecompiler )


		// step 3: we can execute instruction right away since memory access is probably pipelined

		/////////////
		// Execute //
		/////////////

		// execute instruction
		// process all CPU events

	}
	

#else
	
	// load instruction
	//CurInst.Value = Bus->Read ( PC, 0xffffffff );
	CurInst.Value = Bus->Read_t<0xffffffff> ( PC );
	
#endif

#ifdef INLINE_DEBUG
	debug << " INST=" << dec << R5900::Instruction::Print::PrintInstruction ( CurInst.Value ).c_str();
#endif
	
	/////////////////////////
	// Execute Instruction //
	/////////////////////////
	
	
	// execute instruction
	NextPC = PC + 4;
	
#ifdef INLINE_DEBUG
	debug << ";Execute";
#endif

	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	GPR [ 0 ].uq0 = 0;
	GPR [ 0 ].uq1 = 0;
	
#ifndef FORCE_ENABLE_RECOMPILER_R5900
	//if (!bEnableRecompiler)
	if(Status.Value)
	{

		// *note* could use rotate right, right shift, load...
		Instruction::Execute::ExecuteInstruction(CurInst);

	}
	else
#endif
	{
		// check that address block is encoded
		if (!rs->isRecompiled(PC))
		{
#ifdef INLINE_DEBUG
			debug << ";NOT Recompiled";
#endif
			// address is NOT encoded //

#ifdef ENABLE_PS2_VIRTUAL_MACHINE_R5900
			rs->clear_hwrw_bitmap(PC);
			rs->clear_macro_bitmap(PC);

#else
			rs->set_hwrw_bitmap(PC);
			rs->set_macro_bitmap(PC);
#endif

			// recompile block
			rs->Recompile(PC);

		}

#ifdef INLINE_DEBUG
		debug << "\r\nRecompiled: Execute: ADDR: " << hex << PC;
#endif

		__try
		{

#ifdef ENABLE_R5900_PROFILING

			ExecuteRecompiledCode(Index);

#else
			// get the block index
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
			u32 uSrcAddr;

			u32 uBlockStartAddr;
			u64 uSrcCycles;

			// get addr that block starts at
			uBlockStartAddr = PC & ~0x3f;

			//#define VERBOSE_VIRTUAL_MACHINE
#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\nhpsx64: EXCEPTION: Exception code: " << hex << GetExceptionCode();
			cout << "\nException read/write address: " << hex << uExceptionRWAddr64 << " from code at address: " << uExceptionInstrAddr64;
			cout << "\nR5900 PC: " << hex << PC << " NextPC: " << NextPC << " LastPC: " << LastPC << " Status:" << Status.Value;
			cout << "\nCycle#" << CycleCount << " NextEvent:" << *_NextSystemEvent;
			//cout << "\nhpsx64: EXCEPTION: Accessing address: " << hex << addr;
#endif


#ifdef ENABLE_EXCEPTION_INFO_VIRTUAL

			// get the source ps1 address that affected R3000A instruction is at
			uSrcAddr = uExceptionPC64;

			// get the cycles to update after exception
			//uSrcCycles = uExceptionCycles64;
			uSrcCycles = rs->get_cycle_count(uSrcAddr);

#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << " R8PC=" << hex << uExceptionPC64 << dec << " R8Cycles=" << ((s32)uExceptionCycles64) << " StartCycle:" << rs->CycleCount[Index] << " EndCycle:" << (u64)rs->get_cycle_count(uSrcAddr);
#endif

#else

			// search for instruction that caused exception in pCodeStart //

			// get pointer to list of code starts
			u8** pCurCodeStart = rs->get_code_start_ptr(uBlockStartAddr);

			// check which source instr caused the exception
			int i;
			for (i = 1; i < 16; i++)
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

			// get the cycles to update after exception
			uSrcCycles = (u64)rs->get_cycle_count(uSrcAddr);

#endif	// end #ifdef ENABLE_EXCEPTION_INFO_VIRTUAL


#ifdef VERBOSE_VIRTUAL_MACHINE2
			u32 Instr0 = Bus->MainMemory.b32[((uSrcAddr & Bus->MainMemory_Mask) >> 2) - 1];
			u32 Instr1 = Bus->MainMemory.b32[((uSrcAddr & Bus->MainMemory_Mask) >> 2) + 0];
			u32 Instr2 = Bus->MainMemory.b32[((uSrcAddr & Bus->MainMemory_Mask) >> 2) + 1];
			//u32 Instr0 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) - 1];
			//u32 Instr1 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 0];
			//u32 Instr2 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 1];

			cout << "\nthe instructions are:";
			cout << "\n#0: " << hex << Instr0;
			cout << "\n#1: " << hex << Instr1;
			cout << "\n#2: " << hex << Instr2;

#endif

			if (bIsFloatException)
			{
				//#define VERBOSE_VIRTUAL_MACHINE_FLOAT
#ifdef VERBOSE_VIRTUAL_MACHINE_FLOAT
				cout << "\nStart addr for the block: " << hex << uBlockStartAddr;
				cout << "\nPS2 Source Address for affected instruction: " << hex << uSrcAddr;
#endif

				// update cycles before interpreting the instruction
				CycleCount += uSrcCycles;

				// instruction that was interrupted needs to be interpreted //
				PC = uSrcAddr;
				NextPC = PC + 4;

				// interpret the instruction
				CurInst.Value = Bus->Read_t<0xffffffff>(PC);
				Instruction::Execute::ExecuteInstruction(CurInst);


#ifdef VERBOSE_VIRTUAL_MACHINE_FLOAT
				cout << "\nAffected instruction: " << hex << CurInst.Value;
#endif

			}
			else
			{
				if (PC == uSrcAddr)
				{
					// no instruction was executed yet //

					// update cycles before interpreting the instruction
					CycleCount += uSrcCycles;

					// interpret the instruction
					Instruction::Execute::ExecuteInstruction(CurInst);

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
				cout << "\nPS2 Source Address for affected instruction: " << uSrcAddr;
#endif

			}	// end if (bIsFloatException) else


			if (bIsInvalidException || !bIsFloatException)
			{

#ifdef ENABLE_VIRTUAL_MACHINE_MACRO_RUN
				if (uExceptionRWAddr64 & 0x1000000000ull)
				{
					rs->bitset_macro_bitmap(uSrcAddr);

					// ***TODO*** set flag to finsh vu0 execution
					// note: don't forget to update R5900 cyclecount after completion
				}
				else
#endif
				{
					// mark the problem instruction in the bitmap //
					rs->bitset_hwrw_bitmap(uSrcAddr);
				}

#ifdef VERBOSE_VIRTUAL_MACHINE
				cout << "\nBitmap for recompile block: " << rs->get_hwrw_bitmap(uSrcAddr);
				cout << "\nBit check for exception instruction: " << rs->bitget_hwrw_bitmap(uSrcAddr);
#endif

				// recompile the block while the instruction is marked //
				rs->Recompile(uBlockStartAddr);

			}	// end if (bIsInvalidException || !bIsFloatException)

		}	// end __try __except (ExceptionFilter(GetExceptionInformation()))


#ifdef INLINE_DEBUG
		debug << ";RecompilerReturned";
#endif


	}	// end if (!bEnableRecompiler) else


#ifdef INLINE_DEBUG
	debug << ";ExeDone";
#endif
	
	
	///////////////////////////////////////////////////
	// Check if there is anything else going on
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Check delay slots
	// need to do this before executing instruction because load delay slot might stall pipeline

	// check if anything is going on with the delay slots
	// *note* by doing this before execution of instruction, pipeline can be stalled as needed
	//if (Status.Value)
	if(Status.isSomethingBusy)
	{
#ifdef ENABLE_FORCE_VU0_COMPLETION
		// check if vu0 needs to complete
		if (Status.Force_Vu0_Finish)
		{
			// while vu0 is running keep it going
			while (VU0::_VU0->Running)
			{
				VU0::_VU0->Run();
			}

			// set R5900 with the new CycleCount from VU0 since it stalled the entire time
			CycleCount = VU0::_VU0->CycleCount;

			// vu0 has finished
			Status.Force_Vu0_Finish = 0;
		}
#endif

		/////////////////////////////////////////////
		// check for anything in delay slots

#ifdef INLINE_DEBUG
		debug << ";DelaySlotValid";
#endif

		if (Status.DelaySlot_Valid & 1)
		{
#ifdef INLINE_DEBUG
			debug << ";Delay1.Value";
#endif

			//DelaySlots[NextDelaySlotIndex].cb();
			//DelaySlots[NextDelaySlotIndex].Value = 0;
			//DelaySlot0.cb();

			NextPC = DelaySlot0.Data;

#ifdef ENABLE_R5900_BRANCH_PREDICTION
			CycleCount += DelaySlot0_BranchTakenCycles;
#endif

			DelaySlot0.Value = 0;
		}

		///////////////////////////////////
		// move delay slot
		//NextDelaySlotIndex ^= 1;


		///////////////////////////////////////////////////
		// Advance status bits for checking delay slot
		//Status.DelaySlot_Valid = ( Status.DelaySlot_Valid << 1 ) & 0x3;
		Status.DelaySlot_Valid >>= 1;

		//////////////////////////////////////////////
		// check for Asynchronous Interrupts
		// also make sure interrupts are enabled
		if (Status.CheckInterrupt)
		{
			// make sure this is not a COP 0/1/2 instruction ??
#ifdef DISABLE_INTERRUPT_FOR_CPX_INSTRUCTIONS
			if ((CurInst.Opcode & 0x3c) != 0x10)
#endif
			{
				// interrupt has now been checked
				Status.CheckInterrupt = 0;

				if ((CPR0.Status.IE && CPR0.Status.EIE) && (!CPR0.Status.EXL) && (!CPR0.Status.ERL) && (CPR0.Status.l & CPR0.Cause.l & 0x8c00))
				{

#ifdef ENABLE_R5900_IDLE
					// not idle if interrupt hits
					ulIdle = 0;
#endif


					CycleCount++;
					LastPC = PC;
					PC = NextPC;

					// interrupt has been triggered
					ProcessAsynchronousInterrupt();


					return;
				}

			}	// end if ( CurInst.Opcode & 0xfc != 0x10 )

		}	// end if (Status.CheckInterrupt)

	}	// end if (Status.Value)


	/////////////////////////////////////
	// Update Program Counter
	LastPC = PC;
	PC = NextPC;
	
	
	CycleCount++;
	
}




// ------------------------------------------------------------------------------------




void Cpu::ProcessBranchDelaySlot ()
{
	Instruction::Format i;
	u32 Address;
	
	//DelaySlot *d = & ( DelaySlots [ NextDelaySlotIndex ] );
	//i = d->Instruction;
	i = DelaySlot0.Instruction;

	switch ( i.Opcode )
	{
		case OPREGIMM:
		
			switch ( i.Rt )
			{
				case RTBLTZ:	// regimm
				case RTBGEZ:	// regimm
				case RTBLTZL:	// regimm
				case RTBGEZL:	// regimm
					NextPC = PC + ( i.sImmediate << 2 );
					break;

				case RTBLTZAL:	// regimm
				case RTBGEZAL:	// regimm
				case RTBLTZALL:	// regimm
				case RTBGEZALL:	// regimm
					NextPC = PC + ( i.sImmediate << 2 );
					break;
			}
			
			break;
		
		case OPSPECIAL:
		
			switch ( i.Funct )
			{
				case SPJR:
				case SPJALR:
					//NextPC = d->Data;
					NextPC = DelaySlot0.Data;
					break;
			}
			
			break;
			
	
		case OPJ:	// ok
			NextPC = ( 0xf0000000 & PC ) | ( i.JumpAddress << 2 );
			break;
			
			
		case OPJAL:	// ok
			NextPC = ( 0xf0000000 & PC ) | ( i.JumpAddress << 2 );
			break;
			
		case OPBEQ:
		case OPBNE:
		case OPBLEZ:
		case OPBGTZ:
		case OPBEQL:
		case OPBNEL:
		case OPBLEZL:
		case OPBGTZL:
		case OPCOP0:
		case OPCOP1:
		case OPCOP2:
			NextPC = PC + ( i.sImmediate << 2 );
			break;
	}
	
}




u32 Cpu::Read_MFC0 ( u32 Register )
{
	/////////////////////////////////////////////////////////////////////
	// Process storing to COP0 registers
	switch ( Register )
	{
		case 9:
#ifdef INLINE_DEBUG_STORE
			debug << ";Count" << ";Before=" << _CPU->CPR0.Regs [ 9 ] << ";Writing=" << hex << Value;
#endif

			_CPU->CPR0.Regs [ 9 ] = (u32) _CPU->CycleCount;
			return (u32) _CPU->CycleCount;
			break;
			
		default:
#ifdef INLINE_DEBUG_STORE
			debug << ";Other";
#endif
		
			return _CPU->CPR0.Regs [ Register ];
			break;
	}
}



void Cpu::Write_MTC0 ( u32 Register, u32 Value )
{
	/////////////////////////////////////////////////////////////////////
	// Process storing to COP0 registers
	switch ( Register )
	{
		case 1:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Random";
#endif

			// Random register is read-only
			break;
			
		case 8:
#ifdef INLINE_DEBUG_LOAD
			debug << ";BadVAddr";
#endif

			// BadVAddr register is read-only
			break;
		
		case 12:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Status" << ";Before=" << _CPU->CPR0.Regs [ 12 ] << ";Writing=" << hex << Value;
#endif
		
			///////////////////////////////////////////////////
			// Status register
			// bits 5-9,13-14,19-21,24-27 are always zero
			
			// *note* Asynchronous interrupt will be checked for in main processor loop
			_CPU->CPR0.Regs [ 12 ] = ( _CPU->CPR0.Regs [ 12 ] & StatusReg_WriteMask ) | ( Value & (~StatusReg_WriteMask) );
			
			
			//if ( CPR0.Status.SwC )
			//{
			//	cout << "\nhps1x64 ALERT: SwC -> it is invalidting I-Cache entries\n";
			//}
			
			
			// whenever interrupt related stuff is messed with, must update the other interrupt stuff
			_CPU->UpdateInterrupt ();
			
#ifdef INLINE_DEBUG_LOAD
			debug << ";After=" << hex << _CPU->CPR0.Regs [ 12 ];
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
			//CPR0.Regs [ 13 ] = ( CPR0.Regs [ 13 ] & CauseReg_WriteMask ) | ( Value & (~CauseReg_WriteMask) );
			
			// whenever interrupt related stuff is messed with, must update the other interrupt stuff
			//UpdateInterrupt ();
			
			break;
			
			
		case 15:
#ifdef INLINE_DEBUG_LOAD
			debug << ";PRId";
#endif
							
			///////////////////////////////////////////////////
			// PRId register
			// this is register is READ-ONLY
			break;
		
		
		case 16:
			// config register - bits 3-11 and 14-15 and 19-31 are read-only
			break;
			
		default:
#ifdef INLINE_DEBUG_LOAD
			debug << ";Other";
#endif
		
			_CPU->CPR0.Regs [ Register ] = Value;
			break;
	}
}



u32 Cpu::Read_CFC1 ( u32 Register )
{
	//return CPC1 [ Register ];
	
	// for the first 16 registers return register #0
	if ( Register < 16 ) return c_lFCR0;
	
	// for the last 16 registers return register #31
	return CPC1 [ 31 ];
}


void Cpu::Write_CTC1 ( u32 Register, u32 Value )
{
	switch ( Register )
	{
		// read only
		case 0:
			break;
			
		case 31:
			// certain bits must be set or cleared
			//CPC1 [ Register ] = ( Value | c_lFCR31_SetMask ) & ~c_lFCR31_ClearMask;
			CPC1 [ 31 ] = ( CPC1 [ 31 ] & ~c_lFCR31_Mask ) | ( Value & c_lFCR31_Mask );
			break;
			
		default:
			CPC1 [ Register ] = Value;
			
			// don't know if it should be doing this
			cout << "\nhps2x64: ALERT: R5900: FPU: Writing to an unimplement register#" << dec << Register << " Value=" << hex << Value;
			break;
	}
}


void Cpu::_cb_FC ()
{
	if ( _CPU->DelaySlot1.Instruction.Rt != _CPU->LastModifiedRegister )
	{
		_CPU->GPR [ _CPU->DelaySlot1.Instruction.Rt ].u = _CPU->DelaySlot1.Data;
	}
}

void Cpu::_cb_MTC0 ()
{
	_CPU->Write_MTC0 ( _CPU->DelaySlot1.Instruction.Rd, _CPU->DelaySlot1.Data );
}

void Cpu::_cb_MTC2 ()
{
	//_CPU->COP2.Write_MTC ( _CPU->DelaySlot1.Instruction.Rd, _CPU->DelaySlot1.Data );
}

void Cpu::_cb_CTC2 ()
{
	//_CPU->COP2.Write_CTC ( _CPU->DelaySlot1.Instruction.Rd, _CPU->DelaySlot1.Data );
}


Cpu::cbFunction Cpu::Refresh_BranchDelaySlot ( Instruction::Format i )
{
	//Instruction::Format i;
	//u32 Address;
	
	//DelaySlot *d = & ( DelaySlots [ NextDelaySlotIndex ] );
	//i = d->Instruction;
	
	switch ( i.Opcode )
	{
		case OPREGIMM:
		
			//switch ( i.Rt )
			//{
				//case RTBLTZ:	// regimm
				//case RTBGEZ:	// regimm
				//case RTBLTZL:	// regimm
				//case RTBGEZL:	// regimm
				//case RTBLTZAL:	// regimm
				//case RTBGEZAL:	// regimm
				//case RTBLTZALL:	// regimm
				//case RTBGEZALL:	// regimm
					//NextPC = PC + ( i.sImmediate << 2 );
					//break;
			//}
			
			return ProcessBranchDelaySlot_t<OPREGIMM>;
			break;
		
		case OPSPECIAL:
		
			//switch ( i.Funct )
			//{
				//case SPJR:
				//case SPJALR:
					//NextPC = d->Data;
					//break;
			//}
			
			return ProcessBranchDelaySlot_t<OPSPECIAL>;
			break;
			
	
		case OPJ:	// ok
		case OPJAL:	// ok
			//NextPC = ( 0xf0000000 & PC ) | ( i.JumpAddress << 2 );
			
			return ProcessBranchDelaySlot_t<OPJ>;
			break;
			
			
		case OPBEQ:
		case OPBNE:
		case OPBLEZ:
		case OPBGTZ:
		case OPBEQL:
		case OPBNEL:
		case OPBLEZL:
		case OPBGTZL:
		case OPCOP0:
		case OPCOP1:
		case OPCOP2:
			//NextPC = PC + ( i.sImmediate << 2 );
			
			return ProcessBranchDelaySlot_t<OPBEQ>;
			break;
	}

	return NULL;
}


void Cpu::Refresh_AllBranchDelaySlots ()
{
	DelaySlots [ 0 ].cb = Refresh_BranchDelaySlot ( DelaySlots [ 0 ].Instruction );
	DelaySlots [ 1 ].cb = Refresh_BranchDelaySlot ( DelaySlots [ 1 ].Instruction );
	
	// just in case I switch back
	DelaySlot0.cb = Refresh_BranchDelaySlot ( DelaySlot0.Instruction );
	DelaySlot1.cb = Refresh_BranchDelaySlot ( DelaySlot1.Instruction );
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
	//if ( ExceptionType == EXC_SYSCALL )
	{
	debug << "\r\nSynchronous Interrupt PC=" << hex << PC << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
	}
#endif


	// step 3: shift left first 8 bits of status register by 2 and clear top 2 bits of byte
	//CPR0.Status.b0 = ( CPR0.Status.b0 << 2 ) & 0x3f;
	
	// step 4: set to kernel mode with interrupts disabled
	//CPR0.Status.IEc = 0;
	//CPR0.Status.KUc = 1;
	
	// R5900 step 1: switch to exception level 1/kernel mode (set Status.EXL=1)
	CPR0.Status.EXL = 1;
	
	// step 5: Store current address (has not been executed yet) into COP0 register "EPC" unless in branch delay slot
	// if in branch delay slot, then set field "BD" in Cause register and store address of previous instruction in COP0 register "EPC"
	// Branch Delay Slot 1 is correct since instruction is still in the process of being executed
	// probably only branches have delay slot on R5900
	//if ( DelaySlots [ NextDelaySlotIndex ].Value )
	if (Status.DelaySlot_Valid)
	{
		// we are in branch delay slot - instruction in branch delay slot has not been executed yet, since it was "interrupted"
		CPR0.Cause.BD = 1;

		// this is actually not the previous instruction, but it is the previous instruction executed
		// this allows for branches in branch delay slots
		//CPR0.EPC = LastPC;
		CPR0.EPC = PC - 4;

		// no longer want to execute the branch that is in branch delay slot
		//DelaySlots[NextDelaySlotIndex].Value = 0;
		DelaySlot0.Value = 0;

		Status.DelaySlot_Valid = 0;

		// ***testing*** may need to preserve the interrupts
		//Status.DelaySlot_Valid &= 0xfc;
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
	if (CPR0.Status.BEV == 0)
	{
		// check if tlb miss exception - skip for R3000A for now

		// set PC with interrupt vector address
		// kseg1 - 0xa000 0000 - 0xbfff ffff : translated to physical address by removing top three bits
		// this region is not cached
		//NextPC = c_GeneralInterruptVector;
		NextPC = c_CommonInterruptVector;
	}
	else
	{
		// check if tlb miss exception - skip for R3000A for now
		
		// set PC with interrupt vector address
		// kseg0 - 0x8000 0000 - 0x9fff ffff : translated to physical address by removing top bit
		// this region is cached
		//NextPC = c_BootInterruptVector;
		NextPC = c_CommonInterruptVector_BEV;
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
	debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
	DebugNextERET = 1;
#endif

#ifdef INLINE_DEBUG_SYSCALL

	
	// for PS2, the function number being called via SYSCALL is put into the R3 register
	
	// testing
	if ( GPR [ 3 ].u == 122 )
	{
		DebugNextERET = 0;
		return;
	}
	
	DebugNextERET = 1;

	// toss this in possibly
	//debug << "\r\n->PC=" << hex << PC << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	//debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
	
static const char* c_sSyscallFunctionNames [] = {	
   "0. RFU000_FullReset",
   "1 ResetEE",
   "2 SetGsCrt",
   "3 RFU003",
   "4 Exit",
   "5 RFU005",
   "6 LoadExecPS2",
   "7 ExecPS",
   "8 RFU008",
   "9 RFU009",
  "10 AddSbusIntcHandler",
  "11 RemoveSbusIntcHandler",
  "12 Interrupt2Iop",
  "13 SetVTLBRefillHandler",
  "14 SetVCommonHandler",
  "15 SetVInterruptHandler",
  "16 AddIntcHandler; 16 AddIntcHandler2",
  "17 RemoveIntcHandler",
  "18 AddDmacHandler; 18 AddDmacHandler2",
  "19 RemoveDmacHandler",
  "20 _EnableIntc",
  "21 _DisableIntc",
  "22 _EnableDmac",
  "23 _DisableDmac",
 "252 SetAlarm",
 "253 ReleaseAlarm",
 "-26 _iEnableIntc",
 "-27 _iDisableIntc",
 "-28 _iEnableDmac",
 "-29 _iDisableDmac",
 "-254 iSetAlarm",
 "-255 iReleaseAlarm",
  "32 CreateThread",
  "33 DeleteThread",
  "34 StartThread",
  "35 ExitThread",
  "36 ExitDeleteThread",
  "37 TerminateThread",
  "-38 iTerminateThread",
  "39 DisableDispatchThread",
  "40 EnableDispatchThread",
  "41 ChangeThreadPriority",
  "-42 iChangeThreadPriority",
  "43 RotateThreadReadyQueue",
  "-44 _iRotateThreadReadyQueue",
  "45 ReleaseWaitThread",
  "-46 iReleaseWaitThread",
  "47 GetThreadId",
  "48 ReferThreadStatus",
  "-49 iReferThreadStatus",
  "50 SleepThread",
  "51 WakeupThread",
  "-52 _iWakeupThread",
  "53 CancelWakeupThread",
  "-54 iCancelWakeupThread",
  "55 SuspendThread",
  "-56 _iSuspendThread",
  "57 ResumeThread",
  "-58 iResumeThread",
  "59 JoinThread",
  "60 RFU060",
  "61 RFU061",
  "62 EndOfHeap",
  "63 RFU063",
  "64 CreateSema",
  "65 DeleteSema",
  "66 SignalSema",
  "-67 iSignalSema",
  "68 WaitSema",
  "69 PollSema",
  "-70 iPollSema",
  "71 ReferSemaStatus",
  "-72 iReferSemaStatus>:",
  "73 RFU073",
  "74 SetOsdConfigParam",
  "75 GetOsdConfigParam",
  "76 GetGsHParam",
  "77 GetGsVParam",
  "78 SetGsHParam",
  "79 SetGsVParam",
  "80 RFU080_CreateEventFlag",
  "81 RFU081_DeleteEventFlag",
  "82 RFU082_SetEventFlag",
  "-83 RFU083_iSetEventFlag",
  "84 RFU084_ClearEventFlag",
  "-85 RFU085_iClearEventFlag",
  "86 RFU086_WaitEvnetFlag",
  "87 RFU087_PollEvnetFlag",
  "-88 RFU088_iPollEvnetFlag",
  "89 RFU089_ReferEventFlagStatus",
  "-90 RFU090_iReferEventFlagStatus",
  "91 RFU091",
  "92 EnableIntcHandler; -92 iEnableIntcHandler",
  "93 DisableIntcHandler; -93 iDisableIntcHandler",
  "94 EnableDmacHandler; -94 iEnableDmacHandler",
  "95 DisableDmacHandler; -95 iDisableDmacHandler",
  "96 KSeg0",
  "97 EnableCache",
  "98 DisableCache",
  "99 GetCop0",
 "100 FlushCache",
 "101 Unknown",
 "102 CpuConfig",
 "-103 iGetCop0",
 "-104 iFlushCache",
 "105 Unknown",
 "-106 iCpuConfig",
 "107 sceSifStopDma",
 "108 SetCPUTimerHandler",
 "109 SetCPUTimer",
 "110 SetOsdConfigParam2",
 "111 GetOsdConfigParam2",
 "112 GsGetIMR; -112 iGsGetIMR",
 "113 GsPutIMR; -113 iGsPutIMR",
 "114 SetPgifHandler",
 "115 SetVSyncFlag",
 "116 RFU116",
 "117 _print",
 "118 sceSifDmaStat; -118 isceSifDmaStat",
 "119 sceSifSetDma; -119 isceSifSetDma",
 "120 sceSifSetDChain; -120 isceSifSetDChain",
 "121 sceSifSetReg",
 "122 sceSifGetReg",
 "123 ExecOSD",
 "124 Deci2Call",
 "125 PSMode",
 "126 MachineType",
 "127 GetMemorySize" };
	
	if ( CPR0.Cause.ExcCode == EXC_SYSCALL )
	{
		u32 SyscallNumber;
		SyscallNumber = GPR [ 3 ].s;
		if ( GPR [ 3 ].s < 0 ) SyscallNumber = -GPR [ 3 ].s;
		if ( SyscallNumber != 124 )
		{
		debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: ";
		
		// output syscall function name
		if ( SyscallNumber < 128 )
		{
			debugBios << c_sSyscallFunctionNames [ SyscallNumber ];
		}
		else
		{
			debugBios << "Unknown";
		}
		
		debugBios << "; r3 (syscall number) = " << dec << GPR [ 3 ].s;
		debugBios << "; r1 = " << hex << GPR [ 1 ].u;
		debugBios << "; r2 = " << GPR [ 2 ].u;
		debugBios << "; r4 = " << GPR [ 4 ].u;
		debugBios << "; r5 = " << GPR [ 5 ].u;
		debugBios << "; r6 = " << GPR [ 6 ].u;
		}
		else
		{
			static u32 PollCount;
			
			// check reason for deci2call
			// call number is in a0
			switch ( GPR [ 4 ].u )
			{
				// open
				case 0x1:
					PollCount = 0;
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "open";
					break;
					
				// close
				case 0x2:
					PollCount = 0;
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "close";
					break;
					
				// reqsend
				case 0x3:
					PollCount = 0;
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "reqsend";
					break;
					
				// poll
				case 0x4:
					if ( !PollCount )
					{
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "poll";
					PollCount++;
					}
					break;
				
				// exrecv
				case 0x5:
					PollCount = 0;
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "exrecv";
					break;
				
				// exsend
				case 0x6:
					PollCount = 0;
					debugBios << "\r\nSyscall; " << hex << setw(8) << PC << " " << dec << CycleCount << " Function: deci2call->";
					debugBios << "exsend";
					break;
					
				// kputs
				case 0x10:
					PollCount = 0;
					debugBios << "kputs";
					break;
					
				// unknown
				default:
					debugBios << "unknown";
					break;
			}
		}
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
	debug << "\r\nASYNC-INT PC=" << hex << PC << " " << dec << CycleCount << hex << " NextPC=" << NextPC << " LastPC=" << LastPC << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ];
	debug << hex << " ISTAT=" << *_Debug_IntcStat << " IMASK=" << *_Debug_IntcMask;
	//debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
	//debug << "\r\nR29=" << _CPU->GPR [ 29 ].uw0 << " R31=" << _CPU->GPR [ 31 ].uw0;
	// << " 0x3201d0=" << Bus->MainMemory.b32 [ 0x3201d0 >> 2 ];
	DebugNextERET = 1;
#endif

#ifdef VERBOSE_MULTIPLE_INT_CAUSE
	// debugging - check for multiple interrupt reasons
	if ( ( CPR0.Regs [ 12 ] & CPR0.Regs [ 13 ] & 0xc00 ) == 0xc00 )
	{
		cout << "\n******************************************";
		cout << "\nhps2x64: R5900: multiple interrupt causes.\n";
		cout << "********************************************\n";
	}
#endif
	
	// step 1: make sure interrupts are enabled globally (Status register field "IEc" equal to 1) and not masked
	// and that external interrupts are enabled (Status register bit 10 equal to 1)
	// *** note *** already checked for this stuff
//	if ( ( CPR0.Status.IEc == 1 ) && ( CPR0.Regs [ 12 ] & CPR0.Regs [ 13 ] & 0x4 ) )
//	{
		// step 2: make sure corresponding interrupt bit is set (enabled) in 8-bit Status register field "Im" (Interrupt Mask)
		
		// step 3: shift left first 8 bits of status register by 2 and clear top 2 bits of byte
		//CPR0.Status.b0 = ( CPR0.Status.b0 << 2 ) & 0x3f;
		
		// step 4: set to kernel mode with interrupts disabled
		//CPR0.Status.IEc = 0;
		//CPR0.Status.KUc = 1;
		
		// R5900 step 1: switch to exception level 1/kernel mode (set Status.EXL=1)
		CPR0.Status.EXL = 1;

		// step 5: Store current address (has not been executed yet) into COP0 register "EPC" unless in branch delay slot
		// if in branch delay slot, then set field "BD" in Cause register and store address of previous instruction in COP0 register "EPC"
		// for R5900, the only instruction probably with delay slot is branch
		if (Status.DelaySlot_Valid)
		{
#ifdef INLINE_DEBUG_ASYNC_INT
			//debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
#endif

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
			//DelaySlots [ NextDelaySlotIndex ].Value = 0;
			DelaySlot0.Value = 0;

			Status.DelaySlot_Valid = 0;

			// ***testing*** may need to preserve interrupts
			//Status.DelaySlot_Valid &= 0xfc;
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
		if (CPR0.Status.BEV == 0)
		{
			// check if tlb miss exception - skip for R3000A for now

			// set PC with interrupt vector address
			// kseg0 - 0x8000 0000 - 0x9fff ffff : translated to physical address by removing top bit
			// this region is cached
			//PC = c_GeneralInterruptVector;
			PC = c_ExternalInterruptVector;
		}
		else
		{
			// check if tlb miss exception - skip for R3000A for now

			// set PC with interrupt vector address
			// kseg1 - 0xa000 0000 - 0xbfff ffff : translated to physical address by removing top three bits
			// this region is not cached
			//PC = c_BootInterruptVector;
			PC = c_ExternalInterruptVector_BEV;
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
	debug << "\r\n->PC=" << hex << PC << hex << " Status=" << CPR0.Regs [ 12 ] << " Cause=" << CPR0.Regs [ 13 ] << " EPC=" << CPR0.EPC << " BD=" << CPR0.Cause.BD;
	//debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( DelaySlot1.Instruction.Value ).c_str () << "  " << hex << DelaySlot1.Instruction.Value;
#endif
}











void Cpu::SetPC ( u32 Value )
{
	PC = Value;
}





#ifdef ENABLE_GUI_DEBUGGER
void Cpu::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* COP0_Names [] = { "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "Reserved",
								"BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause", "EPC", "PRId",
								"Config", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "BadPAddr",
								"Debug", "Perf", "Reserved", "Reserved", "TagLo", "TagHi", "ErrorEPC", "Reserved" };
								
	static constexpr const char* DisAsm_Window_ColumnHeadings [] = { "Address", "@", ">", "Instruction", "Inst (hex)" };
								
	static constexpr const char* FontName = "Courier New";
	static constexpr int FontSize = 6;
	
	static constexpr const char* DebugWindow_Caption = "R5900 Debug Window";
	static constexpr int DebugWindow_X = 10;
	static constexpr int DebugWindow_Y = 10;
	static constexpr int DebugWindow_Width = 995;
	static constexpr int DebugWindow_Height = 420;
	
	static constexpr int GPRList_X = 0;
	static constexpr int GPRList_Y = 0;
	static constexpr int GPRList_Width = 190;
	static constexpr int GPRList_Height = 380;

	static constexpr int COP1List_X = GPRList_X + GPRList_Width;
	static constexpr int COP1List_Y = 0;
	static constexpr int COP1List_Width = 175;
	static constexpr int COP1List_Height = 300;
	
	static constexpr int COP2_CPCList_X = COP1List_X + COP1List_Width;
	static constexpr int COP2_CPCList_Y = 0;
	static constexpr int COP2_CPCList_Width = 175;
	static constexpr int COP2_CPCList_Height = 300;
	
	static constexpr int COP2_CPRList_X = COP2_CPCList_X + COP2_CPCList_Width;
	static constexpr int COP2_CPRList_Y = 0;
	static constexpr int COP2_CPRList_Width = 175;
	static constexpr int COP2_CPRList_Height = 300;
	
	static constexpr int DisAsm_X = COP2_CPRList_X + COP2_CPRList_Width;
	static constexpr int DisAsm_Y = 0;
	static constexpr int DisAsm_Width = 270;
	static constexpr int DisAsm_Height = 380;
	
	static constexpr int MemoryViewer_Columns = 8;
	static constexpr int MemoryViewer_X = GPRList_X + GPRList_Width;
	static constexpr int MemoryViewer_Y = 300;
	static constexpr int MemoryViewer_Width = 250;
	static constexpr int MemoryViewer_Height = 80;
	
	static constexpr int BkptViewer_X = MemoryViewer_X + MemoryViewer_Width;
	static constexpr int BkptViewer_Y = 300;
	static constexpr int BkptViewer_Width = 275;
	static constexpr int BkptViewer_Height = 80;
	
	int i;
	stringstream ss;
	
#ifdef INLINE_DEBUG
	debug << "\r\nStarting Cpu::DebugWindow_Enable";
#endif
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->Set_Font ( DebugWindow->CreateFontObject ( FontSize, FontName ) );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		GPR_ValueList = new DebugValueList<u32> ();
		COP0_ValueList = new DebugValueList<u32> ();
		COP2_CPCValueList = new DebugValueList<float> ();
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
			ss << "R" << dec << i << "_x";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->GPR [ i ].uw0) );
			ss.str ("");
			ss << "R" << dec << i << "_y";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->GPR [ i ].uw1) );
			ss.str ("");
			ss << "R" << dec << i << "_z";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->GPR [ i ].uw2) );
			ss.str ("");
			ss << "R" << dec << i << "_w";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->GPR [ i ].uw3) );
			
			COP0_ValueList->AddVariable ( COP0_Names [ i ], &(_CPU->CPR0.Regs [ i ]) );

			// add FPU vars too
			ss.str ("");
			ss << "f" << dec << i;
			COP2_CPCValueList->AddVariable ( ss.str().c_str(), &(_CPU->CPR1 [ i ].f) );
			
			/*
			if ( i < 16 )
			{
				ss.str("");
				ss << "CPC2_" << dec << ( i << 1 );
				COP2_CPCValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ i << 1 ]) );
				
				ss.str("");
				ss << "CPC2_" << dec << ( ( i << 1 ) + 1 );
				COP2_CPCValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ ( i << 1 ) + 1 ]) );
			}
			else
			{
				ss.str("");
				ss << "CPR2_" << dec << ( ( i - 16 ) << 1 );
				COP2_CPRValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( i - 16 ) << 1 ]) );
				
				ss.str("");
				ss << "CPR2_" << dec << ( ( ( i - 16 ) << 1 ) + 1 );
				COP2_CPRValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( ( i - 16 ) << 1 ) + 1 ]) );
			}
			*/
		}
		
		// Don't forget to show the HI and LO registers
		//GPR_ValueList->AddVariable ( "LO", &(_CPU->HiLo.uLo) );
		GPR_ValueList->AddVariable ( "LOx", &(_CPU->LO.uw0) );
		GPR_ValueList->AddVariable ( "LOy", &(_CPU->LO.uw1) );
		GPR_ValueList->AddVariable ( "LOz", &(_CPU->LO.uw2) );
		GPR_ValueList->AddVariable ( "LOw", &(_CPU->LO.uw3) );
		//GPR_ValueList->AddVariable ( "HI", &(_CPU->HiLo.uHi) );
		GPR_ValueList->AddVariable ( "HIx", &(_CPU->HI.uw0) );
		GPR_ValueList->AddVariable ( "HIy", &(_CPU->HI.uw1) );
		GPR_ValueList->AddVariable ( "HIz", &(_CPU->HI.uw2) );
		GPR_ValueList->AddVariable ( "HIw", &(_CPU->HI.uw3) );
		
		// also add PC and CycleCount
		GPR_ValueList->AddVariable ( "PC", &(_CPU->PC) );
		GPR_ValueList->AddVariable ( "NextPC", &(_CPU->NextPC) );
		GPR_ValueList->AddVariable ( "LastPC", &(_CPU->LastPC) );
		GPR_ValueList->AddVariable ( "CycleLO", (u32*) &(_CPU->CycleCount) );
		
		GPR_ValueList->AddVariable ( "LastReadAddress", &(_CPU->Last_ReadAddress) );
		GPR_ValueList->AddVariable ( "LastWriteAddress", &(_CPU->Last_WriteAddress) );
		GPR_ValueList->AddVariable ( "LastReadWriteAddress", &(_CPU->Last_ReadWriteAddress) );
		
		// need to add in load delay slot values
		GPR_ValueList->AddVariable ( "D0_INST", &(_CPU->DelaySlot0.Instruction.Value) );
		GPR_ValueList->AddVariable ( "D0_VAL", &(_CPU->DelaySlot0.Data) );
		GPR_ValueList->AddVariable ( "D1_INST", &(_CPU->DelaySlot1.Instruction.Value) );
		GPR_ValueList->AddVariable ( "D1_VAL", &(_CPU->DelaySlot1.Data) );
		
		//GPR_ValueList->AddVariable ( "SPUCC", (u32*) _SpuCycleCount );
		
		GPR_ValueList->AddVariable ( "Trace", &TraceValue );
		
		GPR_ValueList->AddVariable ( "PCount", & Playstation2::GPU::_GPU->Primitive_Count );
		
		GPR_ValueList->AddVariable ( "a0", (u32*) & ( Bus->BIOS.b32 [ 0x251f80 >> 2 ] ) );
		GPR_ValueList->AddVariable ( "a1", (u32*) & ( Bus->MainMemory.b32 [ 0x1e290 >> 2 ] ) );
		GPR_ValueList->AddVariable ( "a2", (u32*) & ( Bus->MainMemory.b32 [ 0x1e294 >> 2 ] ) );
		GPR_ValueList->AddVariable ( "a3", (u32*) & ( Bus->MainMemory.b32 [ 0x1edc0 >> 2 ] ) );
		GPR_ValueList->AddVariable ( "a4", (u32*) & ( Bus->MainMemory.b32 [ 0x1edc4 >> 2 ] ) );
		//GPR_ValueList->AddVariable ( "a1", (u32*) & ( VU::_VU[1]->VuMem64 [ ( 0x56 << 1 ) + 1 ] ) );
		//GPR_ValueList->AddVariable ( "a0", &_CPU->testvar [ 0 ] );
		//GPR_ValueList->AddVariable ( "a1", &_CPU->testvar [ 1 ] );
		//GPR_ValueList->AddVariable ( "a2", &_CPU->testvar [ 2 ] );
		//GPR_ValueList->AddVariable ( "a3", &_CPU->testvar [ 3 ] );
		//GPR_ValueList->AddVariable ( "a4", &_CPU->testvar [ 4 ] );
		GPR_ValueList->AddVariable ( "a5", &_CPU->testvar [ 5 ] );
		GPR_ValueList->AddVariable ( "a6", &_CPU->testvar [ 6 ] );
		GPR_ValueList->AddVariable ( "a7", &_CPU->testvar [ 7 ] );
		
		GPR_ValueList->AddVariable ( "GNE", (u32*) &(Playstation2::GPU::_GPU->lVBlank) );

		// I can always move these next ones around if needed
		//GPR_ValueList->AddVariable ( "LDValid", &(_CPU->Status.LoadStore_Valid) );
		
		// add the loading bitmap
		//GPR_ValueList->AddVariable ( "LoadBitmap", &(_CPU->GPRLoading_Bitmap) );
		
		/*
		for ( i = 0; i < 4; i++ )
		{
			ss.str ("");
			ss << "LD" << i << "_INST";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->LoadBuffer.Buf [ i ].Inst.Value) );
			ss.str ("");
			ss << "LD" << i << "_ADDR";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->LoadBuffer.Buf [ i ].Address) );
			ss.str ("");
			ss << "LD" << i << "_VALUE";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->LoadBuffer.Buf [ i ].Value) );
			ss.str ("");
			ss << "LD" << i << "_INDEX";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->LoadBuffer.Buf [ i ].Index) );
		}
		*/

		/*
		for ( i = 0; i < 4; i++ )
		{
			ss.str ("");
			ss << "ST" << i << "_INST";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->StoreBuffer.Buf [ i ].Inst.Value) );
			ss.str ("");
			ss << "ST" << i << "_ADDR";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->StoreBuffer.Buf [ i ].Address) );
			ss.str ("");
			ss << "ST" << i << "_VALUE";
			GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->StoreBuffer.Buf [ i ].Value) );
			ss.str ("");
			//ss << "ST" << i << "_INDEX";
			//GPR_ValueList->AddVariable ( ss.str().c_str(), &(_CPU->StoreBuffer.Buf [ i ].Index) );
		}
		*/

		// temporary space
		//GPR_ValueList->AddVariable ( "0x1F8001A8", (u32*) &(_CPU->DCache.b8 [ 0x1A8 ]) );

		// create the disassembly window
		DisAsm_Window = new Debug_DisassemblyViewer ( Breakpoints );
		DisAsm_Window->Create ( DebugWindow, DisAsm_X, DisAsm_Y, DisAsm_Width, DisAsm_Height, (Debug_DisassemblyViewer::DisAsmFunction) R5900::Instruction::Print::PrintInstruction );
		
		DisAsm_Window->Add_MemoryDevice ( "RAM", Playstation2::DataBus::MainMemory_Start, Playstation2::DataBus::MainMemory_Size, (u8*) Bus->MainMemory.b8 );
		DisAsm_Window->Add_MemoryDevice ( "BIOS", Playstation2::DataBus::BIOS_Start, Playstation2::DataBus::BIOS_Size, (u8*) Bus->BIOS.b8 );
		
		DisAsm_Window->SetProgramCounter ( &_CPU->PC );
		
		
		// create window area for breakpoints
		Breakpoint_Window = new Debug_BreakpointWindow ( Breakpoints );
		Breakpoint_Window->Create ( DebugWindow, BkptViewer_X, BkptViewer_Y, BkptViewer_Width, BkptViewer_Height );
		
		// create the viewer for D-Cache scratch pad
		ScratchPad_Viewer = new Debug_MemoryViewer ();
		
		ScratchPad_Viewer->Create ( DebugWindow, MemoryViewer_X, MemoryViewer_Y, MemoryViewer_Width, MemoryViewer_Height, MemoryViewer_Columns );
		//ScratchPad_Viewer->Add_MemoryDevice ( "ScratchPad", c_ScratchPadRam_Addr, c_ScratchPadRam_Size, _CPU->DCache.b8 );
		ScratchPad_Viewer->Add_MemoryDevice ( "ScratchPad", Playstation2::DataBus::ScratchPad_Start, Playstation2::DataBus::ScratchPad_Size, (u8*) Bus->ScratchPad.b8 );
		
		
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

#endif


}



