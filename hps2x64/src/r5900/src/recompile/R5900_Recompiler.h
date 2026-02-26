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
#ifndef __R5900_RECOMPILER_H__
#define __R5900_RECOMPILER_H__


#include "R5900.h"
#include "R5900_Lookup.h"
#include "x64Encoder.h"

#include "x64_assembler.h"


namespace R5900
{
	
	class Cpu;
	
	// will probably create two recompilers for each processor. one for single stepping and one for multi-stepping
	class Recompiler
	{
	public:
		static Debug::Log debug;
		
		// number of code cache slots total
		// but the number of blocks should be variable
		//static const int c_iNumBlocks = 1024;
		//static const u32 c_iNumBlocks_Mask = c_iNumBlocks - 1;
		u32 NumBlocks;
		static u32 NumBlocks_Mask;
		//static const int c_iBlockSize_Shift = 4;
		//static const int c_iBlockSize_Mask = ( 1 << c_iBlockSize_Shift ) - 1;
		
		static const u32 c_iAddress_Mask = 0x1fffffff;
		
		u32 TotalCodeCache_Size;	// = c_iNumBlocks * ( 1 << c_iBlockSize_Shift );
		
		u32 BlockSize;
		u32 BlockSize_Shift;
		u32 BlockSize_Mask;
		
		// maximum number of instructions that can be encoded/recompiled in a run
		static u32 MaxStep;
		static u32 MaxStep_Shift;
		static u32 MaxStep_Mask;
		
		// zero if not in delay slot, otherwise has the instruction
		//static u32 Local_DelaySlot;
		//static u32 Local_DelayType;
		//static u32 Local_DelayCount;
		//static u32 Local_DelayCond;
		
		// 0 means unconitional branch, 1 means check condition before branching
		//static u32 Local_Condition;
		
		
		// the amount of stack space required for SEH
		static constexpr const int32_t c_lSEH_StackSize = 40;

		static constexpr uint64_t EXECUTE_CYCLES = 1;
		
		// instruction set to use
		enum { VECTOR_TYPE_SSE, VECTOR_TYPE_AVX2, VECTOR_TYPE_AVX512 };
		static int iVectorType;

		// how many instructions max it can process at a time
		enum { RECOMPILER_WIDTH_SSEX1, RECOMPILER_WIDTH_AVX2X1, RECOMPILER_WIDTH_AVX2X2, RECOMPILER_WIDTH_AVX512X1, RECOMPILER_WIDTH_AVX512X2, RECOMPILER_WIDTH_AVX512X4 };
		static int iRecompilerWidth;

		inline static void SetVectorType(int ivt) { iVectorType = ivt; }
		inline static void SetRecompilerWidth(int irw) { iRecompilerWidth = irw; }

		union RDelaySlot
		{
			struct
			{
				// instruction in delay slot
				u32 Code;
				
				// type of instruction with delay slot
				// 0 - branch (BEQ,BNE,etc), 1 - jump (J,JAL)
				u16 Type;
				
				// branch condition
				// 0 - unconditional, 1 - conditional
				u16 Condition;
			};
			
			u64 Value;
		};
		
		static RDelaySlot RDelaySlots [ 2 ];
		static u32 DSIndex;
		
		static u32 RDelaySlots_Valid;
		
		inline void ClearDelaySlots ( void ) { RDelaySlots [ 0 ].Value = 0; RDelaySlots [ 1 ].Value = 0; }
		
		// need something to use to keep track of dynamic delay slot stuff
		u32 RecompilerDelaySlot_Flags;
		
		
		// the next instruction in the cache block if there is one
		static R5900::Instruction::Format NextInst;
		static R5900::Instruction::Format PrevInst;

		// cycles to load instruction from memory/cache
		static u64 MemCycles;
		static u64 ullLoadCycles;

		// cycles to start execution of instruction (should be 1)
		static const u64 ExeCycles = 1;

		static u64 LocalCycleCount;
		static u64 LocalCycleCount2;
		static u64 CacheBlock_CycleCount;
		
		// also need to know if the addresses in the block are cached or not
		static bool bIsBlockInICache;

		
		// the PC while recompiling
		static u32 LocalPC;
		
		// the optimization level
		// 0 means no optimization at all, anything higher means to optimize
		static s32 OpLevel;
		u32 OptimizeLevel;
		
		static constexpr u32 xyzw_rev_mask_lut32[16] = {
			0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
			0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
		};
		static constexpr u32 xyzw_norm_mask_lut32[16] = {
			0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
			0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
		};

		// the current enabled encoder
		static x64Encoder *e;

		static x64asm::x64Assembler* x;

		//static ICache_Device *ICache;
		static R5900::Cpu *r;
		
		// the encoder for this particular instance
		x64Encoder *InstanceEncoder;
		
		// the maximum number of cache blocks it can encode across
		s32 MaxCacheBlocks;

		static u32 RunCount;
		static u32 RunCount2;

		// a bitmap that shows the usable registers when allocating registers in level 2 compiler
		// set bits need to start from position 0 and need to be contiguous. 0x1f is ok but not 0x12 for example
		static const u32 c_ulUsableRegsBitmap = 0x1fff;	//0x1fff;
		
		// keep of what registers might be in load/store pipeline
		static u64 ullLSRegs;
		
		// multi-pass optimization vars //
		static u64 ullSrcRegBitmap;
		static u64 ullDstRegBitmap;
		
		// the x64 registers that are currently allocated to MIPS registers
		static u64 ullTargetAlloc;
		
		// the MIPS registers that are currently allocated to x64 registers
		static u64 ullSrcRegAlloc;
		
		// the MIPS registers that are currently allocated to be constants
		static u64 ullSrcConstAlloc;
		
		// the MIPS registers that have been modified and not written back yet
		static u64 ullSrcRegsModified;
		
		// registers that are on the stack and need to be restored when done
		static u64 ullRegsOnStack;
		
