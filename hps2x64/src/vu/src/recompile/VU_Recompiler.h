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
#ifndef __VU_RECOMPILER_H__
#define __VU_RECOMPILER_H__

#include "VU.h"
#include "VU_Lookup.h"
#include "x64Encoder.h"

//class Playstation2::VU;

using namespace Playstation2;
using namespace Vu::Instruction;

namespace Playstation2
{
	class VU;
}

namespace Vu
{
	
	//using namespace Playstation2;
	//class Playstation2::VU;
	
	// will probably create two recompilers for each processor. one for single stepping and one for multi-stepping
	class Recompiler
	{
	public:

		static Recompiler *_REC;

		// number of code cache slots total
		// but the number of blocks should be variable
		//static const int c_iNumBlocks = 1024;
		//static const u32 c_iNumBlocks_Mask = c_iNumBlocks - 1;
		u32 NumBlocks;
		u32 NumBlocks_Mask;
		//static const int c_iBlockSize_Shift = 4;
		//static const int c_iBlockSize_Mask = ( 1 << c_iBlockSize_Shift ) - 1;
		
		static const u32 c_iAddress_Mask = 0x1fffffff;
		
		u32 TotalCodeCache_Size;	// = c_iNumBlocks * ( 1 << c_iBlockSize_Shift );
		
		u32 BlockSize;
		u32 BlockSize_Shift;
		u32 BlockSize_Mask;
		
		// maximum number of instructions that can be encoded/recompiled in a run
		u32 MaxStep;
		u32 MaxStep_Shift;
		u32 MaxStep_Mask;

		// the amount of stack space required for SEH
		static const int32_t c_lSEH_StackSize = 40;

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
		

		static constexpr u32 flag_mask_lut32[16] = {
			0x00000000, 0xff000000, 0x00ff0000, 0xffff0000, 0x0000ff00, 0xff00ff00, 0x00ffff00, 0xffffff00,
			0x000000ff, 0xff0000ff, 0x00ff00ff, 0xffff00ff, 0x0000ffff, 0xff00ffff, 0x00ffffff, 0xffffffff
		};


		static constexpr u32 xyzw_mask_lut32[16] = {
			0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
			0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
		};


		static char reverse_bytes_lut128[16];


		RDelaySlot RDelaySlots [ 2 ];
		u32 DSIndex;
		
		u32 RDelaySlots_Valid;
		
		inline void ClearDelaySlots ( void ) { RDelaySlots [ 0 ].Value = 0; RDelaySlots [ 1 ].Value = 0; }
		
		// need something to use to keep track of dynamic delay slot stuff
		u32 RecompilerDelaySlot_Flags;
		
		
		// the next instruction in the cache block if there is one
		Vu::Instruction::Format NextInst;
		
		u64 MemCycles;

		u64 LocalCycleCount;
		u64 CacheBlock_CycleCount;
		
		// also need to know if the addresses in the block are cached or not
		bool bIsBlockInICache;

		
		// the PC while recompiling
		u32 LocalPC;


		// need to know the checksum for code that is recompiled for caching
		u64 ullChecksum;

		// need to calculate the checksum
		static u64 Calc_Checksum( VU *v );


		static int32_t Dispatch_Result_AVX2(x64Encoder* e, VU* v, Vu::Instruction::Format i, int32_t Accum, int32_t sseSrcReg, int32_t VUDestReg);


		// static analysis data //
		
		// up to 2k instructions (16k bytes / 8 bytes per instruction)
		// up to 64 possibilities per instruction (instr count,q/p wait1,q/p wait2)
		// 8-bits/1-byte per possibility
		//u8 LUT_StaticDelay [ 2048 * 256 ];
		//u8 LUT_StaticDelay [ 2048 * 64 ];

		// also need to know where to pull the flags from if instruction needs them
		//u8 LUT_StaticFlags [ 2048 * 4 ];

		// analysis flags
		// bit 0 - upper/lower register hazard (calc wait/delay unless zero, then no need to calc wait/delay)
		// bit 1 - output mac flag registers to history buffer at instruction end
		// bit 2 - output stat flag registers to history buffer at instruction end
		// bit 3 - output clip flag registers to history buffer at instruction end
		// bit 4 - both upper and lower instruction output to mac/stat or clip flag or register, ignore lower instruction
		// bit 5 - complete move instruction at end (store from temporary location, unless cancelled)
		// bit 6 - conditional branch instruction
		// bit 7 - unconditional branch instruction
		// bit 8 - conditional branch delay slot (branch after instruction if instr# not zero)
		// bit 9 - unconditional branch delay slot (branch after instruction if instr# not zero)
		// bit 10 - output int to current delay4 slot (conditional branch next that uses previous int dest reg)
		// bit 11 - load int from previous delay4 slot if instr# count set after conditional branch at end
		// bit 12 - lower register hazard detected
		// bit 13 - experimental int delay4 slot detected (load/store with inc/dec with inc/dec reg used later)
		// bit 14/15 - last conflict count (for float or int load conflict/wait)
		// bit 16 - bit0 has a simple hazard, not a complex one
		// bit 17 - e-bit delay slot
		// bit 18 - output CycleCount to history buffer at instruction end
		// bit 19 - update q (NOT waitq) before upper instruction
		// bit 20 - reverse order of lower/upper execution (execute upper first, then lower)
		// bit 21 - mac flag check
		// bit 22 - stat flag check
		// bit 23 - clip flag check
		// bit 24 - output mac flag data
		// bit 25 - output stat flag data
		// bit 26 - output clip flag data
		// bit 27 - implicit waitq
		// bit 28 - implicit waitp
		// bit 29 - implicit updatep (not waitp) (for mfp instruction)
		// bit 30 - xgkick delay slot

