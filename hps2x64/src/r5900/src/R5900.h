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
    aint32_t with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#pragma once
#ifndef __R5900_H__
#define __R5900_H__


#include <sstream>

#ifdef ENABLE_GUI_DEBUGGER
#include "Debug.h"
#include "WinApiHandler.h"
// Windows API specific code is in this header file
#include "DebugValueList.h"
//#include "InfiniteListView.h"
#include "DisassemblyViewer.h"
#include "BreakpointWindow.h"
#endif


#include "types.h"

//#include "GNUAsmUtility_x64.h"

#include "R5900_Instruction.h"
//#include "R5900_Execute.h"
#include "R5900_ICache.h"
#include "R5900_DCache.h"

#include "PS2DataBus.h"

// include recompiler
//#include "R5900_Recompiler.h"


// some early branch prediction simulation
#define ENABLE_R5900_BRANCH_PREDICTION_REGIMM
#define ENABLE_R5900_BRANCH_PREDICTION_SPECIAL
#define ENABLE_R5900_BRANCH_PREDICTION_JUMP



using namespace std;
//using namespace x64Asm::Utilities;
using namespace R5900::Instruction;

class Debug_MemoryViewer;


namespace Playstation2
{
	class DataBus;
}


namespace R5900
{
	class Recompiler;

	struct EntryLo0_Reg
	{
		union
		{
			uint32_t l;

			struct
			{
				// bit 0 - Global - r/w
				uint32_t G : 1;
				
				// bit 1 - Valid - set to 1 if the TLB entry is enabled - r/w
				uint32_t V : 1;
				
				// bit 2 - Dirty - set to 1 if writable - r/w
				uint32_t D : 1;
				
				// bits 3-5 - TLB page coherency - 2: uncached, 3: Cached, 7: uncached accelerated - r/w
				uint32_t C : 3;
				
				// bits 6-25 - Page frame number (the upper bits of the physical address) - r/w
				uint32_t PFN : 20;
				
				// bits 26-30 - zero
				uint32_t zero : 5;
				
				// bit 31 - Memory type - 0: Main Memory, 1: Scratchpad RAM - r/w - only for EntryLo0
				uint32_t S : 1;
			};

		};
	};
	
	struct EntryLo1_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bit 0 - Global - r/w
				uint32_t G : 1;
				
				// bit 1 - Valid - set to 1 if the TLB entry is enabled - r/w
				uint32_t V : 1;
				
				// bit 2 - Dirty - set to 1 if writable - r/w
				uint32_t D : 1;
				
				// bits 3-5 - TLB page coherency - 2: uncached, 3: Cached, 7: uncached accelerated - r/w
				uint32_t C : 3;
				