		// registers that are needed later on in the code
		static u64 ullNeededLater;
		
		static u64 ullSrcRegBitmaps [ 16 ];
		static u64 ullDstRegBitmaps [ 16 ];
		static u64 ullRegsStillNeeded [ 16 ];
		
		// the actual data stored for the register, either a reg index or a constant value
		static u64 ullTargetData [ 32 ];
		
		// lookup that indicates what register each target index corresponds to
		static const int iRegPriority [ 13 ];
		
		// lookup that indicates if the register requires you to save it on the stack before using it
		// 0: no need to save on stack, 1: save and restore on stack
		static const int iRegStackSave [ 13 ];

		// data on which float components to process
		static u32 xcomp;

		// the history of float instructions
		static u32 FInstHist[4];

		// pre-analysis of run
		static u32 LUT_StatInfo[16];

		static constexpr const u32 STATIC_PARALLEL_EXECx2 = (1 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXECx3 = (2 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXECx4 = (3 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXEC_MASK = (3 << 0);

		static constexpr const u32 STATIC_PARALLEL_TYPE_FLOAT = (1 << 2);
		static constexpr const u32 STATIC_PARALLEL_TYPE_MACRO = (2 << 2);
		static constexpr const u32 STATIC_PARALLEL_TYPE_MASK = (3 << 2);

		static constexpr const u32 STATIC_PARALLEL_OP_ADD = (1 << 4);
		static constexpr const u32 STATIC_PARALLEL_OP_MUL = (2 << 4);
		static constexpr const u32 STATIC_PARALLEL_OP_MADD = (3 << 4);
		static constexpr const u32 STATIC_PARALLEL_OP_MASK = (3 << 4);

		static constexpr const u32 STATIC_PARALLEL_IDX0 = (0 << 6);
		static constexpr const u32 STATIC_PARALLEL_IDX1 = (1 << 6);
		static constexpr const u32 STATIC_PARALLEL_IDX2 = (2 << 6);
		static constexpr const u32 STATIC_PARALLEL_IDX3 = (3 << 6);
		static constexpr const u32 STATIC_PARALLEL_IDX_MASK = (3 << 6);
		static constexpr const u32 STATIC_PARALLEL_IDX_SHIFT = (6);

		static constexpr const u32 STATIC_PARALLEL_BRANCH = (1 << 8);
		static constexpr const u32 STATIC_PARALLEL_BDELAY = (2 << 8);
		static constexpr const u32 STATIC_PARALLEL_BMASK = (3 << 8);

		static constexpr const u32 STATIC_PARALLEL_FLOAT_FLAG_CHECK = (1 << 10);
		static constexpr const u32 STATIC_PARALLEL_MACRO_FLAG_CHECK = (2 << 10);
		static constexpr const u32 STATIC_PARALLEL_FLAG_CHECK_MASK = (3 << 10);

		static constexpr const u32 STATIC_CONFLICT_RAWx2 = (1 << 14);
		static constexpr const u32 STATIC_CONFLICT_RAWx3 = (2 << 14);
		static constexpr const u32 STATIC_CONFLICT_RAWx4 = (4 << 14);
		static constexpr const u32 STATIC_CONFLICT_RAW_X_MASK = (7 << 14);

		
		// returns false on error, returns true otherwise
		//static bool Remove_SrcReg ( int iSrcRegIdx );

		static void Exit_Recompiler(u32 PC, s64 CycleOffset);


		static bool isCOP1(R5900::Instruction::Format i);

		static bool isFloat_ADD(R5900::Instruction::Format i);
		static bool isFloat_ADDA(R5900::Instruction::Format i);
		static bool isFloat_SUB(R5900::Instruction::Format i);
		static bool isFloat_SUBA(R5900::Instruction::Format i);

		static bool isFloat_MUL(R5900::Instruction::Format i);
		static bool isFloat_MULA(R5900::Instruction::Format i);

		static bool isFloat_MADD(R5900::Instruction::Format i);
		static bool isFloat_MADDA(R5900::Instruction::Format i);
		static bool isFloat_MSUB(R5900::Instruction::Format i);
		static bool isFloat_MSUBA(R5900::Instruction::Format i);

		static bool isAlloc ( int iSrcRegIdx );
		static bool isLarge ( u64 ullValue );
		static bool isConst ( int iSrcRegIdx );
		static bool isReg ( int iSrcRegIdx );
		static bool isDisposable( int iSrcRegIdx );
		
		static bool isBothAlloc ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isBothConst ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isBothReg ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isBothDisposable( int iSrcRegIdx1, int iSrcRegIdx2 );

		static bool isEitherAlloc ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isEitherConst ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isEitherReg ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static bool isEitherDisposable( int iSrcRegIdx1, int iSrcRegIdx2 );

		static int SelectAlloc ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static int SelectConst ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static int SelectReg ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static int SelectDisposable( int iSrcRegIdx1, int iSrcRegIdx2 );

		static int SelectNotAlloc ( int iSrcRegIdx1, int iSrcRegIdx2 );
		static int SelectNotDisposable( int iSrcRegIdx1, int iSrcRegIdx2 );
		
		static inline u64 GetConst ( int iSrcRegIdx ) { return ullTargetData [ iSrcRegIdx ]; }
		
		// gets the register on the target device using the register on the source device
		static inline int GetReg ( int iSrcRegIdx ) { return iRegPriority [ ullTargetData [ iSrcRegIdx ] ]; }
		
		// returns -1 if error, otherwise returns index of register on target platform
		// iSrcRegIdx: index of register on source platform (for example, MIPS)
		static int Alloc_SrcReg ( int iSrcRegIdx );
		static int Alloc_DstReg ( int iSrcRegIdx );

		// returns -1 if error, otherwise returns index of register on target platform
		// iSrcRegIdx: index of register on source platform (for example, MIPS)
		static int Alloc_Const ( int iSrcRegIdx, u64 ullValue );
		
		static int DisposeReg ( int iSrcRegIdx );
		static int RenameReg ( int iNewSrcRegIdx, int iOldSrcRegIdx );
		static void WriteBackModifiedRegs ();
		static void RestoreRegsFromStack ();
		
		
		static bool Check_StaticDependencyOk ( R5900::Instruction::Format i );
		static uint64_t GetGPR_SrcRegs ( R5900::Instruction::Format i );
		static uint64_t GetCop1_SrcRegs ( R5900::Instruction::Format i0 );

		
		inline void Set_MaxCacheBlocks ( s32 Value ) { MaxCacheBlocks = Value; }
		
		// constructor
		// block size must be power of two, multiplier shift value
		// so for BlockSize_PowerOfTwo, for a block size of 4 pass 2, block size of 8 pass 3, etc
		// for MaxStep, use 0 for single stepping, 1 for stepping until end of 1 cache block, 2 for stepping until end of 2 cache blocks, etc
		// no, for MaxStep, it should be the maximum number of instructions to step
		// NumberOfBlocks MUST be a power of 2, where 1 means 2, 2 means 4, etc
		Recompiler ( R5900::Cpu* R5900Cpu, u32 NumberOfBlocks, u32 BlockSize_PowerOfTwo, u32 MaxIStep );
		
		// destructor
		~Recompiler ();
		
		
		void Reset ();	// { memset ( this, 0, sizeof( Recompiler ) ); }

		
		static bool isNop(R5900::Instruction::Format i);
		static bool isBranch(R5900::Instruction::Format i);
		static bool isBranchLikely(R5900::Instruction::Format i);
		static bool isJump(R5900::Instruction::Format i);
		static bool isBranchOrJump(R5900::Instruction::Format i);

		static bool isBranchDelayOk ( u32 ulInstruction, u32 Address );

		
		static u32* RGetPointer ( u32 Address );
		
		
		// set the optimization level
		inline void SetOptimizationLevel ( u32 Level ) { OptimizeLevel = Level; }
		
		// accessors
		//inline u32 Get_DoNotCache ( u32 Address ) { return DoNotCache [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		//inline u32 Get_CacheMissCount ( u32 Address ) { return CacheMissCount [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		//inline u32 Get_StartAddress ( u32 Address ) { return StartAddress [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		//inline u32 Get_LastAddress ( u32 Address ) { return LastAddress [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		//inline u32 Get_RunCount ( u32 Address ) { return RunCount [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		//inline u32 Get_MaxCycles ( u32 Address ) { return MaxCycles [ ( Address >> 2 ) & NumBlocks_Mask ]; }
		
		// returns a pointer to the instructions cached starting at Address
		// this function assumes that this instance of recompiler only has one instruction per run
		//inline u32* Get_InstructionsPtr ( u32 Address ) { return & ( Instructions [ ( ( Address >> 2 ) & NumBlocks_Mask ) << MaxStep_Shift ] ); }
		
		// returns a pointer to the start addresses cached starting at Address
		// this function assumes that this instance of recompiler only has one instruction per run
		//inline u32* Get_AddressPtr ( u32 Address ) { return & ( StartAddress [ ( ( Address >> 2 ) & NumBlocks_Mask ) ] ); }
		
		// check that Address is not -1
		// returns 1 if address is ok, returns 0 if it is not ok
		//inline bool isBlockValid ( u32 Address ) { return ( StartAddress [ ( Address >> 2 ) & NumBlocks_Mask ] != 0xffffffff ); }
		
		// check that Address matches StartAddress for the block
		// returns 1 if address is ok, returns 0 if it is not ok
		//inline bool isStartAddressMatch ( u32 Address ) { return ( Address == StartAddress [ ( Address >> 2 ) & NumBlocks_Mask ] ); }
		
		// this function assumes that this instance of recompiler only has one instruction per run
		// returns 1 if the instruction is cached for address, 0 otherwise
		//inline bool isCached ( u32 Address, u32 Instruction ) { return ( Address == StartAddress [ ( Address >> 2 ) & NumBlocks_Mask ] ) && ( Instruction == Instructions [ ( Address >> 2 ) & NumBlocks_Mask ] ); }
		
		inline bool isRecompiled ( u32 Address ) { return ( StartAddress [ ( Address >> ( 2 + MaxStep_Shift ) ) & NumBlocks_Mask ] == ( Address & ~( ( 1 << ( 2 + MaxStep_Shift ) ) - 1 ) ) ); }
		
		
		inline u64 Execute ( u32 Address ) { return InstanceEncoder->ExecuteCodeBlock ( ( Address >> 2 ) & NumBlocks_Mask ); }
		
		
		// initializes a code block that is not being used yet
		u32 InitBlock ( u32 Block );
		
		
		// get a bitmap for the source registers used by the specified instruction
		static u64 GetSourceRegs ( R5900::Instruction::Format i, u32 Address );
		static u64 Get_DelaySlot_DestRegs ( R5900::Instruction::Format i );
		
		static int32_t Generate_Cached_Store(R5900::Instruction::Format i, u32 Address, u32 BitTest, void* StoreFunctionToCall);
		static int32_t Generate_Virtual_Store(R5900::Instruction::Format i, u32 Address, u32 BitTest, void* StoreFunctionToCall);
		static int32_t Generate_Normal_Store ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* StoreFunctionToCall );
		static int32_t Generate_Normal_Store_L2 ( Instruction::Format i, u32 Address, u32 BitTest, u32 BaseAddress );
		static int32_t Generate_Cached_Load(R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall);
		static int32_t Generate_Virtual_Load(R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall);
		static int32_t Generate_Basic_Load(R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall);
		static int32_t Generate_Normal_Load ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall );
		static int32_t Generate_Normal_Load_L2 ( Instruction::Format i, u32 Address, u32 BitTest, u32 BaseAddress );
		static int32_t Generate_Normal_Branch(R5900::Instruction::Format i, u32 Address, void* BranchFunctionToCall);
		static int32_t Generate_Normal_Trap ( R5900::Instruction::Format i, u32 Address );

		// combined load/store
		static int Get_CombinedLoadCount ( R5900::Instruction::Format i, int32_t Address, R5900::Instruction::Format* pInstructionList );
		static int32_t Get_CombinedLoadMaxWidthMask ( int iLoadCount, R5900::Instruction::Format* pLoadList );
		static int32_t Generate_Combined_Load ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall, int iLoadCount, R5900::Instruction::Format* pLoadList );