		// bit 0 - execute instruction in parallel
		// note: need this to determine if instruction is already set to execute in parallel or not
		static constexpr const u32 STATIC_PARALLEL_EXEx2 = (1 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXEx3 = (2 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXEx4 = (3 << 0);
		static constexpr const u32 STATIC_PARALLEL_EXE_MASK = (3 << 0);

		// bit 2 - disable flag in parallel summary (generate flags instead of summarizing them)
		// do this for upcoming branches/jumps and upcoming flag checks (not upcoming flag sets)
		// should only matter with an avx512x4 instruction width since otherwise you don't really gain anything
		static constexpr const u32 STATIC_DISABLE_FLAG_SUMMARY = (1 << 2);

		// bit 3 - both mac and stat flag from this instruction gets overwritten later and never checked
		static constexpr const u32 STATIC_FLAG_OVERWRITE_MACSTAT = (1 << 3);

		// if there is a flag check or branch/jump coming up, then need to output the flag at instruction end (do not combine flag checks here)
		//static constexpr const u32 STATIC_OUTPUT_FLAG_MAC = (1 << 1);
		//static constexpr const u32 STATIC_OUTPUT_FLAG_STAT = (1 << 2);
		//static constexpr const u32 STATIC_OUTPUT_FLAG_CLIP = (1 << 3);

		// the destination register of upper instruction overwrites destination register of lower instruction
		static constexpr const u32 STATIC_SKIP_EXEC_LOWER = (1 << 4);
		static constexpr const u32 STATIC_COMPLETE_FLOAT_MOVE = (1 << 5);

		// there is an actual branch/jump
		//static constexpr const u32 STATIC_COND_BRANCH_START = (1 << 6);
		//static constexpr const u32 STATIC_UNCOND_BRANCH_START = (1 << 7);
		//static constexpr const u32 STATIC_ANY_BRANCH_START = (STATIC_COND_BRANCH_START | STATIC_UNCOND_BRANCH_START);

		// bits 6-7 - parrallel instruction index - indexes go in reverse so that index0 is always where the parallel op gets executed
		// note: index 0 with no index 1 immediately before means that there is no parallel execution
		static constexpr const u32 STATIC_PARALLEL_INDEX0 = (0 << 6);
		static constexpr const u32 STATIC_PARALLEL_INDEX1 = (1 << 6);
		static constexpr const u32 STATIC_PARALLEL_INDEX2 = (2 << 6);
		static constexpr const u32 STATIC_PARALLEL_INDEX3 = (3 << 6);
		static constexpr const u32 STATIC_PARALLEL_INDEX_MASK = (3 << 6);
		static constexpr const u32 STATIC_PARALLEL_INDEX_SHIFT = (6);


		// this is a branch/jump delay slot
		static constexpr const u32 STATIC_COND_BRANCH_DELAY_SLOT = (1 << 8);
		static constexpr const u32 STATIC_UNCOND_BRANCH_DELAY_SLOT = (1 << 9);
		static constexpr const u32 STATIC_ANY_BRANCH_DELAY_SLOT = (STATIC_COND_BRANCH_DELAY_SLOT | STATIC_UNCOND_BRANCH_DELAY_SLOT);

		// move integer value into temporary storage for the delay slot
		static constexpr const u32 STATIC_MOVE_TO_INT_DELAY_SLOT = (1 << 10);

		// move integer value from temporary storage into register after executing the instruction
		static constexpr const u32 STATIC_MOVE_FROM_INT_DELAY_SLOT = (1 << 11);

		// means to assume there is a read-after-write hazard
		// next read-after-write hazard location upper/lower
		//static constexpr const u32 STATIC_HAZARD_RAW_UPPER = (1 << 12);
		//static constexpr const u32 STATIC_HAZARD_RAW_LOWER = (1 << 13);
		//static constexpr const u32 STATIC_HAZARD_RAW_MASK = (3 << 12);

		// bits 12-13 - available
		static constexpr const u32 STATIC_RUNTIME_FORCED_EXIT = (1 << 12);
		static constexpr const u32 STATIC_RUNTIME_FORCED_LEVEL0 = (1 << 13);


		// bits 14-15, count to the next read-after-write hazard/conflict location
		// note: only interested in upper hazards to check for parallel instructions
		static constexpr const u32 STATIC_HAZARD_RAW_1 = (1 << 14);
		static constexpr const u32 STATIC_HAZARD_RAW_2 = (2 << 14);
		static constexpr const u32 STATIC_HAZARD_RAW_3 = (3 << 14);
		static constexpr const u32 STATIC_HAZARD_RAW_X_MASK = (3 << 14);

		// bits 16-17 - e-bit
		//static constexpr const u32 STATIC_EBIT_START = (1 << 16);
		static constexpr const u32 STATIC_EBIT_DELAY_SLOT = (1 << 16);

		// bits 17 - branch/jump/xgkick/e-bit in e-bit delay slot
		static constexpr const u32 STATIC_PROBLEM_IN_EBIT_DELAY_SLOT = (1 << 17);
		//static constexpr const u32 STATIC_XGKICK_IN_EBIT_DELAY_SLOT = (1 << 19);

		// bits 18-19 - parrallel instruction type - add/mul-add/convertto/convertfrom
		// note: can't do parallel instructions through an m-bit/branch-delay-slot/etc
		static constexpr const u32 STATIC_PARALLEL_TYPE_ADD = (0 << 18);
		static constexpr const u32 STATIC_PARALLEL_TYPE_MUL = (1 << 18);
		static constexpr const u32 STATIC_PARALLEL_TYPE_ITOF = (2 << 18);
		static constexpr const u32 STATIC_PARALLEL_TYPE_FTOI = (3 << 18);
		static constexpr const u32 STATIC_PARALLEL_TYPE_MASK = (3 << 18);

		// execute upper instruction first instead of the lower instruction first
		static constexpr const u32 STATIC_REVERSE_EXE_ORDER = (1 << 20);

		// bits 21-23 - flag checks
		static constexpr const u32 STATIC_FLAG_CHECK_MAC = (1 << 21);
		static constexpr const u32 STATIC_FLAG_CHECK_STAT = (1 << 22);
		static constexpr const u32 STATIC_FLAG_CHECK_CLIP = (1 << 23);

		// update q/p - like mulq or mfp, etc
		static constexpr const u32 STATIC_UPDATE_BEFORE_Q = (1 << 24);
		static constexpr const u32 STATIC_UPDATE_BEFORE_P = (1 << 25);

		// wait q/p - like div/sqrt or p instruction, etc
		static constexpr const u32 STATIC_WAIT_BEFORE_Q = (1 << 26);
		static constexpr const u32 STATIC_WAIT_BEFORE_P = (1 << 27);

		// bit 28 - branch in branch delay slot
		static constexpr const u32 STATIC_BRANCH_IN_BRANCH_DELAY_SLOT = (1 << 28);

		// bits 29 - xgkick
		//static constexpr const u32 STATIC_XGKICK_START = (1 << 29);
		static constexpr const u32 STATIC_XGKICK_DELAY_SLOT = (1 << 29);

		// bit 30 - int hazard - read-after-write
		static constexpr const u32 STATIC_HAZARD_INT_RAW = (1 << 30);

		// bit 31 - stat flag set instruction - no need to output stat flag value
		static constexpr const u32 STATIC_FLAG_SET_STAT = (1 << 31);


		// todo: *note* don't forget to make sure q/p set at very end of vu process run (unless a vu0 continue)
		u32 LUT_StaticInfo [ 2048 ];

		// basic tests to allow instructions to execute in parallel
		bool Test_StaticParallel(u32 ulInstCount, int i3, int i2, int i1, int i0);

		// perform static analysis
		void StaticAnalysis ( VU *v );

		// static check for the flags checks
		void StaticAnalysis_FlagCheck(VU* v, u32 ulInstCount_4, u32 ulInstCount_3, u32 ulInstCount_2, u32 ulInstCount_1, u32 ulInstCount_0);

		void StaticAnalysis_HazardCheck(
			VU* v, u32 ulInstCount_3, u32 ulInstCount_2, u32 ulInstCount_1,
			VU::Bitmap128 FSrc_0, VU::Bitmap128 FDst_1, VU::Bitmap128 FDst_2, VU::Bitmap128 FDst_3,
			u64 ISrc_0, u64 IDst_1, u64 IDst_2, u64 IDst_3
		);


		// print static analysis results
		void Print_StaticAnalysis(VU* v);


		// encodes the instructions needed to advance a cycle in the vu recompiler
		bool Recompile_AdvanceCycle(VU* v, u32 Address);

		// check if current encoding position is a known delay slot
		inline bool isKnownEbitDelaySlot(Vu::Instruction::Format64 PrevInst0) { return PrevInst0.Hi.E; }
		inline bool isKnownXgKickDelaySlot(Vu::Instruction::Format64 PrevInst0) { return isXgKick(PrevInst0); }
		inline bool isKnownIntDelaySlot(Vu::Instruction::Format64 PrevInst0) { return isIntCalc(PrevInst0); }
		inline bool isKnownBranchDelaySlot(Vu::Instruction::Format64 PrevInst0) { return isBranchOrJump(PrevInst0); }

		static bool isBranch ( Vu::Instruction::Format64 i );
		static bool isJump(Vu::Instruction::Format64 i);
		static bool isBranchOrJump(Vu::Instruction::Format64 i);
		bool isConditionalBranch ( Vu::Instruction::Format64 i );
		bool isUnconditionalBranch ( Vu::Instruction::Format64 i );
		bool isUnconditionalBranchOrJump(Vu::Instruction::Format64 i);
		bool isMacFlagCheck ( Vu::Instruction::Format64 i );
		bool isStatFlagCheck ( Vu::Instruction::Format64 i );
		bool isClipFlagCheck ( Vu::Instruction::Format64 i );
		bool isStatFlagSetLo ( Vu::Instruction::Format64 i );
		bool isClipFlagSetLo ( Vu::Instruction::Format64 i );
		bool isLowerImmediate ( Vu::Instruction::Format64 i );
		bool isClipFlagSetHi ( Vu::Instruction::Format64 i );
		bool isMacStatFlagSetHi ( Vu::Instruction::Format64 i );

		u64 getSrcRegMapHi ( Vu::Instruction::Format64 i );
		u64 getDstRegMapHi ( Vu::Instruction::Format64 i );
		void getDstFieldMapHi ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i );
		void getSrcFieldMapHi ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i );
		void getDstFieldMapLo ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i );
		void getSrcFieldMapLo ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i );
		u64 getDstRegMapLo ( Vu::Instruction::Format64 i );
		u64 getSrcRegMapLo ( Vu::Instruction::Format64 i );

		bool isMoveToFloatLo ( Vu::Instruction::Format64 i );
		bool isMoveToFloatFromFloatLo ( Vu::Instruction::Format64 i );

		bool isIntLoad ( Vu::Instruction::Format64 i );

		bool isFloatLoadStore ( Vu::Instruction::Format64 i );

		bool isQWait ( Vu::Instruction::Format64 i );
		bool isPWait ( Vu::Instruction::Format64 i );

		bool isQUpdate ( Vu::Instruction::Format64 i );
		bool isPUpdate ( Vu::Instruction::Format64 i );

		static bool isXgKick ( Vu::Instruction::Format64 i );
		static bool isIntCalc ( Vu::Instruction::Format64 i );

		static bool isABS(Vu::Instruction::Format64 i);
		static bool isCLIP(Vu::Instruction::Format64 i);
		static bool isITOF(Vu::Instruction::Format64 i);
		static bool isITOF0(Vu::Instruction::Format64 i);
		static bool isITOF4(Vu::Instruction::Format64 i);
		static bool isITOF12(Vu::Instruction::Format64 i);
		static bool isITOF15(Vu::Instruction::Format64 i);
		static bool isFTOI(Vu::Instruction::Format64 i);
		static bool isFTOI0(Vu::Instruction::Format64 i);
		static bool isFTOI4(Vu::Instruction::Format64 i);
		static bool isFTOI12(Vu::Instruction::Format64 i);
		static bool isFTOI15(Vu::Instruction::Format64 i);
		static bool isMIN(Vu::Instruction::Format64 i);
		static bool isMINBC(Vu::Instruction::Format64 i);
		static bool isMIN_NotI(Vu::Instruction::Format64 i);
		static bool isMINi(Vu::Instruction::Format64 i);
		static bool isMAX(Vu::Instruction::Format64 i);
		static bool isMAXBC(Vu::Instruction::Format64 i);
		static bool isMAX_NotI(Vu::Instruction::Format64 i);
		static bool isMAXi(Vu::Instruction::Format64 i);
		static bool isADD(Vu::Instruction::Format64 i);
		static bool isADDi(Vu::Instruction::Format64 i);
		static bool isADDq(Vu::Instruction::Format64 i);
		static bool isADDBC(Vu::Instruction::Format64 i);
		static bool isADDA(Vu::Instruction::Format64 i);
		static bool isADDAi(Vu::Instruction::Format64 i);
		static bool isADDAq(Vu::Instruction::Format64 i);
		static bool isADDABC(Vu::Instruction::Format64 i);
		static bool isSUB(Vu::Instruction::Format64 i);
		static bool isSUBi(Vu::Instruction::Format64 i);
		static bool isSUBq(Vu::Instruction::Format64 i);
		static bool isSUBBC(Vu::Instruction::Format64 i);
		static bool isSUBA(Vu::Instruction::Format64 i);
		static bool isSUBAi(Vu::Instruction::Format64 i);
		static bool isSUBAq(Vu::Instruction::Format64 i);
		static bool isSUBABC(Vu::Instruction::Format64 i);
		static bool isMUL(Vu::Instruction::Format64 i);
		static bool isMULi(Vu::Instruction::Format64 i);
		static bool isMULq(Vu::Instruction::Format64 i);
		static bool isMULBC(Vu::Instruction::Format64 i);
		static bool isMULA(Vu::Instruction::Format64 i);
		static bool isMULAi(Vu::Instruction::Format64 i);
		static bool isMULAq(Vu::Instruction::Format64 i);
		static bool isMULABC(Vu::Instruction::Format64 i);
		static bool isMADD(Vu::Instruction::Format64 i);
		static bool isMADDi(Vu::Instruction::Format64 i);
		static bool isMADDq(Vu::Instruction::Format64 i);
		static bool isMADDBC(Vu::Instruction::Format64 i);
		static bool isMADDA(Vu::Instruction::Format64 i);
		static bool isMADDAi(Vu::Instruction::Format64 i);
		static bool isMADDAq(Vu::Instruction::Format64 i);
		static bool isMADDABC(Vu::Instruction::Format64 i);
		static bool isMSUB(Vu::Instruction::Format64 i);
		static bool isMSUBi(Vu::Instruction::Format64 i);
		static bool isMSUBq(Vu::Instruction::Format64 i);
		static bool isMSUBBC(Vu::Instruction::Format64 i);
		static bool isMSUBA(Vu::Instruction::Format64 i);
		static bool isMSUBAi(Vu::Instruction::Format64 i);
		static bool isMSUBAq(Vu::Instruction::Format64 i);
		static bool isMSUBABC(Vu::Instruction::Format64 i);
		static bool isOPMULA(Vu::Instruction::Format64 i);
		static bool isOPMSUB(Vu::Instruction::Format64 i);
		static bool isEType(Vu::Instruction::Format64 i);
		static bool isAType(Vu::Instruction::Format64 i);
		static bool isMType(Vu::Instruction::Format64 i);

		static bool Perform_GetMacFlag ( x64Encoder *e, VU* v, u32 Address );
		static bool Perform_GetStatFlag ( x64Encoder *e, VU* v, u32 Address );
		static bool Perform_GetClipFlag ( x64Encoder *e, VU* v, u32 Address );

		static bool Perform_WaitQ(x64Encoder* e, VU* v, u32 Address);
		static bool Perform_UpdateQ(x64Encoder* e, VU* v, u32 Address);

		static bool Perform_WaitP ( x64Encoder *e, VU* v, u32 Address );
		static bool Perform_UpdateP ( x64Encoder *e, VU* v, u32 Address );

		// the optimization level
		// 0 means no optimization at all, anything higher means to optimize
		//s32 OpLevel;
		//u32 OptimizeLevel;
		
		// the current enabled encoder
		x64Encoder *e;
		//static ICache_Device *ICache;
		//static VU::Cpu *r;
		
		// the encoder for this particular instance
		x64Encoder *InstanceEncoder;
		
		// bitmap for branch delay slot
		//u32 Status_BranchDelay;
		//u32 Status_BranchConditional;
		//Vu::Instruction::Format Status_BranchInstruction;
		
		u32 Status_EBit;
		
		// the maximum number of cache blocks it can encode across
		s32 MaxCacheBlocks;
		
		
		//u32 SetStatus_Flag;
		//u32 SetClip_Flag;

		Vu::Instruction::Format instLO;
		Vu::Instruction::Format instHI;
		



		static void AdvanceCycle(VU* v);
		static void AdvanceCycle_rec(x64Encoder* e, VU* v, u32 Address);

		static bool SetDstRegsWait_ReadCycleCount(x64Encoder* e, VU* v, u32 Address);

		static bool SetIDstRegsWait_rec(x64Encoder* e, VU* v, u32 Address);
		static bool SetFDstRegsWait_rec(x64Encoder* e, VU* v, u32 Address);

		static bool WaitForSrcRegs_ReadCycleCount(x64Encoder* e, VU* v, u32 Address);
		static bool WaitForSrcRegs_WriteCycleCount(x64Encoder* e, VU* v, u32 Address);

		static bool WaitForISrcRegs_rec(x64Encoder* e, VU* v, u32 Address);
		static bool WaitForFSrcRegs_rec(x64Encoder* e, VU* v, u32 Address);

		static bool UpdateFlags_rec(x64Encoder* e, VU* v, u32 Address);
		
		inline void Set_MaxCacheBlocks ( s32 Value ) { MaxCacheBlocks = Value; }
		
		// constructor
		// block size must be power of two, multiplier shift value
		// so for BlockSize_PowerOfTwo, for a block size of 4 pass 2, block size of 8 pass 3, etc
		// for MaxStep, use 0 for single stepping, 1 for stepping until end of 1 cache block, 2 for stepping until end of 2 cache blocks, etc
		// no, for MaxStep, it should be the maximum number of instructions to step
		// NumberOfBlocks MUST be a power of 2, where 1 means 2, 2 means 4, etc
		Recompiler ( VU* v, u32 NumberOfBlocks, u32 BlockSize_PowerOfTwo, u32 MaxIStep );
		
		// destructor
		~Recompiler ();
		
		
		void Reset ();	// { memset ( this, 0, sizeof( Recompiler ) ); }

		
		static bool isBranchDelayOk ( u32 ulInstruction, u32 Address );

		
		static u32* RGetPointer ( VU *v, u32 Address );
		
		
		// set the optimization level
		inline void SetOptimizationLevel ( VU* v, u32 Level ) { v->OptimizeLevel = Level; }
		
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


		static int Prefix_MADDW ( VU* v, Vu::Instruction::Format i );
		static int Postfix_MADDW ( VU* v, Vu::Instruction::Format i );
		
		
		// initializes a code block that is not being used yet
		u32 InitBlock ( u32 Block );
		

		inline static void Clear_FSrcReg(VU* v) { v->iFSrcReg_Count = 0; }
		inline static void Add_FSrcReg(VU* v, u32 i, u32 SrcReg) { if (SrcReg) { v->uFSrcReg_Index8[v->iFSrcReg_Count] = SrcReg; v->uFSrcReg_Mask8[v->iFSrcReg_Count++] = (i >> 21) & 0xf; } }
		inline static void Add_FSrcRegBC(VU* v, u32 i, u32 SrcReg) { if (SrcReg) { v->uFSrcReg_Index8[v->iFSrcReg_Count] = SrcReg; v->uFSrcReg_Mask8[v->iFSrcReg_Count++] = 0x8 >> (i & 0x3); } }

		inline static void Clear_ISrcReg(VU* v) { v->ISrcBitmap = 0; v->iISrcReg_Count = 0; }
		inline static void Add_ISrcReg ( VU* v, u32 SrcReg ) { if ( SrcReg & 31 ) { v->uISrcReg_Index8[v->iISrcReg_Count++] = SrcReg; } }

		inline static void Clear_DstReg ( VU* v ) { v->iFDstReg_Count = 0; v->iIDstReg_Count = 0; }
		inline static void Add_FDstReg ( VU* v, u32 i, u32 DstReg ) { if ( DstReg ) { v->uFDstReg_Index8[v->iFDstReg_Count] = DstReg; v->uFDstReg_Mask8[v->iFDstReg_Count++] = (i >> 21) & 0xf; } }
		inline static void Add_IDstReg ( VU* v, u32 DstReg ) { if ( DstReg & 31 ) { v->uIDstReg_Index8[v->iIDstReg_Count++] = DstReg; } }
		
		static void PipelineWaitQ ( VU* v );
		
		static void TestStall ( VU* v );
		static void TestStall_INT ( VU* v );


		int32_t ProcessBranch ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 Address );

		
		//static u64 GetFSourceRegsHI_LoXYZW ( Vu::Instruction::Format i );
		//static u64 GetFSourceRegsHI_HiXYZW ( Vu::Instruction::Format i );
		//static u64 GetFSourceRegsLO_LoXYZW ( Vu::Instruction::Format i );
		//static u64 GetFSourceRegsLO_HiXYZW ( Vu::Instruction::Format i );

		//static u64 GetFDestRegsHI_LoXYZW ( Vu::Instruction::Format i );
		//static u64 GetFDestRegsHI_HiXYZW ( Vu::Instruction::Format i );
		
		// get a bitmap for the source registers used by the specified instruction
		//static u64 GetFSourceRegsHI ( Vu::Instruction::Format i );
		//static u64 GetFSourceRegsLO ( Vu::Instruction::Format i );
		//static u64 GetFDestRegsHI ( Vu::Instruction::Format i );
		static u64 Get_DelaySlot_DestRegs ( Vu::Instruction::Format i );
		
		//static int32_t Generate_Normal_Store ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* StoreFunctionToCall );
		//static int32_t Generate_Normal_Load ( R5900::Instruction::Format i, u32 Address, u32 BitTest, void* LoadFunctionToCall );
		//static int32_t Generate_Normal_Branch ( R5900::Instruction::Format i, u32 Address, void* BranchFunctionToCall );
		//static int32_t Generate_Normal_Trap ( R5900::Instruction::Format i, u32 Address );

		// e-unit //

		static int32_t ENCODE_EATAN(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_EATANxy(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_EATANxz(x64Encoder* e, VU* v, Vu::Instruction::Format i);

		static int32_t ENCODE_EEXP(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ESIN(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ERSQRT(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ERCPR(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ESQRT(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ESADD(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ERSADD(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ESUM(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ELENG(x64Encoder* e, VU* v, Vu::Instruction::Format i);
		static int32_t ENCODE_ERLENG(x64Encoder* e, VU* v, Vu::Instruction::Format i);


		// x1 instructions (SSE/AVX2/AVX512)
		static int32_t Generate_VABSp ( x64Encoder *e, VU* v, Vu::Instruction::Format i );
		static int32_t Generate_VMAXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VMINp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VFTOIXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 IX );
		static int32_t Generate_VITOFXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u64 FX );
		static int32_t Generate_VMOVEp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 Address );
		static int32_t Generate_VMR32p ( x64Encoder *e, VU* v, Vu::Instruction::Format i );
		static int32_t Generate_VMFIRp ( x64Encoder *e, VU* v, Vu::Instruction::Format i );
		static int32_t Generate_VMTIRp ( x64Encoder *e, VU* v, Vu::Instruction::Format i );
		static int32_t Generate_VADDp ( x64Encoder *e, VU* v, u32 bSub, Vu::Instruction::Format i, u32 FtComponent = 4, void *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMULp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 FtComponentp = 0x1b, void *pFd = NULL, u32 *pFt = NULL, u32 FsComponentp = 0x1b );
		static int32_t Generate_VMADDp ( x64Encoder *e, VU* v, u32 bSub, Vu::Instruction::Format i, u32 FtComponentp = 0x1b, void *pFd = NULL, u32 *pFt = NULL, u32 FsComponentp = 0x1b );

		// x2/x4 argument settings (instead of executing the instruction it sets the arguments and then executes it later in parallel)
		static int32_t Generate_XARGS1p(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX);
		static int32_t Generate_XARGS2p(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX);
		static int32_t Generate_XARGS2m(x64Encoder* e, VU* v, Vu::Instruction::Format i, u32* pFt = NULL, u32 FtComponent = 4, int iIndexX = 0);

		// for OPMULA
		static int32_t Generate_XARGS2op(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX);

		// for OPMSUB
		static int32_t Generate_XARGS2opm(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX);

		// x2 instructions (AVX2/AVX512)
		static int32_t Generate_VMAXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);
		static int32_t Generate_VMINpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);
		static int32_t Generate_VFTOIXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, u32 IX);
		static int32_t Generate_VITOFXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, u64 FX);
		static int32_t Generate_VCLIPpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);
		static int32_t Generate_VMULpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);
		static int32_t Generate_VADDpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);
		static int32_t Generate_VMADDpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1);

		// todo: x4 instructions (AVX512)
		static int32_t Generate_VMAXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3);
		static int32_t Generate_VMINpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3);
		static int32_t Generate_VFTOIXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3, u32 IX);
		static int32_t Generate_VITOFXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3, u64 FX);
		static int32_t Generate_VMULpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3);
		static int32_t Generate_VADDpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3);

		static int32_t Generate_VMATpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3);


		// testing
		static int32_t Generate_VFTOIXp_test ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 IX );
		static void Test_FTOI4 ( VU* v, Vu::Instruction::Format i );

		/*
		static int32_t Generate_VABSp ( Vu::Instruction::Format i );
		static int32_t Generate_VMAXp ( Vu::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VMINp ( Vu::Instruction::Format i, u32 *pFt = NULL, u32 FtComponent = 4 );
		static int32_t Generate_VFTOIXp ( Vu::Instruction::Format i, u32 IX );
		static int32_t Generate_VITOFXp ( Vu::Instruction::Format i, u32 FX );
		static int32_t Generate_VMOVEp ( Vu::Instruction::Format i );
		static int32_t Generate_VMR32p ( Vu::Instruction::Format i );
		static int32_t Generate_VMFIRp ( Vu::Instruction::Format i );
		static int32_t Generate_VMTIRp ( Vu::Instruction::Format i );
		static int32_t Generate_VADDp ( u32 bSub, Vu::Instruction::Format i, u32 FtComponent = 4, void *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMULp ( Vu::Instruction::Format i, u32 FtComponentp = 0x1b, void *pFd = NULL, u32 *pFt = NULL, u32 FsComponentp = 0x1b );
		static int32_t Generate_VMADDp ( u32 bSub, Vu::Instruction::Format i, u32 FtComponentp = 0x1b, void *pFd = NULL, u32 *pFt = NULL, u32 FsComponentp = 0x1b );
		
		
		static int32_t Generate_VABS ( Vu::Instruction::Format i, u32 Address, u32 Component );
		static int32_t Generate_VMAX ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFt = NULL );
		static int32_t Generate_VMIN ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFt = NULL );
		static int32_t Generate_VFTOI0 ( Vu::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent );
		static int32_t Generate_VFTOIX ( Vu::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent, u32 IX );
		static int32_t Generate_VITOFX ( Vu::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent, u32 FX );
		static int32_t Generate_VMOVE ( Vu::Instruction::Format i, u32 Address, u32 FtComponent, u32 FsComponent );
		
		static int32_t Generate_VMR32_Load ( Vu::Instruction::Format i, u32 Address, u32 FsComponent );
		static int32_t Generate_VMR32_Store ( Vu::Instruction::Format i, u32 Address, u32 FtComponent );
		
		static int32_t Generate_VMFIR ( Vu::Instruction::Format i, u32 Address, u32 FtComponent );
		static int32_t Generate_VADD ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VSUB ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMUL ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMADD ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		static int32_t Generate_VMSUB ( Vu::Instruction::Format i, u32 Address, u32 FdComponent, u32 FsComponent, u32 FtComponent, u32 *pFd = NULL, u32 *pFt = NULL );
		*/
		
		
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
		u32* StartAddress;	// [ c_iNumBlocks ];
		
		
		// last address that instruction encoding is valid for (inclusive)
		// *note* this should be dynamically allocated
		// It actually makes things MUCH simpler if you store the number of instructions recompiled instead of the last address
		//u32* LastAddress;	// [ c_iNumBlocks ];
		//u8* RunCount;
		
		//u32* Instructions;
		
		// pointer to where the prefix for the instruction starts at (used only internally by recompiler)
		u8** pPrefix_CodeStart;
		
		// where the actual instruction starts at in code block
		u8** pCodeStart;
		
		// the number of offset cycles from this instruction in the code block
		u32* CycleCount;


		// list of branch targets when jumping forwards in code block
		u32* pForwardBranchTargets;
		
		static const int c_ulForwardBranchIndex_Start = 5;
		u32 ForwardBranchIndex;
		
		u32 StartBlockIndex;
		u32 BlockIndex;
		
		// not needed
		//u32* EndAddress;
		
		
		int32_t Generate_Prefix_EventCheck ( u32 Address, bool bIsBranchOrJump );
		
		
		// need to know what address current block starts at
		u32 CurrentBlock_StartAddress;
		
		// also need to know what address the next block starts at
		u32 NextBlock_StartAddress;
		
		
		// max number of cycles that instruction encoding could use up if executed
		// need to know this to single step when there are interrupts in between
		// code block is not valid when this is zero
		// *note* this should be dynamically allocated
		//u64* MaxCycles;	// [ c_iNumBlocks ];
		
		u32 Local_LastModifiedReg;
		u32 Local_NextPCModified;
		
		u32 CurrentCount;
		
		u32 isBranchDelaySlot;
		u32 isLoadDelaySlot;
		
		//u32 bStopEncodingAfter;
		//u32 bStopEncodingBefore;
		
		// set the local cycle count to reset (start from zero) for the next cycle
		u32 bResetCycleCount;
		
		//inline void ResetFlags ( void ) { bStopEncodingBefore = false; bStopEncodingAfter = false; Local_NextPCModified = false; }

		
		// recompiler function
		// returns -1 if the instruction cannot be recompiled, otherwise returns the maximum number of cycles the instruction uses
		typedef int32_t (*Function) ( x64Encoder *e, VU *v, Vu::Instruction::Format Instruction, u32 Address );

		// *** todo *** do not recompile more than one instruction if currently in a branch delay slot or load delay slot!!
		u32 Recompile ( VU* v, u32 StartAddress );

		// new recompile code using static analysis data
		//u32 Recompile2 ( VU* v, u32 StartAddress );


		//void Invalidate ( u32 Address );
		void Invalidate ( u32 Address, u32 Count );
		
		u32 CloseOpLevel ( u32 OptLevel, u32 Address );

		bool isNopHi ( Vu::Instruction::Format i );
		bool isNopLo ( Vu::Instruction::Format i );
		
		
		u32 ulIndex_Mask;
		
		inline u32 Get_Block ( u32 Address ) { return ( Address >> ( 3 + MaxStep_Shift ) ) & NumBlocks_Mask; }
		inline u32 Get_Index ( u32 Address ) { return ( Address >> 3 ) & ulIndex_Mask; }

		//inline static u32 get_start_address(u32 PC) { return StartAddress[Get_Block(PC)]; }
		//inline static u32* get_source_code_ptr(u32 PC) { return &pSourceCode32[Get_Index(PC)]; }
		u8** get_code_start_ptr(u32 Address) { return &(pCodeStart[Get_Index(Address)]); }

		// the bitmap for if prefix is needed to check for delay when recompiling
		u64* pDelayPrefix64;
		void set_delay_bitmap(u32 Address) { pDelayPrefix64[Get_Index(Address) >> 6] = -1ull; }
		void clear_delay_bitmap(u32 Address) { pDelayPrefix64[Get_Index(Address) >> 6] = 0ull; }
		u64 get_delay_bitmap(u32 Address) { return pDelayPrefix64[Get_Index(Address) >> 6]; }
		void bitset_delay_bitmap(u32 Address) { pDelayPrefix64[Get_Index(Address) >> 6] |= (1ull << (Get_Index(Address) & 0x3f)); }
		bool bitget_delay_bitmap(u32 Address) { return ((pDelayPrefix64[Get_Index(Address) >> 6] & (1ull << (Get_Index(Address) & 0x3f))) != 0); }

		// bitmap for if the load at this instruction accesses hardware registers (vu0 only?)
		u64* pHWRWBitmap64;
		void set_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Index(Address) >> 6] = -1ull; }
		void clear_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Index(Address) >> 6] = 0ull; }
		u64 get_hwrw_bitmap(u32 Address) { return pHWRWBitmap64[Get_Index(Address) >> 6]; }
		void bitset_hwrw_bitmap(u32 Address) { pHWRWBitmap64[Get_Index(Address) >> 6] |= (1ull << (Get_Index(Address) & 0x3f)); }
		bool bitget_hwrw_bitmap(u32 Address) { return ((pHWRWBitmap64[Get_Index(Address) >> 6] & (1ull << (Get_Index(Address) & 0x3f))) != 0); }


			static const Function FunctionList [];
			
		// used by object to recompile an instruction into a code block
		// returns -1 if the instruction cannot be recompiled
		// returns 0 if the instruction was recompiled, but MUST start a new block for the next instruction (because it is guaranteed in a delay slot)
		// returns 1 if successful and can continue recompiling
		inline static int32_t RecompileHI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address ) { return Vu::Recompiler::FunctionList [ Vu::Instruction::Lookup::FindByInstructionHI ( i.Value ) ] ( e, v, i, Address ); }
		inline static int32_t RecompileLO ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address ) { return Vu::Recompiler::FunctionList [ Vu::Instruction::Lookup::FindByInstructionLO ( i.Value ) ] ( e, v, i, Address ); }
		
		
		inline void Run ( u32 Address ) { InstanceEncoder->ExecuteCodeBlock ( ( Address >> 2 ) & NumBlocks_Mask ); }


			static int32_t INVALID ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			

			static int32_t ADDBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t SUBBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MADDBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MSUBBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MAXBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MAXBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MAXBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MAXBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MINIBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MINIBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MINIBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MINIBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MULBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MULq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MAXi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MINIi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t OPMSUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			//static int32_t ADDi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t ADDABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MUL ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MAX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t OPMSUM ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MINI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ISUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IADDI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t CALLMS ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t CALLMSR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ITOF0 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FTOI0 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MOVE ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t LQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t DIV ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MTIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t RNEXT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ITOF4 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FTOI4 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ABS ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t MR32 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t SQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t SQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t MFIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t RGET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			//static int32_t ADDABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ADDABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t SUBABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MADDABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MSUBABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t ITOF12 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FTOI12 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MULABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t MULAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ADDAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SUBAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MULA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t OPMULA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t LQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t RSQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ILWR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t RINIT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ITOF15 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FTOI15 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t CLIP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MADDAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MSUBAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t NOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t SQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );


			// lower instructions

			
			static int32_t LQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ILW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ISW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IADDIU ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ISUBIU ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FCEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FCSET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FCAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FCOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FSEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FSSET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FSAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FSOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FMEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FMAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FMOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t FCGET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t B ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t BAL ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t JR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t JALR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBNE ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBLTZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBGTZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBLEZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t IBGEZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			
			//static int32_t DIV ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t EATANxy ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t EATANxz ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t EATAN ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t IADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t ISUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t IADDI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t IAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t IOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t MOVE ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t LQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t DIV ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			//static int32_t MTIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t RNEXT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MFP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t XTOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t XGKICK ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );

			static int32_t MR32 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t MFIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t RGET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			static int32_t XITOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ESADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t EATANxy ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ESQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ESIN ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ERSADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t EATANxz ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ERSQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t EATAN ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t LQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t RSQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ILWR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t RINIT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ELENG ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ESUM ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ERCPR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t EEXP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t SQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t WAITQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ISWR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t RXOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t ERLENG ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			static int32_t WAITP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address );
			
			
	};
};

#endif	// end #ifndef __VU_RECOMPILER_H__