				// bits 6-25 - Page frame number (the upper bits of the physical address) - r/w
				uint32_t PFN : 20;
			};
		};
	};

	struct Context_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-3 - zero
				uint32_t zero : 4;
				
				// bits 4-22 - Virtual page address that caused TLB miss
				uint32_t BadVPN2 : 19;
				
				// bits 23-31 - Page table address
				uint32_t PTEBase : 9;
			};
		};
	};
	
	struct PageMask_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-12 - zero
				uint32_t zero : 13;
				
				// bits 13-24 - Page size comparison mask - 0: 4KB, 3: 16KB, 15: 64KB, 63: 256KB, 65536: 1MB, 256K-1: 4MB, 1M-1: 16MB - r/w
				// start value:
				uint32_t MASK : 12;
			};
		};
	};

	struct EntryHi_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-7 - ASID - Address space ID - r/w
				// initial value:
				uint32_t ASID : 8;
				
				// bits 8-12 - zero
				uint32_t zero : 5;
				
				// bits 13-31 - VPN2 - Virtual page number divided by two - r/w
				// initial value:
				uint32_t VPN2 : 19;
			};
		};
	};

	struct Status_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bit 0 - Interrupt enable
				uint32_t IE : 1;
				
				// bit 1 - Exception level
				uint32_t EXL : 1;
				
				// bit 2 - Error level
				uint32_t ERL : 1;
				
				// bits 3-4 - Operation mode - 0: Kernel mode, 1: Supervisor mode, 2: User mode - r/w - start value:
				uint32_t KSU : 2;
				
				// bits 5-9 - zero
				uint32_t zero0 : 5;
				
				// bits 10-11,15 - Interrupt mask - 0: disables interrupts, 1: enables interrupts - r/w
				uint32_t IM : 2;

				// bit 12 - Bus error mask - 0: signals a bus error, 1: masks a bus error - r/w
				uint32_t BEM : 1;
				
				// bits 13-14 - zero
				uint32_t zero1 : 2;
				
				// bit 15 - Interrupt mask (con't)
				uint32_t IM7 : 1;
				
				// bit 16 - Enable IE - 0: disables all interrupts regardless of IE, 1: enables IE
				uint32_t EIE : 1;
				
				// bit 17 - "EDI" - EI/DI instruction enable - 0: enabled only in kernel mode, 1: enabled in all modes - r/w
				uint32_t EDI : 1;
				
				// bit 18 - "CH" - Status of the most recent cache instruction for the data cache - 0: miss, 1: hit - r/w
				uint32_t CacheHistory : 1;
				
				// bits 19-21 - zero
				uint32_t zero2 : 3;
				
				// bit 22 - Controls address of the TLB Refill or general exception vectors - r/w
				// start value: 1
				uint32_t BEV : 1;
				
				// bit 23 - Controls address of performance counter and the debug exception vectors - r/w
				uint32_t DEV : 1;
				
				// bits 24-27 - zero
				uint32_t zero3 : 4;
						
				// bits 28-31 - Usability of each coprocessor unit - 0: Unusable, 1: Usable - r/w
				uint32_t CU0 : 1;
				uint32_t CU1 : 1;
				uint32_t CU2 : 1;	// this bit MUST be set before you can use COP2 (GTE)
				uint32_t CU3 : 1;
			};
		};
	};

	struct Cause_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-1 - zero
				uint32_t zero0 : 2;
				
				// bits 2-6 - Exception code - 0: Interrupt, 1: TLB Modified, 2: TLB Refill, 3: TLB Refill (store), 4: Address Error, 5: Address Error (store)
				// 6: Bus Error (instruction), 7: Bus Error (data), 8: System Call, 9: Breakpoint, 10: Reserved instruction, 11: Coprocessor Unusable
				// 12: Overflow, 13: Trap
				uint32_t ExcCode : 5;
				
				// bits 7-9 - zero
				uint32_t zero1 : 3;
				
				// bit 10 - Set when Int[1] interrupt is pending
				uint32_t IP2 : 1;
				
				// bit 11 - Set when Int[0] interrupt is pending
				uint32_t IP3 : 1;
				
				// bits 12-14 - zero
				uint32_t zero2 : 3;
				
				// bit 15 - Set when a timer interrupt is pending
				uint32_t IP7 : 1;
				
				// bits 16-18 - Exception codes for level 2 exceptions - 0: Reset, 1: NMI, 2: Performance Counter, 3: Debug
				uint32_t EXC2 : 3;
				
				// bits 19-27 - zero
				uint32_t zero3 : 9;
				
				// bits 28-29 - Coprocessor number when a coprocessor unusable exception is taken
				uint32_t CE : 2;
				
				// bit 30 - Set when level 2 exception occurs in a branch delay slot
				uint32_t BD2 : 1;
				
				// bit 31 - Set when a level 1 exception occurs in a branch delay slot
				uint32_t BD : 1;
			};
		};
	};

	struct PRId_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-7 - Revision number - read only
				// start value: revision number -> set to zero
				uint32_t Rev : 8;
				
				// bits 8-15 - Implementation number - read only
				// start value: 0x2e on R5900
				// start value: 3 on R3000A
				uint32_t Imp : 8;

				// bits 16-31 - reserved
				uint32_t reserved0 : 16;
			};
		};
	};

	struct Config_Reg
	{
		union
		{
			uint32_t l;
			
			struct
			{
				// bits 0-2 - kseg0 cache mode - 0: cached w/o write-back or -allocate, 2: Uncached, 3: Cached, 7: Uncached accelerated
				// start value:
				uint32_t K0 : 3;
				
				// bits 3-5 - zero
				uint32_t zero0 : 3;
				
				// bits 6-8 - Data cache size - read only - 0: 4KB, 1: 8KB, 2: 16KB
				// start value: 1
				uint32_t DC : 3;
				
				// bits 9-11 - Instruction cache size - read only - 0: 4KB, 1: 8KB, 2: 16KB
				// start value: 2
				uint32_t IC : 3;
				
				// bit 12 - Setting this bit to 1 enables branch prediction
				// start value: 0
				uint32_t BPE : 1;
				
				// bit 13 - Setting this bit to 1 enables non-blocking load
				// start value: 0
				uint32_t NBE : 1;
				
				// bits 14-15 - zero
				uint32_t zero1 : 2;
				
				// bit 16 - Setting this bit to 1 enables the data cache
				// start value: 0
				uint32_t DCE : 1;
				
				// bit 17 - Setting this bit to 1 enables the instruction cache
				// start value: 0
				uint32_t ICE : 1;
				
				// bit 18 - Setting this bit to 1 enables the pipeline parallel issue
				// start value: 0
				uint32_t DIE : 1;
				
				// bits 19-27 - zero
				uint32_t zero2 : 9;
				
				// bits 28-30 - Bus clock ratio - 0: processor clock frequency divided by 2
				// start value: 0
				uint32_t EC : 3;
			};
		};
	};

	struct CPR0_Regs
	{
	
		union
		{

			// there are 32 COP0 control registers
			uint32_t Regs [ 32 ];
			
			struct
			{
			
				// Register #0 - "Index"
				// Index that specifies TLB entry for reading or writing - MMU
				// Index - bits 0-5 - r/w
				// start value:
				uint32_t Index;
				
				// Register #1 - "Random" - Read Only
				// Desc: Index that specifies TLB entry for the TLBWR instruction
				// Purpose: MMU
				// Random - bits 0-5 - read only
				// start value:
				uint32_t Random;
				
				// Register #2 - "EntryLo0" - Lower part of the TLB entry - MMU
				EntryLo0_Reg EntryLo0;
				
				// Register #3 - "EntryLo1" - Lower part of the TLB entry - MMU
				// On R3000A this is actually "BPC" - rw - breakpoint on execute
				EntryLo1_Reg EntryLo1;
				
				// Register #4 - "Context" - TLB miss handling information
				Context_Reg Context;
				
				// Register #5 - "PageMask" - Page size comparison mask
				// On R3000A this is actually "BDA" - rw - breakpoint on data access
				PageMask_Reg PageMask;
				
				// Register #6 - "Wired" - The number of wired TLB entries
				// bits 0-5 - Wired
				// On R3000A this may be "PIDMASK"
				uint32_t Wired;
				
				// Register #7 - Reserved
				// On R3000A this is DCIC - rw - breakpoint control
				uint32_t Reserved0;
				
				// Register #8 - "BadVAddr" - Virtual address that causes an error - bits 0-31
				uint32_t BadVAddr;
				
				// Register #9 - "Count" - Timer count value - incremented every clock cycle - r/w
				// On R3000A this is "BDAM" - rw - data access breakpoint mask
				uint32_t Count;
				
				// Register #10 - "EntryHi" - upper parts of a TLB entry
				EntryHi_Reg EntryHi;
				
				// Register #11 - "Compare" - Timer stable value - when the "Count" register reaches this value an interrupt occurs - r/w
				// On R3000A this is "BPCM" - rw - execute breakpoint mask
				uint32_t Compare;
				
				// Register #12 - "Status" - COP0 Status
				Status_Reg Status;
				
				// Register #13 - "Cause" - Cause of the most recent exception
				Cause_Reg Cause;
				
				// Register #14 - "EPC" - address that is to resume after an exception has been serviced
				uint32_t EPC;
				
				// Register #15 - "PRId" - Processor revision
				// Imp is 3 on R3000A
				PRId_Reg PRId;

				// Register #16 - "Config" - Processor configuration
				// On R3000A this may be "ERREG"
				Config_Reg Config;
				
				// Registers #17-#22 - Reserved
				uint32_t Reserved1;
				uint32_t Reserved2;
				uint32_t Reserved3;
				uint32_t Reserved4;
				uint32_t Reserved5;
				uint32_t Reserved6;
				
				// Register #23 - "BadPAddr" - Physical address that caused an error - lower 4 bits are always zero
				// this may or may not be used on R3000A, need to check this
				uint32_t BadPAddr;
				
				// Register #24 - "Debug"
				uint32_t Debug;
				
				// Register #25 - "Perf" - Performance Counter
				uint32_t Perf;
				
				// Registers #26-#27 - Reserved
				uint32_t Reserved7;
				uint32_t Reserved8;
				
				// Register #28 - "TagLo" - Low bits of the Cache Tag
				uint32_t TagLo;
				
				// Register #29 - "TagHi" - High bits of the Cache Tag
				uint32_t TagHi;
				
				// Register #30 - "ErrorEPC" - Error exception program counter
				uint32_t ErrorEPC;
				
				// Register #31 - Reserved
				uint32_t Reserved9;
			};
			
		};

	};


	/*
	union COP2_StatusFlags
	{
		u32 Value;
		
		struct
		{
			// bit 0 - zero flag - gets set when any of the Zx, Zy, Zz, Zw flags are set in MAC flag
			u32 ZeroFlag : 1;
			
			// bit 1 - sign flag
			u32 SignFlag : 1;
			
			// bit 2 - underflow flag
			u32 UnderflowFlag : 1;
			
			// bit 3 - overflow flag
			u32 OverflowFlag : 1;
			
			// bit 4 - invalid flag - gets set on either 0/0 or SQRT/RSQRT on a negative number
			u32 InvalidFlag : 1;
			
			// bit 5 - division by zero flag - get set when either DIV/RSQRT does division by zero
			u32 DivideByZeroFlag : 1;
			
			// bit 6 - sticky zero flag - gets set when any of the Zx, Zy, Zz, Zw flags are set in MAC flag
			u32 StickyZeroFlag : 1;
			
			// bit 7 - sticky sign flag
			u32 StickySignFlag : 1;
			
			// bit 8 - sticky underflow flag
			u32 StickyUnderflowFlag : 1;
			
			// bit 9 - sticky overflow flag
			u32 StickyOverflowFlag : 1;
			
			// bit 10 - sticky invalid flag - gets set on either 0/0 or SQRT/RSQRT on a negative number
			u32 StickyInvalidFlag : 1;
			
			// bit 11 - sticky division by zero flag - get set when either DIV/RSQRT does division by zero
			u32 StickyDivideByZeroFlag : 1;
		};
	};

	union COP2_MACFlags
	{
		u32 Value;
		
		struct
		{
			// bit 0 - W zero flag - set when W component of float register result is zero
			u32 wZeroFlag : 1;
			
			// bit 1 - Z zero flag - set when Z component of float register result is zero
			u32 zZeroFlag : 1;
			
			// bit 2 - Y zero flag - set when Y component of float register result is zero
			u32 yZeroFlag : 1;
			
			// bit 3 - X zero flag - set when X component of float register result is zero
			u32 xZeroFlag : 1;
			
			u32 wSignFlag : 1;
			u32 zSignFlag : 1;
			u32 ySignFlag : 1;
			u32 xSignFlag : 1;
			
			u32 wUnderflowFlag : 1;
			u32 zUnderflowFlag : 1;
			u32 yUnderflowFlag : 1;
			u32 xUnderflowFlag : 1;
			
			u32 wOverflowFlag : 1;
			u32 zOverflowFlag : 1;
			u32 yOverflowFlag : 1;
			u32 xOverflowFlag : 1;
		};
	};


	union COP2_ClippingFlag
	{
		u32 Value;
		
		struct
		{
			u32 PlusX0 : 1;
			u32 MinusX0 : 1;
			u32 PlusY0 : 1;
			u32 MinusY0 : 1;
			u32 PlusZ0 : 1;
			u32 MinusZ0 : 1;
			
			u32 PlusX1 : 1;
			u32 MinusX1 : 1;
			u32 PlusY1 : 1;
			u32 MinusY1 : 1;
			u32 PlusZ1 : 1;
			u32 MinusZ1 : 1;
			
			u32 PlusX2 : 1;
			u32 MinusX2 : 1;
			u32 PlusY2 : 1;
			u32 MinusY2 : 1;
			u32 PlusZ2 : 1;
			u32 MinusZ2 : 1;
			
			u32 PlusX3 : 1;
			u32 MinusX3 : 1;
			u32 PlusY3 : 1;
			u32 MinusY3 : 1;
			u32 PlusZ3 : 1;
			u32 MinusZ3 : 1;
			
			u32 zero : 8;
		};
	
	};
	
	struct CPR2_ControlRegs
	{
	
		union
		{

			// there are 32 COP2 control registers
			uint32_t Regs [ 32 ];
			
			struct
			{
				u32 Reg0;
				u32 Reg1;
				u32 Reg2;
				u32 Reg3;
				u32 Reg4;
				u32 Reg5;
				u32 Reg6;
				u32 Reg7;
				u32 Reg8;
				u32 Reg9;
				u32 Reg10;
				u32 Reg11;
				u32 Reg12;
				u32 Reg13;
				u32 Reg14;
				u32 Reg15;
				COP2_StatusFlags StatusFlags;
				COP2_MACFlags MACFlags;
				COP2_ClippingFlag ClippingFlags;
				u32 Reserved0;
				
				// these three cannot be read or written while VU0 is operating
				u32 R;
				u32 I;
				u32 Q;
				
				// this one is only bits 0-15, the rest are zero
				// cannot be read or written while VU0 is operating
				u32 TPC;
			};
		};
		
	};



	struct CPR2_DataRegs
	{
	
		union
		{

			// there are 32 COP2 data registers
			uint32_t Regs [ 32 ];
			
		};
		
	};
	*/



	union FloatReg
	{
		u32 u;
		s32 s;
		float f;
	};



	
	
	union ProcStatus
	{
		struct
		{
			union
			{
				struct
				{
					u8 CheckInterrupt;
					u8 DelaySlot_Valid;

					// if the vu0 is running while a macro mode instruction gets executed
					// then vu0 should finish first
					u8 Force_Vu0_Finish;

					u8 StoreBuffer_Valid;

					// need to know the cause of any pending interrupts
					// this one should have the bits the same as the cause register
					//u32 uPending;

				};

				u32 isSomethingBusy;
			};

			// disables the recompiler when set
			// 0: enable recompiler, 1: disable recompiler
			u32 uDisableRecompiler;
		};

		u64 Value;

	};

	
	class Cpu
	{
		static Debug::Log debugBios;

	public:
	
		static R5900::Cpu *_CPU;
		
		static const int32_t InstructionSize = 4;
		
		// processor clock speed in Hertz (full cycles)
		static const int32_t ClockSpeed = 33868800;
		
		static const u32 c_ProcessorRevisionNumber = 0x20;
		static const u32 c_ProcessorImplementationNumber = 0x2e;
		
		static const u32 c_GeneralInterruptVector = 0x80000080;
		static const u32 c_BootInterruptVector = 0xbfc00180;
		
		// interrupt vector for "Internal" R5900 interrupts
		static const u32 c_CommonInterruptVector = 0x80000180;
		static const u32 c_CommonInterruptVector_BEV = 0xbfc00380;
		
		// interrupt vector for "External" R5900 interrupts
		static const u32 c_ExternalInterruptVector = 0x80000200;
		static const u32 c_ExternalInterruptVector_BEV = 0xbfc00400;
		
		static const u32 c_ScratchPadRam_Size = 0x400;
		static const u32 c_ScratchPadRam_Mask = c_ScratchPadRam_Size - 1;
		static const u32 c_ScratchPadRam_Addr = 0x1f800000;

		// 1KB of D-Cache - not like a regular D-Cache, but more like a RAM unless you reverse I$ and D$
		//static const u32 DCache_Base = 0x1f800000;
		//static const u32 DCache_Size = 0x400;


		static constexpr const int R5900_INSTRUCTIONS_PER_CACHE_ENTRY_SHIFT = 4;
		static constexpr const int R5900_CODE_BLOCK_SIZE_SHIFT = 14;
		static constexpr const int R5900_CODE_BLOCK_COUNT_SHIFT = 14;	// 9;

		
		///////////////////////////////////
		// Required Functions For Object //
		///////////////////////////////////

		// enabled if the data on the output lines are valid
		// gets cleared by the bus when the request is accepted
		//Data::InOut::Port Output;

		// enabled if the data on the input lines are valid
		// set by the bus when the requested data is on the lines
		// cleared by the processor once the data has been accepted
		//Data::InOut::Port Input;
		
		// need a connection to the data bus
		// this is how processor communicates with the outside world
		// *** todo *** actually does not need a reference to data bus - just use shared memory
		static Playstation2::DataBus *Bus;
		
		
		
		
		/////////////////////////////
		// Constant CPU Parameters //
		/////////////////////////////
		
		static const u32 ICacheMissCycles = 5;
		
		static const u32 MultiplyCycles = 8;
		static const u32 DivideCycles = 35;
		
		static const u32 c_InstructionLoad_Cycles = 0;
		
		// the number of cycles it takes to store/load a value
		static const u32 c_CycleTime_Store = 1;
		static const u32 c_CycleTime_Load = 5;
		
		// cycle penalty for branch mis-predict (jr,jalr,eret,syscall,break,trap, and odd-addresses cannot be predicted??)
		static constexpr u64 c_ullLatency_BranchMisPredict = 2;
		
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Can only modify some parts of COP0 registers
		// AND mask with register and AND inverted mask with value to store, then OR both and store back to register
		// set the bits in mask that can't be written by software
		// bits 5-9,13-14,19-21,24-27 are always zero in Status register
		static const u32 StatusReg_WriteMask = 0x0f3863e0;
		
		//static const u32 CauseReg_WriteMask = 0xfffffcff;	// software can only modify bits 8 and 9
		
		static const u32 c_iConfigReg_StartValue = 0x440;
		static const u32 c_iStatusReg_StartValue = 0x70400004;
		
		// busy until cycles
		u64 BusyUntil_Cycle;
		u64 MulDiv_BusyUntil_Cycle;
		u64 MulDiv_BusyUntil_Cycle1;

		// when the data loaded into each gpr will be ready
		//uint64_t gprBusyUntilCycles64[32];
		
		u32 ulIdle;
		bool bEnable_SkipIdleCycles;
		
		u32 testvar [ 8 ];
		

		// used for debugging
		u32 Last_LoadStore_Address;
		
		// this will be used as the icache
		ICache_Device ICache;
		DCache_Device DCache;
		
		
		// for now add a bias until bus has better simulation (i-cache/d-cache/bus access timing/etc)
		static constexpr u64 c_ullBias = 2;
		
		
		// the number of cpu cycles to refill cache-line
		// this is actually 8 cpu cycles = 4 bus cycles, but bias on cpu cycles for the moment is 2
		static constexpr u64 c_ullCacheRefill_CycleTime = 40 / c_ullBias;
		
		
		// it looks like the r5900 can only handle one load from the bus at a time?
		// some loads are non-blocking with hit under miss support though
		// set this value when loading from bus or dcache-miss, etc
		// check this value on load from bus or dcache-miss to see if blocking is needed and until when
		u64 LoadFromBus_BusyUntilCycle;
		
		// can only have one d-cache miss at a time, or else the next miss is a blocking load/store
		// check this value on d-cache miss to see if the miss is blocking or non-blocking
		// this should be equal to the cycles it takes to load from bus (above) plus the 4 bus cycles (8 cpu cycles) to refill cache line
		// even though it loads the missed data first, it still isn't available until all the data loads into cache (same with i-cache?)
		// actually, this should be the same value as above
		//u64 DCacheMiss_BusyUntilCycle;
		
		// 128 slots in d-cache -> 8KB / 64 bytes per way = 128
		u64 RefillDCache_BusyUntilCycle [ 128 ];
		
		// time before data is available from bus for 32 general purpose registers
		u64 ullReg_BusyUntilCycle [ 32 ];
		
		
		// need to delay a certain number of cycles during cache miss before execution can continue
		u32 ICacheMiss_BusyCycles;
		u64 ICacheMiss_BusyUntil_Cycle;
		
		u32 BusyCycles;
		
		// the current instruction being executed
		Instruction::Format CurInst;
		
		// determines if the current instruction has been successfully executed yet
		bool CurInstExecuted;
		
		// if DMA sets invalidate i-cache status flag, then this is the address to invalidate
		volatile u32 ICacheInvalidate_Address;
		
		
		
		// used for testing execution unit
		// executes an instruction directly from memory and updates program counter all in same cycle
		// ignores TLB system and ignores cycle delays
		void TestExecute ( u32 NumberOfCyclesToExecute );

		// memory used for testing the operation of the CPU
		//static const u32 TestMemory_Base = 0x00000000;
		//static const u32 TestMemory_Size = 2097152;
		//u32 TestMemory [ TestMemory_Size/4 ];

		
		// sets the program counter
		// used only for testing
		void SetPC ( u32 Value );
		
		void Start ();
		
		// address that the last access exception was reading/writing to
		static u64 uExceptionRWAddr64;

		// address of the last instruction that caused an exception
		static u64 uExceptionInstrAddr64;

		// if exception info is enabled in recompiler, then this gives the PC and Cycles for the exception
		static u64 uExceptionPC64;
		static u64 uExceptionCycles64;

		// need to know if this is a float exception or an access exception
		static bool bIsFloatException;
		static bool bIsInvalidException;

		static int ExceptionFilter(LPEXCEPTION_POINTERS pEp);

		// for profiling
		static void ExecuteRecompiledCode(u32 Index);
			
		// runs the processor for one cycle - need to pass external interrupt signal - should be 1 or 0
		void Run ();

		// resets the data for the processor
		void Reset ();
		
		// dma needs to do this when loading in data to memory
		void InvalidateCache ( u32 Address );
		
		// translates virtual address in CPU into physical address for external bus
		static inline u32 GetPhysicalAddress ( u32 VirtualAddress ) { return VirtualAddress & 0x1fffffff; }

		static u32* _Debug_IntcStat;
		static u32* _Debug_IntcMask;
		// connect devices for using/debugging to the processor
		static void ConnectDebugInfo ( u32* _IntcStat, u32* _IntcMask )
		{
			_Debug_IntcStat = _IntcStat;
			_Debug_IntcMask = _IntcMask;
		}
		
		
		// processor status - I'll use this to make it run faster
		ProcStatus Status;
		

		//////////////////////////////////
		// Functions Specific To Object //
		//////////////////////////////////

		bool debug_enabled;


		

		// bits are set according to which interrupts are pending
		u32 ExternalInterruptsPending;
		
		// this is the reason for interrupt/exception
		u32 ExceptionCode;
		enum { EXC_INT,	// interrupt
				EXC_MOD,	// tlb modification
				EXC_TLBL,	// tlb load
				EXC_TLBS,	// tlb store
				
				// the address errors occur when trying to read outside of kuseg in user mode and when address is misaligned
				EXC_ADEL,	// address error - load/I-fetch
				EXC_ADES,	// address error - store
				
				EXC_IBE,	// bus error on instruction fetch
				EXC_DBE,	// bus error on data load
				EXC_SYSCALL,	// generated unconditionally by syscall instruction
				EXC_BP,			// breakpoint - break instruction
				EXC_RI,			// reserved instruction
				EXC_CPU,		// coprocessor unusable
				EXC_OV,			// arithemetic overflow
				EXC_TRAP,		// placeholder for R5900
				EXC_Unknown };	// will use this to see when exception was created by software
				
			// interrupt mask register $1f801074
			// bit 3 - counter 3 - vsync or vblank?
			// bit 4 - counter 0 - system clock
			// bit 5 - counter 1 - horizontal retrace
			// bit 6 - counter 2 - pixel
				

		
		alignas(64) Reg128 GPR [ 32 ];
		
		// need a bitmap to determine which GPR registers are loading from memory so we know if to stall pipeline or not
		u32 GPRLoading_Bitmap;

		//Reg32 Hi, Lo;
		//Reg64 HiLo;
		alignas(64) Reg128 HI, LO;

//		u32 CPR0 [ 32 ];	// COP0 control registers
		alignas(64) CPR0_Regs CPR0;		// COP0 control registers
		alignas(64) int32_t CPC0 [ 32 ];
		
		// CPCOND0 is used for bc0 instructions (to see if dma complete condition is met)
		u32 CPCOND0;
		
		// COP1 registers (floating-point coprocessor)
		
		// accumulator
		// for now, make just float
		//FloatReg ACC;
		//DoubleLong dACC;
		alignas(64) FloatLong dACC;
		
		alignas(64) FloatReg CPR1 [ 32 ];
		alignas(64) int32_t CPC1 [ 32 ];
		
		u32 divide_flag, invalid_zero_flag, invalid_negative_flag;
		u32 divide_stickyflag, invalid_zero_stickyflag, invalid_negative_stickyflag;
		
		// this value actually goes for fpu ctr registers 0-15 according to ps2autotests and is 0x2e30
		//static const u32 c_lFCR0 = 0x00002e00;
		static const u32 c_lFCR0 = 0x00002e30;
		
		// for FCR31
		// bit 3 - Sticky Underflow flag
		// bit 4 - Sticky Overflow flag
		// bit 5 - Sticky Divide flag
		// bit 6 - Sticky Invalid flag (0/0, square root of negative number, or reciprocal square root of negative number)
		// bit 14 - Underflow flag
		// bit 15 - Overflow flag
		// bit 16 - Divide flag
		// bit 17 - Invalid flag
		// bit 23 - Condition bit (gets set when result of a floating point compare operation is true, gets cleared when it is false)
		
		// set bits 0,24 in FCR31
		static const u32 c_lFCR31_SetMask = 0x01000001;
		
		// clear bits 1-2,7-13,18-22,25-31 in FCR31
		static const u32 c_lFCR31_ClearMask = 0xfe7c3f86;
		
		// the set bits in this mask determine what bits can be written
		static const u32 c_lFCR31_Mask = 0x0083c078;


		// COP2 //
		
		// last 16 registers are control registers
		// #16 - Status flag
		// #17 - MAC flag
		// read only
		// #18 - clipping flag
		// #19 - reserved
		// #20 - R register
		// #21 - I register
		// #22 - Q register
		// #23-#25 - reserved
		// #26 - TPC register
		// read only except while running
		// #27 - CMSAR0 register
		// #28 - FBRST register
		// #29 - VPU-STAT register
		// read only
		// #30 - reserved
		// #31 - CMSAR1 register
		// while VU1 is stopped, writing to here starts it at address that is written
		
		// COP2 registers (for now)
		//Reg128 CPR2 [ 32 ];
		//int32_t CPC2 [ 32 ];
		
		//COP2_Device COP2;
		
		// shift amount register
		alignas(64) u32 SA;

		// the program counter
		u32 PC;
		u32 NextPC;
		
		// need the address of the last instruction executed to handle asyncronous interrupts with branches in branch delay slots
		u32 LastPC;

		// will count cycles for troubleshooting to see what order things happen in
		u64 CycleCount;

//		volatile int32_t CycleCount;
//		int32_t CycleTarget;	// this is the target number of cycles for the processor to execute

		// the recompiler object(s) //
		
		// determines whether recompiler is enabled or not
		u32 bEnableRecompiler;

		// this one is for single stepping
		static R5900::Recompiler* rs;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// These are the ones being used to control branch delay slots and load delay slots and crazy stuff //
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// callback functions //
		typedef void (*cbFunction) ( void );
		typedef void (*cbFunctionR) (u32,u64);

		
		// co-processor delay slots
		static void _cb_FC ();
		static void _cb_MTC0 ();
		static void _cb_MTC2 ();
		static void _cb_CTC2 ();
		
		
		struct DelaySlot
		{
			union
			{
				struct
				{
					Instruction::Format Instruction;
					u32 Data;
					cbFunction cb;
				};
				
				struct
				{
					u64 Value;
					cbFunction Value2;
				};
			};
			
			//Execute::Callback_Function cb;
		};
		
		DelaySlot DelaySlot0;
		DelaySlot DelaySlot1;

		cbFunctionR DelaySlot0_Recompiler;
		u32 DelaySlot0_TargetAddr;

		// the number of cycles needed if the branch is taken
		u32 DelaySlot0_BranchTakenCycles;

		u32 NextDelaySlotIndex;
		DelaySlot DelaySlots [ 2 ];
		
		
		
		// invalid counts are negative
		s32 LoadStoreCount;
		
		// the next index to load/store from bus
		s32 NextIndex;
		
		
		////////////////////////////////////////////
		// These can be used for debugging
		u32 Last_ReadAddress;
		u32 Last_WriteAddress;
		u32 Last_ReadWriteAddress;
		
		// looks like I'll need this
		u32 LastModifiedRegister;
		
		
		
		/////////////////////
		// Pipeline Stalls //
		/////////////////////
		
		
		//bool isMultiplyDivideBusy;
		u32 MultiplyDivide_BusyCycles;
		//u32 MultiplyDivideBusy_NumCycles;
		//Instruction::Format MultiplyDivideBusy_Inst;
		
		//bool isCOP2Busy;
		//u32 COP2Busy_Cycles;
		//Instruction::Format COP2Busy_Inst;
		
		
		static u32 TraceValue;
		
		
		volatile int32_t Stop;
		
		// constructor - cpu needs a data bus to operate firstly so it can even read instructions!!
		Cpu ();
		
		// destructor
		~Cpu ( void );
		
		static u32 Read_MFC0 ( u32 Register );
		static void Write_MTC0 ( u32 Register, u32 Value );

		u32 Read_CFC1 ( u32 Register );
		void Write_CTC1 ( u32 Register, u32 Value );
				
		void FlushStoreBuffer ();

		static u64* _SpuCycleCount;
		
		void ConnectDevices ( Playstation2::DataBus* db );
		
		
		////////////////
		// Interrupts //
		////////////////
		

		inline void UpdateInterrupt ()
		{
#ifdef INLINE_DEBUG_UPDATE_INT
	debug << "\r\nUpdateInterrupt;(before) _Intc_Stat=" << hex << *_Intc_Stat << " _Intc_Mask=" << *_Intc_Mask << " _R3000A_Status=" << CPR0.Regs [ 12 ] << " _R3000A_Cause=" << CPR0.Regs [ 13 ] << " _ProcStatus=" << Status.Value;
#endif

			// it looks like INTC affects bit 11 (IP0) in Cause Register (#13)
			if ( *_Intc_Stat & *_Intc_Mask ) CPR0.Regs [ 13 ] |= ( 1 << 10 ); else CPR0.Regs [ 13 ] &= ~( 1 << 10 );
			
			// the interrupt notifying bit is in the first byte of internal processor status
			if ( ( CPR0.Regs [ 13 ] & CPR0.Regs [ 12 ] & 0x8c00 ) && ( CPR0.Regs [ 12 ] & 1 ) && ( CPR0.Regs [ 12 ] & 0x10000 ) && !( CPR0.Regs [ 12 ] & 0x6 ) )
			{
				Status.Value |= ( 1 << 0 );
			}
			/*
			else
			{
				Status.Value &= ~( 1 << 0 );
			}
			*/
			
#ifdef INLINE_DEBUG_UPDATE_INT
	debug << "\r\n(after) _Intc_Stat=" << hex << *_Intc_Stat << " _Intc_Mask=" << *_Intc_Mask << " _R3000A_Status=" << CPR0.Regs [ 12 ] << " _R3000A_Cause=" << CPR0.Regs [ 13 ] << " _ProcStatus=" << Status.Value;
#endif
		}


		// checks if an address points to DCache or to memory
		//inline static bool isDCache ( u32 Address )
		//{
		//	return ( ( Address - DCache_Base ) < DCache_Size );
		//}

				
		void ProcessSynchronousInterrupt ( u32 ExceptionType );

		void Refresh ();
		cbFunction Refresh_BranchDelaySlot ( Instruction::Format i );
		void Refresh_AllBranchDelaySlots ();
		
		
		
		// ***testing***
		u32 DebugNextERET;
		

		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R5900_Status;
		
		inline void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R5900_Stat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			_R5900_Status = _R5900_Stat;
		}

		
		//////////////////////////////////////////
		// Debug Status

		union _DebugStatus
		{
			struct
			{
				u32 Stop : 1;
				u32 Step : 1;
				u32 Done : 1;
				u32 OutputCode : 1;
				u32 SaveBIOSToFile : 1;
				u32 SaveRAMToFile : 1;
				u32 SetBreakPoint : 1;
				u32 SetMemoryStart : 1;
			};
			
			u32 Value;
		};

		static volatile _DebugStatus DebugStatus;

		static const u32 CallStack_Size = 0x8;
		u32 CallStackDepth;
		u32 Debug_CallStack_Address [ 8 ];
		u32 Debug_CallStack_Function [ 8 ];
		u32 Debug_CallStack_ReturnAddress [ 8 ];
		

		void ProcessLoadDelaySlot ();
		
		static u64* _NextSystemEvent;
		
		// Enable/Disable debug window for object
		// Windows API specific code is in here
		static bool DebugWindow_Enabled;
		