		static int Get_CombinedStoreCount ( R5900::Instruction::Format i, int32_t Address, R5900::Instruction::Format* pInstructionList );
		static int32_t Get_CombinedStoreMaxWidthMask ( int iLoadCount, R5900::Instruction::Format* pLoadList );
		static int32_t Generate_Combined_Store ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* StoreFunctionToCall, int iLoadCount, R5900::Instruction::Format* pLoadList );

		static int32_t Dispatch_Result_AVX2(R5900::Instruction::Format i, int32_t Accum, int32_t sseSrcReg, int32_t VUDestReg = 0);

		static int32_t Generate_VABSp ( R5900::Instruction::Format i );
		static int32_t Generate_VMAXp ( R5900::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VMINp ( R5900::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VFTOIXp ( R5900::Instruction::Format i, u32 IX );
		static int32_t Generate_VITOFXp ( R5900::Instruction::Format i, u64 FX );
		static int32_t Generate_VMOVEp ( R5900::Instruction::Format i );
		static int32_t Generate_VMR32p ( R5900::Instruction::Format i );
		static int32_t Generate_VMFIRp ( R5900::Instruction::Format i );
		static int32_t Generate_VMTIRp ( R5900::Instruction::Format i );
		static int32_t Generate_VADDp ( u32 bSub, R5900::Instruction::Format i, u32 FtComponent = 4, void *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMULp(R5900::Instruction::Format i, u32 FtComponentp = 0x1b, void* pFd = NULL, u32* pFt = NULL, u32 FsComponentp = 0x1b);
		static int32_t Generate_VMADDp ( u32 bSub, R5900::Instruction::Format i, u32 FtComponentp = 0x1b, void *pFd = NULL, u32 *pFt = NULL, u32 FsComponentp = 0x1b );
		
		
		static int32_t Generate_VMAX ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFt = NULL );
		static int32_t Generate_VMIN ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFt = NULL );
		static int32_t Generate_VFTOI0 ( R5900::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent );
		static int32_t Generate_VFTOIX ( R5900::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent, u32 IX );
		static int32_t Generate_VITOFX ( R5900::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent, u64 FX );
		static int32_t Generate_VMOVE ( R5900::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent );
		
		static int32_t Generate_VMR32_Load ( R5900::Instruction::Format i, u32 Address, u32 FsComponent );
		static int32_t Generate_VMR32_Store ( R5900::Instruction::Format i, u32 Address, u32 FtComponent );
		
		static int32_t Generate_VMFIR ( R5900::Instruction::Format i, u32 Address, u32 FtComponent );
		static int32_t Generate_VADD ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VSUB ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMUL ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMADD ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMSUB ( R5900::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		
		
		static int32_t Generate_VPrefix ( u32 Address );
		
		static int32_t Generate_FARGSp(x64Encoder* e, R5900::Instruction::Format i, int iIndexX);

		static int32_t Generate_FADDp(x64Encoder* e, u32 i0, u32 i1 = 0, u32 i2 = 0, u32 i3 = 0);
		static int32_t Generate_FMULp(x64Encoder* e, u32 i0, u32 i1 = 0, u32 i2 = 0, u32 i3 = 0);
		static int32_t Generate_FMADDp(x64Encoder* e, u32 i0, u32 i1 = 0, u32 i2 = 0, u32 i3 = 0);

		static int32_t Generate_FMINp(x64Encoder* e, u32 i0);
		static int32_t Generate_FMAXp(x64Encoder* e, u32 i0);
		static int32_t Generate_FFTOIp(x64Encoder* e, u32 i0);
		static int32_t Generate_FITOFp(x64Encoder* e, u32 i0);


		// says if block points to an R3000A instruction that should NOT be EVER cached
		// *note* this should be dynamically allocated
		//u8* DoNotCache;	// [ c_iNumBlocks ];
		
		// number of times a cache miss was encountered while recompiling for this block
		// *note* this should be dynamically allocated
		//u32* CacheMissCount;	// [ c_iNumBlocks ];
		
		// code cache block not invalidated
		// *note* this should be dynamically allocated
		//u8 isValid [ c_iNumBlocks ];
		
		// start address for instruction encodings (inclusive)
		// *note* this should be dynamically allocated
		static u32* StartAddress;	// [ c_iNumBlocks ];
		static u32* LastOffset;
		
		static u32* pSourceCode32;


		// hardware read/write bitmap
		// if the instruction might do a hardware reg read/write then it gets flagged in bitmap
		static u64* pHWRWBitmap64;
		static void set_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Block(Address)] = -1ull; }
		static void clear_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Block(Address)] = 0ull; }
		static u64 get_hwrw_bitmap(u32 Address) { return pHWRWBitmap64[Get_Block(Address)]; }
		static void bitset_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Block(Address)] |= (1ull << (Get_Index(Address) & 0x3f)); }
		static bool bitget_hwrw_bitmap(u32 Address) { return ((pHWRWBitmap64[Get_Block(Address)] & (1ull << (Get_Index(Address) & 0x3f))) != 0); }

		// vu0 running bitmap
		// if the instruction is a vu0 macro mode executed while vu0 is running then it gets flagged in bitmap
		static u16* pMacroBitmap16;
		static void set_macro_bitmap(u32 Address) { pMacroBitmap16[Get_Block(Address)] = -1ull; }
		static void clear_macro_bitmap(u32 Address) { pMacroBitmap16[Get_Block(Address)] = 0ull; }
		static u16 get_macro_bitmap(u32 Address) { return pMacroBitmap16[Get_Block(Address)]; }
		static void bitset_macro_bitmap(u32 Address) { pMacroBitmap16[Get_Block(Address)] |= (1ull << (Get_Index(Address) & 0xf)); }
		static bool bitget_macro_bitmap(u32 Address) { return ((pMacroBitmap16[Get_Block(Address)] & (1ull << (Get_Index(Address) & 0xf))) != 0); }

		// also need to know where the instruction is that caused the hw reg read/write exception
		//static u64 uHWExceptionAddr64;
		//static void set_hw_exception_addr(u64 hw_exception_addr) { uHWExceptionAddr64 = hw_exception_addr; }
		//static void clear_hw_exception_addr() { uHWExceptionAddr64 = -1ull; }

		// last address that instruction encoding is valid for (inclusive)
		// *note* this should be dynamically allocated
		// It actually makes things MUCH simpler if you store the number of instructions recompiled instead of the last address
		//u32* LastAddress;	// [ c_iNumBlocks ];
		//u8* RunCount;
		
		//u32* Instructions;
		
		// pointer to where the prefix for the instruction starts at (used only internally by recompiler)
		static u8** pPrefix_CodeStart;
		
		// where the actual instruction starts at in code block
		static u8** pCodeStart;
		
		// the number of offset cycles from this instruction in the code block
		static u32* CycleCount;
		inline static u32 get_cycle_count(u32 Address) { return CycleCount[Get_Index(Address)]; }

