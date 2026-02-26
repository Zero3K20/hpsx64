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


// *** TODO ***
// 1. branches don't check to see if source register is being loaded currently for some reason
// 2. get rid of per cycle counting


#include "VU_Execute.h"
#include "VU_Recompiler.h"
//#include "ps2_system.h"
#include "VU_Print.h"
//#include "PS2DataBus.h"

using namespace Playstation2;
using namespace Vu;


// static vars
int Recompiler::iVectorType;
int Recompiler::iRecompilerWidth;


// avx/sse float mul/add

//#define ALLOW_AVX2_ARGS1
//#define ALLOW_AVX2_ARGS2

//#define ALLOW_AVX2_FTOIX2
//#define ALLOW_AVX2_ITOFX2

//#define ALLOW_AVX2_MINX2
//#define ALLOW_AVX2_MAXX2

//#define ALLOW_AVX2_MULX2
//#define ALLOW_AVX2_ADDX2

//#define ALLOW_AVX2_CLIPX2


//#define ALLOW_AVX2_FTOIX4
//#define ALLOW_AVX2_ITOFX4
//#define ALLOW_AVX2_MINX4
//#define ALLOW_AVX2_MAXX4

//#define ALLOW_AVX2_MULX4
//#define ALLOW_AVX2_ADDX4




// only read cyclecount once when determining delays for destination register(s)
#define WAIT_DST_READ_CYCLECOUNT_ONCE

// only read cyclecount once when determining delays for source register(s)
#define WAIT_SRC_READ_CYCLECOUNT_ONCE

// only write cyclecount once when determining delays for source register(s)
#define WAIT_SRC_WRITE_CYCLECOUNT_ONCE


// if instruction uses all components store them all at once
#define ENABLE_FDST_FAST_STORE_ALL_COMPONENTS


// force hi-code to oplevel 1 even if lo-code is at oplevel 0
//#define FORCE_HI_CODE_TO_OPLEVEL_1


// try to simulate float multiplication inaccuracy on ps2
#define ENABLE_MUL_INACCURACY_AVX2
//#define ENABLE_MUL_INACCURACY_AVX512


#define USE_NEW_VECTOR_DISPATCH_VMUL_VU
#define USE_NEW_VECTOR_DISPATCH_VADD_VU
#define USE_NEW_VECTOR_DISPATCH_VMADD_VU
#define USE_NEW_VECTOR_DISPATCH_VABS_VU
#define USE_NEW_VECTOR_DISPATCH_VMAX_VU
#define USE_NEW_VECTOR_DISPATCH_VMIN_VU

#define USE_NEW_VECTOR_DISPATCH_FTOIX_VU
#define USE_NEW_VECTOR_DISPATCH_ITOFX_VU


#define USE_NEW_VECTOR_DISPATCH_LQ_VU
#define USE_NEW_VECTOR_DISPATCH_LQI_VU
#define USE_NEW_VECTOR_DISPATCH_LQD_VU



#define ENABLE_AVX2_FTOIX
#define ENABLE_AVX2_ITOFX

#define ENABLE_AVX2_MIN
#define ENABLE_AVX2_MAX

#define ALLOW_AVX2_MULX1
#define ALLOW_AVX2_ADDX1
#define ALLOW_AVX2_MADDX1

#define USE_NEW_VMULX1_AVX2
#define USE_NEW_VADDX1_AVX2
#define USE_NEW_VMADDX1_AVX2

#define USE_NEW_FTOIX_AVX2_VU
#define USE_NEW_ITOFX_AVX2_VU


// try not to return when branching with vu recompiler
//#define ENABLE_VU_AUTO_BRANCH


// skip storing dst bitmap into circular buffer if there is no possible read-after-write hazard
// note: the hazards for the branches/jumps were removed, so need another solution there for this
#define SKIP_MISSING_HAZARDS_RAW


// skip flag checks that don't appear to be needed
//#define SKIP_EXTRA_FLAG_CHECKS
//#define SKIP_EXTRA_FLAG_CHECKS_STAT
//#define SKIP_EXTRA_FLAG_CHECKS_MAC
//#define SKIP_EXTRA_FLAG_CHECKS_CLIP



//#define INLINE_DEBUG_DURING_STATIC_ANALYSIS
//#define INLINE_DEBUG_RECOMPILE2

//#define REQUIRE_PARALLEL_NOP


//#define ENABLE_PARALLEL_FTOI0_MARKx3
//#define ENABLE_PARALLEL_FTOI4_MARKx3
//#define ENABLE_PARALLEL_FTOI12_MARKx3
//#define ENABLE_PARALLEL_FTOI15_MARKx3


//#define ENABLE_PARALLEL_ITOF0_MARKx3
//#define ENABLE_PARALLEL_ITOF4_MARKx3
//#define ENABLE_PARALLEL_ITOF12_MARKx3
//#define ENABLE_PARALLEL_ITOF15_MARKx3


//#define ENABLE_PARALLEL_MIN_MARKx3
//#define ENABLE_PARALLEL_MAX_MARKx3


//#define ENABLE_PARALLEL_MUL_MARKx3
//#define ENABLE_PARALLEL_ADD_MARKx3


//#define ENABLE_PARALLEL_FTOI0_MARK
//#define ENABLE_PARALLEL_FTOI4_MARK
//#define ENABLE_PARALLEL_FTOI12_MARK
//#define ENABLE_PARALLEL_FTOI15_MARK

//#define ENABLE_PARALLEL_ITOF0_MARK
//#define ENABLE_PARALLEL_ITOF4_MARK
//#define ENABLE_PARALLEL_ITOF12_MARK
//#define ENABLE_PARALLEL_ITOF15_MARK

// enable marking min/max for parallel execution
//#define ENABLE_PARALLEL_MIN_MARK
//#define ENABLE_PARALLEL_MAX_MARK


//#define ENABLE_PARALLEL_MUL_MARK
//#define ENABLE_PARALLEL_ADD_MARK
//#define ENABLE_PARALLEL_ATYPE_MARK
//#define ENABLE_PARALLEL_MTYPE_MARK
//#define ENABLE_PARALLEL_CLIP_MARK


//#define ENABLE_PARALLEL_FTOI0_EXECUTE
//#define ENABLE_PARALLEL_FTOI4_EXECUTE
//#define ENABLE_PARALLEL_FTOI12_EXECUTE
//#define ENABLE_PARALLEL_FTOI15_EXECUTE

//#define ENABLE_PARALLEL_ITOF0_EXECUTE
//#define ENABLE_PARALLEL_ITOF4_EXECUTE
//#define ENABLE_PARALLEL_ITOF12_EXECUTE
//#define ENABLE_PARALLEL_ITOF15_EXECUTE

// enable actualling doing min/max parallel execution
//#define ENABLE_PARALLEL_MIN_EXECUTE
//#define ENABLE_PARALLEL_MAX_EXECUTE


//#define ENABLE_PARALLEL_MUL_EXECUTE
//#define ENABLE_PARALLEL_ADD_EXECUTE
//#define ENABLE_PARALLEL_ATYPE_EXECUTE
//#define ENABLE_PARALLEL_MTYPE_EXECUTE
//#define ENABLE_PARALLEL_CLIP_EXECUTE

//#define ENABLE_PARALLEL_OPMULA_EXECUTE
//#define ENABLE_PARALLEL_MULQ_EXECUTE


// debug vu code and static analysis of it
//#define INLINE_DEBUG_PRINT_STATIC_ANALYSIS


#define USE_NEW_RECOMPILE2
#define USE_NEW_RECOMPILE2_EXEORDER
#define USE_NEW_RECOMPILE2_IGNORE_LOWER

// enable this if CycleCount is being updated all at once instead of per cycle
//#define BATCH_UPDATE_CYCLECOUNT



//#define DISABLE_LQD_VU0
//#define DISABLE_LQI_VU0
//#define DISABLE_LQ_VU0
//#define DISABLE_SQD_VU0
//#define DISABLE_SQI_VU0
//#define DISABLE_SQ_VU0
//#define DISABLE_ILW_VU0
//#define DISABLE_ILWR_VU0
//#define DISABLE_ISW_VU0
//#define DISABLE_ISWR_VU0





//#define ENABLE_VFLAGS


#define ENABLE_RECOMPILER_BITMAP		
#define ENABLE_SETDSTBITMAP
#define RECOMPILE_SETDSTBITMAP
#define ENABLE_RECOMPILER_ADVANCE_CYCLE



//#define OPTIMIZE_RO_MULTIPLY_MUL
#define OPTIMIZE_RO_MULTIPLY_MADD

#define USE_NEW_VMUL_CODE
#define USE_NEW_VMADD_CODE



#define ENABLE_INTDELAYSLOT_SQI
#define ENABLE_INTDELAYSLOT_SQD
#define ENABLE_INTDELAYSLOT_LQI
#define ENABLE_INTDELAYSLOT_LQD



#define ENABLE_BITMAP_NOP


#define ENABLE_BITMAP_ABS

#define ENABLE_BITMAP_MAX
#define ENABLE_BITMAP_MAXi
#define ENABLE_BITMAP_MAXX
#define ENABLE_BITMAP_MAXY
#define ENABLE_BITMAP_MAXZ
#define ENABLE_BITMAP_MAXW

#define ENABLE_BITMAP_MINI
#define ENABLE_BITMAP_MINIi
#define ENABLE_BITMAP_MINIX
#define ENABLE_BITMAP_MINIY
#define ENABLE_BITMAP_MINIZ
#define ENABLE_BITMAP_MINIW


// problem
#define ENABLE_BITMAP_FTOI0
#define ENABLE_BITMAP_FTOI4


#define ENABLE_BITMAP_FTOI12
#define ENABLE_BITMAP_FTOI15


#define ENABLE_BITMAP_ITOF0
#define ENABLE_BITMAP_ITOF4
#define ENABLE_BITMAP_ITOF12
#define ENABLE_BITMAP_ITOF15



#define ENABLE_BITMAP_MOVE
#define ENABLE_BITMAP_MR32

#define ENABLE_BITMAP_MFP



#define ENABLE_BITMAP_IADD
#define ENABLE_BITMAP_IADDI
#define ENABLE_BITMAP_IADDIU
#define ENABLE_BITMAP_IAND
#define ENABLE_BITMAP_IOR
#define ENABLE_BITMAP_ISUB
#define ENABLE_BITMAP_ISUBIU



#define ENABLE_BITMAP_LQ
#define ENABLE_BITMAP_LQI
#define ENABLE_BITMAP_LQD
#define ENABLE_BITMAP_ILW
#define ENABLE_BITMAP_ILWR
#define ENABLE_BITMAP_SQ
#define ENABLE_BITMAP_SQI
#define ENABLE_BITMAP_SQD
#define ENABLE_BITMAP_ISW
#define ENABLE_BITMAP_ISWR


#define ENABLE_BITMAP_MFIR
#define ENABLE_BITMAP_MTIR



#define ENABLE_BITMAP_DIV
#define ENABLE_BITMAP_RSQRT
#define ENABLE_BITMAP_SQRT


#define ENABLE_BITMAP_XGKICK



#define ENABLE_BITMAP_RGET
#define ENABLE_BITMAP_RINIT
#define ENABLE_BITMAP_RNEXT
#define ENABLE_BITMAP_RXOR



#define ENABLE_BITMAP_EATAN
#define ENABLE_BITMAP_EATANxy
#define ENABLE_BITMAP_EATANxz
#define ENABLE_BITMAP_EEXP
#define ENABLE_BITMAP_ELENG
#define ENABLE_BITMAP_ERCPR
#define ENABLE_BITMAP_ERLENG
#define ENABLE_BITMAP_ERSADD
#define ENABLE_BITMAP_ERSQRT
#define ENABLE_BITMAP_ESADD
#define ENABLE_BITMAP_ESIN
#define ENABLE_BITMAP_ESQRT
#define ENABLE_BITMAP_ESUM




#define ENABLE_BITMAP_CLIP


#define ENABLE_BITMAP_ADD
#define ENABLE_BITMAP_ADDi
#define ENABLE_BITMAP_ADDq
#define ENABLE_BITMAP_ADDX
#define ENABLE_BITMAP_ADDY
#define ENABLE_BITMAP_ADDZ
#define ENABLE_BITMAP_ADDW



#define ENABLE_BITMAP_ADDA
#define ENABLE_BITMAP_ADDAi
#define ENABLE_BITMAP_ADDAq
#define ENABLE_BITMAP_ADDAX
#define ENABLE_BITMAP_ADDAY
#define ENABLE_BITMAP_ADDAZ
#define ENABLE_BITMAP_ADDAW


#define ENABLE_BITMAP_SUB
#define ENABLE_BITMAP_SUBi
#define ENABLE_BITMAP_SUBq
#define ENABLE_BITMAP_SUBX
#define ENABLE_BITMAP_SUBY
#define ENABLE_BITMAP_SUBZ
#define ENABLE_BITMAP_SUBW


#define ENABLE_BITMAP_SUBA
#define ENABLE_BITMAP_SUBAi
#define ENABLE_BITMAP_SUBAq
#define ENABLE_BITMAP_SUBAX
#define ENABLE_BITMAP_SUBAY
#define ENABLE_BITMAP_SUBAZ
#define ENABLE_BITMAP_SUBAW



#define ENABLE_BITMAP_MUL
#define ENABLE_BITMAP_MULi
#define ENABLE_BITMAP_MULq
#define ENABLE_BITMAP_MULX
#define ENABLE_BITMAP_MULY
#define ENABLE_BITMAP_MULZ
#define ENABLE_BITMAP_MULW


#define ENABLE_BITMAP_MULA
#define ENABLE_BITMAP_MULAi
#define ENABLE_BITMAP_MULAq
#define ENABLE_BITMAP_MULAX
#define ENABLE_BITMAP_MULAY
#define ENABLE_BITMAP_MULAZ
#define ENABLE_BITMAP_MULAW



#define ENABLE_BITMAP_MADD
#define ENABLE_BITMAP_MADDi
#define ENABLE_BITMAP_MADDq
#define ENABLE_BITMAP_MADDX
#define ENABLE_BITMAP_MADDY
#define ENABLE_BITMAP_MADDZ
#define ENABLE_BITMAP_MADDW



#define ENABLE_BITMAP_MADDA
#define ENABLE_BITMAP_MADDAi
#define ENABLE_BITMAP_MADDAq
#define ENABLE_BITMAP_MADDAX
#define ENABLE_BITMAP_MADDAY
#define ENABLE_BITMAP_MADDAZ
#define ENABLE_BITMAP_MADDAW


#define ENABLE_BITMAP_MSUB
#define ENABLE_BITMAP_MSUBi
#define ENABLE_BITMAP_MSUBq
#define ENABLE_BITMAP_MSUBX
#define ENABLE_BITMAP_MSUBY
#define ENABLE_BITMAP_MSUBZ
#define ENABLE_BITMAP_MSUBW


#define ENABLE_BITMAP_MSUBA
#define ENABLE_BITMAP_MSUBAi
#define ENABLE_BITMAP_MSUBAq
#define ENABLE_BITMAP_MSUBAX
#define ENABLE_BITMAP_MSUBAY
#define ENABLE_BITMAP_MSUBAZ
#define ENABLE_BITMAP_MSUBAW


#define ENABLE_BITMAP_OPMSUB
#define ENABLE_BITMAP_OPMULA




// -----------------------------


#define USE_NEW_NOP_RECOMPILE


#define USE_NEW_ABS_RECOMPILE

#define USE_NEW_MAX_RECOMPILE
#define USE_NEW_MIN_RECOMPILE


#define USE_NEW_FTOI0_RECOMPILE
#define USE_NEW_FTOI4_RECOMPILE
#define USE_NEW_FTOI12_RECOMPILE
#define USE_NEW_FTOI15_RECOMPILE


#define USE_NEW_ITOF0_RECOMPILE
#define USE_NEW_ITOF4_RECOMPILE
#define USE_NEW_ITOF12_RECOMPILE
#define USE_NEW_ITOF15_RECOMPILE





// MACRO MODE R5900 ONLY
//#define USE_NEW_CFC2_NI_RECOMPILE
//#define USE_NEW_CTC2_NI_RECOMPILE
//#define USE_NEW_QMFC2_NI_RECOMPILE
//#define USE_NEW_QMTC2_NI_RECOMPILE





#define USE_NEW_CLIP_RECOMPILE



#define USE_NEW_ADD_RECOMPILE
#define USE_NEW_ADDi_RECOMPILE
#define USE_NEW_ADDq_RECOMPILE
#define USE_NEW_ADDX_RECOMPILE


#define USE_NEW_ADDY_RECOMPILE


#define USE_NEW_ADDZ_RECOMPILE
#define USE_NEW_ADDW_RECOMPILE



#define USE_NEW_ADDA_RECOMPILE
#define USE_NEW_ADDAi_RECOMPILE
#define USE_NEW_ADDAq_RECOMPILE
#define USE_NEW_ADDAX_RECOMPILE
#define USE_NEW_ADDAY_RECOMPILE
#define USE_NEW_ADDAZ_RECOMPILE
#define USE_NEW_ADDAW_RECOMPILE



#define USE_NEW_SUB_RECOMPILE
#define USE_NEW_SUBi_RECOMPILE
#define USE_NEW_SUBq_RECOMPILE
#define USE_NEW_SUBX_RECOMPILE
#define USE_NEW_SUBY_RECOMPILE
#define USE_NEW_SUBZ_RECOMPILE
#define USE_NEW_SUBW_RECOMPILE


#define USE_NEW_SUBA_RECOMPILE
#define USE_NEW_SUBAi_RECOMPILE
#define USE_NEW_SUBAq_RECOMPILE
#define USE_NEW_SUBAX_RECOMPILE
#define USE_NEW_SUBAY_RECOMPILE
#define USE_NEW_SUBAZ_RECOMPILE
#define USE_NEW_SUBAW_RECOMPILE



#define USE_NEW_MUL_RECOMPILE
#define USE_NEW_MULi_RECOMPILE
#define USE_NEW_MULq_RECOMPILE
#define USE_NEW_MULX_RECOMPILE
#define USE_NEW_MULY_RECOMPILE
#define USE_NEW_MULZ_RECOMPILE
#define USE_NEW_MULW_RECOMPILE


#define USE_NEW_MULA_RECOMPILE
#define USE_NEW_MULAi_RECOMPILE
#define USE_NEW_MULAq_RECOMPILE

#define USE_NEW_MULAX_RECOMPILE
#define USE_NEW_MULAY_RECOMPILE
#define USE_NEW_MULAZ_RECOMPILE
#define USE_NEW_MULAW_RECOMPILE



#define USE_NEW_MADD_RECOMPILE
#define USE_NEW_MADDi_RECOMPILE
#define USE_NEW_MADDq_RECOMPILE
#define USE_NEW_MADDX_RECOMPILE
#define USE_NEW_MADDY_RECOMPILE
#define USE_NEW_MADDZ_RECOMPILE
#define USE_NEW_MADDW_RECOMPILE



#define USE_NEW_MADDA_RECOMPILE
#define USE_NEW_MADDAi_RECOMPILE
#define USE_NEW_MADDAq_RECOMPILE
#define USE_NEW_MADDAX_RECOMPILE
#define USE_NEW_MADDAY_RECOMPILE
#define USE_NEW_MADDAZ_RECOMPILE
#define USE_NEW_MADDAW_RECOMPILE


#define USE_NEW_MSUB_RECOMPILE
#define USE_NEW_MSUBi_RECOMPILE
#define USE_NEW_MSUBq_RECOMPILE
#define USE_NEW_MSUBX_RECOMPILE
#define USE_NEW_MSUBY_RECOMPILE
#define USE_NEW_MSUBZ_RECOMPILE
#define USE_NEW_MSUBW_RECOMPILE


#define USE_NEW_MSUBA_RECOMPILE
#define USE_NEW_MSUBAi_RECOMPILE
#define USE_NEW_MSUBAq_RECOMPILE
#define USE_NEW_MSUBAX_RECOMPILE
#define USE_NEW_MSUBAY_RECOMPILE
#define USE_NEW_MSUBAZ_RECOMPILE
#define USE_NEW_MSUBAW_RECOMPILE


#define USE_NEW_OPMSUB_RECOMPILE
#define USE_NEW_OPMULA_RECOMPILE




#define USE_NEW_MOVE_RECOMPILE
#define USE_NEW_MR32_RECOMPILE


#define USE_NEW_MFP_RECOMPILE


#define USE_NEW_IADD_RECOMPILE
#define USE_NEW_IADDI_RECOMPILE
#define USE_NEW_IAND_RECOMPILE
#define USE_NEW_IOR_RECOMPILE
#define USE_NEW_ISUB_RECOMPILE

#define USE_NEW_IADDIU_RECOMPILE
#define USE_NEW_ISUBIU_RECOMPILE


// these deal with the integer registers that have delay type slots
// needs work
#define USE_NEW_MFIR_RECOMPILE
#define USE_NEW_MTIR_RECOMPILE



// needs work
#define USE_NEW_DIV_RECOMPILE
#define USE_NEW_RSQRT_RECOMPILE
#define USE_NEW_SQRT_RECOMPILE
#define USE_NEW_WAITQ_RECOMPILE



#define USE_NEW_RXOR_RECOMPILE
#define USE_NEW_RGET_RECOMPILE
#define USE_NEW_RINIT_RECOMPILE
#define USE_NEW_RNEXT_RECOMPILE


#define USE_NEW_XTOP_RECOMPILE
#define USE_NEW_XITOP_RECOMPILE




#define USE_NEW_FCSET_RECOMPILE
#define USE_NEW_FSSET_RECOMPILE


#define USE_NEW_FCGET_RECOMPILE
#define USE_NEW_FCAND_RECOMPILE
#define USE_NEW_FCEQ_RECOMPILE
#define USE_NEW_FCOR_RECOMPILE


#define USE_NEW_FMAND_RECOMPILE
#define USE_NEW_FMEQ_RECOMPILE
#define USE_NEW_FMOR_RECOMPILE


#define USE_NEW_FSAND_RECOMPILE
#define USE_NEW_FSEQ_RECOMPILE
#define USE_NEW_FSOR_RECOMPILE



#define USE_NEW_LQ_RECOMPILE
#define USE_NEW_LQI_RECOMPILE
#define USE_NEW_LQD_RECOMPILE


#define USE_NEW_ILW_RECOMPILE
#define USE_NEW_ILWR_RECOMPILE


#define USE_NEW_SQ_RECOMPILE
#define USE_NEW_SQI_RECOMPILE
#define USE_NEW_SQD_RECOMPILE

#define USE_NEW_ISW_RECOMPILE
#define USE_NEW_ISWR_RECOMPILE


#define USE_NEW_B_RECOMPILE
#define USE_NEW_BAL_RECOMPILE
#define USE_NEW_JALR_RECOMPILE
#define USE_NEW_JR_RECOMPILE


#define USE_NEW_IBEQ_RECOMPILE
#define USE_NEW_IBNE_RECOMPILE
#define USE_NEW_IBLTZ_RECOMPILE
#define USE_NEW_IBLEZ_RECOMPILE
#define USE_NEW_IBGTZ_RECOMPILE
#define USE_NEW_IBGEZ_RECOMPILE



#define USE_NEW_EATAN_RECOMPILE
#define USE_NEW_EATANXY_RECOMPILE
#define USE_NEW_EATANXZ_RECOMPILE
#define USE_NEW_EEXP_RECOMPILE
#define USE_NEW_ESIN_RECOMPILE
#define USE_NEW_ERSQRT_RECOMPILE
#define USE_NEW_ERCPR_RECOMPILE
#define USE_NEW_ESQRT_RECOMPILE
#define USE_NEW_ESADD_RECOMPILE
#define USE_NEW_ERSADD_RECOMPILE
#define USE_NEW_ESUM_RECOMPILE
#define USE_NEW_ELENG_RECOMPILE
#define USE_NEW_ERLENG_RECOMPILE



#define CHECK_EVENT_AFTER_START



#define ENABLE_BRANCH_DELAY_RECOMPILE
#define ENABLE_EBIT_RECOMPILE
#define ENABLE_MBIT_RECOMPILE


#define ENABLE_NOP_HI
#define ENABLE_NOP_LO



// *** remove this when done testing ***
//#define ENABLE_SINGLE_STEP
//#define ENABLE_SINGLE_STEP_BEFORE



#define CACHE_NOT_IMPLEMENTED


// test pc arg pass, new methodology etc
//#define TEST_NEW_RECOMPILE



#define ENCODE_SINGLE_RUN_PER_BLOCK


#define UPDATE_BEFORE_RETURN


// crashes unless you do this ?? Compiler dependent?
#define RESERVE_STACK_FRAME_FOR_CALL


//#define ENABLE_AUTONOMOUS_BRANCH_U
//#define ENABLE_AUTONOMOUS_BRANCH_C


//#define VERBOSE_RECOMPILE
//#define VERBOSE_RECOMPILE_MBIT


Vu::Recompiler *Vu::Recompiler::_REC;






char Recompiler::reverse_bytes_lut128[16] = { 12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3 };



// constructor
// NumberOfBlocks MUST be a power of 2, so 1 would mean 2, 2 would mean 4
Recompiler::Recompiler ( VU* v, u32 NumberOfBlocks, u32 BlockSize_PowerOfTwo, u32 MaxIStep_Shift )
{
	
	BlockSize = 1 << BlockSize_PowerOfTwo;
	
	MaxStep_Shift = MaxIStep_Shift;
	MaxStep = 1 << MaxIStep_Shift;
	MaxStep_Mask = MaxStep - 1;
	
	NumBlocks = 1 << NumberOfBlocks;
	NumBlocks_Mask = NumBlocks - 1;
	
	// need a mask for referencing each encoded instruction
	ulIndex_Mask = 1 << ( NumberOfBlocks + MaxIStep_Shift );
	ulIndex_Mask -= 1;
	
	// allocate variables
	//StartAddress = new u32 [ NumBlocks ];
	//RunCount = new u8 [ NumBlocks ];
	//MaxCycles = new u64 [ NumBlocks ];
	//Instructions = new u32 [ NumBlocks * MaxStep ];
	
	// only need to compare the starting address of the entire block
	StartAddress = new u32 [ NumBlocks ];
	pCodeStart = new u8* [ NumBlocks * MaxStep ];
	CycleCount = new u32 [ NumBlocks * MaxStep ];
	//EndAddress = new u32 [ NumBlocks * MaxStep ];

	pDelayPrefix64 = new u64[(NumBlocks * MaxStep) >> 6];
	pHWRWBitmap64 = new u64[(NumBlocks * MaxStep) >> 6];

	pForwardBranchTargets = new u32 [ MaxStep ];
	
	// used internally by recompiler (in case it branches to a load/store or another branch, etc, then need to go to prefix instead)
	pPrefix_CodeStart = new u8* [ MaxStep ];
	
	
	// create the encoder
	//e = new x64Encoder ( BlockSize_PowerOfTwo, NumBlocks );
	InstanceEncoder = new x64Encoder ( BlockSize_PowerOfTwo, NumBlocks );
	
	e = InstanceEncoder;
	
	
	Reset ();
}


// destructor
Recompiler::~Recompiler ()
{
	delete e;
	
	delete StartAddress;
	
	delete pPrefix_CodeStart;
	delete pCodeStart;
	delete CycleCount;
	delete pForwardBranchTargets;

	delete pDelayPrefix64;
	delete pHWRWBitmap64;

}


void Recompiler::Reset ()
{
	//memset ( this, 0, sizeof( Recompiler ) );	
	// initialize the address and instruction so it is known that it does not refer to anything
	memset ( pForwardBranchTargets, 0x00, sizeof( u32 ) * MaxStep );
	memset ( pPrefix_CodeStart, 0x00, sizeof( u8* ) * MaxStep );
	memset ( StartAddress, 0xff, sizeof( u32 ) * NumBlocks );
	memset ( pCodeStart, 0x00, sizeof( u8* ) * NumBlocks * MaxStep );
	memset ( CycleCount, 0x00, sizeof( u32 ) * NumBlocks * MaxStep );

	memset(pDelayPrefix64, 0xff, (sizeof(u64) * NumBlocks * MaxStep) >> 6);

	// here, testing with setting to use the optimized code first
	memset(pHWRWBitmap64, 0x00, (sizeof(u64) * NumBlocks * MaxStep) >> 6);
	//memset(pHWRWBitmap64, 0xff, (sizeof(u64) * NumBlocks * MaxStep) >> 6);

#ifdef ENABLE_ICACHE
	// reset invalidate arrays
	r->Bus->Reset_Invalidate ();
#endif
}


/** 
 * @fn static void Calc_Checksum( VU *v )
 * @brief calculate checksum for source of recompiled code and store into "ullChecksum"
 * @param v is a pointer into the VU object state that holds the source code that was recompiled
 * @return 64-bit checksum calculated from source vu code mem at the current point in time
 */
u64 Recompiler::Calc_Checksum( VU *v )
{
	u32* pSrcPtr32;
	u64 ullAddress;
	u64 ullCode;
	u64 ullCurChecksum;

	u64 ullLoopCount;

	// get pointer into the vu memory starting from beginning
	pSrcPtr32 = RGetPointer ( v, 0 );

	// init checksum
	ullCurChecksum = 0;

	// determine if this is vu0 or vu1
	// get the size of vu code mem based vu number etc
	ullLoopCount = v->ulVuMem_Size >> 2;

	// calculate the check sum
	for ( ullAddress = 0; ullAddress < ullLoopCount; ullAddress++ )
	{
		// get the source code value
		ullCode = (u64) ( *pSrcPtr32++ );

		// multiply by the address
		ullCode *= ( ullAddress + 1 );

		ullCurChecksum += ullCode;
	}

	// go ahead and return check sum
	return ullCurChecksum;
}


int32_t Recompiler::Dispatch_Result_AVX2(x64Encoder* e, VU* v, Vu::Instruction::Format i, int32_t Accum, int32_t sseSrcReg, int32_t VUDestReg)
{
	int32_t ret = 1;

	if (i.xyzw)
	{
		if (Accum)
		{
			if (i.xyzw == 0xf)
			{
				ret = e->movdqa1_memreg(&v->dACC[0].l, sseSrcReg);
			}
			else
			{
				if ((i.xyzw & 0xc) == 0xc)
				{
					ret = e->movq1_memreg((int64_t*)&v->dACC[0].l, sseSrcReg);
				}
				else if ((i.xyzw & 0xc) == 0x8)
				{
					ret = e->movd1_memreg((int32_t*)&v->dACC[0].l, sseSrcReg);
				}
				else if ((i.xyzw & 0xc) == 0x4)
				{
					ret = e->pextrd1memreg((int32_t*)&v->dACC[1].l, sseSrcReg, 1);
				}

				if ((i.xyzw & 0x3) == 0x3)
				{
					ret = e->pextrq1memreg((int64_t*)&v->dACC[2].l, sseSrcReg, 1);
				}
				else if ((i.xyzw & 0x3) == 0x2)
				{
					ret = e->pextrd1memreg((int32_t*)&v->dACC[2].l, sseSrcReg, 2);
				}
				else if ((i.xyzw & 0x3) == 0x1)
				{
					ret = e->pextrd1memreg((int32_t*)&v->dACC[3].l, sseSrcReg, 3);
				}

			}	// end if (i.xyzw == 0xf) else

		}
		else
		{
			if (VUDestReg)
			{
				if (i.xyzw == 0xf)
				{
					ret = e->movdqa1_memreg(&v->vf[VUDestReg].sw0, sseSrcReg);
				}
				else
				{
					if ((i.xyzw & 0xc) == 0xc)
					{
						ret = e->movq1_memreg((int64_t*)&v->vf[VUDestReg].sw0, sseSrcReg);
					}
					else if ((i.xyzw & 0xc) == 0x8)
					{
						ret = e->movd1_memreg((int32_t*)&v->vf[VUDestReg].sw0, sseSrcReg);
					}
					else if ((i.xyzw & 0xc) == 0x4)
					{
						ret = e->pextrd1memreg((int32_t*)&v->vf[VUDestReg].sw1, sseSrcReg, 1);
					}

					if ((i.xyzw & 0x3) == 0x3)
					{
						ret = e->pextrq1memreg((int64_t*)&v->vf[VUDestReg].sq1, sseSrcReg, 1);
					}
					else if ((i.xyzw & 0x3) == 0x2)
					{
						ret = e->pextrd1memreg((int32_t*)&v->vf[VUDestReg].sw2, sseSrcReg, 2);
					}
					else if ((i.xyzw & 0x3) == 0x1)
					{
						ret = e->pextrd1memreg((int32_t*)&v->vf[VUDestReg].sw3, sseSrcReg, 3);
					}

				}	// end if (i.xyzw == 0xf) else

			}	// end if (VUDestReg)

		}	// end if (Accum) else

	}	// end if (i.xyzw)

	return ret;
}



bool Recompiler::Recompile_AdvanceCycle(VU* v, u32 Address)
{
#ifdef ENABLE_RECOMPILER_ADVANCE_CYCLE

	AdvanceCycle_rec(e, v, Address);

#else

#ifdef RESERVE_STACK_FRAME_FOR_CALL
	e->SubReg64ImmX(RSP, c_lSEH_StackSize);
#endif

	e->LeaRegMem64(RCX, v);
	e->Call((void*)AdvanceCycle);

#ifdef RESERVE_STACK_FRAME_FOR_CALL
	ret = e->AddReg64ImmX(RSP, c_lSEH_StackSize);
#endif

#endif	// end #ifdef+else ENABLE_RECOMPILER_ADVANCE_CYCLE

	return true;
}

// returns 1 if it is ok to have instruction in branch delay slot when recompiling, zero otherwise
bool Recompiler::isBranchDelayOk ( u32 ulInstruction, u32 Address )
{
#ifdef ENCODE_ALL_POSSIBLE_DELAYSLOTS

	u32 ulOpcode, ulFunction;
	
	ulOpcode = ulInstruction >> 26;
	
	
	// second row starting at second column is ok
	if ( ulOpcode >= 0x9 && ulOpcode <= 0xf )
	{

		// constant instructions that will never interrupt //
		//cout << "\nAddress=" << hex << Address << " Opcode=" << ulOpcode;
		return true;
	}
	
	
	
	// for R5900, DADDIU is ok //
	if ( ulOpcode == 0x19 )
	{
		return true;
	}
	
	
	
	// check special instructions
	if ( !ulOpcode )
	{
		ulFunction = ulInstruction & 0x3f;
		
		// first row is mostly ok
		if ( ( ( ulFunction >> 3 ) == 0 ) && ( ulFunction != 1 ) && ( ulFunction != 5 ) )
		{
			return true;
		}
		
		// row 4 is ok except for column 0 and column 2
		if ( ( ulFunction >> 3 ) == 4 && ulFunction != 0x20 && ulFunction != 0x22 )
		{
			return true;
		}
		
		// in row 5 for R3000A columns 2 and 3 are ok
		if ( ulFunction == 0x2a || ulFunction == 0x2b )
		{
			return true;
		}
		
		
		// in row 5 for R5900 columns 5 and 7 are ok //
		if ( ulFunction == 0x2d || ulFunction == 0x2f )
		{
			return true;
		}
		
		
		// for R5900, on row 2 MOVZ and MOVN are ok //
		if ( ulFunction == 0xa || ulFunction == 0xb )
		{
			return true;
		}
		
		
		// for R5900, on row 3 DSLLV, DSRLV, DSRAV or ok //
		if ( ulFunction == 0x14 || ulFunction == 0x16 || ulFunction == 0x17 )
		{
			return true;
		}
		
		// in last row for R5900, all is ok except columns 1 and 5 //
		if ( ( ulFunction >> 3 ) == 7 && ulFunction != 0x39 && ulFunction != 0x3d )
		{
			return true;
		}
		
	}
	
	// will leave out all store instructions for now to play it safe //
	
#else
	if ( !ulInstruction ) return true;
#endif
	
	return false;
}




// force pipeline to wait for the Q register
//void VU::PipelineWaitQ ()
//{
//	PipelineWaitCycle ( QBusyUntil_Cycle );
//	if ( QBusyUntil_Cycle != -1LL )
//	{
//		SetQ ();
//	}
//}

void Recompiler::PipelineWaitQ ( VU* v )
{
	if ( v->CycleCount < v->QBusyUntil_Cycle )
	{
		v->PipelineWaitCycle ( v->QBusyUntil_Cycle );
	}
	
	if ( v->QBusyUntil_Cycle != -1LL )
	{
		v->SetQ ();
	}
}


//inline u64 TestStall ()
//{
//	return TestBitmap ( Pipeline_Bitmap, SrcRegs_Bitmap );
//}

//inline u64 TestStall_INT ()
//{
//	// test integer pipeline for a stall (like if the integer register is not loaded yet)
//	return Int_Pipeline_Bitmap & Int_SrcRegs_Bitmap;
//}


void Recompiler::TestStall ( VU* v )
{
	if ( v->TestStall () )
	{
		// FMAC pipeline stall //
		v->PipelineWait_FMAC ();
	}
}


void Recompiler::TestStall_INT ( VU* v )
{
	if ( v->TestStall_INT () )
	{
		// Integer pipeline stall //
		v->PipelineWait_INT ();
	}
}


bool Recompiler::isNopHi ( Vu::Instruction::Format i )
{
#ifdef ENABLE_NOP_HI
	// check for NOP instruction
	if ( ( i.Funct == 0x3f ) && ( i.Fd == 0xb ) )
	{
		return true;
	}
#endif
	
	return false;
}


bool Recompiler::isNopLo ( Vu::Instruction::Format i )
{
#ifdef ENABLE_NOP_LO
	// check for mov with no xyzw
	if ( i.Opcode == 0x40 )
	{
		if ( ( i.Funct == 0x3c ) && ( i.Fd == 0xc ) )
		{
			// mov instruction //
			
			// if no xyzw is getting moved or if destination is zero register then is NOP
			// note: could be a dependency even if moving to zero register
			//if ( !i.xyzw || !i.Ft )
			if (!i.xyzw)
			{
				return true;
			}
		}
	}
#endif
	
	return false;
}




//inline void UpdateQ ()
//{
//	if ( CycleCount >= QBusyUntil_Cycle )
//	{
//		// set the q register
//		SetQ ();
//	}
//}

//inline void UpdateP ()
//{
//	if ( CycleCount == PBusyUntil_Cycle )
//	{
//		// set the p register
//		SetP ();
//	}
//}


//inline void SetQ ()
//{
//	// set the Q register
//	vi [ REG_Q ].s = NextQ.l;
//	
//	// clear non-sticky div unit flags
//	vi [ REG_STATUSFLAG ].uLo &= ~0x30;
//	
//	// set flags
//	vi [ REG_STATUSFLAG ].uLo |= NextQ_Flag;
//	
//	// don't set the Q register again until div unit is used again
//	// should clear to zero to indicate last event happened far in the past
//	QBusyUntil_Cycle = -1LL;
//	//QBusyUntil_Cycle = 0LL;
//}

//inline void SetP ()
//{
//	vi [ REG_P ].s = NextP.l;
//	
//	// should set this to zero to indicate it happened far in the past
//	//PBusyUntil_Cycle = -1LL;
//	PBusyUntil_Cycle = 0LL;
//}



void Recompiler::Test_FTOI4 ( VU* v, Vu::Instruction::Format i )
{
//#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
//	VU::debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
//	VU::debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
//	VU::debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
//	VU::debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
//#endif

	//VuUpperOpW ( v, i, PS2_Float_Madd );
	
	if ( ( v->test1_result1.sw0 != v->test2_result1.sw0 )
	|| ( v->test1_result1.sw1 != v->test2_result1.sw1 )
	|| ( v->test1_result1.sw2 != v->test2_result1.sw2 )
	|| ( v->test1_result1.sw3 != v->test2_result1.sw3 ) )
	{
		VU::debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
		VU::debug << " test1_src1= x=" << hex << v->test1_src1.fx << " y=" << v->test1_src1.fy << " z=" << v->test1_src1.fz << " w=" << v->test1_src1.fw;
		VU::debug << " test1_src1(hex)= x=" << hex << v->test1_src1.sw0 << " y=" << v->test1_src1.sw1 << " z=" << v->test1_src1.sw2 << " w=" << v->test1_src1.sw3;
		VU::debug << " test1_result1= x=" << hex << v->test1_result1.sw0 << " y=" << v->test1_result1.sw1 << " z=" << v->test1_result1.sw2 << " w=" << v->test1_result1.sw3;
		VU::debug << " test2_result1= x=" << hex << v->test2_result1.sw0 << " y=" << v->test2_result1.sw1 << " z=" << v->test2_result1.sw2 << " w=" << v->test2_result1.sw3;
	}
	
}


int Recompiler::Prefix_MADDW ( VU* v, Vu::Instruction::Format i )
{
//#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	VU::debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	VU::debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	VU::debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	VU::debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
//#endif

	//VuUpperOpW ( v, i, PS2_Float_Madd );

	return true;
}


int Recompiler::Postfix_MADDW ( VU* v, Vu::Instruction::Format i )
{

//#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	VU::debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	//VU::debug << " MAC=" << v->FlagSave [ v->iFlagSave_Index & v->c_lFlag_Delay_Mask ].MACFlag << " STATF=" << v->FlagSave [ v->iFlagSave_Index & v->c_lFlag_Delay_Mask ].StatusFlag;
	VU::debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
//#endif

	return true;
}


bool Recompiler::SetDstRegsWait_ReadCycleCount(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

#ifdef WAIT_DST_READ_CYCLECOUNT_ONCE

	if (v->iFDstReg_Count || v->iIDstReg_Count)
	{
		// load cycle#
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef BATCH_UPDATE_CYCLECOUNT
		// no need to add an offset if updating CycleCount in recompiler per instruction
		e->AddReg64ImmX(RAX, RecompileCount + 4);
#else
		e->AddReg64ImmX(RAX, 4);
#endif

#ifdef ENABLE_FDST_FAST_STORE_ALL_COMPONENTS
		if (v->iFDstReg_Count)
		{
			e->movq_to_sse128(RAX, RAX);
			e->vpbroadcastq2regreg(RAX, RAX);
		}
#endif
	}

#endif

	return ret;
}

bool Recompiler::SetIDstRegsWait_rec(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

	if (v->iIDstReg_Count)
	{
#ifndef WAIT_DST_READ_CYCLECOUNT_ONCE
		// load cycle#
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef BATCH_UPDATE_CYCLECOUNT
		// no need to add an offset if updating CycleCount in recompiler per instruction
		e->AddReg64ImmX(RAX, RecompileCount + 4);
#else
		e->AddReg64ImmX(RAX, 4);
#endif

#endif	// end #ifndef WAIT_DST_READ_CYCLECOUNT_ONCE

		for (int iIndex = 0; iIndex < v->iIDstReg_Count; iIndex++)
		{
			u32 ulDstRegIdx = v->uIDstReg_Index8[iIndex];

			if (ulDstRegIdx)
			{
				// store cycle#
				ret = e->MovMemReg64((int64_t*)&v->IReg_BusyUntil[ulDstRegIdx], RAX);
			}
		}
	}

	return ret;
}

bool Recompiler::SetFDstRegsWait_rec(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

	if (v->iFDstReg_Count)
	{
#ifndef WAIT_DST_READ_CYCLECOUNT_ONCE
		// load cycle#
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef BATCH_UPDATE_CYCLECOUNT
		// no need to add an offset if updating CycleCount in recompiler per instruction
		e->AddReg64ImmX(RAX, RecompileCount + 4);
#else
		e->AddReg64ImmX(RAX, 4);
#endif

#endif	// end #ifndef WAIT_DST_READ_CYCLECOUNT_ONCE

		// do the float regs first
		for (int iIndex = 0; iIndex < v->iFDstReg_Count; iIndex++)
		{
			u32 ulDstRegIdx = v->uFDstReg_Index8[iIndex];
			u32 ulDstRegMask = v->uFDstReg_Mask8[iIndex];

			// store cycle#
			ret = e->MovMemReg64((int64_t*)&v->FReg_BusyUntil[ulDstRegIdx], RAX);

			if (ulDstRegIdx)
			{
#ifdef ENABLE_FDST_FAST_STORE_ALL_COMPONENTS
				// if all the components, then store all at once
				if ((ulDstRegMask & 0xf) == 0xf)
				{
					//e->movq_to_sse128(RAX, RAX);
					//e->vpbroadcastq2regreg(RAX, RAX);
					e->movdqa2_memreg(&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 0]), RAX);
				}
				else
#endif
				{
#ifdef ENABLE_FDST_FAST_STORE_ALL_COMPONENTS
					if ((ulDstRegMask & 0xc) == 0xc)
					{
						e->movdqa1_memreg(&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 0]), RAX);
					}
					else
#endif
					{
						// test each component in the mask
						if (ulDstRegMask & 0x8)
						{
							ret = e->MovMemReg64((int64_t*)&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 0]), RAX);
						}
						if (ulDstRegMask & 0x4)
						{
							ret = e->MovMemReg64((int64_t*)&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 1]), RAX);
						}
					}

#ifdef ENABLE_FDST_FAST_STORE_ALL_COMPONENTS
					if ((ulDstRegMask & 0x3) == 0x3)
					{
						e->movdqa1_memreg(&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 2]), RAX);
					}
					else
#endif
					{
						if (ulDstRegMask & 0x2)
						{
							ret = e->MovMemReg64((int64_t*)&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 2]), RAX);
						}
						if (ulDstRegMask & 0x1)
						{
							ret = e->MovMemReg64((int64_t*)&(v->FRegXYZW_BusyUntil[(ulDstRegIdx << 2) + 3]), RAX);
						}
					}

				}	// end if ((ulDstRegMask & 0xf) == 0xf) else

			}	// end if (ulDstRegIdx)

		}	// end for (int iIndex = 0; iIndex < v->iFDstReg_Count; iIndex++)

	}	// end if (v->iFDstReg_Count)

	return ret;
}


bool Recompiler::WaitForSrcRegs_ReadCycleCount(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

#ifdef WAIT_SRC_READ_CYCLECOUNT_ONCE

	if (v->iFSrcReg_Count || v->iISrcReg_Count)
	{
		ret = e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	}

#endif

	return ret;
}

bool Recompiler::WaitForSrcRegs_WriteCycleCount(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

#ifdef WAIT_SRC_WRITE_CYCLECOUNT_ONCE

	if (v->iFSrcReg_Count || v->iISrcReg_Count)
	{
		ret = e->MovMemReg64((int64_t*)&v->CycleCount, RAX);
	}

#endif

	return ret;
}


bool Recompiler::WaitForISrcRegs_rec(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

	if (v->iISrcReg_Count)
	{
#ifndef WAIT_SRC_READ_CYCLECOUNT_ONCE
		// load the current cycle counter
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
#endif

		for (int iIndex = 0; iIndex < v->iISrcReg_Count; iIndex++)
		{
			u32 ulSrcRegIdx = v->uISrcReg_Index8[iIndex];

			if (ulSrcRegIdx)
			{
				e->CmpRegMem64(RAX, (int64_t*)&(v->IReg_BusyUntil[ulSrcRegIdx]));
				e->CmovBRegMem64(RAX, (int64_t*)&(v->IReg_BusyUntil[ulSrcRegIdx]));
			}
		}

#ifndef WAIT_SRC_WRITE_CYCLECOUNT_ONCE
		// write back the CycleCount
		ret = e->MovMemReg64((int64_t*)&v->CycleCount, RAX);
#endif
	}

	return ret;
}


// before calling call "WaitForSrcRegs_LoadCycleCount"
// when done with both "WaitForFSrcRegs_rec" and "WaitForISrcRegs_rec" then call "WaitForSrcRegs_StoreCycleCount"
bool Recompiler::WaitForFSrcRegs_rec(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

	if (v->iFSrcReg_Count)
	{
#ifndef WAIT_SRC_READ_CYCLECOUNT_ONCE
		// load the current cycle counter
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
#endif

		// this is just the upper instruction since I bit is set
		// so only check float regs for hazards
		for (int iIndex = 0; iIndex < v->iFSrcReg_Count; iIndex++)
		{
			// *** TODO ***
			u32 ulSrcRegIdx = v->uFSrcReg_Index8[iIndex];
			u32 ulSrcRegMask = v->uFSrcReg_Mask8[iIndex];

			if (ulSrcRegIdx)
			{
				if (ulSrcRegMask == 0xf)
				{
					e->CmpRegMem64(RAX, (int64_t*)&(v->FReg_BusyUntil[ulSrcRegIdx]));
					e->CmovBRegMem64(RAX, (int64_t*)&(v->FReg_BusyUntil[ulSrcRegIdx]));
				}
				else
				{
					// test each component in the mask
					if (ulSrcRegMask & 0x8)
					{
						e->CmpRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 0]));
						e->CmovBRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 0]));
					}
					if (ulSrcRegMask & 0x4)
					{
						e->CmpRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 1]));
						e->CmovBRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 1]));
					}
					if (ulSrcRegMask & 0x2)
					{
						e->CmpRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 2]));
						e->CmovBRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 2]));
					}
					if (ulSrcRegMask & 0x1)
					{
						e->CmpRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 3]));
						e->CmovBRegMem64(RAX, (int64_t*)&(v->FRegXYZW_BusyUntil[(ulSrcRegIdx << 2) + 3]));
					}
				}	// end if (ulSrcRegMask == 0xf) else
			}
		}

#ifndef WAIT_SRC_WRITE_CYCLECOUNT_ONCE
		// write back the CycleCount
		ret = e->MovMemReg64((int64_t*)&v->CycleCount, RAX);
#endif
	}

	return ret;
}

bool Recompiler::UpdateFlags_rec(x64Encoder* e, VU* v, u32 Address)
{
	bool ret = true;

	u32 uRecompileCount;
	uRecompileCount = (Address & v->ulVuMem_Mask) >> 3;

#ifdef SKIP_EXTRA_FLAG_CHECKS
	if ((v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_STAT)
		|| (v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_MAC)
		|| (v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_CLIP)
		)
#endif
	{
#ifdef USE_CYCLE_NUMBER_FLAG_INDEX
		// use cycle#
		e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
		//e->pinsrq1regreg(RAX, RAX, RAX, 1);

#else
		//v->iFlagSave_Index++;
		//int FlagIndex = ( v->iFlagSave_Index ) & VU::c_lFlag_Delay_Mask;
		e->MovRegMem32(RAX, (int32_t*)&v->iFlagSave_Index);
		e->IncReg32(RAX);
		e->MovMemReg32((int32_t*)&v->iFlagSave_Index, RAX);

#endif


#ifdef SKIP_EXTRA_FLAG_CHECKS_STAT
		if (v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_STAT)
#endif
		{
			//v->FlagSave [ FlagIndex ].StatusFlag = v->vi [ VU::REG_STATUSFLAG ].u;
			//e->MovRegMem32(RDX, &v->vi[VU::REG_STATUSFLAG].s);
			//e->MovRegToMem16(RDX, RCX, RAX, SCALE_EIGHT, 0);
			e->pinsrw1regmem(RAX, RAX, &v->vi[VU::REG_STATUSFLAG].sh0, 0);
		}

#ifdef SKIP_EXTRA_FLAG_CHECKS_MAC
		if (v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_MAC)
#endif
		{
			//v->FlagSave [ FlagIndex ].MACFlag = v->vi [ VU::REG_MACFLAG ].u;
			//e->MovRegMem32(RDX, &v->vi[VU::REG_MACFLAG].s);
			//e->MovRegToMem16(RDX, RCX, RAX, SCALE_EIGHT, 2);
			e->pinsrw1regmem(RAX, RAX, &v->vi[VU::REG_MACFLAG].sh0, 1);
		}

#ifdef SKIP_EXTRA_FLAG_CHECKS_CLIP
		if (v->pLUT_StaticInfo[uRecompileCount] & STATIC_FLAG_CHECK_CLIP)
#endif
		{
			//v->FlagSave [ FlagIndex ].ClipFlag = v->vi [ VU::REG_CLIPFLAG ].u;
			//e->MovRegMem32(RDX, &v->vi[VU::REG_CLIPFLAG].s);
			//e->MovRegToMem32(RDX, RCX, RAX, SCALE_EIGHT, 4);
			e->pinsrd1regmem(RAX, RAX, &v->vi[VU::REG_CLIPFLAG].s, 1);
		}


#ifdef BATCH_UPDATE_CYCLECOUNT
		// no need to add an offset if updating CycleCount in recompiler per instruction
		e->AddReg64ImmX(RDX, RecompileCount);
#endif


		// write cycle#
		//e->MovRegMem64(RDX, (int64_t*)&v->CycleCount);

		e->MovRegMem64(RDX, (int64_t*)&v->CycleCount);
		e->pinsrq1regreg(RAX, RAX, RDX, 1);

		e->LeaRegMem64(RCX, &v->FlagSave);

		e->AndReg32ImmX(RAX, VU::c_lFlag_Delay_Mask);
		//e->ShlRegImm32(RAX, 2);
		e->AddRegReg32(RAX, RAX);

		//ret = e->MovRegToMem64(RDX, RCX, RAX, SCALE_EIGHT, 8);
		ret = e->movdqa1_memreg(RAX, RCX, RAX, SCALE_EIGHT, 0);
	}

	return ret;
}

void Recompiler::AdvanceCycle_rec ( x64Encoder *e, VU* v, u32 Address )
{
#ifdef VERBOSE_RECOMPILE
	cout << " AdvanceCycle_rec";
	cout << " VU#" << v->Number;
	cout << " eOffset=" << dec << e->x64NextOffset;
#endif


	//u32 uRecompileCount;
	//uRecompileCount = (Address & v->ulVuMem_Mask) >> 3;


#ifndef BATCH_UPDATE_CYCLECOUNT

	// CycleCount not otherwise needed if not updating q
	e->IncMem64((int64_t*)&v->CycleCount);

#endif


#ifdef VERBOSE_RECOMPILE
cout << "->AdvanceCycle_rec_DONE";
#endif
}


void Recompiler::AdvanceCycle ( VU* v )
{

#ifndef BATCH_UPDATE_CYCLECOUNT

	v->CycleCount++;

#endif

	/*
	// update q and p registers here for now
	v->UpdateQ ();
	//v->UpdateP ();
	
	v->iFlagSave_Index++;
	
	// set the flags
	int FlagIndex = ( v->iFlagSave_Index ) & VU::c_lFlag_Delay_Mask;

//#ifdef ENABLE_SNAPSHOTS
#ifdef ENABLE_VFLAGS
	v->vFlagSave [ FlagIndex ].Value0 = v->CurrentVFlags.Value0;
	v->vFlagSave [ FlagIndex ].Value1 = v->CurrentVFlags.Value1;
#else
	v->FlagSave [ FlagIndex ].StatusFlag = v->vi [ VU::REG_STATUSFLAG ].u;
	v->FlagSave [ FlagIndex ].MACFlag = v->vi [ VU::REG_MACFLAG ].u;
#endif

	v->FlagSave [ FlagIndex ].ClipFlag = v->vi [ VU::REG_CLIPFLAG ].u;

//#ifdef ENABLE_STALLS
	// for now, will process bitmap on every instruction
	
	// remove bitmap from pipeline
	//RemoveBitmap ( Pipeline_Bitmap, FlagSave [ FlagIndex ].Bitmap );
	v->RemovePipeline ();
	
	// need to clear the bitmap for the entry for now
	VU::ClearBitmap ( v->FlagSave [ FlagIndex ].Bitmap );
	*/
	
	// remove from MACRO pipeline also
	//v->Int_Pipeline_Bitmap &= ~v->FlagSave [ FlagIndex ].Int_Bitmap;
	//v->FlagSave [ FlagIndex ].Int_Bitmap = 0;
//#endif

//#ifdef INLINE_DEBUG_ADVANCE_CYCLE
//	VU::debug << "\r\nhps2x64: VU#" << dec << v->Number << ": ADVANCE_CYCLE_REC: P0=" << hex << v->Pipeline_Bitmap.b0 << " P1=" << v->Pipeline_Bitmap.b1 << " S0=" << v->SrcRegs_Bitmap.b0 << " S1=" << v->SrcRegs_Bitmap.b1 << " PC=" << v->PC << " Cycle#" << dec << v->CycleCount << "\r\n";
//	VU::debug << " fi=" << (( v->iFlagSave_Index - 4 ) & v->c_lFlag_Delay_Mask);
//	VU::debug << " fb0= " << hex << v->FlagSave [ 0 ].Bitmap.b0 << " " << v->FlagSave [ 0 ].Bitmap.b1;
//	VU::debug << " fb1= " << v->FlagSave [ 1 ].Bitmap.b0 << " " << v->FlagSave [ 1 ].Bitmap.b1;
//	VU::debug << " fb2= " << v->FlagSave [ 2 ].Bitmap.b0 << " " << v->FlagSave [ 2 ].Bitmap.b1;
//	VU::debug << " fb3= " << v->FlagSave [ 3 ].Bitmap.b0 << " " << v->FlagSave [ 3 ].Bitmap.b1;
//#endif

}


// returns the address jumping to or zero if it doesn't know the address
int32_t Recompiler::ProcessBranch ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{


	switch ( i.Opcode )
	{
		// B
		case 0x20:
			
		// BAL
		case 0x21:
	
		// IBEQ
		case 0x28:
		
		// IBGEZ
		case 0x2f:
		
		// IBGTZ
		case 0x2d:
		
		// IBLEZ
		case 0x2e:
		
		// IBLTZ
		case 0x2c:
		
		// IBNE
		case 0x29:
		
			e->MovMemImm32 ( (int32_t*) & v->NextPC, ( Address + ( i.Imm11 << 3 ) ) & v->ulVuMem_Mask );

			return ((Address + (i.Imm11 << 3)) & v->ulVuMem_Mask);
			
			break;
			
			
		// JALR
		case 0x25:
		
		// JR
		case 0x24:
		
			// it must be multiplying the register address by 8 before jumping to it
			//NextPC = d->Data << 3;
			e->MovRegMem32 ( RAX, (int32_t*) & v->Recompiler_BranchDelayAddress );
			e->ShlRegImm32 ( RAX, 3 );
			
			// make sure that address is not outside range (no memory management on VU)
			//NextPC &= ulVuMem_Mask;
			e->AndReg32ImmX ( RAX, v->ulVuMem_Mask );
			e->MovMemReg32 ( (int32_t*) & v->NextPC, RAX );
			
			break;
	}
	
	// return
	//return e->Ret ();
	return 0;
}


// if block has no recompiled code, then it should return 1 (meaning to recompile the instruction(s))
u32 Recompiler::InitBlock ( u32 Block )
{
	// set the encoder to use
	e = InstanceEncoder;

	// start encoding in block
	e->StartCodeBlock ( Block );
	
	// set return value 1 (return value for X64 goes in register A)
	e->LoadImm32 ( RAX, 1 );
	
	// return
	e->x64EncodeReturn ();
	
	// done encoding in block
	e->EndCodeBlock ();

	return true;
}


/** 
 * @fn static u32* RGetPointer ( VU *v, u32 Address )
 * @brief return a point into PROGRAM memory for vu at address
 * @param v is a pointer into the VU object state that you want the pointer for
 * @param Address is the address into vu program memory for that vu to get the pointer for
 * @return pointer into specified vu memory for the specified Address
 */

u32* Recompiler::RGetPointer ( VU *v, u32 Address )
{
#ifdef VERBOSE_RECOMPILE
cout << "\nRGetPointer: NON-CACHED";
#endif

	return (u32*) & v->MicroMem64 [ ( Address & v->ulVuMem_Mask ) >> 3 ];
}




// returns the bitmap for the destination registers for instruction
// if the instruction is not supported, then it will return -1ULL
u64 Recompiler::Get_DelaySlot_DestRegs ( Vu::Instruction::Format i )
{
	
	
	
	
	// any other instructions are not cleared to go
	return 0;
}



// convert const float to double for addition without regard for underflow
#define CVT_FLOAT_TO_DOUBLE_ADD(c)	((((c)&0x7fffffffull)<<29)|(((c)&0x80000000ull)<<32))

// convert const float to double for multiply without regard for underflow
#define CVT_FLOAT_TO_DOUBLE_MUL(c)	((((((c)>>23)&0xffull)+(1023ull - 127ull)) << 52)|(((c) & 0x7fffffull)<<29)|(((c)&0x80000000ull)<<32))

// convert const float to double for multiply with room for underflow (and if not zero)
#define CVT_FLOAT_TO_DOUBLE_MUL_UND(c)	((((((c)>>23)&0xffull)+(2046ull - 0ull)) << 52)|(((c) & 0x7fffff)<<29)|(((c)&0x80000000ull)<<32))

static int64_t max_value = 0x0fffffffe0000000ull;
static int64_t min_value = 0x8fffffffe0000000ull;
static int64_t mul_value = (2046ull - 127ull) << 52;

static int64_t div_cvt_value = (127ull) << 52;

static int64_t dT1 = CVT_FLOAT_TO_DOUBLE_MUL(0x3f7ffff5ull);
static int64_t dT2 = CVT_FLOAT_TO_DOUBLE_MUL(0xbeaaa61cull);
static int64_t dT3 = CVT_FLOAT_TO_DOUBLE_MUL(0x3e4c40a6ull);
static int64_t dT4 = CVT_FLOAT_TO_DOUBLE_MUL(0xbe0e6c63ull);
static int64_t dT5 = CVT_FLOAT_TO_DOUBLE_MUL(0x3dc577dfull);
static int64_t dT6 = CVT_FLOAT_TO_DOUBLE_MUL(0xbd6501c4ull);
static int64_t dT7 = CVT_FLOAT_TO_DOUBLE_MUL(0x3cb31652ull);
static int64_t dT8 = CVT_FLOAT_TO_DOUBLE_MUL(0xbb84d7e7ull);

static int64_t dPI4 = CVT_FLOAT_TO_DOUBLE_ADD(0x3f490fdbull);

static int64_t dOne = CVT_FLOAT_TO_DOUBLE_ADD(0x3f800000ull);

static int64_t dE1 = CVT_FLOAT_TO_DOUBLE_MUL(0x3e7fffa8ull);
static int64_t dE2 = CVT_FLOAT_TO_DOUBLE_MUL(0x3d0007f4ull);
static int64_t dE3 = CVT_FLOAT_TO_DOUBLE_MUL(0x3b29d3ffull);
static int64_t dE4 = CVT_FLOAT_TO_DOUBLE_MUL(0x3933e553ull);
static int64_t dE5 = CVT_FLOAT_TO_DOUBLE_MUL(0x36b63510ull);
static int64_t dE6 = CVT_FLOAT_TO_DOUBLE_MUL(0x353961acull);

static int64_t dS1 = CVT_FLOAT_TO_DOUBLE_MUL(0x3f800000ull);
static int64_t dS2 = CVT_FLOAT_TO_DOUBLE_MUL(0xbe2aaaa4ull);
static int64_t dS3 = CVT_FLOAT_TO_DOUBLE_MUL(0x3c08873eull);
static int64_t dS4 = CVT_FLOAT_TO_DOUBLE_MUL(0xb94fb21full);
static int64_t dS5 = CVT_FLOAT_TO_DOUBLE_MUL(0x362e9c14ull);

alignas(32) static int64_t dS14[4] = { dS1, dS2, dS4, dS3 };

alignas(32) static int64_t dT14[4] = { dT1, dT2, dT4, dT3 };
alignas(32) static int64_t dT58[4] = { dT6, dT5, dT8, dT7 };

alignas(32) static int64_t dE14[4] = { dE1, dE2, dE4, dE3 };
alignas(32) static int64_t dE56[2] = { dE6, dE5 };



int32_t Recompiler::ENCODE_EATAN(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 53/54 cycles
	static const u64 c_CycleTime = 53;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);


	// eatan vfs.fsf

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = T1 * x + T2 * x^3 + T3 * x^5 + T4 * x^7 + T5 * x^9 + T6 * x^11 + T7 * x^13 + T8 * x^15 + pi/4
	// where x=(x-1)/(x+1)

	// load input
	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	//e->vpbroadcastd1regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// cvt input to double
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->pand1regreg(RAX, RAX, 5);

	// 1 ->rcx
	e->movsd1_regmem(RCX, (int64_t*)&dOne);

	// x(rax) + 1(rcx) ->rdx.x ->rdx.y
	e->vaddsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RAX, RAX, RDX);


	// x(rax) - 1(rcx) ->rdx.x
	e->vsubsd(RDX, RAX, RCX);
	e->vminpd(RDX, RDX, 4);
	e->vmaxpd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);


	// (x-1)(rdx.x)/(x+1)(rdx.y->rax.x) ->rax.x
	e->pshufd1regregimm(RAX, RDX, 0xee);
	e->vdivsd(RAX, RDX, RAX);


	// convert div result(rax.x) ->rax.x
	e->movsd1_regmem(RCX, (int64_t*)&div_cvt_value);
	e->vmulsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);


	// mul ready x^2 -> rcx
	e->vmulsd(RCX, RAX, RBX);
	e->vmulsd(RCX, RAX, RCX);
	e->vminsd(RCX, RCX, 4);
	e->pand1regreg(RCX, RCX, 5);

	// x^3 -> RDX.x -> RAX.y
	e->vmulsd(RCX, RCX, RBX);
	e->vmulsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RAX, RAX, RDX);


	// mul ready x^2(rcx) * x^3(rdx) = x^5 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^5(rdx) = x^7 -> RDX.x -> RAX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RAX, RAX, RDX, 1);

	// mul ready x^2(rcx) * x^7(rdx) = x^9 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^9(rdx) = x^11 -> RDX.x -> RBX lo
	e->vmulsd(RDX, RDX, RCX);
	e->movdqa1_regreg(RBX, RDX);

	// mul ready x^2(rcx) * x^11(rdx) = x^13 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^13(rdx) = x^15 -> RDX.x -> RBX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RBX, RBX, RDX, 1);

	// min/max x,x^2,x^5,x^7
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RAX, RAX, 5);
	e->pand2regreg(RBX, RBX, 5);


	// mul f1 through f4(rcx) -> rax
	e->movdqa2_regmem(RCX, (void*)&dT14);
	e->vmulpd2(RAX, RAX, RCX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);


	// mul f5 through f8(rcx) -> rbx
	e->movdqa2_regmem(RCX, (void*)&dT58);
	e->vmulpd2(RBX, RBX, RCX);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RBX, RBX, 5);

	// add fc1 through fc4(rax) + fc5 through fc8(rbx) ->rax
	e->vaddpd2(RAX, RAX, RBX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);

	// add fc1 through fc8 ->rbx
	e->vextracti128regreg(RCX, RAX, 1);
	e->vaddpd(RAX, RAX, RCX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc12 and fc34 ->rax
	e->pshufd2regregimm(RCX, RAX, 0xee);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add pi/4 ->rax
	e->movsd1_regmem(RCX, (int64_t*)&dPI4);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);

	// cvt to ps2 float
	e->psrlq1regimm(RCX, RAX, 63);
	e->pslld1regimm(RCX, RCX, 31);
	e->psrlq1regimm(RAX, RAX, 29);
	e->por1regreg(RAX, RAX, RCX);

	// store result
	//e->movdqa1_memreg((void*)&v->NextP, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);


	return ret;
}


int32_t Recompiler::ENCODE_EATANxy(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 53/54 cycles
	static const u64 c_CycleTime = 53;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	// eatanxy vfs[x,y]


	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = T1 * x + T2 * x^3 + T3 * x^5 + T4 * x^7 + T5 * x^9 + T6 * x^11 + T7 * x^13 + T8 * x^15 + pi/4
	// where x=(y-1)/(y+x)

	// load input
	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	e->movq1_regmem(RAX, (int64_t*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// cvt input to double
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->pand1regreg(RAX, RAX, 5);


	// y -> rcx
	e->pshufd1regregimm(RCX, RAX, 0xee);

	// x(rax) + y(rcx) ->rdx.x ->rdx.y
	e->vaddsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RCX, RCX, RDX);


	// y(rcx) - x(rdx) ->rdx.x
	e->vsubsd(RDX, RCX, RAX);
	e->vminpd(RDX, RDX, 4);
	e->vmaxpd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);


	// (y-1)(rdx.x)/(x+y)(rdx.y->rax.x) ->rax.x
	e->pshufd1regregimm(RAX, RDX, 0xee);
	e->vdivsd(RAX, RDX, RAX);


	// convert div result(rax.x) ->rax.x
	e->movsd1_regmem(RCX, (int64_t*)&div_cvt_value);
	e->vmulsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);


	// mul ready x^2 -> rcx
	e->vmulsd(RCX, RAX, RBX);
	e->vmulsd(RCX, RAX, RCX);
	e->vminsd(RCX, RCX, 4);
	e->pand1regreg(RCX, RCX, 5);


	// x^3 -> RDX.x -> RAX.y
	e->vmulsd(RCX, RCX, RBX);
	e->vmulsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RAX, RAX, RDX);


	// mul ready x^2(rcx) * x^3(rdx) = x^5 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^5(rdx) = x^7 -> RDX.x -> RAX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RAX, RAX, RDX, 1);

	// mul ready x^2(rcx) * x^7(rdx) = x^9 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^9(rdx) = x^11 -> RDX.x -> RBX lo
	e->vmulsd(RDX, RDX, RCX);
	e->movdqa1_regreg(RBX, RDX);

	// mul ready x^2(rcx) * x^11(rdx) = x^13 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^13(rdx) = x^15 -> RDX.x -> RBX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RBX, RBX, RDX, 1);

	// min/max x,x^2,x^5,x^7
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RAX, RAX, 5);
	e->pand2regreg(RBX, RBX, 5);


	// mul f1 through f4(rcx) -> rax
	e->movdqa2_regmem(RCX, (void*)&dT14);
	e->vmulpd2(RAX, RAX, RCX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);


	// mul f5 through f8(rcx) -> rbx
	e->movdqa2_regmem(RCX, (void*)&dT58);
	e->vmulpd2(RBX, RBX, RCX);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RBX, RBX, 5);

	// add fc1 through fc4(rax) + fc5 through fc8(rbx) ->rax
	e->vaddpd2(RAX, RAX, RBX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);

	// add fc1 through fc8 ->rbx
	e->vextracti128regreg(RCX, RAX, 1);
	e->vaddpd(RAX, RAX, RCX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc12 and fc34 ->rax
	e->pshufd1regregimm(RCX, RAX, 0xee);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add pi/4 ->rax
	e->movsd1_regmem(RCX, (int64_t*)&dPI4);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);

	// cvt to ps2 float
	e->psrlq1regimm(RCX, RAX, 63);
	e->pslld1regimm(RCX, RCX, 31);
	e->psrlq1regimm(RAX, RAX, 29);
	e->por1regreg(RAX, RAX, RCX);

	// store result
	//e->movdqa1_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_EATANxz(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 53/54 cycles
	static const u64 c_CycleTime = 53;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = T1 * x + T2 * x^3 + T3 * x^5 + T4 * x^7 + T5 * x^9 + T6 * x^11 + T7 * x^13 + T8 * x^15 + pi/4
	// where x=(z-x)/(z+x)

	// load input
	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// cvt input to double
	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 29);
	e->pand2regreg(RAX, RAX, 5);


	// z -> rcx
	e->vextracti128regreg(RCX, RAX, 1);

	// x(rax) + z(rcx) ->rdx.x ->rdx.y
	e->vaddsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RCX, RCX, RDX);


	// z(rcx) - x(rax) ->rdx.x
	e->vsubsd(RDX, RCX, RAX);
	e->vminpd(RDX, RDX, 4);
	e->vmaxpd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);


	// (z-x)(rdx.x)/(z+x)(rdx.y->rax.x) ->rax.x
	e->pshufd1regregimm(RAX, RDX, 0xee);
	e->vdivsd(RAX, RDX, RAX);


	// convert div result(rax.x) ->rax.x
	e->movsd1_regmem(RCX, (int64_t*)&div_cvt_value);
	e->vmulsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);


	// mul ready x^2 -> rcx
	e->vmulsd(RCX, RAX, RBX);
	e->vmulsd(RCX, RAX, RCX);
	e->vminsd(RCX, RCX, 4);
	e->pand1regreg(RCX, RCX, 5);


	// x^3 -> RDX.x -> RAX.y
	e->vmulsd(RCX, RCX, RBX);
	e->vmulsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RAX, RAX, RDX);


	// mul ready x^2(rcx) * x^3(rdx) = x^5 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^5(rdx) = x^7 -> RDX.x -> RAX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RAX, RAX, RDX, 1);

	// mul ready x^2(rcx) * x^7(rdx) = x^9 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^9(rdx) = x^11 -> RDX.x -> RBX lo
	e->vmulsd(RDX, RDX, RCX);
	e->movdqa1_regreg(RBX, RDX);

	// mul ready x^2(rcx) * x^11(rdx) = x^13 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^13(rdx) = x^15 -> RDX.x -> RBX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RBX, RBX, RDX, 1);

	// min/max x,x^2,x^5,x^7
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RAX, RAX, 5);
	e->pand2regreg(RBX, RBX, 5);


	// mul f1 through f4(rcx) -> rax
	e->movdqa2_regmem(RCX, (void*)&dT14);
	e->vmulpd2(RAX, RAX, RCX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);


	// mul f5 through f8(rcx) -> rbx
	e->movdqa2_regmem(RCX, (void*)&dT58);
	e->vmulpd2(RBX, RBX, RCX);
	e->vminpd2(RBX, RBX, 4);
	e->vmaxpd2(RBX, RBX, 5);
	e->pand2regreg(RBX, RBX, 5);

	// add fc1 through fc4(rax) + fc5 through fc8(rbx) ->rax
	e->vaddpd2(RAX, RAX, RBX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);

	// add fc1 through fc8 ->rbx
	e->vextracti128regreg(RCX, RAX, 1);
	e->vaddpd(RAX, RAX, RCX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc12 and fc34 ->rax
	e->pshufd2regregimm(RCX, RAX, 0xee);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add pi/4 ->rax
	e->movsd1_regmem(RCX, (int64_t*)&dPI4);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);

	// cvt to ps2 float
	e->psrlq1regimm(RCX, RAX, 63);
	e->pslld1regimm(RCX, RCX, 31);
	e->psrlq1regimm(RAX, RAX, 29);
	e->por1regreg(RAX, RAX, RCX);

	// store result
	//e->movdqa1_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_EEXP(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 43/44 cycles
	static const u64 c_CycleTime = 43;	//44;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	// eexp vfs.fsf

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = 1/(1 + E1 * x + E2 * x^2 + E3 * x^3 + E4 * x^4 + E5 * x^5 + E6 * x^6)^4
	// E1 = 0x3e7fffa8
	// E2 = 0x3d0007f4
	// E3 = 0x3b29d3ff
	// E4 = 0x3933e553
	// E5 = 0x36b63510
	// E6 = 0x353961ac

	// load input
	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	//e->vpbroadcastd1regmem(RAX, (int32_t*)&vs.u);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// cvt input to double
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->pand1regreg(RAX, RAX, 5);

	// mul ready x^1-> RCX.x (positive)
	e->vmulsd(RCX, RAX, RBX);

	// x^2 -> RDX.x -> RAX.y
	e->vmulsd(RDX, RAX, RCX);
	e->punpcklqdq1regreg(RAX, RAX, RDX);

	// mul ready x^1(rcx) * x^2(rdx) = x^3 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^1(rcx) * x^3(rdx) = x^4 -> RDX.x -> RAX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RAX, RAX, RDX, 1);


	// min/max x^1,x^2,x^3,x^4
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);

	// mul ready x^1(rcx) * x^4(rdx) = x^5 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^1(rcx) * x^5(rdx) = x^6 -> RDX.x
	e->vmulsd(RDX, RDX, RCX);

	// min/max x^5,x^6
	e->vminpd(RDX, RDX, 4);
	e->vmaxpd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);


	// mul f1 through f4(rcx) -> rax
	e->movdqa2_regmem(RCX, (void*)&dE14);
	e->vmulpd2(RAX, RAX, RCX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);


	// mul f5(rcx) through f6 ->rdx
	e->movdqa1_regmem(RCX, (void*)&dE56);
	e->vmulpd(RDX, RDX, RCX);
	e->vminpd(RDX, RDX, 4);
	e->vmaxpd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);

	// add fc12 through fc43 ->rax
	e->vextracti128regreg(RCX, RAX, 1);
	e->vaddpd(RAX, RAX, RCX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// then add with fc56(rdx)
	e->vaddpd(RAX, RAX, RDX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc12 and fc34 ->rax
	e->pshufd1regregimm(RCX, RAX, 0xee);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add one
	e->movsd1_regmem(RCX, (int64_t*)&dOne);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// to the squared -> rcx
	e->vmulsd(RCX, RAX, RBX);
	e->vmulsd(RCX, RCX, RAX);
	e->vminsd(RCX, RCX, 4);
	e->vmaxsd(RCX, RCX, 5);
	e->pand1regreg(RCX, RCX, 5);

	// to the fourth -> rax
	e->vmulsd(RAX, RCX, RBX);
	e->vmulsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// reciprocal
	e->movsd1_regmem(RCX, (int64_t*)&dOne);
	e->movsd1_regmem(RDX, (int64_t*)&div_cvt_value);
	e->vdivsd(RAX, RCX, RAX);
	e->vmulsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);


	// cvt to ps2 float
	e->psrlq1regimm(RAX, RAX, 29);

	// store result
	//e->movdqa1_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ESIN(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 28/29 cycles
	static const u64 c_CycleTime = 28;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);


	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = S1 * x + S2 * x^3 + S3 * x^5 + S4 * x^7 + S5 * x^9
	// S1 = 0x3f800000
	// S2 = 0xbe2aaaa4
	// S3 = 0x3c08873e
	// S4 = 0xb94fb21f
	// S5 = 0x362e9c14

	// load input
	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	//e->vpbroadcastd1regmem(RAX, (int32_t*)&vs.u);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// cvt input to double
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->pand1regreg(RAX, RAX, 5);

	// mul ready x^2-> RCX.x (positive)
	e->vmulsd(RCX, RAX, RBX);
	e->vmulsd(RCX, RAX, RCX);
	e->vminsd(RCX, RCX, 4);
	e->pand1regreg(RCX, RCX, 5);
	e->vmulsd(RCX, RCX, RBX);

	// x^3 -> RDX.x -> RAX.y
	e->vmulsd(RDX, RAX, RCX);

	//e->pinsrq1regreg(RAX, RAX, RDX, 1);
	e->punpcklqdq1regreg(RAX, RAX, RDX);


	// mul ready x^2(rcx) * x^3(rdx) = x^5 -> RDX.x -> RDX.y
	e->vmulsd(RDX, RDX, RCX);
	e->punpcklqdq1regreg(RDX, RDX, RDX);

	// mul ready x^2(rcx) * x^5(rdx) = x^7 -> RDX.x -> RAX hi
	e->vmulsd(RDX, RDX, RCX);
	e->vinserti128regreg(RAX, RAX, RDX, 1);


	// min/max x,x^2,x^5,x^7
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);

	// mul ready x^2(rcx) * x^7(rdx) = x^9 -> RDX.x
	e->vmulsd(RDX, RDX, RCX);
	e->vminsd(RDX, RDX, 4);
	e->vmaxsd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);


	// mul f1 through f4(rcx) -> rax
	e->movdqa2_regmem(RCX, (void*)&dS14);
	e->vmulpd2(RAX, RAX, RCX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);
	e->pand2regreg(RAX, RAX, 5);


	// mul f5(rcx) ->rdx
	e->movsd1_regmem(RCX, (int64_t*)&dS5);
	e->vmulsd(RDX, RDX, RCX);
	e->vminsd(RDX, RDX, 4);
	e->vmaxsd(RDX, RDX, 5);
	e->pand1regreg(RDX, RDX, 5);

	// add fc1 through fc4 ->rax
	e->vextracti128regreg(RCX, RAX, 1);
	e->vaddpd(RAX, RAX, RCX);
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc12 and fc34 ->rax
	e->pshufd2regregimm(RCX, RAX, 0xee);
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// add fc1234(rax) and fc5(rdx)
	e->vaddsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);

	// cvt to ps2 float
	e->psrlq1regimm(RCX, RAX, 63);
	e->pslld1regimm(RCX, RCX, 31);
	e->psrlq1regimm(RAX, RAX, 29);
	e->por1regreg(RAX, RAX, RCX);

	// store result
	//e->movdqa1_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ERSQRT(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);


	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = rsqrt(vf(fsf))

	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&div_cvt_value);

	e->movq1_regmem(RCX, (int64_t*)&dOne);

	// note: no sign because it is squared
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 33);
	e->psrlq1regimm(RAX, RAX, 4);

	e->vmulsd(RAX, RAX, RBX);
	e->sqrtsd(RAX, RAX);

	e->cvtsd2ss1regreg(RAX, RAX);


	// the cvtsd2ss does not clear the upper half of the 64-bits
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);


	e->vdivsd(RAX, RCX, RAX);


	e->vmulsd(RAX, RAX, 5);
	e->vminpd2(RAX, RAX, 4);

	e->psrlq1regimm(RAX, RAX, 29);


	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ERCPR(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);


	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = rcpr(vf(fsf))

	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&div_cvt_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);

	e->movq1_regmem(RCX, (int64_t*)&dOne);


	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->pand1regreg(RAX, RAX, 5);

	e->vdivsd(RAX, RCX, RAX);
	e->vmulsd(RAX, RAX, RBX);
	e->vminpd2(RAX, RAX, 4);
	e->vmaxpd2(RAX, RAX, 5);

	// get sign
	e->psrlq1regimm(RBX, RAX, 63);
	e->pslld1regimm(RBX, RBX, 31);

	e->psrlq1regimm(RAX, RAX, 29);
	e->por1regreg(RAX, RAX, RBX);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ESQRT(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = sqrt(vf(fsf))

	//e->movdqa1_regmem(RAX, (void*)&vs.u);
	e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vuw[i.fsf]);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	//e->vpbroadcastq2regmem(4, (int64_t*)&mul2_value);

	// load min/mask ->r5
	//e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// note: no sign because it is squared
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 33);
	e->psrlq1regimm(RAX, RAX, 4);
	e->vmulsd(RAX, RAX, RBX);
	e->sqrtsd(RAX, RAX);

	e->cvtsd2ss1regreg(RAX, RAX);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ESADD(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 10/11 cycles
	static const u64 c_CycleTime = 10;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = x^2 + y^2 + z^2

	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// note: no sign because it is squared
	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 33);
	e->psrlq2regimm(RAX, RAX, 4);
	e->vmulpd2(RCX, RAX, RBX);
	e->vmulpd2(RAX, RAX, RCX);


	// max+mask
	e->vminpd2(RAX, RAX, 4);
	e->pand2regreg(RAX, RAX, 5);

	// now add them together
	e->pshufd1regregimm(RCX, RAX, 0x0e);
	e->vextracti128regreg(RDX, RAX, 1);


	// rax+rcx+rdx
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);


	e->vaddsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->psrlq1regimm(RAX, RAX, 29);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ERSADD(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = 1 / (x^2 + y^2 + z^2)


	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// note: no sign because it is squared
	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 33);
	e->psrlq2regimm(RAX, RAX, 4);
	e->vmulpd2(RCX, RAX, RBX);
	e->vmulpd2(RAX, RAX, RCX);


	// max+mask
	e->vminpd2(RAX, RAX, 4);
	e->pand2regreg(RAX, RAX, 5);

	// now add them together
	e->pshufd1regregimm(RCX, RAX, 0x0e);
	e->vextracti128regreg(RDX, RAX, 1);


	// rax+rcx+rdx
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);


	e->vaddsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);

	e->movq1_regmem(RCX, (int64_t*)&dOne);
	e->movq1_regmem(RDX, (int64_t*)&div_cvt_value);
	e->vdivsd(RAX, RCX, RAX);
	e->vmulsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->psrlq1regimm(RAX, RAX, 29);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ESUM(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = x + y + z + w

	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	//e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 29);
	e->pand2regreg(RAX, RAX, 5);


	// split components
	e->vextracti128regreg(RCX, RAX, 1);

	// add
	e->vaddpd(RAX, RAX, RCX);

	// min/max
	e->vminpd(RAX, RAX, 4);
	e->vmaxpd(RAX, RAX, 5);

	// mask
	e->pand1regreg(RAX, RAX, 5);

	// split components
	e->pshufd1regregimm(RCX, RAX, 0x0e);

	// add
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->vmaxsd(RAX, RAX, 5);
	e->pand1regreg(RAX, RAX, 5);


	// shift
	e->psrlq1regimm(RAX, RAX, 29);

	// get sign
	e->psrlq1regimm(RCX, RAX, 3 + 31);
	e->pslld1regimm(RCX, RCX, 31);
	e->por1regreg(RAX, RAX, RCX);

	// store result (unless overflow, it appears ESUM clears lowest bit?)
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ELENG(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = sqrt(x^2 + y^2 + z^2)

	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// note: no sign because it is squared
	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 33);
	e->psrlq2regimm(RAX, RAX, 4);
	e->vmulpd2(RCX, RAX, RBX);
	e->vmulpd2(RAX, RAX, RCX);

	// max+mask
	e->vminpd2(RAX, RAX, 4);
	e->pand2regreg(RAX, RAX, 5);

	// now add them together
	e->pshufd1regregimm(RCX, RAX, 0x0e);
	e->vextracti128regreg(RDX, RAX, 1);


	// rax+rcx+rdx
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);


	e->vaddsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);
	e->vmulsd(RAX, RAX, RBX);
	e->vsqrtsd(RAX, RAX);
	e->cvtsd2ss1regreg(RAX, RAX);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}

int32_t Recompiler::ENCODE_ERLENG(x64Encoder* e, VU* v, Vu::Instruction::Format i)
{
	// 23/24 cycles
	static const u64 c_CycleTime = 23;

	int32_t ret = 1;

	// set the p-register
	//vi[REG_P].s = NextP.l;
	e->MovRegMem32(RAX, (int32_t*)&v->NextP.l);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RAX);

	//if (CycleCount < PBusyUntil_Cycle)
	//{
	//	CycleCount = PBusyUntil_Cycle;
	//}
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);
	e->MovRegMem64(RCX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmpRegReg64(RAX, RCX);
	e->CmovBRegReg64(RAX, RCX);

	//v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	e->AddReg64ImmX(RAX, c_CycleTime);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	//flresult.f = PS2_Float_Add(PS2_Float_Add(PS2_Float_Mul((float&)fsx, (float&)fsx, 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsy, (float&)fsy, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags), PS2_Float_Mul((float&)fsz, (float&)fsz, 0, &NoFlags, &NoFlags), 0, &NoFlags, &NoFlags);
	// p = 1 / sqrt(x^2 + y^2 + z^2)

	e->movdqa1_regmem(RAX, (void*)&v->vf[i.Fs].u);

	// load multiplier ->rbx
	e->vpbroadcastq2regmem(RBX, (int64_t*)&mul_value);

	// load max ->r4
	e->vpbroadcastq2regmem(4, (int64_t*)&max_value);

	// load min/mask ->r5
	e->vpbroadcastq2regmem(5, (int64_t*)&min_value);


	// note: no sign because it is squared
	e->pmovsxdq2regreg(RAX, RAX);
	e->psllq2regimm(RAX, RAX, 33);
	e->psrlq2regimm(RAX, RAX, 4);
	e->vmulpd2(RCX, RAX, RBX);
	e->vmulpd2(RAX, RAX, RCX);


	// max+mask
	e->vminpd2(RAX, RAX, 4);
	e->pand2regreg(RAX, RAX, 5);

	// now add them together
	e->pshufd1regregimm(RCX, RAX, 0x0e);
	e->vextracti128regreg(RDX, RAX, 1);


	// rax+rcx+rdx
	e->vaddsd(RAX, RAX, RCX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);


	e->vaddsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->pand1regreg(RAX, RAX, 5);
	e->vmulsd(RAX, RAX, RBX);
	e->vsqrtsd(RAX, RAX);
	e->cvtsd2ss1regreg(RAX, RAX);

	e->movq1_regmem(RCX, (int64_t*)&dOne);
	e->movq1_regmem(RDX, (int64_t*)&div_cvt_value);
	e->pmovsxdq1regreg(RAX, RAX);
	e->psllq1regimm(RAX, RAX, 29);
	e->vdivsd(RAX, RCX, RAX);
	e->vmulsd(RAX, RAX, RDX);
	e->vminsd(RAX, RAX, 4);
	e->psrlq1regimm(RAX, RAX, 29);

	// store result
	//e->movdqa_memreg((void*)&vd.u, RAX);
	ret = e->movd1_memreg(&v->NextP.l, RAX);

	return ret;
}



int32_t Recompiler::Generate_VABSp ( x64Encoder *e, VU* v, Vu::Instruction::Format i )
{
	int32_t ret;
	
	ret = 1;
	
	if ( i.Ft && i.xyzw )
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			e->movdqa1_regmem(RCX, &v->vf[i.Fs].sw0);

			//e->pslldregimm ( RCX, 1 );
			e->paddd1regreg(RCX, RCX, RCX );
			e->psrld1regimm(RCX, RCX, 1);

#ifdef USE_NEW_VECTOR_DISPATCH_VABS_VU

			ret = Dispatch_Result_AVX2(e, v, i, false, RCX, i.Ft);

#else
			if (i.xyzw != 0xf)
			{
				e->pblendw1regmemimm(RCX, RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RCX);

#endif

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
		else
		{
			if (!i.Fs)
			{
				e->movdqa_regmem(RCX, &v->vf[i.Fs].sw0);

				if (i.xyzw != 0xf)
				{
					e->pblendwregmemimm(RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}

				ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RCX);
			}
			else
			{
				e->movdqa_regmem(RCX, &v->vf[i.Fs].sw0);

				//e->pslldregimm ( RCX, 1 );
				e->padddregreg(RCX, RCX);
				e->psrldregimm(RCX, 1);

				if (i.xyzw != 0xf)
				{
					//e->pblendwregregimm ( RCX, RAX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm(RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}

				ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RCX);
			}

		}	// end else

	}	// end if ( i.Ft && i.xyzw )
	
	return ret;
}




int32_t Recompiler::Generate_VMAXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 *pFt, u32 FtComponent )
{
	//lfs = ( lfs >= 0 ) ? lfs : ~( lfs & 0x7fffffff );
	//lft = ( lft >= 0 ) ? lft : ~( lft & 0x7fffffff );
	// compare as integer and return original value?
	//fResult = ( ( lfs > lft ) ? fs : ft );
	int32_t ret;
	
	Instruction::Format64 ii;
	int32_t bc0;

	ii.Hi.Value = i.Value;
	//i1_64.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	ret = 1;
	
	
	if ( i.Fd && i.xyzw )
	{
#ifdef ENABLE_AVX2_MAX
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);

			if (isMAXBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMAXi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			/*
			if (!pFt)
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)pFt);
			}
			*/

			// load xyzw mask (reversed of course)
			e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));

			//e->psrad1regimm(RDX, RBX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RBX);
			e->psrad_rri128(RDX, RBX, 31);
			e->psrld_rri128(RDX, RDX, 1);
			e->pxor_rrr128(RDX, RDX, RBX);

			//e->psrad1regimm(RAX, RCX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RCX);
			e->psrad_rri128(RAX, RCX, 31);
			e->psrld_rri128(RAX, RAX, 1);
			e->pxor_rrr128(RAX, RAX, RCX);

			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr128(2, RAX, RDX);
			e->movdqa32_rr128(RBX, RCX, 2);

			//ret = e->movdqa1_memreg(&v->vf[i.Fd].sw0, RBX);
			ret = e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
		}
#ifdef ENABLE_AVX2_MAX
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

			e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);

			if (!pFt)
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				//e->movd1_regmem(RCX, (int32_t*)pFt);
				// need to "broadcast" the value in sse ??
				//e->pshufd1regregimm(RCX, RCX, 0);
				e->pshufd1regmemimm(RCX, (int32_t*)pFt, 0);
			}


			//e->movdqa_regreg(RDX, RBX);
			e->psrad1regimm(RDX, RBX, 31);
			e->psrld1regimm(RDX, RDX, 1);
			e->pxor1regreg(RDX, RDX, RBX);

			//e->movdqa_regreg(RAX, RCX);
			e->psrad1regimm(RAX, RCX, 31);
			e->psrld1regimm(RAX, RAX, 1);
			e->pxor1regreg(RAX, RAX, RCX);

			e->pcmpgtd1regreg(RAX, RAX, RDX);
			e->pblendvb1regreg(RBX, RBX, RCX, RAX);

#ifdef USE_NEW_VECTOR_DISPATCH_VMAX_VU

			ret = Dispatch_Result_AVX2(e, v, i, false, RBX, i.Fd);

#else
			if (i.xyzw != 0xf)
			{
				e->pblendw1regmemimm(RBX, RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa1_memreg(&v->vf[i.Fd].sw0, RBX);
#endif

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif
#ifdef ENABLE_VMAX_SSE
		else
		{

			e->movdqa_regmem(RBX, &v->vf[i.Fs].sw0);

			if (!pFt)
			{
				e->movdqa_regmem(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				e->movd_regmem(RCX, (int32_t*)pFt);
			}

			//if ( i.xyzw != 0xf )
			//{
			//	e->movdqa_regmem ( 5, & v->vf [ i.Fd ].sw0 );
			//}

			e->movdqa_regreg(RDX, RBX);
			//e->movdqa_regreg ( 4, RBX );
			//e->pslldregimm ( RDX, 1 );
			e->psradregimm(RDX, 31);
			e->psrldregimm(RDX, 1);
			e->pxorregreg(RDX, RBX);

			if (pFt)
			{
				// need to "broadcast" the value in sse ??
				e->pshufdregregimm(RCX, RCX, 0);
			}

			e->movdqa_regreg(RAX, RCX);
			//e->movdqa_regreg ( 4, RCX );
			//e->pslldregimm ( RAX, 1 );
			e->psradregimm(RAX, 31);
			e->psrldregimm(RAX, 1);
			e->pxorregreg(RAX, RCX);

			e->pcmpgtdregreg(RAX, RDX);
			e->pblendvbregreg(RBX, RCX);

			if (i.xyzw != 0xf)
			{
				//e->pblendwregregimm ( RBX, 5, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				e->pblendwregmemimm(RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa_memreg(&v->vf[i.Fd].sw0, RBX);

		}	// end else
#endif

	}	// end if ( i.Fd && i.xyzw )
	
	
	return ret;
}


int32_t Recompiler::Generate_XARGS1p(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX)
{
	int32_t ret = 1;

	if (i.Ft && i.xyzw)
	{
//#define ALLOW_AVX2_ARGS1
#ifdef ALLOW_AVX2_ARGS1
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			ret = e->OrMem32ImmX ((int32_t*)&v->xcomp[0], (xyzw_mask_lut32[i.xyzw]) << (iIndexX << 2));
		}
#ifdef ALLOW_AVX2_ARGS1
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			/*
			cout << "\n**********************";
			cout << "\Generate_XARGS1p";
			cout << "\ni.Fs: " << dec << i.Fs;
			cout << "\ni.Ft: " << dec << i.Ft;
			cout << "\ni.xyzw: " << hex << i.xyzw;
			cout << "\niIndexX: " << hex << iIndexX;
			cout << "\n**********************";
			cin.ignore();
			*/


			//e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);

			//e->movdqa1_memreg(&(v->xargs0[iIndexX].sw0), RBX);

			// set parallel computation to happen in the future
			//ret = e->OrMem32ImmX ((int32_t*)&v->xcomp, i.xyzw << (iIndexX<<2));
			ret = e->MovMemImm32((int32_t*)&v->xcomp[iIndexX], flag_mask_lut32[i.xyzw]);
		}
#endif	// end #ifdef ALLOW_AVX2_ARGS1
	}

	return ret;
}

int32_t Recompiler::Generate_XARGS2p(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX)
{
	int32_t ret = 1;

	if (i.xyzw)
	{
#ifdef ALLOW_AVX2_ARGS2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			ret = e->OrMem32ImmX((int32_t*)&v->xcomp[0], (xyzw_mask_lut32[i.xyzw]) << (iIndexX << 2));
		}
#ifdef ALLOW_AVX2_ARGS2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			// set parallel computation to happen in the future
			ret = e->MovMemImm32((int32_t*)&v->xcomp[iIndexX], flag_mask_lut32[i.xyzw]);
		}
#endif

	}

	return ret;
}


// for OPMULA
int32_t Recompiler::Generate_XARGS2op(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX)
{
	int32_t ret = 1;

	if (i.xyzw)
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			// 0x84, 0x60

			//e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);
			//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, 0x60);
			//e->pshufd1regmemimm(RAX, &v->vf[i.Ft].sw0, 0x84);
			e->pshufd1regmemimm(RBX, &v->vf[i.Fs].sw0, 0x09);
			e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, 0x12);

			e->movdqa1_memreg(&v->xargs0[iIndexX].sw0, RBX);
			e->movdqa1_memreg(&v->xargs1[iIndexX].sw0, RCX);
		}
		else
		{
		}

		// set parallel computation to happen in the future
		ret = e->OrMem32ImmX((int32_t*)&v->xcomp, i.xyzw << (iIndexX << 2));
	}

	return ret;
}

// for OPMSUB
int32_t Recompiler::Generate_XARGS2opm(x64Encoder* e, VU* v, Vu::Instruction::Format i, int iIndexX)
{
	int32_t ret = 1;

	if (i.xyzw)
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			// 0x84, 0x60

			//e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);
			//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, 0x60);
			//e->pshufd1regmemimm(RAX, &v->vf[i.Ft].sw0, 0x84);
			e->pshufd1regmemimm(RBX, &v->vf[i.Fs].sw0, 0x09);
			e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, 0x12);

			// make mask
			e->pcmpeqb1regreg(5, 5, 5);
			e->pslld1regimm(5, 5, 31);

			// reverse sign on second argument
			e->pxor1regreg(RCX, RCX, 5);


			e->movdqa1_memreg(&v->xargs0[iIndexX].sw0, RBX);
			e->movdqa1_memreg(&v->xargs1[iIndexX].sw0, RCX);
		}
		else
		{
		}

		// set parallel computation to happen in the future
		ret = e->OrMem32ImmX((int32_t*)&v->xcomp, i.xyzw << (iIndexX << 2));
	}

	return ret;
}




// for sub,msub,etc
int32_t Recompiler::Generate_XARGS2m(x64Encoder* e, VU* v, Vu::Instruction::Format i, u32* pFt, u32 FtComponent, int iIndexX)
{
	int32_t ret = 1;

	if (i.xyzw)
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);

			if (!pFt)
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				e->movd1_regmem(RCX, (int32_t*)pFt);
			}

			// make mask
			e->pcmpeqb1regreg(5, 5, 5);
			e->pslld1regimm(5, 5, 31);

			if (pFt)
			{
				// need to "broadcast" the value in sse ??
				e->pshufd1regregimm(RCX, RCX, 0);
			}

			// reverse sign on second argument
			e->pxor1regreg(RCX, RCX, 5);

			e->movdqa1_memreg(&v->xargs0[iIndexX].sw0, RBX);
			e->movdqa1_memreg(&v->xargs1[iIndexX].sw0, RCX);
		}
		else
		{
		}

		// set parallel computation to happen in the future
		ret = e->OrMem32ImmX((int32_t*)&v->xcomp, i.xyzw << (iIndexX << 2));
	}

	return ret;
}


int32_t Recompiler::Generate_VMAXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1, ii2, ii3;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;
	ii2.Hi.Value = i2.Value;
	ii3.Hi.Value = i3.Value;


	if ((i.Fd && i.xyzw) || (i1.Fd && i1.xyzw) || (i2.Fd && i2.xyzw) || (i3.Fd && i3.xyzw))
	{
#ifdef ALLOW_AVX2_MAXX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i3.Fs].sw0, 3);

			if (isMAXBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMAXi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMAXBC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			}
			else if (isMAXi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			if (isMAXBC(ii2))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i2.Ft].vuw[i2.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 2);
			}
			else if (isMAXi(ii2))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 2);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i2.Ft].sw0, 2);
			}

			if (isMAXBC(ii3))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i3.Ft].vuw[i3.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 3);
			}
			else if (isMAXi(ii3))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 3);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i3.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i3.Ft].sw0, 3);
			}


			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			//e->psrad1regimm(RDX, RBX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RBX);
			e->psrad_rri512(RDX, RBX, 31);
			e->psrld_rri512(RDX, RDX, 1);
			e->pxor_rrr512(RDX, RDX, RBX);

			//e->psrad1regimm(RAX, RCX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RCX);
			e->psrad_rri512(RAX, RCX, 31);
			e->psrld_rri512(RAX, RAX, 1);
			e->pxor_rrr512(RAX, RAX, RCX);

			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr512(2, RAX, RDX);
			e->movdqa32_rr512(RBX, RCX, 2);

			e->kshiftrd(7, 1, 12);
			e->vextracti32x4_mri512((void*)&v->vf[i3.Fd].uw0, RBX, 3, 7);
			e->kshiftrd(7, 1, 8);
			e->vextracti32x4_mri512((void*)&v->vf[i2.Fd].uw0, RBX, 2, 7);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri512((void*)&v->vf[i1.Fd].uw0, RBX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Fd].uw0, RBX, 1);
		}

		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);
	}

	return ret;
}


// i - the current instruction
// i1 - the previous instruction
int32_t Recompiler::Generate_VMAXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1 )
{
	//lfs = ( lfs >= 0 ) ? lfs : ~( lfs & 0x7fffffff );
	//lft = ( lft >= 0 ) ? lft : ~( lft & 0x7fffffff );
	// compare as integer and return original value?
	//fResult = ( ( lfs > lft ) ? fs : ft );
	int32_t ret = 1;

	Instruction::Format64 i_64, i1_64;
	Instruction::Format64 ii, ii1;
	int32_t bc0, bc1;

	i_64.Hi.Value = i.Value;
	i1_64.Hi.Value = i1.Value;
	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	if ((i.Fd && i.xyzw) || (i1.Fd && i1.xyzw))
	{
#ifdef ALLOW_AVX2_MAXX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi256(RBX, RBX, &v->vf[i1.Fs].sw0, 1);

			if (isMAXBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMAXi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMAXBC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
				e->vinserti32x4_rri256(RCX, RCX, RDX, 1);
			}
			else if (isMAXi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri256(RCX, RCX, RDX, 1);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi256(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			//e->psrad1regimm(RDX, RBX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RBX);
			e->psrad_rri256(RDX, RBX, 31);
			e->psrld_rri256(RDX, RDX, 1);
			e->pxor_rrr256(RDX, RDX, RBX);

			//e->psrad1regimm(RAX, RCX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RCX);
			e->psrad_rri256(RAX, RCX, 31);
			e->psrld_rri256(RAX, RAX, 1);
			e->pxor_rrr256(RAX, RAX, RCX);

			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr256(2, RAX, RDX);
			e->movdqa32_rr256(RBX, RCX, 2);

			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri256((void*)&v->vf[i1.Fd].uw0, RBX, 1, 7);
			ret = e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
		}
#ifdef ALLOW_AVX2_MAXX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			//e->movdqa2_regmem(RBX, &v->xargs0[0].sw0);
			//e->movdqa2_regmem(RCX, &v->xargs1[0].sw0);
			e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RBX, RBX, &v->vf[i1.Fs].sw0, 1);

			if (isMAXBC(i_64))
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, bc0);
			}
			else if (isMAXi(i_64))
			{
				e->pshufd1regmemimm(RCX, &v->vi[VU::REG_I].sw0, 0);
			}
			else
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMAXBC(i1_64))
			{
				e->pshufd1regmemimm(RDX, &v->vf[i1.Ft].sw0, bc1);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else if (isMAXi(i1_64))
			{
				e->pshufd1regmemimm(RDX, &v->vi[VU::REG_I].sw0, 0);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else
			{
				e->vinserti128regmem(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			//e->movdqa_regreg(RDX, RBX);
			e->psrad2regimm(RDX, RBX, 31);
			e->psrld2regimm(RDX, RDX, 1);
			e->pxor2regreg(RDX, RDX, RBX);

			//e->movdqa_regreg(RAX, RCX);
			e->psrad2regimm(RAX, RCX, 31);
			e->psrld2regimm(RAX, RAX, 1);
			e->pxor2regreg(RAX, RAX, RCX);

			e->pcmpgtd2regreg(RAX, RAX, RDX);
			e->pblendvb2regreg(RAX, RBX, RCX, RAX);

			//if ((i.xyzw != 0xf) || (i1.xyzw != 0xf))
			{
				// get previous values
				e->movdqa1_regmem(RBX, (void*)&v->vf[i.Fd].uw0);
				e->vinserti128regmem(RBX, RBX, (void*)&v->vf[i1.Fd].uw0, 1);

				// get mask
				e->vpmovsxbd2regmem(RCX, (int64_t*)&v->xcomp[0]);

				// merge result
				e->pblendvb2regreg(RAX, RBX, RAX, RCX);
			}

			// store result
			e->vextracti128memreg((void*)&v->vf[i1.Fd].uw0, RAX, 1);
			ret = e->movdqa1_memreg((void*)&v->vf[i.Fd].uw0, RAX);

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ALLOW_AVX2_MAXX2

		// prepare for the next parallel instruction
		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	}	// end if ( i.Fd && i.xyzw )


	return ret;
}


int32_t Recompiler::Generate_VMINpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1, ii2, ii3;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;
	ii2.Hi.Value = i2.Value;
	ii3.Hi.Value = i3.Value;


	if ((i.Fd && i.xyzw) || (i1.Fd && i1.xyzw) || (i2.Fd && i2.xyzw) || (i3.Fd && i3.xyzw))
	{
#ifdef ALLOW_AVX2_MINX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RBX, RBX, &v->vf[i3.Fs].sw0, 3);

			if (isMINBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMINi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMINBC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			}
			else if (isMINi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			if (isMINBC(ii2))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i2.Ft].vuw[i2.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 2);
			}
			else if (isMINi(ii2))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 2);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i2.Ft].sw0, 2);
			}

			if (isMINBC(ii3))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i3.Ft].vuw[i3.bc]);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 3);
			}
			else if (isMINi(ii3))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri512(RCX, RCX, RDX, 3);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i3.Ft].sw0);
				e->vinserti32x4_rmi512(RCX, RCX, &v->vf[i3.Ft].sw0, 3);
			}


			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			//e->psrad1regimm(RAX, RBX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RBX);
			e->psrad_rri512(RAX, RBX, 31);
			e->psrld_rri512(RAX, RAX, 1);
			e->pxor_rrr512(RAX, RAX, RBX);

			//e->psrad1regimm(RDX, RCX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RCX);
			e->psrad_rri512(RDX, RCX, 31);
			e->psrld_rri512(RDX, RDX, 1);
			e->pxor_rrr512(RDX, RDX, RCX);


			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr512(2, RAX, RDX);
			e->movdqa32_rr512(RBX, RCX, 2);

			e->kshiftrd(7, 1, 12);
			e->vextracti32x4_mri512((void*)&v->vf[i3.Fd].uw0, RBX, 3, 7);
			e->kshiftrd(7, 1, 8);
			e->vextracti32x4_mri512((void*)&v->vf[i2.Fd].uw0, RBX, 2, 7);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri512((void*)&v->vf[i1.Fd].uw0, RBX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Fd].uw0, RBX, 1);
		}

		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);
	}

	return ret;
}


int32_t Recompiler::Generate_VMINpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1)
{
	//lfs = ( lfs >= 0 ) ? lfs : ~( lfs & 0x7fffffff );
	//lft = ( lft >= 0 ) ? lft : ~( lft & 0x7fffffff );
	// compare as integer and return original value?
	//fResult = ( ( lfs > lft ) ? fs : ft );
	int32_t ret = 1;

	Instruction::Format64 i_64, i1_64;
	Instruction::Format64 ii, ii1;
	int32_t bc0, bc1;

	i_64.Hi.Value = i.Value;
	i1_64.Hi.Value = i1.Value;
	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	if ((i.Fd && i.xyzw) || (i1.Fd && i1.xyzw))
	{
#ifdef ALLOW_AVX2_MINX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi256(RBX, RBX, &v->vf[i1.Fs].sw0, 1);

			if (isMINBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMINi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMINBC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
				e->vinserti32x4_rri256(RCX, RCX, RDX, 1);
			}
			else if (isMINi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
				e->vinserti32x4_rri256(RCX, RCX, RDX, 1);
			}
			else
			{
				//e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
				e->vinserti32x4_rmi256(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			//e->psrad1regimm(RAX, RBX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RBX);
			e->psrad_rri256(RAX, RBX, 31);
			e->psrld_rri256(RAX, RAX, 1);
			e->pxor_rrr256(RAX, RAX, RBX);

			//e->psrad1regimm(RDX, RCX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RCX);
			e->psrad_rri256(RDX, RCX, 31);
			e->psrld_rri256(RDX, RDX, 1);
			e->pxor_rrr256(RDX, RDX, RCX);


			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr256(2, RAX, RDX);
			e->movdqa32_rr256(RBX, RCX, 2);

			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri256((void*)&v->vf[i1.Fd].uw0, RBX, 1, 7);
			ret = e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
		}
#ifdef ALLOW_AVX2_MINX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			//e->movdqa2_regmem(RBX, &v->xargs0[0].sw0);
			//e->movdqa2_regmem(RCX, &v->xargs1[0].sw0);
			e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RBX, RBX, &v->vf[i1.Fs].sw0, 1);

			if (isMINBC(i_64))
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, bc0);
			}
			else if (isMINi(i_64))
			{
				e->pshufd1regmemimm(RCX, &v->vi[VU::REG_I].sw0, 0);
			}
			else
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMINBC(i1_64))
			{
				e->pshufd1regmemimm(RDX, &v->vf[i1.Ft].sw0, bc1);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else if (isMINi(i1_64))
			{
				e->pshufd1regmemimm(RDX, &v->vi[VU::REG_I].sw0, 0);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else
			{
				e->vinserti128regmem(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			//e->movdqa_regreg(RAX, RBX);
			e->psrad2regimm(RAX, RBX, 31);
			e->psrld2regimm(RAX, RAX, 1);
			e->pxor2regreg(RAX, RAX, RBX);


			//e->movdqa_regreg(RDX, RCX);
			e->psrad2regimm(RDX, RCX, 31);
			e->psrld2regimm(RDX, RDX, 1);
			e->pxor2regreg(RDX, RDX, RCX);


			e->pcmpgtd2regreg(RAX, RAX, RDX);
			e->pblendvb2regreg(RAX, RBX, RCX, RAX);

			//if ((i.xyzw != 0xf) || (i1.xyzw != 0xf))
			{
				// get previous values
				e->movdqa1_regmem(RBX, (void*)&v->vf[i.Fd].uw0);
				e->vinserti128regmem(RBX, RBX, (void*)&v->vf[i1.Fd].uw0, 1);

				// get mask
				e->vpmovsxbd2regmem(RCX, (int64_t*)&v->xcomp[0]);

				// merge result
				e->pblendvb2regreg(RAX, RBX, RAX, RCX);
			}

			// store result
			e->vextracti128memreg((void*)&v->vf[i1.Fd].uw0, RAX, 1);
			ret = e->movdqa1_memreg((void*)&v->vf[i.Fd].uw0, RAX);

			/*
			if (i.xyzw != 0xf)
			{
				// todo: use avx2
				//e->pblendwregmemimm(RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pblendw1regmemimm(RBX, RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa1_memreg(&v->vf[i.Fd].sw0, RBX);
			*/

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ALLOW_AVX2_MINX2

		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	}	// end if ( i.Fd && i.xyzw )


	return ret;
}


int32_t Recompiler::Generate_VFTOIXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3, u32 IX)
{
	int32_t ret = 1;

	if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw) || (i2.Ft && i2.xyzw) || (i3.Ft && i3.xyzw))
	{
#ifdef ALLOW_AVX2_FTOIX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			//e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			//e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->movdqa32_rm128(RAX, &v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i3.Fs].sw0, 3);

			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// make mask -> R5
			//e->pcmpeqb1regreg(5, 5, 5);
			e->vpternlogd_rrr512(5, 5, 5, 0xff);

			// add IX to exponent -> RCX
			if (IX)
			{
				// make a mask 127 -> RDX
				//e->psrld1regimm(RDX, 5, 25);
				e->psrld_rri512(RDX, 5, 25);

				switch (IX)
				{
				case 4:
					//e->psrld1regimm(4, 5, 31);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri512(4, 5, 31);
					e->pslld_rri512(4, 4, 2);
					break;

				case 12:
					//e->psrld1regimm(4, 5, 30);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri512(4, 5, 30);
					e->pslld_rri512(4, 4, 2);
					break;

				case 15:
					//e->psrld1regimm(4, 5, 28);
					e->psrld_rri512(4, 5, 28);
					break;
				}

				// add 127 + IX -> RDX
				//e->paddd1regreg(RDX, RDX, 4);
				//e->pslld1regimm(RDX, RDX, 23);
				e->paddd_rrr512(RDX, RDX, 4);
				e->pslld_rri512(RDX, RDX, 23);

				// move decimal point over
				//e->vmulps(RAX, RAX, RDX);
				e->vmulps_rrr512(RAX, RAX, RDX);
			}


			// convert to int -> RCX
			//e->cvttps2dq1regreg(RCX, RAX);
			e->cvttps2dq_rr512(RCX, RAX);

			// make max value -> RDX
			//e->psrld1regimm(4, 5, 1);
			//e->psrld1regimm(RDX, RAX, 31);
			//e->paddd1regreg(RDX, RDX, 4);
			e->psrld_rri512(4, 5, 1);
			e->psrld_rri512(RDX, RAX, 31);
			e->paddd_rrr512(RDX, RDX, 4);

			// transfer max values -> RCX
			//e->pslld1regimm(5, 5, 31);
			//e->pcmpeqd1regreg(4, RCX, 5);
			//e->pblendvb1regreg(RAX, RCX, RDX, 4);
			e->pslld_rri512(5, 5, 31);
			e->vpcmpdeq_rrr512(2, RCX, 5);
			e->movdqa32_rr512(RCX, RDX, 2);


			// store result
			//e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RAX, 1);
			//ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RAX);
			e->kshiftrd(7, 1, 12);
			e->vextracti32x4_mri512((void*)&v->vf[i3.Ft].uw0, RCX, 3, 7);
			e->kshiftrd(7, 1, 8);
			e->vextracti32x4_mri512((void*)&v->vf[i2.Ft].uw0, RCX, 2, 7);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri512((void*)&v->vf[i1.Ft].uw0, RCX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Ft].uw0, RCX, 1);
		}

		// prepare for the next parallel instruction
		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);
	}

	return ret;
}

int32_t Recompiler::Generate_VFTOIXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, u32 IX)
{
	int32_t ret = 1;

	if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw))
	{
#ifdef ALLOW_AVX2_FTOIX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			//e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			//e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->movdqa32_rm128(RAX, &v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi256(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// make mask -> R5
			//e->pcmpeqb1regreg(5, 5, 5);
			e->vpternlogd_rrr256(5, 5, 5, 0xff);

			// add IX to exponent -> RCX
			if (IX)
			{
				// make a mask 127 -> RDX
				//e->psrld1regimm(RDX, 5, 25);
				e->psrld_rri256(RDX, 5, 25);

				switch (IX)
				{
				case 4:
					//e->psrld1regimm(4, 5, 31);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri256(4, 5, 31);
					e->pslld_rri256(4, 4, 2);
					break;

				case 12:
					//e->psrld1regimm(4, 5, 30);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri256(4, 5, 30);
					e->pslld_rri256(4, 4, 2);
					break;

				case 15:
					//e->psrld1regimm(4, 5, 28);
					e->psrld_rri256(4, 5, 28);
					break;
				}

				// add 127 + IX -> RDX
				//e->paddd1regreg(RDX, RDX, 4);
				//e->pslld1regimm(RDX, RDX, 23);
				e->paddd_rrr256(RDX, RDX, 4);
				e->pslld_rri256(RDX, RDX, 23);

				// move decimal point over
				//e->vmulps(RAX, RAX, RDX);
				e->vmulps_rrr256(RAX, RAX, RDX);
			}


			// convert to int -> RCX
			//e->cvttps2dq1regreg(RCX, RAX);
			e->cvttps2dq_rr256(RCX, RAX);

			// make max value -> RDX
			//e->psrld1regimm(4, 5, 1);
			//e->psrld1regimm(RDX, RAX, 31);
			//e->paddd1regreg(RDX, RDX, 4);
			e->psrld_rri256(4, 5, 1);
			e->psrld_rri256(RDX, RAX, 31);
			e->paddd_rrr256(RDX, RDX, 4);

			// transfer max values -> RCX
			//e->pslld1regimm(5, 5, 31);
			//e->pcmpeqd1regreg(4, RCX, 5);
			//e->pblendvb1regreg(RAX, RCX, RDX, 4);
			e->pslld_rri256(5, 5, 31);
			e->vpcmpdeq_rrr256(2, RCX, 5);
			e->movdqa32_rr256(RCX, RDX, 2);


			// store result
			//e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RAX, 1);
			//ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RAX);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri256((void*)&v->vf[i1.Ft].uw0, RCX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Ft].uw0, RCX, 1);
		}
#ifdef ALLOW_AVX2_FTOIX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			//e->movdqa2_regmem(RAX, &v->xargs0[0].sw0);
			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			// make mask -> R5
			e->pcmpeqb2regreg(5, 5, 5);

			// add IX to exponent -> RCX
			if (IX)
			{
				// make a mask 127 -> RDX
				e->psrld2regimm(RDX, 5, 25);

				switch (IX)
				{
				case 4:
					e->psrld2regimm(4, 5, 31);
					e->pslld2regimm(4, 4, 2);
					break;

				case 12:
					e->psrld2regimm(4, 5, 30);
					e->pslld2regimm(4, 4, 2);
					break;

				case 15:
					e->psrld2regimm(4, 5, 28);
					break;
				}

				// add 127 + IX -> RDX
				e->paddd2regreg(RDX, RDX, 4);
				e->pslld2regimm(RDX, RDX, 23);

				// move decimal point over
				e->vmulps2(RAX, RAX, RDX);

			}

			// convert to int -> RCX
			e->cvttps2dq2regreg(RCX, RAX);

			// make max value -> RDX
			e->psrld2regimm(4, 5, 1);
			e->psrld2regimm(RDX, RAX, 31);
			e->paddd2regreg(RDX, RDX, 4);

			// transfer max values -> RCX
			e->pslld2regimm(5, 5, 31);
			e->pcmpeqd2regreg(4, RCX, 5);
			e->pblendvb2regreg(RAX, RCX, RDX, 4);

			//if ((i.xyzw != 0xf) || (i1.xyzw != 0xf))
			{
				// get previous values
				e->movdqa1_regmem(RBX, (void*)&v->vf[i.Ft].uw0);
				e->vinserti128regmem(RBX, RBX, (void*)&v->vf[i1.Ft].uw0, 1);

				// get mask
				e->vpmovsxbd2regmem(RCX, (int64_t*) & v->xcomp[0]);

				// merge result
				e->pblendvb2regreg(RAX, RBX, RAX, RCX);
			}

			// store result
			e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RAX, 1);
			ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RAX);

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ALLOW_AVX2_FTOIX2

		// prepare for the next parallel instruction
		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	}	// end if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw))

	return ret;
}


int32_t Recompiler::Generate_VITOFXpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3, u64 FX)
{
	int32_t ret = 1;

	if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw) || (i2.Ft && i2.xyzw) || (i3.Ft && i3.xyzw))
	{
#ifdef ALLOW_AVX2_ITOFX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			//e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			//e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->movdqa32_rm128(RAX, &v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i3.Fs].sw0, 3);

			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			e->cvtdq2ps_rr512(RCX, RAX);

			if (FX)
			{
				// make constant 127 -> RDX
				//e->pcmpeqb2regreg(5, 5, 5);
				e->vpternlogd_rrr512(5, 5, 5, 0xff);


				switch (FX)
				{
				case 4:
					//e->psrld2regimm(4, 5, 31);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri512(4, 5, 31);
					e->pslld_rri512(4, 4, 2);
					break;

				case 12:
					//e->psrld2regimm(4, 5, 30);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri512(4, 5, 30);
					e->pslld_rri512(4, 4, 2);
					break;

				case 15:
					//e->psrld2regimm(4, 5, 28);
					e->psrld_rri512(4, 5, 28);
					break;
				}

				// subtract from exponent
				//e->pslld2regimm(RDX, 4, 23);
				//e->pxor2regreg(4, 4, 4);
				//e->pcmpeqd2regreg(RAX, RAX, 4);
				//e->pandn2regreg(RDX, RAX, RDX);
				//e->psubd2regreg(RCX, RCX, RDX);
				e->pslld_rri512(RDX, 4, 23);
				e->pxor_rrr512(4, 4, 4);
				e->vpcmpdne_rrr512(2, RAX, 4);
				//e->movdqa32_rr128(RDX, RDX, 2, 1);
				e->psubd_rrr512(RCX, RCX, RDX, 2, 1);
			}

			// store result
			//e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RCX, 1);
			//ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RCX);
			e->kshiftrd(7, 1, 12);
			e->vextracti32x4_mri512((void*)&v->vf[i3.Ft].uw0, RCX, 3, 7);
			e->kshiftrd(7, 1, 8);
			e->vextracti32x4_mri512((void*)&v->vf[i2.Ft].uw0, RCX, 2, 7);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri512((void*)&v->vf[i1.Ft].uw0, RCX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Ft].uw0, RCX, 1);
		}

		// prepare for the next parallel instruction
		ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);
	}

	return ret;
}


int32_t Recompiler::Generate_VITOFXpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, u64 FX)
{
	int32_t ret = 1;

	/*
	cout << "\n**********************";
	cout << "\nGenerate_VITOFXpx2 FX: " << dec << FX;
	cout << "\ni.Fs: " << dec << i.Fs << " i1.Fs: " << i1.Fs;
	cout << "\ni.Ft: " << dec << i.Ft << " i1.Ft: " << i1.Ft;
	cout << "\ni.xyzw: " << hex << i.xyzw << " i1.xyzw: " << i1.xyzw;
	cout << "\n**********************";
	cin.ignore();
	*/

	if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw))
	{
#ifdef ALLOW_AVX2_ITOFX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			//e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			//e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->movdqa32_rm128(RAX, &v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi256(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			e->cvtdq2ps_rr256(RCX, RAX);

			if (FX)
			{
				// make constant 127 -> RDX
				//e->pcmpeqb2regreg(5, 5, 5);
				e->vpternlogd_rrr256(5, 5, 5, 0xff);


				switch (FX)
				{
				case 4:
					//e->psrld2regimm(4, 5, 31);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri256(4, 5, 31);
					e->pslld_rri256(4, 4, 2);
					break;

				case 12:
					//e->psrld2regimm(4, 5, 30);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri256(4, 5, 30);
					e->pslld_rri256(4, 4, 2);
					break;

				case 15:
					//e->psrld2regimm(4, 5, 28);
					e->psrld_rri256(4, 5, 28);
					break;
				}

				// subtract from exponent
				//e->pslld2regimm(RDX, 4, 23);
				//e->pxor2regreg(4, 4, 4);
				//e->pcmpeqd2regreg(RAX, RAX, 4);
				//e->pandn2regreg(RDX, RAX, RDX);
				//e->psubd2regreg(RCX, RCX, RDX);
				e->pslld_rri256(RDX, 4, 23);
				e->pxor_rrr256(4, 4, 4);
				e->vpcmpdne_rrr256(2, RAX, 4);
				//e->movdqa32_rr128(RDX, RDX, 2, 1);
				e->psubd_rrr256(RCX, RCX, RDX, 2, 1);
			}

			// store result
			//e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RCX, 1);
			//ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RCX);
			e->kshiftrd(7, 1, 4);
			e->vextracti32x4_mri256((void*)&v->vf[i1.Ft].uw0, RCX, 1, 7);
			ret = e->movdqa32_mr128((void*)&v->vf[i.Ft].uw0, RCX, 1);
		}
#ifdef ALLOW_AVX2_ITOFX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			//e->movdqa2_regmem(RAX, &v->xargs0[0].sw0);
			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			// convert to float
			e->cvtdq2ps2regreg(RCX, RAX);

			if (FX)
			{
				// make constant 127 -> RDX
				e->pcmpeqb2regreg(5, 5, 5);

				switch (FX)
				{
				case 4:
					e->psrld2regimm(4, 5, 31);
					e->pslld2regimm(4, 4, 2);
					break;

				case 12:
					e->psrld2regimm(4, 5, 30);
					e->pslld2regimm(4, 4, 2);
					break;

				case 15:
					e->psrld2regimm(4, 5, 28);
					break;
				}

				// subtract from exponent
				e->pslld2regimm(RDX, 4, 23);
				e->pxor2regreg(4, 4, 4);
				e->pcmpeqd2regreg(RAX, RAX, 4);
				e->pandn2regreg(RDX, RAX, RDX);
				e->psubd2regreg(RCX, RCX, RDX);
			}

			//if ((i.xyzw != 0xf) || (i1.xyzw != 0xf))
			{
				// get previous values
				e->movdqa1_regmem(RBX, (void*)&v->vf[i.Ft].uw0);
				e->vinserti128regmem(RBX, RBX, (void*)&v->vf[i1.Ft].uw0, 1);

				// get mask
				e->vpmovsxbd2regmem(RDX, (int64_t*)&(v->xcomp[0]));

				// merge result
				e->pblendvb2regreg(RCX, RBX, RCX, RDX);
			}

			// store result
			e->vextracti128memreg((void*)&v->vf[i1.Ft].uw0, RCX, 1);
			ret = e->movdqa1_memreg((void*)&v->vf[i.Ft].uw0, RCX);

			// select result
			//if (i.xyzw != 0xf)
			//{
			//	// todo: use avx2
			//	e->pblendw1regmemimm(RCX, RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//}

			// store result
			//ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RCX);

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ALLOW_AVX2_ITOFX2

		// prepare for the next parallel instruction
		//ret = e->MovMemImm32((int32_t*)&v->xcomp, 0);
		ret = e->MovMemImm64((int64_t*)&(v->xcomp[0]), 0);

	}	// end if ((i.Ft && i.xyzw) || (i1.Ft && i1.xyzw))

	return ret;
}


int32_t Recompiler::Generate_VCLIPpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1)
{
	int32_t ret = 1;


#ifdef ALLOW_AVX2_CLIPX2
	if (iVectorType == VECTOR_TYPE_AVX512)
#endif
	{
		e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);
		e->vinserti32x4_rmi256(RBX, RBX, &v->vf[i1.Fs].sw0, 1);
		e->movdqa32_rm128(RCX, (void*)&v->vf[i.Ft].sw0);
		e->vinserti32x4_rmi256(RCX, RCX, &v->vf[i1.Ft].sw0, 1);

		// get fs zzzz ->rax
		e->pshufd_rri256(RAX, RBX, 0xaa);

		// get fs xxyy ->rbx
		e->pshufd_rri256(RBX, RBX, 0x50);

		// convert xxyyzz to integer
		e->psrad_rri256(4, RAX, 31);
		e->psrad_rri256(5, RBX, 31);
		e->psrld_rri256(4, 4, 1);
		e->psrld_rri256(5, 5, 1);
		e->pxor_rrr256(RAX, RAX, 4);
		e->pxor_rrr256(RBX, RBX, 5);

		// get ft wwww ->rcx
		e->pshufd_rri256(RCX, RCX, 0xff);

		// make sign mask
		e->vpternlogd_rrr256(5, 5, 5, 0xff);
		e->psrld_rri256(4, 5, 1);

		// +w ->rcx
		// -w ->rdx
		e->pand_rrr256(RDX, RDX, 4);
		e->pand_rrr256(RCX, RCX, 4);
		e->pxor_rrr256(RDX, RDX, 5);

		// get alternating mask
		e->MovReg32ImmX(RAX, 0xaaaa);
		e->kmovdmskreg(1, RAX);

		// xxyy(rbx) > +w(rcx) ->k2
		// xxyy(rbx) < -w(rdx) ->k3
		e->vpcmpdgt_rrr256(2, RBX, RCX);
		e->vpcmpdlt_rrr256(2, RBX, RDX, 1);

		// zzzz(rax) > +w(rcx) ->k2
		// zzzz(rax) < -w(rdx) ->k3
		e->vpcmpdgt_rrr256(3, RAX, RCX);
		e->vpcmpdlt_rrr256(3, RAX, RDX, 1);

		// pull into regs
		e->kmovdregmsk(8, 2);
		e->kmovdregmsk(9, 3);

		// start off the w
		e->ShlRegImm32(8, 4);

		// load clip flag
		e->MovRegMem32(RAX, &v->vi[VU::REG_CLIPFLAG].s);

		// combine x2
		e->MovRegReg32(RDX, 8);
		e->AndReg32ImmX(RDX, 0xf);
		e->OrRegReg32(RDX, 9);
		e->ShlRegImm32(RDX, 6);

		e->MovRegMem8(RCX, (char*)&((char*)v->xcomp)[0]);
		e->ShrRegReg32(RDX);
		e->ShlRegReg32(RAX);
		e->AndReg32ImmX(RDX, 0x3f);
		e->OrRegReg32(RAX, RDX);

		// combine again
		e->ShrRegImm32(8, 4);
		e->ShrRegImm32(9, 4);

		e->MovRegReg32(RDX, 8);
		e->AndReg32ImmX(RDX, 0xf);
		e->OrRegReg32(RDX, 9);
		e->ShlRegImm32(RDX, 6);

		e->MovRegMem8(RCX, (char*)&((char*)v->xcomp)[1]);
		e->ShrRegReg32(RDX);
		e->ShlRegReg32(RAX);
		e->AndReg32ImmX(RDX, 0x3f);
		e->OrRegReg32(RAX, RDX);

		// store clip flag
		e->MovMemReg32(&v->vi[VU::REG_CLIPFLAG].s, RAX);
	}
#ifdef ALLOW_AVX2_CLIPX2
	else if (iVectorType == VECTOR_TYPE_AVX2)
	{
		// load clip flag
		e->MovRegMem32(RAX, &v->vi[VU::REG_CLIPFLAG].s);

		//e->movdqa_regmem(RBX, &v->vf[i.Ft].sw0);
		e->movdqa2_regmem(RBX, &v->xargs0[0].sw0);
		e->movdqa2_regmem(RCX, &v->xargs1[0].sw0);

		//if (!i.Fs)
		//{
		//	e->pxorregreg(RAX, RAX);
		//}
		//else if (i.Fs == i.Ft)
		//{
		//	e->movdqa_regreg(RAX, RBX);
		//}
		//else
		//{
		//	e->movdqa_regmem(RAX, &v->vf[i.Fs].sw0);
		//}

		// get w from ft
		e->pshufd2regregimm(RBX, RBX, 0xff);


		// get +w into RBX
		e->pslld2regimm(RBX, RBX, 1);
		e->psrld2regimm(RBX, RBX, 1);

		// get -w into RCX
		e->pcmpeqb2regreg(RDX, RCX, RCX);
		//e->movdqa2_regreg(RDX, RCX);
		e->pxor2regreg(RCX, RBX, RDX);

		// get x,y from fs into RDX
		e->pshufd2regregimm(RDX, RAX, (1 << 6) | (1 << 4) | (0 << 2) | (0 << 0));
		//e->movdqa2_regreg(4, RDX);
		e->psrad2regimm(4, RDX, 31);
		e->psrld2regimm(4, 4, 1);
		e->pxor2regreg(RDX, RDX, 4);

		// get greater than +w into R4 and less than -w into R5
		//e->movdqa2_regreg(4, RDX);
		e->pcmpgtd2regreg(4, RDX, RBX);
		//e->movdqa2_regreg(5, RCX);
		e->pcmpgtd2regreg(5, RCX, RDX);

		// get x and y flags into R4
		e->pblendw2regregimm(4, 4, 5, 0xcc);


		// get z from fs into RAX
		e->pshufd2regregimm(RAX, RAX, (2 << 6) | (2 << 4) | (2 << 2) | (2 << 0));
		//e->movdqa2_regreg(5, RAX);
		e->psrad2regimm(5, RAX, 31);
		e->psrld2regimm(5, 5, 1);
		e->pxor2regreg(RAX, RAX, 5);

		// get greater than into RAX and less than into RCX
		e->pcmpgtd2regreg(RCX, RCX, RAX);
		e->pcmpgtd2regreg(RAX, RAX, RBX);

		// get z flags into RAX
		e->pblendw2regregimm(RAX, RAX, RCX, 0xcc);

		// pull flags
		e->movmskps2regreg(RCX, 4);
		e->movmskps2regreg(RDX, RAX);

		// check to make sure we executed the first one
		e->TestMem32ImmX((int32_t*)&v->xcomp, 0xf0);
		e->Jmp8_E(0, 0);

		// combine flags
		e->MovRegReg32(8, RDX);
		e->AndReg32ImmX(8, 0x30);
		e->MovRegReg32(9, RCX);
		e->ShrRegImm32(9, 4);
		e->OrRegReg32(8, 9);

		// shift clip flag
		e->ShlRegImm32(RAX, 6);

		// add flags
		e->OrRegReg32(RAX, 8);

		e->SetJmpTarget8(0);

		// combine flags
		e->ShlRegImm32(RDX, 4);
		e->AndReg32ImmX(RCX, 0xf);
		e->OrRegReg32(RCX, RDX);
		e->AndReg32ImmX(RCX, 0x3f);

		// combine into rest of the clipping flags
		e->ShlRegImm32(RAX, 6);
		e->OrRegReg32(RAX, RCX);
		e->AndReg32ImmX(RAX, 0x00ffffff);

		// write back to clipping flag
		e->MovMemReg32(&v->vi[VU::REG_CLIPFLAG].s, RAX);
	}
#endif

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	return ret;
}


int32_t Recompiler::Generate_VADDpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1, ii2, ii3;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;
	ii2.Hi.Value = i2.Value;
	ii3.Hi.Value = i3.Value;


	if (i.xyzw || i1.xyzw || i2.xyzw || i3.xyzw)
	{
#ifdef ALLOW_AVX2_ADDX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i3.Fs].sw0, 3);

			if (isADDBC(ii) || isADDABC(ii) || isSUBBC(ii) || isSUBABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isADDi(ii) || isADDAi(ii) || isSUBi(ii) || isSUBAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii) || isADDAq(ii) || isSUBq(ii) || isSUBAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isADDBC(ii1) || isADDABC(ii1) || isSUBBC(ii1) || isSUBABC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
			}
			else if (isADDi(ii1) || isADDAi(ii1) || isSUBi(ii1) || isSUBAi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii1) || isADDAq(ii1) || isSUBq(ii1) || isSUBAq(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
			}

			if (isADDBC(ii2) || isADDABC(ii2) || isSUBBC(ii2) || isSUBABC(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vf[i2.Ft].vuw[i2.bc]);
			}
			else if (isADDi(ii2) || isADDAi(ii2) || isSUBi(ii2) || isSUBAi(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii2) || isADDAq(ii2) || isSUBq(ii2) || isSUBAq(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(4, &v->vf[i2.Ft].sw0);
			}

			if (isADDBC(ii3) || isADDABC(ii3) || isSUBBC(ii3) || isSUBABC(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vf[i3.Ft].vuw[i3.bc]);
			}
			else if (isADDi(ii3) || isADDAi(ii3) || isSUBi(ii3) || isSUBAi(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii3) || isADDAq(ii3) || isSUBq(ii3) || isSUBAq(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(5, &v->vf[i3.Ft].sw0);
			}


			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// make zero ->r16
			e->pxor_rrr512(16, 16, 16);

			// get set mask ->r17
			e->vpternlogd_rrr512(17, 17, 17, 0xff);

			// make exp+mantissa mask ->r18
			e->psrld_rri512(18, 17, 1);

			// get sign mask ->r19
			e->pslld_rri512(19, 17, 31);

			// get mantissa mask ->r20
			e->psrld_rri512(20, 17, 9);

			// make exponent mask -> r21
			e->pxor_rrr512(21, 18, 20);

			// make exponent 25 ->r22
			e->MovReg32ImmX(RAX, 25 << 23);
			e->vpbroadcastd_rx512(22, RAX);

			// create mask

			// check if doing subtraction instead of addition
			if (isSUB(ii) || isSUBA(ii))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(RCX, RCX, 19);
			}
			if (isSUB(ii1) || isSUBA(ii1))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(RDX, RDX, 19);
			}
			if (isSUB(ii2) || isSUBA(ii2))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(4, 4, 19);
			}
			if (isSUB(ii3) || isSUBA(ii3))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(5, 5, 19);
			}

			e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			e->vinserti32x4_rri512(RCX, RCX, 4, 2);
			e->vinserti32x4_rri512(RCX, RCX, 5, 3);

			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn_rrr512(RBX, 19, RAX);
			e->pandn_rrr512(RDX, 19, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->vpcmpdgt_rrr512(2, RDX, RBX);
			e->vpblendmd_rrr512(RBX, RCX, RAX, 2);
			e->vpblendmd_rrr512(RAX, RAX, RCX, 2);


			// get max value exp and sub if non-zero
			e->pand_rrr512(RDX, RAX, 21);
			e->vpcmpdne_rrr512(2, RDX, 16);
			e->psubd_rrr512(RDX, RDX, 22, 2, 1);

			// sub to shift values over if non-zero
			e->psubd_rrr512(RAX, RAX, RDX);

			// check et for zero
			e->pand_rrr512(5, RBX, 21);
			e->vpcmpdne_rrr512(2, 5, 16);
			e->psubd_rrr512(RCX, RBX, RDX, 2, 1);
			e->pxor_rrr512(RBX, RBX, RCX);
			e->psrad_rri512(RBX, RBX, 31);
			e->pandn_rrr512(RBX, RBX, RCX);


			// float add
			e->vaddps_rrr512(RAX, RAX, RBX);

			// get zero ->r5
			e->pand_rrr512(5, 21, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->vpcmpdne_rrr512(5, 5, 16);
			e->movdqa32_rr512(RBX, RAX);
			e->paddd_rrr512(RBX, RBX, RDX, 5);


			// check for underflow/overflow ->rax
			e->pxor_rrr512(RAX, RAX, RBX);
			e->psrad_rri512(RAX, RAX, 31);
			e->pand_rrr512(RAX, RAX, RDX, 1, 1);

			// get underflow ->r4 ->k4
			// get overflow ->rdx ->k3
			// 0 ->rcx
			// overflow(k3) or underflow(k4)-> k6
			e->vpcmpdgt_rrr512(4, 16, RAX);
			e->vpcmpdgt_rrr512(3, RAX, 16);

			// toggle sign on under/over flow
			e->kord(6, 3, 4);
			e->pxor_rrr512(RBX, RBX, 19, 6);

			// zero flag ->r5 -> k5
			e->pand_rrr512(5, RBX, 21);
			e->vpcmpdeq_rrr512(5, 5, 16);

			// also zero on underflow
			e->kord(5, 5, 4);

			// clear on zero
			e->pand_rrr512(RBX, RBX, 19, 5);

			// max on overflow
			e->por_rrr512(RBX, RBX, 18, 3);


			// sign flag ->r4
			e->psrad_rri512(RAX, RBX, 31, 1, 1);


			e->kshiftrd(7, 1, 12);
			if (isADDA(ii3) || isSUBA(ii3))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RBX, 3, 7);
			}
			else
			{
				if (i3.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i3.Fd].sw0, RBX, 3, 7);
				}
			}

			e->kshiftrd(7, 1, 8);
			if (isADDA(ii2) || isSUBA(ii2))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RBX, 2, 7);
			}
			else
			{
				if (i2.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i2.Fd].sw0, RBX, 2, 7);
				}
			}

			e->kshiftrd(7, 1, 4);
			if (isADDA(ii1) || isSUBA(ii1))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RBX, 1, 7);
			}
			else
			{
				if (i1.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i1.Fd].sw0, RBX, 1, 7);
				}
			}

			if (isADDA(ii) || isSUBA(ii))
			{
				e->movdqa32_mr128((void*)&v->dACC[0].l, RBX, 1);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
				}
			}


			// overflow(k3)->rdx underflow(k4)->r4 sign->rax zero(k5)->r5
			e->kandd(5, 5, 1);
			e->vpmovm2d_rr512(RDX, 3);
			e->vpmovm2d_rr512(4, 4);
			e->vpmovm2d_rr512(5, 5);


			// put sign(rax) on top of zero(r5)
			e->vinserti32x4_rri256(RBX, 5, RAX, 1);

			// put overflow(rdx) on top of underflow(r4)
			e->vinserti32x4_rri256(RCX, 4, RDX, 1);

			// put overflow/underflow(rcx) on top of sign/zero(r5)
			e->vinserti32x8_rri512(RBX, RBX, RCX, 1);

			// shuffle xyzw
			e->pshufd_rri512(RBX, RBX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// make mask for mac flags ->k2
			e->vpmovd2m_rr512(2, RBX);

			// pull byte mask for stat flags ->k3
			e->vpmovm2b_rr128(RBX, 2);
			e->vpcmpdne_rrr128(3, RBX, 16);


			// -------x2 sticky flags

			// put sign(rax) on top of zero(r5)
			e->packsdw_rrr512(RAX, 5, RAX);

			// put overflow(rdx) on top of underflow(r4)
			e->packsdw_rrr512(RBX, 4, RDX);

			// put overflow/underflow(rbx) on top of sign/zero(rax)
			e->packsdw_rrr512(RAX, RAX, RBX);

			// another pack because 512-bit
			e->packsdw_rrr512(RAX, RAX, RAX);

			// make flags
			e->vpcmpqne_rrr256(4, RAX, 16);

			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->kmovdregmsk(RCX, 2);
			e->kmovdregmsk(RDX, 3);
			e->kmovdregmsk(RAX, 4);



			//e->kmovdregmsk(RCX, 2);
			//e->kmovdregmsk(RAX, 3);


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RDX);

				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )
		}

	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	return ret;
}


int32_t Recompiler::Generate_VADDpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1;
	int32_t bc0, bc1;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	if (i.xyzw || i1.xyzw)
	{
#ifdef ALLOW_AVX2_ADDX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi256(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			if (isADDBC(ii) || isADDABC(ii) || isSUBBC(ii) || isSUBABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isADDi(ii) || isADDAi(ii) || isSUBi(ii) || isSUBAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii) || isADDAq(ii) || isSUBq(ii) || isSUBAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			if (isADDBC(ii1) || isADDABC(ii1) || isSUBBC(ii1) || isSUBABC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
			}
			else if (isADDi(ii1) || isADDAi(ii1) || isSUBi(ii1) || isSUBAi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii1) || isADDAq(ii1) || isSUBq(ii1) || isSUBAq(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
			}


			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// make zero ->r16
			e->pxor_rrr256(16, 16, 16);

			// get set mask ->r17
			e->vpternlogd_rrr256(17, 17, 17, 0xff);

			// make exp+mantissa mask ->r18
			e->psrld_rri256(18, 17, 1);

			// get sign mask ->r19
			e->pslld_rri256(19, 17, 31);

			// get mantissa mask ->r20
			e->psrld_rri256(20, 17, 9);

			// make exponent mask -> r21
			e->pxor_rrr256(21, 18, 20);

			// make exponent 25 ->r22
			e->MovReg32ImmX(RAX, 25 << 23);
			e->vpbroadcastd_rx256(22, RAX);

			// create mask

			// check if doing subtraction instead of addition
			if (isSUB(ii) || isSUBA(ii))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(RCX, RCX, 19);
			}
			if (isSUB(ii1) || isSUBA(ii1))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				e->pxor_rrr128(RDX, RDX, 19);
			}

			e->vinserti32x4_rri256(RCX, RCX, RDX, 1);

			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn_rrr256(RBX, 19, RAX);
			e->pandn_rrr256(RDX, 19, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->vpcmpdgt_rrr256(2, RDX, RBX);
			e->vpblendmd_rrr256(RBX, RCX, RAX, 2);
			e->vpblendmd_rrr256(RAX, RAX, RCX, 2);


			// get max value exp and sub if non-zero
			e->pand_rrr256(RDX, RAX, 21);
			e->vpcmpdne_rrr256(2, RDX, 16);
			e->psubd_rrr256(RDX, RDX, 22, 2, 1);

			// sub to shift values over if non-zero
			e->psubd_rrr256(RAX, RAX, RDX);

			// check et for zero
			e->pand_rrr256(5, RBX, 21);
			e->vpcmpdne_rrr256(2, 5, 16);
			e->psubd_rrr256(RCX, RBX, RDX, 2, 1);
			e->pxor_rrr256(RBX, RBX, RCX);
			e->psrad_rri256(RBX, RBX, 31);
			e->pandn_rrr256(RBX, RBX, RCX);


			// float add
			e->vaddps_rrr256(RAX, RAX, RBX);

			// get zero ->r5
			e->pand_rrr256(5, 21, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->vpcmpdne_rrr256(5, 5, 16);
			e->movdqa32_rr256(RBX, RAX);
			e->paddd_rrr256(RBX, RBX, RDX, 5);


			// check for underflow/overflow ->rax
			e->pxor_rrr256(RAX, RAX, RBX);
			e->psrad_rri256(RAX, RAX, 31);
			e->pand_rrr256(RAX, RAX, RDX, 1, 1);

			// get underflow ->r4 ->k4
			// get overflow ->rdx ->k3
			// 0 ->rcx
			// overflow(k3) or underflow(k4)-> k6
			e->vpcmpdgt_rrr256(4, 16, RAX);
			e->vpcmpdgt_rrr256(3, RAX, 16);

			// toggle sign on under/over flow
			e->kord(6, 3, 4);
			e->pxor_rrr256(RBX, RBX, 19, 6);

			// zero flag ->r5 -> k5
			e->pand_rrr256(5, RBX, 21);
			e->vpcmpdeq_rrr256(5, 5, 16);

			// also zero on underflow
			e->kord(5, 5, 4);

			// clear on zero
			e->pand_rrr256(RBX, RBX, 19, 5);

			// max on overflow
			e->por_rrr256(RBX, RBX, 18, 3);


			// sign flag ->r4
			e->psrad_rri256(RAX, RBX, 31, 1, 1);


			e->kshiftrd(7, 1, 4);
			if (isADDA(ii1) || isSUBA(ii1))
			{
				//e->movdqa32_mr128((void*)&v->dACC[0].l, RAX, 7);
				e->vextracti32x4_mri256((void*)&v->dACC[0].l, RBX, 1, 7);
			}
			else
			{
				if (i1.Fd)
				{
					//e->movdqa32_mr128(&v->vf[i1.Fd].sw0, RAX, 7);
					e->vextracti32x4_mri256((void*)&v->vf[i1.Fd].sw0, RBX, 1, 7);
				}
			}

			if (isADDA(ii) || isSUBA(ii))
			{
				e->movdqa32_mr128((void*)&v->dACC[0].l, RBX, 1);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
				}
			}


			// overflow(k3)->rdx underflow(k4)->r4 sign->rax zero(k5)->r5
			e->kandd(5, 5, 1);
			e->vpmovm2d_rr256(RDX, 3);
			e->vpmovm2d_rr256(4, 4);
			e->vpmovm2d_rr256(5, 5);


			// put sign(rax) on top of zero(r5)
			e->vinserti32x4_rri256(RBX, 5, RAX, 1);

			// put overflow(rdx) on top of underflow(r4)
			e->vinserti32x4_rri256(RCX, 4, RDX, 1);

			// put overflow/underflow(rcx) on top of sign/zero(r5)
			e->vinserti32x8_rri512(RBX, RBX, RCX, 1);

			// shuffle xyzw
			e->pshufd_rri512(RBX, RBX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// make mask for mac flags ->k2
			e->vpmovd2m_rr512(2, RBX);

			// pull byte mask for stat flags ->k3
			e->vpmovm2b_rr128(RBX, 2);
			e->vpcmpdne_rrr128(3, RBX, 16);


			// -------x2 sticky flags

			// put sign(rax) on top of zero(r5)
			e->packsdw_rrr256(RAX, 5, RAX);

			// put overflow(rdx) on top of underflow(r4)
			e->packsdw_rrr256(RBX, 4, RDX);

			// put overflow/underflow(rbx) on top of sign/zero(rax)
			e->packsdw_rrr256(RAX, RAX, RBX);

			// make flags
			e->vpcmpqne_rrr256(4, RAX, 16);

			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->kmovdregmsk(RCX, 2);
			e->kmovdregmsk(RDX, 3);
			e->kmovdregmsk(RAX, 4);



			//e->kmovdregmsk(RCX, 2);
			//e->kmovdregmsk(RAX, 3);


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RDX);

				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )
		}
#ifdef ALLOW_AVX2_ADDX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			if (isADDBC(ii) || isADDABC(ii) || isSUBBC(ii) || isSUBABC(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, bc0);
			}
			else if (isADDi(ii) || isADDAi(ii) || isSUBi(ii) || isSUBAi(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vi[VU::REG_I].sw0, 0);
			}
			else
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}

			// create mask
			e->pcmpeqb2regreg(5, 5, 5);
			e->pslld2regimm(4, 5, 31);

			// negate if instruction0 is subtraction
			if (isSUB(ii) || isSUBA(ii))
			{
				e->pxor2regreg(RCX, RCX, 4);
			}


			if (isADDBC(ii1) || isADDABC(ii1) || isSUBBC(ii1) || isSUBABC(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vf[i1.Ft].sw0, bc1);
				//e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else if (isADDi(ii1) || isADDAi(ii1) || isSUBi(ii1) || isSUBAi(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vi[VU::REG_I].sw0, 0);
				//e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else
			{
				//e->vinserti128regmem(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
				e->movdqa1_regmem(RDX, &v->vf[i1.Ft].sw0);
			}

			// negate if instruction1 is subtraction
			if (isSUB(ii1) || isSUBA(ii1))
			{
				e->pxor2regreg(RDX, RDX, 4);
			}

			// combine ft
			e->vinserti128regreg(RCX, RCX, RDX, 1);

			// shuffle here for the flags for now
			//e->pshufd2regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			//e->pshufd2regregimm(RCX, RCX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));


			// check if doing subtraction instead of addition
			//if (bSub)
			//{
			//	// toggle the sign on RCX (right-hand argument) for subtraction for now
			//	e->pxor1regreg(RCX, RCX, 4);
			//}

			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn2regreg(RBX, 4, RAX);
			e->pandn2regreg(RDX, 4, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->pcmpgtd2regreg(RDX, RDX, RBX);
			e->pblendvb2regreg(RBX, RCX, RAX, RDX);
			e->pblendvb2regreg(RAX, RAX, RCX, RDX);



			// make exp mask ->r4
			// make exponent 127 ->r5
			// make exponent 24 ->r5
			e->psrld2regimm(4, 5, 24);
			e->pslld2regimm(4, 4, 23);
			e->MovReg32ImmX(RAX, 25 << 23);
			e->pinsrd2(5, 5, RAX, 0);
			//e->pshufd1regregimm(5, 5, 0);
			e->vpbroadcastd2regreg(5, 5);


			// get max value exp and sub if non-zero
			e->pand2regreg(RDX, RAX, 4);
			e->psignd2regreg(5, 5, RDX);
			e->psubd2regreg(RDX, RDX, 5);

			// sub to shift values over if non-zero
			e->psubd2regreg(RAX, RAX, RDX);

			// check et for zero
			e->pand2regreg(5, RBX, 4);
			e->psignd2regreg(RCX, RDX, 5);
			e->psubd2regreg(RCX, RBX, RCX);
			e->pxor2regreg(RBX, RBX, RCX);
			e->psrad2regimm(RBX, RBX, 31);
			e->pandn2regreg(RBX, RBX, RCX);


			// float add
			e->vaddps2(RAX, RAX, RBX);


			// get zero ->r5
			e->pand2regreg(5, 4, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->psignd2regreg(RDX, RDX, 5);
			e->paddd2regreg(RBX, RAX, RDX);


			// check zero
			e->pand2regreg(4, 4, RBX);
			e->psignd2regreg(5, 5, 4);


			// check for underflow/overflow ->rax
			e->pxor2regreg(RAX, RAX, RBX);
			e->psrad2regimm(RAX, RAX, 31);
			e->pand2regreg(RAX, RAX, RDX);

			// get underflow ->r4
			// get overflow ->rdx
			// 0 ->rcx
			e->pxor2regreg(RCX, RCX, RCX);
			e->pcmpgtd2regreg(4, RCX, RAX);
			e->pcmpgtd2regreg(RDX, RAX, RCX);

			// toggle sign on under/over flow
			e->pxor2regreg(RBX, RBX, RDX);
			e->pxor2regreg(RBX, RBX, 4);

			// zero flag ->r5
			e->pcmpeqd2regreg(5, 5, RCX);

			// also zero on underflow
			e->por2regreg(5, 5, 4);

			// not zero on overflow (could get a false reading from above)
			e->pandn2regreg(5, RDX, 5);

			// clear on zero
			e->psrld2regimm(RAX, 5, 1);
			e->pandn2regreg(RBX, RAX, RBX);


			// max on overflow
			e->psrld2regimm(RAX, RDX, 1);
			e->por2regreg(RBX, RBX, RAX);

			// sign flag ->rax
			e->psrad2regimm(RAX, RBX, 31);

			// set result
			//e->pshufd2regregimm(RBX, RBX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));




			// get registers in the order overflow(RDX),underflow(R4),sign(RAX),zero(R5)
			// 0 ->rcx
			e->packsdw2regreg(5, 5, RAX);
			e->packsdw2regreg(4, 4, RDX);

			// get mask -> RDX
			e->vpmovsxbd2regmem(RDX, (int64_t*)&v->xcomp[0]);
			//e->pshufd2regregimm(RDX, RDX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			//if (i.xyzw != 0xf)
			{
				//e->pblendw1regregimm(5, 5, RCX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				//e->pblendw1regregimm(4, 4, RCX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));

				// merge result
				//e->pblendvb2regreg(5, RCX, 5, RDX);
				//e->pblendvb2regreg(4, RCX, 4, RDX);
				e->pand2regreg(5, 5, RDX);
				e->pand2regreg(4, 4, RDX);
			}

			e->packswb2regreg(RAX, 5, 4);

			// get mac -> R5
			//if (i.xyzw != 0xf)
			{
				e->packuswb2regreg(5, 5, 4);
				e->por2regreg(5, RAX, 5);
			}

			// now get stat -> RAX
			e->psrld2regimm(RAX, RAX, 1);
			e->pcmpgtd2regreg(RAX, RAX, RCX);

			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->pshufb1regmem(5, 5, (void*)reverse_bytes_lut128);
			e->movmskb1regreg(RCX, 5);
			//e->movmskps1regreg(RAX, RAX);

			// new for avx2x2, put put the sticky flags into RAX and stat flags into RDX
			e->vextracti128regreg(RCX, RAX, 1);
			e->movmskps1regreg(RDX, RAX);
			e->por1regreg(RCX, RCX, RAX);
			e->movmskps1regreg(RAX, RCX);

			// get the result register (unable to have two MULA's in series here!)
			if (isADDA(ii) || isSUBA(ii))
			{
				e->movdqa1_regmem(5, (void*)&v->dACC[0].l);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_regmem(5, (void*)&v->vf[i.Fd].sw0);
				}
			}

			if (isADDA(ii1) || isSUBA(ii1))
			{
				e->vinserti128regmem(5, 5, (void*)&v->dACC[0].l, 1);
			}
			else
			{
				if (i1.Fd)
				{
					e->vinserti128regmem(5, 5, (void*)&v->vf[i1.Fd].sw0, 1);
				}
			}

			// merge w/ result
			//e->pshufd2regregimm(RDX, RDX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->pblendvb2regreg(RBX, 5, RBX, RDX);


			// store result (RBX)
			if (isADDA(ii1) || isSUBA(ii1))
			{
				e->vextracti128memreg((void*)&v->dACC[0].l, RBX, 1);
			}
			else
			{
				if (i1.Fd)
				{
					e->vextracti128memreg((void*)&v->vf[i1.Fd].sw0, RBX, 1);
				}
			}

			if (isADDA(ii) || isSUBA(ii))
			{
				e->movdqa1_memreg((void*)&v->dACC[0].l, RBX);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg((void*)&v->vf[i.Fd].sw0, RBX);
				}
			}


			// store mac
			// set MAC flags (RCX)
			//e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);


			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				//e->OrRegReg32(RAX, RCX);
				e->OrRegReg32(RAX, RDX);

				//ret = e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);
			}	// end if ( !v->SetStatus_Flag )

		}
#endif

	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	return ret;
}


// no opmula, because it goes with opmsub
int32_t Recompiler::Generate_VMULpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1, ii2, ii3;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;
	ii2.Hi.Value = i2.Value;
	ii3.Hi.Value = i3.Value;

	// *** note: excluding opmula here, will pair with opmsub *** //

	if (i.xyzw || i1.xyzw || i2.xyzw || i3.xyzw)
	{
#ifdef ALLOW_AVX2_MULX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i3.Fs].sw0, 3);


			if (isMULBC(ii) || isMULABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMULi(ii) || isMULAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii) || isMULAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}


			if (isMULBC(ii1) || isMULABC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
			}
			else if (isMULi(ii1) || isMULAi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii1) || isMULAq(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
			}

			if (isMULBC(ii2) || isMULABC(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vf[i2.Ft].vuw[i2.bc]);
			}
			else if (isMULi(ii2) || isMULAi(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii2) || isMULAq(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(4, &v->vf[i2.Ft].sw0);
			}

			if (isMULBC(ii3) || isMULABC(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vf[i3.Ft].vuw[i3.bc]);
			}
			else if (isMULi(ii3) || isMULAi(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii3) || isMULAq(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(5, &v->vf[i3.Ft].sw0);
			}


			e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			e->vinserti32x4_rri512(RCX, RCX, 4, 2);
			e->vinserti32x4_rri512(RCX, RCX, 5, 3);


			// load xyzw mask (reversed of course)
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// make zero ->r16 (0x00000000)
			e->pxor_rrr512(16, 16, 16);

			// get set mask ->r17 (0xffffffff)
			e->vpternlogd_rrr512(17, 17, 17, 0xff);

			// make exp+mantissa mask ->r18 (0x7fffffff)
			e->psrld_rri512(18, 17, 1);

			// make sign flag ->r19
			e->pslld_rri512(19, 17, 31);

			// get mantissa mask ->r20 (0x007fffff)
			e->psrld_rri512(20, 17, 9);

			// make exponent mask -> r21 (0x7f800000)
			e->pxor_rrr512(21, 18, 20);

			// make 127 ->r22
			e->psrld_rri512(22, 17, 25);


			// check mantissa
			e->pand_rrr512(RDX, RCX, 20);
			e->vpcmpdeq_rrr512(2, RDX, 20);
			e->paddd_rrr512(RCX, RCX, 17, 2);


			// get es ->rbx
			// get et ->rdx
			e->pand_rrr512(RDX, RAX, 21);
			e->vpcmpdne_rrr512(2, RDX, 16);
			e->pand_rrr512(RBX, RAX, 18, 2, 1);
			e->pand_rrr512(RDX, RCX, 21, 2, 1);
			e->vpcmpdne_rrr512(2, RDX, 16);
			e->pand_rrr512(RDX, RDX, 18, 2, 1);

			// top es bit ->r4
			e->psrld_rri512(4, RBX, 30);
			e->pslld_rri512(4, 4, 23);
			e->psubd_rrr512(RAX, RAX, 4);

			// get ed=es+et ->rbx
			e->paddd_rrr512(RBX, RBX, RDX, 2, 1);

			// top et bit ->rdx
			e->psrld_rri512(RDX, RDX, 30);
			e->pslld_rri512(RDX, RDX, 23);
			e->psubd_rrr512(RCX, RCX, RDX);


			// do the multiply ->rax
			e->vmulps_rrr512(RAX, RAX, RCX);

			// add top bits
			e->paddd_rrr512(RDX, RDX, 4, 2, 1);


			// zero mask if intermediate exponent is zero (if zero but not underflow)
			e->psrld_rri512(RBX, RBX, 23);


			// ed-=127
			e->psubd_rrr512(RBX, RBX, 22, 2, 1);

			// get zero flag
			e->vpcmpdle_rrr512(5, RBX, 16);

			// get underflow ->rcx
			e->psrad_rri512(RCX, RBX, 31, 1, 1);

			// get overflow ->rbx
			e->pandn_rrr512(RBX, RCX, RBX, 1, 1);
			e->pslld_rri512(RBX, RBX, 23);
			e->psrad_rri512(RBX, RBX, 31);

			// clear top bits on overflow
			e->pandn_rrr512(RDX, RBX, RDX);

			// add top bits to result ->rdx
			e->paddd_rrr512(RAX, RAX, RDX);


			// max on overflow
			e->psrld_rri512(4, RBX, 1);
			e->por_rrr512(RAX, RAX, 4);


			// clear result on zero
			e->pand_rrr512(RAX, RAX, 19, 5);

			// sign flag ->r4
			//e->psrad1regimm(4, RAX, 31);
			e->psrad_rri512(4, RAX, 31, 1, 1);


			e->kshiftrd(7, 1, 12);
			if (isMULA(ii3))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RAX, 3, 7);
			}
			else
			{
				if (i3.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i3.Fd].sw0, RAX, 3, 7);
				}
			}

			e->kshiftrd(7, 1, 8);
			if (isMULA(ii2))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RAX, 2, 7);
			}
			else
			{
				if (i2.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i2.Fd].sw0, RAX, 2, 7);
				}
			}

			e->kshiftrd(7, 1, 4);
			if (isMULA(ii1))
			{
				e->vextracti32x4_mri512((void*)&v->dACC[0].l, RAX, 1, 7);
			}
			else
			{
				if (i1.Fd)
				{
					e->vextracti32x4_mri512((void*)&v->vf[i1.Fd].sw0, RAX, 1, 7);
				}
			}

			if (isMULA(ii))
			{
				e->movdqa32_mr128((void*)&v->dACC[0].l, RAX, 1);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RAX, 1);
				}
			}



			// overflow->rbx underflow->rcx sign->r4 zero->rdx

			// get registers in the order overflow(RBX),underflow(RCX),sign(R4),zero(RDX)
			// get zero flag
			e->kandd(5, 5, 1);
			e->vpmovm2d_rr512(RDX, 5);


			// put sign(r4) on top of zero(rdx)
			e->vinserti32x4_rri256(RAX, RDX, 4, 1);

			// put overflow(rbx) on top of underflow(rcx)
			e->vinserti32x4_rri256(5, RCX, RBX, 1);

			// put overflow/underflow(rcx) on top of sign/zero(rdx)
			e->vinserti32x8_rri512(RAX, RAX, 5, 1);

			// shuffle xyzw
			e->pshufd_rri512(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// make mask for mac flags ->k2
			e->vpmovd2m_rr512(2, RAX);

			// pull byte mask for stat flags ->k3
			e->vpmovm2b_rr128(RAX, 2);
			e->vpcmpdne_rrr128(3, RAX, 16);


			// ---- x2 sticky flags ------

			// put sign(r4) on top of zero(rdx)
			e->packsdw_rrr512(RDX, RDX, 4);

			// put overflow(rbx) on top of underflow(rcx)
			e->packsdw_rrr512(RCX, RCX, RBX);

			// put overflow/underflow(rcx) on top of sign/zero(rdx)
			e->packsdw_rrr512(RDX, RDX, RCX);

			// need to pack again because 512-bit
			e->packsdw_rrr512(RDX, RDX, RDX);

			// make flags
			e->vpcmpqne_rrr256(4, RDX, 16);

			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->kmovdregmsk(RCX, 2);
			e->kmovdregmsk(RDX, 3);
			e->kmovdregmsk(RAX, 4);


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RDX);

				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

		}
	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	return ret;
}



int32_t Recompiler::Generate_VMULpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1;
	int32_t bc0, bc1;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;

	//bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	if (i.xyzw || i1.xyzw)
	{
#ifdef ALLOW_AVX2_MULX2
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			if (isOPMULA(ii))
			{
				e->pshufd_rmi128(RAX, &v->vf[i.Fs].sw0, 0x09);
			}
			else
			{
				e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			}

			if (isOPMULA(ii1))
			{
				e->pshufd_rmi128(RBX, &v->vf[i1.Fs].sw0, 0x09);
			}
			else
			{
				e->movdqa32_rm128(RBX, (void*)&v->vf[i1.Fs].sw0);
			}


			if (isMULBC(ii) || isMULABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMULi(ii) || isMULAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii) || isMULAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else if (isOPMULA(ii))
			{
				e->pshufd_rmi128(RCX, &v->vf[i.Ft].sw0, 0x12);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}


			if (isMULBC(ii1) || isMULABC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
			}
			else if (isMULi(ii1) || isMULAi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii1) || isMULAq(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else if (isOPMULA(ii1))
			{
				e->pshufd_rmi128(RDX, &v->vf[i1.Ft].sw0, 0x12);
			}
			else
			{
				e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
			}


			e->vinserti32x4_rri256(RAX, RAX, RBX, 1);
			e->vinserti32x4_rri256(RCX, RCX, RDX, 1);


			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);
			
			// make zero ->r16 (0x00000000)
			e->pxor_rrr256(16, 16, 16);

			// get set mask ->r17 (0xffffffff)
			e->vpternlogd_rrr256(17, 17, 17, 0xff);

			// make exp+mantissa mask ->r18 (0x7fffffff)
			e->psrld_rri256(18, 17, 1);

			// make sign flag ->r19
			e->pslld_rri256(19, 17, 31);

			// get mantissa mask ->r20 (0x007fffff)
			e->psrld_rri256(20, 17, 9);

			// make exponent mask -> r21 (0x7f800000)
			e->pxor_rrr256(21, 18, 20);

			// make 127 ->r22
			e->psrld_rri256(22, 17, 25);


			// check mantissa
			e->pand_rrr256(RDX, RCX, 20);
			e->vpcmpdeq_rrr256(2, RDX, 20);
			e->paddd_rrr256(RCX, RCX, 17, 2);


			// get es ->rbx
			// get et ->rdx
			e->pand_rrr256(RDX, RAX, 21);
			e->vpcmpdne_rrr256(2, RDX, 16);
			e->pand_rrr256(RBX, RAX, 18, 2, 1);
			e->pand_rrr256(RDX, RCX, 21, 2, 1);
			e->vpcmpdne_rrr256(2, RDX, 16);
			e->pand_rrr256(RDX, RDX, 18, 2, 1);

			// top es bit ->r4
			e->psrld_rri256(4, RBX, 30);
			e->pslld_rri256(4, 4, 23);
			e->psubd_rrr256(RAX, RAX, 4);

			// get ed=es+et ->rbx
			e->paddd_rrr256(RBX, RBX, RDX, 2, 1);

			// top et bit ->rdx
			e->psrld_rri256(RDX, RDX, 30);
			e->pslld_rri256(RDX, RDX, 23);
			e->psubd_rrr256(RCX, RCX, RDX);


			// do the multiply ->rax
			e->vmulps_rrr256(RAX, RAX, RCX);

			// add top bits
			e->paddd_rrr256(RDX, RDX, 4, 2, 1);


			// zero mask if intermediate exponent is zero (if zero but not underflow)
			e->psrld_rri256(RBX, RBX, 23);


			// ed-=127
			e->psubd_rrr256(RBX, RBX, 22, 2, 1);

			// get zero flag
			e->vpcmpdle_rrr256(5, RBX, 16);

			// get underflow ->rcx
			e->psrad_rri256(RCX, RBX, 31, 1, 1);

			// get overflow ->rbx
			e->pandn_rrr256(RBX, RCX, RBX, 1, 1);
			e->pslld_rri256(RBX, RBX, 23);
			e->psrad_rri256(RBX, RBX, 31);

			// clear top bits on overflow
			e->pandn_rrr256(RDX, RBX, RDX);

			// add top bits to result ->rdx
			e->paddd_rrr256(RAX, RAX, RDX);


			// max on overflow
			e->psrld_rri256(4, RBX, 1);
			e->por_rrr256(RAX, RAX, 4);


			// clear result on zero
			e->pand_rrr256(RAX, RAX, 19, 5);

			// sign flag ->r4
			//e->psrad1regimm(4, RAX, 31);
			e->psrad_rri256(4, RAX, 31, 1, 1);


			e->kshiftrd(7, 1, 4);
			if (isMULA(ii1))
			{
				//e->movdqa32_mr128((void*)&v->dACC[0].l, RAX, 7);
				e->vextracti32x4_mri256((void*)&v->dACC[0].l, RAX, 1, 7);
			}
			else
			{
				if (i1.Fd)
				{
					//e->movdqa32_mr128(&v->vf[i1.Fd].sw0, RAX, 7);
					e->vextracti32x4_mri256((void*)&v->vf[i1.Fd].sw0, RAX, 1, 7);
				}
			}

			//if (pFd)
			if (isMULA(ii))
			{
				e->movdqa32_mr128((void*)&v->dACC[0].l, RAX, 1);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RAX, 1);
				}
			}



			// overflow->rbx underflow->rcx sign->r4 zero->rdx

			// get registers in the order overflow(RBX),underflow(RCX),sign(R4),zero(RDX)
			// get zero flag
			e->kandd(5, 5, 1);
			e->vpmovm2d_rr256(RDX, 5);


			// put sign(r4) on top of zero(rdx)
			e->vinserti32x4_rri256(RAX, RDX, 4, 1);

			// put overflow(rbx) on top of underflow(rcx)
			e->vinserti32x4_rri256(5, RCX, RBX, 1);

			// put overflow/underflow(rcx) on top of sign/zero(rdx)
			e->vinserti32x8_rri512(RAX, RAX, 5, 1);

			// shuffle xyzw
			e->pshufd_rri512(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// make mask for mac flags ->k2
			e->vpmovd2m_rr512(2, RAX);

			// pull byte mask for stat flags ->k3
			e->vpmovm2b_rr128(RAX, 2);
			e->vpcmpdne_rrr128(3, RAX, 16);


			// ---- x2 sticky flags ------

			// put sign(r4) on top of zero(rdx)
			e->packsdw_rrr256(RDX, RDX, 4);

			// put overflow(rbx) on top of underflow(rcx)
			e->packsdw_rrr256(RCX, RCX, RBX);

			// put overflow/underflow(rcx) on top of sign/zero(rdx)
			e->packsdw_rrr256(RDX, RDX, RCX);

			// make flags
			e->vpcmpqne_rrr256(4, RDX, 16);

			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->kmovdregmsk(RCX, 2);
			e->kmovdregmsk(RDX, 3);
			e->kmovdregmsk(RAX, 4);


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				//e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RDX);

				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

		}
#ifdef ALLOW_AVX2_MULX2
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{
			// get args //
			
			//e->pshufd2regmemimm(RAX, &v->xargs0[0].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			//e->pshufd2regmemimm(RCX, &v->xargs1[0].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			//e->movdqa2_regmem(RBX, &v->xargs0[0].sw0);
			//e->movdqa2_regmem(RCX, &v->xargs1[0].sw0);
			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			if (isMULBC(ii) || isMULABC(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, bc0);
			}
			else if (isMULi(ii) || isMULAi(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vi[VU::REG_I].sw0, 0);
			}
			else
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMULBC(ii1) || isMULABC(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vf[i1.Ft].sw0, bc1);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else if (isMULi(ii1) || isMULAi(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vi[VU::REG_I].sw0, 0);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else
			{
				e->vinserti128regmem(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			// shuffle here for the flags for now
			//e->pshufd2regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			//e->pshufd2regregimm(RCX, RCX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));


			// do mul //

			// get mantissa mask ->r4
			e->pcmpeqb2regreg(5, 5, 5);
			e->psrld2regimm(4, 5, 9);

			// check mantissa
			e->pand2regreg(RDX, RCX, 4);
			e->pcmpeqd2regreg(RDX, RDX, 4);
			e->paddd2regreg(RCX, RCX, RDX);


			// get exp+mantissa mask ->r5
			e->psrld2regimm(5, 5, 1);

			// get es ->rbx
			// get et ->rdx
			e->pand2regreg(RBX, RAX, 5);
			e->pandn2regreg(RDX, 4, RBX);
			e->psignd2regreg(RBX, RBX, RDX);
			e->pand2regreg(RDX, RCX, 5);
			e->pandn2regreg(4, 4, RDX);
			e->psignd2regreg(RDX, RDX, 4);

			// top es bit ->r4
			e->psrld2regimm(4, RBX, 30);
			e->pslld2regimm(4, 4, 23);
			e->psubd2regreg(RAX, RAX, 4);

			// get ed=es+et ->rbx
			e->paddd2regreg(RBX, RBX, RDX);

			// top et bit ->rdx
			e->psrld2regimm(RDX, RDX, 30);
			e->pslld2regimm(RDX, RDX, 23);
			e->psubd2regreg(RCX, RCX, RDX);


			// do the multiply ->rax
			e->vmulps2(RAX, RAX, RCX);


			// add top bits
			e->paddd2regreg(RDX, RDX, 4);

			// clear top bits on zero result (if zero or underflow)
			e->pand2regreg(4, RAX, 5);
			e->psignd2regreg(RDX, RDX, 4);

			// get mask 127 ->r5
			e->psrld2regimm(5, 5, 24);

			// zero mask if intermediate exponent is zero (if zero but not underflow)
			e->psrld2regimm(RBX, RBX, 23);
			e->psignd2regreg(5, 5, RBX);

			// ed-=127
			e->psubd2regreg(RBX, RBX, 5);

			// get underflow ->rcx
			e->psrad2regimm(RCX, RBX, 31);

			// add top bits to result ->rdx
			e->paddd2regreg(RDX, RDX, RAX);

			// overflow ->rbx
			e->pxor2regreg(RBX, RDX, RAX);
			e->psrad2regimm(RBX, RBX, 31);

			// max and toggle sign on overflow ->rax
			// result ->rax
			e->pxor2regreg(RDX, RDX, RBX);
			e->psrld2regimm(5, RBX, 1);
			e->por2regreg(RAX, RDX, 5);

			// zero flag ->rdx
			// 0 ->r5
			e->pxor2regreg(5, 5, 5);
			e->pcmpeqd2regreg(RDX, 4, 5);

			// sign flag ->r4
			e->psrad2regimm(4, RAX, 31);

			// set result
			//e->pshufd2regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));


			// overflow->rbx underflow->rcx sign->r4 zero->rdx 0->r5

			// get registers in the order overflow(RBX),underflow(RAX),sign(RDX),zero(RCX)
			e->packsdw2regreg(RDX, RDX, 4);
			e->packsdw2regreg(RBX, RCX, RBX);

			// get mask
			e->vpmovsxbd2regmem(4, (int64_t*)&v->xcomp[0]);
			//e->pshufd2regregimm(4, 4, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			//if (i.xyzw != 0xf)
			{
				//e->pblendw1regregimm(RDX, RDX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				//e->pblendw1regregimm(RBX, RBX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));

				// merge result
				//e->pblendvb2regreg(RDX, 5, RDX, 4);
				//e->pblendvb2regreg(RBX, 5, RBX, 4);
				e->pand2regreg(RDX, RDX, 4);
				e->pand2regreg(RBX, RBX, 4);
			}

			e->packswb2regreg(RCX, RDX, RBX);

			//if (i.xyzw != 0xf)
			{
				e->packuswb2regreg(RBX, RDX, RBX);
				e->por2regreg(RCX, RCX, RBX);
			}

			// now get stat
			e->psrld2regimm(RDX, RCX, 1);
			e->pcmpgtd2regreg(RDX, RDX, 5);


			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->pshufb1regmem(RCX, RCX, (void*)reverse_bytes_lut128);
			e->movmskb1regreg(RCX, RCX);

			// new for avx2x2, put put the sticky flags into RAX and stat flags into RDX
			e->vextracti128regreg(RCX, RDX, 1);
			e->movmskps1regreg(RDX, RDX);
			e->por1regreg(RCX, RCX, RDX);
			e->movmskps1regreg(RAX, RCX);

			// get the result register (unable to have two MULA's in series here!)
			if (isMULA(ii))
			{
				e->movdqa1_regmem(5, (void*)&v->dACC[0].l);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_regmem(5, (void*)&v->vf[i.Fd].sw0);
				}
			}

			if (isMULA(ii1))
			{
				e->vinserti128regmem(5, 5, (void*)&v->dACC[0].l, 1);
			}
			else
			{
				if (i1.Fd)
				{
					e->vinserti128regmem(5, 5, (void*)&v->vf[i1.Fd].sw0, 1);
				}
			}

			// merge w/ result
			//e->pshufd2regregimm(4, 4, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->pblendvb2regreg(RAX, 5, RAX, 4);


			if (isMULA(ii1))
			{
				//e->movdqa1_memreg(pFd, RAX);
				e->vextracti128memreg((void*)&v->dACC[0].l, RAX, 1);
			}
			else
			{
				if (i1.Fd)
				{
					//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
					e->vextracti128memreg((void*)&v->vf[i1.Fd].sw0, RAX, 1);
				}
			}

			if (isMULA(ii))
			{
				//e->movdqa1_memreg(pFd, RAX);
				e->movdqa1_memreg((void*)&v->dACC[0].l, RAX);
			}
			else
			{
				if (i.Fd)
				{
					//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
					e->movdqa1_memreg((void*)&v->vf[i.Fd].sw0, RAX);
				}
			}


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				//e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				//e->OrRegReg32(RAX, RCX);
				e->OrRegReg32(RAX, RDX);

				//ret = e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )
		}
#endif


	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp[0], 0);

	return ret;
}




int32_t Recompiler::Generate_VMADDpx2(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1;
	int32_t bc0, bc1;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	if (i.xyzw || i1.xyzw)
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			// get args //

			//e->pshufd2regmemimm(RAX, &v->xargs0[0].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			//e->pshufd2regmemimm(RCX, &v->xargs1[0].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			//e->movdqa2_regmem(RBX, &v->xargs0[0].sw0);
			//e->movdqa2_regmem(RCX, &v->xargs1[0].sw0);
			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);
			e->vinserti128regmem(RAX, RAX, &v->vf[i1.Fs].sw0, 1);

			if (isMULBC(ii) || isMULABC(ii) || isMADDBC(ii) || isMADDABC(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, bc0);
			}
			else if (isMULi(ii) || isMULAi(ii) || isMADDi(ii) || isMADDAi(ii))
			{
				e->pshufd1regmemimm(RCX, &v->vi[VU::REG_I].sw0, 0);
			}
			else
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}

			if (isMULBC(ii1) || isMULABC(ii1) || isMADDBC(ii1) || isMADDABC(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vf[i1.Ft].sw0, bc1);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else if (isMULi(ii1) || isMULAi(ii1) || isMADDi(ii1) || isMADDAi(ii1))
			{
				e->pshufd1regmemimm(RDX, &v->vi[VU::REG_I].sw0, 0);
				e->vinserti128regreg(RCX, RCX, RDX, 1);
			}
			else
			{
				e->vinserti128regmem(RCX, RCX, &v->vf[i1.Ft].sw0, 1);
			}

			// shuffle here for the flags for now
			//e->pshufd2regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			//e->pshufd2regregimm(RCX, RCX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));


			// do mul //

			// get mantissa mask ->r4
			e->pcmpeqb2regreg(5, 5, 5);
			e->psrld2regimm(4, 5, 9);

			// check mantissa
			e->pand2regreg(RDX, RCX, 4);
			e->pcmpeqd2regreg(RDX, RDX, 4);
			e->paddd2regreg(RCX, RCX, RDX);


			// get exp+mantissa mask ->r5
			e->psrld2regimm(5, 5, 1);

			// get es ->rbx
			// get et ->rdx
			e->pand2regreg(RBX, RAX, 5);
			e->pandn2regreg(RDX, 4, RBX);
			e->psignd2regreg(RBX, RBX, RDX);
			e->pand2regreg(RDX, RCX, 5);
			e->pandn2regreg(4, 4, RDX);
			e->psignd2regreg(RDX, RDX, 4);

			// top es bit ->r4
			e->psrld2regimm(4, RBX, 30);
			e->pslld2regimm(4, 4, 23);
			e->psubd2regreg(RAX, RAX, 4);

			// get ed=es+et ->rbx
			e->paddd2regreg(RBX, RBX, RDX);

			// top et bit ->rdx
			e->psrld2regimm(RDX, RDX, 30);
			e->pslld2regimm(RDX, RDX, 23);
			e->psubd2regreg(RCX, RCX, RDX);


			// do the multiply ->rax
			e->vmulps2(RAX, RAX, RCX);


			// add top bits
			e->paddd2regreg(RDX, RDX, 4);

			// clear top bits on zero result (if zero or underflow)
			e->pand2regreg(4, RAX, 5);
			e->psignd2regreg(RDX, RDX, 4);

			// get mask 127 ->r5
			e->psrld2regimm(5, 5, 24);

			// zero mask if intermediate exponent is zero (if zero but not underflow)
			e->psrld2regimm(RBX, RBX, 23);
			e->psignd2regreg(5, 5, RBX);

			// ed-=127
			e->psubd2regreg(RBX, RBX, 5);

			// get underflow ->rcx
			e->psrad2regimm(RCX, RBX, 31);

			// add top bits to result ->rdx
			e->paddd2regreg(RDX, RDX, RAX);

			// overflow ->rbx
			e->pxor2regreg(RBX, RDX, RAX);
			e->psrad2regimm(RBX, RBX, 31);

			// max and toggle sign on overflow ->rax
			// result ->rax
			e->pxor2regreg(RDX, RDX, RBX);
			e->psrld2regimm(5, RBX, 1);
			e->por2regreg(RAX, RDX, 5);

			// zero flag ->rdx
			// 0 ->r5
			//e->pxor2regreg(5, 5, 5);
			//e->pcmpeqd2regreg(RDX, 4, 5);

			// sign flag ->r4
			//e->psrad2regimm(4, RAX, 31);

			// set result
			//e->pshufd2regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));




			// overflow->rbx underflow->rcx sign->r4 zero->rdx 0->r5

			// get registers in the order overflow(RBX),underflow(RAX),sign(RDX),zero(RCX)
			//e->packsdw2regreg(RDX, RDX, 4);
			//e->packsdw2regreg(RBX, RCX, RBX);

			// get mask
			e->vpmovsxbd2regmem(4, (int64_t*)&v->xcomp[0]);
			//e->pshufd2regregimm(4, 4, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// mask to zero unwanted components
			e->pand2regreg(RCX, RCX, 4);

			// get sticky stat flag for mul underflow(RCX) only
			e->vextracti128regreg(RBX, RCX, 1);
			e->por1regreg(RBX, RBX, RCX);
			e->movmskps1regreg(8, RBX);

			/*
			//if (i.xyzw != 0xf)
			{
				//e->pblendw1regregimm(RDX, RDX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				//e->pblendw1regregimm(RBX, RBX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));

				// merge result
				e->pblendvb2regreg(RDX, 5, RDX, 4);
				e->pblendvb2regreg(RBX, 5, RBX, 4);
			}

			e->packswb2regreg(RCX, RDX, RBX);

			//if (i.xyzw != 0xf)
			{
				e->packuswb2regreg(RBX, RDX, RBX);
				e->por2regreg(RCX, RCX, RBX);
			}

			// now get stat
			e->psrld2regimm(RDX, RCX, 1);
			e->pcmpgtd2regreg(RDX, RDX, 5);


			// now pull mac(R5)->RCX and stat(RAX)->RAX
			e->pshufb1regmem(RCX, RCX, (void*)reverse_bytes_lut128);
			e->movmskb1regreg(RCX, RCX);

			// new for avx2x2, put put the sticky flags into RAX and stat flags into RDX
			e->vextracti128regreg(RCX, RDX, 1);
			e->movmskps1regreg(RDX, RDX);
			e->por1regreg(RCX, RCX, RDX);
			e->movmskps1regreg(RAX, RCX);
			*/

			// get the result register (unable to have two MULA's in series here!)
			if (isMULA(ii) || isMADDA(ii))
			{
				e->movdqa1_regmem(5, (void*)&v->dACC[0].l);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_regmem(5, (void*)&v->vf[i.Fd].sw0);
				}
			}

			if (isMULA(ii1) || isMADDA(ii1))
			{
				e->vinserti128regmem(5, 5, (void*)&v->dACC[0].l, 1);
			}
			else
			{
				if (i1.Fd)
				{
					e->vinserti128regmem(5, 5, (void*)&v->vf[i1.Fd].sw0, 1);
				}
			}

			// merge w/ result
			//e->pshufd2regregimm(4, 4, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->pblendvb2regreg(RAX, 5, RAX, 4);


			if (isMULA(ii1))
			{
				//e->movdqa1_memreg(pFd, RAX);
				e->vextracti128memreg((void*)&v->dACC[0].l, RAX, 1);
			}
			else
			{
				if (i1.Fd)
				{
					//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
					e->vextracti128memreg((void*)&v->vf[i1.Fd].sw0, RAX, 1);
				}
			}

			if (isMULA(ii))
			{
				//e->movdqa1_memreg(pFd, RAX);
				e->movdqa1_memreg((void*)&v->dACC[0].l, RAX);
			}
			else
			{
				if (i.Fd)
				{
					//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
					e->movdqa1_memreg((void*)&v->vf[i.Fd].sw0, RAX);
				}
			}


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				//e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				//e->OrRegReg32(RAX, RCX);
				e->OrRegReg32(RAX, RDX);

				//ret = e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

		}
		else
		{
		}


	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp[0], 0);

	return ret;
}





int32_t Recompiler::Generate_VMINp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 *pFt, u32 FtComponent )
{
	//lfs = ( lfs >= 0 ) ? lfs : ~( lfs & 0x7fffffff );
	//lft = ( lft >= 0 ) ? lft : ~( lft & 0x7fffffff );
	// compare as integer and return original value?
	//fResult = ( ( lfs > lft ) ? fs : ft );
	int32_t ret;
	
	Instruction::Format64 ii;
	//int32_t bc0;

	ii.Hi.Value = i.Value;
	//i1_64.Hi.Value = i1.Value;

	//bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	ret = 1;
	
	if ( i.Fd && i.xyzw )
	{
#ifdef ENABLE_AVX2_MIN
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RBX, (void*)&v->vf[i.Fs].sw0);

			if (isMINBC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
				//e->vpbroadcastd_rm128(RCX, (int32_t*)pFt);
			}
			else if (isMINi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
				//e->vpbroadcastd_rm128(RCX, (int32_t*)pFt);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			/*
			if (!pFt)
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)pFt);
			}
			*/


			// load xyzw mask (reversed of course)
			e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));

			//e->psrad1regimm(RAX, RBX, 31);
			//e->psrld1regimm(RAX, RAX, 1);
			//e->pxor1regreg(RAX, RAX, RBX);
			e->psrad_rri128(RAX, RBX, 31);
			e->psrld_rri128(RAX, RAX, 1);
			e->pxor_rrr128(RAX, RAX, RBX);

			//e->psrad1regimm(RDX, RCX, 31);
			//e->psrld1regimm(RDX, RDX, 1);
			//e->pxor1regreg(RDX, RDX, RCX);
			e->psrad_rri128(RDX, RCX, 31);
			e->psrld_rri128(RDX, RDX, 1);
			e->pxor_rrr128(RDX, RDX, RCX);


			//e->pcmpgtd1regreg(RAX, RAX, RDX);
			//e->pblendvb1regreg(RBX, RBX, RCX, RAX);
			e->vpcmpdgt_rrr128(2, RAX, RDX);
			e->movdqa32_rr128(RBX, RCX, 2);

			ret = e->movdqa32_mr128(&v->vf[i.Fd].sw0, RBX, 1);
		}
#ifdef ENABLE_AVX2_MIN
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

			e->movdqa1_regmem(RBX, &v->vf[i.Fs].sw0);

			if (!pFt)
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				//e->movd1_regmem(RCX, (int32_t*)pFt);
				// need to "broadcast" the value in sse ??
				//e->pshufd1regregimm(RCX, RCX, 0);
				e->pshufd1regmemimm(RCX, (int32_t*)pFt, 0);
			}


			//e->movdqa_regreg(RAX, RBX);
			e->psrad1regimm(RAX, RBX, 31);
			e->psrld1regimm(RAX, RAX, 1);
			e->pxor1regreg(RAX, RAX, RBX);


			//e->movdqa_regreg(RDX, RCX);
			e->psrad1regimm(RDX, RCX, 31);
			e->psrld1regimm(RDX, RDX, 1);
			e->pxor1regreg(RDX, RDX, RCX);


			e->pcmpgtd1regreg(RAX, RAX, RDX);
			e->pblendvb1regreg(RBX, RBX, RCX, RAX);

#ifdef USE_NEW_VECTOR_DISPATCH_VMIN_VU

			ret = Dispatch_Result_AVX2(e, v, i, false, RBX, i.Fd);

#else
			if (i.xyzw != 0xf)
			{
				// todo: use avx2
				//e->pblendwregmemimm(RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pblendw1regmemimm(RBX, RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa1_memreg(&v->vf[i.Fd].sw0, RBX);
#endif

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end ENABLE_AVX2_MIN
#ifdef ENABLE_VMIN_SSE
		else
		{

			e->movdqa_regmem(RBX, &v->vf[i.Fs].sw0);

			if (!pFt)
			{
				e->movdqa_regmem(RCX, &v->vf[i.Ft].sw0);
			}
			else
			{
				e->movd_regmem(RCX, (int32_t*)pFt);
			}

			//if ( i.xyzw != 0xf )
			//{
			//	e->movdqa_regmem ( 5, & v->vf [ i.Fd ].sw0 );
			//}

			e->movdqa_regreg(RAX, RBX);
			//e->movdqa_regreg ( 4, RBX );
			//e->pslldregimm ( RAX, 1 );
			e->psradregimm(RAX, 31);
			e->psrldregimm(RAX, 1);
			e->pxorregreg(RAX, RBX);


			if (pFt)
			{
				// need to "broadcast" the value in sse ??
				e->pshufdregregimm(RCX, RCX, 0);
			}

			e->movdqa_regreg(RDX, RCX);
			//e->movdqa_regreg ( 4, RCX );
			//e->pslldregimm ( RDX, 1 );
			e->psradregimm(RDX, 31);
			e->psrldregimm(RDX, 1);
			e->pxorregreg(RDX, RCX);


			e->pcmpgtdregreg(RAX, RDX);
			e->pblendvbregreg(RBX, RCX);

			if (i.xyzw != 0xf)
			{
				//e->pblendwregregimm ( RBX, 5, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				e->pblendwregmemimm(RBX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa_memreg(&v->vf[i.Fd].sw0, RBX);

		}	// end else
#endif

	}	// end if ( i.Fd && i.xyzw )
	
	
	return ret;
}






int32_t Recompiler::Generate_VFTOIXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 IX )
{
	int32_t ret;
	
	ret = 1;

	if ( i.Ft && i.xyzw )
	{
#ifdef ENABLE_AVX2_FTOIX
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);

			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&vs.sw0);
			e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));

			// make mask -> R5
			//e->pcmpeqb1regreg(5, 5, 5);
			e->vpternlogd_rrr128(5, 5, 5, 0xff);

			// add IX to exponent -> RCX
			if (IX)
			{
				// make a mask 127 -> RDX
				//e->psrld1regimm(RDX, 5, 25);
				e->psrld_rri128(RDX, 5, 25);

				switch (IX)
				{
				case 4:
					//e->psrld1regimm(4, 5, 31);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri128(4, 5, 31);
					e->pslld_rri128(4, 4, 2);
					break;

				case 12:
					//e->psrld1regimm(4, 5, 30);
					//e->pslld1regimm(4, 4, 2);
					e->psrld_rri128(4, 5, 30);
					e->pslld_rri128(4, 4, 2);
					break;

				case 15:
					//e->psrld1regimm(4, 5, 28);
					e->psrld_rri128(4, 5, 28);
					break;
				}

				// add 127 + IX -> RDX
				//e->paddd1regreg(RDX, RDX, 4);
				//e->pslld1regimm(RDX, RDX, 23);
				e->paddd_rrr128(RDX, RDX, 4);
				e->pslld_rri128(RDX, RDX, 23);

				// move decimal point over
				//e->vmulps(RAX, RAX, RDX);
				e->vmulps_rrr128(RAX, RAX, RDX);
			}


			// convert to int -> RCX
			//e->cvttps2dq1regreg(RCX, RAX);
			e->cvttps2dq_rr128(RCX, RAX);

			// make max value -> RDX
			//e->psrld1regimm(4, 5, 1);
			//e->psrld1regimm(RDX, RAX, 31);
			//e->paddd1regreg(RDX, RDX, 4);
			e->psrld_rri128(4, 5, 1);
			e->psrld_rri128(RDX, RAX, 31);
			e->paddd_rrr128(RDX, RDX, 4);

			// transfer max values -> RCX
			//e->pslld1regimm(5, 5, 31);
			//e->pcmpeqd1regreg(4, RCX, 5);
			//e->pblendvb1regreg(RAX, RCX, RDX, 4);
			e->pslld_rri128(5, 5, 31);
			e->vpcmpdeq_rrr128(2, RCX, 5);
			e->movdqa32_rr128(RCX, RDX, 2);

			// store result
			e->movdqa32_mr128((void*)&v->vf[i.Ft].sw0, RCX, 1);
		}
#ifdef ENABLE_AVX2_FTOIX
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);

#ifdef USE_NEW_FTOIX_AVX2_VU

			if (IX)
			{
				e->MovReg32ImmX(RAX, (127 + IX) << 23);
				e->movd_to_sse128(RCX, RAX);
				e->vpbroadcastd1regreg(RCX, RCX);
				e->vmulps(RAX, RAX, RCX);
			}

			e->cvttps2dq1regreg(RCX, RAX);
			e->pxor1regreg(RDX, RAX, RCX);
			e->psrld1regimm(RDX, RDX, 31);
			e->psignd1regreg(RDX, RDX, RCX);
			e->psubd1regreg(RAX, RCX, RDX);

#else

			// make mask -> R5
			e->pcmpeqb1regreg(5, 5, 5);

			// add IX to exponent -> RCX
			if (IX)
			{
				// make a mask 127 -> RDX
				//e->psrld1regimm(RDX, 5, 25);

				switch (IX)
				{
				case 4:
					e->psrld1regimm(4, 5, 31);
					e->pslld1regimm(4, 4, (2 + 23));
					//e->pslld1regimm(4, 4, 2);
					break;

				case 12:
					e->psrld1regimm(4, 5, 30);
					e->pslld1regimm(4, 4, (2 + 23));
					//e->pslld1regimm(4, 4, 2);
					break;

				case 15:
					e->psrld1regimm(4, 5, 28);
					e->pslld1regimm(4, 4, (0 + 23));
					break;
				}

				// add 127 + IX -> RDX
				//e->paddd1regreg(RDX, RDX, 4);
				//e->pslld1regimm(RDX, RDX, 23);

				// move decimal point over
				//e->vmulps(RAX, RAX, RDX);

				// add and max on overflow
				e->paddd1regreg(RBX, RAX, 4);

				// check for overflow
				e->pxor1regreg(RCX, RAX, RBX);
				e->psrad1regimm(RCX, RCX, 31);
				e->psrld1regimm(RCX, RCX, 1);

				// max on overflow
				e->por1regreg(RAX, RBX, RCX);

			}

			// make sure exp is less or equal to 0x4e800000
			e->MovReg32ImmX(RAX, 0x9d);
			e->movd_to_sse128(RBX, RAX);
			e->pshufd1regregimm(RBX, RBX, 0);

			// make sure value in range
			e->paddd1regreg(RCX, RAX, RAX);
			e->psrld1regimm(RCX, RCX, 24);
			e->pcmpgtd1regreg(RCX, RCX, RBX);

			// save sign and clear if outside range
			e->psrld1regimm(RDX, RAX, 31);
			e->pandn1regreg(RAX, RCX, RAX);
			e->pand1regreg(RDX, RDX, RCX);

			// make mask
			e->psrld1regimm(RCX, RCX, 1);
			e->paddd1regreg(RDX, RDX, RCX);
 
			// convert to int -> RCX
			e->cvttps2dq1regreg(RCX, RAX);

			// or with mask
			e->por1regreg(RAX, RCX, RDX);

			// make max value -> RDX
			//e->psrld1regimm(4, 5, 1);
			//e->psrld1regimm(RDX, RAX, 31);
			//e->paddd1regreg(RDX, RDX, 4);

			// transfer max values -> RCX
			//e->pslld1regimm(5, 5, 31);
			//e->pcmpeqd1regreg(4, RCX, 5);
			//e->pblendvb1regreg(RAX, RCX, RDX, 4);

#endif


#ifdef USE_NEW_VECTOR_DISPATCH_FTOIX_VU

			ret = Dispatch_Result_AVX2(e, v, i, false, RAX, i.Ft);

#else
			if (i.xyzw != 0xf)
			{
				e->pblendw1regmemimm(RAX, RAX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			// get result
			ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RAX);
#endif

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ENABLE_AVX2_FTOIX
#ifdef ENABLE_FTOIX_SSE
		else
		{

			//e->MovRegMem32 ( RAX, ( &v->vf [ i.Fs ].sw0 ) + FsComponent );
			e->movdqa_regmem(RBX, &v->vf[i.Fs].sw0);


			if (IX)
			{
				e->MovRegImm32(RAX, IX << 23);
				e->movd_to_sse(RCX, RAX);
				e->pshufdregregimm(RCX, RCX, 0);
				e->padddregreg(RCX, RBX);
			}
			else
			{
				//e->MovRegReg32 ( RCX, RAX );
				e->movdqa_regreg(RCX, RBX);
			}

			// move the registers now to floating point unit
			//e->movd_to_sse ( RAX, RCX );

			// convert single precision to signed 
			//e->cvttss2si ( RCX, RAX );
			e->cvttps2dq_regreg(RCX, RCX);

			//e->Cdq ();
			//e->AndReg32ImmX ( RAX, 0x7f800000 );
			//e->CmovERegReg32 ( RDX, RAX );


			// compare exponent of magnitude and maximize if needed
			//e->CmpReg32ImmX ( RAX, 0x4e800000 - ( IX << 23 ) );
			//e->MovReg32ImmX ( RAX, 0x7fffffff );
			//e->CmovLERegReg32 ( RAX, RCX );
			//e->ShlRegImm32 ( RDX, 31 );
			//e->OrRegReg32 ( RAX, RDX );
			//e->MovRegImm32 ( RAX, 0x4f000000 - ( IX << 23 ) - 1 );
			e->MovRegImm32(RAX, 0x4f000000 - 1);
			e->movd_to_sse(RDX, RAX);
			e->pshufdregregimm(RDX, RDX, 0);
			e->pcmpeqbregreg(RAX, RAX);
			e->psrldregimm(RAX, 1);

			e->movdqa_regreg(5, RAX);

			e->pandregreg(RAX, RBX);

			e->psrldregimm(RBX, 31);
			e->padddregreg(RBX, 5);

			//if ( i.xyzw != 0xf )
			//{
			//	e->movdqa_regmem ( 5, & v->vf [ i.Ft ].sw0 );
			//}

			e->pcmpgtdregreg(RAX, RDX);

			e->pblendvbregreg(RCX, RBX);

			if (i.xyzw != 0xf)
			{
				//e->pblendwregregimm ( RCX, 5, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				e->pblendwregmemimm(RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			// set result
			//ret = e->MovMemReg32 ( ( & v->vf [ i.Ft ].sw0 ) + FtComponent, RAX );
			ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RCX);

		}	// end else
#endif

	}	// end if ( i.Ft && i.xyzw )

	return ret;
}


int32_t Recompiler::Generate_VITOFXp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u64 FX )
{
	int32_t ret;
	
	ret = 1;

	if ( i.Ft && i.xyzw )
	{
#ifdef ENABLE_AVX2_ITOFX
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);

			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&vs.sw0);
			e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));

			e->cvtdq2ps_rr128(RCX, RAX);

			if (FX)
			{
				// make constant 127 -> RDX
				//e->pcmpeqb2regreg(5, 5, 5);
				e->vpternlogd_rrr128(5, 5, 5, 0xff);


				switch (FX)
				{
				case 4:
					//e->psrld2regimm(4, 5, 31);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri128(4, 5, 31);
					e->pslld_rri128(4, 4, 2);
					break;

				case 12:
					//e->psrld2regimm(4, 5, 30);
					//e->pslld2regimm(4, 4, 2);
					e->psrld_rri128(4, 5, 30);
					e->pslld_rri128(4, 4, 2);
					break;

				case 15:
					//e->psrld2regimm(4, 5, 28);
					e->psrld_rri128(4, 5, 28);
					break;
				}

				// subtract from exponent
				//e->pslld2regimm(RDX, 4, 23);
				//e->pxor2regreg(4, 4, 4);
				//e->pcmpeqd2regreg(RAX, RAX, 4);
				//e->pandn2regreg(RDX, RAX, RDX);
				//e->psubd2regreg(RCX, RCX, RDX);
				e->pslld_rri128(RDX, 4, 23);
				e->pxor_rrr128(4, 4, 4);
				e->vpcmpdne_rrr128(2, RAX, 4);
				//e->movdqa32_rr128(RDX, RDX, 2, 1);
				e->psubd_rrr128(RCX, RCX, RDX, 2, 1);
			}

			// store result
			e->movdqa32_mr128((void*)&v->vf[i.Ft].sw0, RCX, 1);
		}
#ifdef ENABLE_AVX2_ITOFX
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

			e->movdqa1_regmem(RAX, &v->vf[i.Fs].sw0);

#ifdef USE_NEW_ITOFX_AVX2_VU

			// convert to float
			e->cvtdq2ps1regreg(RCX, RAX);

			if (FX)
			{
				e->MovReg32ImmX(RAX, (127 - FX) << 23);
				e->movd_to_sse128(RAX, RAX);
				e->vpbroadcastd1regreg(RAX, RAX);
				e->vmulps(RCX, RCX, RAX);
			}

#else

			// get absolute value -> RBX
			//e->pabsd1regreg(RBX, RAX);


			// debug -> abs(input)
			//e->movdqa_memreg(&v0, RBX);


			// get rid of rounding errors -> RCX
			//e->psrld1regimm(RCX, RBX, 24);
			//e->pandn1regreg(RCX, RCX, RBX);


			// debug -> precision bits
			//e->movdqa_memreg(&v1, RCX);


			// convert to float
			e->cvtdq2ps1regreg(RCX, RAX);


			// debug -> positive float before decimal placement
			//e->movdqa_memreg(&v2, RCX);




			if (FX)
			{
				// make mask to test if RBX is zero -> R4
				//e->pxor1regreg(4, 4, 4);
				//e->pcmpeqd1regreg(4, 4, RBX);

				// make constant 127 -> RDX
				e->pcmpeqb1regreg(5, 5, 5);
				//e->psrld1regimm(RDX, 5, 25);

				switch (FX)
				{
				case 4:
					e->psrld1regimm(4, 5, 31);
					//e->pslld1regimm(4, 4, (2 + 23));
					e->pslld1regimm(4, 4, 2);
					break;

				case 12:
					e->psrld1regimm(4, 5, 30);
					//e->pslld1regimm(4, 4, (2 + 23));
					e->pslld1regimm(4, 4, 2);
					break;

				case 15:
					e->psrld1regimm(4, 5, 28);
					//e->pslld1regimm(4, 4, (0 + 23));
					break;
				}

				// clear mask if RBX is zero
				//e->pandn1regreg(5, 4, 5);

				// subtract from exponent
				//e->psubd1regreg(RCX, RCX, 5);
				//e->psubd1regreg(RDX, RDX, 4);
				e->pslld1regimm(RDX, 4, 23);
				e->pxor1regreg(4, 4, 4);
				e->pcmpeqd1regreg(RAX, RAX, 4);
				e->pandn1regreg(RDX, RAX, RDX);
				//e->vmulps(RAX, RCX, RDX);
				e->psubd1regreg(RCX, RCX, RDX);
			}

			// debug -> positive float after decimal placement
			//e->movdqa_memreg(&v3, RCX);


			// add the sign
			//e->psrld1regimm(RAX, RAX, 31);
			//e->pslld1regimm(RAX, RAX, 31);
			//e->por1regreg(RAX, RAX, RCX);

#endif


#ifdef USE_NEW_VECTOR_DISPATCH_ITOFX_VU

			ret = Dispatch_Result_AVX2(e, v, i, false, RCX, i.Ft);

#else
			// select result
			if (i.xyzw != 0xf)
			{
				// todo: use avx2
				//e->pblendwregmemimm(RAX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pblendw1regmemimm(RCX, RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			// store result
			ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RCX);

#endif

		}	// end if (iVectorType == VECTOR_TYPE_AVX2)
#endif	// end #ifdef ENABLE_AVX2_ITOFX
#ifdef ENABLE_ITOFX_SSE
		else
		{

			//e->MovRegMem32 ( RAX, ( &v->vf [ i.Fs ].sw0 ) + FsComponent );
			e->movdqa_regmem(RBX, &v->vf[i.Fs].sw0);

			// convert single precision to signed 
			//e->cvtsi2sd ( RAX, RAX );
			//e->movq_from_sse ( RAX, RAX );
			e->cvtdq2pd(RCX, RBX);


			//e->MovReg64ImmX ( RCX, ( 896 << 23 ) + ( FX << 23 ) );
			//e->Cqo();
			e->MovReg64ImmX(RAX, (896ull << 23) + (FX << 23));
			e->movq_to_sse(RDX, RAX);
			e->movddup_regreg(RDX, RDX);

			//e->ShrRegImm64 ( RAX, 29 );
			//e->CmovERegReg64 ( RCX, RDX );
			//e->SubRegReg64 ( RAX, RCX );
			e->movdqa_regreg(4, RCX);
			e->psrlqregimm(4, 63);
			e->pslldregimm(4, 31);
			e->psrlqregimm(RCX, 29);
			e->psubqregreg(RCX, RDX);
			e->porregreg(RCX, 4);

			e->pshufdregregimm(5, RBX, (3 << 2) | (2 << 0));
			e->cvtdq2pd(5, 5);

			e->movdqa_regreg(RAX, 5);
			e->psrlqregimm(RAX, 63);
			e->pslldregimm(RAX, 31);
			e->psrlqregimm(5, 29);
			e->psubqregreg(5, RDX);
			e->porregreg(5, RAX);

			// combine RCX (bottom) and 5 (top)
			e->pshufdregregimm(RCX, RCX, (2 << 2) | (0 << 0));
			e->pshufdregregimm(RDX, 5, (2 << 6) | (0 << 4));
			e->pblendwregregimm(RCX, RDX, 0xf0);

			// load destination register
			//if ( i.xyzw != 0xf )
			//{
			//	e->movdqa_regmem ( 5, & v->vf [ i.Ft ].sw0 );
			//}

			// clear zeros
			e->pxorregreg(RAX, RAX);
			e->pcmpeqdregreg(RAX, RBX);
			e->pandnregreg(RAX, RCX);

			// select result
			if (i.xyzw != 0xf)
			{
				//e->pblendwregregimm ( RAX, 5, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				e->pblendwregmemimm(RAX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			// store result
			ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RAX);

		}	// end else
#endif
		
	}	// end if ( i.Ft && i.xyzw )

	return ret;
}



int32_t Recompiler::Generate_VMOVEp ( x64Encoder *e, VU* v, Vu::Instruction::Format i, u32 Address )
{
	int32_t ret;
	
	ret = 1;

	if (i.Ft && i.xyzw)
	{
		if (i.Ft != i.Fs)
		{
			if (iVectorType == VECTOR_TYPE_AVX2)
			{
				e->movdqa1_regmem(RCX, &v->vf[i.Fs].sw0);

				if (i.xyzw != 0xf)
				{
					e->pblendw1regmemimm(RCX, RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}

				// check if can set register directly or not
				ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RCX);

			}	// end if (iVectorType == VECTOR_TYPE_AVX2)
			else
			{
				e->movdqa_regmem(RCX, &v->vf[i.Fs].sw0);

				if (i.xyzw != 0xf)
				{
					e->pblendwregmemimm(RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}

				// check if can set register directly or not
				ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RCX);

			}	// end else

		}	// end if (i.Ft != i.Fs)

	}	// end if ( i.Ft && i.xyzw )

	return ret;
}


int32_t Recompiler::Generate_VMR32p ( x64Encoder *e, VU* v, Vu::Instruction::Format i )
{
	int32_t ret;
	
	ret = 1;

	if ( i.Ft && i.xyzw )
	{
		if (iVectorType == VECTOR_TYPE_AVX2)
		{
			//e->movdqa1_regmem(RCX, &v->vf[i.Fs].sw0);
			//e->pshufd1regregimm(RCX, RCX, (0 << 6) | (3 << 4) | (2 << 2) | (1 << 0));
			e->pshufd1regmemimm(RCX, &v->vf[i.Fs].sw0, (0 << 6) | (3 << 4) | (2 << 2) | (1 << 0));

			if (i.xyzw != 0xf)
			{
				e->pblendw1regmemimm(RCX, RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa1_memreg(&v->vf[i.Ft].sw0, RCX);
		}
		else
		{
			e->movdqa_regmem(RCX, &v->vf[i.Fs].sw0);

			e->pshufdregregimm(RCX, RCX, (0 << 6) | (3 << 4) | (2 << 2) | (1 << 0));

			if (i.xyzw != 0xf)
			{
				e->pblendwregmemimm(RCX, &v->vf[i.Ft].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			}

			ret = e->movdqa_memreg(&v->vf[i.Ft].sw0, RCX);

		}	// end else

	}	// end if ( i.Ft && i.xyzw )

	return ret;
}



int32_t Recompiler::Generate_VMFIRp ( x64Encoder *e, VU* v, Vu::Instruction::Format i )
{
	int32_t ret;
	
	ret = 1;

	if ( i.Ft && i.xyzw )
	{
		/*
		if ( !( i.is & 0xf ) )
		{
			e->pxorregreg ( RCX, RCX );
			
			if ( i.xyzw != 0xf )
			{
				e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
		}
		else
		*/
		{
			if (!(i.is & 0xf))
			{
				// coming from zero integer register
				e->XorRegReg32(RAX, RAX);
			}
			else
			{
				// load from vu integer register
				e->MovRegMem32(RAX, &v->vi[i.is & 0xf].s);

				// sign-extend from 16-bit to 32-bit
				e->Cwde();
			}

			// store to vu float register(s)
			if (i.destx) ret = e->MovMemReg32(&v->vf[i.Ft].sw0, RAX);
			if (i.desty) ret = e->MovMemReg32(&v->vf[i.Ft].sw1, RAX);
			if (i.destz) ret = e->MovMemReg32(&v->vf[i.Ft].sw2, RAX);
			if (i.destw) ret = e->MovMemReg32(&v->vf[i.Ft].sw3, RAX);


			/*
			e->movd_to_sse ( RCX, RAX );
			e->pshufdregregimm ( RCX, RCX, 0 );
			
			if ( i.xyzw != 0xf )
			{
				//e->pblendwregregimm ( RCX, RAX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			// set result
			//ret = e->MovMemReg32 ( ( &v->vf [ i.Ft ].sw0 ) + FtComponent, RAX );
			ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
			*/
		}
		
	}	// end if ( i.Ft && i.xyzw )

	return ret;
}




int32_t Recompiler::Generate_VMTIRp ( x64Encoder *e, VU* v, Vu::Instruction::Format i )
{
	int32_t ret;
	
	ret = 1;


	if ( ( i.it & 0xf ) )
	{
		//v->Set_IntDelaySlot ( i.it & 0xf, (u16) v->vf [ i.Fs ].vsw [ i.fsf ] );

		//if ( ( !i.Fs ) && ( i.fsf < 3 ) )
		if (!i.Fs)
		{
			// value coming float register where lower 16-bits are zero
			ret = e->MovMemImm32 ( & v->vi [ i.it & 0xf ].s, 0 );
		}
		else
		{
			e->MovRegMem32 ( RAX, & v->vf [ i.Fs ].vsw [ i.fsf ] );
			e->AndReg32ImmX ( RAX, 0xffff );
			
			// set result
			ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
		}
		
	}


	return ret;
}


// set bSub to 1 for subtraction
int32_t Recompiler::Generate_VADDp ( x64Encoder *e, VU* v, u32 bSub, Vu::Instruction::Format i, u32 FtComponent, void *pFd, u32 *pFt )
{
	static constexpr int64_t max_double = 0x47ffffffffffffffull;
	static constexpr int64_t min_double = 0x3800000000000000ull;

	static constexpr int64_t bit1_double = 0x0010000000000000ull;
	static constexpr int64_t bit1_double_mask = 0x4000000000000000ull;
	static constexpr int32_t bit1_float = 0x00800000;
	static constexpr int32_t bit1_float_mask = 0x40000000;

	static constexpr int64_t flt_double_mask = 0xffffffffe0000000ull;
	static constexpr int64_t ps2_mul1_double_mask = 0xffffffffc0000000ull;

	static constexpr int64_t mantissa_double_mask = 0xfffffe0000000000ull;

	static constexpr int64_t zero_double = 0ull;

	int32_t ret;

	Instruction::Format64 ii;
	//int32_t bc0;

	u64 start, end, addr;

	u32 Address = e->x64CurrentSourceAddress;

	ii.Hi.Value = i.Value;
	//i1_64.Hi.Value = i1.Value;

	//bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	ret = 1;


	if ( i.xyzw )
	{

#ifdef ALLOW_AVX2_ADDX1
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			int32_t kreg = 0;
			if (i.xyzw != 0xf)
			{
				kreg = 1;
				e->kmovdmskmem(kreg, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			}


			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);

			if (isADDBC(ii) || isADDABC(ii) || isSUBBC(ii) || isSUBABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isADDi(ii) || isADDAi(ii) || isSUBi(ii) || isSUBAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isADDq(ii) || isADDAq(ii) || isSUBq(ii) || isSUBAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}


			e->vinserti32x4_rri256(RAX, RAX, RCX, 1);

			//e->movdqa32_rm128(RAX, (void*)&vs.uw0);
			//e->vinserti32x4_rmi256(RAX, RAX, (void*)&vt.uw0, 1);
			e->vptestmd_rrb256(2, RAX, (int32_t*)&bit1_float_mask);
			e->psubd_rrb256(RAX, RAX, (int32_t*)&bit1_float, 2);
			e->cvtps2pd_rr512(RAX, RAX);
			e->paddq_rrb512(RAX, RAX, (int64_t*)&bit1_double, 2);
			//e->pshufd_rri128(RCX, RAX, 0x0e);
			e->vextracti64x4_rri512(RCX, RAX, 1);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v1.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v1.uq1, RCX);
			//e->movdqa32_mr128((void*)&v1.uq0, RAX);
			//e->kmovdmemmsk((int32_t*)&v2.uw0, 2);

			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RCX);




			// get guard mask
			//e->pxor_rrr256(RDX, RAX, RCX);
			if (bSub)
			{
				e->pand_rrr256(RDX, RAX, RCX);
			}
			else
			{
				e->pxor_rrr256(RDX, RAX, RCX);
			}
			e->psrlq_rri256(RDX, RDX, 63);
			e->psllq_rri256(RDX, RDX, 27);

			//e->vaddpd_rrr256(RAX, RAX, RCX);
			if (bSub)
			{
				// sub
				e->vsubpd_rrr256(RAX, RAX, RCX);
			}
			else
			{
				// add
				e->vaddpd_rrr256(RAX, RAX, RCX);
			}

			e->paddq_rrr256(RAX, RAX, RDX);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v2.uq0, RAX);


			e->vrangepd_rbi256(RBX, RAX, (int64_t*)&max_double, 2);
			//e->vrangepd_rbi256(RCX, RAX, (int64_t*)&min_double, 2);


			// overflow -> k2
			e->cmppdne_rrr256(2, RAX, RBX, kreg);

			// underflow -> k3
			//e->pxor_rrr256(RDX, RDX, RDX);
			//e->cmppdeq_rrr256(3, RAX, RCX);
			//e->cmppdne_rrr256(3, RAX, RDX, 3);


			// zero result on underflow
			// note: need these two instructions if caching double values
			//e->vrangepd_rri256(RBX, RBX, RDX, 2, 3);
			//e->pandq_rrm128(RBX, RBX, (void*)&flt_double_mask2);


			// testing value
			//e->movq_to_sse_rm128(RBX, (int64_t*)&test_value);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v4.uw0, RBX);
			//e->movq_to_x64_mr128((int64_t*)&v4.uq1, RCX);


			// convert result(rbx) to float ->rax
			e->vptestmq_rrb256(5, RBX, (int64_t*)&bit1_double_mask);
			e->psubq_rrb256(RBX, RBX, (int64_t*)&bit1_double, 5);
			e->cvtpd2ps_rr256(RAX, RBX);
			e->paddd_rrb128(RAX, RAX, (int32_t*)&bit1_float, 5);

			// zero -> k4
			// underflow -> k3
			e->pxor_rrr128(RDX, RDX, RDX);
			e->cmppseq_rrr128(4, RAX, RDX, kreg);
			e->cmppdne_rrr256(3, RBX, RDX, 4);

			// sign -> k5
			e->cmppdlt_rrr256(5, RBX, RDX, kreg);

			// store result
			//e->movdqa32_mr128((int32_t*)&vd.uw0, RAX);
			if (pFd)
			{
				//e->movdqa1_memreg(pFd, RAX);
				e->movdqa32_mr128(pFd, RAX, kreg);
			}
			else
			{
				if (i.Fd)
				{
					//e->movdqa1_memreg(&VU0::_VU0->vf[i.Fd].sw0, RAX);
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RAX, kreg);
				}
			}

			// get flags
			//e->movd_to_sse_rm128(RAX, (int32_t*)&statflag);
			//e->pandd_rrb128(RAX, RAX, (int32_t*)&sticky_flag_mask);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&ovf_float_flag, 2);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&und_float_flag, 3);
			//e->movd_to_x64_mr128((int32_t*)&statflag, RAX);

			// ousz flags
			e->kshiftlb(5, 5, 4);
			e->kshiftlb(2, 2, 4);
			e->korb(4, 4, 5);
			e->korb(2, 2, 3);
			e->kunpackbw(2, 2, 4);

			e->vpmovm2d_rr512(RAX, 2);
			e->pshufd_rri512(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->vpmovd2m_rr512(2, RAX);
			ret = e->kmovdmemmsk(&v->vi[VU::REG_MACFLAG].s, 2);

			if (!v->SetStatus_Flag)
			{
				e->vpmovm2b_rr128(RAX, 2);
				e->vpcmpdne_rrr128(3, RAX, RDX);
				e->kshiftld(4, 3, 6);
				e->kord(3, 3, 4);
				e->kmovdregmsk(RAX, 3);

				e->AndMem32ImmX(&v->vi[VU::REG_STATUSFLAG].s, ~0xf);
				ret = e->OrMemReg32(&v->vi[VU::REG_STATUSFLAG].s, RAX);
			}

			return ret;

		}
#ifdef ALLOW_AVX2_ADDX1
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

			if (pFt)
			{
				e->vpbroadcastd1regmem(RCX, (int32_t*)pFt);
			}
			else
			{

				if (FtComponent < 4)
				{
					e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, (FtComponent << 6) | (FtComponent << 4) | (FtComponent << 2) | (FtComponent << 0));
				}
				else
				{
					e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
				}
			}

			e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

#ifdef USE_NEW_VADDX1_AVX2

			e->vinserti128regreg(RCX, RAX, RCX, 1);

			//e->movdqa1_regmem(RCX, (void*)&vs.uw0);
			//e->vinserti128regmem(RCX, RCX, (void*)&vt.uw0, 1);
			e->vsubps2(RBX, RCX, RCX);
			e->psrld2regimm(RBX, RBX, 7);
			e->psubd2regreg(RCX, RCX, RBX);
			e->cvtps2pd2regreg(RAX, RCX);
			e->pmovsxdq2regreg(RDX, RBX);
			e->psllq2regimm(RDX, RDX, 29);
			e->paddq2regreg(RAX, RAX, RDX);
			e->vextracti128regreg(RBX, RBX, 1);
			e->vextracti128regreg(RCX, RCX, 1);
			e->cvtps2pd2regreg(RCX, RCX);
			e->pmovsxdq2regreg(RBX, RBX);
			e->psllq2regimm(RBX, RBX, 29);
			e->paddq2regreg(RCX, RCX, RBX);


			// get guard mask
			//e->pxor2regreg(RBX, RAX, RCX);
			e->pxor2regreg(RBX, RAX, RCX);

			if (bSub)
			{
				//e->pand2regreg(RBX, RAX, RCX);
				e->pcmpeqb2regreg(RDX, RDX, RDX);
				e->pxor2regreg(RBX, RBX, RDX);
			}

			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 27);


			//e->vaddpd2(RAX, RAX, RCX);
			if (bSub)
			{
				// sub
				e->vsubpd2(RAX, RAX, RCX);
			}
			else
			{
				// add
				e->vaddpd2(RAX, RAX, RCX);
			}

			e->paddq2regreg(RAX, RAX, RBX);


			// testing
			//e->ldmxcsr((int32_t*)&macflag);

			// convert result(rax) to float -> rcx
			e->psllq2regimm(RBX, RAX, 1);
			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 52);
			e->psubq2regreg(RCX, RAX, RBX);
			e->cvtpd2ps2regreg(RCX, RCX);


			// convert back (rcx) -> r4
			e->cvtps2pd2regreg(4, RCX);


			// these two only needed when writing back as float
			// result -> rcx
			// todo: need pshufps ?
			e->psrlq2regimm(RDX, RBX, 29);

			// pshufps
			e->vextracti128regreg(5, RDX, 1);


			// testing
			//e->movdqa1_memreg((void*)&v0.uq0, RCX);
			//e->movdqa1_memreg((void*)&v1.uq0, 5);


			e->pshufps1regregimm(RDX, RDX, 5, 0x88);

			e->paddd1regreg(RDX, RDX, RCX);


			// restore clamped value -> rbx
			e->paddq2regreg(RBX, RBX, 4);

			// reverse float value (rdx) for storage -> rdx
			e->pshufd1regregimm(RDX, RDX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

#ifdef USE_NEW_VECTOR_DISPATCH_VADD_VU

			if (pFd)
			{
				Dispatch_Result_AVX2(e, v, i, true, RDX, i.Fd);
			}
			else
			{
				Dispatch_Result_AVX2(e, v, i, false, RDX, i.Fd);
			}

#else
			// store result (rcx)
			//e->movdqa1_memreg((void*)&vd.uw0, RCX);
			if (i.xyzw != 0xf)
			{
				if (pFd)
				{
					e->pblendw1regmemimm(RDX, RDX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}
				else
				{
					if (i.Fd)
					{
						e->pblendw1regmemimm(RDX, RDX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
					}
				}
			}

			if (pFd)
			{
				e->movdqa1_memreg(pFd, RDX);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
				}
			}

#endif

			// get overflow -> r4
			// clamped result in (rbx)
			// un-clamped result in (rax)
			e->pand2regreg(4, RAX, RBX);
			e->cmpnepd(4, 4, RBX);

			// get underflow -> rax
			e->pxor2regreg(RDX, RDX, RDX);
			e->cmpeqpd(RAX, RAX, RDX);
			e->cmpeqpd(RAX, RAX, RBX);

			// overflow (r4) -> rbx
			// underflow (rax) -> rax
			e->cvtpd2ps2regreg(RBX, 4);
			e->cvtpd2ps2regreg(RAX, RAX);


			// sign flag -> r4
			// zero flag -> rcx
			//e->cmpgtps(4, RDX, RCX);
			e->pcmpgtd1regreg(4, RDX, RCX);
			e->cmpeqps(RCX, RDX, RCX);

			// testing
			//e->stmxcsr((int32_t*)& macflag);


			// get flags
			// could combine sign(r4) | zero(rcx) here for simd ->rcx
			e->packsdw1regreg(RCX, RCX, 4);
			e->packsdw1regreg(RAX, RAX, RBX);


			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				e->pblendw1regregimm(RAX, RAX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

			e->packswb1regreg(RAX, RCX, RAX);

			// now get stat
			e->psrld1regimm(RCX, RAX, 7);
			e->pcmpgtd1regreg(RCX, RCX, RDX);


			// now pull mac(RAX)->RCX and stat(RCX)->RAX
			e->movmskb1regreg(RCX, RAX);
			e->movmskps1regreg(RAX, RCX);

			// store mac
			// set MAC flags (RCX)
			//e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				//e->OrRegMem32(RAX, (int32_t*)&statflag);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				//e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

#else

			// create mask
			e->pcmpeqb2regreg(5, 5, 5);

			// check if doing subtraction instead of addition
			//if (bSub)
			//{
			//	// toggle the sign on RCX (right-hand argument) for subtraction for now
			//	e->pslld1regimm(4, 5, 31);
			//	e->pxor1regreg(RCX, RCX, 4);
			//}

			// get mask
			e->psrlq2regimm(4, 5, 61);
			e->psllq2regimm(4, 4, 60);

			// cvt w/ sign extend
			e->pmovsxdq2regreg(RAX, RAX);
			e->pmovsxdq2regreg(RCX, RCX);


			// cvt float to double
			e->psllq2regimm(RAX, RAX, 29);
			e->psllq2regimm(RCX, RCX, 29);
			e->pandn2regreg(RAX, 4, RAX);
			e->pandn2regreg(RCX, 4, RCX);

			// get multipliers
			// 127 << 23 ->rbx
			// 1023+127 << 52 ->r4
			e->psrlq2regimm(4, 5, 54);
			e->psrlq2regimm(RBX, 5, 57);
			e->paddq2regreg(4, 4, RBX);
			e->psllq2regimm(RBX, RBX, 23);
			e->psllq2regimm(4, 4, 52);

			// get guard mask
			if (bSub)
			{
				e->pand2regreg(RDX, RAX, RCX);
			}
			else
			{
				e->pxor2regreg(RDX, RAX, RCX);
			}

			e->psrlq2regimm(RDX, RDX, 63);
			e->psllq2regimm(RDX, RDX, 27);

			// use multiplier
			e->vmulpd2(RAX, RAX, 4);
			e->vmulpd2(RCX, RCX, 4);

			if (bSub)
			{
				// sub
				e->vsubpd2(RAX, RAX, RCX);
			}
			else
			{
				// add
				e->vaddpd2(RAX, RAX, RCX);
			}

			// adjust ??
			e->paddq2regreg(RAX, RAX, RDX);

			// get zero ->r4
			e->pxor2regreg(4, 4, 4);

			// get sign ->rdx
			//e->pcmpgtq2regreg(RDX, 4, RAX);
			//e->vextracti128regreg(RCX, RDX, 1);
			//e->packsdw1regreg(RDX, RDX, RCX);
			e->vextracti128regreg(RDX, RAX, 1);
			e->packsdw1regreg(RDX, RAX, RDX);

			// get result ->rax
			e->paddq2regreg(RAX, RAX, RAX);
			e->psrlq2regimm(RAX, RAX, 30);

			// check for zero before ->rcx
			e->pcmpeqq2regreg(RCX, RAX, 4);

			// subtract 127(rbx) from exponent
			e->psubq2regreg(RAX, RAX, RBX);

			// clear result(rax) if it was originally zero(rcx)
			e->pandn2regreg(RAX, RCX, RAX);

			// get underflow ->rbx
			e->pshufd2regregimm(RAX, RAX, 0xd8);
			e->vextracti128regreg(RCX, RAX, 1);
			e->punpckhqdq1regreg(RBX, RAX, RCX);

			// get float result ->rax
			e->punpcklqdq1regreg(RAX, RAX, RCX);

			// check for zero exponent ->rcx
			e->psrld1regimm(RCX, RAX, 23);
			e->pcmpeqd1regreg(RCX, RCX, 4);

			// get underflow | overflow ->rbx
			//e->psrad1regimm(RBX, RAX, 31);

			// clear result again on zero
			//e->pandn1regreg(RAX, RCX, RAX);

			// get underflow ->rbx
			//e->pcmpgtq2regreg(RBX, 4, RAX);

			// only underflow(rbx) if not zero(rcx)
			//e->pandn1regreg(RBX, RCX, RBX);

			// also zero(rcx) on underflow(rbx)
			e->por1regreg(RCX, RCX, RBX);

			// clear result(rax) on zero(rcx)
			e->pandn1regreg(RAX, RCX, RAX);


			// mask sign(rdx) ->rdx
			e->psrld1regimm(5, 5, 1);
			e->pandn1regreg(RDX, 5, RDX);

			// max result on overflow ->r5
			e->pminud1regreg(5, 5, RAX);

			// combine overflow(rax) | underflow(rbx) ->rbx
			e->pcmpgtd1regreg(RAX, 4, RAX);
			e->packsdw1regreg(RBX, RBX, RAX);

			// combine sign(rdx) and result(r5) ->rax
			e->por1regreg(RAX, RDX, 5);


			// combine sign(rdx) | zero(rcx) here for simd ->rcx
			// sign is only if negative number and not zero
			e->pandn1regreg(5, RCX, RDX);
			e->packsdw1regreg(RCX, RCX, 5);


			// get overflow ->r5
			// might need to use the first method for simd
			//e->pcmpgtd1regreg(5, 4, RAX);

			// max result(rax) on overflow(r5)
			//e->por1regreg(RAX, RAX, 5);

			// combine overflow(r5) | underflow(rbx) ->rbx
			//e->packsdw1regreg(RBX, RBX, 5);


			// clear sign bit in result(RAX)
			//e->paddd1regreg(RAX, RAX, RAX);
			//e->psrld1regimm(RAX, RAX, 1);

			// add sign(rdx)
			//e->pslld1regimm(RDX, RDX, 31);
			//e->por1regreg(RAX, RAX, RDX);

			// set result
			e->pshufd1regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			if (i.xyzw != 0xf)
			{
				if (pFd)
				{
					e->pblendw1regmemimm(RAX, RAX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}
				else
				{
					if (i.Fd)
					{
						e->pblendw1regmemimm(RAX, RAX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
					}
				}
			}

			if (pFd)
			{
				e->movdqa1_memreg(pFd, RAX);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
				}
			}

			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, 4, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				e->pblendw1regregimm(RBX, RBX, 4, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

			e->packswb1regreg(RAX, RCX, RBX);

			// now get stat
			e->psrld1regimm(RCX, RAX, 7);
			e->pcmpgtd1regreg(RCX, RCX, 4);

			// now pull mac(RAX)->RCX and stat(RCX)->RAX
			e->movmskb1regreg(RCX, RAX);
			e->movmskps1regreg(RAX, RCX);


			// store mac
			// set MAC flags (RCX)
			//e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);
			

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				//e->OrRegMem32(RAX, (int32_t*)&statflag);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);
				

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				//e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

#endif	// end #ifdef USE_NEW_VADDX1_AVX2

		}
#endif


	}	// end if ( i.xyzw )
	
	
	return ret;
}



int32_t Recompiler::Generate_VMULp(x64Encoder* e, VU* v, Vu::Instruction::Format i, u32 FtComponentp, void* pFd, u32* pFt, u32 FsComponentp)
{
	static constexpr int64_t max_double = 0x47ffffffffffffffull;
	static constexpr int64_t min_double = 0x3800000000000000ull;

	static constexpr int64_t bit1_double = 0x0010000000000000ull;
	static constexpr int64_t bit1_double_mask = 0x4000000000000000ull;
	static constexpr int32_t bit1_float = 0x00800000;
	static constexpr int32_t bit1_float_mask = 0x40000000;

	static constexpr int64_t flt_double_mask = 0xffffffffe0000000ull;
	static constexpr int64_t ps2_mul1_double_mask = 0xffffffffc0000000ull;

	static constexpr int64_t mantissa_double_mask = 0xfffffe0000000000ull;

	static constexpr int64_t zero_double = 0ull;

	int32_t ret;

	Instruction::Format64 ii;
	int32_t bc0;

	u64 start, end, addr;

	u32 Address = e->x64CurrentSourceAddress;

	ii.Hi.Value = i.Value;
	//i1_64.Hi.Value = i1.Value;

	bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	ret = 1;


	if (i.xyzw)
	{

#ifdef ALLOW_AVX2_MULX1
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{

			int32_t kreg = 0;
			if (i.xyzw != 0xf)
			{
				kreg = 1;
				e->kmovdmskmem(kreg, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			}


			if (isOPMULA(ii))
			{
				e->pshufd_rmi128(RAX, &v->vf[i.Fs].sw0, 0x09);
			}
			else
			{
				e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			}


			if (isMULBC(ii) || isMULABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMULi(ii) || isMULAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii) || isMULAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else if (isOPMULA(ii))
			{
				e->pshufd_rmi128(RCX, &v->vf[i.Ft].sw0, 0x12);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}


			e->vinserti32x4_rri256(RAX, RAX, RCX, 1);

			//e->movdqa32_rm128(RAX, (void*)&vs.uw0);
			//e->vinserti32x4_rmi256(RAX, RAX, (void*)&vt.uw0, 1);
			e->vptestmd_rrb256(2, RAX, (int32_t*)&bit1_float_mask);
			e->psubd_rrb256(RAX, RAX, (int32_t*)&bit1_float, 2);
			e->cvtps2pd_rr512(RAX, RAX);
			e->paddq_rrb512(RAX, RAX, (int64_t*)&bit1_double, 2);
			//e->pshufd_rri128(RCX, RAX, 0x0e);
			e->vextracti64x4_rri512(RCX, RAX, 1);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v1.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v1.uq1, RCX);
			//e->movdqa32_mr128((void*)&v1.uq0, RAX);
			//e->kmovdmemmsk((int32_t*)&v2.uw0, 2);

			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RCX);


#define ENABLE_MUL_INACCURACY_AVX3
#ifdef ENABLE_MUL_INACCURACY_AVX3

	// do x*y adjustment here
	//e->vpternlogd_rrr128(5, 5, 5, 0xff);
	//e->psrld_rri128(4, 5, 9);
	//e->pand_rrr128(RDX, RCX, 4);
	//e->vpcmpdeq_rrr128(2, RDX, 4);
	//e->paddd_rrr128(RCX, RCX, 5, 2);

			e->psllq_rri256(RDX, RCX, 12);
			e->vpcmpqeq_rrb256(2, RDX, (int64_t*)&mantissa_double_mask);
			e->pandq_rrb256(RCX, RCX, (int64_t*)&ps2_mul1_double_mask, 2);

#endif


			e->vmulpd_rrr256(RAX, RAX, RCX);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v2.uq0, RAX);


			e->vrangepd_rbi256(RBX, RAX, (int64_t*)&max_double, 2);
			//e->vrangepd_rbi256(RCX, RAX, (int64_t*)&min_double, 2);


			// overflow -> k2
			e->cmppdne_rrr256(2, RAX, RBX, kreg);

			// underflow -> k3
			//e->pxor_rrr256(RDX, RDX, RDX);
			//e->cmppdeq_rrr256(3, RAX, RCX);
			//e->cmppdne_rrr256(3, RAX, RDX, 3);


			// zero result on underflow
			// note: need these two instructions if caching double values
			//e->vrangepd_rri256(RBX, RBX, RDX, 2, 3);
			//e->pandq_rrm128(RBX, RBX, (void*)&flt_double_mask2);


			// testing value
			//e->movq_to_sse_rm128(RBX, (int64_t*)&test_value);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v4.uw0, RBX);
			//e->movq_to_x64_mr128((int64_t*)&v4.uq1, RCX);


			// convert result(rbx) to float ->rax
			e->vptestmq_rrb256(5, RBX, (int64_t*)&bit1_double_mask);
			e->psubq_rrb256(RBX, RBX, (int64_t*)&bit1_double, 5);
			e->cvtpd2ps_rr256(RAX, RBX);
			e->paddd_rrb128(RAX, RAX, (int32_t*)&bit1_float, 5);

			// zero -> k4
			// underflow -> k3
			e->pxor_rrr128(RDX, RDX, RDX);
			e->cmppseq_rrr128(4, RAX, RDX, kreg);
			e->cmppdne_rrr256(3, RBX, RDX, 4);

			// sign -> k5
			e->cmppdlt_rrr256(5, RBX, RDX, kreg);

			// store result
			//e->movdqa32_mr128((int32_t*)&vd.uw0, RAX);
			if (pFd)
			{
				e->movdqa32_mr128(pFd, RAX, kreg);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RAX, kreg);
				}
			}

			// get flags
			//e->movd_to_sse_rm128(RAX, (int32_t*)&statflag);
			//e->pandd_rrb128(RAX, RAX, (int32_t*)&sticky_flag_mask);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&ovf_float_flag, 2);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&und_float_flag, 3);
			//e->movd_to_x64_mr128((int32_t*)&statflag, RAX);

			// ousz flags
			e->kshiftlb(5, 5, 4);
			e->kshiftlb(2, 2, 4);
			e->korb(4, 4, 5);
			e->korb(2, 2, 3);
			e->kunpackbw(2, 2, 4);

			e->vpmovm2d_rr512(RAX, 2);
			e->pshufd_rri512(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->vpmovd2m_rr512(2, RAX);
			ret = e->kmovdmemmsk(&v->vi[VU::REG_MACFLAG].s, 2);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->vpmovm2b_rr128(RAX, 2);
				e->vpcmpdne_rrr128(3, RAX, RDX);
				e->kshiftld(4, 3, 6);
				e->kord(3, 3, 4);
				e->kmovdregmsk(RAX, 3);

				e->AndMem32ImmX(&v->vi[VU::REG_STATUSFLAG].s, ~0xf);
				ret = e->OrMemReg32(&v->vi[VU::REG_STATUSFLAG].s, RAX);
			}

			return ret;

		}
#ifdef ALLOW_AVX2_MULX1
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

#ifdef USE_NEW_VMULX1_AVX2

			if (pFt)
			{
				e->vpbroadcastd1regmem(RCX, (int32_t*)pFt);
			}
			else
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
			}

			e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);

			e->vinserti128regreg(RCX, RAX, RCX, 1);

			//e->movdqa1_regmem(RCX, (void*)&vs.uw0);
			//e->vinserti128regmem(RCX, RCX, (void*)&vt.uw0, 1);
			e->vsubps2(RBX, RCX, RCX);
			e->psrld2regimm(RBX, RBX, 7);
			e->psubd2regreg(RCX, RCX, RBX);
			e->cvtps2pd2regreg(RAX, RCX);
			e->pmovsxdq2regreg(RDX, RBX);
			e->psllq2regimm(RDX, RDX, 29);
			e->paddq2regreg(RAX, RAX, RDX);
			e->vextracti128regreg(RBX, RBX, 1);
			e->vextracti128regreg(RCX, RCX, 1);
			e->cvtps2pd2regreg(RCX, RCX);
			e->pmovsxdq2regreg(RBX, RBX);
			e->psllq2regimm(RBX, RBX, 29);
			e->paddq2regreg(RCX, RCX, RBX);


#define ENABLE_MUL_INACCURACY_AVX2
#ifdef ENABLE_MUL_INACCURACY_AVX2

			// do x*y adjustment here

			e->pcmpeqb2regreg(5, 5, 5);
			e->psrlq2regimm(4, 5, 41);
			e->psllq2regimm(4, 4, 29);

			e->pand2regreg(RBX, RCX, 4);
			e->pcmpeqq2regreg(RBX, RBX, 4);
			e->paddq2regreg(RBX, RBX, RCX);
			e->pand2regreg(RCX, RCX, RBX);

#endif

			e->vmulpd2(RAX, RAX, RCX);


			// testing
			//e->ldmxcsr((int32_t*)&macflag);

			// convert result(rax) to float -> rcx
			e->psllq2regimm(RBX, RAX, 1);
			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 52);
			e->psubq2regreg(RCX, RAX, RBX);
			e->cvtpd2ps2regreg(RCX, RCX);


			// convert back (rcx) -> r4
			e->cvtps2pd2regreg(4, RCX);


			// these two only needed when writing back as float
			// result -> rcx
			// todo: need pshufps ?
			e->psrlq2regimm(RDX, RBX, 29);

			// pshufps
			e->vextracti128regreg(5, RDX, 1);


			// testing
			//e->movdqa1_memreg((void*)&v0.uq0, RCX);
			//e->movdqa1_memreg((void*)&v1.uq0, 5);


			e->pshufps1regregimm(RDX, RDX, 5, 0x88);

			e->paddd1regreg(RDX, RDX, RCX);


			// restore clamped value -> rbx
			e->paddq2regreg(RBX, RBX, 4);

			// reverse float value (rdx) for storage -> rdx
			e->pshufd1regregimm(RDX, RDX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

#ifdef USE_NEW_VECTOR_DISPATCH_VMUL_VU

			if (pFd)
			{
				Dispatch_Result_AVX2(e, v, i, true, RDX, i.Fd);
			}
			else
			{
				Dispatch_Result_AVX2(e, v, i, false, RDX, i.Fd);
			}

#else

			// store result (rcx)
			//e->movdqa1_memreg((void*)&vd.uw0, RCX);
			if (i.xyzw != 0xf)
			{
				if (pFd)
				{
					e->pblendw1regmemimm(RDX, RDX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}
				else
				{
					if (i.Fd)
					{
						e->pblendw1regmemimm(RDX, RDX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
					}
				}
			}

			if (pFd)
			{
				e->movdqa1_memreg(pFd, RDX);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
				}
			}

#endif

			// get overflow -> r4
			// clamped result in (rbx)
			// un-clamped result in (rax)
			e->pand2regreg(4, RAX, RBX);
			e->cmpnepd(4, 4, RBX);

			// get underflow -> rax
			e->pxor2regreg(RDX, RDX, RDX);
			e->cmpeqpd(RAX, RAX, RDX);
			e->cmpeqpd(RAX, RAX, RBX);

			// overflow (r4) -> rbx
			// underflow (rax) -> rax
			e->cvtpd2ps2regreg(RBX, 4);
			e->cvtpd2ps2regreg(RAX, RAX);


			// sign flag -> r4
			// zero flag -> rcx
			//e->cmpgtps(4, RDX, RCX);
			e->pcmpgtd1regreg(4, RDX, RCX);
			e->cmpeqps(RCX, RDX, RCX);

			// testing
			//e->stmxcsr((int32_t*)& macflag);


			// get flags
			// could combine sign(r4) | zero(rcx) here for simd ->rcx
			e->packsdw1regreg(RCX, RCX, 4);
			e->packsdw1regreg(RAX, RAX, RBX);


			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				e->pblendw1regregimm(RAX, RAX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

			e->packswb1regreg(RAX, RCX, RAX);

			// now get stat
			e->psrld1regimm(RCX, RAX, 7);
			e->pcmpgtd1regreg(RCX, RCX, RDX);


			// now pull mac(RAX)->RCX and stat(RCX)->RAX
			e->movmskb1regreg(RCX, RAX);
			e->movmskps1regreg(RAX, RCX);

			// store mac
			// set MAC flags (RCX)
			//e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				//e->OrRegMem32(RAX, (int32_t*)&statflag);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				//e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

#else

			if (pFt)
			{
				e->vpbroadcastd1regmem(RCX, (int32_t*)pFt);
			}
			else
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
			}

			e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);


			// create mask
			e->pcmpeqb2regreg(5, 5, 5);

#define ENABLE_MUL_INACCURACY_AVX2
#ifdef ENABLE_MUL_INACCURACY_AVX2
			// do x*y adjustment here
			e->psrld1regimm(4, 5, 9);
			e->pand1regreg(RDX, RCX, 4);
			e->pcmpeqd1regreg(RDX, RDX, 4);
			e->paddd1regreg(RCX, RCX, RDX);
#endif

			// get sign ->rdx
			e->pxor1regreg(RDX, RAX, RCX);

			// cvt w/ sign extend
			e->pmovsxdq2regreg(RAX, RAX);
			e->pmovsxdq2regreg(RCX, RCX);

			// cvt float to double
			e->psllq2regimm(RAX, RAX, 33);
			e->psrlq2regimm(RAX, RAX, 4);
			e->psllq2regimm(RCX, RCX, 33);
			e->psrlq2regimm(RCX, RCX, 4);


			// get multipliers
			// 1023+1023 << 52 ->r4
			e->psrlq2regimm(4, 5, 54);
			e->psllq2regimm(4, 4, 53);

			// use multiplier
			e->vmulpd2(RCX, RCX, 4);


			// mul
			e->vmulpd2(RAX, RAX, RCX);


			// get zero ->r4
			e->pxor2regreg(4, 4, 4);

			// get sign
			//e->pcmpgtd1regreg(RDX, 4, RDX);

			// get result ->rax
			e->psrlq2regimm(RAX, RAX, 29);


			// check for zero before ->rcx
			e->pcmpeqq2regreg(RCX, RAX, 4);

			// get multipliers
			// 127 << 23 ->rbx
			e->psrlq2regimm(RBX, 5, 57);
			e->psllq2regimm(RBX, RBX, 23);


			// subtract 127(rbx) from exponent
			e->psubq2regreg(RAX, RAX, RBX);

			// clear result(rax) if it was originally zero(rcx)
			e->pandn2regreg(RAX, RCX, RAX);


			// get underflow ->rbx
			e->pshufd2regregimm(RAX, RAX, 0xd8);
			e->vextracti128regreg(RCX, RAX, 1);
			e->punpckhqdq1regreg(RBX, RAX, RCX);

			// get float result ->rax
			e->punpcklqdq1regreg(RAX, RAX, RCX);

			// check for zero exponent ->rcx
			e->psrld1regimm(RCX, RAX, 23);
			e->pcmpeqd1regreg(RCX, RCX, 4);

			// also zero(rcx) on underflow(rbx)
			e->por1regreg(RCX, RCX, RBX);

			// clear result(rax) on zero(rcx)
			e->pandn1regreg(RAX, RCX, RAX);

			// mask sign(rdx) ->rdx
			e->psrld1regimm(5, 5, 1);
			e->pandn1regreg(RDX, 5, RDX);
			
			// max result on overflow ->r5
			e->pminud1regreg(5, 5, RAX);

			// combine overflow(rax) | underflow(rbx) ->rbx
			e->pcmpgtd1regreg(RAX, 4, RAX);
			e->packsdw1regreg(RBX, RBX, RAX);

			// combine sign(rdx) and result(r5) ->rax
			e->por1regreg(RAX, RDX, 5);

			// could combine sign(rdx) | zero(rcx) here for simd ->rcx
			// sign flag is for negative numbers not including zero
			e->pandn1regreg(5, RCX, RDX);
			e->packsdw1regreg(RCX, RCX, 5);


			// get overflow ->r5
			// might need to use the first method for simd
			//e->pcmpgtd1regreg(5, 4, RAX);

			// max result(rax) on overflow(r5)
			//e->por1regreg(RAX, RAX, 5);

			// combine overflow(r5) | underflow(rbx) ->rbx
			//e->packsdw1regreg(RBX, RBX, 5);


			// clear sign bit in result(RAX)
			//e->paddd1regreg(RAX, RAX, RAX);
			//e->psrld1regimm(RAX, RAX, 1);

			// add sign(rdx)
			//e->pslld1regimm(RDX, RDX, 31);
			//e->por1regreg(RAX, RAX, RDX);

			// set result
			e->pshufd1regregimm(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			if (i.xyzw != 0xf)
			{
				if (pFd)
				{
					e->pblendw1regmemimm(RAX, RAX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}
				else
				{
					if (i.Fd)
					{
						e->pblendw1regmemimm(RAX, RAX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
					}
				}
			}

			if (pFd)
			{
				e->movdqa1_memreg(pFd, RAX);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg(&v->vf[i.Fd].sw0, RAX);
				}
			}

			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, 4, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				e->pblendw1regregimm(RBX, RBX, 4, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

			e->packswb1regreg(RAX, RCX, RBX);

			// now get stat
			e->psrld1regimm(RCX, RAX, 7);
			e->pcmpgtd1regreg(RCX, RCX, 4);

			// now pull mac(RAX)->RCX and stat(RCX)->RAX
			e->movmskb1regreg(RCX, RAX);
			e->movmskps1regreg(RAX, RCX);


			// store mac
			// set MAC flags (RCX)
			//ret = e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				//e->OrRegMem32(RAX, (int32_t*)&statflag);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				//ret = e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

#endif	// end #ifdef USE_NEW_VMULX1_AVX2

		}
#endif	// #ifdef ALLOW_AVX2_MULX1


	}	// end if ( i.xyzw )

	return ret;
}


int32_t Recompiler::Generate_VMATpx4(x64Encoder* e, VU* v, Vu::Instruction::Format i, Vu::Instruction::Format i1, Vu::Instruction::Format i2, Vu::Instruction::Format i3)
{
	int32_t ret = 1;

	Instruction::Format64 ii, ii1, ii2, ii3;

	ii.Hi.Value = i.Value;
	ii1.Hi.Value = i1.Value;
	ii2.Hi.Value = i2.Value;
	ii3.Hi.Value = i3.Value;

	// *** note: excluding opmula here, will pair with opmsub *** //

	if (i.xyzw || i1.xyzw || i2.xyzw || i3.xyzw)
	{
#ifdef ALLOW_AVX2_MULX4
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{
			e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i1.Fs].sw0, 1);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i2.Fs].sw0, 2);
			e->vinserti32x4_rmi512(RAX, RAX, &v->vf[i3.Fs].sw0, 3);


			if (isMULBC(ii) || isMULABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMULi(ii) || isMULAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMULq(ii) || isMULAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}


			if (isMADDBC(ii1) || isMADDABC(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vf[i1.Ft].vuw[i1.bc]);
			}
			else if (isMADDi(ii1) || isMADDAi(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMADDq(ii1) || isMADDAq(ii1))
			{
				e->vpbroadcastd_rm128(RDX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(RDX, &v->vf[i1.Ft].sw0);
			}

			if (isMADDBC(ii2) || isMADDABC(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vf[i2.Ft].vuw[i2.bc]);
			}
			else if (isMADDi(ii2) || isMADDAi(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMADDq(ii2) || isMADDAq(ii2))
			{
				e->vpbroadcastd_rm128(4, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(4, &v->vf[i2.Ft].sw0);
			}

			if (isMADDBC(ii3) || isMADDABC(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vf[i3.Ft].vuw[i3.bc]);
			}
			else if (isMADDi(ii3) || isMADDAi(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMADDq(ii3) || isMADDAq(ii3))
			{
				e->vpbroadcastd_rm128(5, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else
			{
				e->movdqa32_rm128(5, &v->vf[i3.Ft].sw0);
			}


			e->vinserti32x4_rri512(RCX, RCX, RDX, 1);
			e->vinserti32x4_rri512(RCX, RCX, 4, 2);
			e->vinserti32x4_rri512(RCX, RCX, 5, 3);


			// load xyzw mask (reversed of course)
			e->kmovdmskmem(1, (int32_t*)&v->xcomp[0]);

			// load xyzw mask (reversed of course)
			//e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));

			// make zero ->r16 (0x00000000)
			e->pxor_rrr512(16, 16, 16);

			// get set mask ->r17 (0xffffffff)
			e->vpternlogd_rrr512(17, 17, 17, 0xff);

			// make exp+mantissa mask ->r18 (0x7fffffff)
			e->psrld_rri512(18, 17, 1);

			// make sign flag ->r19
			e->pslld_rri512(19, 17, 31);

			// get mantissa mask ->r20 (0x007fffff)
			e->psrld_rri512(20, 17, 9);

			// make exponent mask -> r21 (0x7f800000)
			e->pxor_rrr512(21, 18, 20);

			// make 127 ->r22
			e->psrld_rri512(22, 17, 25, 1, 1);



			// check mantissa
			e->pand_rrr512(RDX, RCX, 20);
			e->vpcmpdeq_rrr512(2, RDX, 20);
			e->paddd_rrr512(RCX, RCX, 17, 2);


			// get es ->rbx
			// get et ->rdx
			e->pand_rrr512(RDX, RAX, 21);
			e->vpcmpdne_rrr512(2, RDX, 16);
			e->pand_rrr512(RBX, RAX, 18, 2, 1);
			e->pand_rrr512(RDX, RCX, 21, 2, 1);
			e->vpcmpdne_rrr512(2, RDX, 16);
			e->pand_rrr512(RDX, RDX, 18, 2, 1);

			// top es bit ->r4
			e->psrld_rri512(4, RBX, 30);
			e->pslld_rri512(4, 4, 23);
			e->psubd_rrr512(RAX, RAX, 4);

			// get ed=es+et ->rbx
			e->paddd_rrr512(RBX, RBX, RDX, 2, 1);

			// top et bit ->rdx
			e->psrld_rri512(RDX, RDX, 30);
			e->pslld_rri512(RDX, RDX, 23);
			e->psubd_rrr512(RCX, RCX, RDX);


			// do the multiply ->rax
			e->vmulps_rrr512(RAX, RAX, RCX);

			// add top bits
			e->paddd_rrr512(RDX, RDX, 4, 2, 1);


			// zero mask if intermediate exponent is zero (if zero but not underflow)
			e->psrld_rri512(RBX, RBX, 23, 1, 1);


			// ed-=127
			e->psubd_rrr512(RBX, RBX, 22, 2, 1);

			// get zero flag ->k5
			e->vpcmpdle_rrr512(5, RBX, 16);

			// get underflow ->rcx ->k4
			e->vpcmpdlt_rrr512(4, RBX, 16);

			// get overflow ->rbx ->k3
			e->pslld_rri512(RBX, RBX, 23);
			e->vpcmpdlt_rrr512(3, RBX, 16);

			// only overflow if not underflow
			// multiply overflow ->k6
			e->kandnd(6, 4, 3);

			// clear top bits on overflow
			e->movdqa32_rr512(RDX, 16, 3);

			// add top bits to result ->rdx
			e->paddd_rrr512(RAX, RAX, RDX);


			// max on overflow
			e->por_rrr512(RAX, RAX, 18, 3);


			// clear result on zero
			e->pand_rrr512(RAX, RAX, 19, 5);

			// sign flag ->r25
			//e->psrad_rri128(4, RAX, 31, 1, 1);



			// store underflow flag
			e->kmovdregmsk(RDX, 4);
			e->NegReg32(RDX);
			e->SbbRegReg32(RDX, RDX);
			e->AndReg32ImmX(RDX, 0x100);



			// ----- MAT ADD -------

			// make exponent 25 ->r22
			e->MovReg32ImmX(RAX, 25 << 23);
			e->vpbroadcastd_rx128(22, RAX);


			// zero flag(k5) ->r24
			e->vpmovm2d_rr512(24, 5);

			// sign flag ->r25
			e->psrad_rri128(25, RAX, 31, 1, 1);

			// underflow flag(k4) ->r26
			e->vpmovm2d_rr512(26, 4);

			// overflow flag(k3) ->r27
			e->vpmovm2d_rr512(27, 3);


			// multiply result ->r28
			e->movdqa32_rr512(28, RAX);


			// load accumulator
			//e->movdqa32_rm128(RCX, (void*)&v->dACC[0].l);
			e->vextracti32x4_rri512(RCX, 28, 3);

			// load multiply result
			e->vextracti32x4_rri512(RAX, 28, 2);

			// check if doing subtraction instead of addition
			if (isMSUB(ii) || isMSUBA(ii))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				//e->pxor_rrr128(RCX, RCX, 19);
				e->pxor_rrr128(RAX, RAX, 19);
			}


			// get current multiply overflow(k6) ->k7
			e->kshiftrd(7, 6, 8);

			// check for multiply overflow
			e->vpblendmd_rrr128(RCX, RCX, RAX, 7);

			// copy +/-max values from ACC
			// ACC exp -> RBX
			e->pand_rrr128(RBX, RCX, 18);
			e->vpcmpdeq_rrr128(2, RBX, 18);

			// multiply result or +/-max ACC -> RCX
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);

			// get the current xyzw flags(k1) ->k7
			e->kshiftrd(7, 1, 8);


			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn_rrr128(RBX, 19, RAX);
			e->pandn_rrr128(RDX, 19, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->vpcmpdgt_rrr128(2, RDX, RBX);
			e->vpblendmd_rrr128(RBX, RCX, RAX, 2);
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);


			// get max value exp and sub if non-zero
			e->pand_rrr128(RDX, RAX, 21);
			e->vpcmpdne_rrr128(2, RDX, 16);
			e->psubd_rrr128(RDX, RDX, 22, 2, 1);

			// sub to shift values over if non-zero
			e->psubd_rrr128(RAX, RAX, RDX);

			// check et for zero
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdne_rrr128(2, 5, 16);
			e->psubd_rrr128(RCX, RBX, RDX, 2, 1);
			e->pxor_rrr128(RBX, RBX, RCX);
			e->psrad_rri128(RBX, RBX, 31);
			e->pandn_rrr128(RBX, RBX, RCX);


			// float add
			e->vaddps_rrr128(RAX, RAX, RBX);

			// get zero ->r5
			e->pand_rrr128(5, 21, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->vpcmpdne_rrr128(5, 5, 16);
			e->movdqa32_rr128(RBX, RAX);
			e->paddd_rrr128(RBX, RBX, RDX, 5);


			// check for underflow/overflow ->rax
			e->pxor_rrr128(RAX, RAX, RBX);
			e->psrad_rri128(RAX, RAX, 31);
			//e->pand_rrr128(RAX, RAX, RDX, 1, 1);
			e->pand_rrr128(RAX, RAX, RDX, 7, 1);

			// get underflow ->r4 ->k4
			// get overflow ->rdx ->k3
			// 0 ->rcx
			// overflow(k3) or underflow(k4)-> k6
			e->vpcmpdgt_rrr128(4, 16, RAX);
			e->vpcmpdgt_rrr128(3, RAX, 16);

			// toggle sign on under/over flow
			e->kord(2, 3, 4);
			e->pxor_rrr128(RBX, RBX, 19, 2);

			// zero flag ->r5 -> k5
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdeq_rrr128(5, 5, 16);

			// also zero on underflow
			e->kord(5, 5, 4);

			// clear on zero
			e->pand_rrr128(RBX, RBX, 19, 5);

			// max on overflow
			e->por_rrr128(RBX, RBX, 18, 3);


			// sign flag ->r4
			//e->psrad_rri128(RAX, RBX, 31, 1, 1);
			e->psrad_rri128(RAX, RBX, 31, 7, 1);


			// overflow(k3)->rdx underflow(k4)->r4 sign->rax zero(k5)->r5
			e->kandd(5, 5, 7);
			e->vpmovm2d_rr128(RDX, 3);
			e->vpmovm2d_rr128(4, 4);
			e->vpmovm2d_rr128(5, 5);

			// zero flag(r5) ->r24
			e->vinserti32x4_rri512(24, 24, 5, 2);
			e->vinserti32x4_rri512(25, 25, RAX, 2);
			e->vinserti32x4_rri512(26, 26, 4, 2);
			e->vinserti32x4_rri512(27, 27, RDX, 2);


			// ---------- ADD2 ----------------

			// load accumulator
			//e->movdqa32_rm128(RCX, (void*)&v->dACC[0].l);
			e->movdqa32_rr128(RCX, RBX);

			// load multiply result
			e->vextracti32x4_rri512(RAX, 28, 1);

			// check if doing subtraction instead of addition
			if (isMSUB(ii) || isMSUBA(ii))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				//e->pxor_rrr128(RCX, RCX, 19);
				e->pxor_rrr128(RAX, RAX, 19);
			}


			// get current multiply overflow(k6) ->k7
			e->kshiftrd(7, 6, 4);

			// check for multiply overflow
			e->vpblendmd_rrr128(RCX, RCX, RAX, 7);

			// copy +/-max values from ACC
			// ACC exp -> RBX
			e->pand_rrr128(RBX, RCX, 18);
			e->vpcmpdeq_rrr128(2, RBX, 18);

			// multiply result or +/-max ACC -> RCX
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);

			// get the current xyzw flags(k1) ->k7
			e->kshiftrd(7, 1, 4);


			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn_rrr128(RBX, 19, RAX);
			e->pandn_rrr128(RDX, 19, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->vpcmpdgt_rrr128(2, RDX, RBX);
			e->vpblendmd_rrr128(RBX, RCX, RAX, 2);
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);


			// get max value exp and sub if non-zero
			e->pand_rrr128(RDX, RAX, 21);
			e->vpcmpdne_rrr128(2, RDX, 16);
			e->psubd_rrr128(RDX, RDX, 22, 2, 1);

			// sub to shift values over if non-zero
			e->psubd_rrr128(RAX, RAX, RDX);

			// check et for zero
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdne_rrr128(2, 5, 16);
			e->psubd_rrr128(RCX, RBX, RDX, 2, 1);
			e->pxor_rrr128(RBX, RBX, RCX);
			e->psrad_rri128(RBX, RBX, 31);
			e->pandn_rrr128(RBX, RBX, RCX);


			// float add
			e->vaddps_rrr128(RAX, RAX, RBX);

			// get zero ->r5
			e->pand_rrr128(5, 21, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->vpcmpdne_rrr128(5, 5, 16);
			e->movdqa32_rr128(RBX, RAX);
			e->paddd_rrr128(RBX, RBX, RDX, 5);


			// check for underflow/overflow ->rax
			e->pxor_rrr128(RAX, RAX, RBX);
			e->psrad_rri128(RAX, RAX, 31);
			//e->pand_rrr128(RAX, RAX, RDX, 1, 1);
			e->pand_rrr128(RAX, RAX, RDX, 7, 1);

			// get underflow ->r4 ->k4
			// get overflow ->rdx ->k3
			// 0 ->rcx
			// overflow(k3) or underflow(k4)-> k6
			e->vpcmpdgt_rrr128(4, 16, RAX);
			e->vpcmpdgt_rrr128(3, RAX, 16);

			// toggle sign on under/over flow
			e->kord(2, 3, 4);
			e->pxor_rrr128(RBX, RBX, 19, 2);

			// zero flag ->r5 -> k5
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdeq_rrr128(5, 5, 16);

			// also zero on underflow
			e->kord(5, 5, 4);

			// clear on zero
			e->pand_rrr128(RBX, RBX, 19, 5);

			// max on overflow
			e->por_rrr128(RBX, RBX, 18, 3);


			// sign flag ->r4
			//e->psrad_rri128(RAX, RBX, 31, 1, 1);
			e->psrad_rri128(RAX, RBX, 31, 7, 1);


			// overflow(k3)->rdx underflow(k4)->r4 sign->rax zero(k5)->r5
			e->kandd(5, 5, 7);
			e->vpmovm2d_rr128(RDX, 3);
			e->vpmovm2d_rr128(4, 4);
			e->vpmovm2d_rr128(5, 5);

			// zero flag(r5) ->r24
			e->vinserti32x4_rri512(24, 24, 5, 2);
			e->vinserti32x4_rri512(25, 25, RAX, 2);
			e->vinserti32x4_rri512(26, 26, 4, 2);
			e->vinserti32x4_rri512(27, 27, RDX, 2);


			// ---------- ADD3 ----------------

			// load accumulator
			//e->movdqa32_rm128(RCX, (void*)&v->dACC[0].l);
			e->movdqa32_rr128(RCX, RBX);

			// load multiply result
			//e->vextracti32x4_rri512(RAX, 28, 1);
			e->movdqa32_rr128(RAX, 28);

			// check if doing subtraction instead of addition
			if (isMSUB(ii) || isMSUBA(ii))
			{
				// toggle the sign on RCX (right-hand argument) for subtraction for now
				//e->pxor_rrr128(RCX, RCX, 19);
				e->pxor_rrr128(RAX, RAX, 19);
			}


			// get current multiply overflow(k6) ->k7
			e->kshiftrd(7, 6, 0);

			// check for multiply overflow
			e->vpblendmd_rrr128(RCX, RCX, RAX, 7);

			// copy +/-max values from ACC
			// ACC exp -> RBX
			e->pand_rrr128(RBX, RCX, 18);
			e->vpcmpdeq_rrr128(2, RBX, 18);

			// multiply result or +/-max ACC -> RCX
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);

			// get the current xyzw flags(k1) ->k7
			e->kshiftrd(7, 1, 0);


			// get the magnitude
			//ms = fs & 0x7fffffff; -> RBX
			//mt = ft & 0x7fffffff; -> RDX
			e->pandn_rrr128(RBX, 19, RAX);
			e->pandn_rrr128(RDX, 19, RCX);

			// sort the values by magnitude
			//if (ms < mt)
			//{
			//	temp = fs;
			//	fs = ft;	-> RAX
			//	ft = temp;	-> RBX
			//}
			e->vpcmpdgt_rrr128(2, RDX, RBX);
			e->vpblendmd_rrr128(RBX, RCX, RAX, 2);
			e->vpblendmd_rrr128(RAX, RAX, RCX, 2);


			// get max value exp and sub if non-zero
			e->pand_rrr128(RDX, RAX, 21);
			e->vpcmpdne_rrr128(2, RDX, 16);
			e->psubd_rrr128(RDX, RDX, 22, 2, 1);

			// sub to shift values over if non-zero
			e->psubd_rrr128(RAX, RAX, RDX);

			// check et for zero
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdne_rrr128(2, 5, 16);
			e->psubd_rrr128(RCX, RBX, RDX, 2, 1);
			e->pxor_rrr128(RBX, RBX, RCX);
			e->psrad_rri128(RBX, RBX, 31);
			e->pandn_rrr128(RBX, RBX, RCX);


			// float add
			e->vaddps_rrr128(RAX, RAX, RBX);

			// get zero ->r5
			e->pand_rrr128(5, 21, RAX);

			// shift value back over after add if nonzero
			// result ->rbx
			e->vpcmpdne_rrr128(5, 5, 16);
			e->movdqa32_rr128(RBX, RAX);
			e->paddd_rrr128(RBX, RBX, RDX, 5);


			// check for underflow/overflow ->rax
			e->pxor_rrr128(RAX, RAX, RBX);
			e->psrad_rri128(RAX, RAX, 31);
			//e->pand_rrr128(RAX, RAX, RDX, 1, 1);
			e->pand_rrr128(RAX, RAX, RDX, 7, 1);

			// get underflow ->r4 ->k4
			// get overflow ->rdx ->k3
			// 0 ->rcx
			// overflow(k3) or underflow(k4)-> k6
			e->vpcmpdgt_rrr128(4, 16, RAX);
			e->vpcmpdgt_rrr128(3, RAX, 16);

			// toggle sign on under/over flow
			e->kord(2, 3, 4);
			e->pxor_rrr128(RBX, RBX, 19, 2);

			// zero flag ->r5 -> k5
			e->pand_rrr128(5, RBX, 21);
			e->vpcmpdeq_rrr128(5, 5, 16);

			// also zero on underflow
			e->kord(5, 5, 4);

			// clear on zero
			e->pand_rrr128(RBX, RBX, 19, 5);

			// max on overflow
			e->por_rrr128(RBX, RBX, 18, 3);


			// sign flag ->r4
			//e->psrad_rri128(RAX, RBX, 31, 1, 1);
			e->psrad_rri128(RAX, RBX, 31, 7, 1);


			// overflow(k3)->rdx underflow(k4)->r4 sign->rax zero(k5)->r5
			e->kandd(5, 5, 7);
			e->vpmovm2d_rr128(RDX, 3);
			e->vpmovm2d_rr128(4, 4);
			e->vpmovm2d_rr128(5, 5);

			// zero flag(r5) ->r24
			e->vinserti32x4_rri512(5, 24, 5, 2);
			e->vinserti32x4_rri512(RAX, 25, RAX, 2);
			e->vinserti32x4_rri512(4, 26, 4, 2);
			e->vinserti32x4_rri512(RDX, 27, RDX, 2);



			// ----------- flags ----------------

			// put sign(rax) on top of zero(r5)
			e->vinserti32x4_rri256(RBX, 5, RAX, 1);

			// put overflow(rdx) on top of underflow(r4)
			e->vinserti32x4_rri256(RCX, 4, RDX, 1);

			// put overflow/underflow(rcx) on top of sign/zero(r5)
			e->vinserti32x8_rri512(RBX, RBX, RCX, 1);

			// shuffle xyzw
			e->pshufd_rri512(RBX, RBX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			// make mask for mac flags ->k2
			e->vpmovd2m_rr512(2, RBX);

			// pull byte mask for stat flags ->k3
			e->vpmovm2b_rr128(RBX, 2);
			e->vpcmpdne_rrr128(3, RBX, 16);


			// ***TODO*** sticky flags



			e->kmovdregmsk(RCX, 2);
			e->kmovdregmsk(RAX, 3);


			// store mac
			// set MAC flags (RCX)
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				// orr with sticky underflow
				e->OrRegReg32(RAX, RDX);

				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )

		}
	}

	// prepare for the next parallel instruction
	ret = e->MovMemImm64((int64_t*)&v->xcomp, 0);

	return ret;
}


int32_t Recompiler::Generate_VMADDp ( x64Encoder *e, VU* v, u32 bSub, Vu::Instruction::Format i, u32 FtComponentp, void *pFd, u32 *pFt, u32 FsComponentp )
{
	static constexpr int64_t max_double_float = 0x47ffffffe0000000ull;

	static constexpr int64_t max_double = 0x47ffffffffffffffull;
	static constexpr int64_t min_double = 0x3800000000000000ull;

	static constexpr int64_t test_ovf_double = 0x47ffffffe0000000ull;


	static constexpr int64_t bit1_double = 0x0010000000000000ull;
	static constexpr int64_t bit1_double_mask = 0x4000000000000000ull;
	static constexpr int32_t bit1_float = 0x00800000;
	static constexpr int32_t bit1_float_mask = 0x40000000;

	static constexpr int64_t flt_double_mask = 0xffffffffe0000000ull;
	static constexpr int64_t ps2_mul1_double_mask = 0xffffffffc0000000ull;

	static constexpr int64_t mantissa_double_mask = 0xfffffe0000000000ull;

	static constexpr int64_t zero_double = 0ull;

	static constexpr int32_t mxcsr_start_value = 0xffe0;

	int32_t ret;
	
	Instruction::Format64 ii;
	int32_t bc0;

	u64 start, end, addr;

	u32 Address = e->x64CurrentSourceAddress;

	ii.Hi.Value = i.Value;
	//i1_64.Hi.Value = i1.Value;

	//bc0 = (i.bc) | (i.bc << 2) | (i.bc << 4) | (i.bc << 6);
	//bc1 = (i1.bc) | (i1.bc << 2) | (i1.bc << 4) | (i1.bc << 6);

	ret = 1;


	if ( i.xyzw )
	{

#ifdef ALLOW_AVX2_MADDX1
		if (iVectorType == VECTOR_TYPE_AVX512)
#endif
		{

			int32_t kreg = 0;
			if (i.xyzw != 0xf)
			{
				kreg = 1;
				e->kmovdmskmem(1, (int32_t*)&(xyzw_mask_lut32[i.xyzw]));
			}

			if (isOPMSUB(ii))
			{
				e->pshufd_rmi128(RAX, &v->vf[i.Fs].sw0, 0x09);
			}
			else
			{
				e->movdqa32_rm128(RAX, (void*)&v->vf[i.Fs].sw0);
			}


			if (isMADDBC(ii) || isMADDABC(ii) || isMSUBBC(ii) || isMSUBABC(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vf[i.Ft].vuw[i.bc]);
			}
			else if (isMADDi(ii) || isMADDAi(ii) || isMSUBi(ii) || isMSUBAi(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_I].sw0);
			}
			else if (isMADDq(ii) || isMADDAq(ii) || isMSUBq(ii) || isMSUBAq(ii))
			{
				e->vpbroadcastd_rm128(RCX, (int32_t*)&v->vi[VU::REG_Q].sw0);
			}
			else if (isOPMSUB(ii))
			{
				e->pshufd_rmi128(RCX, &v->vf[i.Ft].sw0, 0x12);
			}
			else
			{
				e->movdqa32_rm128(RCX, &v->vf[i.Ft].sw0);
			}

			e->vinserti32x4_rri256(RAX, RAX, RCX, 1);

			//e->movdqa32_rm128(RAX, (void*)&vs.uw0);
			//e->vinserti32x4_rmi256(RAX, RAX, (void*)&vt.uw0, 1);
			e->vptestmd_rrb256(2, RAX, (int32_t*)&bit1_float_mask);
			e->psubd_rrb256(RAX, RAX, (int32_t*)&bit1_float, 2);
			e->cvtps2pd_rr512(RAX, RAX);
			e->paddq_rrb512(RAX, RAX, (int64_t*)&bit1_double, 2);
			//e->pshufd_rri128(RCX, RAX, 0x0e);
			e->vextracti64x4_rri512(RCX, RAX, 1);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v1.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v1.uq1, RCX);
			//e->movdqa32_mr128((void*)&v1.uq0, RAX);
			//e->kmovdmemmsk((int32_t*)&v2.uw0, 2);

			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RCX);


#define ENABLE_MUL_INACCURACY_AVX3
#ifdef ENABLE_MUL_INACCURACY_AVX3

	// do x*y adjustment here
	//e->vpternlogd_rrr128(5, 5, 5, 0xff);
	//e->psrld_rri128(4, 5, 9);
	//e->pand_rrr128(RDX, RCX, 4);
	//e->vpcmpdeq_rrr128(2, RDX, 4);
	//e->paddd_rrr128(RCX, RCX, 5, 2);

			e->psllq_rri256(RDX, RCX, 12);
			e->vpcmpqeq_rrb256(2, RDX, (int64_t*)&mantissa_double_mask);
			e->pandq_rrb256(RCX, RCX, (int64_t*)&ps2_mul1_double_mask, 2);

#endif


			e->vmulpd_rrr256(RAX, RAX, RCX);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v2.uq0, RAX);


			e->vrangepd_rbi256(RBX, RAX, (int64_t*)&max_double, 2);
			e->cvtpd2ps_rr256(RCX, RAX);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RBX);


			// overflow -> k3
			e->cmppdne_rrr256(3, RAX, RBX);

			// zero -> k4
			// sticky underflow -> k6
			e->pxor_rrr256(RDX, RDX, RDX);
			e->cmppseq_rrr128(4, RCX, RDX, kreg);
			e->cmppdne_rrr256(6, RBX, RDX, 4);

			// zero result on underflow or zero
			// note: need these two instructions if caching double values
			e->vrangepd_rri256(RBX, RBX, RDX, 2, 4);
			e->pandq_rrb256(RAX, RBX, (int64_t*)&flt_double_mask);

			// if sub, then negate the result
			if (bSub)
			{
				e->vsubpd_rrr256(RAX, RDX, RAX);
			}

			// load acc
			e->movdqa32_rm128(5, (void*)&v->dACC[0].l);
			e->vptestmd_rrb128(2, 5, (int32_t*)&bit1_float_mask);
			e->psubd_rrb128(5, 5, (int32_t*)&bit1_float, 2);
			e->cvtps2pd_rr256(5, 5);
			e->paddq_rrb256(5, 5, (int64_t*)&bit1_double, 2);

			// on overflow, copy result to acc
			e->movdqa64_rr256(5, RAX, 3);

			// on acc overflow copy to result
			e->vrangepd_rbi256(4, 5, (int64_t*)&test_ovf_double, 2);
			e->cmppdne_rrr256(4, 4, 5);
			e->movdqa64_rr256(RAX, 5, 4);

			// get guard mask
			//e->pxor_rrr256(RDX, RAX, RCX);
			//if (bSub)
			//{
			//	e->pand_rrr256(RBX, RAX, 5);
			//}
			//else
			{
				e->pxor_rrr256(RBX, RAX, 5);
			}
			e->psrlq_rri256(RBX, RBX, 63);
			e->psllq_rri256(RBX, RBX, 27);

			//e->vaddpd_rrr256(RAX, RAX, RCX);
			//if (bSub)
			//{
			//	// sub
			//	e->vsubpd_rrr256(RAX, RAX, 5);
			//}
			//else
			{
				// add
				e->vaddpd_rrr256(RAX, RAX, 5);
			}

			e->paddq_rrr256(RAX, RAX, RBX);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v2.uq0, RAX);


			e->vrangepd_rbi256(RBX, RAX, (int64_t*)&max_double, 2);
			//e->vrangesd_rmi64(RCX, RAX, (void*)&min_double, 2);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RBX);


			// overflow -> k2
			e->cmppdne_rrr256(2, RAX, RBX, kreg);


			// zero result on underflow
			// note: need these two instructions if caching double values
			//e->vrangepd_rri256(RBX, RBX, RDX, 2, 3);
			//e->pandq_rrb256(RBX, RBX, (int64_t*)&flt_double_mask2);


			// testing value
			//e->movq_to_sse_rm128(RBX, (int64_t*)&test_value);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v4.uw0, RBX);
			//e->movq_to_x64_mr128((int64_t*)&v4.uq1, RCX);


			// convert result(rbx) to float ->rax
			e->vptestmq_rrb256(5, RBX, (int64_t*)&bit1_double_mask);
			e->psubq_rrb256(RBX, RBX, (int64_t*)&bit1_double, 5);
			e->cvtpd2ps_rr256(RAX, RBX);
			e->paddd_rrb128(RAX, RAX, (int32_t*)&bit1_float, 5);

			// zero -> k4
			// underflow -> k3
			//e->pxor_rrr128(RDX, RDX, RDX);
			e->cmppseq_rrr128(4, RAX, RDX, kreg);
			e->cmppdne_rrr256(3, RBX, RDX, 4);

			// sign -> k5
			e->cmppdlt_rrr256(5, RBX, RDX, kreg);

			// store result
			//e->movdqa32_mr128((int32_t*)&vd.uw0, RAX);
			if (pFd)
			{
				e->movdqa32_mr128(pFd, RAX, kreg);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa32_mr128(&v->vf[i.Fd].sw0, RAX, kreg);
				}
			}


			// get flags
			//e->movd_to_sse_rm128(RAX, (int32_t*)&statflag);
			//e->pandd_rrb128(RAX, RAX, (int32_t*)&sticky_flag_mask);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&ovf_float_flag, 2);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&sticky_und_float_flag, 3);
			//e->pord_rrb128(RAX, RAX, (int32_t*)&und_float_flag, 6);
			//e->movd_to_x64_mr128((int32_t*)&statflag, RAX);

			// ousz flags
			e->kshiftlb(5, 5, 4);
			e->kshiftlb(2, 2, 4);
			e->korb(4, 4, 5);
			e->korb(2, 2, 3);
			e->kunpackbw(2, 2, 4);

			e->vpmovm2d_rr512(RAX, 2);
			e->pshufd_rri512(RAX, RAX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->vpmovd2m_rr512(2, RAX);
			ret = e->kmovdmemmsk((int32_t*)&v->vi[VU::REG_MACFLAG].s, 2);

			if (!v->SetStatus_Flag)
			{
				e->vpmovm2b_rr128(RAX, 2);
				e->vpcmpdne_rrr128(3, RAX, RDX);
				e->kshiftld(4, 3, 6);
				e->kord(3, 3, 4);
				e->kmovdregmsk(RAX, 3);

				// sticky underflow flag
				e->kmovdregmsk(RCX, 6);
				e->NegReg32(RCX);
				e->SbbRegReg32(RCX, RCX);
				e->AndReg32ImmX(RCX, 0x100);
				e->OrRegReg32(RAX, RCX);

				e->AndMem32ImmX((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, ~0xf);
				ret = e->OrMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);
			}

			return ret;

		}
#ifdef ALLOW_AVX2_MADDX1
		else if (iVectorType == VECTOR_TYPE_AVX2)
		{

//#define ENABLE_EARLY_FLOAT_CALC_MADD_VU
#ifdef ENABLE_EARLY_FLOAT_CALC_MADD_VU

			e->ldmxcsr((int32_t*)&mxcsr_start_value);

			if (pFt)
			{
				e->vpbroadcastd1regmem(RCX, (int32_t*)pFt);
			}
			else
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
			}

			e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);

			e->pxor1regreg(RDX, RDX, RDX);

			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x11) | (i.desty * 0x22) | (i.destz * 0x44) | (i.destw * 0x88)));
			}

			e->vmulps(RAX, RAX, RCX);

			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RAX, RAX, RDX, ~((i.destx * 0x11) | (i.desty * 0x22) | (i.destz * 0x44) | (i.destw * 0x88)));
			}

			e->pshufd1regmemimm(RCX, (void*)&v->dACC[0].l, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x11) | (i.desty * 0x22) | (i.destz * 0x44) | (i.destw * 0x88)));
			}

			// if sub, then negate the result
			if (bSub)
			{
				e->vsubps(RCX, RDX, RCX);
			}

			e->vaddps(RCX, RCX, RAX);

			e->vsubps(5, RCX, RCX);
			e->cvttps2dq1regreg(5, 5);

			e->stmxcsr(&v->mxcsr_flag_value);

			// clear overflow | underflow flags
			e->pxor1regreg(RAX, RAX, RAX);

			// clear sticky underflow
			e->XorRegReg32(RDX, RDX);

			e->TestMem32ImmX(&v->mxcsr_flag_value, 0x19);
			e->Jmp_E(0, 0);

#endif

			if (pFt)
			{
				e->vpbroadcastd1regmem(RCX, (int32_t*)pFt);
			}
			else
			{
				e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
			}

			e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);


			e->vinserti128regreg(RCX, RAX, RCX, 1);

			//e->movdqa1_regmem(RCX, (void*)&vs.uw0);
			//e->vinserti128regmem(RCX, RCX, (void*)&vt.uw0, 1);
			e->vsubps2(RBX, RCX, RCX);
			e->psrld2regimm(RBX, RBX, 7);
			e->psubd2regreg(RCX, RCX, RBX);
			e->cvtps2pd2regreg(RAX, RCX);
			e->pmovsxdq2regreg(RDX, RBX);
			e->psllq2regimm(RDX, RDX, 29);
			e->paddq2regreg(RAX, RAX, RDX);
			e->vextracti128regreg(RBX, RBX, 1);
			e->vextracti128regreg(RCX, RCX, 1);
			e->cvtps2pd2regreg(RCX, RCX);
			e->pmovsxdq2regreg(RBX, RBX);
			e->psllq2regimm(RBX, RBX, 29);
			e->paddq2regreg(RCX, RCX, RBX);


#define ENABLE_MUL_INACCURACY_AVX2
#ifdef ENABLE_MUL_INACCURACY_AVX2

			// do x*y adjustment here

			e->pcmpeqb2regreg(5, 5, 5);
			e->psrlq2regimm(4, 5, 41);
			e->psllq2regimm(4, 4, 29);

			e->pand2regreg(RBX, RCX, 4);
			e->pcmpeqq2regreg(RBX, RBX, 4);
			e->paddq2regreg(RBX, RBX, RCX);
			e->pand2regreg(RCX, RCX, RBX);

#endif


			e->vmulpd2(RAX, RAX, RCX);


			// testing
			//e->movdqa1_memreg((void*)&v0.uq0, RAX);


			// testing value
			//e->movq_to_sse_rm128(RBX, (int64_t*)&test_value);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v4.uw0, RBX);
			//e->movq_to_x64_mr128((int64_t*)&v4.uq1, RCX);


			// testing
			//e->ldmxcsr((int32_t*)&macflag);

			// convert result(rax) to float -> rcx
			e->psllq2regimm(RBX, RAX, 1);
			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 52);
			e->psubq2regreg(RDX, RAX, RBX);
			e->cvtpd2ps2regreg(RDX, RDX);


			// convert back (rdx) -> r4
			e->cvtps2pd2regreg(4, RDX);


			// testing
			//e->movdqa1_memreg((void*)&v0.uq0, RCX);
			//e->movdqa1_memreg((void*)&v1.uq0, 5);


			// restore clamped value -> rbx
			e->paddq2regreg(RBX, RBX, 4);


			// store result (rcx)
			//e->movdqa1_memreg((void*)&vd.uw0, RCX);


			// get overflow -> rcx
			// clamped result in (rbx)
			// un-clamped result in (rax)
			e->pand2regreg(RCX, RAX, RBX);
			e->cmpnepd(RCX, RCX, RBX);

			// get underflow -> rax
			e->pxor1regreg(4, 4, 4);
			e->cmpeqpd(RAX, RAX, 4);
			e->cmpeqpd(RAX, RAX, RBX);


			// if sub, then negate the result
			if (bSub)
			{
				//e->vsubpd_rrr256(RBX, 4, RBX);
				e->vsubpd2(RBX, 4, RBX);
			}


			// save sticky underflow
			e->movmskpd2regreg(RAX, RAX);

			// zero flag -> rcx
			// sign flag -> r4
			//e->cmpeqps(RCX, RDX, 5);
			//e->cmpltps(4, RDX, 5);

			// get underflow sticky flag
			e->MovReg32ImmX(RDX, 0x100);
			e->AndReg32ImmX(RAX, i.xyzw);
			e->CmovERegReg32(RDX, RAX);

			// load acc -> rax
			//e->movdqa1_regmem(RAX, (void*)&vd.uw0);
			e->pshufd1regmemimm(RAX, (void*)&v->dACC[0].l, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
			e->vsubps(RDX, RAX, RAX);
			e->psrld1regimm(RDX, RDX, 7);
			e->psubd1regreg(RAX, RAX, RDX);
			e->cvtps2pd2regreg(RAX, RAX);
			e->pmovsxdq2regreg(RDX, RDX);
			e->psllq2regimm(RDX, RDX, 29);
			e->paddq2regreg(RAX, RAX, RDX);


			// copy mul result (rbx) to acc (r5) on overflow (rcx) -> rcx
			e->pblendvb2regreg(RCX, RAX, RBX, RCX);

			// check for acc overflow
			//e->psllq2regimm(5, 5, 29);
			//e->psubq2regreg(5, RCX, 5);
			//e->psllq2regimm(5, 5, 5);
			//e->pcmpeqq2regreg(RDX, 4, 5);



			// check for acc overflow
			e->MovReg64ImmX(RAX, (int64_t)max_double_float);
			e->movq_to_sse128(4, RAX);

			// need to broadcast
			e->vpbroadcastq2regreg(4, 4);

			//e->psllq2regimm(RCX, 5, 1);
			//e->psrlq2regimm(RCX, RCX, 1);
			//e->cmpeqpd(4, 4, RCX);
			e->pand2regreg(5, 4, RCX);
			e->pcmpeqq2regreg(RDX, 4, 5);

			// copy acc(r5) to result(rbx) on overflow (r4) -> rax
			e->pblendvb2regreg(RAX, RBX, RCX, RDX);


			// get guard mask
			e->pxor2regreg(RBX, RAX, RCX);
			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 27);


			e->vaddpd2(RAX, RAX, RCX);


			e->paddq2regreg(RAX, RAX, RBX);


			// testing value
			//e->movq_to_sse_rm128(RBX, (int64_t*)&test_value);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v4.uw0, RBX);
			//e->movq_to_x64_mr128((int64_t*)&v4.uq1, RCX);


			// testing
			//e->ldmxcsr((int32_t*)&macflag);

			// convert result(rax) to float -> rcx
			e->psllq2regimm(RBX, RAX, 1);
			e->psrlq2regimm(RBX, RBX, 63);
			e->psllq2regimm(RBX, RBX, 52);
			e->psubq2regreg(RCX, RAX, RBX);
			e->cvtpd2ps2regreg(RCX, RCX);


			// convert back (rcx) -> r4
			e->cvtps2pd2regreg(4, RCX);


			// these two only needed when writing back as float
			// result -> rcx
			// todo: need pshufps ?
			e->psrlq2regimm(RDX, RBX, 29);

			// pshufps
			e->vextracti128regreg(5, RDX, 1);


			// testing
			//e->movdqa1_memreg((void*)&v0.uq0, RCX);
			//e->movdqa1_memreg((void*)&v1.uq0, 5);


			e->pshufps1regregimm(RDX, RDX, 5, 0x88);

			e->paddd1regreg(RCX, RCX, RDX);

			// restore clamped value -> rbx
			e->paddq2regreg(RBX, RBX, 4);

			// get overflow -> r4
			// clamped result in (rbx)
			// un-clamped result in (rax)
			e->pand2regreg(4, RAX, RBX);
			e->cmpnepd(4, 4, RBX);

			// get underflow -> rax
			e->pxor2regreg(RDX, RDX, RDX);
			e->cmpeqpd(RAX, RAX, RDX);
			e->cmpeqpd(RAX, RAX, RBX);

			// overflow (r4) -> rbx
			// underflow (rax) -> rax
			e->cvtpd2ps2regreg(RBX, 4);
			e->cvtpd2ps2regreg(RAX, RAX);

			// get flags
			// get overflow | underflow
			e->packsdw1regreg(RAX, RAX, RBX);

			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RAX, RAX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

#ifdef ENABLE_EARLY_FLOAT_CALC_MADD_VU

			e->SetJmpTarget(0);

#endif

			// store float result (rdx)
			//e->movdqa1_memreg((void*)&vd.uw0, RDX);
			e->pshufd1regregimm(5, RCX, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

#ifdef USE_NEW_VECTOR_DISPATCH_VMADD_VU

			if (pFd)
			{
				Dispatch_Result_AVX2(e, v, i, true, 5, i.Fd);
			}
			else
			{
				Dispatch_Result_AVX2(e, v, i, false, 5, i.Fd);
			}

#else
			if (i.xyzw != 0xf)
			{
				if (pFd)
				{
					e->pblendw1regmemimm(5, 5, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				}
				else
				{
					if (i.Fd)
					{
						e->pblendw1regmemimm(5, 5, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
					}
				}
			}

			if (pFd)
			{
				e->movdqa1_memreg(pFd, 5);
			}
			else
			{
				if (i.Fd)
				{
					e->movdqa1_memreg(&v->vf[i.Fd].sw0, 5);
				}
			}
#endif



			// sign flag -> r4
			// zero flag -> rcx
			//e->cmpgtps(4, RDX, RCX);
			e->pcmpgtd1regreg(4, RDX, RCX);
			e->cmpeqps(RCX, RDX, RCX);

			// testing
			//e->stmxcsr((int32_t*)& macflag);


			// testing
			//e->movq_to_x64_mr128((int64_t*)&v3.uq0, RAX);
			//e->movq_to_x64_mr128((int64_t*)&v3.uq1, RBX);


			// get flags
			// could combine sign(r4) | zero(rcx) here for simd ->rcx
			e->packsdw1regreg(RCX, RCX, 4);


			// flags
			if (i.xyzw != 0xf)
			{
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
				//e->pblendw1regregimm(RAX, RAX, RDX, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
			}

			e->packswb1regreg(RAX, RCX, RAX);

			// now get stat
			e->psrld1regimm(RCX, RAX, 7);
			e->pcmpgtd1regreg(RCX, RCX, RDX);


			// now pull mac(RAX)->RCX and stat(RCX)->RAX
			e->movmskb1regreg(RCX, RAX);
			e->movmskps1regreg(RAX, RCX);

			// store mac
			// set MAC flags (RCX)
			//e->MovMemReg32((int32_t*)&macflag, RCX);
			ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_MACFLAG].s, RCX);

			// update stat
			// check if the lower instruction set stat flag already (there's only like one instruction that does this)
			if (!v->SetStatus_Flag)
			{
				e->MovRegReg32(RCX, RAX);

				e->ShlRegImm32(RAX, 6);
				//e->OrRegMem32(RAX, (int32_t*)&statflag);
				e->OrRegMem32(RAX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].s);

				// these two instructions are if you need to set non-sticky flags
				e->AndReg32ImmX(RAX, ~0xf);
				e->OrRegReg32(RAX, RCX);

				// add sticky underflow
				e->OrRegReg32(RAX, RDX);

				//e->MovMemReg32((int32_t*)&statflag, RAX);
				ret = e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG].s, RAX);

			}	// end if ( !v->SetStatus_Flag )



		}
#endif	// end #ifdef ALLOW_AVX2_MADDX1

	}	// end if ( i.xyzw )
	
	return ret;
}





// returns number of instructions that were recompiled
u32 Recompiler::Recompile ( VU* v, u32 BeginAddress )
{
	// todo:
	// check int delay slot variable for int delay slot instead of instruction count
	// check for e-bit in e-bit delay slot
	// check for xgkick in xgkick delay slot

	u32 Address, Block;
	s32 ret, Cycles;
	s32 reti;
	
	s32 retLo, retHi;
	s32 OpLvlSave;

	//Vu::Instruction::Format inst;
	//Vu::Instruction::Format instLO;
	//Vu::Instruction::Format instHI;
	//Vu::Instruction::Format v->NextInstLO;

	Vu::Instruction::Format64 CurInst;

	VU::Bitmap128 FSrcBitmapLo;
	VU::Bitmap128 FDstBitmapLo;
	
	//u32 StartBlockIndex, BlockIndex, SaveBlockIndex;
	
	// number of instructions in current run
	u32 RunCount;
	
	u32 RecompileCount;
	u32 MaxCount;
	
	//u32 ProjectedMaxCount;
	
	//u32 Snapshot_Address [ 4 ];
	//u32 Snapshot_RecompileCount [ 4 ];
	
	//static u64 MemCycles;
	u32 SetCycles;
	
	//u32* pInstrPtr;
	
	u32* pSrcCodePtr;
	u32* pNextCode;
	
	//u32* pCmpCodePtr;
	//u32* pSaveCodePtr;
	//u32* pSaveCmpPtr;
	
	u32 SaveReg0;
	u32 ulCacheLineCount;
	
	//u64 LocalCycleCount, CacheBlock_CycleCount;
	
	int RetJumpCounter;
	
	//char* ReturnFromCacheReload;
	
	int i;
	
	uint32_t First_LastModifiedReg;
	
	s32 MaxBlocks;
	
	u32 NextAddress;
	
#ifdef VERBOSE_RECOMPILE
cout << "\nrecompile: starting recompile.";
#endif

	// set current recompiler as the one doing the recompiling
	_REC = this;

	// need to first clear forward branch targets for the block
	memset ( pForwardBranchTargets, 0x00, sizeof( u32 ) * MaxStep );
	
	// initialize forward branch index
	// note: Will need a larger branch index table in the encoder object for larger code blocks than 128 instructions
	ForwardBranchIndex = c_ulForwardBranchIndex_Start;


	// mask address
	// don't do this
	//StartAddress &= c_iAddress_Mask;
	
	
	// set the encoder to use
	e = InstanceEncoder;
	
	// the starting address needs to be on a block boundary
	BeginAddress = ( BeginAddress >> ( 3 + MaxStep_Shift ) ) << ( 3 + MaxStep_Shift );
	
	// save the address?
	Address = BeginAddress;
	
	// set the start address for the current block so recompiler can access it
	CurrentBlock_StartAddress = BeginAddress;
	
	// set the start address for the next block also
	NextBlock_StartAddress = CurrentBlock_StartAddress + ( 1 << ( 3 + MaxStep_Shift ) );
	
	// set the current optimization level
	v->OpLevel = v->OptimizeLevel;
	
	// get the block to encode in
	// new formula
	Block = ( BeginAddress >> ( 3 + MaxStep_Shift ) ) & NumBlocks_Mask;
	
	
	
	// start in code block
	e->StartCodeBlock ( Block );
	
	// set the start address for code block
	// address must actually match exactly. No mask
	StartAddress [ Block ] = BeginAddress;
	
	// set the instruction
	//Instructions [ Block ] = *((u32*) SrcCode);
	//pInstrPtr = & ( Instructions [ Block << MaxStep_Shift ] );
	
	
	// start cycles at zero
	Cycles = 0;
	
	// start PC
	//LocalPC = r->PC;
	
	
	// init count of recompiled instructions
	RecompileCount = 0;
	
	
	// want to stop at cache boundaries (would need extra code there anyways)
	// this is handled in loop now
	//MaxCount = MaxStep - ( ( Address >> 2 ) & MaxStep_Mask );
	//if ( MaxCount <= 0 ) MaxCount = 1;
	// set the maximum number of instructions to encode
	MaxCount = MaxStep;
	
	
	// NextPC has not been modified yet
	Local_NextPCModified = false;
	
	// some instructions need to stop encoding either before or after the instruction, at least for now
	// if stopping before, it keeps the instruction if there is nothing before it in the run
	v->bStopEncodingAfter = false;
	v->bStopEncodingBefore = false;
	
	// don't reset the cycle count yet
	bResetCycleCount = false;


	
	// should set local last modified register to 255
	Local_LastModifiedReg = 255;
	
	reti = 1;
	
	
	

	// clear delay slot
	//RDelaySlots [ 0 ].Value = 0;
	//RDelaySlots [ 1 ].Value = 0;

	// clear delay slot valid bits
	//RDelaySlots_Valid = 0;
	

	
	/////////////////////////////////////////////////////
	// note: multiply and divide require cycle count to be updated first
	// since they take more than one cycle to complete
	// same for mfhi and mflo, because they are interlocked
	// same for COP2 instructions
	// same for load and store
	// do the same for jumps and branches
	//////////////////////////////////////////////////////

	
	
	// get the starting block to store instruction addresses and cycle counts
	StartBlockIndex = ( Address >> 3 ) & ulIndex_Mask;
	BlockIndex = StartBlockIndex;

	// instruction count for current run
	RunCount = 0;
	
	// current delay slot index
	//DSIndex = 0;
	

	// initialize the previous instruction to zero
	v->PrevInst.Value = 0;
	CurInst.Value = 0;

	
	// this should get pointer to the instruction
	pSrcCodePtr = RGetPointer ( v, Address );
	
	
	
	// one cycle to fetch each instruction
	MemCycles = 1;


	// need to keep track of cycles for run
	//LocalCycleCount = MemCycles - 1;
	//CacheBlock_CycleCount = 0;
	LocalCycleCount = 0;

	// need to know of any other jumps to return
	RetJumpCounter = 0;
	
	
	
	// no branch delay slots yet
	v->Status_BranchDelay = 0;
	
	// no e-bit delay slots yet
	Status_EBit = 0;
	
#ifdef VERBOSE_RECOMPILE
cout << "\nRecompiler: Starting loop";
#endif

	// for loads
	// 1. check that there are no events. If so, update Cycles,NextPC, then return
	// 2. check for synchronous interrupt
	// 3. check that there are no conflicts. If so, put load in delay slot, update Cycles,NextPC, then return
	// 4. encode load, then encode load delay slot
	// 5. if going across cache line and next line is not loaded, then put load in delay slot and return
	// 6. if it is a store in the delay slot, then can just process normally as if there is no delay slot and immediately load
	
	// for stores
	// 1. check that there are no events. If so, update Cycles,NextPC, then return
	// 2. check for synchronous interrupt
	// 3. encode store
	
	// for jumps/branches
	// 1. check that there are no events. If so, update Cycles,NextPC, then return
	// 2. check for synchronous interrupt (for jumps that might have them)
	// 3. check that there are no loads,stores,branches,delay slots, in the delay slot. If so, put branch/jump in delay slot, update Cycles,NextPC, then return
	// 4. encode jump/branch then encode delay slot
	// 5. if branching backwards within same block, if cached then make sure cache-block is loaded and then jump, implement forward jumps later?
	// 6. if not branching within same block or forward jumping before implementation, then update Cycles,NextPC, then return
	// 7. if going across cache blocks and next block not loaded, then put in delay slot and return
	
	// other delay slot instructions
	// 1. check that there are no conflicts with delay slot. If so, update Cycles,NextPC, then return
	// 2. encode instruction then encode delay slot
	// 3. if going across cache blocks and next block not loaded, then put in delay slot and return
	
	// finding source registers
	// special instructions can use rs,rt as source registers
	// stores use rs,rt as source registers
	// immediates and loads use only rs as source register

	//for ( int i = 0; i < MaxStep; i++, Address += 4 )
	for ( i = 0; i < MaxCount; i++ )
	{
#ifdef VERBOSE_RECOMPILE
cout << "\nRecompiling: ADDR=" << hex << Address;
#endif


		RecompileCount = (Address & v->ulVuMem_Mask) >> 3;
		
		// set the address being recompiled for source device (vu 0/1)
		e->x64CurrentSourceAddress = Address;
		
		// start encoding a MIPS instruction
		e->StartInstructionBlock ();

		
#ifdef ENABLE_SINGLE_STEP
				
			v->bStopEncodingAfter = true;
#endif



#ifdef VERBOSE_RECOMPILE
cout << " RunCount=" << dec << RunCount;
#endif

		// the VUs run cycle by cycle, so if putting more than one instruction in a run will need to advance cycle
		// needs to be in transition to next instruction but not after starting point from main loop
		if ( RunCount )
		{
#ifdef VERBOSE_RECOMPILE
cout << " AdvanceCycle";
#endif
			
			// put the relevant stat/mac/clip flags into next snapshot entry if needed
			UpdateFlags_rec(e, v, Address - 8);

			// note: this is actually advancing the cycle for the previous address
			//static void AdvanceCycle ( VU* v )
			//Recompile_AdvanceCycle(v, Address);
			Recompile_AdvanceCycle(v, Address - 8);


#ifdef VERBOSE_RECOMPILE
cout << "->AdvanceCycle_DONE";
#endif

		}	// end if ( RunCount )
			
		
#ifdef VERBOSE_RECOMPILE
cout << " INSTR#" << dec << i;
//cout << " LOC=" << hex << ((u64) e->Get_CodeBlock_CurrentPtr ());
cout << " CycleDiff=" << dec << LocalCycleCount;
#endif


		// set r0 to zero for now
		//e->MovMemImm32 ( &r->GPR [ 0 ].u, 0 );
		
		// in front of the instruction, set NextPC to the next instruction
		// do this at beginning of code block
		// NextPC = PC + 4
		//e->MovMemImm32 ( &r->NextPC, Address + 4 );

		pSrcCodePtr = RGetPointer(v, Address);
		if (Address)
		{
			v->PrevInst.Lo.Value = *(pSrcCodePtr - 2);
			v->PrevInst.Hi.Value = *(pSrcCodePtr - 1);
		}
		else
		{
			// get the previous instruction
			v->PrevInst.ValueLoHi = CurInst.ValueLoHi;
		}

		if (Address >= 24)
		{
			v->PrevInst2.Lo.Value = *(pSrcCodePtr - 4);
			v->PrevInst2.Hi.Value = *(pSrcCodePtr - 3);

			v->PrevInst3.Lo.Value = *(pSrcCodePtr - 6);
			v->PrevInst3.Hi.Value = *(pSrcCodePtr - 5);
		}

		// get the instruction
		//inst.Value = *((u32*) SrcCode);
		instLO.Value = *(pSrcCodePtr + 0);
		instHI.Value = *(pSrcCodePtr + 1);
		
		CurInst.Lo.Value = instLO.Value;
		CurInst.Hi.Value = instHI.Value;
		
		// get the next instruction
		// note: this does not work if the next address is in a new cache block and the region is cached
		v->NextInstLO.Value = *(pSrcCodePtr + 2);
		v->NextInstHI.Value = *(pSrcCodePtr + 3);
		v->NextInst.Lo.Value = v->NextInstLO.Value;
		v->NextInst.Hi.Value = v->NextInstHI.Value;

		
		{
			// not in cached region //
			
			// still need to check against edge of block
			if ( ! ( ( Address + 8 ) & ( MaxStep_Mask << 3 ) ) )
			{
				// this can actually happen, so need to prevent optimizations there
				NextInst.Value = -1;
			}
		}
		
		


#ifdef VERBOSE_RECOMPILE
cout << " OL=" << v->OpLevel;
#endif

		// check if a forward branch target needs to be set
		if ( pForwardBranchTargets [ BlockIndex & MaxStep_Mask ] )
		{
			// set the branch target
			e->SetJmpTarget ( pForwardBranchTargets [ BlockIndex & MaxStep_Mask ] );
		}
		
		// this is internal to recompiler and says where heading for instruction starts at
		pPrefix_CodeStart [ BlockIndex & MaxStep_Mask ] = (u8*) e->Get_CodeBlock_CurrentPtr ();
		
		// this can be changed by the instruction being recompiled to point to where the starting entry point should be for instruction instead of prefix
		pCodeStart [ BlockIndex ] = (u8*) e->Get_CodeBlock_CurrentPtr ();
		
	
			// must add one to the cycle offset for starting point because the interpreter adds an extra cycle at the end of run
			//CycleCount [ BlockIndex ] = LocalCycleCount + 1;
			//CycleCount [ BlockIndex ] = LocalCycleCount;
			CycleCount[BlockIndex] = RecompileCount;

		
		//EndAddress [ BlockIndex ] = -1;
		


	
	if ( instHI.E )
	{
#ifdef INLINE_DEBUG
	debug << "; ***E-BIT SET***";
#endif

		//Status.EBitDelaySlot_Valid |= 0x2;
		e->OrMemImm32((int32_t*)&v->Status.ValueHi, 0x2);

		// if e-bit in e-bit delay slot, then need to use interpreter for that
		if (v->NextInstHI.E)
		{
			v->bStopEncodingAfter = true;
		}

		/*
		switch ( v->OpLevel )
		{
				
#ifdef ENABLE_EBIT_RECOMPILE
			case 1:
				Status_EBit = 2;
				
				e->MovMemImm32 ( & v->Recompiler_EnableEBitDelay, 1 );
				break;
#endif

			default:
				// delay slot after e-bit
				v->bStopEncodingAfter = true;

				//Status.EBitDelaySlot_Valid |= 0x2;
				e->OrMemImm32 ( (int32_t*) & v->Status.ValueHi, 0x2 );
				
				Status_EBit = 0;
				break;

		}
		*/

	}	// end if ( instHI.E )
	
	
#ifdef ENABLE_MBIT_RECOMPILE

	// M-bit must be VU0 only
	if ( !v->Number )
	{
		if ( instHI.M )
		{
#ifdef VERBOSE_RECOMPILE_MBIT
			// for now should alert
			cout << "\nhps2x64: VU0: NOTICE: M-bit set encountered during recompile!\n";
#endif
			
			// this should hopefully do the trick
			v->bStopEncodingAfter = true;
		}
	}

#endif	// end #ifdef ENABLE_MBIT_RECOMPILE
	
	
#ifdef VERBOSE_RECOMPILE_DBIT

	// alert if d or t is set
	//if ( CurInstHI.D )
	if ( instHI.D )
	{
		// register #28 is the FBRST register
		// the de bit says if the d-bit is enabled or not
		// de0 register looks to be bit 2
		// de1 register looks to be bit 10
		if ( !v->Number )
		{
			// check de0
			//if ( vi [ 28 ].u & ( 1 << 2 ) )
			//{
				cout << "\nhps2x64: ALERT: VU#" << v->Number << " D-bit is set! de0=" << hex << v->vi [ 28 ].u << "\n";
			//}
		}
		else
		{
			// check de1
			//if ( vi [ 28 ].u & ( 1 << 10 ) )
			//{
				cout << "\nhps2x64: ALERT: VU#" << v->Number << " D-bit is set! de1=" << hex << v->vi [ 28 ].u << "\n";
			//}
		}
	}

#endif	// end #ifdef VERBOSE_RECOMPILE_DBIT

	
#ifdef VERBOSE_RECOMPILE_TBIT

	//if ( CurInstHI.T )
	if ( instHI.T )
	{
		cout << "\nhps2x64: ALERT: VU#" << v->Number << " T-bit is set!\n";
	}

#endif	// end #ifdef VERBOSE_RECOMPILE_TBIT
	
	
	// execute HI instruction first ??
	
	// make sure the return values are set in case instruction is skipped
	retHi = 1;
	retLo = 1;
	
	// lower instruction has not set stat or clip flag
	v->SetStatus_Flag = 0;
	v->SetClip_Flag = 0;
	
	// check if Immediate or End of execution bit is set
	if ( instHI.I )
	{
		// lower instruction contains an immediate value //
		
		// first need to wait for source (and destination??) registers //


		// clear the bitmaps for source and destination registers
		Clear_FSrcReg ( v );
		Clear_ISrcReg ( v );
		Clear_DstReg ( v );
		

		OpLvlSave = v->OpLevel;
		v->OpLevel = -1;
		Vu::Recompiler::RecompileHI ( InstanceEncoder, v, instHI, Address );
		v->OpLevel = OpLvlSave;


		// before executing the instruction (I bit set) //

		// read cyclecount
		WaitForSrcRegs_ReadCycleCount(e, v, Address);

		// wait for source registers to become available
		WaitForFSrcRegs_rec(e, v, Address);

		// write-back cycle count
		WaitForSrcRegs_WriteCycleCount(e, v, Address);

		// wait for q/p registers if needed


		
		// start encoding a MIPS instruction
		e->StartInstructionBlock ();


		if ( !isNopHi ( instHI ) )
		{
#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
			// hi instruction can be forced to op level 1
			OpLvlSave = v->OpLevel;
			v->OpLevel = 1;
#endif

			// *important* MUST execute the HI instruction BEFORE storing the immediate
			retHi = Vu::Recompiler::RecompileHI ( InstanceEncoder, v, instHI, Address );

#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
			// restore op level
			v->OpLevel = OpLvlSave;
#endif
		}
#ifdef VERBOSE_RECOMPILE
		else
		{
cout << " NOP-HI";
		}
#endif
		
		// load immediate regiser with LO instruction
		//vi [ 21 ].u = CurInstLO.Value;
		//vi [ 21 ].u = CurInstLOHI.Lo.Value;
		ret = e->MovMemImm32(&v->vi[VU::REG_I].s, instLO.Value);
		

		// after the instruction
		// 1. store cycle# and dst mask into the dst reg index
		// 2. store mac+stat flag and cycle# (if either modified)

#ifdef SKIP_MISSING_HAZARDS_RAW
		// if there is a possible read-after-write hazard later, then store dst bitmap so it can be resolved when it happens
		if ((LUT_StaticInfo[RecompileCount] & STATIC_HAZARD_RAW_X_MASK))
#endif
		{
			SetDstRegsWait_ReadCycleCount(e, v, Address);

			SetFDstRegsWait_rec(e, v, Address);
		}


		// I-bit is set, so no int regs to store (would only be for int load)


		// put the relevant stat/mac/clip flags into next snapshot entry if needed
		//UpdateFlags_rec(e, v, Address);


	}
	else
	{
		// execute lo/hi instruction normally //
		// unsure of order
		

		// clear the bitmaps for source and destination registers
		Clear_FSrcReg ( v );
		Clear_ISrcReg ( v );
		Clear_DstReg ( v );
		

		OpLvlSave = v->OpLevel;
		v->OpLevel = -1;
		Vu::Recompiler::RecompileLO ( InstanceEncoder, v, instLO, Address );
		Vu::Recompiler::RecompileHI ( InstanceEncoder, v, instHI, Address );
		v->OpLevel = OpLvlSave;


		// read cyclecount
		WaitForSrcRegs_ReadCycleCount(e, v, Address);

		// wait for the float source registers to be ready in the pipeline
		WaitForFSrcRegs_rec(e, v, Address);

		// wait for the integer source registers to be ready in the pipeline
		WaitForISrcRegs_rec(e, v, Address);

		// write cyclecount
		WaitForSrcRegs_WriteCycleCount(e, v, Address);


		// recompile the instruction

#ifdef USE_NEW_RECOMPILE2_EXEORDER
		// check if should reverse execution order of upper/lower instructions (analysis bit 20)
		if (LUT_StaticInfo[RecompileCount] & STATIC_REVERSE_EXE_ORDER)
		{
			if (!isNopHi(instHI))
			{
#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
				// hi instruction can be forced to op level 1
				OpLvlSave = v->OpLevel;
				v->OpLevel = 1;
#endif

				retHi = Vu::Recompiler::RecompileHI(InstanceEncoder, v, instHI, Address);

#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
				// restore op level
				v->OpLevel = OpLvlSave;
#endif
			}
		}

#endif	// end #ifdef USE_NEW_RECOMPILE2_EXEORDER
		
#ifdef USE_NEW_RECOMPILE2_IGNORE_LOWER
		// check if should ignore lower instruction (analysis bit 4)
		if (!(LUT_StaticInfo[RecompileCount] & STATIC_SKIP_EXEC_LOWER))
#endif
		{
			if (!isNopLo(instLO))
			{
				retLo = Vu::Recompiler::RecompileLO(InstanceEncoder, v, instLO, Address);

			}
#ifdef VERBOSE_RECOMPILE
			else
			{
				cout << " NOP-LO";
			}
#endif
		}
		

#ifdef USE_NEW_RECOMPILE2_EXEORDER
		// check if should NOT reverse execution order of upper/lower instructions (analysis bit 20)
		if (!(LUT_StaticInfo[RecompileCount] & STATIC_REVERSE_EXE_ORDER))
#endif
		{
			if (!isNopHi(instHI))
			{
#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
				// hi instruction can be forced to op level 1
				OpLvlSave = v->OpLevel;
				v->OpLevel = 1;
#endif

				retHi = Vu::Recompiler::RecompileHI(InstanceEncoder, v, instHI, Address);

#ifdef FORCE_HI_CODE_TO_OPLEVEL_1
				// restore op level
				v->OpLevel = OpLvlSave;
#endif
			}
#ifdef VERBOSE_RECOMPILE
			else
			{
				cout << " NOP-HI";
			}
#endif
		}


		// check if need to complete move instruction (analysis bit 5) //
		if (LUT_StaticInfo[RecompileCount] & STATIC_COMPLETE_FLOAT_MOVE)
		{
#ifdef INLINE_DEBUG_RECOMPILE2
			VU::debug << "\r\n>COMPLETE-MOVE";
#endif

			if (instLO.Ft)
			{
				// load move data
				e->movdqa_regmem(RAX, &v->LoadMoveDelayReg.uw0);

				//if ( instLO.xyzw != 0xf )
				//{
				//	e->pblendwregmemimm ( RAX, & v->vf [ instLO.Ft ].sw0, ~( ( instLO.destx * 0x03 ) | ( instLO.desty * 0x0c ) | ( instLO.destz * 0x30 ) | ( instLO.destw * 0xc0 ) ) );
				//}

				e->movdqa_memreg(&v->vf[instLO.Ft].sw0, RAX);

				// make sure load/move delay slot is cleared
				e->MovMemImm8((char*)&v->Status.EnableLoadMoveDelaySlot, 0);
			}
		}


		// check if need to load int from delay slot (analysis bit 11) //
		if (LUT_StaticInfo[RecompileCount] & STATIC_MOVE_FROM_INT_DELAY_SLOT)
		{
#ifdef INLINE_DEBUG_RECOMPILE2
			VU::debug << "\r\n>INT-DELAY-SLOT";
#endif
			if (v->IntDelayReg & 0xf)
			{
				// make sure instruction count is not zero
				//e->MovRegFromMem64(RAX, (int64_t*)&v->InstrCount);
				//e->AddReg64ImmX(RAX, RecompileCount);

				// load int delay slot enable
				e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
				e->AndReg32ImmX(RAX, 0xf);

				// check that instruction count is greater than 1, else jump
				//e->Jmp8_E( 0, 0 );
				 
				// get int value
				e->MovRegFromMem16(RAX, (short*)&v->IntDelayValue);

				// load the current register value if instruction count is zero
				// note: can set IntDelayReg during recompile of instruction
				e->CmovERegMem16(RAX, (short*)&v->vi[v->IntDelayReg & 0xf].u);

				// store to correct register
				e->MovRegToMem16((short*)&v->vi[v->IntDelayReg & 0xf].u, RAX);

				// just in case the int calc was run at level zero, clear delay slots
				e->MovMemImm8((char*)&v->Status.IntDelayValid, 0);
			}

			// done
			//e->SetJmpTarget8 ( 0 );
		}

		// after the instruction
		// 1. store cycle# and dst mask into the dst reg index
		// 2. store mac+stat flag and cycle# (if either modified)

#ifdef SKIP_MISSING_HAZARDS_RAW
		if ((LUT_StaticInfo[RecompileCount] & (STATIC_HAZARD_RAW_X_MASK | STATIC_HAZARD_INT_RAW)))
#endif
		{
			// read the current cycle count#
			SetDstRegsWait_ReadCycleCount(e, v, Address);

#ifdef SKIP_MISSING_HAZARDS_RAW
			// if there is a possible read-after-write hazard later, then store dst bitmap so it can be resolved when it happens
			if ((LUT_StaticInfo[RecompileCount] & STATIC_HAZARD_RAW_X_MASK))
#endif
			{
				SetFDstRegsWait_rec(e, v, Address);
			}


#ifdef SKIP_MISSING_HAZARDS_RAW
			// if there is a possible read-after-write hazard later, then store dst bitmap so it can be resolved when it happens
			if ((LUT_StaticInfo[RecompileCount] & STATIC_HAZARD_INT_RAW))
#endif
			{
				SetIDstRegsWait_rec(e, v, Address);
			}

		}	// end if ((LUT_StaticInfo[RecompileCount] & (STATIC_HAZARD_RAW_X_MASK | STATIC_HAZARD_INT_RAW)))

		// put the relevant stat/mac/clip flags into next snapshot entry if needed
		//UpdateFlags_rec(e, v, Address);
	}

		
#ifdef VERBOSE_RECOMPILE
cout << " retLo=" << retLo;
cout << " retHi=" << retHi;
//cout << " ENC0=" << hex << (((u64*) (pCodeStart [ BlockIndex ])) [ 0 ]);
//cout << " ENC1=" << hex << (((u64*) (pCodeStart [ BlockIndex ])) [ 1 ]);
cout << " ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO ( instLO.Value ).c_str ();
//cout << " IDX-LO: " << dec << Vu::Instruction::Lookup::FindByInstructionLO ( instLO.Value );
cout << " ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI ( instHI.Value ).c_str ();
//cout << " IDX-HI: " << dec << Vu::Instruction::Lookup::FindByInstructionHI ( instHI.Value );
#endif


		// check if there was a problem recompiling hi or lo instruction at current level
		if ( ( retLo <= 0 ) || ( retHi <= 0 ) )
		{
			// there was a problem, and recompiling is done

#define VERBOSE_RECOMPILE_LEVEL0
#ifdef VERBOSE_RECOMPILE_LEVEL0
			// if this is in the middle of a parallel x2 instruction at any index, then possible problem
			if (LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_EXE_MASK)
			{
				//if ((LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX0)
				{
					cout << "\nHPS2X64: VU RECOMPILER: PROBLEM: LEVEL 0 RECOMPILER DURING PARALLEL INSTRUCTION\n";
					cout << " ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(instLO.Value).c_str();
					cout << " ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(instHI.Value).c_str();
				}

			}
#endif

			// need to undo whatever we did for this instruction
			e->UndoInstructionBlock ();
			Local_NextPCModified = false;
			
//cout << "\nUndo: Address=" << hex << Address;
			
			// TODO: if no instructions have been encoded yet, then just try again with a lower optimization level
			if ( v->OpLevel > 0 )
			{
//cout << "\nNext Op Level down";

				// could not encode the instruction at optimization level, so go down a level and try again
				v->OpLevel--;
				
				//Address -= 4;
				
				// at this point, this should be the last instruction since we had to go down an op level
				// this shouldn't be so, actually
				//MaxCount = 1;
				
				// here we need to reset and redo the instruction
				v->bStopEncodingBefore = false;
				v->bStopEncodingAfter = false;
				Local_NextPCModified = false;
				
				bResetCycleCount = false;
				
				
				// redo the instruction
				i--;
				continue;
			}
			else
			{
			
				cout << "\nhps2x64: VU#" << v->Number << ": Recompiler: Error: Unable to encode instruction. PC=" << hex << Address << " retLO=" << dec << retLo << " retHI=" << retHi;
				cout << "\n ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO ( instLO.Value ).c_str ();
				cout << "\n ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI ( instHI.Value ).c_str ();
				
				// mark block as unable to recompile if there were no instructions recompiled at all
				//if ( !Cycles ) DoNotCache [ Block ] = 1;
				
				// done
				break;
			}
		}
		
#ifdef ENABLE_SINGLE_STEP_BEFORE
			if ( !v->OpLevel )
			{
				v->bStopEncodingBefore = true;
			}
#endif
		
		
			// if this is not the first instruction, then it can halt encoding before it
			if ( RunCount && v->bStopEncodingBefore )
			{
#ifdef VERBOSE_RECOMPILE
cout << " v->bStopEncodingBefore";
#endif

				// stop enconding before the instruction already encoded //


				// if this is in the middle of a parallel x2 instruction at index 0, then problem
				if (LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_EXE_MASK)
				{
					if ((LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX0)
					{
						cout << "\nHPS2X64: VU RECOMPILER: PROBLEM: EXITING RECOMPILER BEFORE INDEX0 PARALLEL INSTRUCTION\n";
						cout << " ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(instLO.Value).c_str();
						cout << " ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(instHI.Value).c_str();
					}

				}

				/*
				if (isBranchOrJump(v->PrevInst))
				{
					cout << "\nHPS2X64: VU RECOMPILER: PROBLEM: exiting when there is a pending branch delay";
					cout << "\nAddress: " << hex << Address << "**************************";
					cout << "\nPREV-ASM: " << hex << v->PrevInst.Value;
					cout << "\nPREV-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->PrevInst.Lo.Value).c_str();
					cout << "\nPREV-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->PrevInst.Hi.Value).c_str();
					cout << "\nCUR-ASM: " << hex << CurInst.Value;
					cout << "\nCUR-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(CurInst.Lo.Value).c_str();
					cout << "\nCUR-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(CurInst.Hi.Value).c_str();
					cout << "\nNEXT-ASM: " << hex << v->NextInst.Value;
					cout << "\nNEXT-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->NextInst.Lo.Value).c_str();
					cout << "\nNEXT-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->NextInst.Hi.Value).c_str();
				}
				*/



#ifdef ENCODE_SINGLE_RUN_PER_BLOCK

				// first need to take back the instruction just encoded
				e->UndoInstructionBlock ();

				
				/*
				if ( Status_EBit == 1 )
				{
					//Status.EBitDelaySlot_Valid |= 0x2;
					e->OrMemImm32 ( (int32_t*) & v->Status.ValueHi, 0x2 );
				}
				*/

				
#ifdef UPDATE_BEFORE_RETURN
				// run count has not been updated yet for the instruction to stop encoding before
				if ( RunCount > 1 )
				{
					// check that NextPC was not modified
					// doesn't matter here except that RunCount>=1 so it is not first instruction in run, which is handled above
					// next pc was not modified because that will be handled differently now
					//if ( RunCount > 1 && !Local_NextPCModified )
					//{
						// update NextPC
						//e->MovMemImm32 ( (int32_t*) & v->NextPC, Address );


					//}
					
				}
#endif	// end #ifdef UPDATE_BEFORE_RETURN


#ifdef VERBOSE_RECOMPILE
cout << " RETURN";
#endif

				// clear instruction count
				e->AddMem64ImmX((int64_t*)&v->InstrCount, RecompileCount);

				// update cycle count
				//e->AddMem64ImmX((int64_t*)&v->CycleCount, RecompileCount);

				// update pc incase of branch ??
				e->MovMemImm32((int32_t*)&v->PC, Address);

				// update pc if not branching ?? or even if branching ??
				e->MovMemImm32((int32_t*)&v->NextPC, Address);

				// return;
				reti &= e->x64EncodeReturn ();


				
				// set the current optimization level
				// note: don't do this here, because the optimization level might have been changed by current instruction
				//v->OpLevel = v->OptimizeLevel;
				
				// reset flags
				v->bStopEncodingBefore = false;
				v->bStopEncodingAfter = false;
				Local_NextPCModified = false;
				
				bResetCycleCount = false;
				
				// starting a new run
				RunCount = 0;
				
				// restart cycle count back to zero
				LocalCycleCount = 0;
				
				
				
				
				// clear delay slots
				//RDelaySlots [ 0 ].Value = 0;
				//RDelaySlots [ 1 ].Value = 0;
				v->Status_BranchDelay = 0;

				// clear e-bit delay slot
				Status_EBit = 0;
				
				//LocalCycleCount = MemCycles - 1;
				
				// need to redo this instruction at this address
				// since I needed to insert the code to stop the block at this point
				i--;
				continue;
#else

				// do not encode instruction and done encoding
				e->UndoInstructionBlock ();
				Local_NextPCModified = false;
				break;

#endif
			} // end if ( RunCount && v->bStopEncodingBefore )

				


#ifdef ENABLE_BRANCH_DELAY_RECOMPILE

			// check if conditional/unconditional branch delay slot (analysis bits 8/9)
			if (LUT_StaticInfo[RecompileCount] & STATIC_ANY_BRANCH_DELAY_SLOT)
			{
				// check if branch is taken
				e->MovRegMem32(RCX, (int32_t*)&v->Recompiler_EnableBranchDelay);

				// whether or not branch is taken, need to clear condition
				// note: if branch in branch delay slot it should be interpreted
				// branch being handled
				e->MovMemImm32((int32_t*)&v->Recompiler_EnableBranchDelay, 0);

				// also clear delay slot valid
				//e->MovMemImm8((char*)&v->Status.DelaySlot_Valid, 0);

				//e->Jmp8_ECXZ(0, 1);
				e->OrRegReg32(RCX, RCX);
				e->Jmp_E(0, 1);

				// clear instruction count
				e->MovMemImm64((int64_t*)&v->InstrCount, -1);

				bool bAutoBranch = false;
#ifdef ENABLE_VU_AUTO_BRANCH

				// todo: for testing, this is VU1 ONLY
				// make sure this is not a delay slot
				//cout << "\nHPS2X64: VU: RECOMPILER: Checking auto branch.*** number=" << dec << v->Number << " RecompileCount=" << RecompileCount << " StaticInfo=" << hex << LUT_StaticInfo[RecompileCount];
				if (
					// for testing: start with VU1 only first
					(v->Number) &&

					// check for delay slots
					// note: this is a branch delay slot so it obviously can't be an xgkick delay slot
					// note: for now not accounting for moving from int delay slot (has to happen after getting branch address
					!(LUT_StaticInfo[RecompileCount] & (STATIC_MOVE_TO_INT_DELAY_SLOT | STATIC_EBIT_DELAY_SLOT | STATIC_XGKICK_DELAY_SLOT))

					// also for now watch out for e-bit in the branch delay slot
					&& !CurInst.Hi.E

					// also make sure it isn't a branch in a branch delay slot
					&& !isBranchOrJump(CurInst)
					)
				{

					//cout << "\nHPS2X64: VU: RECOMPILER: PrevInst=" << hex << v->PrevInst.ValueLoHi << " CurInst=" << instHILO.ValueLoHi << " NextInst=" << v->NextInst.ValueLoHi;

					// check if branch or jump delay slot
					if (isBranch(v->PrevInst))
					{
						// relative branch delay slot //

						// todo: check CycleCount against a stop cycle

						// get amount for relative branch and check if negative (backwards branch)
						int32_t branchamt = v->PrevInst.Lo.Imm11;

						//cout << "\nHPS2X64: VU: RECOMPILER: RecompileCount=" << dec << RecompileCount << " branchamt=" << branchamt;

						// get the address it is branching to
						int32_t addr = RecompileCount + branchamt;


						// check if forwards or backwards branch
						if (branchamt < 0)
						{
							// backwards branch //


							//cout << "\nHPS2X64: VU: RECOMPILER: RecompileCount=" << dec << RecompileCount << " branchamt=" << branchamt << " addr=" << addr;

							// validate the address is positive
							if (addr >= 0)
							{
								// target address ok and pointer is known //



								// advance CycleCount
								Recompile_AdvanceCycle(v);

								// set new address
								//e->MovMemImm32((int32_t*)&v->NextPC, (Address + (v->PrevInst.Lo.Imm11 << 3)) & v->ulVuMem_Mask);

								// get pointer into jump target addr
								u8* codeptr;
								codeptr = (u8*)(pCodeStart[addr]);

								// do the jump to absolute address
								e->JMP((void*)codeptr);

								// created an auto branch
								bAutoBranch = true;

								//cout << "\nHPS2X64: VU: RECOMPILER: Implementing backwards branch: branchamt=" << hex << branchamt << " targetaddr=" << hex << addr << " ptr=" << ((u64)codeptr);
							}
							else
							{
								cout << "\nHPS2X64: VU: RECOMPILER: PROBLEM: auto branch to address that is outside of VU code memory.\n";
							}
						}
						else
						{
							// forward branch //

							// todo: check CycleCount against a stop cycle

							// advance CycleCount
							Recompile_AdvanceCycle(v);

							// get a pointer to where the code pointer is stored
							//e->LeaRegMem64(RAX, &(pCodeStart[addr]));

							// jump to the address stored in memory
							e->JmpMem64((int64_t*) & (pCodeStart[addr]));

							// created an auto branch
							bAutoBranch = true;
						}

					}	// end if (isBranch( v->PrevInst ))
					else if (isJump(v->PrevInst))
					{
						// jump //
						
						// todo: check CycleCount against a stop cycle
					
						// advance CycleCount
						Recompile_AdvanceCycle(v);

						// get the address to jump to
						e->MovRegMem32(RAX, (int32_t*)&v->Recompiler_BranchDelayAddress);

						// get a pointer into code starting points
						e->LeaRegMem64(RCX, (void*)pCodeStart);

						// load the address
						e->MovRegFromMem64(RDX, RCX, RAX, SCALE_EIGHT, 0);

						// jump to the address
						// todo: combine this with previous instruction into 1
						e->JmpReg64(RDX);

						// created an auto branch
						bAutoBranch = true;
					}	// end else if (isJump(v->PrevInst))
				}

#endif

				// if not automatically branching then return
				if ( !bAutoBranch )
				{
					u32 NewAddr;

					NewAddr = ProcessBranch(e, v, v->Status_BranchInstruction, Address);

					// for now, update int delay slot after branch is calculated but before jump
					// unsure if int delay slot applies when it lands after jump or not yet

					// process pending int delay slot ?? //

					// note: the next three things should be checked before returning from VU code

					// check if integer has been put into delay slot
					if (LUT_StaticInfo[RecompileCount] & STATIC_MOVE_TO_INT_DELAY_SLOT)
					{
						/*
						// load int delay slot enable
						e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
						//e->AndReg32ImmX(RAX, 0xf);

						// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
						e->ShlRegImm32(RAX, 1);
						e->MovMemReg8((char*)&v->Status.IntDelayValid, RAX);
						*/

//#define VERBOSE_RECOMPILE_MOVE_DELAY
#ifdef VERBOSE_RECOMPILE_MOVE_DELAY
						//cout << " retLo=" << retLo;
						//cout << " retHi=" << retHi;
						//cout << " ENC0=" << hex << (((u64*) (pCodeStart [ BlockIndex ])) [ 0 ]);
						//cout << " ENC1=" << hex << (((u64*) (pCodeStart [ BlockIndex ])) [ 1 ]);
						cout << "\nAddress: " << hex << Address << "**************************";
						cout << "\nPREV-ASM: " << hex << v->PrevInst.Value;
						cout << "\nPREV-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->PrevInst.Lo.Value).c_str();
						//cout << " IDX-LO: " << dec << Vu::Instruction::Lookup::FindByInstructionLO ( instLO.Value );
						cout << "\nPREV-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->PrevInst.Hi.Value).c_str();
						//cout << " IDX-HI: " << dec << Vu::Instruction::Lookup::FindByInstructionHI ( instHI.Value );
						cout << "\nCUR-ASM: " << hex << CurInst.Value;
						cout << "\nCUR-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(CurInst.Lo.Value).c_str();
						cout << "\nCUR-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(CurInst.Hi.Value).c_str();
						cout << "\nNEXT-ASM: " << hex << v->NextInst.Value;
						cout << "\nNEXT-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->NextInst.Lo.Value).c_str();
						cout << "\nNEXT-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->NextInst.Hi.Value).c_str();
						if (NewAddr > 1)
						{
							u64 *pJumpInstPtr = (u64*) RGetPointer(v,NewAddr);
							Vu::Instruction::Format64 jinst64;
							jinst64.ValueLoHi = pJumpInstPtr[0];

							cout << "\nJUMPAddress: " << hex << NewAddr << "**************************";
							cout << "\nJUMP-ASM: " << hex << pJumpInstPtr[0];
							cout << "\nJUMP-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(jinst64.Lo.Value).c_str();
							cout << "\nJUMP-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(jinst64.Hi.Value).c_str();
						}
						else
						{
							cout << "\nDYNAMIC-JUMP";
						}
#endif

					}

					// check for e-bit delay slot
					if (LUT_StaticInfo[RecompileCount] & STATIC_EBIT_DELAY_SLOT)
					{
						//e->MovMemImm8((char*)&v->Status.EBitDelaySlot_Valid, 1);

						
						// load delay slot enable and shift right
						e->MovRegMem8(RAX, (char*)&v->Status.EBitDelaySlot_Valid);
						e->AndReg32ImmX(RAX, 0xf);

						// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
						e->ShrRegImm32(RAX, 1);
						e->MovMemReg8((char*)&v->Status.EBitDelaySlot_Valid, RAX);
						
					}

					/*
					// check for xgkick delay slot
					if (LUT_StaticInfo[RecompileCount] & STATIC_XGKICK_DELAY_SLOT)
					{
						//e->MovMemImm8((char*)&v->Status.XgKickDelay_Valid, 1);

						// load delay slot enable and shift right
						e->MovRegMem8(RAX, (char*)&v->Status.XgKickDelay_Valid);
						//e->AndReg32ImmX(RAX, 0xf);

						// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
						e->ShrRegImm32(RAX, 1);
						e->MovMemReg8((char*)&v->Status.XgKickDelay_Valid, RAX);
					}
					*/

					// done, return
					e->Ret();

				}	// end if ( !bAutoBranch )

				// jump to here if not jumping
				//e->SetJmpTarget8(1);
				e->SetJmpTarget(1);

				// clear vars
				v->Status_BranchDelay = 0;

			}	// end if (LUT_StaticInfo[RecompileCount] & STATIC_ANY_BRANCH_DELAY_SLOT)
			else
			{
				/*
				if (v->Status_BranchDelay == 1)
				{
					cout << "\nHPS2X64: VU: RECOMPILER: branch delay set but not handled.";
					v->Status_BranchDelay = 0;
					cout << "\nAddress: " << hex << Address << "**************************";
					cout << "\nPREV-ASM: " << hex << v->PrevInst.Value;
					cout << "\nPREV-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->PrevInst.Lo.Value).c_str();
					cout << "\nPREV-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->PrevInst.Hi.Value).c_str();
					cout << "\nCUR-ASM: " << hex << CurInst.Value;
					cout << "\nCUR-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(CurInst.Lo.Value).c_str();
					cout << "\nCUR-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(CurInst.Hi.Value).c_str();
					cout << "\nNEXT-ASM: " << hex << v->NextInst.Value;
					cout << "\nNEXT-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->NextInst.Lo.Value).c_str();
					cout << "\nNEXT-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->NextInst.Hi.Value).c_str();
				}
				*/
			}

#endif	// end #ifdef ENABLE_BRANCH_DELAY_RECOMPILE


#ifdef ENABLE_EBIT_RECOMPILE

			// check if xgkick delay slot (analysis bit 30) //
			if (LUT_StaticInfo[RecompileCount] & STATIC_XGKICK_DELAY_SLOT)
			{
#ifdef INLINE_DEBUG_RECOMPILE2
				VU::debug << "\r\n>XGKICK-DELAY-SLOT";
#endif

				// get updated instruction count
				//e->MovRegFromMem64(RAX, (int64_t*)&v->InstrCount);
				//e->AddReg64ImmX(RAX, RecompileCount);

				// get xgkick delay slot
				e->MovRegMem8(RAX, (char*)&v->Status.XgKickDelay_Valid);
				e->AndReg32ImmX(RAX, 0xf);

				// check that instruction count is greater than 1, else jump
				e->Jmp8_E(0, 2);

				// clear instruction count
				e->AddMem64ImmX((int64_t*)&v->InstrCount, RecompileCount);

				// update cycle count
				//e->AddMem64ImmX((int64_t*)&v->CycleCount, RecompileCount);

				// update pc incase of branch ??
				e->MovMemImm32((int32_t*)&v->PC, Address);

				// update pc if not branching ?? or even if branching ??
				e->MovMemImm32((int32_t*)&v->NextPC, Address + 8);

				// set xgkick
				e->MovMemImm8((char*)&v->Status.XgKickDelay_Valid, 1);

				// check if integer has been put into delay slot
				/*
				if (LUT_StaticInfo[RecompileCount] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					// load int delay slot enable
					e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
					//e->AndReg32ImmX(RAX, 0xf);

					// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
					e->ShlRegImm32(RAX, 1);
					e->MovMemReg8((char*)&v->Status.IntDelayValid, RAX);
				}
				*/

				// check for e-bit delay slot
				if (LUT_StaticInfo[RecompileCount] & STATIC_EBIT_DELAY_SLOT)
				{
					//e->MovMemImm8((char*)&v->Status.EBitDelaySlot_Valid, 1);

					// load delay slot enable and shift right
					e->MovRegMem8(RAX, (char*)&v->Status.EBitDelaySlot_Valid);
					e->AndReg32ImmX(RAX, 0xf);

					// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
					e->ShrRegImm32(RAX, 1);
					e->MovMemReg8((char*)&v->Status.EBitDelaySlot_Valid, RAX);
				}

				// return to allow any other transfers or potential transfers to catch up
				e->Ret();

				// catch jump(s) here
				e->SetJmpTarget8(2);
				//e->SetJmpTarget( 2 );
			}


			// check if e-bit delay slot (analysis bit 17) //
			// if not an xgkick delay slot, then check if just an e-bit delay slot
			else if (LUT_StaticInfo[RecompileCount] & STATIC_EBIT_DELAY_SLOT)
			{
#ifdef INLINE_DEBUG_RECOMPILE2
				VU::debug << "\r\n>E-BIT-DELAY-SLOT";
#endif
				// get updated instruction count
				//e->MovRegFromMem64(RAX, (int64_t*)&v->InstrCount);
				//e->AddReg64ImmX(RAX, RecompileCount);

				// get e-bit delay slot
				e->MovRegMem8(RAX, (char*)&v->Status.EBitDelaySlot_Valid);
				e->AndReg32ImmX(RAX, 0xf);

				// check that instruction count is greater than 1, else jump
				e->Jmp8_E(0, 2);

				// clear instruction count
				e->AddMem64ImmX((int64_t*)&v->InstrCount, RecompileCount);

				// update cycle count
				//e->AddMem64ImmX((int64_t*)&v->CycleCount, RecompileCount);

				// update pc incase of branch ??
				e->MovMemImm32((int32_t*)&v->PC, Address);

				// update pc if not branching ?? or even if branching ??
				e->MovMemImm32((int32_t*)&v->NextPC, Address + 8);

				e->MovMemImm8((char*)&v->Status.EBitDelaySlot_Valid, 1);

				// check if integer has been put into delay slot
				/*
				if (LUT_StaticInfo[RecompileCount] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					// load int delay slot enable
					e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
					//e->AndReg32ImmX(RAX, 0xf);

					// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
					e->ShlRegImm32(RAX, 1);
					e->MovMemReg8((char*)&v->Status.IntDelayValid, RAX);
				}
				*/

				// check for xgkick delay slot
				// note: checked for this already with an else+if
				/*
				if (LUT_StaticInfo[RecompileCount] & STATIC_XGKICK_DELAY_SLOT)
				{
					//e->MovMemImm8((char*)&v->Status.XgKickDelay_Valid, 1);

					// load delay slot enable and shift right
					e->MovRegMem8(RAX, (char*)&v->Status.XgKickDelay_Valid);
					//e->AndReg32ImmX(RAX, 0xf);

					// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
					e->ShrRegImm32(RAX, 1);
					e->MovMemReg8((char*)&v->Status.XgKickDelay_Valid, RAX);
				}
				*/

				// done, return
				e->Ret();

				// catch jump(s) here
				e->SetJmpTarget8(2);
				//e->SetJmpTarget( 2 );
			}



#endif	// end #ifdef ENABLE_EBIT_RECOMPILE



			// instruction successfully encoded from MIPS into x64
			e->EndInstructionBlock ();
			
//cout << "\nCool: Address=" << hex << Address << " ret=" << dec << ret << " inst=" << hex << *pSrcCodePtr << " i=" << dec << i;

			// update number of instructions that have been recompiled
			RecompileCount++;
			
			// update to next instruction
			//SrcCode += 4;
			// *pInstrPtr++ = *pSrcCodePtr++;
			//pSrcCodePtr++;
			pSrcCodePtr += 2;
			
			// add number of cycles encoded
			Cycles += ret;
			
			// update address
			//Address += 4;
			Address += 8;

			// update instruction count for run
			RunCount++;
			
			// go to next block index
			BlockIndex++;
			
			// update the cycles for run
			LocalCycleCount += MemCycles;


			// reset the optimization level for next instruction
			v->OpLevel = v->OptimizeLevel;
			
			
			// track branch delay for testing
			v->Status_BranchDelay >>= 1;

		
		// if directed to stop encoding after the instruction, then do so
		if ( v->bStopEncodingAfter )
		{
#ifdef VERBOSE_RECOMPILE
cout << " v->bStopEncodingAfter";
#endif

			// if this is in the middle of a parallel x2 instruction at index 1, then problem
			if (LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_EXE_MASK)
			{
				if (
					((LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX1)
					|| ((LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX2)
					|| ((LUT_StaticInfo[RecompileCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX3)
					)
				{
					cout << "\nHPS2X64: VU RECOMPILER: PROBLEM: EXITING RECOMPILER AFTER INDEX1/2/3 PARALLEL INSTRUCTION\n";
				}

			}

			/*
			//if (v->Status_BranchDelay)
			if (isBranchOrJump(CurInst))
			{
				cout << "\nHPS2X64: VU RECOMPILER: PROBLEM: exiting when there is a pending branch delay";
				cout << "\nAddress: " << hex << Address << "**************************";
				cout << "\nPREV-ASM: " << hex << v->PrevInst.Value;
				cout << "\nPREV-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->PrevInst.Lo.Value).c_str();
				cout << "\nPREV-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->PrevInst.Hi.Value).c_str();
				cout << "\nCUR-ASM: " << hex << CurInst.Value;
				cout << "\nCUR-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(CurInst.Lo.Value).c_str();
				cout << "\nCUR-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(CurInst.Hi.Value).c_str();
				cout << "\nNEXT-ASM: " << hex << v->NextInst.ValueLoHi << " Lo:" << v->NextInstLO.Value << " Hi:" << v->NextInstHI.Value;
				cout << "\nNEXT-ASM-LO: " << Vu::Instruction::Print::PrintInstructionLO(v->NextInst.Lo.Value).c_str();
				cout << "\nNEXT-ASM-HI: " << Vu::Instruction::Print::PrintInstructionHI(v->NextInst.Hi.Value).c_str();
			}
			*/

#ifdef ENCODE_SINGLE_RUN_PER_BLOCK

			//if ( Status_EBit == 1 )
			if ( instHI.E )
			{
				//Status.EBitDelaySlot_Valid |= 0x2;
				e->OrMemImm32 ( (int32_t*) & v->Status.ValueHi, 0x2 );
			}


#ifdef UPDATE_BEFORE_RETURN

			/*
			// run count has already been updated at this point, but still on instruction#1
			if ( RunCount > 1 )
			{
				// there is more than one instruction in run //
				
				// check that NextPC was not modified and that this is not an isolated instruction
				// actually just need to check if NextPC was modified by the encoded instruction
				if ( !Local_NextPCModified )
				{
					// update NextPC
					e->MovMemImm32 ( (int32_t*) & v->NextPC, Address );
				}




				// update CycleCount
				// *note* for VU cycle count is already updated
				//e->AddMem64ImmX ( & v->CycleCount, LocalCycleCount - MemCycles );
			}
			*/

#endif	// end #ifdef UPDATE_BEFORE_RETURN

				
#ifdef VERBOSE_RECOMPILE
cout << " RETURN";
#endif

			// clear instruction count
			e->AddMem64ImmX((int64_t*)&v->InstrCount, RecompileCount);

			// update cycle count
			//e->AddMem64ImmX((int64_t*)&v->CycleCount, RecompileCount);

			// update pc incase of branch ??
			e->MovMemImm32((int32_t*)&v->PC, Address - 8);

			// update pc if not branching ?? or even if branching ??
			e->MovMemImm32((int32_t*)&v->NextPC, Address);

			// check if integer has been put into delay slot
			// note: using RecompileCount-1 because this is returning after the current instruction, but RecompileCount was updated above
			/*
			if (LUT_StaticInfo[RecompileCount-1] & STATIC_MOVE_TO_INT_DELAY_SLOT)
			{
				// load int delay slot enable
				e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
				//e->AndReg32ImmX(RAX, 0xf);

				// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
				e->ShlRegImm32(RAX, 1);
				e->MovMemReg8((char*)&v->Status.IntDelayValid, RAX);
			}
			*/

			// return;
			reti &= e->x64EncodeReturn ();


			// set the current optimization level
			v->OpLevel = v->OptimizeLevel;
			
			// reset flags
			v->bStopEncodingBefore = false;
			v->bStopEncodingAfter = false;
			Local_NextPCModified = false;
			
			bResetCycleCount = false;
			
			// clear delay slots
			//RDelaySlots [ 0 ].Value = 0;
			//RDelaySlots [ 1 ].Value = 0;
			v->Status_BranchDelay = 0;
			
			// starting a new run
			RunCount = 0;
			
			// restart cycle count to zero
			LocalCycleCount = 0;


			// clear e-bit delay slot
			Status_EBit = 0;

			
			// cycle counts should start over
			//LocalCycleCount = MemCycles - 1;
			//CacheBlock_CycleCount = 0;
			
#else

				break;

#endif	// end #ifdef+else ENCODE_SINGLE_RUN_PER_BLOCK
		} // if ( v->bStopEncodingAfter )
			
			
			
		// reset flags
		v->bStopEncodingBefore = false;
		v->bStopEncodingAfter = false;
		Local_NextPCModified = false;
		
		bResetCycleCount = false;
		

	} // end for ( int i = 0; i < MaxStep; i++, Address += 4 )

#ifdef VERBOSE_RECOMPILE
cout << "\nRecompiler: Done with loop";
#endif




#ifdef ENCODE_SINGLE_RUN_PER_BLOCK

	// at end of block need to return ok //

	/*
	// encode return if it has not already been encoded at end of block
	if ( RunCount )
	{
	
#ifdef UPDATE_BEFORE_RETURN
		// check that NextPC was not modified and that this is not an isolated instruction
		if ( !Local_NextPCModified )
		{
			// update NextPC
			e->MovMemImm32 ( (int32_t*) & v->NextPC, Address );
		}
		
		// update CycleCount
		// after update need to put in the minus MemCycles
		// *note* for VU cycle count is already updated
		//e->AddMem64ImmX ( & v->CycleCount, LocalCycleCount - MemCycles );
#endif

	} // end if ( RunCount )
	*/



#ifdef VERBOSE_RECOMPILE
cout << "\nRecompiler: Encoding RETURN";
#endif


	// clear instruction count
	e->AddMem64ImmX((int64_t*)&v->InstrCount, RecompileCount);

	// update cycle count
	//e->AddMem64ImmX((int64_t*)&v->CycleCount, RecompileCount);

	// update pc incase of branch ??
	e->MovMemImm32((int32_t*)&v->PC, Address);

	// update pc if not branching ?? or even if branching ??
	e->MovMemImm32((int32_t*)&v->NextPC, Address + 8);


	// check if integer has been put into delay slot
	// note: using RecompileCount-1 because this is returning after the current instruction, but RecompileCount was updated above
	/*
	if (LUT_StaticInfo[RecompileCount-1] & STATIC_MOVE_TO_INT_DELAY_SLOT)
	{
		// load int delay slot enable
		e->MovRegMem8(RAX, (char*)&v->Status.IntDelayValid);
		//e->AndReg32ImmX(RAX, 0xf);

		// in this case, the integer won't be stored back until after the next instruction, so shift left and store back
		e->ShlRegImm32(RAX, 1);
		e->MovMemReg8((char*)&v->Status.IntDelayValid, RAX);
	}
	*/

	// return;
	reti &= e->x64EncodeReturn ();
	
#endif	// end #ifdef ENCODE_SINGLE_RUN_PER_BLOCK


	
	// done encoding block
	e->EndCodeBlock ();
	
	// address is now encoded
	
	
	if ( !reti )
	{
		cout << "\nRecompiler: Out of space in code block.";
	}

#ifdef VERBOSE_RECOMPILE
//cout << "\n(when all done)TEST0=" << hex << (((u64*) (pCodeStart [ 0x27e4 >> 2 ])) [ 0 ]);
//cout << " TEST1=" << hex << (((u64*) (pCodeStart [ 0x27e4 >> 2 ])) [ 1 ]);
#endif

	
	return reti;
	//return RecompileCount;
}



bool Recompiler::Perform_GetMacFlag ( x64Encoder *e, VU* v, u32 Address )
{
	// get updated cycle count, adjusted to compare value
	e->MovRegMem64(RCX, (int64_t*)&v->CycleCount);

	e->SubReg64ImmX(RCX, 3);

	// if ((CompareCycle >= FlagSave[0].ullBusyUntil_Cycle) && (CurCycle < FlagSave[0].ullBusyUntil_Cycle))
	// { CurFlag = FlagSave[0].ClipFlag; CurCycle = FlagSave[0].ullBusyUntil_Cycle; }
	e->MovReg32ImmX(RDX, 5);
	e->MovRegMem16(8, (short*)&v->vi[VU::REG_MACFLAG].u);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[0].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[0].MACFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[1].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[1].MACFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[2].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[2].MACFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[3].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	//e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[3].MACFlag);

	// return stat flag in rax
	e->MovRegReg32(RAX, 8);

	return true;
}

bool Recompiler::Perform_GetStatFlag ( x64Encoder *e, VU* v, u32 Address )
{
	// get updated cycle count, adjusted to compare value
	e->MovRegMem64(RCX, (int64_t*)&v->CycleCount);

	e->SubReg64ImmX(RCX, 3);

	// if ((CompareCycle >= FlagSave[0].ullBusyUntil_Cycle) && (CurCycle < FlagSave[0].ullBusyUntil_Cycle))
	// { CurFlag = FlagSave[0].ClipFlag; CurCycle = FlagSave[0].ullBusyUntil_Cycle; }
	e->MovReg32ImmX(RDX, 5);
	e->MovRegMem16(8, (short*)&v->vi[VU::REG_STATUSFLAG].u);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[0].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[0].StatusFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[1].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[1].StatusFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[2].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[2].StatusFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[3].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	//e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem16(8, (short*)&v->FlagSave[3].StatusFlag);

	// return stat flag in rax
	e->MovRegReg32(RAX, 8);

	return true;
}

bool Recompiler::Perform_GetClipFlag ( x64Encoder *e, VU* v, u32 Address )
{
	// get updated cycle count, adjusted to compare value
	e->MovRegMem64(RCX, (int64_t*)&v->CycleCount);

	e->SubReg64ImmX(RCX, 3);

	// if ((CompareCycle >= FlagSave[0].ullBusyUntil_Cycle) && (CurCycle < FlagSave[0].ullBusyUntil_Cycle))
	// { CurFlag = FlagSave[0].ClipFlag; CurCycle = FlagSave[0].ullBusyUntil_Cycle; }
	e->MovReg32ImmX(RDX, 5);
	e->MovRegMem32(8, (int32_t*)&v->vi[VU::REG_CLIPFLAG].u);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[0].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem32(8, (int32_t*)&v->FlagSave[0].ClipFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[1].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem32(8, (int32_t*)&v->FlagSave[1].ClipFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[2].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem32(8, (int32_t*)&v->FlagSave[2].ClipFlag);

	e->MovRegMem64(RAX, (int64_t*)&(v->FlagSave[3].ullBusyUntil_Cycle));
	e->SubRegReg64(RAX, RCX);
	e->CmpRegReg64(RAX, RDX);
	//e->CmovARegReg64(RDX, RAX);
	e->CmovARegMem32(8, (int32_t*)&v->FlagSave[3].ClipFlag);

	// return stat flag in rax
	e->MovRegReg32(RAX, 8);

	return true;
}

// performs a waitq (like for waitq,div,sqrt,rsqrt)
bool Recompiler::Perform_WaitQ(x64Encoder* e, VU* v, u32 Address)
{
	u32 RecompileCount;
	RecompileCount = (Address & v->ulVuMem_Mask) >> 3;

	/*
	e->AddMem64ImmX ( (int64_t*) & v->CycleCount, RecompileCount );

#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v );
			e->Call ( (const void*) VU::WaitQ_Micro2 );

#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

	e->SubMem64ImmX ( (int64_t*) & v->CycleCount, RecompileCount );
	*/

	//if (CycleCount < QBusyUntil_Cycle) CycleCount = QBusyUntil_Cycle;
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef BATCH_UPDATE_CYCLECOUNT
	// no need to add an offset if updating CycleCount in recompiler per instruction
	e->AddReg64ImmX(RAX, RecompileCount);
#endif

	e->CmpRegMem64(RAX, (int64_t*)&v->QBusyUntil_Cycle);
	e->CmovBRegMem64(RAX, (int64_t*)&v->QBusyUntil_Cycle);
	e->MovMemReg64((int64_t*)&v->QBusyUntil_Cycle, RAX);

	// vi [ REG_Q ].s = NextQ.l;
	e->MovRegMem32(RCX, (int32_t*)&v->NextQ);

	// vi [ REG_STATUSFLAG ].uLo &= ~0x30;
	// vi [ REG_STATUSFLAG ].uLo |= NextQ_Flag;
	e->MovRegMem32(RDX, (int32_t*)&v->vi[VU::REG_STATUSFLAG]);
	e->MovRegMem32(8, (int32_t*)&v->NextQ_Flag);
	e->AndReg32ImmX(RDX, ~0x30);
	e->OrRegReg32(RDX, 8);

	// NextQ_Flag &= ~0xc00;
	e->AndReg32ImmX(8, ~0xc00);

	e->MovMemReg32((int32_t*)&v->vi[VU::REG_Q].s, RCX);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG], RDX);
	e->MovMemReg32((int32_t*)&v->NextQ_Flag, 8);

	return true;
}

// doesn't wait for q, but updates it if needed (like for mulq, addq, etc)
bool Recompiler::Perform_UpdateQ(x64Encoder* e, VU* v, u32 Address)
{
	u32 RecompileCount;
	RecompileCount = (Address & v->ulVuMem_Mask) >> 3;

	/*
	e->AddMem64ImmX ( (int64_t*) & v->CycleCount, RecompileCount );

#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v );
			e->Call ( (const void*) VU::UpdateQ_Micro2 );

#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

	e->SubMem64ImmX ( (int64_t*) & v->CycleCount, RecompileCount );
	*/

	// if (CycleCount >= QBusyUntil_Cycle)
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

	// vi [ REG_Q ].s = NextQ.l;
	e->MovRegMem32(RCX, (int32_t*)&v->NextQ);

	// vi [ REG_STATUSFLAG ].uLo &= ~0x30;
	// vi [ REG_STATUSFLAG ].uLo |= NextQ_Flag;
	e->MovRegMem32(RDX, (int32_t*)&v->vi[VU::REG_STATUSFLAG]);
	e->MovRegMem32(8, (int32_t*)&v->NextQ_Flag);
	e->AndReg32ImmX(RDX, ~0x30);
	e->OrRegReg32(RDX, 8);

	// NextQ_Flag &= ~0xc00;
	e->AndReg32ImmX(8, ~0xc00);

#ifdef BATCH_UPDATE_CYCLECOUNT
	// no need to add an offset if updating CycleCount in recompiler per instruction
	e->AddReg64ImmX(RAX, RecompileCount);
#endif

	// if (CycleCount >= QBusyUntil_Cycle)
	e->CmpRegMem64(RAX, (int64_t*)&v->QBusyUntil_Cycle);
	e->CmovBRegMem32(RCX, (int32_t*)&v->vi[VU::REG_Q].s);
	e->CmovBRegMem32(RDX, (int32_t*)&v->vi[VU::REG_STATUSFLAG]);
	e->CmovBRegMem32(8, (int32_t*)&v->NextQ_Flag);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_Q].s, RCX);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_STATUSFLAG], RDX);
	e->MovMemReg32((int32_t*)&v->NextQ_Flag, 8);

	return true;
}



bool Recompiler::Perform_WaitP ( x64Encoder *e, VU* v, u32 Address )
{
	//if (CycleCount < PBusyUntil_Cycle) CycleCount = PBusyUntil_Cycle;
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef BATCH_UPDATE_CYCLECOUNT
	// no need to add an offset if updating CycleCount in recompiler per instruction
	e->AddReg64ImmX(RAX, RecompileCount);
#endif

	e->CmpRegMem64(RAX, (int64_t*)&v->PBusyUntil_Cycle);
	e->CmovBRegMem64(RAX, (int64_t*)&v->PBusyUntil_Cycle);
	e->MovMemReg64((int64_t*)&v->PBusyUntil_Cycle, RAX);

	e->MovRegMem32(RCX, (int32_t*)&v->NextP);
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].s, RCX);

	return true;
}



// doesn't wait for p, but updates it if needed (for mfp)
bool Recompiler::Perform_UpdateP ( x64Encoder *e, VU* v, u32 Address )
{
	u32 RecompileCount;

	RecompileCount = ( Address & v->ulVuMem_Mask ) >> 3;

	// get current cycle count
	e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

	// get next p value
	e->MovRegFromMem32(RCX, (int32_t*)&v->NextP.l);

#ifdef BATCH_UPDATE_CYCLECOUNT
	// no need to add an offset if updating CycleCount in recompiler per instruction
	e->AddReg64ImmX(RAX, RecompileCount);
#endif

	// check if ext unit is done
	e->CmpRegMem64(RAX, (int64_t*)&v->PBusyUntil_Cycle);

	// note: below or equal is the correct comparison here for UpdateP
	// (since it should be strictly less than PBusyUntil_Cycle+1)
	e->CmovBERegMem32(RCX, (int32_t*)&v->vi[VU::REG_P].u);

	// store correct p value
	e->MovMemReg32((int32_t*)&v->vi[VU::REG_P].u, RCX);

	return true;
}






// code generation //

// generate the instruction prefix to check for any pending events
// will also update NextPC,CycleCount (CPU) and return if there is an event
// will also update CycleCount (System) on a load or store
/*
int32_t Recompiler::Generate_Prefix_EventCheck ( u32 Address, bool bIsBranchOrJump )
{
	int32_t ret;
	
	// get updated CycleCount value for CPU
	e->MovRegMem64 ( RAX, & r->CycleCount );
	e->AddReg64ImmX ( RAX, LocalCycleCount );
	
	
	// want check that there are no events pending //
	
	// get the current cycle count and compare with next event cycle
	// note: actually need to either offset the next event cycle and correct when done or
	// or need to offset the next even cycle into another variable and check against that one
	e->CmpRegMem64 ( RAX, & Playstation1::System::_SYSTEM->NextEvent_Cycle );
	
	// branch if current cycle is greater (or equal?) than next event cycle
	// changing this so that it branches if not returning
	//e->Jmp_A ( 0, 100 + RetJumpCounter++ );
	e->Jmp8_B ( 0, 0 );
	
	// update NextPC
	e->MovMemImm32 ( & r->NextPC, Address );
	
	// update CPU CycleCount
	e->MovMemReg64 ( & r->CycleCount, RAX );
	
	// done for now - return
	ret = e->Ret ();
	
	// jump to here to continue execution in code block
	e->SetJmpTarget8 ( 0 );
	
	// if it is a branch or a jump, then no need to update the System CycleCount
	if ( !bIsBranchOrJump )
	{
		// since we have not reached the next event cycle, should write back the current system cycle
		// so that the correct cycle# gets seen when the store is executed
		// no need to update the CPU cycle count until either a branch/jump is encountered or returning
		// this way, there is no need to reset the current cycle number tally unless a branch/jump is encountered
		ret = e->MovMemReg64 ( & Playstation1::System::_SYSTEM->CycleCount, RAX );
	}

	return ret;
}
*/



int32_t Recompiler::INVALID ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "INVALID";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::INVALID;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ABS ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ABS";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ABS;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ABS
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

			
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ABS_RECOMPILE
		case 1:
			Generate_VABSp ( e, v, i );

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}




int32_t Recompiler::ADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADD;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADD_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDi_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ADDq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vi[VU::REG_Q].u);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDX_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDY_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDZ_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDW_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDA_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, -1, &v->dACC[0].l);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDAi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDAi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAi_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDAq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDAq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vi[VU::REG_Q].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDABCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDABCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAX_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDABCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDABCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAY_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDABCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDABCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAZ_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ADDABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ADDABCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ADDABCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ADDAW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ADDAW_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}










int32_t Recompiler::SUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUB";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUB;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUB
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUB_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBi_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vi[VU::REG_Q].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBX_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBY_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBZ_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBW_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBA_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, -1, &v->dACC[0].l);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBAi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBAi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAi_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBAq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBAq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vi[VU::REG_Q].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBABCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBABCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAX_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBABCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBABCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAY_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBABCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBABCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAZ_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SUBABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SUBABCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SUBABCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SUBAW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SUBAW_RECOMPILE
		case 1:

			ret = Generate_VADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::MUL ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MUL";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MUL;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MUL
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MUL_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULi_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0, NULL, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VMULp(e, v, i, 0, NULL, &v->vi[VU::REG_Q].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULX_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULY_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULZ_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULW_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, NULL, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULA_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x1b, &v->dACC[0].l);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULAi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULAi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAi_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0, &v->dACC[0].l, &v->vi[VU::REG_I].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULAq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULAq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

			ret = Generate_VMULp(e, v, i, 0, &v->dACC[0].l, &v->vi[VU::REG_Q].u);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULABCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULABCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAX_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULABCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULABCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAY_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULABCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULABCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAZ_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MULABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MULABCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MULABCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MULAW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MULAW_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x00, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}








int32_t Recompiler::MADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADD;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADD_RECOMPILE
		case 1:

			ret = Generate_VMADDp(e, v, 0, i);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDi_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vi[VU::REG_I].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vi[VU::REG_Q].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vi[VU::REG_Q].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw0, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0x00);
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDY_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw1, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0x55);
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDZ_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw2, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0xaa);
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDW_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw3, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0xff);
				ret = Generate_VMADDp(e, v, 0, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDA_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, nullptr, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 0, i, 0x1b, &v->dACC[0].l);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDAi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDAi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAi_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vi[VU::REG_I].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDAq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDAq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vi[VU::REG_Q].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vi[VU::REG_Q].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDABCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDABCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw0, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0x00, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDABCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDABCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAY_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw1, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0x55, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDABCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDABCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAZ_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw2, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0xaa, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MADDABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MADDABCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MADDABCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MADDAW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MADDAW_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, &v->vf[i.Ft].uw3, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 0, i, 0xff, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 0, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::MSUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUB";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUB;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUB
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUB_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, nullptr, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBi_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vi[VU::REG_I].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vi[VU::REG_Q].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vi[VU::REG_Q].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw0, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0x00);
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBY_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw1, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0x55);
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBZ_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw2, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0xaa);
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBW_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw3, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0xff);
				ret = Generate_VMADDp(e, v, 1, i, 0, NULL, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBA_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, nullptr, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0x1b, &v->dACC[0].l);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBAi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBAi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBAi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAi_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vi[VU::REG_I].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBAq ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBAq";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBAq;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAq
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAq_RECOMPILE
		case 1:

#ifdef ENABLE_ASYNC_QFLAG
			Perform_UpdateQ ( e, v, Address );
#endif

#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vi[VU::REG_Q].u, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vi[VU::REG_Q].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBABCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBABCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBABCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw0, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0x00, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBABCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBABCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBABCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAY_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw1, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0x55, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBABCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBABCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBABCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAZ_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw2, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0xaa, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MSUBABCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MSUBABCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MSUBABCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MSUBAW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MSUBAW_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2m(e, v, i, &v->vf[i.Ft].uw3, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				//ret = Generate_VMADDp(e, v, 1, i, 0xff, &v->dACC[0].l);
				ret = Generate_VMADDp(e, v, 1, i, 0, &v->dACC[0].l, &v->vf[i.Ft].vuw[i.bc]);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}




int32_t Recompiler::MAX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MAXi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAXi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAXi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAXi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::MAXBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAXBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAXBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAXX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i, &v->vf[i.Ft].uw0);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MAXBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAXBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAXBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAXY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i, &v->vf[i.Ft].uw1);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MAXBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAXBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAXBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAXZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i, &v->vf[i.Ft].uw2);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MAXBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MAXBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MAXBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MAXW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MAX_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MAX_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMAXpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMAXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMAXp(e, v, i, &v->vf[i.Ft].uw3);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}





int32_t Recompiler::MINI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINI";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINI;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINI
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MINIi ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINIi";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINIi;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINIi
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i, &v->vi[VU::REG_I].u);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::MINIBCX ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINIBCX";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINIBCX;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINIX
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i, &v->vf[i.Ft].uw0);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MINIBCY ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINIBCY";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINIBCY;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINIY
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i, &v->vf[i.Ft].uw1);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MINIBCZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINIBCZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINIBCZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINIZ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i, &v->vf[i.Ft].uw2);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MINIBCW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MINIBCW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MINIBCW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MINIW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcRegBC ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MIN_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MIN_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VMINpx2(e, v, i, v->PrevInst.Hi);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 });
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VMINpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi);
						break;
					}
				}
			}
			else
#endif
			{
				ret = Generate_VMINp(e, v, i, &v->vf[i.Ft].uw3);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}







int32_t Recompiler::FTOI0 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FTOI0";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FTOI0;
	
	int ret = 1;
	
	switch (v->OpLevel)
	{
#ifdef ENABLE_BITMAP_FTOI0
		// get source and destination register(s) bitmap
	case -1:
		//v->Set_SrcReg ( i.Value, i.Fs );
		Add_FSrcReg(v, i.Value, i.Fs);

		//v->Set_DestReg_Upper ( i.Value, i.Ft );
		Add_FDstReg(v, i.Value, i.Ft);
		break;
#endif

	case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
		e->SubReg64ImmX(RSP, c_lSEH_StackSize);
#endif

		// load arguments
		e->LeaRegMem64(RCX, v); e->LoadImm32(RDX, i.Value); e->MovMemImm32((int32_t*)&v->PC, Address);
		ret = e->Call(c_vFunction);

#ifdef RESERVE_STACK_FRAME_FOR_CALL
		ret = e->AddReg64ImmX(RSP, c_lSEH_StackSize);
#endif

		break;

#ifdef USE_NEW_FTOI0_RECOMPILE
	case 1:

#ifdef ENABLE_PARALLEL_FTOI0_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VFTOIXpx2(e, v, i, v->PrevInst.Hi, 0);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 0);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 0);
						break;
					}
				}
			}
			else
#endif
			{
				
				Generate_VFTOIXp(e, v, i, 0);
			}

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FTOI4 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FTOI4";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FTOI4;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_FTOI4
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );

			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FTOI4_RECOMPILE
		case 1:

#ifdef ENABLE_PARALLEL_FTOI4_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VFTOIXpx2(e, v, i, v->PrevInst.Hi, 4);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 4);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 4);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VFTOIXp(e, v, i, 4);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FTOI12 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FTOI12";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FTOI12;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_FTOI12
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FTOI12_RECOMPILE
		case 1:

#ifdef ENABLE_PARALLEL_FTOI12_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VFTOIXpx2(e, v, i, v->PrevInst.Hi, 12);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 12);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 12);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VFTOIXp(e, v, i, 12);
			}

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FTOI15 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FTOI15";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FTOI15;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_FTOI15
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FTOI15_RECOMPILE
		case 1:

#ifdef ENABLE_PARALLEL_FTOI15_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VFTOIXpx2(e, v, i, v->PrevInst.Hi, 15);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 15);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VFTOIXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 15);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VFTOIXp(e, v, i, 15);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ITOF0 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ITOF0";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ITOF0;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ITOF0
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ITOF0_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_ITOF0_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VITOFXpx2(e, v, i, v->PrevInst.Hi, 0);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 0);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 0);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VITOFXp(e, v, i, 0);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ITOF4 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ITOF4";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ITOF4;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ITOF4
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ITOF4_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_ITOF4_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VITOFXpx2(e, v, i, v->PrevInst.Hi, 4);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 4);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 4);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VITOFXp(e, v, i, 4);
			}

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ITOF12 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ITOF12";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ITOF12;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ITOF12
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ITOF12_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_ITOF12_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VITOFXpx2(e, v, i, v->PrevInst.Hi, 12);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 12);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 12);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VITOFXp(e, v, i, 12);
			}

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ITOF15 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ITOF15";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ITOF15;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ITOF15
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ITOF15_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_ITOF15_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS1p(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					switch (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
					{
					case STATIC_PARALLEL_EXEx2:
						Generate_VITOFXpx2(e, v, i, v->PrevInst.Hi, 15);
						break;
					case STATIC_PARALLEL_EXEx3:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, { 0 }, 15);
						break;
					case STATIC_PARALLEL_EXEx4:
						Generate_VITOFXpx4(e, v, i, v->PrevInst.Hi, v->PrevInst2.Hi, v->PrevInst3.Hi, 15);
						break;
					}
				}
			}
			else
#endif
			{
				Generate_VITOFXp(e, v, i, 15);
			}

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::NOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "NOP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::NOP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_NOP_RECOMPILE
		case 1:
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::OPMULA ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "OPMULA";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::OPMULA;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_OPMULA
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_OPMULA_RECOMPILE
		case 1:

			ret = Generate_VMULp(e, v, i, 0x84, &v->dACC[0].l, NULL, 0x60);
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::OPMSUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "OPMSUB";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::OPMSUB;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_OPMSUB
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Ft );
			
			//v->Set_DestReg_Upper ( i.Value, i.Fd );
			Add_FDstReg ( v, i.Value, i.Fd );
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_OPMSUB_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_MTYPE_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2opm(e, v, i, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VMADDpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif
			{
				ret = Generate_VMADDp(e, v, 1, i, 0x84, NULL, NULL, 0x60);
			}
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::CLIP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "CLIP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::CLIP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_CLIP
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );

			// fs.xyz
			Add_FSrcReg ( v, i.Value, i.Fs );

			// ft.w
			Add_FSrcReg ( v, 1 << 21, i.Ft );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_CLIP_RECOMPILE
		case 1:
#ifdef ENABLE_PARALLEL_CLIP_EXECUTE
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_EXE_MASK)
			{
				// put in the arguments
				Generate_XARGS2p(e, v, i, nullptr, 0, (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK) >> STATIC_PARALLEL_INDEX_SHIFT);

				// check if it is time to execute the instructions in parralel (index 0)
				if (!(v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_PARALLEL_INDEX_MASK))
				{
					Generate_VCLIPpx2(e, v, i, v->PrevInst.Hi);
				}
			}
			else
#endif

			// only set clip flag if not set by lower instruction
			if ( !v->SetClip_Flag )
			{
				// load clip flag
				e->MovRegMem32 ( RAX, &v->vi [ VU::REG_CLIPFLAG ].s );
				
				// flush ps2 float to zero
				e->movdqa_regmem ( RBX, &v->vf [ i.Ft ].sw0 );
				
				if ( !i.Fs )
				{
					e->pxorregreg ( RAX, RAX );
				}
				else if ( i.Fs == i.Ft )
				{
					e->movdqa_regreg ( RAX, RBX );
				}
				else
				{
					e->movdqa_regmem ( RAX, &v->vf [ i.Fs ].sw0 );
				}
				
				// get w from ft
				e->pshufdregregimm ( RBX, RBX, 0xff );
				
				
				// get +w into RBX
				e->pslldregimm ( RBX, 1 );
				e->psrldregimm ( RBX, 1 );
				
				// get -w into RCX
				e->pcmpeqbregreg ( RCX, RCX );
				e->movdqa_regreg ( RDX, RCX );
				e->pxorregreg ( RCX, RBX );
				//e->psubdregreg ( RCX, RDX );
				
				// get x,y from fs into RDX
				e->pshufdregregimm ( RDX, RAX, ( 1 << 6 ) | ( 1 << 4 ) | ( 0 << 2 ) | ( 0 << 0 ) );
				e->movdqa_regreg ( 4, RDX );
				e->psradregimm ( 4, 31 );
				//e->pslldregimm ( RDX, 1 );
				//e->psrldregimm ( RDX, 1 );
				e->psrldregimm ( 4, 1 );
				e->pxorregreg ( RDX, 4 );
				//e->psubdregreg ( RDX, 4 );
				
				// get greater than +w into R4 and less than -w into R5
				e->movdqa_regreg ( 4, RDX );
				e->pcmpgtdregreg ( 4, RBX );
				e->movdqa_regreg ( 5, RCX );
				e->pcmpgtdregreg ( 5, RDX );
				
				// get x and y flags into R4
				e->pblendwregregimm ( 4, 5, 0xcc );
				
				
				// get z from fs into RAX
				e->pshufdregregimm ( RAX, RAX, ( 2 << 6 ) | ( 2 << 4 ) | ( 2 << 2 ) | ( 2 << 0 ) );
				e->movdqa_regreg ( 5, RAX );
				e->psradregimm ( 5, 31 );
				//e->pslldregimm ( RAX, 1 );
				//e->psrldregimm ( RAX, 1 );
				e->psrldregimm ( 5, 1 );
				e->pxorregreg ( RAX, 5 );
				//e->psubdregreg ( RAX, 5 );
				
				// get greater than into RAX and less than into RCX
				e->pcmpgtdregreg ( RCX, RAX );
				e->pcmpgtdregreg ( RAX, RBX );
				
				// get z flags into RAX
				e->pblendwregregimm ( RAX, RCX, 0xcc );
				
				// pull flags
				e->movmskpsregreg ( RCX, 4 );
				e->movmskpsregreg ( RDX, RAX );
				
				// combine flags
				e->ShlRegImm32 ( RDX, 4 );
				e->OrRegReg32 ( RCX, RDX );
				e->AndReg32ImmX ( RCX, 0x3f );
				
				// combine into rest of the clipping flags
				e->ShlRegImm32 ( RAX, 6 );
				e->OrRegReg32 ( RAX, RCX );
				e->AndReg32ImmX ( RAX, 0x00ffffff );
				
				// write back to clipping flag
				e->MovMemReg32 ( &v->vi [ VU::REG_CLIPFLAG ].s, RAX );
				
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}





// lower instructions

int32_t Recompiler::DIV ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "DIV";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::DIV;
	
	static const u64 c_CycleTime = 7;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_DIV
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegBC ( i.ftf, i.Ft );
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.ftf, i.Ft );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

			// ***TODO*** when div affects flags it should also affect all snapshots of the flags too
			// ***TODO*** at what point does div affect flag register? immediately or later ?
#ifdef USE_NEW_DIV_RECOMPILE
		case 1:

			// have to do a wait-q to update the q-reg and flags with the current values before setting the next values
			// otherwise the current values get overwritten before being set
			Perform_WaitQ ( e, v, Address );

			// now can do the DIV //

			e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vsw[i.fsf]);
			e->movd1_regmem(RCX, (int32_t*)&v->vf[i.Ft].vsw[i.ftf]);

			// make constant multiplier ->rbx
			//e->MovReg64ImmX(RAX, 127ull << 52);
			e->MovReg64ImmX(RAX, 896ull << 23);
			e->movq_to_sse128(RBX, RAX);

			// check if fs,ft is float zero
			// 0 -> r5
			e->pxor1regreg(RDX, RDX, RDX);
			e->paddd1regreg(4, RAX, RAX);
			e->pcmpeqb1regreg(4, 4, RDX);
			e->paddd1regreg(5, RCX, RCX);
			e->pcmpeqb1regreg(5, 5, RDX);

			// make masks
			e->pcmpgtd1regreg(4, RDX, 4);
			e->pcmpgtd1regreg(5, RDX, 5);


			// check if fs==0 and ft==0 -> invalid ->r4
			e->pand1regreg(4, 4, 5);

			// check if fs!=0 and ft==0 -> div by zero -> r5
			//e->pandn1regreg(5, 4, 5);

			e->movd_from_sse128(RAX, 4);
			e->movd_from_sse128(RCX, 5);


			// sign ->rdx
			e->pxor1regreg(RDX, RAX, RCX);

			// adjust fs if 0/0
			e->por1regreg(RAX, RAX, 4);

			// cvt float to double
			e->psllq1regimm(RAX, RAX, 33);
			e->psrlq1regimm(RAX, RAX, 4);
			e->psllq1regimm(RCX, RCX, 33);
			e->psrlq1regimm(RCX, RCX, 4);


			// divide
			e->vdivsd(RAX, RAX, RCX);

			// multiply with multiplier
			//e->vmulsd(RAX, RAX, RBX);

			// make adjustment ->r4
			e->MovReg64ImmX(RDX, 1ull << 28);
			e->movq_to_sse128(4, RDX);

			// adjust result
			e->paddq1regreg(RAX, RAX, 4);

			// get result
			e->psrlq1regimm(RAX, RAX, 29);

			// offset exponent
			e->psubq1regreg(RAX, RAX, RBX);

			// clear on underflow or zero
			e->pxor1regreg(5, 5, 5);
			e->pcmpgtq1regreg(4, 5, RAX);
			e->pandn1regreg(RAX, 4, RAX);

			// maximize on overflow
			e->pcmpeqb1regreg(5, 5, 5);
			e->psrlq1regimm(4, 5, 33);
			e->pcmpgtq1regreg(RBX, RAX, 4);
			e->por1regreg(RAX, RAX, RBX);
			e->pand1regreg(RAX, RAX, 4);

			// put sign in
			e->pandn1regreg(RDX, 4, RDX);
			e->por1regreg(RAX, RAX, RDX);

			// store result
			e->movd1_memreg((int32_t*)&v->NextQ.l, RAX);

			// get flags
			//e->MovRegMem32(RDX, (int32_t*)&statflag);
			e->XorRegReg32(RCX, RAX);
			e->AndReg32ImmX(RAX, 0x410);
			e->AndReg32ImmX(RCX, 0x820);
			e->OrRegReg32(RAX, RCX);
			//e->AndReg32ImmX(RDX, ~0x30000);
			//e->OrRegReg32(RAX, RDX);
			e->MovMemReg32((int32_t*)&v->NextQ_Flag, RAX);



			// set time to process
			e->MovRegMem64 ( RAX, (int64_t*) & v->CycleCount );

#ifdef USE_NEW_RECOMPILE2_DIV
			e->AddReg64ImmX ( RAX, c_CycleTime + ( ( Address & v->ulVuMem_Mask ) >> 3 ) );
#else
			e->AddReg64ImmX ( RAX, c_CycleTime );
#endif

			e->MovMemReg64 ( (int64_t*) & v->QBusyUntil_Cycle, RAX );

			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IADD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IADD;
	
	int ret = 1;

	// set int delay reg for recompiler
	v->IntDelayReg = i.id & 0xf;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_IADD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );q (
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IADD_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.id & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.id & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.id & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->AddRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( ( !( i.is & 0xf ) ) && ( !( i.it & 0xf ) ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.id & 0xf ].u, 0 );
					}
					else if ( ( !( i.is & 0xf ) ) || ( !( i.it & 0xf ) ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ ( i.is & 0xf ) + ( i.it & 0xf ) ].u );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.is & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->AddMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AddMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.is & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AddRegReg16 ( RAX, RAX );
						
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AddRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
				}
			}
			
			break;
#endif

			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IADDI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IADDI";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IADDI;
	
	int ret = 1;

	// set int delay reg for recompiler
	v->IntDelayReg = i.it & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_IADDI
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IADDI_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.it & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->AddRegImm16 ( RAX, ( (s16) i.Imm5 ) );
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( !( i.is & 0xf ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.it & 0xf ].s, ( (s16) i.Imm5 ) );
					}
					else if ( i.it == i.is )
					{
						e->AddMemImm16 ( (s16*) &v->vi [ i.it & 0xf ].s, ( (s16) i.Imm5 ) );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AddRegImm16 ( RAX, ( (s16) i.Imm5 ) );
						e->MovMemReg16 ( (s16*) &v->vi [ i.it & 0xf ].s, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}


int32_t Recompiler::IADDIU ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IADDIU";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IADDIU;
	
	int ret = 1;
	
	// set int delay reg for recompiler
	v->IntDelayReg = i.it & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_IADDIU
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			break;
#endif

		case 0:
			// integer math at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IADDIU_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.it & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem32 ( RAX, &v->vi [ i.is & 0xf ].s );
					e->AddRegImm16 ( RAX, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					e->MovMemReg32 ( (int32_t*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( !( i.is & 0xf ) )
					{
						e->MovMemImm16 ( &v->vi [ i.it & 0xf ].sLo, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					}
					else if ( i.it == i.is )
					{
						e->AddMemImm16 ( &v->vi [ i.it & 0xf ].sLo, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					}
					else
					{
						e->MovRegMem32 ( RAX, &v->vi [ i.is & 0xf ].s );
						e->AddRegImm16 ( RAX, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
						e->MovMemReg32 ( &v->vi [ i.it & 0xf ].s, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IAND";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IAND;
	
	int ret = 1;
	
	// set int delay reg for recompiler
	v->IntDelayReg = i.id & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_IAND
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IAND_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.id & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.id & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.id & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->AndRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( ( !( i.is & 0xf ) ) || ( !( i.it & 0xf ) ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.id & 0xf ].u, 0 );
					}
					else if ( ( i.is & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.is & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->AndMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AndMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->AndRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::IOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IOR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IOR;
	
	int ret = 1;
	
	// set int delay reg for recompiler
	v->IntDelayReg = i.id & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_IOR
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IOR_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.id & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.id & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.id & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->OrRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( ( !( i.is & 0xf ) ) && ( !( i.it & 0xf ) ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.id & 0xf ].u, 0 );
					}
					else if ( ( !( i.is & 0xf ) ) || ( !( i.it & 0xf ) ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ ( i.is & 0xf ) + ( i.it & 0xf ) ].u );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.is & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.is & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->OrMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->OrMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->OrRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ISUB ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ISUB";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ISUB;
	
	int ret = 1;
	
	// set int delay reg for recompiler
	v->IntDelayReg = i.id & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ISUB
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ISUB_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.id & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.id & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.id & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->SubRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
					
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					if ( ( !( i.is & 0xf ) ) && ( !( i.it & 0xf ) ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.id & 0xf ].u, 0 );
					}
					else if ( ( i.is & 0xf ) == ( i.it & 0xf ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.id & 0xf ].u, 0 );
					}
					else if ( !( i.it & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( !( i.is & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->NegReg16 ( RAX );
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else if ( ( i.id & 0xf ) == ( i.is & 0xf ) )
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						e->SubMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->SubRegMem16 ( RAX, (s16*) &v->vi [ i.it & 0xf ].s );
						
						e->MovMemReg16 ( (s16*) &v->vi [ i.id & 0xf ].u, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}


int32_t Recompiler::ISUBIU ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ISUBIU";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ISUBIU;
	
	int ret = 1;
	
	// set int delay reg for recompiler
	v->IntDelayReg = i.it & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ISUBIU
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			break;
#endif

		case 0:
			// integer math at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ISUBIU_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.it & 0xf )
			{
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
					e->SubRegImm16 ( RAX, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
				
					if ( !( i.is & 0xf ) )
					{
						e->MovMemImm16 ( (s16*) &v->vi [ i.it & 0xf ].s, -( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					}
					else if ( i.it == i.is )
					{
						e->SubMemImm16 ( (s16*) &v->vi [ i.it & 0xf ].s, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
					}
					else
					{
						e->MovRegMem16 ( RAX, (s16*) &v->vi [ i.is & 0xf ].s );
						e->SubRegImm16 ( RAX, ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
						e->MovMemReg16 ( (s16*) &v->vi [ i.it & 0xf ].s, RAX );
					}
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}




int32_t Recompiler::ILWR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ILWR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ILWR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ILWR
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
	
			//v->Set_DestReg_Lower ( ( i.it & 0xf ) + 32 );
			Add_IDstReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// load at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ILWR_RECOMPILE
		case 1:
		
#ifdef DISABLE_ILWR_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.it )
			{
				//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
				e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
				
				
				//pVuMem32 = v->GetMemPtr ( LoadAddress );
				//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
				//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
				e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );

				// special code for VU#0
				if ( !v->Number )
				{
					// check if Address & 0xf000 == 0x4000
					e->MovRegReg32 ( RDX, RAX );
					e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
					e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
					
					// here it will be loading/storing from/to the registers for VU#1
					e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
					e->CmovERegReg64 ( RCX, RDX );
					
					// ***TODO*** check if storing to TPC
				}
				
				if ( !v->Number )
				{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
				}
				else
				{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
				}
				e->AddRegReg32 ( RAX, RAX );
				
				switch( i.xyzw )
				{
					case 8:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
						break;
						
					case 4:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 4 );
						break;
						
					case 2:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 8 );
						break;
						
					case 1:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 12 );
						break;
						
					default:
						cout << "\nVU: Recompiler: ALERT: ILWR with illegal xyzw=" << hex << i.xyzw << "\n";
						break;
				}
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				//}
				//ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );
				
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ISWR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ISWR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ISWR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ISWR
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ISWR_RECOMPILE
		case 1:
		
#ifdef DISABLE_ISWR_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
				
			//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
			e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
			e->movd_regmem ( RAX, & v->vi [ i.it & 0xf ].s );
			
			//e->movdqa_regmem ( RAX, & v->vf [ i.Fs ].sw0 );
			
			//pVuMem32 = v->GetMemPtr ( LoadAddress );
			//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
			//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
			e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );

			// special code for VU#0
			if ( !v->Number )
			{
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
				e->AndReg32ImmX ( RAX, 0x7fff );
#endif

				// check if Address & 0xf000 == 0x4000
				e->MovRegReg32 ( RDX, RAX );
				e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
				e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
				e->Jmp8_NE ( 0, 1 );
				
				// here it will be loading/storing from/to the registers for VU#1
				//e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				//e->CmovERegReg64 ( RCX, RDX );
				e->LeaRegMem64 ( RCX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				
				// ***TODO*** check if storing to TPC
				e->CmpReg32ImmX ( RAX, 0x43a );
				e->Jmp8_NE ( 0, 0 );
				
				//VU1::_VU1->Running = 1;
				e->MovMemImm32 ( (int32_t*) & VU1::_VU1->Running, 1 );
				
				//VU1::_VU1->CycleCount = *VU::_DebugCycleCount + 1;
				e->MovRegMem64 ( RDX, (int64_t*) VU::_DebugCycleCount );
				
				// set VBSx in VPU STAT to 1 (running)
				//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( 1 << 3 ) );
				e->OrMemImm32 ( & VU0::_VU0->vi [ 29 ].s, ( 1 << ( 1 << 3 ) ) );
				
				// also set VIFx STAT to indicate program is running
				//VU1::_VU1->VifRegs.STAT.VEW = 1;
				e->OrMemImm32 ( (int32_t*) & VU1::_VU1->VifRegs.STAT.Value, ( 1 << 2 ) );
				
				// finish handling the new cycle count
				e->IncReg64 ( RDX );
				e->MovMemReg64 ( (int64_t*) & VU1::_VU1->CycleCount, RDX );
				
				e->SetJmpTarget8 ( 0 );
				
				// mask address for accessing registers
				e->AndReg32ImmX ( RAX, 0x3f );
				
				e->SetJmpTarget8 ( 1 );
			}
			
			if ( !v->Number )
			{
			e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
			}
			else
			{
			e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
			}
			e->AddRegReg32 ( RAX, RAX );

			if ( i.xyzw != 0xf )
			{
				e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
			}
			
			e->pmovzxwdregreg ( RAX, RAX );
			e->pshufdregregimm ( RAX, RAX, 0 );
			//e->pslldregimm ( RAX, 16 );
			//e->psrldregimm ( RAX, 16 );
			
			if ( i.xyzw != 0xf )
			{
				//e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
				e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			//ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );
			ret = e->movdqa_to_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::LQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "LQD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::LQD;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	v->IntDelayReg = i.is & 0xf;
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_LQD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			// destination for move instruction needs to be set only if move is made
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// load at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_LQD_RECOMPILE
		case 1:
		
#ifdef DISABLE_LQD_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif

			/*		
#ifdef ENABLE_INTDELAYSLOT_LQD
			// check for a conditional branch that might be affected by integer destination register
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.is & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.is & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
#endif

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			
			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
			*/

			if ( i.Ft )
			{
				// add destination register to bitmap at end
				//Add_FDstReg ( v, i.Value, i.Ft );
				
				//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
				e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
				
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->movdqa_regmem ( RCX, & v->vf [ i.Ft ].sw0 );
				//}
				
				//pVuMem32 = v->GetMemPtr ( LoadAddress );
				//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
				//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
				e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
				
				// special code for VU#0
				if ( !v->Number )
				{
					// check if Address & 0xf000 == 0x4000
					e->MovRegReg32 ( RDX, RAX );
					e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
					e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
					
					// here it will be loading/storing from/to the registers for VU#1
					e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
					e->CmovERegReg64 ( RCX, RDX );
					
				}
				
				
				
				// post-inc
				if ( i.is & 0xf )
				{
					e->DecReg16 ( RAX );

					// analysis bit 10 - output int to int delay slot - next conditional branch uses the register
					if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
					{
						e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
					else
					{
						e->MovMemReg16 ( & v->vi [ i.is & 0xf ].sLo, RAX );
					}
				}
				
				if ( !v->Number )
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
				}
				else
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
				}
				e->AddRegReg32 ( RAX, RAX );
				
				//e->MovRegFromMem32 ( RDX, RCX, RAX, SCALE_EIGHT, 0 );
				e->movdqa_from_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
				
#ifdef USE_NEW_VECTOR_DISPATCH_LQD_VU

				ret = Dispatch_Result_AVX2(e, v, i, false, RAX, i.Ft);

#else
				if ( i.xyzw != 0xf )
				{
					//e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm ( RAX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );
#endif
			}
			else
			{
				if ( i.is & 0xf )
				{
					e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
					e->DecReg16 ( RAX );
					if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
					{
						e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
					else
					{
						e->MovMemReg16 ( & v->vi [ i.is & 0xf ].sLo, RAX );
					}
				}
			}

				/*
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::LQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "LQI";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::LQI;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	v->IntDelayReg = i.is & 0xf;
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_LQI
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			// destination for move instruction needs to be set only if move is made
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// load at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_LQI_RECOMPILE
		case 1:
		
#ifdef DISABLE_LQI_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif

			/*
#ifdef ENABLE_INTDELAYSLOT_LQI
			// check for a conditional branch that might be affected by integer destination register
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.is & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.is & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
#endif

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			
			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
			*/

			if ( i.Ft )
			{
				// add destination register to bitmap at end
				//Add_FDstReg ( v, i.Value, i.Ft );
				
				//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
				e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
				
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->movdqa_regmem ( RCX, & v->vf [ i.Ft ].sw0 );
				//}

				//pVuMem32 = v->GetMemPtr ( LoadAddress );
				//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
				//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
				e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
				
				// special code for VU#0
				if ( !v->Number )
				{
					// check if Address & 0xf000 == 0x4000
					e->MovRegReg32 ( RDX, RAX );
					e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
					e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
					
					// here it will be loading/storing from/to the registers for VU#1
					e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
					e->CmovERegReg64 ( RCX, RDX );
					
				}

				
				// post-inc
				if ( i.is & 0xf )
				{
					e->LeaRegRegImm32 ( RDX, RAX, 1 );

					// analysis bit 10 - output int to int delay slot - next conditional branch uses the register
					if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
					{
						e->MovMemReg16 ( (s16*) &v->IntDelayValue, RDX );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
					else
					{
						e->MovMemReg16 ( & v->vi [ i.is & 0xf ].sLo, RDX );
					}
				}
				
				if ( !v->Number )
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
				}
				else
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
				}
				e->AddRegReg32 ( RAX, RAX );
				
				//e->MovRegFromMem32 ( RDX, RCX, RAX, SCALE_EIGHT, 0 );
				e->movdqa_from_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
				
#ifdef USE_NEW_VECTOR_DISPATCH_LQI_VU

				ret = Dispatch_Result_AVX2(e, v, i, false, RAX, i.Ft);

#else

				if ( i.xyzw != 0xf )
				{
					//e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm ( RAX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );

#endif
				
			}
			else
			{
				if ( i.is & 0xf )
				{
					e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
					e->IncReg32 ( RAX );
					if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
					{
						e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
					else
					{
						e->MovMemReg16 ( & v->vi [ i.is & 0xf ].sLo, RAX );
					}
				}
			}

			/*
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}







int32_t Recompiler::MFIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MFIR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MFIR;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MFIR
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			// destination for move instruction needs to be set only if move is made
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;


#ifdef USE_NEW_MFIR_RECOMPILE
		case 1:
			ret = Generate_VMFIRp ( e, v, i );

			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
					
					ret = Generate_VMFIRp ( e, v, i );
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif


			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MTIR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MTIR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MTIR;
	
	int ret = 1;


	v->IntDelayReg = i.it & 0xf;

	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MTIR
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			//Add_IDstReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// integer register destination
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MTIR_RECOMPILE
		case 1:
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/

			//ret = Generate_VMTIRp ( e, v, i );
			if ( ( i.it & 0xf ) )
			{
				//v->Set_IntDelaySlot ( i.it & 0xf, (u16) v->vf [ i.Fs ].vsw [ i.fsf ] );
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					if ( ( !i.Fs ) && ( i.fsf < 3 ) )
					{
						//e->MovMemImm32 ( ( & v->vf [ i.Ft ].sw0 ) + FtComponent, 0 );
						ret = e->MovMemImm32 ( (int32_t*) & v->IntDelayValue, 0 );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
					else
					{
						// flush ps2 float to zero
						e->MovRegMem32 ( RAX, & v->vf [ i.Fs ].vsw [ i.fsf ] );
						e->AndReg32ImmX ( RAX, 0xffff );
						
						// set result
						ret = e->MovMemReg32 ( (int32_t*) & v->IntDelayValue, RAX );
						e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
						e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
					}
				}
				else
				{
					// flush ps2 float to zero
					if ( ( !i.Fs ) && ( i.fsf < 3 ) )
					{
						//e->MovMemImm32 ( ( & v->vf [ i.Ft ].sw0 ) + FtComponent, 0 );
						ret = e->MovMemImm32 ( & v->vi [ i.it & 0xf ].s, 0 );
					}
					else
					{
						// flush ps2 float to zero
						e->MovRegMem32 ( RAX, & v->vf [ i.Fs ].vsw [ i.fsf ] );
						e->AndReg32ImmX ( RAX, 0xffff );
						
						// set result
						ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
					}
				}
			}


			

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MOVE ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MOVE";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MOVE;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MOVE
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			// destination for move instruction needs to be set only if move is made
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );

			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MOVE_RECOMPILE
		case 1:
			if ( i.Ft && i.xyzw )
			{
				e->movdqa_regmem ( RCX, & v->vf [ i.Fs ].sw0 );
				
				if ( i.xyzw != 0xf )
				{
					e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				// check if can set register directly or not
				//ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
				if (v->pStaticInfo[v->Number][(Address & v->ulVuMem_Mask) >> 3] & STATIC_COMPLETE_FLOAT_MOVE)
				{
					// set temp storage for move data
					ret = e->movdqa_memreg(&v->LoadMoveDelayReg.uw0, RCX);
				}
				else
				{
					// set result
					ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
				}
			}


			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				// todo: what if destination and source for move are the same ?? //
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
					
					ret = Generate_VMOVEp ( e, v, i );
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MR32 ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MR32";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MR32;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_MR32
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcReg ( i.Value, i.Fs );
			Add_FSrcReg ( v, ( ( i.Value << 1 ) & ( 0xe << 21 ) ) | ( ( i.Value >> 3 ) & ( 1 << 21 ) ), i.Fs );
			//Add_FSrcReg ( v, ( ( i.Value >> 1 ) & ( 0x7 << 21 ) ) | ( ( i.Value << 3 ) & ( 0x8 << 21 ) ), i.Fs );
			
			// destination for move instruction needs to be set only if move is made
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );

			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;


#ifdef USE_NEW_MR32_RECOMPILE
		case 1:
			if ( i.Ft && i.xyzw )
			{
				e->movdqa_regmem ( RCX, & v->vf [ i.Fs ].sw0 );

				e->pshufdregregimm ( RCX, RCX, ( 0 << 6 ) | ( 3 << 4 ) | ( 2 << 2 ) | ( 1 << 0 ) );

				if ( i.xyzw != 0xf )
				{
					e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				// check if can set register directly or not
				//ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
				if ( v->pStaticInfo[v->Number] [ ( Address & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 5 ) )
				{
					// set temp storage for move data
					ret = e->movdqa_memreg ( & v->LoadMoveDelayReg.uw0, RCX );
				}
				else
				{
					// set result
					ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
				}
			}


			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				// todo: what if destination and source for move are the same ?? //
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
					
					ret = Generate_VMR32p ( e, v, i );
				}
				else
				{
					return -1;
				}
			}
			*/

			break;
#endif

			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::RGET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "RGET";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::RGET;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_RGET
		// get source and destination register(s) bitmap
		case -1:
		
			// destination for move instruction needs to be set only if move is made
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_RGET_RECOMPILE
		case 1:
			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
			*/
					
			//ret = Generate_VMOVEp ( v, i );
			if ( i.Ft && i.xyzw )
			{
				//e->MovRegMem64 ( RAX, (int64_t*) & v->CycleCount );
				//e->CmpRegMem64 ( RAX, (int64_t*) & v->PBusyUntil_Cycle );
				
				
				// get new P register value if needed
				e->movd_regmem ( RCX, & v->vi [ VU::REG_R ].s );
				//e->MovRegMem32 ( RAX, ( &v->vi [ VU::REG_R ].s ) );
				//e->CmovAERegMem32 ( RAX, & v->NextP.l );
				//e->MovMemReg32 ( & v->vi [ VU::REG_P ].s, RAX );
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->movdqa_regmem ( RAX, & v->vf [ i.Ft ].sw0 );
				//}
				
				// sign-extend from 16-bit to 32-bit
				//e->Cwde();
				
				//e->movd_to_sse ( RCX, RAX );
				e->pshufdregregimm ( RCX, RCX, 0 );
				
				if ( i.xyzw != 0xf )
				{
					//e->pblendwregregimm ( RCX, RAX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				// set result
				//ret = e->MovMemReg32 ( ( &v->vf [ i.Ft ].sw0 ) + FtComponent, RAX );
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
			}

			/*
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::RINIT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "RINIT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::RINIT;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_RINIT
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_RINIT_RECOMPILE
		case 1:
			e->MovRegMem32 ( RAX, &v->vf [ i.Fs ].vsw [ i.fsf ] );
			//e->XorRegMem32 ( RAX, &v->vi [ VU::REG_R ].s );
			e->AndReg32ImmX ( RAX, 0x7fffff );
			e->OrReg32ImmX ( RAX, ( 0x7f << 23 ) );
			e->MovMemReg32 ( &v->vi [ VU::REG_R ].s, RAX );
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::RNEXT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "RNEXT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::RNEXT;
	
	static const uint32_t c_ulRandMask = 0x7ffb18;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_RNEXT
		// get source and destination register(s) bitmap
		case -1:
		
			// destination for move instruction needs to be set only if move is made
			//v->Set_DestReg_Upper ( i.Value, i.Ft );
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_RNEXT_RECOMPILE
		case 1:
			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
			*/
					
			//ret = Generate_VMOVEp ( v, i );
			if ( i.Ft && i.xyzw )
			{
				//e->MovRegMem64 ( RAX, (int64_t*) & v->CycleCount );
				//e->CmpRegMem64 ( RAX, (int64_t*) & v->PBusyUntil_Cycle );
				
				
				// get new P register value if needed
				//e->movd_regmem ( RCX, & v->vi [ VU::REG_R ].s );
				e->MovRegMem32 ( RAX, ( &v->vi [ VU::REG_R ].s ) );
				
				e->MovRegReg32 ( RCX, RAX );
				e->AndReg32ImmX( RAX, c_ulRandMask );
				//e->Set_PO ( RCX );
				e->PopCnt32 ( RAX, RAX );
				e->AndReg32ImmX ( RAX, 1 );
				e->AddRegReg32 ( RCX, RCX );
				e->OrRegReg32 ( RAX, RCX );
				
				e->AndReg32ImmX ( RAX, 0x7fffff );
				e->OrReg32ImmX ( RAX, ( 0x7f << 23 ) );
				e->MovMemReg32 ( &v->vi [ VU::REG_R ].s, RAX );
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->movdqa_regmem ( RAX, & v->vf [ i.Ft ].sw0 );
				//}
				
				// sign-extend from 16-bit to 32-bit
				//e->Cwde();
				
				e->movd_to_sse ( RCX, RAX );
				e->pshufdregregimm ( RCX, RCX, 0 );
				
				if ( i.xyzw != 0xf )
				{
					//e->pblendwregregimm ( RCX, RAX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				// set result
				//ret = e->MovMemReg32 ( ( &v->vf [ i.Ft ].sw0 ) + FtComponent, RAX );
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
			}

			/*
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::RXOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "RXOR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::RXOR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_RXOR
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_RXOR_RECOMPILE
		case 1:
			e->MovRegMem32 ( RAX, &v->vf [ i.Fs ].vsw [ i.fsf ] );
			e->XorRegMem32 ( RAX, &v->vi [ VU::REG_R ].s );
			e->AndReg32ImmX ( RAX, 0x7fffff );
			e->OrReg32ImmX ( RAX, ( 0x7f << 23 ) );
			e->MovMemReg32 ( &v->vi [ VU::REG_R ].s, RAX );
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::RSQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "RSQRT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::RSQRT;
	
	static const u64 c_CycleTime = 13;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_RSQRT
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegBC ( i.ftf, i.Ft );
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.ftf, i.Ft );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;


#ifdef USE_NEW_RSQRT_RECOMPILE
		case 1:

			// have to do a wait-q to update the q-reg and flags with the current values before setting the next values
			// otherwise the current values get overwritten before being set
			Perform_WaitQ(e, v, Address);

			/*
			// clear bits 14 and 15 in the flag register first
			//e->AndMem32ImmX ( &VU0::_VU0->vi [ VU::REG_STATUSFLAG ].u, ~0x00000030 );
			
			// flush ps2 float to zero
			e->MovRegMem32 ( RAX, &v->vf [ i.Ft ].vsw [ i.ftf ] );
			e->XorRegReg32 ( 11, 11 );
			e->MovReg64ImmX ( RCX, 896ULL << 23 );
			
			// get flags
			e->Cdq();
			e->AndReg32ImmX ( RDX, 0x00410 );
			
			
			e->AndReg32ImmX ( RAX, 0x7fffffff );
			//e->LeaRegRegReg64 ( 8, RAX, RCX );
			e->AddRegReg64 ( RCX, RAX );
			e->AndReg32ImmX ( RAX, 0x7f800000 );
			e->MovReg32ImmX ( 8, 0x00820 );
			e->CmovNERegReg32 ( 8, RDX );
			e->CmovNERegReg64 ( RAX, RCX );
			e->ShlRegImm64 ( RAX, 29 );
			
			
			// set flags
			//e->OrMemReg32 ( &VU0::_VU0->vi [ VU::REG_STATUSFLAG ].u, 8 );
			e->MovMemReg16 ( (short*) &v->NextQ_Flag, 8 );
			
			
			// move the registers now to floating point unit
			e->movq_to_sse ( RAX, RAX );
			//e->movq_to_sse ( RCX, RDX );
			
			
			// sqrt
			e->sqrtsd ( RAX, RAX );
			e->movq_from_sse ( RAX, RAX );
			
			// ??
			e->AddReg64ImmX ( RAX, 0x10000000 );
			e->AndReg64ImmX ( RAX, ~0x1fffffff );
			

			e->movq_to_sse ( RCX, RAX );


			e->MovRegMem32 ( RAX, &v->vf [ i.Fs ].vsw [ i.fsf ] );
			//e->MovRegReg32 ( RCX, RAX );
			e->Cdq ();
			e->AndReg32ImmX ( RAX, 0x7fffffff );
			//e->LeaRegRegReg64 ( RDX, RAX, RCX );
			e->TestReg32ImmX ( RAX, 0x7f800000 );
			e->CmovERegReg64 ( RAX, 11 );
			//e->ShrRegImm32 ( 10, 31 );
			//e->ShlRegImm64 ( 10, 63 );
			e->ShlRegImm64 ( RAX, 29 );
			//e->OrRegReg64 ( RAX, 10 );
			e->movq_to_sse ( RAX, RAX );

			
			// divide
			e->divsd ( RAX, RCX );
			
			
			// get result
			e->movq_from_sse ( RAX, RAX );
			
			
			// shift back down without sign
			e->ShrRegImm64 ( RAX, 29 );
			
			// subtract exponent
			//e->XorRegReg32 ( 10, 10 );
			//e->MovRegReg32 ( RDX, RAX );
			//e->AndReg64ImmX ( RAX, ~0x007fffff );
			//e->SubRegReg64 ( RAX, RCX );
			e->TestReg32ImmX ( RAX, 0xff800000 );
			
			// clear on underflow or zero
			//e->CmovLERegReg32 ( RAX, 10 );
			//e->CmovLERegReg32 ( RDX, 10 );
			e->CmovERegReg32 ( RAX, 11 );
			
			
			// set to max on overflow
			e->MovReg32ImmX ( RCX, 0x7fffffff );
			//e->OrRegReg32 ( RDX, RDX );
			e->CmovSRegReg32 ( RAX, RCX );
			
			
			// or if any flags are set indicating denominator is zero
			e->AndReg32ImmX ( 8, 0x00020 );
			e->CmovNERegReg32 ( RAX, RCX );

			
			// set sign
			e->AndReg32ImmX ( RDX, 0x80000000 );
			e->OrRegReg32 ( RAX, RDX );
			

			// store result
			//e->MovMemReg32 ( &VU0::_VU0->vi [ VU::REG_Q ].u, RAX );		// &r->CPR1 [ i.Fd ].u, RAX );
			e->MovMemReg32 ( &v->NextQ.l, RAX );
			*/

			e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Fs].vsw[i.fsf]);
			e->movd1_regmem(RCX, (int32_t*)&v->vf[i.Ft].vsw[i.ftf]);

			// make constant multiplier
			e->MovReg64ImmX(RAX, (1023ull + 896ull) << 52);
			e->movq_to_sse128(RBX, RAX);

			// check if ft(rcx) is zero (D flag) (ft=0) ->r5
			e->pxor1regreg(RDX, RDX, RDX);
			e->paddd1regreg(5, RCX, RCX);
			e->pcmpeqb1regreg(5, 5, RDX);

			// check if fs(rax) is zero -> r4
			e->paddd1regreg(4, RAX, RAX);
			e->pcmpeqb1regreg(4, 4, RDX);

			// check if both fs==0 and ft==0 ->r4
			e->pand1regreg(4, 4, 5);

			// get full flags
			e->pcmpgtd1regreg(4, RDX, 4);
			e->pcmpgtd1regreg(5, RDX, 5);

			// if fs and ft are both float zero, adjust fs
			e->psrld1regimm(4, 4, 1);
			e->por1regreg(RAX, RAX, 4);


			// get I-flag (ft(rcx)<0) ->r4
			e->pcmpgtd1regreg(4, RDX, RCX);
			e->pandn1regreg(4, 5, 4);

			// get flags for later
			e->movd_from_sse128(RCX, 4);
			e->movd_from_sse128(RDX, 5);

			// make constant adjustment
			e->MovReg64ImmX(RAX, 3ull << 25);
			e->movq_to_sse128(RDX, RAX);

			// cvt float to double
			e->psllq1regimm(RCX, RCX, 33);
			e->psrlq1regimm(RCX, RCX, 4);


			// multiply with multiplier
			e->vmulsd(RCX, RCX, RBX);


			// sqrt
			e->vsqrtsd(RCX, RCX);

			// adjust result
			e->paddq1regreg(RCX, RCX, RDX);

			// get result
			e->cvtsd2ss1regreg(RCX, RCX);

			// save sign ->rdx
			e->psrad1regimm(RDX, RAX, 31);

			// make constant multiplier
			//e->MovReg64ImmX(RAX, 127ull << 52);
			e->MovReg64ImmX(RAX, 896ull << 23);
			e->movq_to_sse128(4, RAX);

			// convert sqrt back to double
			e->psllq1regimm(RAX, RAX, 33);
			e->psrlq1regimm(RAX, RAX, 4);
			e->psllq1regimm(RCX, RCX, 33);
			e->psrlq1regimm(RCX, RCX, 4);


			// divide
			e->vdivsd(RAX, RAX, RCX);


			// multiply with multiplier
			//e->vmulsd(RAX, RAX, 4);


			// adjust result
			//e->paddq1regreg(RAX, RAX, 4);

			// get result
			e->psrlq1regimm(RAX, RAX, 29);

			// offset exponent
			e->psubq1regreg(RAX, RAX, 4);

			// clear on underflow or zero
			e->pxor1regreg(5, 5, 5);
			e->pcmpgtq1regreg(4, 5, RAX);
			e->pandn1regreg(RAX, 4, RAX);

			// maximize on overflow
			e->pcmpeqb1regreg(5, 5, 5);
			e->psrlq1regimm(4, 5, 33);
			e->pcmpgtq1regreg(RBX, RAX, 4);
			e->por1regreg(RAX, RAX, RBX);
			e->pand1regreg(RAX, RAX, 4);


			// put sign in
			e->pandn1regreg(RDX, 4, RDX);
			e->por1regreg(RAX, RAX, RDX);

			// store result
			e->movd1_memreg((int32_t*)&v->NextQ.l, RAX);


			// get flags
			//e->MovRegMem32(RAX, (int32_t*)&v->NextQ_Flag);
			e->AndReg32ImmX(RCX, 0x410);
			e->AndReg32ImmX(RDX, 0x820);
			e->OrRegReg32(RCX, RDX);
			//e->AndReg32ImmX(RAX, ~0x30000);
			//e->OrRegReg32(RAX, RCX);
			e->MovMemReg32((int32_t*)&v->NextQ_Flag, RCX);

			
			// set time to process
			e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef USE_NEW_RECOMPILE2_RSQRT
			e->AddReg64ImmX ( RAX, c_CycleTime + ( ( Address & v->ulVuMem_Mask ) >> 3 ) );
#else
			e->AddReg64ImmX ( RAX, c_CycleTime );
#endif
			
			e->MovMemReg64 ( (int64_t*) & v->QBusyUntil_Cycle, RAX );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SQRT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SQRT;
	
	static const u64 c_CycleTime = 7;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SQRT
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegBC ( i.ftf, i.Ft );
			Add_FSrcRegBC ( v, i.ftf, i.Ft );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;


#ifdef USE_NEW_SQRT_RECOMPILE
		case 1:

			// have to do a wait-q to update the q-reg and flags with the current values before setting the next values
			// otherwise the current values get overwritten before being set
			Perform_WaitQ(e, v, Address);

			/*
			// clear bits 14 and 15 in the flag register first
			//e->AndMem32ImmX ( &VU0::_VU0->vi [ VU::REG_STATUSFLAG ].u, ~0x00000030 );
			
			// flush ps2 float to zero
			e->MovRegMem32 ( RAX, &v->vf [ i.Ft ].vsw [ i.ftf ] );
			e->MovReg64ImmX ( RCX, 896ULL << 23 );
			
			// get flags
			e->Cdq();
			e->AndReg32ImmX ( RDX, 0x00410 );
			
			e->AndReg32ImmX ( RAX, 0x7fffffff );
			e->LeaRegRegReg64 ( 8, RAX, RCX );
			e->AndReg32ImmX ( RAX, 0x7f800000 );
			e->CmovERegReg32 ( RDX, RAX );
			e->CmovNERegReg64 ( RAX, 8 );
			e->ShlRegImm64 ( RAX, 29 );
			
			// set flags
			//e->OrMemReg32 ( &VU0::_VU0->vi [ VU::REG_STATUSFLAG ].u, RDX );
			e->MovMemReg16 ( (short*) &v->NextQ_Flag, RDX );
			
			// move the registers now to floating point unit
			e->movq_to_sse ( RAX, RAX );
			//e->movq_to_sse ( RCX, RDX );
			
			// sqrt
			e->sqrtsd ( RAX, RAX );
			e->movq_from_sse ( RAX, RAX );
			
			// ??
			e->AddReg64ImmX ( RAX, 0x10000000 );
			
			// shift back down without sign
			e->ShrRegImm64 ( RAX, 29 );
			
			// if zero, then clear RCX
			e->CmovERegReg64 ( RCX, RAX );
			
			// subtract exponent
			e->SubRegReg64 ( RAX, RCX );
			
			// set result
			//ret = e->MovMemReg32 ( &VU0::_VU0->vi [ VU::REG_Q ].u, RAX );
			e->MovMemReg32 ( &v->NextQ.l, RAX );
			*/
			
			e->movd1_regmem(RAX, (int32_t*)&v->vf[i.Ft].vsw[i.ftf]);
			//e->movd1_regmem(RCX, (int32_t*)&vt.uw0);

			// make constant multiplier
			e->MovReg64ImmX(RAX, (1023ull + 896ull) << 52);
			e->movq_to_sse128(RBX, RAX);

			// make adjustment
			e->MovReg64ImmX(RAX, 3ull << 25);
			e->movq_to_sse128(RDX, RAX);

			// get initial I-flag ->r5
			e->psrad1regimm(5, RAX, 31);

			// cvt float to double
			e->psllq1regimm(RAX, RAX, 33);
			e->psrlq1regimm(RAX, RAX, 4);
			//e->psllq1regimm(RCX, RCX, 33);
			//e->psrlq1regimm(RCX, RCX, 4);

			// multiply with multiplier
			e->vmulsd(RAX, RAX, RBX);

			// check for zero ->r4
			e->pxor1regreg(4, 4, 4);
			e->pcmpeqq1regreg(4, 4, RAX);

			// only set I-flag if negative number not zero ?
			e->pandn1regreg(5, 4, 5);

			// get final I-flag ->rcx
			e->movd_from_sse128(RCX, 5);

			// sqrt
			e->vsqrtsd(RAX, RAX);

			// adjust result
			e->paddq1regreg(RAX, RAX, RDX);

			// get result
			e->cvtsd2ss1regreg(RAX, RAX);

			// store result
			e->movd1_memreg((int32_t*)&v->NextQ.l, RAX);

			// get flags
			//e->MovRegMem32(RAX, (int32_t*)&statflag);
			e->AndReg32ImmX(RCX, 0x410);
			//e->AndReg32ImmX(RAX, ~0x30000);
			//e->OrRegReg32(RAX, RCX);
			e->MovMemReg32((int32_t*)&v->NextQ_Flag, RCX);


			// set time to process
			e->MovRegMem64(RAX, (int64_t*)&v->CycleCount);

#ifdef USE_NEW_RECOMPILE2_SQRT
			e->AddReg64ImmX ( RAX, c_CycleTime + ( ( Address & v->ulVuMem_Mask ) >> 3 ) );
#else
			e->AddReg64ImmX ( RAX, c_CycleTime );
#endif
			
			e->MovMemReg64 ( (int64_t*) & v->QBusyUntil_Cycle, RAX );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}





int32_t Recompiler::SQD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SQD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SQD;
	
	int ret = 1;
	
	v->IntDelayReg = i.it & 0xf;
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SQD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// store post/pre inc/dec currently implemented with delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SQD_RECOMPILE
		case 1:
		
#ifdef DISABLE_SQD_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif

			/*		
#ifdef ENABLE_INTDELAYSLOT_SQD
			// check for a conditional branch that might be affected by integer destination register
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
#endif

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
			e->MovRegMem32 ( RAX, & v->vi [ i.it & 0xf ].s );
			
			
			e->movdqa_regmem ( RAX, & v->vf [ i.Fs ].sw0 );
			
			//pVuMem32 = v->GetMemPtr ( LoadAddress );
			//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
			//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
			e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );

			// special code for VU#0
			if ( !v->Number )
			{
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
				e->AndReg32ImmX ( RAX, 0x7fff );
#endif

				// check if Address & 0xf000 == 0x4000
				e->MovRegReg32 ( RDX, RAX );
				e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
				e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
				e->Jmp8_NE ( 0, 1 );
				
				// here it will be loading/storing from/to the registers for VU#1
				//e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				//e->CmovERegReg64 ( RCX, RDX );
				e->LeaRegMem64 ( RCX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				
				// ***TODO*** check if storing to TPC
				e->CmpReg32ImmX ( RAX, 0x43a );
				e->Jmp8_NE ( 0, 0 );
				
				//VU1::_VU1->Running = 1;
				e->MovMemImm32 ( (int32_t*) & VU1::_VU1->Running, 1 );
				
				//VU1::_VU1->CycleCount = *VU::_DebugCycleCount + 1;
				e->MovRegMem64 ( RDX, (int64_t*) VU::_DebugCycleCount );
				
				// set VBSx in VPU STAT to 1 (running)
				//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( 1 << 3 ) );
				e->OrMemImm32 ( & VU0::_VU0->vi [ 29 ].s, ( 1 << ( 1 << 3 ) ) );
				
				// also set VIFx STAT to indicate program is running
				//VU1::_VU1->VifRegs.STAT.VEW = 1;
				e->OrMemImm32 ( (int32_t*) & VU1::_VU1->VifRegs.STAT.Value, ( 1 << 2 ) );
				
				// finish handling the new cycle count
				e->IncReg64 ( RDX );
				e->MovMemReg64 ( (int64_t*) & VU1::_VU1->CycleCount, RDX );
				
				e->SetJmpTarget8 ( 0 );
				
				// mask address for accessing registers
				e->AndReg32ImmX ( RAX, 0x3f );
				
				e->SetJmpTarget8 ( 1 );
			}
			
			
			if ( i.it & 0xf )
			{
				// pre-dec
				e->DecReg32 ( RAX );

				// analysis bit 10 - output int to int delay slot - next conditional branch uses the register
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RAX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					e->MovMemReg16 ( & v->vi [ i.it & 0xf ].sLo, RAX );
				}
			}
			
			if ( !v->Number )
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
			}
			else
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
			}
			e->AddRegReg32 ( RAX, RAX );
			
			
#define ENABLE_VU_MASK_STORE_SQD
#ifdef ENABLE_VU_MASK_STORE_SQD

			if (i.xyzw != 0xf)
			{
				//e->movdqa_from_mem128(RCX, RCX, RAX, SCALE_EIGHT, 0);
				//e->pblendwregregimm(RAX, RCX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pcmpeqb1regreg(RCX, RCX, RCX);
				e->pxor1regreg(RDX, RDX, RDX);
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->vmaskmovd1_memreg(RCX, RAX, RCX, RAX, SCALE_EIGHT, 0);
			}
			else
			{
				ret = e->movdqa_to_mem128(RAX, RCX, RAX, SCALE_EIGHT, 0);
			}

#else
			if ( i.xyzw != 0xf )
			{
				e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
				e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			ret = e->movdqa_to_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
#endif
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}


//static void test_value ( u64 value )
//{
//	cout << "\nvalue(dec)=" << dec << value << " value(hex)=" << hex << value;
//}

int32_t Recompiler::SQI ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SQI";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SQI;
	
	int ret = 1;
	
	v->IntDelayReg = i.it & 0xf;
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SQI
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// store post/pre inc/dec currently implemented with delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SQI_RECOMPILE
		case 1:

			/*		
#ifdef DISABLE_SQI_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
#ifdef ENABLE_INTDELAYSLOT_SQI
			// check for a conditional branch that might be affected by integer destination register
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}
#endif
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
			e->MovRegMem32 ( RAX, & v->vi [ i.it & 0xf ].s );
			
			e->movdqa_regmem ( RAX, & v->vf [ i.Fs ].sw0 );

			//pVuMem32 = v->GetMemPtr ( LoadAddress );
			//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
			//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
			e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
			
			// special code for VU#0
			if ( !v->Number )
			{
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
				e->AndReg32ImmX ( RAX, 0x7fff );
#endif

				// check if Address & 0xf000 == 0x4000
				e->MovRegReg32 ( RDX, RAX );
				e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
				e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
				e->Jmp8_NE ( 0, 1 );
				
				// here it will be loading/storing from/to the registers for VU#1
				//e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				//e->CmovERegReg64 ( RCX, RDX );
				e->LeaRegMem64 ( RCX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				
				// ***TODO*** check if storing to TPC
				e->CmpReg32ImmX ( RAX, 0x43a );
				e->Jmp8_NE ( 0, 0 );
				
				//VU1::_VU1->Running = 1;
				e->MovMemImm32 ( (int32_t*) & VU1::_VU1->Running, 1 );
				
				//VU1::_VU1->CycleCount = *VU::_DebugCycleCount + 1;
				e->MovRegMem64 ( RDX, (int64_t*) VU::_DebugCycleCount );
				
				// set VBSx in VPU STAT to 1 (running)
				//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( 1 << 3 ) );
				e->OrMemImm32 ( & VU0::_VU0->vi [ 29 ].s, ( 1 << ( 1 << 3 ) ) );
				
				// also set VIFx STAT to indicate program is running
				//VU1::_VU1->VifRegs.STAT.VEW = 1;
				e->OrMemImm32 ( (int32_t*) & VU1::_VU1->VifRegs.STAT.Value, ( 1 << 2 ) );
				
				// finish handling the new cycle count
				e->IncReg64 ( RDX );
				e->MovMemReg64 ( (int64_t*) & VU1::_VU1->CycleCount, RDX );
				
				e->SetJmpTarget8 ( 0 );
				
				// mask address for accessing registers
				e->AndReg32ImmX ( RAX, 0x3f );
				
				e->SetJmpTarget8 ( 1 );
			}
			
			
			// post-inc
			if ( i.it & 0xf )
			{
				e->LeaRegRegImm32 ( RDX, RAX, 1 );

				// analysis bit 10 - output int to int delay slot - next conditional branch uses the register
				if ( v->pLUT_StaticInfo [ ( Address & v->ulVuMem_Mask ) >> 3 ] & STATIC_MOVE_TO_INT_DELAY_SLOT)
				{
					e->MovMemReg16 ( (s16*) &v->IntDelayValue, RDX );
					e->MovMemImm32((int32_t*)&v->IntDelayReg, v->IntDelayReg);
					e->MovMemImm8((char*)&v->Status.IntDelayValid, 2);
				}
				else
				{
					e->MovMemReg16 ( & v->vi [ i.it & 0xf ].sLo, RDX );
				}
			}
			
			if ( !v->Number )
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
			}
			else
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
			}
			e->AddRegReg32 ( RAX, RAX );
			
			
#define ENABLE_VU_MASK_STORE_SQI
#ifdef ENABLE_VU_MASK_STORE_SQI

			if (i.xyzw != 0xf)
			{
				//e->movdqa_from_mem128(RCX, RCX, RAX, SCALE_EIGHT, 0);
				//e->pblendwregregimm(RAX, RCX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pcmpeqb1regreg(RCX, RCX, RCX);
				e->pxor1regreg(RDX, RDX, RDX);
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->vmaskmovd1_memreg(RCX, RAX, RCX, RAX, SCALE_EIGHT, 0);
			}
			else
			{
				ret = e->movdqa_to_mem128(RAX, RCX, RAX, SCALE_EIGHT, 0);
			}

#else
			if ( i.xyzw != 0xf )
			{
				e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
				e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			ret = e->movdqa_to_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
#endif
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::WAITQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "WAITQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::WAITQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_WAITQ_RECOMPILE
		case 1:

#ifdef USE_NEW_RECOMPILE2_WAITQ
			// drop in the waitq code
			Perform_WaitQ ( e, v, Address );
#else

			// check if QBusyUntil_Cycle is -1
			e->MovRegMem64 ( RAX, (int64_t*) & v->QBusyUntil_Cycle );
			e->CmpReg64ImmX ( RAX, -1 );
			e->Jmp8_E ( 0, 0 );
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			e->LeaRegMem64 ( RCX, v );
			ret = e->Call ( (void*) PipelineWaitQ );
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			e->SetJmpTarget8 ( 0 );
#endif

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::B ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "B";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::B;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_B_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			// check if next instruction is xgkick
			//if (isXgKick(v->NextInst))
			//{
			//	return -1;
			//}


			// do the same as the interpreter here
			// note: branch in branch delay slots get interpreted so is not an issue
			//e->MovMemImm32((int32_t*)&v->NextDelaySlotIndex, 0);
			//e->MovMemImm64((int64_t*)&v->DelaySlots[0], i.Value);
			//e->MovMemImm8((char*)&v->Status.DelaySlot_Valid, 2);


			e->MovMemImm32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, 1 );

			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 0;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::BAL ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "BAL";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::BAL;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);
			
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_BAL_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			e->MovMemImm32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, 1 );
			
			// v->vi [ i.it ].uLo = ( v->PC + 16 ) >> 3;
			e->MovMemImm32 ( & v->vi [ i.it & 0xf ].s, ( Address + 16 ) >> 3 );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 0;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::JALR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "JALR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::JALR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_JALR_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}


			// d->Data = v->vi [ i.is ].uLo;
			// *note* must do this first incase is and it registers are the same.. then it overwrites!
			e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
			
			e->MovMemImm32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, 1 );
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_BranchDelayAddress, RAX );
			
			
			// v->vi [ i.it ].uLo = ( v->PC + 16 ) >> 3;
			e->MovMemImm32 ( & v->vi [ i.it & 0xf ].s, ( Address + 16 ) >> 3 );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 0;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::JR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "JR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::JR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_JR_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}


			// d->Data = v->vi [ i.is ].uLo;
			e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );

			e->MovMemImm32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, 1 );
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_BranchDelayAddress, RAX );
			
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 0;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}




int32_t Recompiler::IBEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBEQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBEQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBEQ_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.it & 0xf ].sLo );
			e->MovRegMem16 ( RCX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegReg16 ( RAX, RCX );
			e->Set_E ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IBGEZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBGEZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBGEZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBGEZ_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegImm16 ( RAX, 0 );
			e->Set_GE ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IBGTZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBGTZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBGTZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBGTZ_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegImm16 ( RAX, 0 );
			e->Set_G ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IBLEZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBLEZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBLEZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBLEZ_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}
			
			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegImm16 ( RAX, 0 );
			e->Set_LE ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IBLTZ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBLTZ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBLTZ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBLTZ_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}

			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegImm16 ( RAX, 0 );
			e->Set_L ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::IBNE ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "IBNE";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::IBNE;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// branches/jumps need updated PC at level 0
			//v->bStopEncodingBefore = true;
			e->MovMemImm32((int32_t*)&v->PC, Address);

			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_IBNE_RECOMPILE
		case 1:
			// ***TODO*** check if next instruction is branch
			
			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			//if ( v->Status_BranchDelay )
			//if ( v->NextInstLO.Value & 0x40000000 )
			//{
			//	//cout << "\nhps2x64: ALERT: VU: BRANCH IN BRANCH DELAY SLOT";
			//	return -1;
			//}
			
			if (isBranchOrJump(v->NextInst))
			{
				return -1;
			}

			if (isBranchOrJump(v->PrevInst))
			{
				return -1;
			}

			// check if encoding stops after this instruction (since the static delay slot is after it)
			if ( v->bStopEncodingAfter )
			{
				return -1;
			}

			// check if e-bit delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_EBIT_DELAY_SLOT)
			{
				return -1;
			}

			// check if xgkick delay slot
			if (v->pLUT_StaticInfo[(Address & v->ulVuMem_Mask) >> 3] & STATIC_XGKICK_DELAY_SLOT)
			{
				return -1;
			}

			
			//if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
			e->MovRegMem16 ( RAX, & v->vi [ i.it & 0xf ].sLo );
			e->MovRegMem16 ( RCX, & v->vi [ i.is & 0xf ].sLo );
			e->XorRegReg32 ( RDX, RDX );
			e->CmpRegReg16 ( RAX, RCX );
			e->Set_NE ( RDX );
			
			e->MovMemReg32 ( (int32_t*) & v->Recompiler_EnableBranchDelay, RDX );
			
			// check for branch delay
			v->Status_BranchDelay = 2;
			
			// not a conditional branch
			v->Status_BranchConditional = 1;
			
			// need to know what type of branch
			v->Status_BranchInstruction = i;
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FCAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FCAND";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FCAND;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FCAND_RECOMPILE
		case 1:

			// get the current clip flag from snapshot history
			Perform_GetClipFlag(e, v, Address);

			e->XorRegReg32 ( RCX, RCX );
			e->AndReg32ImmX ( RAX, i.Imm24 );
			e->Set_NE ( RCX );
			
			// store to 16-bits or 32-bits ??
			e->MovMemReg32 ( & v->vi [ 1 ].s, RCX );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FCEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FCEQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FCEQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_FCEQ_RECOMPILE
		case 1:
			// get the current clip flag from snapshot history
			Perform_GetClipFlag(e, v, Address);

			e->XorRegReg32 ( RCX, RCX );
			e->XorReg32ImmX ( RAX, i.Imm24 );
			e->AndReg32ImmX ( RAX, 0x00ffffff );
			e->Set_E ( RCX );
			
			// store to 16-bits or 32-bits ??
			e->MovMemReg32 ( & v->vi [ 1 ].s, RCX );
			
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FCGET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FCGET";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FCGET;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_FCGET_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// get the current clip flag from snapshot history
				Perform_GetClipFlag(e, v, Address);

				e->AndReg32ImmX ( RAX, 0xfff );
				e->MovMemReg16 ( & v->vi [ i.it & 0xf ].sLo, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FCOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FCOR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FCOR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FCOR_RECOMPILE
		case 1:

			// get the current clip flag from snapshot history
			Perform_GetClipFlag(e, v, Address);

			e->XorRegReg32 ( RCX, RCX );
			e->OrReg32ImmX ( RAX, i.Imm24 );
			e->AndReg32ImmX ( RAX, 0x00ffffff );
			e->CmpReg32ImmX ( RAX, 0xffffff );
			e->Set_E ( RCX );
			
			// store to 16-bits or 32-bits ??
			e->MovMemReg32 ( & v->vi [ 1 ].s, RCX );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FCSET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FCSET";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FCSET;
	
	int ret = 1;
	
	// sets clip flag in lower instruction
	v->SetClip_Flag = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FCSET_RECOMPILE
		case 1:
			ret = e->MovMemImm32 ( &v->vi [ VU::REG_CLIPFLAG ].s, i.Imm24 );
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}




int32_t Recompiler::FMAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FMAND";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FMAND;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FMAND_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				if ( ! ( i.is & 0xf ) )
				{
					e->MovMemImm32 ( & v->vi [ i.it & 0xf ].s, 0 );
				}
				else
				{
					// get the current mac flag from snapshot history
					Perform_GetMacFlag(e, v, Address);
					
					e->MovRegMem32 ( RCX, & v->vi [ i.is & 0xf ].s );
					e->AndRegReg32 ( RAX, RCX );
					
					// store to 16-bits or 32-bits ??
					ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
				}
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )

	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FMEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FMEQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FMEQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FMEQ_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// get the current mac flag from snapshot history
				Perform_GetMacFlag(e, v, Address);

				if ( ! ( i.is & 0xf ) )
				{
					e->XorRegReg32 ( RCX, RCX );
				}
				else
				{
					e->MovRegMem16 ( RCX, & v->vi [ i.is & 0xf ].sLo );
				}
				
				e->XorRegReg32 ( RDX, RDX );
				e->CmpRegReg16 ( RAX, RCX );
				e->Set_E ( RDX );
				
				// store to 16-bits or 32-bits ??
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RDX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FMOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FMOR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FMOR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FMOR_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// get the current mac flag from snapshot history
				Perform_GetMacFlag(e, v, Address);
				
				if ( ( i.is & 0xf ) )
				{
					e->MovRegMem16 ( RCX, & v->vi [ i.is & 0xf ].sLo );
					e->OrRegReg16 ( RAX, RCX );
				}
				
				// store to 16-bits or 32-bits ??
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::FSAND ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FSAND";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FSAND;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FSAND_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// put the q-flags into the stat flag before reading
				Perform_UpdateQ(e, v, Address);

				// puts current stat flag in RAX from snapshot history
				Perform_GetStatFlag(e, v, Address);

				// put q-flags into current stat flag
				e->MovRegMem32(RCX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].u);
				e->AndReg32ImmX(RAX, ~0xc30);
				e->AndReg32ImmX(RCX, 0xc30);
				e->OrRegReg32(RAX, RCX);

				e->AndReg32ImmX(RAX, (((i.Imm15_1 << 11) | (i.Imm15_0)) & 0xfff));
				
				// store to 16-bits or 32-bits ??
				ret = e->MovMemReg32(&v->vi[i.it & 0xf].s, RAX);
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FSEQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FSEQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FSEQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FSEQ_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// put the q-flags into the stat flag before reading
				Perform_UpdateQ(e, v, Address);

				// puts current stat flag in RAX from snapshot history
				Perform_GetStatFlag(e, v, Address);

				// put q-flags into current stat flag
				e->MovRegMem32(RCX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].u);
				e->AndReg32ImmX(RAX, ~0xc30);
				e->AndReg32ImmX(RCX, 0xc30);
				e->OrRegReg32(RAX, RCX);

				e->XorRegReg32 ( RCX, RCX );
				//e->CmpRegImm16 ( RAX, ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff ) );
				e->XorReg16ImmX ( RAX, ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff ) );
				e->AndReg16ImmX ( RAX, 0xfff );
				e->Set_E ( RCX );
				
				// store to 16-bits or 32-bits ??
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RCX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FSOR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FSOR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FSOR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FSOR_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				// put the q-flags into the stat flag before reading
				Perform_UpdateQ(e, v, Address);

				// puts current stat flag in RAX from snapshot history
				Perform_GetStatFlag(e, v, Address);

				// put q-flags into current stat flag
				e->MovRegMem32(RCX, (int32_t*)&v->vi[VU::REG_STATUSFLAG].u);
				e->AndReg32ImmX(RAX, ~0xc30);
				e->AndReg32ImmX(RCX, 0xc30);
				e->OrRegReg32(RAX, RCX);


				if ( ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff ) )
				{
					e->OrReg32ImmX ( RAX, ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff ) );
				}
				
				e->AndReg32ImmX ( RAX, 0x00000fff );
				
				// store to 16-bits or 32-bits ??
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::FSSET ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "FSSET";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::FSSET;
	
	int ret = 1;
	
	// sets stat flag in lower instruction
	v->SetStatus_Flag = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_FSSET_RECOMPILE
		case 1:

			// put the q-flags into the stat flag before reading
			// have to update q flags before setting stat because this clears NextQ sticky flags
			Perform_UpdateQ(e, v, Address);

			e->MovRegMem32 ( RAX, & v->vi [ VU::REG_STATUSFLAG ].s );
			e->AndReg32ImmX ( RAX, 0x3f );
			e->OrReg32ImmX ( RAX, ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfc0 ) );
			ret = e->MovMemReg32 ( & v->vi [ VU::REG_STATUSFLAG ].s, RAX );

			break;
#endif

			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}









int32_t Recompiler::ILW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ILW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ILW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ILW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			Add_IDstReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// load at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ILW_RECOMPILE
		case 1:
		
#ifdef DISABLE_ILW_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
			// check for a conditional branch that might be affected by integer destination register
			/*
			if ( ( v->NextInstLO.Opcode & 0x28 ) == 0x28 )
			{
				if ( ( ( i.it & 0xf ) == ( v->NextInstLO.it & 0xf ) ) || ( ( i.it & 0xf ) == ( v->NextInstLO.is & 0xf ) ) )
				{
					return -1;
				}
			}

			// make sure we are not in a known branch delay slot
			// E-Bit delay slot wouldn't matter, just branch delay
			if ( v->Status_BranchDelay )
			{
				return -1;
			}
			*/
			
			if ( i.it )
			{
				//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
				e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
				
				
				//pVuMem32 = v->GetMemPtr ( LoadAddress );
				//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
				//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
				e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
				
				e->AddReg32ImmX ( RAX, (s32) i.Imm11 );
				
				
				// special code for VU#0
				if ( !v->Number )
				{
					// check if Address & 0xf000 == 0x4000
					e->MovRegReg32 ( RDX, RAX );
					e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
					e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
					
					// here it will be loading/storing from/to the registers for VU#1
					e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
					e->CmovERegReg64 ( RCX, RDX );
					
					// ***TODO*** check if storing to TPC
				}
				
				
				if ( !v->Number )
				{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
				}
				else
				{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
				}
				e->AddRegReg32 ( RAX, RAX );
				
				switch( i.xyzw )
				{
					case 8:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
						break;
						
					case 4:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 4 );
						break;
						
					case 2:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 8 );
						break;
						
					case 1:
						e->MovRegFromMem32 ( RAX, RCX, RAX, SCALE_EIGHT, 12 );
						break;
						
					default:
						cout << "\nVU: Recompiler: ALERT: ILWR with illegal xyzw=" << hex << i.xyzw << "\n";
						break;
				}
				
				
				ret = e->MovMemReg32 ( & v->vi [ i.it & 0xf ].s, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::ISW ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ISW";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ISW;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ISW
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_ISW_RECOMPILE
		case 1:
		
#ifdef DISABLE_ISW_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
				
			//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
			e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
			e->movd_regmem ( RAX, & v->vi [ i.it & 0xf ].s );
			
			//e->movdqa_regmem ( RAX, & v->vf [ i.Fs ].sw0 );
			
			//pVuMem32 = v->GetMemPtr ( LoadAddress );
			//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
			//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
			e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
			
			e->AddReg32ImmX ( RAX, (s32) i.Imm11 );
			
			// special code for VU#0
			if ( !v->Number )
			{
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
				e->AndReg32ImmX ( RAX, 0x7fff );
#endif

				// check if Address & 0xf000 == 0x4000
				e->MovRegReg32 ( RDX, RAX );
				e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
				e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
				e->Jmp8_NE ( 0, 1 );
				
				// here it will be loading/storing from/to the registers for VU#1
				//e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				//e->CmovERegReg64 ( RCX, RDX );
				e->LeaRegMem64 ( RCX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				
				// ***TODO*** check if storing to TPC
				e->CmpReg32ImmX ( RAX, 0x43a );
				e->Jmp8_NE ( 0, 0 );
				
				//VU1::_VU1->Running = 1;
				e->MovMemImm32 ( (int32_t*) & VU1::_VU1->Running, 1 );
				
				//VU1::_VU1->CycleCount = *VU::_DebugCycleCount + 1;
				e->MovRegMem64 ( RDX, (int64_t*) VU::_DebugCycleCount );
				
				// set VBSx in VPU STAT to 1 (running)
				//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( 1 << 3 ) );
				e->OrMemImm32 ( & VU0::_VU0->vi [ 29 ].s, ( 1 << ( 1 << 3 ) ) );
				
				// also set VIFx STAT to indicate program is running
				//VU1::_VU1->VifRegs.STAT.VEW = 1;
				e->OrMemImm32 ( (int32_t*) & VU1::_VU1->VifRegs.STAT.Value, ( 1 << 2 ) );
				
				// finish handling the new cycle count
				e->IncReg64 ( RDX );
				e->MovMemReg64 ( (int64_t*) & VU1::_VU1->CycleCount, RDX );
				
				e->SetJmpTarget8 ( 0 );
				
				// mask address for accessing registers
				e->AndReg32ImmX ( RAX, 0x3f );
				
				e->SetJmpTarget8 ( 1 );
			}
			
			if ( !v->Number )
			{
			e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
			}
			else
			{
			e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
			}
			e->AddRegReg32 ( RAX, RAX );

			if ( i.xyzw != 0xf )
			{
				e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
			}
			
			e->pmovzxwdregreg ( RAX, RAX );
			e->pshufdregregimm ( RAX, RAX, 0 );
			//e->pslldregimm ( RAX, 16 );
			//e->psrldregimm ( RAX, 16 );
			
			if ( i.xyzw != 0xf )
			{
				//e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
				e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			//ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );
			ret = e->movdqa_to_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::LQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "LQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::LQ;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_LQ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			// destination for move instruction needs to be set only if move is made
			Add_FDstReg ( v, i.Value, i.Ft );
			
			break;
#endif

		case 0:
			// load at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_LQ_RECOMPILE
		case 1:
		
#ifdef DISABLE_LQ_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
		
			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
			*/

			if ( i.Ft )
			{
				// add destination register to bitmap at end
				//Add_FDstReg ( v, i.Value, i.Ft );
				
				//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
				e->MovRegMem32 ( RAX, & v->vi [ i.is & 0xf ].s );
				
				//if ( i.xyzw != 0xf )
				//{
				//	e->movdqa_regmem ( RCX, & v->vf [ i.Ft ].sw0 );
				//}

				e->AddReg32ImmX ( RAX, (s32) i.Imm11 );
				
				//pVuMem32 = v->GetMemPtr ( LoadAddress );
				//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
				//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
				e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
				
				// special code for VU#0
				if ( !v->Number )
				{
					// check if Address & 0xf000 == 0x4000
					e->MovRegReg32 ( RDX, RAX );
					e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
					e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
					
					// here it will be loading/storing from/to the registers for VU#1
					e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
					e->CmovERegReg64 ( RCX, RDX );
					
				}
				
				
				if ( !v->Number )
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
				}
				else
				{
					e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
				}
				e->AddRegReg32 ( RAX, RAX );
				
				//e->MovRegFromMem32 ( RDX, RCX, RAX, SCALE_EIGHT, 0 );
				e->movdqa_from_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
				
#ifdef USE_NEW_VECTOR_DISPATCH_LQ_VU

				ret = Dispatch_Result_AVX2(e, v, i, false, RAX, i.Ft);

#else

				if ( i.xyzw != 0xf )
				{
					//e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
					e->pblendwregmemimm ( RAX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}
				
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RAX );

#endif
			}

			/*
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::SQ ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "SQ";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::SQ;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_SQ
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
			Add_FSrcReg ( v, i.Value, i.Fs );
			Add_ISrcReg ( v, i.it & 0xf );
			
			break;
#endif

		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_SQ_RECOMPILE
		case 1:
		
#ifdef DISABLE_SQ_VU0
			// not doing VU#0 for now since it has more involved with load/store
			if ( !v->Number )
			{
				return -1;
			}
#endif
				
			//LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
			e->MovRegMem32 ( RAX, & v->vi [ i.it & 0xf ].s );
			
			e->movdqa_regmem ( RAX, & v->vf [ i.Fs ].sw0 );
			
			//pVuMem32 = v->GetMemPtr ( LoadAddress );
			//return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
			//e->MovRegImm64 ( RCX, (u64) & v->VuMem32 [ 0 ] );
			e->LeaRegMem64 ( RCX, & v->VuMem32 [ 0 ] );
			
			e->AddReg32ImmX ( RAX, (s32) i.Imm11 );
			
			// special code for VU#0
			if ( !v->Number )
			{
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
				e->AndReg32ImmX ( RAX, 0x7fff );
#endif

				// check if Address & 0xf000 == 0x4000
				e->MovRegReg32 ( RDX, RAX );
				e->AndReg32ImmX ( RDX, 0xf000 >> 4 );
				e->CmpReg32ImmX ( RDX, 0x4000 >> 4 );
				e->Jmp8_NE ( 0, 1 );
				
				// here it will be loading/storing from/to the registers for VU#1
				//e->LeaRegMem64 ( RDX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				//e->CmovERegReg64 ( RCX, RDX );
				e->LeaRegMem64 ( RCX, & VU::_VU [ 1 ]->vf [ 0 ].sw0 );
				
				// ***TODO*** check if storing to TPC
				e->CmpReg32ImmX ( RAX, 0x43a );
				e->Jmp8_NE ( 0, 0 );
				
				//VU1::_VU1->Running = 1;
				e->MovMemImm32 ( (int32_t*) & VU1::_VU1->Running, 1 );
				
				//VU1::_VU1->CycleCount = *VU::_DebugCycleCount + 1;
				e->MovRegMem64 ( RDX, (int64_t*) VU::_DebugCycleCount );
				
				// set VBSx in VPU STAT to 1 (running)
				//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( 1 << 3 ) );
				e->OrMemImm32 ( & VU0::_VU0->vi [ 29 ].s, ( 1 << ( 1 << 3 ) ) );
				
				// also set VIFx STAT to indicate program is running
				//VU1::_VU1->VifRegs.STAT.VEW = 1;
				e->OrMemImm32 ( (int32_t*) & VU1::_VU1->VifRegs.STAT.Value, ( 1 << 2 ) );
				
				// finish handling the new cycle count
				e->IncReg64 ( RDX );
				e->MovMemReg64 ( (int64_t*) & VU1::_VU1->CycleCount, RDX );
				
				e->SetJmpTarget8 ( 0 );
				
				// mask address for accessing registers
				e->AndReg32ImmX ( RAX, 0x3f );
				
				e->SetJmpTarget8 ( 1 );
			}
			
			if ( !v->Number )
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem0_Mask >> 4 );
			}
			else
			{
				e->AndReg32ImmX ( RAX, VU::c_ulVuMem1_Mask >> 4 );
			}
			e->AddRegReg32 ( RAX, RAX );
			
#define ENABLE_VU_MASK_STORE_SQ
#ifdef ENABLE_VU_MASK_STORE_SQ

			if (i.xyzw != 0xf)
			{
				//e->movdqa_from_mem128(RCX, RCX, RAX, SCALE_EIGHT, 0);
				//e->pblendwregregimm(RAX, RCX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->pcmpeqb1regreg(RCX, RCX, RCX);
				e->pxor1regreg(RDX, RDX, RDX);
				e->pblendw1regregimm(RCX, RCX, RDX, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				e->vmaskmovd1_memreg(RCX, RAX, RCX, RAX, SCALE_EIGHT, 0);
			}
			else
			{
				ret = e->movdqa_to_mem128(RAX, RCX, RAX, SCALE_EIGHT, 0);
			}

#else
			if ( i.xyzw != 0xf )
			{
				e->movdqa_from_mem128 ( RCX, RCX, RAX, SCALE_EIGHT, 0 );
				e->pblendwregregimm ( RAX, RCX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
			}
			
			ret = e->movdqa_to_mem128 ( RAX, RCX, RAX, SCALE_EIGHT, 0 );
#endif
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::MFP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "MFP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::MFP;
	
	int ret = 1;
	VU::Bitmap128 bmTemp;
	
	switch ( v->OpLevel )
	{
		case 0:
			// delay slot issue
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_MFP_RECOMPILE
		case 1:

			if ( i.Ft && i.xyzw )
			{
				// update p
#ifdef USE_NEW_RECOMPILE2
				// this puts correct p value in RCX anyway
				Perform_UpdateP ( e, v, Address );
#else

				// get current cycle count
				e->MovRegMem64 ( RAX, (int64_t*) & v->CycleCount );
				e->AddReg64ImmX ( RAX, ( Address & v->ulVuMem_Mask ) >> 3 );

				// check if ext unit is done
				e->SubRegMem64 ( RAX, (int64_t*) & v->PBusyUntil_Cycle );

				// get next p value
				e->MovRegFromMem32 ( RCX, (int32_t*) & v->vi [ VU::REG_P ].u );
				//e->MovRegFromMem32 ( RDX, (int32_t*) & v->vi [ VU::REG_STATUSFLAG ].u );
				e->CmovAERegMem32 ( RCX, (int32_t*) & v->NextP );
				e->MovMemReg32 ( (int32_t*) & v->vi [ VU::REG_P ].u, RCX );
#endif

				e->movd_to_sse ( RCX, RCX );
				e->pshufdregregimm ( RCX, RCX, 0 );
				
				if ( i.xyzw != 0xf )
				{
					e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
				}

				// set result
				ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );

			}

			// check if instruction should be cancelled (if it writes to same reg as upper instruction)
			/*
			if ( !( ( 1 << i.Ft ) & v->IDstBitmap ) )
			{
				// check if destination for move is source for upper instruction
				VU::ClearBitmap ( bmTemp );
				VU::AddBitmap ( bmTemp, i.xyzw, i.Ft );
				if ( !VU::TestBitmap ( bmTemp, v->FSrcBitmap ) )
				{
					// add destination register to bitmap at end
					Add_FDstReg ( v, i.Value, i.Ft );
					
					//ret = Generate_VMOVEp ( v, i );
					if ( i.Ft && i.xyzw )
					{
						e->MovRegMem64 ( RAX, (int64_t*) & v->CycleCount );
						e->CmpRegMem64 ( RAX, (int64_t*) & v->PBusyUntil_Cycle );
						
						
						// get new P register value if needed
						//e->movd_regmem ( RCX, & v->vi [ VU::REG_P ].s );
						e->MovRegMem32 ( RAX, ( &v->vi [ VU::REG_P ].s ) );
						e->CmovAERegMem32 ( RAX, & v->NextP.l );
						e->MovMemReg32 ( & v->vi [ VU::REG_P ].s, RAX );
						
						//if ( i.xyzw != 0xf )
						//{
						//	e->movdqa_regmem ( RAX, & v->vf [ i.Ft ].sw0 );
						//}
						
						// sign-extend from 16-bit to 32-bit
						//e->Cwde();
						
						e->movd_to_sse ( RCX, RAX );
						e->pshufdregregimm ( RCX, RCX, 0 );
						
						if ( i.xyzw != 0xf )
						{
							//e->pblendwregregimm ( RCX, RAX, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
							e->pblendwregmemimm ( RCX, & v->vf [ i.Ft ].sw0, ~( ( i.destx * 0x03 ) | ( i.desty * 0x0c ) | ( i.destz * 0x30 ) | ( i.destw * 0xc0 ) ) );
						}
						
						// set result
						//ret = e->MovMemReg32 ( ( &v->vf [ i.Ft ].sw0 ) + FtComponent, RAX );
						ret = e->movdqa_memreg ( & v->vf [ i.Ft ].sw0, RCX );
					}
				}
				else
				{
					return -1;
				}
			}
			*/
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::WAITP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "WAITP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::WAITP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// ***testing***
			//v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_RECOMPILE2_WAITP
		case 1:

			Perform_WaitP ( e, v, Address );

			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::XGKICK ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "XGKICK";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::XGKICK;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_XGKICK
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
			Add_ISrcReg ( v, i.is & 0xf );
			
			break;
#endif

		case 0:
			// xgkick currently at level 0 has delay slot
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::XITOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "XITOP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::XITOP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// integer register destination
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_XITOP_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				e->MovRegMem32 ( RAX, (int32_t*) &v->iVifRegs.ITOP );
				
				if ( !v->Number )
				{
					e->AndReg32ImmX ( RAX, 0xff );
				}
				else
				{
					e->AndReg32ImmX ( RAX, 0x3ff );
				}
				
				// store to 16-bit or full 32-bit ??
				e->MovMemReg16 ( &v->vi [ i.it & 0xf ].sLo, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::XTOP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "XTOP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::XTOP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
		case 0:
			// integer register destination
			v->bStopEncodingAfter = true;
			
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_XTOP_RECOMPILE
		case 1:
			if ( i.it & 0xf )
			{
				e->MovRegMem32 ( RAX, (int32_t*) &v->iVifRegs.TOP );
				e->AndReg32ImmX ( RAX, 0x3ff );
				
				// store to 16-bit or full 32-bit ??
				e->MovMemReg16 ( &v->vi [ i.it & 0xf ].sLo, RAX );
			}
			
			break;
#endif
			
		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






// external unit


int32_t Recompiler::EATAN ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "EATAN";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::EATAN;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_EATAN
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;

#ifdef USE_NEW_EATAN_RECOMPILE
		case 1:
			ENCODE_EATAN(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::EATANxy ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "EATANxy";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::EATANxy;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_EATANxy
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_EATANXY_RECOMPILE
		case 1:
			ENCODE_EATANxy(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::EATANxz ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "EATANxz";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::EATANxz;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_EATANxz
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_EATANXZ_RECOMPILE
		case 1:
			ENCODE_EATANxz(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::EEXP ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "EEXP";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::EEXP;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_EEXP
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_EEXP_RECOMPILE
		case 1:
			ENCODE_EEXP(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ELENG ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ELENG";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ELENG;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ELENG
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ELENG_RECOMPILE
		case 1:
			ENCODE_ELENG(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ERCPR ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ERCPR";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ERCPR;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ERCPR
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ERCPR_RECOMPILE
		case 1:
			ENCODE_ERCPR(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ERLENG ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ERLENG";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ERLENG;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ERLENG
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ERLENG_RECOMPILE
		case 1:
			ENCODE_ERLENG(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ERSADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ERSADD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ERSADD;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ERSADD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ERSADD_RECOMPILE
		case 1:
			ENCODE_ERSADD(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}






int32_t Recompiler::ERSQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ERSQRT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ERSQRT;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ERSQRT
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ERSQRT_RECOMPILE
		case 1:
			ENCODE_ERSQRT(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ESADD ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ESADD";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ESADD;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ESADD
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ESADD_RECOMPILE
		case 1:
			ENCODE_ESADD(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ESIN ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ESIN";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ESIN;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ESIN
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ESIN_RECOMPILE
		case 1:
			ENCODE_ESIN(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ESQRT ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ESQRT";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ESQRT;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ESQRT
		// get source and destination register(s) bitmap
		case -1:
			//v->Add_SrcRegBC ( i.fsf, i.Fs );
			Add_FSrcRegBC ( v, i.fsf, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ESQRT_RECOMPILE
		case 1:
			ENCODE_ESQRT(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}



int32_t Recompiler::ESUM ( x64Encoder *e, VU *v, Vu::Instruction::Format i, u32 Address )
{
	static constexpr const char *c_sName = "ESUM";
	static const void *c_vFunction = (const void*) Vu::Instruction::Execute::ESUM;
	
	int ret = 1;
	
	switch ( v->OpLevel )
	{
#ifdef ENABLE_BITMAP_ESUM
		// get source and destination register(s) bitmap
		case -1:
			//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
			Add_FSrcReg ( v, i.Value, i.Fs );
			
			break;
#endif

		case 0:
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			e->SubReg64ImmX ( RSP, c_lSEH_StackSize );
#endif

			// load arguments
			e->LeaRegMem64 ( RCX, v ); e->LoadImm32 ( RDX, i.Value ); e->MovMemImm32((int32_t*)&v->PC, Address);
			ret = e->Call ( c_vFunction );
				
#ifdef RESERVE_STACK_FRAME_FOR_CALL
			ret = e->AddReg64ImmX ( RSP, c_lSEH_StackSize );
#endif
			
			break;
			
#ifdef USE_NEW_ESUM_RECOMPILE
		case 1:
			ENCODE_ESUM(e, v, i);
			break;
#endif

		default:
			return -1;
			break;
			
	} // end switch ( v->OpLevel )
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding " << c_sName << " instruction.\n";
		return -1;
	}
	
	return 1;
}


// branch is relative, so you know the address it is going to
bool Vu::Recompiler::isBranch(Vu::Instruction::Format64 i)
{
	return ((!i.Hi.I) && ((i.Lo.Opcode & 0x70) == 0x20) && ((i.Lo.Opcode & 0x7e) != 0x24));
}

bool Vu::Recompiler::isJump(Vu::Instruction::Format64 i)
{
	return ((!i.Hi.I) && ((i.Lo.Opcode & 0x7e) == 0x24));
}

// jump is not relative and has the address stored in a register, like when returning from BAL
bool Vu::Recompiler::isBranchOrJump ( Vu::Instruction::Format64 i )
{
	return ( ( !i.Hi.I ) && ( ( i.Lo.Opcode & 0x70 ) == 0x20 ) );
}

bool Vu::Recompiler::isConditionalBranch ( Vu::Instruction::Format64 i )
{
	return ( ( !i.Hi.I ) && ( ( i.Lo.Opcode & 0x78 ) == 0x28 ) );
}

bool Vu::Recompiler::isUnconditionalBranch(Vu::Instruction::Format64 i)
{
	return ((!i.Hi.I) && ((i.Lo.Opcode & 0x78) == 0x20) && ((i.Lo.Opcode & 0x7e) != 0x24));
}

bool Vu::Recompiler::isUnconditionalBranchOrJump ( Vu::Instruction::Format64 i )
{
	return ( ( !i.Hi.I ) && ( ( i.Lo.Opcode & 0x78 ) == 0x20 ) );
}

bool Vu::Recompiler::isMacFlagCheck ( Vu::Instruction::Format64 i )
{
	return ( ( !i.Hi.I ) && ( ( i.Lo.Opcode & 0x7c ) == 0x18 ) );
}

bool Vu::Recompiler::isStatFlagCheck ( Vu::Instruction::Format64 i )
{
	// make sure it is a check and not a set (not fsset)
	return ( ( !i.Hi.I ) && ( ( ( i.Lo.Opcode & 0x7c ) == 0x14 ) && ( ( i.Lo.Opcode & 0x3 ) != 0x1 ) ) );
}

bool Vu::Recompiler::isClipFlagCheck ( Vu::Instruction::Format64 i )
{
	// make sure it is a check and not a set (not fcset)
	return ( ( !i.Hi.I ) && ( ( ( ( i.Lo.Opcode & 0x7c ) == 0x10 ) || ( ( i.Lo.Opcode & 0x7c ) == 0x1c ) ) && ( ( i.Lo.Opcode & 0x3 ) != 0x1 ) ) );
}

bool Vu::Recompiler::isStatFlagSetLo ( Vu::Instruction::Format64 i )
{
	// fsset
	return ( ( !i.Hi.I ) && ( ( ( i.Lo.Opcode & 0x7c ) == 0x14 ) && ( ( i.Lo.Opcode & 0x3 ) == 0x1 ) ) );
}

bool Vu::Recompiler::isClipFlagSetLo ( Vu::Instruction::Format64 i )
{
	// fcset
	return ( ( !i.Hi.I ) && ( ( ( i.Lo.Opcode & 0x7c ) == 0x10 ) && ( ( i.Lo.Opcode & 0x3 ) == 0x1 ) ) );
}


bool Vu::Recompiler::isLowerImmediate ( Vu::Instruction::Format64 i )
{
	return ( i.Hi.I != 0 );
}

bool Vu::Recompiler::isClipFlagSetHi ( Vu::Instruction::Format64 i )
{
	// check for clip instruction
	return ( i.Hi.Imm11 == 0x1ff );
}


// bits 0-31: float reg map
// bits 32-47: int reg map
// bits
u64 Vu::Recompiler::getSrcRegMapLo ( Vu::Instruction::Format64 i )
{
	// float: move,mr32,mtir,sq,sqd,sqi,rinit,div,rsqrt,sqrt,eatan,eatanxy,eatanxz,eexp,eleng,ercpr,erleng,ersadd,ersqrt,esadd,esin,esqrt,esum
	// int: mfir,fmand,fmeq,fmor,iaddi,iaddiu,iand,ibeq,ibgez,ibgtz,iblez,ibltz,ibne,ilw,ilwr,ior,isub,isubiu,isw,iswr,jalr,jr,lq,lqd,lqi,xgkick

	u64 ullBm = 0;

	// if lo is immediate, then done
	if ( i.Hi.I ) return 0;

	// sq reads from fs and it
	// opcode=0x01
	if ( i.Lo.Opcode == 0x01 ) ullBm |= ( 1ull << i.Lo.Fs ) | ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	// lq,ilw,iaddiu,isubiu,fmand,fmeq,fmor,ibgez,ibgtz,iblez,ibltz reads from is
	// also jr(op=0x24),jalr(op=0x25)
	// opcode=0x00,0x04,0x08,0x09,0x1a,0x18,0x1b,0x2f,0x2d,0x2e,0x2c
	if ( !i.Lo.Opcode ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x04 ) || ( i.Lo.Opcode == 0x08 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x09 ) || ( i.Lo.Opcode == 0x18 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x1a ) || ( i.Lo.Opcode == 0x1b ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode >= 0x2c ) && ( i.Lo.Opcode <= 0x2f ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x24 ) || ( i.Lo.Opcode == 0x25 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );

	// ibeq,ibne reads from is and it
	// opcode=0x28,0x29
	if ( ( i.Lo.Opcode == 0x28 ) || ( i.Lo.Opcode == 0x29 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) ) | ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	if ( i.Lo.Opcode != 0x40 ) return ullBm;

	// iadd,iand,ior,isub read from is and it
	// Funct=0x30,0x34,0x35,0x31
	if ( ( i.Lo.Funct == 0x30 ) || ( i.Lo.Funct == 0x31 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) ) | ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Funct == 0x34 ) || ( i.Lo.Funct == 0x35 ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) ) | ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );


	// move,mr32,mtir,rinit,rxor,eatan,eatanxy,eatanxz,eexp,eleng,ercpr,erleng,ersadd,ersqrt,esadd,esin,esqrt,esum reads from fs
	// Imm11=0x33c,0x33d,0x3fc,0x43e,0x43f,0x7fd,0x77c,0x77d,0x7fe,0x73e,0x7be,0x73f,0x73d,0x7bd,0x73c,0x7fc,0x7bc,0x77e
	if ( i.Lo.Imm11 == 0x3fc ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm11 >= 0x33c ) && ( i.Lo.Imm11 <= 0x33d ) ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm15_0 >= 0x43e ) && ( i.Lo.Imm15_0 <= 0x43f ) ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm15_0 >= 0x7fc ) && ( i.Lo.Imm15_0 <= 0x7fe ) ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm15_0 >= 0x77c ) && ( i.Lo.Imm15_0 <= 0x77e ) ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm15_0 >= 0x73c ) && ( i.Lo.Imm15_0 <= 0x73f ) ) ullBm |= ( 1ull << i.Lo.Fs );
	if ( ( i.Lo.Imm15_0 >= 0x7bc ) && ( i.Lo.Imm15_0 <= 0x7be ) ) ullBm |= ( 1ull << i.Lo.Fs );

	// div,rsqrt reads from fs and ft
	// Imm11=0x3bc,0x3be
	if ( ( i.Lo.Imm11 == 0x3bc ) || ( i.Lo.Imm11 == 0x3be ) ) ullBm |= ( 1ull << i.Lo.Fs ) | ( 1ull << i.Lo.Ft );

	// sqd,sqi reads from fs and it
	// Imm11=0x37f,0x37d
	if ( ( i.Lo.Imm11 == 0x37f ) || ( i.Lo.Imm11 == 0x37d ) ) ullBm |= ( 1ull << i.Lo.Fs ) | ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	// sqrt reads from ft
	// Imm11=0x3bd
	if ( i.Lo.Imm11 == 0x3bd ) ullBm |= ( 1ull << i.Lo.Ft );

	// mfir,ilwr,xgkick reads from is
	// Imm11=0x3fd,0x3fe,0x6fc
	if ( i.Lo.Imm15_0 == 0x6fc ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );
	if ( ( i.Lo.Imm11 == 0x3fd ) || ( i.Lo.Imm11 == 0x3fe ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );


	return ullBm;
}

u64 Vu::Recompiler::getDstRegMapLo ( Vu::Instruction::Format64 i )
{
	// float: move,mr32,mfir,mfp,lq,lqd,lqi,rget,rnext,rxor
	// int: mtir,xtop,xitop

	u64 ullBm = 0;

	// if lo is immediate, then done
	if ( i.Hi.I ) return 0;

	// lq opcode=0x0 writes to ft
	if ( !i.Lo.Opcode ) ullBm |= ( 1ull << i.Lo.Ft );

	// fcand,fceq,fcor write to vi01
	// opcode=0x12,0x10,0x13
	if ( ( i.Lo.Opcode == 0x12 ) || ( i.Lo.Opcode == 0x10 ) || ( i.Lo.Opcode == 0x13 ) ) ullBm |= ( 1ull << 33 );


	// fcget,fmand,fmeq,fmor,fsand,fseq,fsor,iaddiu,isubiu,ilw,bal,jalr writes to it
	// opcode=0x1c,0x1a,0x18,0x1b,0x16,0x14,0x17,0x08,0x09,0x04,0x21,0x25
	if ( i.Lo.Opcode == 0x04 ) ullBm |= ( 1ull << ( i.Lo.it + 32 ) );
	if ( ( i.Lo.Opcode >= 0x08 ) && ( i.Lo.Opcode <= 0x09 ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x14 ) || ( i.Lo.Opcode == 0x16 ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode >= 0x17 ) && ( i.Lo.Opcode <= 0x18 ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode >= 0x1a ) && ( i.Lo.Opcode <= 0x1c ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Opcode == 0x21 ) || ( i.Lo.Opcode == 0x25 ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );


	if ( i.Lo.Opcode != 0x40 ) return ullBm;

	// iaddi writes to it
	// funct=0x32
	if ( i.Lo.Funct == 0x32 ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	// ilwr,mtir,xtop,xitop writes to it
	// Imm11=0x3fe,0x3fc,0x6bc,0x6bd
	if ( ( i.Lo.Imm11 == 0x3fe ) || ( i.Lo.Imm11 == 0x3fc ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );
	if ( ( i.Lo.Imm15_0 == 0x6bc ) || ( i.Lo.Imm15_0 == 0x6bd ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	// iadd,iand,ior,isub writes to id
	// funct=0x30,0x34,0x35,0x31
	if ( ( i.Lo.Funct == 0x30 ) || ( i.Lo.Funct == 0x31 ) ) ullBm |= ( 1ull << ( ( i.Lo.id & 0xf ) + 32 ) );
	if ( ( i.Lo.Funct == 0x34 ) || ( i.Lo.Funct == 0x35 ) ) ullBm |= ( 1ull << ( ( i.Lo.id & 0xf ) + 32 ) );


	// opcode=0x40
	// lqd,lqi,mfir,mfp,move,mr32,rget,rnext
	// Imm11=0x37e,0x37c,0x3fd,0x67c,0x33c,0x33d,0x43d,0x43c

	// lqd, lqi
	if ( ( i.Lo.Imm11 == 0x37e ) || ( i.Lo.Imm11 == 0x37c ) ) ullBm |= ( 1ull << i.Lo.Ft );

	// mfir,mfp
	if ( ( i.Lo.Imm11 == 0x3fd ) || ( i.Lo.Imm15_0 == 0x67c ) ) ullBm |= ( 1ull << i.Lo.Ft );

	// move,mr32
	if ( ( i.Lo.Imm11 == 0x33c ) || ( i.Lo.Imm11 == 0x33d ) ) ullBm |= ( 1ull << i.Lo.Ft );

	// rget,rnext
	if ( ( i.Lo.Imm15_0 == 0x43d ) || ( i.Lo.Imm15_0 == 0x43c ) ) ullBm |= ( 1ull << i.Lo.Ft );

	// lqd, lqi also updates is
	if ( ( i.Lo.Imm11 == 0x37e ) || ( i.Lo.Imm11 == 0x37c ) ) ullBm |= ( 1ull << ( ( i.Lo.is & 0xf ) + 32 ) );

	// sqd, sqi also updates it
	if ( ( i.Lo.Imm11 == 0x37f ) || ( i.Lo.Imm11 == 0x37d ) ) ullBm |= ( 1ull << ( ( i.Lo.it & 0xf ) + 32 ) );

	return ullBm;
}


void Vu::Recompiler::getSrcFieldMapLo ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i )
{
	// move,mr32,mtir,sq,sqd,sqi,rinit,rxor,div,rsqrt,sqrt,eatan,eatanxy,eatanxz,eexp,eleng,ercpr,erleng,ersadd,ersqrt,esadd,esin,esqrt,esum

	// clear the bitmap first, in case there are no float src regs
	VU::ClearBitmap( Bm );

	// if lo is immediate, then done
	if ( i.Hi.I ) return;

	// sq, opcode=0x01, src fs.dest and it
	if ( i.Lo.Opcode == 0x1 ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Fs );

	// if opcode now is not 0x40, then return
	if ( i.Lo.Opcode != 0x40 ) return;

	// src fs.dest: move,eatanxy,eatanxz,eleng,erleng,ersadd,esadd,esum
	// 0x33c,0x77c,0x77d,0x73e,0x73f,0x73d,0x73c,0x77e

	// move
	if ( i.Lo.Imm11 == 0x33c ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Fs );

	// eatanxy,eatanxz,esum
	if ( ( i.Lo.Imm15_0 >= 0x77c ) && ( i.Lo.Imm15_0 <= 0x77e ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Fs );

	// eleng,erleng,ersadd,esadd
	if ( ( i.Lo.Imm15_0 >= 0x73c ) && ( i.Lo.Imm15_0 <= 0x73f ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Fs );

	// src fs.fsf: mtir,rinit,rxor,eatan,eexp,ercpr,ersqrt,esin,esqrt
	// 0x3fc,0x43e,0x43f,0x7fd,0x7fe,0x7be,0x7bd,0x7fc,0x7bc

	// mtir
	if ( i.Lo.Imm11 == 0x3fc ) VU::CreateBitmap ( Bm, 0x8 >> i.Lo.fsf, i.Lo.Fs );

	// rinit,rxor
	if ( ( i.Lo.Imm15_0 >= 0x43e ) && ( i.Lo.Imm15_0 <= 0x43f ) ) VU::CreateBitmap ( Bm, 0x8 >> i.Lo.fsf, i.Lo.Fs );

	// eatan,eexp,ercpr,ersqrt
	if ( ( i.Lo.Imm15_0 >= 0x7fc ) && ( i.Lo.Imm15_0 <= 0x7fe ) ) VU::CreateBitmap ( Bm, 0x8 >> i.Lo.fsf, i.Lo.Fs );

	// esqrt,
	if ( ( i.Lo.Imm15_0 >= 0x7bc ) && ( i.Lo.Imm15_0 <= 0x7be ) ) VU::CreateBitmap ( Bm, 0x8 >> i.Lo.fsf, i.Lo.Fs );

	// src fs.fsf, ft.ftf: div,rsqrt,sqrt
	// 0x3bc,0x3be,0x3bd
	if ( ( i.Lo.Imm11 >= 0x3bc ) && ( i.Lo.Imm11 <= 0x3be ) ) { VU::CreateBitmap ( Bm, 0x8 >> i.Lo.fsf, i.Lo.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Lo.ftf, i.Lo.Ft ); }

	// for mr32, it must be the rotated xyzw as source ??
	//if ( i.Lo.Imm11 == 0x33d ) VU::CreateBitmap ( Bm, ( ( i.Lo.xyzw << 1 ) | ( i.Lo.xyzw >> 3 ) ) & 0xf, i.Lo.Fs );
	if (i.Lo.Imm11 == 0x33d) VU::CreateBitmap(Bm, ((i.Lo.xyzw >> 1) | (i.Lo.xyzw << 3)) & 0xf, i.Lo.Fs);

	// sqd,sqi have fs.dest and it as source
	// 0x37f,0x37d
	if ( ( i.Lo.Imm11 == 0x37f ) || ( i.Lo.Imm11 == 0x37d ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Fs );

	return;
}

void Vu::Recompiler::getDstFieldMapLo ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i )
{
	// move,mr32,mfir,mfp,lq,lqd,lqi,rget,rnext
	// dest reg always ft.dest

	// clear the bitmap first, in case there are no dst regs
	VU::ClearBitmap( Bm );

	// if lo is immediate, then done
	if ( i.Hi.I ) return;

	// lq opcode=0x0
	if ( !i.Lo.Opcode ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Ft );

	// if opcode now is not 0x40, then return
	if ( i.Lo.Opcode != 0x40 ) return;

	// opcode=0x40
	// lqd,lqi,mfir,mfp,move,mr32,rget,rnext
	// 0x37e,0x37c,0x3fd,0x67c,0x33c,0x33d,0x43d,0x43c

	// lqd, lqi
	if ( ( i.Lo.Imm11 == 0x37e ) || ( i.Lo.Imm11 == 0x37c ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Ft );

	// mfir,mfp
	if ( ( i.Lo.Imm11 == 0x3fd ) || ( i.Lo.Imm11 == 0x67c ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Ft );

	// move,mr32
	if ( ( i.Lo.Imm11 == 0x33c ) || ( i.Lo.Imm11 == 0x33d ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Ft );

	// rget,rnext
	if ( ( i.Lo.Imm15_0 == 0x43d ) || ( i.Lo.Imm15_0 == 0x43c ) ) VU::CreateBitmap ( Bm, i.Lo.xyzw, i.Lo.Ft );

	return;
}

void Vu::Recompiler::getSrcFieldMapHi ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i )
{
	// nop has no src regs
	//if ( i.Hi.Imm11 == 0x2ff ) return 0;

	// clear the bitmap first, in case there are no src regs
	VU::ClearBitmap( Bm );

	// i,q,abs,ftoi,itof only have one source reg fs.dest
	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x27 ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Funct >= 0x1c ) && ( i.Hi.Funct <= 0x1f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x1fc ) && ( i.Hi.Imm11 <= 0x1fe ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x17c ) && ( i.Hi.Imm11 <= 0x17f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x13c ) && ( i.Hi.Imm11 <= 0x13f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x23c ) && ( i.Hi.Imm11 <= 0x23f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x27c ) && ( i.Hi.Imm11 <= 0x27f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs );

	// clip has fs.xyz and ft.w as source
	if ( i.Hi.Imm11 == 0x1ff ) { VU::CreateBitmap ( Bm, 0xe, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x1, i.Hi.Ft ); }

	// max,min,add,adda,madd,madda,msub,msuba,mul,mula,sub,suba have fs.dest and ft.dest as source
	if ( ( i.Hi.Funct >= 0x28 ) && ( i.Hi.Funct <= 0x2d ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft ); }
	if ( i.Hi.Funct == 0x2f ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft ); }

	// adda,madda,msuba,mula,suba has fs.dest and ft.dest as source
	if ( ( i.Hi.Imm11 >= 0x2bc ) && ( i.Hi.Imm11 <= 0x2be ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft ); }
	if ( ( i.Hi.Imm11 >= 0x2fc ) && ( i.Hi.Imm11 <= 0x2fd ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft ); }

	// opmsub has fs.xyz and ft.xyz as source
	if ( i.Hi.Funct == 0x2e ) { VU::CreateBitmap ( Bm, 0xe, i.Hi.Fs ); VU::AddBitmap ( Bm, 0xe, i.Hi.Ft ); }

	// opmula has fs.xyz and ft.xyz as source
	if ( i.Hi.Imm11 == 0x2fe ) { VU::CreateBitmap ( Bm, 0xe, i.Hi.Fs ); VU::AddBitmap ( Bm, 0xe, i.Hi.Ft ); }

	// bc ops has fs.dest and ft.bc as source
	if ( i.Hi.Funct <= 0x1b ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	// adda bc
	if ( ( i.Hi.Imm11 >= 0x03c ) && ( i.Hi.Imm11 <= 0x03f ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	// suba bc
	if ( ( i.Hi.Imm11 >= 0x07c ) && ( i.Hi.Imm11 <= 0x07f ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	// madda bc
	if ( ( i.Hi.Imm11 >= 0x0bc ) && ( i.Hi.Imm11 <= 0x0bf ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	// msuba bc
	if ( ( i.Hi.Imm11 >= 0x0fc ) && ( i.Hi.Imm11 <= 0x0ff ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	// mula bc
	if ( ( i.Hi.Imm11 >= 0x1bc ) && ( i.Hi.Imm11 <= 0x1bf ) ) { VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fs ); VU::AddBitmap ( Bm, 0x8 >> i.Hi.bc, i.Hi.Ft ); }

	return;
}



void Vu::Recompiler::getDstFieldMapHi ( VU::Bitmap128 &Bm, Vu::Instruction::Format64 i )
{
	// nop has no dst regs
	//if ( i.Hi.Imm11 == 0x2ff ) return 0;

	// clear the bitmap first, in case there is no dst reg
	VU::ClearBitmap( Bm );

	// 6-bit i,q,abs have dst reg fd.dest

	// 6-bit i,q
	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x27 ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );

	// min,max i,q
	if ( ( i.Hi.Funct >= 0x1c ) && ( i.Hi.Funct <= 0x1f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );

	// abs
	if ( i.Hi.Imm11 == 0x1fd ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );

	// ftoi,itof has dst reg ft.dest
	if ( ( i.Hi.Imm11 >= 0x17c ) && ( i.Hi.Imm11 <= 0x17f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft );
	if ( ( i.Hi.Imm11 >= 0x13c ) && ( i.Hi.Imm11 <= 0x13f ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Ft );

	// max,min,add,madd,msub,mul,sub has dst reg fd.dest
	if ( ( i.Hi.Funct >= 0x28 ) && ( i.Hi.Funct <= 0x2d ) ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );
	if ( i.Hi.Funct == 0x2f ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );

	// adda,madda,msuba,mula,suba store to acc
	// opmula store to acc

	// opmsub has has dst reg fd.xyz
	if ( i.Hi.Funct == 0x2e ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );


	// bc ops has fd.dest as dst reg unless storing to acc
	if ( i.Hi.Funct <= 0x1b ) VU::CreateBitmap ( Bm, i.Hi.xyzw, i.Hi.Fd );

	// adda bc
	// suba bc
	// madda bc
	// msuba bc
	// mula bc
	// clip
	// nop

	return;
}




u64 Vu::Recompiler::getDstRegMapHi ( Vu::Instruction::Format64 i )
{
	// nop has no dst regs
	if ( i.Hi.Imm11 == 0x2ff ) return 0;

	// 6-bit i,q,abs have dst reg fd.dest

	// 6-bit i,q
	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x27 ) ) return ( 1ull << i.Hi.Fd );

	// min,max i,q
	if ( ( i.Hi.Funct >= 0x1c ) && ( i.Hi.Funct <= 0x1f ) ) return ( 1ull << i.Hi.Fd );

	// abs
	if ( i.Hi.Imm11 == 0x1fd ) return ( 1ull << i.Hi.Fd );

	// ftoi,itof has dst reg ft.dest
	if ( ( i.Hi.Imm11 >= 0x17c ) && ( i.Hi.Imm11 <= 0x17f ) ) return ( 1ull << i.Hi.Ft );
	if ( ( i.Hi.Imm11 >= 0x13c ) && ( i.Hi.Imm11 <= 0x13f ) ) return ( 1ull << i.Hi.Ft );

	// max,min,add,madd,msub,mul,sub has dst reg fd.dest
	if ( ( i.Hi.Funct >= 0x28 ) && ( i.Hi.Funct <= 0x2d ) ) return ( 1ull << i.Hi.Fd );
	if ( i.Hi.Funct == 0x2f ) return ( 1ull << i.Hi.Fd );

	// adda,madda,msuba,mula,suba store to acc
	// opmula store to acc

	// opmsub has has dst reg fd.xyz
	if ( i.Hi.Funct == 0x2e ) return ( 1ull << i.Hi.Fd );


	// bc ops has fd.dest as dst reg unless storing to acc
	if ( i.Hi.Funct <= 0x1b ) return ( 1ull << i.Hi.Fd );

	// adda bc
	// suba bc
	// madda bc
	// msuba bc
	// mula bc
	// clip
	// nop

	return 0;
}

u64 Vu::Recompiler::getSrcRegMapHi ( Vu::Instruction::Format64 i )
{
	// nop has no src regs
	if ( i.Hi.Imm11 == 0x2ff ) return 0;

	// i,q,abs,ftoi,itof only have one source reg fs.dest
	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x27 ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Funct >= 0x1c ) && ( i.Hi.Funct <= 0x1f ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x1fc ) && ( i.Hi.Imm11 <= 0x1fe ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x17c ) && ( i.Hi.Imm11 <= 0x17f ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x13c ) && ( i.Hi.Imm11 <= 0x13f ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x23c ) && ( i.Hi.Imm11 <= 0x23f ) ) return ( 1ull << i.Hi.Fs );
	if ( ( i.Hi.Imm11 >= 0x27c ) && ( i.Hi.Imm11 <= 0x27f ) ) return ( 1ull << i.Hi.Fs );

	// clip has fs.xyz and ft.w as source
	if ( i.Hi.Imm11 == 0x1ff ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// max,min,add,adda,madd,madda,msub,msuba,mul,mula,sub,suba have fs.dest and ft.dest as source
	if ( ( i.Hi.Funct >= 0x28 ) && ( i.Hi.Funct <= 0x2d ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );
	if ( i.Hi.Funct == 0x2f ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// adda,madda,msuba,mula,suba has fs.dest and ft.dest as source
	if ( ( i.Hi.Funct >= 0x2bc ) && ( i.Hi.Funct <= 0x2be ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );
	if ( ( i.Hi.Funct >= 0x2fc ) && ( i.Hi.Funct <= 0x2fd ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// opmsub has fs.xyz and ft.xyz as source
	if ( i.Hi.Funct == 0x2e ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// opmula has fs.xyz and ft.xyz as source
	if ( i.Hi.Imm11 == 0x2fe ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// bc ops has fs.dest and ft.bc as source
	if ( i.Hi.Funct <= 0x1b ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// adda bc
	if ( ( i.Hi.Imm11 >= 0x03c ) && ( i.Hi.Imm11 <= 0x03f ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// suba bc
	if ( ( i.Hi.Imm11 >= 0x07c ) && ( i.Hi.Imm11 <= 0x07f ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// madda bc
	if ( ( i.Hi.Imm11 >= 0x0bc ) && ( i.Hi.Imm11 <= 0x0bf ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// msuba bc
	if ( ( i.Hi.Imm11 >= 0x0fc ) && ( i.Hi.Imm11 <= 0x0ff ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	// mula bc
	if ( ( i.Hi.Imm11 >= 0x1bc ) && ( i.Hi.Imm11 <= 0x1bf ) ) return ( ( 1ull << i.Hi.Fs ) | ( 1ull << i.Hi.Ft ) );

	return 0;
}


bool Vu::Recompiler::isMacStatFlagSetHi ( Vu::Instruction::Format64 i )
{
	// upper instructions not setting mac/stat:
	// 11-bit code
	// NOP(0x2ff),ABS(0x1fd),CLIP(0x1ff),FTOI(0x17c,0x17d,0x17e,0x17f),ITOF(0x13c,0x13d,0x13e,0x13f)
	// 6-bit code
	// MAX(0x2b,0x1d,0x10,0x11,0x12,0x13),MIN(0x2f,0x1f,0x14,0x15,0x16,0x17)
	// upper instructions that set mac/stat
	// ADD(0x28,0x22,0x20,0x00,0x01,0x02,0x03),ADDA(0x2bc,0x23e,0x23c,0x03c,0x03d,0x03e,0x03f),
	// MADD(0x29,0x23,0x21,0x08,0x09,0x0a,0x0b),MADDA(0x2bd,0x23f,0x23d,0x0bc,0x0bd,0x0be,0x0bf),
	// MSUB(0x2d,0x27,0x25,0x0c,0x0d,0x0e,0x0f),MSUBA(0x2fd,0x27f,0x27d,0x0fc,0x0fd,0x0fe,0x0ff),
	// MUL(0x2a,0x1e,0x1c,0x18,0x19,0x1a,0x1b),MULA(0x2be,0x1fe,0x1fc,0x1bc,0x1bd,0x1be,0x1bf),
	// SUB(0x2c,0x26,0x24,0x04,0x05,0x06,0x07),SUBA(0x2fc,0x27e,0x27c,0x07c,0x07d,0x07e,0x07f),
	// OPMULA(0x2fe),OPMSUB(0x2e)

	if ( i.Hi.Funct <= 0x0f ) return true;

	if ( ( i.Hi.Funct >= 0x18 ) && ( i.Hi.Funct <= 0x1c ) ) return true;

	if ( i.Hi.Funct == 0x1e ) return true;

	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x2a ) ) return true;

	if ( ( i.Hi.Funct >= 0x2c ) && ( i.Hi.Funct <= 0x2e ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x03c ) && ( i.Hi.Imm11 <= 0x03f ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x07c ) && ( i.Hi.Imm11 <= 0x07f ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x0bc ) && ( i.Hi.Imm11 <= 0x0bf ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x0fc ) && ( i.Hi.Imm11 <= 0x0ff ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x1bc ) && ( i.Hi.Imm11 <= 0x1bf ) ) return true;


	if ( ( i.Hi.Imm11 >= 0x23c ) && ( i.Hi.Imm11 <= 0x23f ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x27c ) && ( i.Hi.Imm11 <= 0x27f ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x2bc ) && ( i.Hi.Imm11 <= 0x2be ) ) return true;

	if ( ( i.Hi.Imm11 >= 0x2fc ) && ( i.Hi.Imm11 <= 0x2fe ) ) return true;

	if ( ( i.Hi.Imm11 == 0x1fc ) || ( i.Hi.Imm11 == 0x1fe ) ) return true;

	return false;
}

// move, mr32, mfir, mfp, rget, rnext, lq, lqd, lqi
bool Vu::Recompiler::isMoveToFloatLo ( Vu::Instruction::Format64 i )
{
	// move, mr32, mfir, mfp, rget, rnext, lq, lqd, lqi
	// 0x33c,0x33d,0x3fd,0x67c,0x43d,0x43c,lq,0x37e,0x37c

	// make sure lower instruction is not immediate
	if ( i.Hi.I ) return false;

	// if no destination, then instruction doesn't operate
	if ( !i.Lo.xyzw ) return false;

	// lq
	if ( !i.Lo.Opcode ) return true;

	// make sure lower op
	if ( i.Lo.Opcode != 0x40 ) return false;

	if ( i.Lo.Imm11 == 0x3fd ) return true;
	if ( i.Lo.Imm15_0 == 0x67c ) return true;
	if ( ( i.Lo.Imm11 == 0x33c ) || ( i.Lo.Imm11 == 0x33d ) ) return true;
	if ( ( i.Lo.Imm15_0 == 0x43c ) || ( i.Lo.Imm15_0 == 0x43d ) ) return true;
	if ( ( i.Lo.Imm11 == 0x37c ) || ( i.Lo.Imm11 == 0x37e ) ) return true;

	return false;
}

// move or mr32
bool Vu::Recompiler::isMoveToFloatFromFloatLo ( Vu::Instruction::Format64 i )
{
	// make sure lower instruction is not immediate
	if ( i.Hi.I ) return false;

	// if no destination, then instruction doesn't operate
	if ( !i.Lo.xyzw ) return false;

	// make sure lower op
	if ( i.Lo.Opcode != 0x40 ) return false;

	if ( ( i.Lo.Imm11 == 0x33c ) || ( i.Lo.Imm11 == 0x33d ) ) return true;

	return false;
}

// ilw(op=4),ilwr(funct=0x3fe)
bool Vu::Recompiler::isIntLoad ( Vu::Instruction::Format64 i )
{
	if ( i.Hi.I ) return false;

	if ( i.Lo.Opcode == 0x4 ) return true;

	if ( i.Lo.Opcode != 0x40 ) return false;

	if ( i.Lo.Funct == 0x3fe ) return true;

	return false;
}

// checks for lq,sq,lqi,lqd,sqi,sqd
bool Vu::Recompiler::isFloatLoadStore ( Vu::Instruction::Format64 i )
{
	if ( i.Hi.I ) return false;

	// lq, sq
	if ( i.Lo.Opcode <= 0x1 ) return true;

	if ( i.Lo.Opcode != 0x40 ) return false;

	// lqd, lqi
	if ( ( i.Lo.Imm11 == 0x37e ) || ( i.Lo.Imm11 == 0x37c ) ) return true;

	// sqd, sqi
	if ( ( i.Lo.Imm11 == 0x37f ) || ( i.Lo.Imm11 == 0x37d ) ) return true;

	return false;
}


// mulq,addq,etc
// note: would also need to check for stat flag updates separately?
bool Vu::Recompiler::isQUpdate ( Vu::Instruction::Format64 i )
{
	// addq,maddq,msubq,mulq,subq
	// funct=0x20,0x21,0x25,0x1c,0x24
	if ( ( i.Hi.Funct >= 0x20 ) && ( i.Hi.Funct <= 0x21 ) ) return true;
	if ( ( i.Hi.Funct >= 0x24 ) && ( i.Hi.Funct <= 0x25 ) ) return true;
	if ( i.Hi.Funct == 0x1c ) return true;

	// addaq,maddaq,msubaq,mulaq,subaq
	// imm11=0x23c,0x23d,0x27d,0x1fc,0x27c
	if ( ( i.Hi.Imm11 >= 0x23c ) && ( i.Hi.Imm11 <= 0x23d ) ) return true;
	if ( ( i.Hi.Imm11 >= 0x27c ) && ( i.Hi.Imm11 <= 0x27d ) ) return true;
	if ( i.Hi.Imm11 == 0x1fc ) return true;

	return false;
}


// basically just mfp
bool Vu::Recompiler::isPUpdate ( Vu::Instruction::Format64 i )
{
	// mfp
	// imm15_0=0x67c
	if ( i.Hi.I ) return false;
	if ( i.Lo.Opcode != 0x40 ) return false;
	if ( i.Lo.Imm15_0 == 0x67c ) return true;

	return false;
}


bool Vu::Recompiler::isQWait ( Vu::Instruction::Format64 i )
{
	// waitq,div,sqrt,rsqrt
	// (imm11=0x3bf,0x3bc,0x3bd,0x3be)
	if ( i.Hi.I ) return false;
	if ( i.Lo.Opcode != 0x40 ) return false;

	if ( ( i.Lo.Imm11 >= 0x3bc ) && ( i.Lo.Imm11 <= 0x3bf ) ) return true;

	return false;
}

bool Vu::Recompiler::isPWait ( Vu::Instruction::Format64 i )
{
	// waitp (imm11=0x7bf)
	// eatan,eatanxy,eatanxz,eexp,eleng,ercpr,erleng,ersadd,ersqrt,esadd,esin,esqrt,esum reads from fs
	// Imm11=0x7fd,0x77c,0x77d,0x7fe,0x73e,0x7be,0x73f,0x73d,0x7bd,0x73c,0x7fc,0x7bc,0x77e
	if ( i.Hi.I ) return false;
	if ( i.Lo.Opcode != 0x40 ) return false;
	if ( ( i.Lo.Imm15_0 >= 0x73c ) && ( i.Lo.Imm15_0 <= 0x73f ) ) return true;
	if ( ( i.Lo.Imm15_0 >= 0x77c ) && ( i.Lo.Imm15_0 <= 0x77e ) ) return true;
	if ( ( i.Lo.Imm15_0 >= 0x7bc ) && ( i.Lo.Imm15_0 <= 0x7bf ) ) return true;
	if ( ( i.Lo.Imm15_0 >= 0x7fc ) && ( i.Lo.Imm15_0 <= 0x7fe ) ) return true;

	return false;
}

bool Vu::Recompiler::isXgKick ( Vu::Instruction::Format64 i )
{
	// xgkick (Imm15_0=0x6fc)
	if ( i.Hi.I ) return false;
	if ( i.Lo.Opcode != 0x40 ) return false;
	if ( i.Lo.Imm15_0 == 0x6fc ) return true;

	return false;
}

bool Vu::Recompiler::isABS(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x1fd);
}

bool Vu::Recompiler::isCLIP(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x1ff);
}

bool Vu::Recompiler::isITOF(Vu::Instruction::Format64 i)
{
	return ((i.Hi.Imm11 & 0x7fc) == 0x13c);
}

bool Vu::Recompiler::isITOF0(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x13c);
}
bool Vu::Recompiler::isITOF4(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x13d);
}
bool Vu::Recompiler::isITOF12(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x13e);
}
bool Vu::Recompiler::isITOF15(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x13f);
}

bool Vu::Recompiler::isFTOI(Vu::Instruction::Format64 i)
{
	return ((i.Hi.Imm11 & 0x7fc) == 0x17c);
}

bool Vu::Recompiler::isFTOI0(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x17c);
}
bool Vu::Recompiler::isFTOI4(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x17d);
}
bool Vu::Recompiler::isFTOI12(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x17e);
}
bool Vu::Recompiler::isFTOI15(Vu::Instruction::Format64 i)
{
	return (i.Hi.Imm11 == 0x17f);
}


bool Vu::Recompiler::isMIN(Vu::Instruction::Format64 i)
{
	// MIN,MINI,MINBC
	return ((i.Hi.Funct == 0x2f) || (i.Hi.Funct == 0x1f) || ((i.Hi.Funct & 0x3c) == 0x14));
}

bool Vu::Recompiler::isMINBC(Vu::Instruction::Format64 i)
{
	// MINBC
	return (((i.Hi.Funct & 0x3c) == 0x14));
}

bool Vu::Recompiler::isMIN_NotI(Vu::Instruction::Format64 i)
{
	// MIN,MINBC
	return ((i.Hi.Funct == 0x2f) || ((i.Hi.Funct & 0x3c) == 0x14));
}

bool Vu::Recompiler::isMINi(Vu::Instruction::Format64 i)
{
	// MINI
	return (i.Hi.Funct == 0x1f);
}

bool Vu::Recompiler::isMAX(Vu::Instruction::Format64 i)
{
	// MAX,MAXI,MAXBC
	return ((i.Hi.Funct == 0x2b) || (i.Hi.Funct == 0x1d) || ((i.Hi.Funct & 0x3c) == 0x10));
}

bool Vu::Recompiler::isMAXBC(Vu::Instruction::Format64 i)
{
	// MAXBC
	return (((i.Hi.Funct & 0x3c) == 0x10));
}

bool Vu::Recompiler::isMAX_NotI(Vu::Instruction::Format64 i)
{
	// MAX,MAXBC
	return ((i.Hi.Funct == 0x2b) || ((i.Hi.Funct & 0x3c) == 0x10));
}

bool Vu::Recompiler::isMAXi(Vu::Instruction::Format64 i)
{
	// MAXI
	return (i.Hi.Funct == 0x1d);
}


bool Vu::Recompiler::isADD(Vu::Instruction::Format64 i)
{
	// ADD,ADDI,ADDQ,ADDBC
	return ((i.Hi.Funct == 0x28) || (i.Hi.Funct == 0x22) || (i.Hi.Funct == 0x20) || ((i.Hi.Funct & 0x3c) == 0x00));
}

bool Vu::Recompiler::isADDi(Vu::Instruction::Format64 i)
{
	// ADDI
	return ((i.Hi.Funct == 0x22));
}

bool Vu::Recompiler::isADDq(Vu::Instruction::Format64 i)
{
	// ADDQ
	return ((i.Hi.Funct == 0x20));
}

bool Vu::Recompiler::isADDBC(Vu::Instruction::Format64 i)
{
	// ADDBC
	return (((i.Hi.Funct & 0x3c) == 0x00));
}

bool Vu::Recompiler::isADDA(Vu::Instruction::Format64 i)
{
	// ADDA,ADDAI,ADDAQ,ADDABC
	return ((i.Hi.Imm11 == 0x2bc) || (i.Hi.Imm11 == 0x23e) || (i.Hi.Imm11 == 0x23c) || ((i.Hi.Imm11 & 0x7fc) == 0x03c));
}

bool Vu::Recompiler::isADDAi(Vu::Instruction::Format64 i)
{
	// ADDAI
	return ((i.Hi.Imm11 == 0x23e));
}
bool Vu::Recompiler::isADDAq(Vu::Instruction::Format64 i)
{
	// ADDAQ
	return ((i.Hi.Imm11 == 0x23c));
}
bool Vu::Recompiler::isADDABC(Vu::Instruction::Format64 i)
{
	// ADDABC
	return (((i.Hi.Imm11 & 0x7fc) == 0x03c));
}

bool Vu::Recompiler::isSUB(Vu::Instruction::Format64 i)
{
	// SUB,SUBI,SUBQ,SUBBC
	return ((i.Hi.Funct == 0x2c) || (i.Hi.Funct == 0x26) || (i.Hi.Funct == 0x24) || ((i.Hi.Funct & 0x3c) == 0x04));
}

bool Vu::Recompiler::isSUBi(Vu::Instruction::Format64 i)
{
	// SUBI
	return ((i.Hi.Funct == 0x26));
}
bool Vu::Recompiler::isSUBq(Vu::Instruction::Format64 i)
{
	// SUBQ
	return ((i.Hi.Funct == 0x24));
}
bool Vu::Recompiler::isSUBBC(Vu::Instruction::Format64 i)
{
	// SUBBC
	return (((i.Hi.Funct & 0x3c) == 0x04));
}

bool Vu::Recompiler::isSUBA(Vu::Instruction::Format64 i)
{
	// SUBA,SUBAI,SUBAQ,SUBABC
	return ((i.Hi.Imm11 == 0x2fc) || (i.Hi.Imm11 == 0x27e) || (i.Hi.Imm11 == 0x27c) || ((i.Hi.Imm11 & 0x7fc) == 0x07c));
}

bool Vu::Recompiler::isSUBAi(Vu::Instruction::Format64 i)
{
	// SUBAI
	return ((i.Hi.Imm11 == 0x27e));
}
bool Vu::Recompiler::isSUBAq(Vu::Instruction::Format64 i)
{
	// SUBAQ
	return ((i.Hi.Imm11 == 0x27c));
}
bool Vu::Recompiler::isSUBABC(Vu::Instruction::Format64 i)
{
	// SUBABC
	return (((i.Hi.Imm11 & 0x7fc) == 0x07c));
}


bool Vu::Recompiler::isMUL(Vu::Instruction::Format64 i)
{
	// MUL,MULI,MULQ,MULBC
	return ((i.Hi.Funct == 0x2a) || (i.Hi.Funct == 0x1e) || (i.Hi.Funct == 0x1c) || ((i.Hi.Funct & 0x3c) == 0x18));
}

bool Vu::Recompiler::isMULi(Vu::Instruction::Format64 i)
{
	// MULI
	return ((i.Hi.Funct == 0x1e));
}

bool Vu::Recompiler::isMULq(Vu::Instruction::Format64 i)
{
	// MULQ
	return ((i.Hi.Funct == 0x1c));
}

bool Vu::Recompiler::isMULBC(Vu::Instruction::Format64 i)
{
	// MULBC
	return (((i.Hi.Funct & 0x3c) == 0x18));
}

bool Vu::Recompiler::isMULA(Vu::Instruction::Format64 i)
{
	// MULA,MULAI,MULAQ,MULABC,OPMULA
	return ((i.Hi.Imm11 == 0x2be) || (i.Hi.Imm11 == 0x1fe) || (i.Hi.Imm11 == 0x1fc) || ((i.Hi.Imm11 & 0x7fc) == 0x1bc) || (i.Hi.Imm11 == 0x2fe));
}

bool Vu::Recompiler::isMULAi(Vu::Instruction::Format64 i)
{
	// MULAI
	return ((i.Hi.Imm11 == 0x1fe));
}

bool Vu::Recompiler::isMULAq(Vu::Instruction::Format64 i)
{
	// MULAQ
	return ((i.Hi.Imm11 == 0x1fc));
}

bool Vu::Recompiler::isMULABC(Vu::Instruction::Format64 i)
{
	// MULABC
	return (((i.Hi.Imm11 & 0x7fc) == 0x1bc));
}


bool Vu::Recompiler::isMADD(Vu::Instruction::Format64 i)
{
	// MADD,MADDI,MADDQ,MADDBC
	return ((i.Hi.Funct == 0x29) || (i.Hi.Funct == 0x23) || (i.Hi.Funct == 0x21) || ((i.Hi.Funct & 0x3c) == 0x08));
}

bool Vu::Recompiler::isMADDi(Vu::Instruction::Format64 i)
{
	// MADDI
	return ((i.Hi.Funct == 0x23));
}
bool Vu::Recompiler::isMADDq(Vu::Instruction::Format64 i)
{
	// MADDQ
	return ((i.Hi.Funct == 0x21));
}
bool Vu::Recompiler::isMADDBC(Vu::Instruction::Format64 i)
{
	// MADDBC
	return (((i.Hi.Funct & 0x3c) == 0x08));
}

bool Vu::Recompiler::isMADDA(Vu::Instruction::Format64 i)
{
	// MADDA,MADDAI,MADDAQ,MADDABC
	return ((i.Hi.Imm11 == 0x2bd) || (i.Hi.Imm11 == 0x23f) || (i.Hi.Imm11 == 0x23d) || ((i.Hi.Imm11 & 0x7fc) == 0x0bc));
}

bool Vu::Recompiler::isMADDAi(Vu::Instruction::Format64 i)
{
	// MADDAI
	return ((i.Hi.Imm11 == 0x23f));
}
bool Vu::Recompiler::isMADDAq(Vu::Instruction::Format64 i)
{
	// MADDAQ
	return ((i.Hi.Imm11 == 0x23d));
}
bool Vu::Recompiler::isMADDABC(Vu::Instruction::Format64 i)
{
	// MADDABC
	return (((i.Hi.Imm11 & 0x7fc) == 0x0bc));
}

bool Vu::Recompiler::isMSUB(Vu::Instruction::Format64 i)
{
	// MSUB,MSUBI,MSUBQ,MSUBBC,OPMSUB
	return ((i.Hi.Funct == 0x2d) || (i.Hi.Funct == 0x27) || (i.Hi.Funct == 0x25) || ((i.Hi.Funct & 0x3c) == 0x0c) || (i.Hi.Funct == 0x2e));
}

bool Vu::Recompiler::isMSUBi(Vu::Instruction::Format64 i)
{
	// MSUBI
	return ((i.Hi.Funct == 0x27));
}
bool Vu::Recompiler::isMSUBq(Vu::Instruction::Format64 i)
{
	// MSUBQ
	return ((i.Hi.Funct == 0x25));
}
bool Vu::Recompiler::isMSUBBC(Vu::Instruction::Format64 i)
{
	// MSUBBC
	return (((i.Hi.Funct & 0x3c) == 0x0c));
}


bool Vu::Recompiler::isMSUBA(Vu::Instruction::Format64 i)
{
	// MSUBA,MSUBAI,MSUBAQ,MSUBABC
	return ((i.Hi.Imm11 == 0x2fd) || (i.Hi.Imm11 == 0x27f) || (i.Hi.Imm11 == 0x27d) || ((i.Hi.Imm11 & 0x7fc) == 0x0fc));
}

bool Vu::Recompiler::isMSUBAi(Vu::Instruction::Format64 i)
{
	// MSUBAI
	return ((i.Hi.Imm11 == 0x27f));
}
bool Vu::Recompiler::isMSUBAq(Vu::Instruction::Format64 i)
{
	// MSUBAQ
	return ((i.Hi.Imm11 == 0x27d));
}
bool Vu::Recompiler::isMSUBABC(Vu::Instruction::Format64 i)
{
	// MSUBABC
	return (((i.Hi.Imm11 & 0x7fc) == 0x0fc));
}


bool Vu::Recompiler::isOPMULA(Vu::Instruction::Format64 i)
{
	// OPMULA
	return ((i.Hi.Imm11 == 0x2fe));
}

bool Vu::Recompiler::isOPMSUB(Vu::Instruction::Format64 i)
{
	// OPMSUB
	return ((i.Hi.Funct == 0x2e));
}


// check if is a E unit function like ESUM,etc, excluding WAITP
bool Vu::Recompiler::isEType(Vu::Instruction::Format64 i)
{
	// ESUM,ESIN,EEXP,etc, excluding WAITP
	return ((!i.Hi.I) && (i.Lo.Opcode == 0x40) && ((i.Lo.Imm15_0 & 0x700) == 0x700) && (i.Lo.Imm15_0 != 0x7bf));
}

// check if it is an add/sub type instruction
bool Vu::Recompiler::isAType(Vu::Instruction::Format64 i)
{
	return (isADD(i) || isSUB(i) || isADDA(i) || isSUBA(i));
}

// check if it is an mul/madd/msub/opmula/opmsub type instruction
bool Vu::Recompiler::isMType(Vu::Instruction::Format64 i)
{
	return (isMUL(i) || isMULA(i) || isMADD(i) || isMSUB(i) || isMADDA(i) || isMSUBA(i));
}


bool Vu::Recompiler::isIntCalc ( Vu::Instruction::Format64 i )
{
	// iadd,iaddi,iaddiu,iand,ior,isub,isubu,mtir
	// iadd (funct=0x30), iaddi (funct=0x32), iand(funct=0x34), ior(funct=0x35),isub(funct=0x31)
	// iaddiu (op=0x08), isubiu (op=0x09)
	// mtir (imm11=0x3fc)
	if ( i.Hi.I ) return false;

	// iaddiu, isubiu
	if ( ( i.Lo.Opcode >= 0x08 ) && ( i.Lo.Opcode <= 0x09 ) ) return true;

	if ( i.Lo.Opcode != 0x40 ) return false;

	// iadd, isub, iaddi
	if ( ( i.Lo.Funct >= 0x30 ) && ( i.Lo.Funct <= 0x32 ) ) return true;

	// iand, ior
	if ( ( i.Lo.Funct >= 0x34 ) && ( i.Lo.Funct <= 0x35 ) ) return true;

	// mtir
	if ( i.Lo.Imm11 == 0x3fc ) return true;

	// optionally, lqi/lqd/sqi/sqd //

	// lqd, lqi also updates is
	//if ( ( i.Lo.Imm11 == 0x37e ) || ( i.Lo.Imm11 == 0x37c ) ) return true;
	// sqd, sqi also updates it
	//if ( ( i.Lo.Imm11 == 0x37f ) || ( i.Lo.Imm11 == 0x37d ) ) return true;

	return false;
}


void Vu::Recompiler::StaticAnalysis_HazardCheck(
	VU* v, u32 ulInstCount_3, u32 ulInstCount_2, u32 ulInstCount_1,
	VU::Bitmap128 FSrc_0, VU::Bitmap128 FDst_1, VU::Bitmap128 FDst_2, VU::Bitmap128 FDst_3,
	u64 ISrc_0, u64 IDst_1, u64 IDst_2, u64 IDst_3
	)
{
	Vu::Instruction::Format64 oInst64_1 = { v->MicroMem64[ulInstCount_1 & 0x7ff] };
	Vu::Instruction::Format64 oInst64_2 = { v->MicroMem64[ulInstCount_2 & 0x7ff] };
	Vu::Instruction::Format64 oInst64_3 = { v->MicroMem64[ulInstCount_3 & 0x7ff] };

	// check for conflict between source regs and destination regs

	if ((FSrc_0.b0 & FDst_1.b0 & ~0xfull) || (FSrc_0.b1 & FDst_1.b1))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_1 & 0x7ff] |= STATIC_HAZARD_RAW_1;
	}
	else if ((FSrc_0.b0 & FDst_2.b0 & ~0xfull) || (FSrc_0.b1 & FDst_2.b1))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_2 & 0x7ff] |= STATIC_HAZARD_RAW_2;
	}
	else if ((FSrc_0.b0 & FDst_3.b0 & ~0xfull) || (FSrc_0.b1 & FDst_3.b1))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_3 & 0x7ff] |= STATIC_HAZARD_RAW_3;
	}


	// check for int conflict //

	if ((((ISrc_0 & IDst_1) >> 32) & ~1ull) && isIntLoad(oInst64_1))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_1 & 0x7ff] |= STATIC_HAZARD_INT_RAW;
	}
	else if ((((ISrc_0 & IDst_2) >> 32) & ~1ull) && isIntLoad(oInst64_2))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_2 & 0x7ff] |= STATIC_HAZARD_INT_RAW;
	}
	else if ((((ISrc_0 & IDst_3) >> 32) & ~1ull) && isIntLoad(oInst64_3))
	{
		// set that there is a conflict //
		LUT_StaticInfo[ulInstCount_3 & 0x7ff] |= STATIC_HAZARD_INT_RAW;
	}
}

void Vu::Recompiler::StaticAnalysis_FlagCheck(VU* v, u32 ulInstCount_4,u32 ulInstCount_3,u32 ulInstCount_2,u32 ulInstCount_1,u32 ulInstCount_0)
{
	Vu::Instruction::Format64 oInst64_0 = { v->MicroMem64[ulInstCount_0 & 0x7ff] };
	if (isMacFlagCheck(oInst64_0))
	{
		// mark all to save flag history
		LUT_StaticInfo[ulInstCount_1 & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
		LUT_StaticInfo[ulInstCount_2 & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
		LUT_StaticInfo[ulInstCount_3 & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
		LUT_StaticInfo[ulInstCount_4 & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
	}

	if (isStatFlagCheck(oInst64_0))
	{
		LUT_StaticInfo[ulInstCount_1 & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
		LUT_StaticInfo[ulInstCount_2 & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
		LUT_StaticInfo[ulInstCount_3 & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
		LUT_StaticInfo[ulInstCount_4 & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
	}

	if (isClipFlagCheck(oInst64_0))
	{
		LUT_StaticInfo[ulInstCount_1 & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
		LUT_StaticInfo[ulInstCount_2 & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
		LUT_StaticInfo[ulInstCount_3 & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
		LUT_StaticInfo[ulInstCount_4 & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
	}
}


bool Vu::Recompiler::Test_StaticParallel(u32 ulInstCount, int i3, int i2, int i1, int i0)
{
	if (
		// make sure the instructions are not already set to parallel (first one in group would be parallel)
		!(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_PARALLEL_EXE_MASK)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_PARALLEL_EXE_MASK)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_PARALLEL_EXE_MASK)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_PARALLEL_EXE_MASK)

		// make sure a branch does not cut off the instructions
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)

		// make sure there is no e-bit that cuts off the instructions
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)

		// make sure there is no float conflicts between the instructions
		// needs to be checked separately
		// && !((LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)

		// also, for now, make sure flag summary is not disabled
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)

		// make sure there is no reverse exe, which would indicate some sort of upper vs lower conflict
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_REVERSE_EXE_ORDER)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_REVERSE_EXE_ORDER)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_REVERSE_EXE_ORDER)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_REVERSE_EXE_ORDER)

		// also make sure no other upper vs lower conflict with move
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & STATIC_COMPLETE_FLOAT_MOVE)
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & STATIC_COMPLETE_FLOAT_MOVE)
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & STATIC_COMPLETE_FLOAT_MOVE)
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & STATIC_COMPLETE_FLOAT_MOVE)


		// make sure the flag isn't getting checked for now
		&& !(LUT_StaticInfo[(ulInstCount - i3) & 0x7ff] & (STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP))
		&& !(LUT_StaticInfo[(ulInstCount - i2) & 0x7ff] & (STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP))
		&& !(LUT_StaticInfo[(ulInstCount - i1) & 0x7ff] & (STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP))
		&& !(LUT_StaticInfo[(ulInstCount - i0) & 0x7ff] & (STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP))

		/*
		// testing
		// && (isNopLo(oInst64[3].Lo) && isNopLo(oInst64[2].Lo))
		// && ((ulAddress > 0x0400) && (ulAddress < 0x0500))

		// also make sure there are no flag checks or sets for now
		&& !(isMacFlagCheck(oInst64[3]) || isStatFlagCheck(oInst64[3]) || isClipFlagCheck(oInst64[3]) || isStatFlagSetLo(oInst64[3]) || isClipFlagSetLo(oInst64[3]))
		&& !(isMacFlagCheck(oInst64[2]) || isStatFlagCheck(oInst64[2]) || isClipFlagCheck(oInst64[2]) || isStatFlagSetLo(oInst64[2]) || isClipFlagSetLo(oInst64[2]))
		&& !(isMacFlagCheck(oInst64[1]) || isStatFlagCheck(oInst64[1]) || isClipFlagCheck(oInst64[1]) || isStatFlagSetLo(oInst64[1]) || isClipFlagSetLo(oInst64[1]))
		&& !(isMacFlagCheck(oInst64[0]) || isStatFlagCheck(oInst64[0]) || isClipFlagCheck(oInst64[0]) || isStatFlagSetLo(oInst64[0]) || isClipFlagSetLo(oInst64[0]))
		// && !(isMacFlagCheck(oNextInst64) || isStatFlagCheck(oNextInst64) || isClipFlagCheck(oNextInst64) || isStatFlagSetLo(oNextInst64) || isClipFlagSetLo(oNextInst64))

		// **IMPORTANT NOTE** this only applies to avx2, but shouldn't apply to avx512
		&& !(IDstHi[i3] & ISrcHi[i2])

		// make sure no I-bit at least for now
		&& !(oInst64[3].Hi.I || oInst64[2].Hi.I || oInst64[1].Hi.I || oInst64[0].Hi.I)

		// also make sure no m-bit
		&& !(oInst64[3].Hi.M || oInst64[2].Hi.M || oInst64[1].Hi.M || oInst64[0].Hi.M)

		// also no q-update (like mulq,addq,etc)
		&& !(isQUpdate(oInst64[3]) || isQUpdate(oInst64[2]) || isQUpdate(oInst64[1]) || isQUpdate(oInst64[0]))

		// for now, E-unit instructions in the lower part (reverts to level-0)
		&& !(isEType(oInst64[3]) || isEType(oInst64[2]) || isEType(oInst64[1]) || isEType(oInst64[0]))
		*/
		
		)
	{
		return true;
	}

	return false;
}


void Vu::Recompiler::StaticAnalysis(VU* v)
{
	//cout << "\nVu::Recompiler::StaticAnalysis(VU* v)";

	// todo:
	// what if jr/jalr register is modified in jump delay slot
	// what if fcset or fsset instruction paired with clip or stat setting instruction, which happens first or not?

	// current instruction
	Vu::Instruction::Format64 oInst64[4];

	// pointer into memory
	u32* pSrcCodePtr;

	// points to the base of the code (index 0)
	u32* pSrcCodePtrBase;

	// the current address
	u32 ulAddress;

	// the count of instructions
	u32 ulInstCount;

	// the max number of instructions
	u32 ulInstMax;


	// size of the memory divided by 8 bytes per instruction is the number of instructions
	ulInstMax = v->ulVuMem_Size >> 3;

	// circular buffer for FDST bitmap
	VU::Bitmap128 FDst4[4];
	VU::Bitmap128 FSrc4[4];
	u64 IDst4[4];
	u64 ISrc4[4];

	VU::Bitmap128 FDstLo[4];
	VU::Bitmap128 FDstHi[4];
	VU::Bitmap128 FDst[4];

	VU::Bitmap128 FSrcLo[4];
	VU::Bitmap128 FSrcHi[4];
	VU::Bitmap128 FSrc[4];

	u64 IDstLo[4];
	u64 IDstHi[4];
	u64 IDst[4];

	u64 ISrcLo[4];
	u64 ISrcHi[4];
	u64 ISrc[4];

	//VU::Bitmap128 bmFConflictBmp;
	//u64 IConflictBmp;

	int iIdx;

	// address based indexes for previous 3 instructions
	int iIdx0, iIdx1, iIdx2, iIdx3;

	// address offset
	int iAddrIdx0, iAddrIdx1, iAddrIdx2, iAddrIdx3;

	// the q/p wait possibilities
	int wait0, wait1, wait2;
	int iCount;

	int iLastConflict;

	// just want the usage/dependancy bitmaps from the recompiler
	//v->OpLevel = -1;

#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
	VU::debug << "\r\n" << hex << "VU#" << v->Number << " ***STATIC-ANALYSIS-START***\r\n";
#endif

	// clear all delays
	//memset( LUT_StaticDelay, 0, sizeof( LUT_StaticDelay ) );

	// clear all flags and data (note: does this below)
	//memset( LUT_StaticFlags, 0, sizeof( LUT_StaticFlags ) );
	//memset( LUT_StaticInfo, 0, sizeof( LUT_StaticInfo ) );

	// start address at zero
	ulAddress = 0;

	// instruction count starts at zer
	ulInstCount = 0;


	// clear the circular buffers
	memset(FDstHi, 0, sizeof(FDstHi));
	memset(FSrcHi, 0, sizeof(FSrcHi));
	memset(FDstLo, 0, sizeof(FDstLo));
	memset(FSrcLo, 0, sizeof(FSrcLo));
	memset(FDst, 0, sizeof(FDst));
	memset(FSrc, 0, sizeof(FSrc));
	memset(IDstHi, 0, sizeof(IDstHi));
	memset(ISrcHi, 0, sizeof(ISrcHi));
	memset(IDstLo, 0, sizeof(IDstLo));
	memset(ISrcLo, 0, sizeof(ISrcLo));
	memset(IDst, 0, sizeof(IDst));
	memset(ISrc, 0, sizeof(ISrc));

	// clear current/previous instructions before starting
	memset(oInst64, 0, sizeof(oInst64));

	// clear static info array first, because might traverse branches to mark conflicts,etc
	memset(LUT_StaticInfo, 0, sizeof(LUT_StaticInfo));

	// get pointer into VU instruction memory device
	// MicroMem32
	pSrcCodePtr = &(v->MicroMem32[0]);

	// will also need the base pointer to get instructions from known branches
	pSrcCodePtrBase = pSrcCodePtr;


	while (ulAddress < v->ulVuMem_Size)
	{

		oInst64[0].Lo.Value = *pSrcCodePtr++;
		oInst64[0].Hi.Value = *pSrcCodePtr++;


		// clear info flags first thing for current instruction //
		// note: this is cleared first thing above
		//LUT_StaticInfo [ ulInstCount ] = 0;


		// get the bitmaps //

		getDstFieldMapHi(FDstHi[0], oInst64[0]);
		getSrcFieldMapHi(FSrcHi[0], oInst64[0]);

		IDstHi[0] = getDstRegMapHi(oInst64[0]);
		ISrcHi[0] = getSrcRegMapHi(oInst64[0]);

		if (!oInst64[0].Hi.I)
		{
			getDstFieldMapLo(FDstLo[0], oInst64[0]);
			getSrcFieldMapLo(FSrcLo[0], oInst64[0]);
			IDstLo[0] = getDstRegMapLo(oInst64[0]);
			ISrcLo[0] = getSrcRegMapLo(oInst64[0]);
		}
		else
		{
			FDstLo[0].b0 = 0;
			FDstLo[0].b1 = 0;
			FSrcLo[0].b0 = 0;
			FSrcLo[0].b1 = 0;
			IDstLo[0] = 0;
			ISrcLo[0] = 0;
		}

		// remove zero registers
		FDstHi[0].b0 &= ~0xfull;
		FDstLo[0].b0 &= ~0xfull;
		FSrcHi[0].b0 &= ~0xfull;
		FSrcLo[0].b0 &= ~0xfull;
		IDstHi[0] &= ~((1ull << 32) | 1ull);
		IDstLo[0] &= ~((1ull << 32) | 1ull);
		ISrcHi[0] &= ~((1ull << 32) | 1ull);
		ISrcLo[0] &= ~((1ull << 32) | 1ull);

		// combine
		FDst[0].b0 = FDstHi[0].b0 | FDstLo[0].b0;
		FDst[0].b1 = FDstHi[0].b1 | FDstLo[0].b1;
		FSrc[0].b0 = FSrcHi[0].b0 | FSrcLo[0].b0;
		FSrc[0].b1 = FSrcHi[0].b1 | FSrcLo[0].b1;
		IDst[0] = IDstHi[0] | IDstLo[0];
		ISrc[0] = ISrcHi[0] | ISrcLo[0];



		// check for some important conditions //

#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
		VU::debug << "\r\n";
#endif




		// make sure lower instruction is not an immediate
		if (!oInst64[0].Hi.I)
		{
			// for the flag setting instructions (fcset/fsset), reverse exe order so that they overwrite upper instruction flags
			// note: lower flag setting instructions overwrite flag for the upper flag setting instruction
			if (isStatFlagSetLo(oInst64[0]) || isClipFlagSetLo(oInst64[0]))
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">FLAG-SET(REVERSE-EXE-ORDER)";
#endif

#ifdef VERBOSE_STATIC_FLAG_SET
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: *** FLAG-SET *** Address=" << hex << ulAddress;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif

				// reverse exe order so flag gets overwritten by the lower flag setting instruction
				LUT_StaticInfo[ulInstCount] |= STATIC_REVERSE_EXE_ORDER;

				// if the stat flag is set, then mark since the upper instruction no int32_ter needs to output the stat flag
				// note: the clip instruction can just check for the lower fcset instruction for now (if not an immediate)
				if (isStatFlagSetLo(oInst64[0]))
				{
					LUT_StaticInfo[ulInstCount] |= STATIC_FLAG_SET_STAT;
				}

			}

			// if both hi and lo instructions output to same float reg, then ignore lower instruction //
			// note: but if lower instruction is LQI/LQD, then alert to console
			// have no idea if integer reg should increment/decrement if float result is discarded
			// note: ignore f0 register
			if (getDstRegMapHi(oInst64[0]) & getDstRegMapLo(oInst64[0]) & 0xfffffffeull)
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">IGNORE-LOWER(OVERWRITE)";
#endif

				// ignore lower instruction, higher one takes precedence
				LUT_StaticInfo[ulInstCount] |= STATIC_SKIP_EXEC_LOWER;

				// check for lqi/lqd
				if ((oInst64[0].Lo.Opcode == 0x40) && ((oInst64[0].Lo.Imm11 == 0x37c) || (oInst64[0].Lo.Imm11 == 0x37e)))
				{
					cout << "\nhps2x64: VU: STATIC-ANALYSIS: ALERT: LQI/LQD output to same float reg as upper. Instruction cancelled ??\n";
				}
			}
			else
			{
				// check if lq,lqd,lqi,rget,rnext,mfir,mfp (move with float destination but NON-FLOAT source register(s))
				if (!oInst64[0].Lo.Opcode
					|| (
						(oInst64[0].Lo.Opcode == 0x40)
						&& (oInst64[0].Lo.Imm11 == 0x37e || oInst64[0].Lo.Imm11 == 0x37c || oInst64[0].Lo.Imm11 == 0x3fd || oInst64[0].Lo.Imm15_0 == 0x67c || oInst64[0].Lo.Imm15_0 == 0x43d || oInst64[0].Lo.Imm15_0 == 0x43c)
						)
					)
				{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
					VU::debug << ">CHANGE-EXE-ORDER";
#endif

					// swap execution order for lq,lqd,lqi,rget,rnext,mfir,mfp
					LUT_StaticInfo[ulInstCount] |= STATIC_REVERSE_EXE_ORDER;
				}


				// otherwise, if this is a move type instruction between float regs then mark it
				// since it has to read the source but only store to dst at end of upper instruction
				// since the destination of the move could be the source in the upper instruction
				// move, mr32, mfir, mfp, rget, rnext, lq, lqd, lqi
				// actually, just for move or mr32 (float to float move)
				if (isMoveToFloatFromFloatLo(oInst64[0]))
				{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
					VU::debug << ">PULL-FROM-LOAD-DELAY-SLOT";
#endif
					// mark that need to pull data from load delay slot at end
					//LUT_StaticInfo [ ulInstCount ] |= ( 1 << 5 );

					// if upper source is in lower destination AND upper destination is in lower source
					// then need to use temp variable for move
					if (((FSrcHi[0].b0 & FDstLo[0].b0 & ~0xfull) || (FSrcHi[0].b1 & FDstLo[0].b1))
						&& ((FDstHi[0].b0 & FSrcLo[0].b0 & ~0xfull) || (FDstHi[0].b1 & FSrcLo[0].b1))
						)
					{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
						VU::debug << ">PULL-FROM-LOAD-DELAY-SLOT";
#endif

						// mark that need to pull data from load delay slot at end
						LUT_StaticInfo[ulInstCount] |= STATIC_COMPLETE_FLOAT_MOVE;
					}
					// otherwise check if bitmap from upper source conflicts with lower destination
					else if ((FSrcHi[0].b0 & FDstLo[0].b0 & ~0xfull) || (FSrcHi[0].b1 & FDstLo[0].b1))
					{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
						VU::debug << ">CHANGE-EXE-ORDER";
#endif

						// here just swap execution order (analysis bit 20) //
						// *** testing ***
						LUT_StaticInfo[ulInstCount] |= STATIC_REVERSE_EXE_ORDER;

					}

				}	// end else if ( isMoveToFloatFromFloatLo ( oInst64[0] ) )
			}


			// check if previous instruction was in branch delay slot
			if (isBranchOrJump(oInst64[2]))
			{
				// previous instruction was branch delay slot //

				// check if previous instruction was int-calc in branch delay slot
				// not including lqi/lqd/sqi/sqd here
				if (isIntCalc(oInst64[1]))
				{
					// previous lo instruction should output int result to delay slot when in branch delay slot
					// IMPORTANT: it is possible that when program lands after the jump that it is either on an int calc or a branch that uses the int register (two completely different cases)
					// todo: possible solution is to ALWAYS check delay slot BEFORE int instruction and only check delay slot AFTER branch instruction
					//LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_MOVE_TO_INT_DELAY_SLOT;

					// which would make this the int delay slot after which we load the value into the vu int register
					//LUT_StaticInfo[ulInstCount] |= STATIC_MOVE_FROM_INT_DELAY_SLOT;

#ifdef VERBOSE_INT_REG_DELAY_SLOT
				// alert that integer reg is not being stored in branch delay slot
					cout << "\nhps2x64: VU: STATIC-ANALYSIS: Int reg not written back in branch delay slot. Address=" << hex << ulAddress;
					//cout << "\n" << Print::PrintInstructionHI ( oInst64[3].Hi.Value ).c_str () << " " << hex << oInst64[3].Hi.Value;
					//cout << "\n" << Print::PrintInstructionLO ( oInst64[3].Lo.Value ).c_str () << " " << hex << oInst64[3].Lo.Value;
					cout << "\n" << Print::PrintInstructionHI(oInst64[2].Hi.Value).c_str() << " " << hex << oInst64[2].Hi.Value;
					cout << "\n" << Print::PrintInstructionLO(oInst64[2].Lo.Value).c_str() << " " << hex << oInst64[2].Lo.Value;
					cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
					cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
					cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
					cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
				}
			}


			// check for integer reg delay slot conflict if this is a conditional branch //
			if (isConditionalBranch(oInst64[0]))
			{
				// conditional branch //

				// conditional branch start
				//LUT_StaticInfo[ulInstCount] |= STATIC_COND_BRANCH_START;

				// check if integer register src for branch conflicts with last integer dst //
				if (((getSrcRegMapLo(oInst64[0]) & getDstRegMapLo(oInst64[1])) >> 32) & ~1)
				{
					// finally, make sure the previous lower instruction was not a flag check instruction
					// make sure previous instruction was an integer calculation or mtir
					//if ( !( isMacFlagCheck ( oInst64[1] ) || isStatFlagCheck ( oInst64[1] ) || isClipFlagCheck ( oInst64[1] ) ) )
					if (isIntCalc(oInst64[1]))
					{
						// only do this for int calc, or mtir
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
						VU::debug << ">INTREG-SKIPS-BRANCH";
#endif
						// previous lo instruction should output int result to delay slot
						LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_MOVE_TO_INT_DELAY_SLOT;

						// current instruction should pull int reg from delay slot at end if instr# not zero
						LUT_StaticInfo[ulInstCount] |= STATIC_MOVE_FROM_INT_DELAY_SLOT;


						// check if previous-previous instruction is e-bit
						if (oInst64[2].Hi.E)
						{
							// alert that e-bit delay slot outputs to int delay slot
							cout << "\nhps2x64: VU: STATIC-ANALYSIS: Int reg not written back in e-bit delay slot.\n";
						}

					}	// end if ( isIntCalc ( oInst64[1] ) )

					// check if previous is lqi or lqd (is)
					if ((oInst64[1].Lo.Opcode == 0x40) && ((oInst64[1].Lo.Imm11 == 0x37c) || (oInst64[1].Lo.Imm11 == 0x37e)))
					{
						// check if is of previous is equal to it of next
						if (oInst64[1].Lo.is == oInst64[0].Lo.it)
						{
							// appears this is an issue ?? //

							// previous lo instruction should output int result to delay slot
							LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_MOVE_TO_INT_DELAY_SLOT;

							// current instruction should pull int reg from delay slot at end if instr# not zero
							LUT_StaticInfo[ulInstCount] |= STATIC_MOVE_FROM_INT_DELAY_SLOT;

#ifdef VERBOSE_LQI_LQD_DELAY_SLOT
							cout << "\nhps2x64: VU: STATIC-ANALYSIS: lqi/lqd delay slot with branch. Address=" << hex << ulAddress;
							cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
							cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
							cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
							cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
						}
					}


					// check if previous is sqi or sqd (it)
					if ((oInst64[1].Lo.Opcode == 0x40) && ((oInst64[1].Lo.Imm11 == 0x37d) || (oInst64[1].Lo.Imm11 == 0x37f)))
					{
						// check if is of previous it equal to is of next
						if (oInst64[1].Lo.it == oInst64[0].Lo.is)
						{
							// appears this is an issue ?? //

							// previous lo instruction should output int result to delay slot
							LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_MOVE_TO_INT_DELAY_SLOT;

							// current instruction should pull int reg from delay slot at end if instr# not zero
							LUT_StaticInfo[ulInstCount] |= STATIC_MOVE_FROM_INT_DELAY_SLOT;

#ifdef VERBOSE_SQI_SQD_DELAY_SLOT
							cout << "\nhps2x64: VU: STATIC-ANALYSIS: sqi/sqd delay slot with branch. Address=" << hex << ulAddress;
							cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
							cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
							cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
							cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
						}
					}
				}

			}	// end if ( isConditionalBranch( oInst64[0] ) )


			// check if unconditional jump //
			if (isUnconditionalBranchOrJump(oInst64[0]))
			{
				// unconditional branch //

				// mark as unconditional branch/jump
				//LUT_StaticInfo[ulInstCount] |= STATIC_UNCOND_BRANCH_START;

				// check if integer register src for branch conflicts with last integer dst //
				if (((getSrcRegMapLo(oInst64[0]) & getDstRegMapLo(oInst64[1])) >> 32) & 0xffff)
				{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
					VU::debug << ">INTREG-SKIPS-JUMP??";
#endif

#ifdef VERBOSE_INT_REG_BRANCH
					cout << "\nhps2x64: VU: STATIC-ANALYSIS: Unconditional Jump reads dst int reg from previous instruction.\n";
#endif
				}
			}

			// if lower instruction is mfp, then need updatep before //
			if (isPUpdate(oInst64[0]))
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">P-UPDATE";
#endif
				// update p (analysis bit 29)
				LUT_StaticInfo[ulInstCount] |= STATIC_UPDATE_BEFORE_P;
			}

			// check for pwait (analysis bit 28)
			// todo: exclude waitp instruction??
			if (isPWait(oInst64[0]))
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">P-WAIT";
#endif
				// wait q (analysis bit 28)
				LUT_StaticInfo[ulInstCount] |= STATIC_WAIT_BEFORE_P;
			}

			// check for qwait (analysis bit 27)
			// todo: exclude waitq instruction??
			if (isQWait(oInst64[0]))
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">Q-WAIT";
#endif
				// wait q (analysis bit 27)
				LUT_StaticInfo[ulInstCount] |= STATIC_WAIT_BEFORE_Q;
			}

		}	// end if ( !oInst64[0].Hi.I )


		// if upper instruction is addq,mulq,etc, then need updateq before (analysis bit 19) //
		if (isQUpdate(oInst64[0]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">Q-UPDATE";
#endif
			// update q (analysis bit 19)
			LUT_StaticInfo[ulInstCount] |= STATIC_UPDATE_BEFORE_Q;
		}


		// check if is flag check instruction //

		if (isMacFlagCheck(oInst64[0]))
		{
			// mark all to save flag history
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
			LUT_StaticInfo[(ulInstCount - 4) & 0x7ff] |= STATIC_FLAG_CHECK_MAC;
		}

		if (isStatFlagCheck(oInst64[0]))
		{
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
			LUT_StaticInfo[(ulInstCount - 4) & 0x7ff] |= STATIC_FLAG_CHECK_STAT;
		}

		if (isClipFlagCheck(oInst64[0]))
		{
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 4) & 0x7ff] |= STATIC_FLAG_CHECK_CLIP;
		}

		// check if branch delay slot //
		// check if previous instruction was a branch
		if (isBranchOrJump(oInst64[1]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">BRANCH-DELAY-SLOT";
#endif

			// mark all to save flag history
			LUT_StaticInfo[(ulInstCount - 0) & 0x7ff] |= STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP;
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_FLAG_CHECK_MAC | STATIC_FLAG_CHECK_STAT | STATIC_FLAG_CHECK_CLIP;

			// also set as conflicts
			LUT_StaticInfo[(ulInstCount - 0) & 0x7ff] |= STATIC_HAZARD_INT_RAW | STATIC_HAZARD_RAW_1;
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_HAZARD_INT_RAW | STATIC_HAZARD_RAW_2;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_HAZARD_INT_RAW | STATIC_HAZARD_RAW_3;


			if (isConditionalBranch(oInst64[1]))
			{
				// branch delay slot //
				// unsure if it matters if it is conditional or unconditional
				LUT_StaticInfo[ulInstCount] |= STATIC_COND_BRANCH_DELAY_SLOT;
			}
			else
			{
				// note: this could be an unconditional branch or jump
				LUT_StaticInfo[ulInstCount] |= STATIC_UNCOND_BRANCH_DELAY_SLOT;
			}

			// while we are at it, go ahead and check for a branch in the branch delay slot
			if (isBranchOrJump(oInst64[0]))
			{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
				VU::debug << ">BRANCH-IN-BRANCH-DELAY-SLOT";
#endif

#ifdef VERBOSE_BRANCH_IN_BRANCH_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: branch in branch delay slot!!!";
#endif

				LUT_StaticInfo[ulInstCount] |= STATIC_BRANCH_IN_BRANCH_DELAY_SLOT;
			}
		}


		// check if e-bit delay slot
		if (oInst64[1].Hi.E)
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">E-BIT-DELAY-SLOT";
#endif

			// e-bit delay slot //
			LUT_StaticInfo[ulInstCount] |= STATIC_EBIT_DELAY_SLOT;

			// todo?: clear branches in e-bit delay slot ??

			// check if there is an e-bit in delay slot
			if (oInst64[0].Hi.E)
			{
#ifdef VERBOSE_EBIT_IN_EBIT_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: e-bit in e-bit delay slot!!!\n";
#endif

				LUT_StaticInfo[ulInstCount] |= STATIC_PROBLEM_IN_EBIT_DELAY_SLOT;
			}

			// check if there is a branch in e-bit delay slot
			if (isBranchOrJump(oInst64[0]))
			{
#ifdef VERBOSE_BRANCH_IN_EBIT_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: BRANCH in e-bit delay slot!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif

				LUT_StaticInfo[ulInstCount] |= STATIC_PROBLEM_IN_EBIT_DELAY_SLOT;
			}

			// check if branch is with e-bit simultaneously
			if (isBranchOrJump(oInst64[1]))
			{
#ifdef VERBOSE_BRANCH_WITH_EBIT_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: BRANCH with e-bit!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}


			// check if there is a xgkick in e-bit delay slot
			if (isXgKick(oInst64[0]))
			{
#ifdef VERBOSE_XGKICK_IN_EBIT_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: XGKICK in e-bit delay slot!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif

				LUT_StaticInfo[ulInstCount] |= STATIC_PROBLEM_IN_EBIT_DELAY_SLOT;
			}

			// check if branch is with e-bit simultaneously
			if (isXgKick(oInst64[1]))
			{
#ifdef VERBOSE_XGKICK_WITH_EBIT_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: XGKICK with e-bit!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}

		}


		// check if xgkick delay slot
		if (isXgKick(oInst64[1]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">XGKICK-DELAY-SLOT";
#endif

			// xgkick delay slot (analysis bit 30) //
			LUT_StaticInfo[ulInstCount] |= STATIC_XGKICK_DELAY_SLOT;

			if (isXgKick(oInst64[0]))
			{
#ifdef VERBOSE_XGKICK_IN_XGKICK_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: xgkick in xgkick delay slot!!!\n";
#endif
			}


			// check if there is a branch in xgkick delay slot
			if (isBranchOrJump(oInst64[0]))
			{
#ifdef VERBOSE_BRANCH_IN_XGKICK_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: BRANCH in xgkick delay slot!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}

			// check if branch is with xgkick simultaneously
			if (isBranchOrJump(oInst64[1]))
			{
#ifdef VERBOSE_BRANCH_WITH_XGKICK_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: BRANCH with xgkick!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}


			// check if there is a e-bit in xgkick delay slot
			if (oInst64[0].Hi.E)
			{
#ifdef VERBOSE_EBIT_IN_XGKICK_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: E-BIT in xgkick delay slot!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}

			// check if e-bit is with xgkick simultaneously
			if (oInst64[1].Hi.E)
			{
#ifdef VERBOSE_EBIT_WITH_XGKICK_DELAY_SLOT
				cout << "\nhps2x64: VU: STATIC-ANALYSIS: E-BIT with xgkick!!!\n";
				cout << "\n" << Print::PrintInstructionHI(oInst64[1].Hi.Value).c_str() << " " << hex << oInst64[1].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[1].Lo.Value).c_str() << " " << hex << oInst64[1].Lo.Value;
				cout << "\n" << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << " " << hex << oInst64[0].Hi.Value;
				cout << "\n" << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << " " << hex << oInst64[0].Lo.Value;
#endif
			}

		}


		// check if clip flag check instruction //

		// if mac/stat/clip flag check instruction, then mark wait0,wait1,wait2 to output //
		// also mark mac/stat or clip flag0-flag3 to output //



		// if wait0 is 3+, then the flag comes from previous instruction
		// if wait0+wait1 is 2+, then the flag comes from instruction before that
		// if wait0+wait1+wait2 is 1+ (any of them set), then flag comes from instruction before that
		// otherwise flag comes from instruction before that


		// when branching, will clear the waits and fill the mac/stat/clip flags


		// check if mac flag check instruction //
		/*
		if ( isMacFlagCheck ( oInst64[0] ) )
		{
	#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
	VU::debug << ">MAC-FLAG-CHECK";
	#endif

			// mac flag check //

			// mark last 3 instructions to output wait/flag data
			// bit 18 - output cycle count
			// bit 1 - output mac flag
			LUT_StaticInfo [ ( ulInstCount - 1 ) & 0x7ff ] |= ( 1 << 1 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 2 ) & 0x7ff ] |= ( 1 << 1 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 3 ) & 0x7ff ] |= ( 1 << 1 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 4 ) & 0x7ff ] |= ( 1 << 1 );

			// mac flag check (analysis bit 21)
			LUT_StaticInfo [ ulInstCount ] |= ( 1 << 21 );

			cout << "\nhps2x64: VU: STATIC-ANALYSIS: mac flag check.\n";
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionHI ( oInst64[3].Hi.Value ).c_str () << " " << hex << oInst64[3].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionLO ( oInst64[3].Lo.Value ).c_str () << " " << hex << oInst64[3].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionHI ( oInst64[2].Hi.Value ).c_str () << " " << hex << oInst64[2].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionLO ( oInst64[2].Lo.Value ).c_str () << " " << hex << oInst64[2].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionHI ( oInst64[1].Hi.Value ).c_str () << " " << hex << oInst64[1].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionLO ( oInst64[1].Lo.Value ).c_str () << " " << hex << oInst64[1].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionHI ( oInst64[0].Hi.Value ).c_str () << " " << hex << oInst64[0].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionLO ( oInst64[0].Lo.Value ).c_str () << " " << hex << oInst64[0].Lo.Value;


		}
		*/

		// check if mac/stat flag setting instruction
		if (isMacStatFlagSetHi(oInst64[0]) || isStatFlagSetLo(oInst64[0]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">MAC-FLAG-SET";
#endif

			// mac flag set (analysis bit 1)
			//LUT_StaticInfo [ ulInstCount ] |= ( 1 << 1 );

		}


		// check if stat flag check instruction //
		/*
		if ( isStatFlagCheck ( oInst64[0] ) )
		{
	#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
	VU::debug << ">STAT-FLAG-CHECK";
	#endif

			// mark last 3 instructions to output wait/flag data
			// bit 18 - output cycle count
			// bit 2 - output stat flag
			LUT_StaticInfo [ ( ulInstCount - 1 ) & 0x7ff ] |= ( 1 << 2 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 2 ) & 0x7ff ] |= ( 1 << 2 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 3 ) & 0x7ff ] |= ( 1 << 2 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 4 ) & 0x7ff ] |= ( 1 << 2 );

			// stat flag check (analysis bit 22)
			LUT_StaticInfo [ ulInstCount ] |= ( 1 << 22 );
		}
		*/


		/*
		if ( isClipFlagCheck ( oInst64[0] ) )
		{
	#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
	VU::debug << ">CLIP-FLAG-CHECK";
	#endif

			// clip flag check //

			// mark last 3 instructions to output wait/flag data
			// bit 18 - output cycle count
			// bit 3 - output clip flag
			LUT_StaticInfo [ ( ulInstCount - 1 ) & 0x7ff ] |= ( 1 << 3 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 2 ) & 0x7ff ] |= ( 1 << 3 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 3 ) & 0x7ff ] |= ( 1 << 3 ) | ( 1 << 18 );
			LUT_StaticInfo [ ( ulInstCount - 4 ) & 0x7ff ] |= ( 1 << 3 );

			// clip flag check (analysis bit 23)
			LUT_StaticInfo [ ulInstCount ] |= ( 1 << 23 );

			cout << "\nhps2x64: VU: STATIC-ANALYSIS: clip flag check.\n";
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionHI ( oInst64[3].Hi.Value ).c_str () << " " << hex << oInst64[3].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionLO ( oInst64[3].Lo.Value ).c_str () << " " << hex << oInst64[3].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionHI ( oInst64[2].Hi.Value ).c_str () << " " << hex << oInst64[2].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionLO ( oInst64[2].Lo.Value ).c_str () << " " << hex << oInst64[2].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionHI ( oInst64[1].Hi.Value ).c_str () << " " << hex << oInst64[1].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionLO ( oInst64[1].Lo.Value ).c_str () << " " << hex << oInst64[1].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionHI ( oInst64[0].Hi.Value ).c_str () << " " << hex << oInst64[0].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionLO ( oInst64[0].Lo.Value ).c_str () << " " << hex << oInst64[0].Lo.Value;

		}
		*/

		// check if clip instruction or fcset instruction
		if ((oInst64[0].Hi.Imm11 == 0x1ff) || (oInst64[0].Lo.Opcode == 0x11))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">CLIP-FLAG-SET";
#endif

			// clip flag set (analysis bit 3)
			//LUT_StaticInfo[ulInstCount] |= (1 << 3);

			/*
			cout << "\nhps2x64: VU: STATIC-ANALYSIS: clip flag check.\n";
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionHI ( oInst64[3].Hi.Value ).c_str () << " " << hex << oInst64[3].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-24) << " " << Print::PrintInstructionLO ( oInst64[3].Lo.Value ).c_str () << " " << hex << oInst64[3].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionHI ( oInst64[2].Hi.Value ).c_str () << " " << hex << oInst64[2].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-16) << " " << Print::PrintInstructionLO ( oInst64[2].Lo.Value ).c_str () << " " << hex << oInst64[2].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionHI ( oInst64[1].Hi.Value ).c_str () << " " << hex << oInst64[1].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-8) << " " << Print::PrintInstructionLO ( oInst64[1].Lo.Value ).c_str () << " " << hex << oInst64[1].Lo.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionHI ( oInst64[0].Hi.Value ).c_str () << " " << hex << oInst64[0].Hi.Value;
			cout << "\nAddress:" << hex << (ulAddress-0) << " " << Print::PrintInstructionLO ( oInst64[0].Lo.Value ).c_str () << " " << hex << oInst64[0].Lo.Value;
			*/
		}




		// if this is a branch or jump, then for now assume it and anything surrounding it is a read-after-write hazard
		if (isBranchOrJump(oInst64[0]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-321-BRANCH";
#endif

			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_DISABLE_FLAG_SUMMARY;
			LUT_StaticInfo[ulInstCount] |= STATIC_DISABLE_FLAG_SUMMARY;
			LUT_StaticInfo[(ulInstCount + 1) & 0x7ff] |= STATIC_DISABLE_FLAG_SUMMARY;
		}


		// if this is a flag check then disable flag summary
		if (isMacFlagCheck(oInst64[0]) || isStatFlagCheck(oInst64[0]))
		{
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_DISABLE_FLAG_SUMMARY;
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_DISABLE_FLAG_SUMMARY;
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_DISABLE_FLAG_SUMMARY;
		}




		// check for conflict between source regs and destination regs

		if ((FSrc[0].b0 & FDst[1].b0 & ~0xfull) || (FSrc[0].b1 & FDst[1].b1))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-1";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_HAZARD_RAW_1;

		}

		else if ((FSrc[0].b0 & FDst[2].b0 & ~0xfull) || (FSrc[0].b1 & FDst[2].b1))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-2";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_HAZARD_RAW_2;

		}

		else if ((FSrc[0].b0 & FDst[3].b0 & ~0xfull) || (FSrc[0].b1 & FDst[3].b1))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-3";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_HAZARD_RAW_3;

		}


		// check for int conflict //

		if ((((ISrc[0] & IDst[1]) >> 32) & ~1ull) && isIntLoad(oInst64[1]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-1";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_HAZARD_INT_RAW;
		}
		else if ((((ISrc[0] & IDst[2]) >> 32) & ~1ull) && isIntLoad(oInst64[2]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-2";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_HAZARD_INT_RAW;
		}
		else if ((((ISrc[0] & IDst[3]) >> 32) & ~1ull) && isIntLoad(oInst64[3]))
		{
#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
			VU::debug << ">FLT-CONFLICT-PREV-3";
#endif

			// set that there is a conflict //
			LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_HAZARD_INT_RAW;
		}



		// check for instructions that can be run in parallel
#ifdef ENABLE_PARALLEL_VU_INSTS
		// make sure that the top instruction isn't already in a parallel block and isn't in a delay slot for branch or e-bit
		// also make sure no conflicts
		if (
			// make sure the instructions are not already set to parallel (first one in group would be parallel)
			!(LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_PARALLEL_EXE_MASK)

			// make sure a branch does not cut off the instructions
			&& !(LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)

			// make sure there is no e-bit that cuts off the instructions
			&& !(LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)

			// make sure there is no conflicts between the instructions
			&& !((LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)

			// also, for now, make sure flag summary is not disabled
			&& !(LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)
			&& !(LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)

			// also make sure there are no flag checks or sets for now
			&& !(isMacFlagCheck(oInst64[3]) || isStatFlagCheck(oInst64[3]) || isClipFlagCheck(oInst64[3]) || isStatFlagSetLo(oInst64[3]) || isClipFlagSetLo(oInst64[3]))
			&& !(isMacFlagCheck(oInst64[2]) || isStatFlagCheck(oInst64[2]) || isClipFlagCheck(oInst64[2]) || isStatFlagSetLo(oInst64[2]) || isClipFlagSetLo(oInst64[2]))

			// also make sure no m-bit
			&& !(oInst64[3].Hi.M || oInst64[2].Hi.M)

			// for now, E-unit instructions in the lower part (reverts to level-0)
			&& !(isEType(oInst64[3]) || isEType(oInst64[2]))
			)
		{
			// check if this is an x2 or x4 recompilation
			if ((iRecompilerWidth == RECOMPILER_WIDTH_AVX2X2) || (iRecompilerWidth == RECOMPILER_WIDTH_AVX512X2) || (iRecompilerWidth == RECOMPILER_WIDTH_AVX512X4))
			{
				if (
#ifdef ENABLE_PARALLEL_MIN_MARK
					// || (isMIN_NotI(oInst64[3]) && isMIN_NotI(oInst64[2]))
					(isMIN_NotI(oInst64[3]) && isMIN_NotI(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_MAX_MARK
					|| (isMAX_NotI(oInst64[3]) && isMAX_NotI(oInst64[2]))
#endif
					//(isMType(oInst64[3]) && isMType(oInst64[2])) 
#ifdef ENABLE_PARALLEL_ATYPE
					|| (isAType(oInst64[3]) && isAType(oInst64[2]))
					//(isAType(oInst64[3]) && isAType(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_MTYPE
				//|| ((isMADD(oInst64[3]) || isMADDA(oInst64[3])) && (isMADD(oInst64[2]) || isMADDA(oInst64[2])))
				//|| ((isMADD(oInst64[3]) || isMADDA(oInst64[3]) || isMSUB(oInst64[3]) || isMSUBA(oInst64[3]) || isOPMSUB(oInst64[3])) && (isMADD(oInst64[2]) || isMADDA(oInst64[2]) || isMSUB(oInst64[2]) || isMSUBA(oInst64[2]) || isOPMSUB(oInst64[2])))
				//|| ((isMUL(oInst64[3]) || isMULA(oInst64[3]) ) && (isMADD(oInst64[2]) || isMADDA(oInst64[2])))
					|| ((isMType(oInst64[3])) && (isMADD(oInst64[2]) || isMADDA(oInst64[2])))
#endif
#ifdef ENABLE_PARALLEL_MUL
					|| ((isMUL(oInst64[3]) || isMULA(oInst64[3])) && (isMUL(oInst64[2]) || isMULA(oInst64[2])))
#endif
#ifdef ENABLE_PARALLEL_CLIP
					|| (isCLIP(oInst64[3]) && isCLIP(oInst64[2]))
					//(isCLIP(oInst64[3]) && isCLIP(oInst64[2]))
#endif

				// todo: these instructions actually don't care about any flags
#ifdef ENABLE_PARALLEL_FTOI0
					|| (isFTOI0(oInst64[3]) && isFTOI0(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_FTOI4
					|| (isFTOI4(oInst64[3]) && isFTOI4(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_FTOI12
					|| (isFTOI12(oInst64[3]) && isFTOI12(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_FTOI15
					|| (isFTOI15(oInst64[3]) && isFTOI15(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_ITOF0
					|| (isITOF0(oInst64[3]) && isITOF0(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_ITOF4
					|| (isITOF4(oInst64[3]) && isITOF4(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_ITOF12
					|| (isITOF12(oInst64[3]) && isITOF12(oInst64[2]))
#endif
#ifdef ENABLE_PARALLEL_ITOF15
					|| (isITOF15(oInst64[3]) && isITOF15(oInst64[2]))
#endif
					)
				{
#ifdef REQUIRE_PARALLEL_NOP
					// for now, require nops on the low
					if (isNopLo(oInst64[3].Lo) && isNopLo(oInst64[2].Lo))
#endif
					{
						// mark as parallel
						LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_PARALLEL_EXEx2;
						LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_PARALLEL_EXEx2;

						// mark order/index
						LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_PARALLEL_INDEX1;
						LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_PARALLEL_INDEX0;
					}
				}

				// check if this is an x4 recompilation
				if ((iRecompilerWidth == RECOMPILER_WIDTH_AVX512X4))
				{
					// check the rest of the instructions
					if (
						!((LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)
						&& !((LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_2)
						&& !((LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_3)
						//&& !(LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_PARALLEL_EXE)
						&& !(LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)
						&& !(LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)
						&& !((LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)
						&& !((LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_2)
						//&& !(LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] & STATIC_PARALLEL_EXE)
						&& !(LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] & STATIC_ANY_BRANCH_DELAY_SLOT)
						&& !(LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] & STATIC_EBIT_DELAY_SLOT)
						&& !((LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)

						// also, for now, make sure flag summary is not disabled
						&& !(LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)
						&& !(LUT_StaticInfo[(ulInstCount - 0) & 0x7ff] & STATIC_DISABLE_FLAG_SUMMARY)

						// also make sure there are no flag checks or sets for now
						&& !(isMacFlagCheck(oInst64[1]) || isStatFlagCheck(oInst64[1]) || isClipFlagCheck(oInst64[1]) || isStatFlagSetLo(oInst64[1]) || isClipFlagSetLo(oInst64[1]))
						&& !(isMacFlagCheck(oInst64[0]) || isStatFlagCheck(oInst64[0]) || isClipFlagCheck(oInst64[0]) || isStatFlagSetLo(oInst64[0]) || isClipFlagSetLo(oInst64[0]))
						)
					{
						if (
							(isMType(oInst64[3]) && isMType(oInst64[2]) && isMType(oInst64[1]) && isMType(oInst64[0]))
							|| (isAType(oInst64[3]) && isAType(oInst64[2]) && isAType(oInst64[1]) && isAType(oInst64[0]))
							|| (isFTOI(oInst64[3]) && isFTOI(oInst64[2]) && isFTOI(oInst64[1]) && isFTOI(oInst64[0]))
							|| (isITOF(oInst64[3]) && isITOF(oInst64[2]) && isITOF(oInst64[1]) && isITOF(oInst64[0]))
							)
						{
							// mark as parallel

							// clear any x2
							LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] &= ~STATIC_PARALLEL_EXE_MASK;
							LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] &= ~STATIC_PARALLEL_EXE_MASK;

							LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_PARALLEL_EXEx4;
							LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_PARALLEL_EXEx4;
							LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_PARALLEL_EXEx4;
							LUT_StaticInfo[(ulInstCount - 0) & 0x7ff] |= STATIC_PARALLEL_EXEx4;

							// mark order/index
							LUT_StaticInfo[(ulInstCount - 3) & 0x7ff] |= STATIC_PARALLEL_INDEX3;
							LUT_StaticInfo[(ulInstCount - 2) & 0x7ff] |= STATIC_PARALLEL_INDEX2;
							LUT_StaticInfo[(ulInstCount - 1) & 0x7ff] |= STATIC_PARALLEL_INDEX1;
							LUT_StaticInfo[(ulInstCount - 0) & 0x7ff] |= STATIC_PARALLEL_INDEX0;
						}
					}

				}	// end if ((iRecompilerWidth == RECOMPILER_WIDTH_AVX512X4))

			}	// end if ((iRecompilerWidth == RECOMPILER_WIDTH_AVX2X2) || (iRecompilerWidth == RECOMPILER_WIDTH_AVX512X2) || (iRecompilerWidth == RECOMPILER_WIDTH_AVX512X4))
		}
#endif	// end #ifdef ENABLE_PARALLEL_VU_INSTS



		// alerts //

		// check if branch delay slot AND register sent to intdelayreg
		if (LUT_StaticInfo[ulInstCount] & STATIC_ANY_BRANCH_DELAY_SLOT)
		{
			if (LUT_StaticInfo[ulInstCount] & STATIC_MOVE_TO_INT_DELAY_SLOT)
			{
				cout << "\nhps2x64: ***ALERT: in branch delay slot but register is being output to IntDelayReg***\n";
			}
		}


		// check for and set parallel instructions at the end depending on whether x2 or x4 //





#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
		VU::debug << " Instr#" << dec << ulInstCount << " FLAGS:" << hex << LUT_StaticInfo[ulInstCount];
		VU::debug << "\r\n" << "PC=" << hex << (ulAddress + 0) << " " << Print::PrintInstructionHI(oInst64[0].Hi.Value).c_str() << "; " << hex << oInst64[0].Hi.Value;
		VU::debug << " FSrc=" << FSrcHi[0].b1 << " " << FSrcHi[0].b0 << " FDst=" << FDstHi[0].b1 << " " << FDstHi[0].b0;
		VU::debug << " ISrc=" << ISrcHi[0] << " IDst=" << IDstHi[0];
		if (!oInst64[0].Hi.I)
		{
			VU::debug << "\r\n" << "PC=" << hex << (ulAddress + 4) << " " << Print::PrintInstructionLO(oInst64[0].Lo.Value).c_str() << "; " << hex << oInst64[0].Lo.Value;
			VU::debug << " FSrc=" << FSrcLo[0].b1 << " " << FSrcLo[0].b0 << " FDst=" << FDstLo[0].b1 << " " << FDstLo[0].b0;
			VU::debug << " ISrc=" << ISrcLo[0] << " IDst=" << IDstLo[0];
			//VU::debug << " Lo.Imm11=" << oInst64[0].Lo.Imm11 << " getDstRegMapLo=" << getDstRegMapLo( oInst64[0] );
		}
#endif


		// go to next instruction //

		ulAddress += 8;
		ulInstCount++;

		oInst64[3].ValueLoHi = oInst64[2].ValueLoHi; oInst64[2].ValueLoHi = oInst64[1].ValueLoHi; oInst64[1].ValueLoHi = oInst64[0].ValueLoHi;

		FDstLo[3].b0 = FDstLo[2].b0; FDstLo[2].b0 = FDstLo[1].b0; FDstLo[1].b0 = FDstLo[0].b0;
		FDstLo[3].b1 = FDstLo[2].b1; FDstLo[2].b1 = FDstLo[1].b1; FDstLo[1].b1 = FDstLo[0].b1;
		FDstHi[3].b0 = FDstHi[2].b0; FDstHi[2].b0 = FDstHi[1].b0; FDstHi[1].b0 = FDstHi[0].b0;
		FDstHi[3].b1 = FDstHi[2].b1; FDstHi[2].b1 = FDstHi[1].b1; FDstHi[1].b1 = FDstHi[0].b1;
		FDst[3].b0 = FDst[2].b0; FDst[2].b0 = FDst[1].b0; FDst[1].b0 = FDst[0].b0;
		FDst[3].b1 = FDst[2].b1; FDst[2].b1 = FDst[1].b1; FDst[1].b1 = FDst[0].b1;

		FSrcLo[3].b0 = FSrcLo[2].b0; FSrcLo[2].b0 = FSrcLo[1].b0; FSrcLo[1].b0 = FSrcLo[0].b0;
		FSrcLo[3].b1 = FSrcLo[2].b1; FSrcLo[2].b1 = FSrcLo[1].b1; FSrcLo[1].b1 = FSrcLo[0].b1;
		FSrcHi[3].b0 = FSrcHi[2].b0; FSrcHi[2].b0 = FSrcHi[1].b0; FSrcHi[1].b0 = FSrcHi[0].b0;
		FSrcHi[3].b1 = FSrcHi[2].b1; FSrcHi[2].b1 = FSrcHi[1].b1; FSrcHi[1].b1 = FSrcHi[0].b1;
		FSrc[3].b0 = FSrc[2].b0; FSrc[2].b0 = FSrc[1].b0; FSrc[1].b0 = FSrc[0].b0;
		FSrc[3].b1 = FSrc[2].b1; FSrc[2].b1 = FSrc[1].b1; FSrc[1].b1 = FSrc[0].b1;

		IDstLo[3] = IDstLo[2]; IDstLo[2] = IDstLo[1]; IDstLo[1] = IDstLo[0];
		IDstHi[3] = IDstHi[2]; IDstHi[2] = IDstHi[1]; IDstLo[1] = IDstHi[0];
		IDst[3] = IDst[2]; IDst[2] = IDst[1]; IDst[1] = IDst[0];

		ISrcLo[3] = ISrcLo[2]; ISrcLo[2] = ISrcLo[1]; ISrcLo[1] = ISrcLo[0];
		ISrcHi[3] = ISrcHi[2]; ISrcHi[2] = ISrcHi[1]; ISrcHi[1] = ISrcHi[0];
		ISrc[3] = ISrc[2]; ISrc[2] = ISrc[1]; ISrc[1] = ISrc[0];

	}	// end while ( ulAddress < v->ulVuMem_Size )

#ifdef INLINE_DEBUG_DURING_STATIC_ANALYSIS
	VU::debug << "\r\n***STATIC-ANALYSIS-END***\r\n";
#endif

}



void Vu::Recompiler::Print_StaticAnalysis(VU* v)
{
	//cout << "\nVu::Recompiler::Print_StaticAnalysis(VU* v)";

	// current instruction
	Vu::Instruction::Format64 oInst64;

	// pointer into memory
	u32* pSrcCodePtr;


	stringstream ss;

	u32 ulInstCount = 0;

	// the current address
	u32 ulAddress = 0;

	// get pointer into VU instruction memory device
	// MicroMem32
	pSrcCodePtr = &(v->MicroMem32[0]);


	ss << "\n*** STATIC-ANALYSIS-OUTPUT (VU#" << v->Number << " VUSIZE=" << hex << v->ulVuMem_Size << " VUMASK=" << v->ulVuMem_Mask << ") ***\n";

	while (ulAddress < v->ulVuMem_Size)
	{
		oInst64.Lo.Value = *pSrcCodePtr++;
		oInst64.Hi.Value = *pSrcCodePtr++;

		ss << "\n\n ADDR: " << hex << ulAddress;

		// put in static analysis here //

		if (LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_EXE_MASK)
		{
			if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_EXE_MASK) == STATIC_PARALLEL_EXEx2)
			{
				ss << " +EXECx2";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_EXE_MASK) == STATIC_PARALLEL_EXEx3)
			{
				ss << " +EXECx3";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_EXE_MASK) == STATIC_PARALLEL_EXEx4)
			{
				ss << " +EXECx4";
			}


			if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX0)
			{
				ss << " +PARALLEL0";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX1)
			{
				ss << " +PARALLEL1";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX2)
			{
				ss << " +PARALLEL2";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_PARALLEL_INDEX_MASK) == STATIC_PARALLEL_INDEX3)
			{
				ss << " +PARALLEL3";
			}
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_HAZARD_RAW_X_MASK)
		{
			if ((LUT_StaticInfo[ulInstCount] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_1)
			{
				ss << " +HAZARD-RAW-1";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_2)
			{
				ss << " +HAZARD-RAW-2";
			}
			else if ((LUT_StaticInfo[ulInstCount] & STATIC_HAZARD_RAW_X_MASK) == STATIC_HAZARD_RAW_3)
			{
				ss << " +HAZARD-RAW-3";
			}
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_SKIP_EXEC_LOWER)
		{
			ss << " +SKIP-EXEC-LOWER";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_DISABLE_FLAG_SUMMARY)
		{
			ss << " +DISABLE-FLAG-SUMMARY";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_COMPLETE_FLOAT_MOVE)
		{
			ss << " +COMPLETE-FLOAT-MOVE";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_COND_BRANCH_DELAY_SLOT)
		{
			ss << " +COND-BRANCH-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_UNCOND_BRANCH_DELAY_SLOT)
		{
			ss << " +UNCOND-BRANCH-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_MOVE_TO_INT_DELAY_SLOT)
		{
			ss << " +MOVE-TO-INT-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_MOVE_FROM_INT_DELAY_SLOT)
		{
			ss << " +MOVE-FROM-INT-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_EBIT_DELAY_SLOT)
		{
			ss << " +EBIT-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_PROBLEM_IN_EBIT_DELAY_SLOT)
		{
			ss << " +PROBLEM-IN-EBIT-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_REVERSE_EXE_ORDER)
		{
			ss << " +REVERSE-EXE-ORDER";
		}


		if (LUT_StaticInfo[ulInstCount] & STATIC_FLAG_CHECK_MAC)
		{
			ss << " +STATIC_FLAG_CHECK_MAC";
		}
		if (LUT_StaticInfo[ulInstCount] & STATIC_FLAG_CHECK_STAT)
		{
			ss << " +STATIC_FLAG_CHECK_STAT";
		}
		if (LUT_StaticInfo[ulInstCount] & STATIC_FLAG_CHECK_CLIP)
		{
			ss << " +STATIC_FLAG_CHECK_CLIP";
		}



		if (LUT_StaticInfo[ulInstCount] & STATIC_BRANCH_IN_BRANCH_DELAY_SLOT)
		{
			ss << " +BRANCH-IN-BRANCH-DELAY-SLOT";
		}

		if (LUT_StaticInfo[ulInstCount] & STATIC_XGKICK_DELAY_SLOT)
		{
			ss << " +XGKICK-DELAY-SLOT";
		}



		if (LUT_StaticInfo[ulInstCount] & STATIC_REVERSE_EXE_ORDER)
		{
			ss << "\n" << Print::PrintInstructionHI(oInst64.Hi.Value).c_str() << " " << hex << oInst64.Hi.Value;
			ss << "\n" << Print::PrintInstructionLO(oInst64.Lo.Value).c_str() << " " << hex << oInst64.Lo.Value;
		}
		else
		{
			if (!oInst64.Hi.I)
			{
				ss << "\n" << Print::PrintInstructionLO(oInst64.Lo.Value).c_str() << " " << hex << oInst64.Lo.Value;
			}
			ss << "\n" << Print::PrintInstructionHI(oInst64.Hi.Value).c_str() << " " << hex << oInst64.Hi.Value;
		}


		ulAddress += 8;
		ulInstCount++;
	}

	ss << "\n*** STATIC-ANALYSIS-END ***\n";

#ifdef VERBOSE_PRINT_STATIC_ANALYSIS
	cout << ss.str().c_str();
#endif

#ifdef INLINE_DEBUG_PRINT_STATIC_ANALYSIS
	VU::debug << ss.str().c_str();
#endif

}




const Vu::Recompiler::Function Vu::Recompiler::FunctionList []
{
	Vu::Recompiler::INVALID,
	
	// VU macro mode instructions //
	
	//Vu::Recompiler::COP2
	//Vu::Recompiler::QMFC2_NI, Vu::Recompiler::QMFC2_I, Vu::Recompiler::QMTC2_NI, Vu::Recompiler::QMTC2_I, Vu::Recompiler::LQC2, Vu::Recompiler::SQC2,
	//Vu::Recompiler::CALLMS, Vu::Recompiler::CALLMSR,
	
	// upper instructions //
	
	// 24
	Vu::Recompiler::ABS,
	Vu::Recompiler::ADD, Vu::Recompiler::ADDi, Vu::Recompiler::ADDq, Vu::Recompiler::ADDBCX, Vu::Recompiler::ADDBCY, Vu::Recompiler::ADDBCZ, Vu::Recompiler::ADDBCW,
	Vu::Recompiler::ADDA, Vu::Recompiler::ADDAi, Vu::Recompiler::ADDAq, Vu::Recompiler::ADDABCX, Vu::Recompiler::ADDABCY, Vu::Recompiler::ADDABCZ, Vu::Recompiler::ADDABCW,
	Vu::Recompiler::CLIP,
	Vu::Recompiler::FTOI0, Vu::Recompiler::FTOI4, Vu::Recompiler::FTOI12, Vu::Recompiler::FTOI15,
	Vu::Recompiler::ITOF0, Vu::Recompiler::ITOF4, Vu::Recompiler::ITOF12, Vu::Recompiler::ITOF15,
	
	// 26
	Vu::Recompiler::MADD, Vu::Recompiler::MADDi, Vu::Recompiler::MADDq, Vu::Recompiler::MADDBCX, Vu::Recompiler::MADDBCY, Vu::Recompiler::MADDBCZ, Vu::Recompiler::MADDBCW,
	Vu::Recompiler::MADDA, Vu::Recompiler::MADDAi, Vu::Recompiler::MADDAq, Vu::Recompiler::MADDABCX, Vu::Recompiler::MADDABCY, Vu::Recompiler::MADDABCZ, Vu::Recompiler::MADDABCW,
	Vu::Recompiler::MAX, Vu::Recompiler::MAXi, Vu::Recompiler::MAXBCX, Vu::Recompiler::MAXBCY, Vu::Recompiler::MAXBCZ, Vu::Recompiler::MAXBCW,
	Vu::Recompiler::MINI, Vu::Recompiler::MINIi, Vu::Recompiler::MINIBCX, Vu::Recompiler::MINIBCY, Vu::Recompiler::MINIBCZ, Vu::Recompiler::MINIBCW,
	
	Vu::Recompiler::MSUB, Vu::Recompiler::MSUBi, Vu::Recompiler::MSUBq, Vu::Recompiler::MSUBBCX, Vu::Recompiler::MSUBBCY, Vu::Recompiler::MSUBBCZ, Vu::Recompiler::MSUBBCW,
	Vu::Recompiler::MSUBA, Vu::Recompiler::MSUBAi, Vu::Recompiler::MSUBAq, Vu::Recompiler::MSUBABCX, Vu::Recompiler::MSUBABCY, Vu::Recompiler::MSUBABCZ, Vu::Recompiler::MSUBABCW,
	Vu::Recompiler::MUL, Vu::Recompiler::MULi, Vu::Recompiler::MULq, Vu::Recompiler::MULBCX, Vu::Recompiler::MULBCY, Vu::Recompiler::MULBCZ, Vu::Recompiler::MULBCW,
	Vu::Recompiler::MULA, Vu::Recompiler::MULAi, Vu::Recompiler::MULAq, Vu::Recompiler::MULABCX, Vu::Recompiler::MULABCY, Vu::Recompiler::MULABCZ, Vu::Recompiler::MULABCW,
	Vu::Recompiler::NOP, Vu::Recompiler::OPMSUB, Vu::Recompiler::OPMULA,
	Vu::Recompiler::SUB, Vu::Recompiler::SUBi, Vu::Recompiler::SUBq, Vu::Recompiler::SUBBCX, Vu::Recompiler::SUBBCY, Vu::Recompiler::SUBBCZ, Vu::Recompiler::SUBBCW,
	Vu::Recompiler::SUBA, Vu::Recompiler::SUBAi, Vu::Recompiler::SUBAq, Vu::Recompiler::SUBABCX, Vu::Recompiler::SUBABCY, Vu::Recompiler::SUBABCZ, Vu::Recompiler::SUBABCW,
	
	// lower instructions //
	
	Vu::Recompiler::DIV,
	Vu::Recompiler::IADD, Vu::Recompiler::IADDI, Vu::Recompiler::IAND,
	Vu::Recompiler::ILWR,
	Vu::Recompiler::IOR, Vu::Recompiler::ISUB,
	Vu::Recompiler::ISWR,
	Vu::Recompiler::LQD, Vu::Recompiler::LQI,
	Vu::Recompiler::MFIR, Vu::Recompiler::MOVE, Vu::Recompiler::MR32, Vu::Recompiler::MTIR,
	Vu::Recompiler::RGET, Vu::Recompiler::RINIT, Vu::Recompiler::RNEXT,
	Vu::Recompiler::RSQRT,
	Vu::Recompiler::RXOR,
	Vu::Recompiler::SQD, Vu::Recompiler::SQI,
	Vu::Recompiler::SQRT,
	Vu::Recompiler::WAITQ,

	// instructions not in macro mode //
	
	Vu::Recompiler::B, Vu::Recompiler::BAL,
	Vu::Recompiler::FCAND, Vu::Recompiler::FCEQ, Vu::Recompiler::FCGET, Vu::Recompiler::FCOR, Vu::Recompiler::FCSET,
	Vu::Recompiler::FMAND, Vu::Recompiler::FMEQ, Vu::Recompiler::FMOR,
	Vu::Recompiler::FSAND, Vu::Recompiler::FSEQ, Vu::Recompiler::FSOR, Vu::Recompiler::FSSET,
	Vu::Recompiler::IADDIU,
	Vu::Recompiler::IBEQ, Vu::Recompiler::IBGEZ, Vu::Recompiler::IBGTZ, Vu::Recompiler::IBLEZ, Vu::Recompiler::IBLTZ, Vu::Recompiler::IBNE,
	Vu::Recompiler::ILW,
	Vu::Recompiler::ISUBIU, Vu::Recompiler::ISW,
	Vu::Recompiler::JALR, Vu::Recompiler::JR,
	Vu::Recompiler::LQ,
	Vu::Recompiler::MFP,
	Vu::Recompiler::SQ,
	Vu::Recompiler::WAITP,
	Vu::Recompiler::XGKICK, Vu::Recompiler::XITOP, Vu::Recompiler::XTOP,

	// External Unit //

	Vu::Recompiler::EATAN, Vu::Recompiler::EATANxy, Vu::Recompiler::EATANxz, Vu::Recompiler::EEXP, Vu::Recompiler::ELENG, Vu::Recompiler::ERCPR, Vu::Recompiler::ERLENG, Vu::Recompiler::ERSADD,
	Vu::Recompiler::ERSQRT, Vu::Recompiler::ESADD, Vu::Recompiler::ESIN, Vu::Recompiler::ESQRT, Vu::Recompiler::ESUM
};