#ifdef ENABLE_GUI_DEBUGGER
		static WindowClass::Window *DebugWindow;
		static DebugValueList<u32> *GPR_ValueList;
		static DebugValueList<u32> *COP0_ValueList;
		static DebugValueList<float> *COP2_CPCValueList;
		static DebugValueList<u32> *COP2_CPRValueList;
		static Debug_DisassemblyViewer *DisAsm_Window;
		static Debug_BreakpointWindow *Breakpoint_Window;
		static Debug_MemoryViewer *ScratchPad_Viewer;
		static Debug_BreakPoints *Breakpoints;
#endif

		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();
		
		//static const u32 DisAsm_Window_Rows = 0x3ffffff;
		//static string DisAsm_Window_GetText ( int row, int col );

	private:
	
		////////////////////////////////////////////////
		// Debug Output
#ifdef ENABLE_GUI_DEBUGGER
		static Debug::Log debug;
#endif
		
		
		// multi-threading stuff
		// volatile u64 Ran -> this will be defined in the system object to say what devices have ran for cycle
		volatile u32 Running;
		
	
		// gnu stuff keeps ignoring my solid code, have to try this way
		bool ProcessExternalLoad ();
		void ProcessExternalStore ();
		void ProcessAsynchronousInterrupt ();
		
		
		void ProcessBranchDelaySlot ();