#ifdef ENABLE_R5900_CHECKSUM
		// 64-bit checksum of the source - final check to determine if recompile is really needed
		static u64* pChecksum64;
#endif
		
		u64 Calc_Checksum( u32 StartAddress );


		
		// list of branch targets when jumping forwards in code block
		static u32* pForwardBranchTargets;
		static const int c_ulForwardBranchIndex_Start = 100;
		static u32 ForwardBranchIndex;

		static void FJMP (int32_t AddressFrom, int32_t AddressTo );
		static void Set_FJMPs (int32_t AddressTo );



		static u32 StartBlockIndex;
		static u32 BlockIndex;
		
		// not needed
		//u32* EndAddress;
		
		
		int32_t Generate_Prefix_EventCheck ( u32 Address, bool bIsBranchOrJump );
		
		
		
		// need to know what address current block starts at
		static u32 CurrentBlock_StartAddress;
		
		// also need to know what address the next block starts at
		static u32 NextBlock_StartAddress;
		
		
		// max number of cycles that instruction encoding could use up if executed
		// need to know this to single step when there are interrupts in between
		// code block is not valid when this is zero
		// *note* this should be dynamically allocated
		//u64* MaxCycles;	// [ c_iNumBlocks ];
		
		static u32 Local_LastModifiedReg;
		static u32 Local_NextPCModified;
		
		static u32 CurrentCount;
		
		static u32 isBranchDelaySlot;
		static u32 isLoadDelaySlot;
		
		static u32 bStopEncodingAfter;
		static u32 bStopEncodingBefore;
		
		// set the local cycle count to reset (start from zero) for the next cycle
		static u32 bResetCycleCount;
		
		inline void ResetFlags ( void ) { bStopEncodingBefore = false; bStopEncodingAfter = false; Local_NextPCModified = false; }

		
		
		// recompiler function
		// returns -1 if the instruction cannot be recompiled, otherwise returns the maximum number of cycles the instruction uses
		typedef int32_t (*Function) ( R5900::Instruction::Format Instruction, u32 Address );

		// *** todo *** do not recompile more than one instruction if currently in a branch delay slot or load delay slot!!
		u32 Recompile ( u32 StartAddress );
		u32 Recompile2 ( u32 ulBeginAddress );
		
		//void Invalidate ( u32 Address );
		void Invalidate ( u32 Address, u32 Count );
		
		u32 CloseOpLevel ( u32 OptLevel, u32 Address );
		
		static bool isABS(R5900::Instruction::Format i);
		static bool isCLIP(R5900::Instruction::Format i);
		static bool isITOF(R5900::Instruction::Format i);
		static bool isITOF0(R5900::Instruction::Format i);
		static bool isITOF4(R5900::Instruction::Format i);
		static bool isITOF12(R5900::Instruction::Format i);
		static bool isITOF15(R5900::Instruction::Format i);
		static bool isFTOI(R5900::Instruction::Format i);
		static bool isFTOI0(R5900::Instruction::Format i);
		static bool isFTOI4(R5900::Instruction::Format i);
		static bool isFTOI12(R5900::Instruction::Format i);
		static bool isFTOI15(R5900::Instruction::Format i);
		static bool isMIN(R5900::Instruction::Format i);
		static bool isMINBC(R5900::Instruction::Format i);
		static bool isMIN_NotI(R5900::Instruction::Format i);
		static bool isMINi(R5900::Instruction::Format i);
		static bool isMAX(R5900::Instruction::Format i);
		static bool isMAXBC(R5900::Instruction::Format i);
		static bool isMAX_NotI(R5900::Instruction::Format i);
		static bool isMAXi(R5900::Instruction::Format i);
		static bool isADD(R5900::Instruction::Format i);
		static bool isADDi(R5900::Instruction::Format i);
		static bool isADDq(R5900::Instruction::Format i);
		static bool isADDBC(R5900::Instruction::Format i);
		static bool isADDA(R5900::Instruction::Format i);
		static bool isADDAi(R5900::Instruction::Format i);
		static bool isADDAq(R5900::Instruction::Format i);
		static bool isADDABC(R5900::Instruction::Format i);
		static bool isSUB(R5900::Instruction::Format i);
		static bool isSUBi(R5900::Instruction::Format i);
		static bool isSUBq(R5900::Instruction::Format i);
		static bool isSUBBC(R5900::Instruction::Format i);
		static bool isSUBA(R5900::Instruction::Format i);
		static bool isSUBAi(R5900::Instruction::Format i);
		static bool isSUBAq(R5900::Instruction::Format i);
		static bool isSUBABC(R5900::Instruction::Format i);
		static bool isMUL(R5900::Instruction::Format i);
		static bool isMULi(R5900::Instruction::Format i);
		static bool isMULq(R5900::Instruction::Format i);
		static bool isMULBC(R5900::Instruction::Format i);
		static bool isMULA(R5900::Instruction::Format i);
		static bool isMULAi(R5900::Instruction::Format i);
		static bool isMULAq(R5900::Instruction::Format i);
		static bool isMULABC(R5900::Instruction::Format i);
		static bool isMADD(R5900::Instruction::Format i);
		static bool isMADDi(R5900::Instruction::Format i);
		static bool isMADDq(R5900::Instruction::Format i);
		static bool isMADDBC(R5900::Instruction::Format i);
		static bool isMADDA(R5900::Instruction::Format i);
		static bool isMADDAi(R5900::Instruction::Format i);
		static bool isMADDAq(R5900::Instruction::Format i);
		static bool isMADDABC(R5900::Instruction::Format i);
		static bool isMSUB(R5900::Instruction::Format i);
		static bool isMSUBi(R5900::Instruction::Format i);
		static bool isMSUBq(R5900::Instruction::Format i);
		static bool isMSUBBC(R5900::Instruction::Format i);
		static bool isMSUBA(R5900::Instruction::Format i);
		static bool isMSUBAi(R5900::Instruction::Format i);
		static bool isMSUBAq(R5900::Instruction::Format i);
		static bool isMSUBABC(R5900::Instruction::Format i);
		static bool isOPMULA(R5900::Instruction::Format i);
		static bool isOPMSUB(R5900::Instruction::Format i);

		static u32 ulIndex_Mask;
		
		inline static u32 Get_Block ( u32 Address ) { return ( Address >> ( 2 + MaxStep_Shift ) ) & NumBlocks_Mask; }
		inline static u32 Get_Index ( u32 Address ) { return ( Address >> 2 ) & ulIndex_Mask; }
		
		inline static u32 get_start_address(u32 PC) { return StartAddress[Get_Block(PC)]; }
		inline static u32* get_source_code_ptr(u32 PC) { return &pSourceCode32[Get_Index(PC)]; }
		static u8** get_code_start_ptr(u32 Address) { return &(pCodeStart[Get_Index(Address)]); }

		//void Recompile ( u32 Instruction );
		
			static const Function FunctionList [];
			
		// used by object to recompile an instruction into a code block
		// returns -1 if the instruction cannot be recompiled
		// returns 0 if the instruction was recompiled, but MUST start a new block for the next instruction (because it is guaranteed in a delay slot)
		// returns 1 if successful and can continue recompiling
		inline static int32_t Recompile ( R5900::Instruction::Format i, u32 Address ) { return R5900::Recompiler::FunctionList [ R5900::Instruction::Lookup::FindByInstruction ( i.Value ) ] ( i, Address ); }
		
		
		inline void Run ( u32 Address ) { InstanceEncoder->ExecuteCodeBlock ( ( Address >> 2 ) & NumBlocks_Mask ); }

			static int32_t Invalid ( R5900::Instruction::Format i, u32 Address );

			static int32_t J ( R5900::Instruction::Format i, u32 Address );
			static int32_t JAL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BEQ ( R5900::Instruction::Format i, u32 Address );
			static int32_t BNE ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLEZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGTZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADDI ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADDIU ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLTI ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLTIU ( R5900::Instruction::Format i, u32 Address );
			static int32_t ANDI ( R5900::Instruction::Format i, u32 Address );
			static int32_t ORI ( R5900::Instruction::Format i, u32 Address );
			static int32_t XORI ( R5900::Instruction::Format i, u32 Address );
			static int32_t LUI ( R5900::Instruction::Format i, u32 Address );
			static int32_t LB ( R5900::Instruction::Format i, u32 Address );
			static int32_t LH ( R5900::Instruction::Format i, u32 Address );
			static int32_t LWL ( R5900::Instruction::Format i, u32 Address );
			static int32_t LW ( R5900::Instruction::Format i, u32 Address );
			static int32_t LBU ( R5900::Instruction::Format i, u32 Address );
			static int32_t LHU ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t LWR ( R5900::Instruction::Format i, u32 Address );
			static int32_t SB ( R5900::Instruction::Format i, u32 Address );
			static int32_t SH ( R5900::Instruction::Format i, u32 Address );
			static int32_t SWL ( R5900::Instruction::Format i, u32 Address );
			static int32_t SW ( R5900::Instruction::Format i, u32 Address );
			static int32_t SWR ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLL ( R5900::Instruction::Format i, u32 Address );
			static int32_t SRL ( R5900::Instruction::Format i, u32 Address );
			static int32_t SRA ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLLV ( R5900::Instruction::Format i, u32 Address );
			static int32_t SRLV ( R5900::Instruction::Format i, u32 Address );
			static int32_t SRAV ( R5900::Instruction::Format i, u32 Address );
			static int32_t JR ( R5900::Instruction::Format i, u32 Address );
			static int32_t JALR ( R5900::Instruction::Format i, u32 Address );
			static int32_t SYSCALL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BREAK ( R5900::Instruction::Format i, u32 Address );
			static int32_t MFHI ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTHI ( R5900::Instruction::Format i, u32 Address );

			static int32_t MFLO ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTLO ( R5900::Instruction::Format i, u32 Address );
			static int32_t MULT ( R5900::Instruction::Format i, u32 Address );
			static int32_t MULTU ( R5900::Instruction::Format i, u32 Address );
			static int32_t DIV ( R5900::Instruction::Format i, u32 Address );
			static int32_t DIVU ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADDU ( R5900::Instruction::Format i, u32 Address );
			static int32_t SUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t SUBU ( R5900::Instruction::Format i, u32 Address );
			static int32_t AND ( R5900::Instruction::Format i, u32 Address );
			static int32_t OR ( R5900::Instruction::Format i, u32 Address );
			static int32_t XOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t NOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLT ( R5900::Instruction::Format i, u32 Address );
			static int32_t SLTU ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLTZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGEZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLTZAL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGEZAL ( R5900::Instruction::Format i, u32 Address );

			static int32_t MFC0 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTC0 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t CFC2_I ( R5900::Instruction::Format i, u32 Address );
			static int32_t CTC2_I ( R5900::Instruction::Format i, u32 Address );
			static int32_t CFC2_NI ( R5900::Instruction::Format i, u32 Address );
			static int32_t CTC2_NI ( R5900::Instruction::Format i, u32 Address );
			
			/*
			static int32_t MFC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t LWC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t SWC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t RFE ( R5900::Instruction::Format i, u32 Address );
			static int32_t RTPS ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCLIP ( R5900::Instruction::Format i, u32 Address );
			static int32_t OP ( R5900::Instruction::Format i, u32 Address );
			static int32_t DPCS ( R5900::Instruction::Format i, u32 Address );
			static int32_t INTPL ( R5900::Instruction::Format i, u32 Address );
			static int32_t MVMVA ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCDS ( R5900::Instruction::Format i, u32 Address );
			static int32_t CDP ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCDT ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCCS ( R5900::Instruction::Format i, u32 Address );
			static int32_t CC ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCS ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCT ( R5900::Instruction::Format i, u32 Address );
			static int32_t SQR ( R5900::Instruction::Format i, u32 Address );
			static int32_t DCPL ( R5900::Instruction::Format i, u32 Address );
			static int32_t DPCT ( R5900::Instruction::Format i, u32 Address );
			static int32_t AVSZ3 ( R5900::Instruction::Format i, u32 Address );
			static int32_t AVSZ4 ( R5900::Instruction::Format i, u32 Address );
			static int32_t RTPT ( R5900::Instruction::Format i, u32 Address );
			static int32_t GPF ( R5900::Instruction::Format i, u32 Address );
			static int32_t GPL ( R5900::Instruction::Format i, u32 Address );
			static int32_t NCCT ( R5900::Instruction::Format i, u32 Address );
			*/

			static int32_t COP2 ( R5900::Instruction::Format i, u32 Address );
			
			
			// *** R5900 Instructions *** //
			
			static int32_t BC0T ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC0TL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC0F ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC0FL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC1T ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC1TL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC1F ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC1FL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC2T ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC2TL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC2F ( R5900::Instruction::Format i, u32 Address );
			static int32_t BC2FL ( R5900::Instruction::Format i, u32 Address );
			

			static int32_t CFC0 ( R5900::Instruction::Format i, u32 Address );
			static int32_t CTC0 ( R5900::Instruction::Format i, u32 Address );
			static int32_t EI ( R5900::Instruction::Format i, u32 Address );
			static int32_t DI ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t SD ( R5900::Instruction::Format i, u32 Address );
			static int32_t LD ( R5900::Instruction::Format i, u32 Address );
			static int32_t LWU ( R5900::Instruction::Format i, u32 Address );
			static int32_t SDL ( R5900::Instruction::Format i, u32 Address );
			static int32_t SDR ( R5900::Instruction::Format i, u32 Address );
			static int32_t LDL ( R5900::Instruction::Format i, u32 Address );
			static int32_t LDR ( R5900::Instruction::Format i, u32 Address );
			static int32_t LQ ( R5900::Instruction::Format i, u32 Address );
			static int32_t SQ ( R5900::Instruction::Format i, u32 Address );
			
			
			// arithemetic instructions //
			static int32_t DADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t DADDI ( R5900::Instruction::Format i, u32 Address );
			static int32_t DADDU ( R5900::Instruction::Format i, u32 Address );
			static int32_t DADDIU ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSUBU ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSLL ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSLL32 ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSLLV ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRA ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRA32 ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRAV ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRL ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRL32 ( R5900::Instruction::Format i, u32 Address );
			static int32_t DSRLV ( R5900::Instruction::Format i, u32 Address );
			
			

			static int32_t MFC1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t CFC1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTC1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t CTC1 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t BEQL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BNEL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGEZL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLEZL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGTZL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLTZL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BLTZALL ( R5900::Instruction::Format i, u32 Address );
			static int32_t BGEZALL ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t CACHE ( R5900::Instruction::Format i, u32 Address );
			static int32_t PREF ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t TGEI ( R5900::Instruction::Format i, u32 Address );
			static int32_t TGEIU ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLTI ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLTIU ( R5900::Instruction::Format i, u32 Address );
			static int32_t TEQI ( R5900::Instruction::Format i, u32 Address );
			static int32_t TNEI ( R5900::Instruction::Format i, u32 Address );
			static int32_t TGE ( R5900::Instruction::Format i, u32 Address );
			static int32_t TGEU ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLT ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLTU ( R5900::Instruction::Format i, u32 Address );
			static int32_t TEQ ( R5900::Instruction::Format i, u32 Address );
			static int32_t TNE ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t MOVCI ( R5900::Instruction::Format i, u32 Address );
			static int32_t MOVZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t MOVN ( R5900::Instruction::Format i, u32 Address );
			static int32_t SYNC ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t MFHI1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTHI1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MFLO1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTLO1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MULT1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MULTU1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t DIV1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t DIVU1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADD1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADDU ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADDU1 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t MFSA ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTSA ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTSAB ( R5900::Instruction::Format i, u32 Address );
			static int32_t MTSAH ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t TLBR ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLBWI ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLBWR ( R5900::Instruction::Format i, u32 Address );
			static int32_t TLBP ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t ERET ( R5900::Instruction::Format i, u32 Address );
			static int32_t DERET ( R5900::Instruction::Format i, u32 Address );
			static int32_t WAIT ( R5900::Instruction::Format i, u32 Address );
			
			
			// Parallel instructions (SIMD) //
			static int32_t PABSH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PABSW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDSB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDSH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDSW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDUH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADDUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PADSBH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PAND ( R5900::Instruction::Format i, u32 Address );
			static int32_t POR ( R5900::Instruction::Format i, u32 Address );
			static int32_t PXOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t PNOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCEQB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCEQH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCEQW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCGTB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCGTH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCGTW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCPYH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCPYLD ( R5900::Instruction::Format i, u32 Address );
			static int32_t PCPYUD ( R5900::Instruction::Format i, u32 Address );
			static int32_t PDIVBW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PDIVUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PDIVW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXCH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXCW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXEH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXEW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXT5 ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTLB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTLH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTLW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTUH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PEXTUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PHMADH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PHMSBH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PINTEH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PINTH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PLZCW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMADDH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMADDW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMADDUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMAXH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMAXW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMINH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMINW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHI ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFLO ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMTHI ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMTLO ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHL_LH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHL_SH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHL_LW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHL_UW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMFHL_SLW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMTHL_LW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMSUBH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMSUBW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMULTH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMULTW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PMULTUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PPAC5 ( R5900::Instruction::Format i, u32 Address );
			static int32_t PPACB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PPACH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PPACW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PREVH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PROT3W ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSLLH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSLLVW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSLLW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRAH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRAW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRAVW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRLH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRLW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSRLVW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBSB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBSH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBSW ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBUH ( R5900::Instruction::Format i, u32 Address );
			static int32_t PSUBUW ( R5900::Instruction::Format i, u32 Address );
			static int32_t QFSRV ( R5900::Instruction::Format i, u32 Address );
			

			// floating point instructions //

			static int32_t LWC1 ( R5900::Instruction::Format i, u32 Address );
			static int32_t SWC1 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t ABS_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADD_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t ADDA_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t C_EQ_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t C_F_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t C_LE_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t C_LT_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t CVT_S_W ( R5900::Instruction::Format i, u32 Address );
			static int32_t CVT_W_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t DIV_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADD_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MADDA_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MAX_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MIN_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MOV_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MSUB_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MSUBA_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MUL_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t MULA_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t NEG_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t RSQRT_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t SQRT_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t SUB_S ( R5900::Instruction::Format i, u32 Address );
			static int32_t SUBA_S ( R5900::Instruction::Format i, u32 Address );
			

			// PS2 COP2 instructions //

			static int32_t LQC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t SQC2 ( R5900::Instruction::Format i, u32 Address );
			static int32_t QMFC2_NI ( R5900::Instruction::Format i, u32 Address );
			static int32_t QMTC2_NI ( R5900::Instruction::Format i, u32 Address );
			static int32_t QMFC2_I ( R5900::Instruction::Format i, u32 Address );
			static int32_t QMTC2_I ( R5900::Instruction::Format i, u32 Address );
			
			
			static int32_t VABS ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VSUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMUL ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMSUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMAX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMAXi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMAXBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMAXBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMAXBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMAXBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMINI ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMINIi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMINIBCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMINIBCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMINIBCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMINIBCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VDIV ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VADDA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDAi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDAq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDABCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDABCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDABCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VADDABCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VSUBA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBAi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBAq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBABCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBABCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBABCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSUBABCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMULA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULAi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULAq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULABCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULABCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULABCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMULABCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMADDA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDAi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDAq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDABCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDABCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDABCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMADDABCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VMSUBA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBAi ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBAq ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBABCX ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBABCY ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBABCZ ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMSUBABCW ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VOPMULA ( R5900::Instruction::Format i, u32 Address );
			static int32_t VOPMSUB ( R5900::Instruction::Format i, u32 Address );

			static int32_t VNOP ( R5900::Instruction::Format i, u32 Address );
			static int32_t VCLIP ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSQRT ( R5900::Instruction::Format i, u32 Address );
			static int32_t VRSQRT ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMR32 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VRINIT ( R5900::Instruction::Format i, u32 Address );
			static int32_t VRGET ( R5900::Instruction::Format i, u32 Address );
			static int32_t VRNEXT ( R5900::Instruction::Format i, u32 Address );
			static int32_t VRXOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMOVE ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMFIR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VMTIR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VLQD ( R5900::Instruction::Format i, u32 Address );
			static int32_t VLQI ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSQD ( R5900::Instruction::Format i, u32 Address );
			static int32_t VSQI ( R5900::Instruction::Format i, u32 Address );
			static int32_t VWAITQ ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VFTOI0 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VFTOI4 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VFTOI12 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VFTOI15 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VITOF0 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VITOF4 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VITOF12 ( R5900::Instruction::Format i, u32 Address );
			static int32_t VITOF15 ( R5900::Instruction::Format i, u32 Address );
			
			static int32_t VIADD ( R5900::Instruction::Format i, u32 Address );
			static int32_t VISUB ( R5900::Instruction::Format i, u32 Address );
			static int32_t VIADDI ( R5900::Instruction::Format i, u32 Address );
			static int32_t VIAND ( R5900::Instruction::Format i, u32 Address );
			static int32_t VIOR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VILWR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VISWR ( R5900::Instruction::Format i, u32 Address );
			static int32_t VCALLMS ( R5900::Instruction::Format i, u32 Address );
			static int32_t VCALLMSR ( R5900::Instruction::Format i, u32 Address );
	};
};

#endif	// end #ifndef __R5900_RECOMPILER_H__