public:

template<const u32 OPCODE>
static void ProcessBranchDelaySlot_t ()
{
	Instruction::Format i;
	
	//DelaySlot *d = & ( _CPU->DelaySlots [ _CPU->NextDelaySlotIndex ] );
	//i = d->Instruction;
	i = _CPU->DelaySlot0.Instruction;

	switch ( OPCODE )
	{
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
		case OPREGIMM:
			//case RTBLTZ:	// regimm
			//case RTBGEZ:	// regimm
			//case RTBLTZL:	// regimm
			//case RTBGEZL:	// regimm
			//case RTBLTZAL:	// regimm
			//case RTBGEZAL:	// regimm
			//case RTBLTZALL:	// regimm
			//case RTBGEZALL:	// regimm
			
			_CPU->NextPC = _CPU->PC + ( i.sImmediate << 2 );

#ifdef ENABLE_R5900_BRANCH_PREDICTION_REGIMM
			// if jumping to an odd address, then branch can't be predicted?
			// same issue if jumping to uncached address
			if ( ( _CPU->NextPC & 4 ) /*|| ( _CPU->NextPC & 0x60000000 )*/ )
			{
				_CPU->CycleCount += c_ullLatency_BranchMisPredict;
			}
#endif
			
			break;
			
		
		case OPSPECIAL:
			//case SPJR:
			//case SPJALR:
			//_CPU->NextPC = d->Data;
			_CPU->NextPC = _CPU->DelaySlot0.Data;

#ifdef ENABLE_R5900_BRANCH_PREDICTION_SPECIAL
			// no branch prediction for jr,jalr ??
			_CPU->CycleCount += c_ullLatency_BranchMisPredict;
#endif
			
			break;
			
	
		case OPJ:	// ok
		case OPJAL:	// ok
			_CPU->NextPC = ( 0xf0000000 & _CPU->PC ) | ( i.JumpAddress << 2 );
			
#ifdef ENABLE_R5900_BRANCH_PREDICTION_JUMP
			// if jumping to an odd address, then branch can't be predicted?
			if ( ( _CPU->NextPC & 4 ) /*|| ( _CPU->NextPC & 0x60000000 )*/ )
			{
				_CPU->CycleCount += c_ullLatency_BranchMisPredict;
			}
#endif
			
			break;
			
	}
}

template<const u32 OPCODE>
static void Recompiler_ProcessBranchDelaySlot_t(u32 PC, u64 CycleOffset)
{
	Instruction::Format i;

	// todo: could maybe have it return this later instead ?
	_CPU->CycleCount += CycleOffset;

	//DelaySlot *d = & ( _CPU->DelaySlots [ _CPU->NextDelaySlotIndex ] );
	//i = d->Instruction;
	i = _CPU->DelaySlot0.Instruction;

	switch (OPCODE)
	{
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
	case OPREGIMM:
		//case RTBLTZ:	// regimm
		//case RTBGEZ:	// regimm
		//case RTBLTZL:	// regimm
		//case RTBGEZL:	// regimm
		//case RTBLTZAL:	// regimm
		//case RTBGEZAL:	// regimm
		//case RTBLTZALL:	// regimm
		//case RTBGEZALL:	// regimm

		_CPU->NextPC = PC + (i.sImmediate << 2);

#ifdef ENABLE_R5900_BRANCH_PREDICTION_REGIMM
		// if jumping to an odd address, then branch can't be predicted?
		// same issue if jumping to uncached address
		if ((_CPU->NextPC & 4) /*|| ( _CPU->NextPC & 0x60000000 )*/)
		{
			_CPU->CycleCount += c_ullLatency_BranchMisPredict;
		}
#endif

		break;


	case OPSPECIAL:
		//case SPJR:
		//case SPJALR:
		//_CPU->NextPC = d->Data;
		_CPU->NextPC = _CPU->DelaySlot0.Data;

#ifdef ENABLE_R5900_BRANCH_PREDICTION_SPECIAL
		// no branch prediction for jr,jalr ??
		_CPU->CycleCount += c_ullLatency_BranchMisPredict;
#endif

		break;


	case OPJ:	// ok
	case OPJAL:	// ok
		_CPU->NextPC = (0xf0000000 & PC) | (i.JumpAddress << 2);

#ifdef ENABLE_R5900_BRANCH_PREDICTION_JUMP
		// if jumping to an odd address, then branch can't be predicted?
		if ((_CPU->NextPC & 4) /*|| ( _CPU->NextPC & 0x60000000 )*/)
		{
			_CPU->CycleCount += c_ullLatency_BranchMisPredict;
		}
#endif

		break;

	}
}



template<const u32 ExceptionType>
static void ProcessSynchronousInterrupt2_t(u32 PC, u64 CycleOffset)
{
	_CPU->PC = PC;
	_CPU->CycleCount += CycleOffset;

	ProcessSynchronousInterrupt_t<ExceptionType>();
}

// *** todo *** should probably be a branch mispredict penalty for a synchronous interrupt
template<const u32 ExceptionType>
static void ProcessSynchronousInterrupt_t ( void )
{
	// *** todo *** Synchronous Interrupts can also be triggered by setting bits 8 or 9 (IP0 or IP1) of Cause register

	//////////////////////////////////////////////////////////////////////////////////
	// At this point:
	// PC points to instruction currently in the process of being executed
	// LastPC points to instruction before the one in the process of being executed
	// NextPC DOES NOT point to the next instruction to execute
	
#ifdef INLINE_DEBUG_SYNC_INT
	if ( _CPU->CPR0.Cause.ExcCode == EXC_SYSCALL )
	{
	debug << "\r\nSynchronous Interrupt PC=" << hex << _CPU->PC << " NextPC=" << _CPU->NextPC << " LastPC=" << _CPU->LastPC << " Status=" << _CPU->CPR0.Regs [ 12 ] << " Cause=" << _CPU->CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( _CPU->DelaySlot1.Instruction.Value ).c_str () << "  " << hex << _CPU->DelaySlot1.Instruction.Value;
	}
#endif


	// step 3: shift left first 8 bits of status register by 2 and clear top 2 bits of byte
	//CPR0.Status.b0 = ( CPR0.Status.b0 << 2 ) & 0x3f;
	
	// step 4: set to kernel mode with interrupts disabled
	//CPR0.Status.IEc = 0;
	//CPR0.Status.KUc = 1;
	
	// R5900 step 1: switch to exception level 1/kernel mode (set Status.EXL=1)
	_CPU->CPR0.Status.EXL = 1;
	
	// step 5: Store current address (has not been executed yet) into COP0 register "EPC" unless in branch delay slot
	// if in branch delay slot, then set field "BD" in Cause register and store address of previous instruction in COP0 register "EPC"
	// Branch Delay Slot 1 is correct since instruction is still in the process of being executed
	// probably only branches have delay slot on R5900
	//if ( _CPU->DelaySlots [ _CPU->NextDelaySlotIndex ].Value )
	if (_CPU->Status.DelaySlot_Valid)
	{
		// we are in branch delay slot - instruction in branch delay slot has not been executed yet, since it was "interrupted"
		_CPU->CPR0.Cause.BD = 1;

		// this is actually not the previous instruction, but it is the previous instruction executed
		// this allows for branches in branch delay slots
		//_CPU->CPR0.EPC = _CPU->LastPC;
		_CPU->CPR0.EPC = _CPU->PC - 4;

		// no int32_ter want to execute the branch that is in branch delay slot
		//_CPU->DelaySlots[_CPU->NextDelaySlotIndex].Value = 0;
		_CPU->DelaySlot0.Value = 0;

		_CPU->Status.DelaySlot_Valid = 0;

		// ***testing*** may need to preserve the interrupts
		//Status.DelaySlot_Valid &= 0xfc;
	}
	else
	{
		// we are not in branch delay slot
		_CPU->CPR0.Cause.BD = 0;
		
		// this is synchronous interrupt, so EPC gets set to the instruction that caused the interrupt
		_CPU->CPR0.EPC = _CPU->PC;
	}

	// step 6: Set correct interrupt pending bit in "IP" (Interrupt Pending) field of Cause register
	// actually, we need to send an interrupt acknowledge signal back to the interrupting device probably
	
	// step 7: set PC to interrupt vector address
	if ( _CPU->CPR0.Status.BEV == 0 )
	{
		// check if tlb miss exception - skip for R3000A for now
		
		// set PC with interrupt vector address
		// kseg1 - 0xa000 0000 - 0xbfff ffff : translated to physical address by removing top three bits
		// this region is not cached
		//NextPC = c_GeneralInterruptVector;
		_CPU->NextPC = c_CommonInterruptVector;
	}
	else
	{
		// check if tlb miss exception - skip for R3000A for now
		
		// set PC with interrupt vector address
		// kseg0 - 0x8000 0000 - 0x9fff ffff : translated to physical address by removing top bit
		// this region is cached
		//NextPC = c_BootInterruptVector;
		_CPU->NextPC = c_CommonInterruptVector_BEV;
	}
	
	// step 8: set Cause register so that software can see the reason for the exception
	// we know that this is an synchronous interrupt
	//CPR0.Cause.ExcCode = Status.ExceptionType;
	_CPU->CPR0.Cause.ExcCode = ExceptionType;
	
	// *** todo *** instruction needs to be passed above because lower 2 bits of opcode go into Cause Reg bits 28 and 29


	// step 9: continue execution
	
	// when returning from interrupt you should
	// Step 1: enable interrupts globally
	// Step 2: shift right first 8 bits of status register
	// Step 3: set ExceptionCode to EXC_Unknown
		

#ifdef INLINE_DEBUG_SYNC_INT
	debug << "\r\n->PC=" << hex << _CPU->PC << " NextPC=" << _CPU->NextPC << " LastPC=" << _CPU->LastPC << " Status=" << _CPU->CPR0.Regs [ 12 ] << " Cause=" << _CPU->CPR0.Regs [ 13 ];
	debug << "\r\nBranch Delay Instruction: " << R5900::Instruction::Print::PrintInstruction ( _CPU->DelaySlot1.Instruction.Value ).c_str () << "  " << hex << _CPU->DelaySlot1.Instruction.Value;
#endif
}

		

	};

}

#endif	// end #ifndef __R5900_H__

