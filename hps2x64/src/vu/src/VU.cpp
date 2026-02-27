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


// need a define to toggle sse2 on and off for now
// on 64-bit systems, sse2 is supposed to come as standard
//#define _ENABLE_SSE2
//#define _ENABLE_SSE41


#include "VU.h"
#include "VU_Print.h"
#include "VU_Execute.h"

#include "R5900.h"

// used for restarting VU Dma transfer - unless its going to check every cycle, needs to alert dma to recheck ability to transfer
#include "PS2_Dma.h"

#include "PS2_Gpu.h"


#include "VU_Recompiler.h"


using namespace Playstation2;
using namespace Vu;
//using namespace x64Asm::Utilities;
//using namespace Math::Reciprocal;

#if defined(__GNUC__) && !defined(_MSC_VER)
// GCC on Windows does not support __try/__except in C++ mode.
// Map __except to catch(...) so the code compiles; __try is already an alias for try in GCC.
#ifndef __except
#define __except(x) catch(...)
#endif
#endif



//#define VERBOSE_RECOMPILE
//#define VERBOSE_CHECKSUM



//#define VERBOSE_MAIN_THREAD
//#define VERBOSE_VU_THREAD


// do a full pipeline clear when vu performs a branch/jump
//#define ENABLE_PIPELINE_CLEAR_ON_BRANCH



// enable running checksum instead of re-calculating checksum all at once
//#define ENABLE_RUNNING_CHECKSUM



#define ENABLE_VU_RECOMPILER



#define DISABLE_PATH1_WRAP


// enable to allow vu1 multi-threading. Probably more accurate also (should work with or without multi-threading)
#define ENABLE_PATH1_BUFFER


// can enable this as long as path1 buffer is enabled
#define ALLOW_VU1_MULTITHREADING


// enables passive multi-threading on VU thread. must be enabled for best performance
#define ENABLE_WAIT_DURING_SPIN_THREAD_VU



//#define ENABLE_XGKICK_WAIT

// looks like it should perform load/store if it is in e-bit delay slot?
//#define DISALLOW_LOADSTORE_IN_DELAY_SLOT
// looks like it can also perform xgkick from e-bit delay slot?
//#define DISALLOW_XGKICK_IN_DELAY_SLOT

#define DISALLOW_BRANCH_IN_DELAY_SLOT


// this should be enabled to delay the flag update, which is how it appears to work on PS2
#define DELAY_FLAG_UPDATE


//#define ENABLE_NEW_CLIP_BUFFER
//#define ENABLE_NEW_FLAG_BUFFER
#define ENABLE_SNAPSHOTS



#define ENABLE_RECOMPILER_VU
#define ALLOW_RECOMPILE_INTERPRETER


// each device on ps2 has a buffer
//#define ENABLE_QWBUFFER
//#define ENABLE_QWBUFFER_DMA



//#define VERBOSE_UNPACK

//#define VERBOSE_MSCAL
//#define VERBOSE_MSCNT
//#define VERBOSE_MSCALF
//#define VERBOSE_MSKPATH3
//#define VERBOSE_VU_MARK
//#define VERBOSE_VUINT


// will need this for accurate operation, and cycle accuracy is required
#define ENABLE_STALLS
#define ENABLE_INTDELAYSLOT

// says whether flags should get updated immediately in vu0 macro mode or not
#define UPDATE_MACRO_FLAGS_IMMEDIATELY


//#define ENABLE_FLOAT_ADD
//#define ENABLE_FLOAT_TOTAL


#define DISABLE_UNPACK_INDETERMINATE


// forces all upper vu0 addresses to access vu1, instead of just address with 0x4000 bit set
//#define ALL_VU0_UPPER_ADDRS_ACCESS_VU1


#define HALT_DIRECT_WHILE_VU_RUNNING
#define HALT_DIRECTHL_WHILE_VU_RUNNING

#define ENABLE_SET_VGW



#define USE_NEW_RECOMPILE2


// comment out to disable re-arranging exe order based on static analysis results
#define USE_NEW_RECOMPILE2_EXEORDER


// testing path 3 mask
// defines also in PS2_GPU.cpp
//#define DISABLE_PATH3_MASK
//#define ENABLE_PATH3_MASK_NO_CONDITION
//#define ENABLE_PATH3_MASK_CONDITION_END_PACKET
//#define ENABLE_PATH3_MASK_CONDITION_END_TRANSFER


//#define ENABLE_PASSIVE_WAIT

//#define ALLOW_VERBOSE_DEBUG_PS2_VU_READ
//#define ALLOW_VERBOSE_DEBUG_PS2_VU_WRITE

// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE


//#define INLINE_DEBUG_SPLIT



//#define INLINE_DEBUG

//#define INLINE_DEBUG_RECOMPILE2

/*
#define INLINE_DEBUG_VURUN


#define INLINE_DEBUG_INTDELAYSLOT


//#define INLINE_DEBUG_PIPELINE


#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE

//#define INLINE_DEBUG_DMA_READ

#define INLINE_DEBUG_DMA_WRITE
#define INLINE_DEBUG_DMA_WRITE_READY

#define INLINE_DEBUG_DMA_READ_READY

#define INLINE_DEBUG_VUCOM
//#define INLINE_DEBUG_VUCOM_MT





//#define INLINE_DEBUG_FIFO

//#define INLINE_DEBUG_ADVANCE_CYCLE


//#define INLINE_DEBUG_UNPACK
//#define INLINE_DEBUG_UNPACK_2
//#define INLINE_DEBUG_UNPACK_3


// this sends info to the vu execute debug on when vu gets started, etc
#define INLINE_DEBUG_VUEXECUTE
//#define INLINE_DEBUG_VUEXECUTE_MT
*/


#define INLINE_DEBUG_GETMEMPTR_INVALID
//#define INLINE_DEBUG_INVALID
//#define INLINE_DEBUG_INVALID_MT


#endif


VU::ps2_vu_func_template VU::ps2_arr_execute_unpack_lut[1 << 8];
VU::ps2_vu_func_template2 VU::ps2_arr_execute_unpack_avx2_lut[1 << 8];

alignas(16) u32 VU::ps2_arr_mask_row_avx2_lut[256 << 2];
alignas(16) u32 VU::ps2_arr_mask_col_avx2_lut[256 << 2];
alignas(16) u32 VU::ps2_arr_mask_write_avx2_lut[256 << 2];
alignas(16) u32 VU::ps2_arr_mask_rowwrite_avx2_lut[256 << 2];


HANDLE VU::ghEvent_PS2VU1_Update;
HANDLE VU::ghEvent_PS2VU1_Running;
HANDLE VU::ghEvent_PS2VU1_Finish;

Api::Thread* VU::VUThreads [ 2 ];


u32 VU::ulThreadCount;
u32 VU::ulThreadCount_Created;



#ifdef ENABLE_VU_MULTI_THREAD

volatile u64 VU::ullCommBuf_ReadIdx;
volatile u64 VU::ullCommBuf_WriteIdx;
volatile u64 VU::ullCommBuf_TargetIdx2;
volatile u32 VU::pCommandBuffer32 [ VU::c_ullCommBufSize ];

#endif



//u16 VU::Temp_StatusFlag, VU::Temp_MacFlag;
//VU::Bitmap128 VU::Temp_Bitmap;


VU *VU::_VU [ VU::c_iMaxInstances ];


u32 *VU::pStaticInfo [ 2 ];


//Vu::Instruction::Format2 VU::CurInstLOHI;

//Vu::Instruction::Format VU::CurInstLO;
//Vu::Instruction::Format VU::CurInstHI;


Vu::Recompiler* VU::vrs [ 2 ];


// need recompiler cache for vu
//Vu::Recompiler* vu_recompiler_cache [ 32 ] [ 2 ];
Vu::Recompiler* vu_recompiler_cache[VU::c_iVURecompilerCache_MaxEntries][2];


VU *VU0::_VU0;
VU *VU1::_VU1;

int VU::iInstance = 0;


u32 VU::bCodeModified [ 2 ];


// bitmaps for recompiler
//VU::Bitmap128 VU::FSrcBitmap;
//u64 VU::ISrcBitmap;

//VU::Bitmap128 VU::FDstBitmap;
//u64 VU::IDstBitmap;


volatile u32 VU::ulKillVuThread;



alignas(16) u64 VU::ReadData128 [ 2 ];


u64 VU::uExceptionRWAddr64;
u64 VU::uExceptionInstrAddr64;
u64 VU::uExceptionPC64;
u64 VU::uExceptionCycles64;

bool VU::bIsFloatException;
bool VU::bIsInvalidException;


u32* VU::_DebugPC;
u64* VU::_DebugCycleCount;

//u32* VU::_Intc_Master;
u32* VU::_Intc_Stat;
u32* VU::_Intc_Mask;
u32* VU::_R5900_Status_12;
u32* VU::_R5900_Cause_13;
u64* VU::_ProcStatus;

//long *Playstation2::pAddress;	// = & ( VU::_VU[1]->VuMem64[ 0x56 << 1 ] );

//VU* VU::_VU;


u64* VU::_NextSystemEvent;

u32* VU::_NextEventIdx;

bool VU::bEnableVerboseDebugRead;
bool VU::bEnableVerboseDebugWrite;

// needs to be removed sometime - no longer needed
u32* VU::DebugCpuPC;




bool VU::DebugWindow_Enabled [ VU::c_iMaxInstances ];

#ifdef ENABLE_GUI_DEBUGGER
WindowClass::Window *VU::DebugWindow [ VU::c_iMaxInstances ];
DebugValueList<float> *VU::GPR_ValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP0_ValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP2_CPCValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP2_CPRValueList [ VU::c_iMaxInstances ];
Debug_DisassemblyViewer *VU::DisAsm_Window [ VU::c_iMaxInstances ];
Debug_BreakpointWindow *VU::Breakpoint_Window [ VU::c_iMaxInstances ];
Debug_MemoryViewer *VU::ScratchPad_Viewer [ VU::c_iMaxInstances ];
Debug_BreakPoints *VU::Breakpoints [ VU::c_iMaxInstances ];
#endif



Debug::Log VU::debug;



const char* VU::VU0RegNames [ 24 ] = { "STAT", "FBRST", "ERR", "MARK", "CYCLE", "MODE", "NUM", "MASK", "CODE", "ITOPS", "RES", "RES", "RES", "ITOP", "RES", "RES",
											"R0", "R1", "R2", "R3", "C0", "C1", "C2", "C3" };
											
const char* VU::VU1RegNames [ 24 ] = { "STAT", "FBRST", "ERR", "MARK", "CYCLE", "MODE", "NUM", "MASK", "CODE", "ITOPS", "BASE", "OFST", "TOPS", "ITOP", "TOP", "RES",
											"R0", "R1", "R2", "R3", "C0", "C1", "C2", "C3" };


template<const int OFFSET = 0, const int IDX = 0>
static inline constexpr void init_vu_array_mask_unpack()
{
	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int M0 = ((IDX2 >> 0) & 3);
	constexpr const int M1 = ((IDX2 >> 2) & 3);
	constexpr const int M2 = ((IDX2 >> 4) & 3);
	constexpr const int M3 = ((IDX2 >> 6) & 3);

	constexpr const int ROW0 = (M0 == 1) ? -1u : 0;
	constexpr const int ROW1 = (M1 == 1) ? -1u : 0;
	constexpr const int ROW2 = (M2 == 1) ? -1u : 0;
	constexpr const int ROW3 = (M3 == 1) ? -1u : 0;

	constexpr const int COL0 = (M0 == 2) ? -1u : 0;
	constexpr const int COL1 = (M1 == 2) ? -1u : 0;
	constexpr const int COL2 = (M2 == 2) ? -1u : 0;
	constexpr const int COL3 = (M3 == 2) ? -1u : 0;

	constexpr const int WRITE0 = (M0 != 3) ? -1u : 0;
	constexpr const int WRITE1 = (M1 != 3) ? -1u : 0;
	constexpr const int WRITE2 = (M2 != 3) ? -1u : 0;
	constexpr const int WRITE3 = (M3 != 3) ? -1u : 0;

	constexpr const int ROWWRITE0 = (M0 == 0) ? -1u : 0;
	constexpr const int ROWWRITE1 = (M1 == 0) ? -1u : 0;
	constexpr const int ROWWRITE2 = (M2 == 0) ? -1u : 0;
	constexpr const int ROWWRITE3 = (M3 == 0) ? -1u : 0;

#define USE_TEMPLATES_VU_MASK_UNPACK_AVX2
#ifdef USE_TEMPLATES_VU_MASK_UNPACK_AVX2

	// rowwrite mask (mx==0)
	VU::ps2_arr_mask_rowwrite_avx2_lut[(IDX2 << 2) + 0] = ROWWRITE0;
	VU::ps2_arr_mask_rowwrite_avx2_lut[(IDX2 << 2) + 1] = ROWWRITE1;
	VU::ps2_arr_mask_rowwrite_avx2_lut[(IDX2 << 2) + 2] = ROWWRITE2;
	VU::ps2_arr_mask_rowwrite_avx2_lut[(IDX2 << 2) + 3] = ROWWRITE3;

	// row mask (mx==1)
	VU::ps2_arr_mask_row_avx2_lut[(IDX2 << 2) + 0] = ROW0;
	VU::ps2_arr_mask_row_avx2_lut[(IDX2 << 2) + 1] = ROW1;
	VU::ps2_arr_mask_row_avx2_lut[(IDX2 << 2) + 2] = ROW2;
	VU::ps2_arr_mask_row_avx2_lut[(IDX2 << 2) + 3] = ROW3;

	// col mask (mx==2)
	VU::ps2_arr_mask_col_avx2_lut[(IDX2 << 2) + 0] = COL0;
	VU::ps2_arr_mask_col_avx2_lut[(IDX2 << 2) + 1] = COL1;
	VU::ps2_arr_mask_col_avx2_lut[(IDX2 << 2) + 2] = COL2;
	VU::ps2_arr_mask_col_avx2_lut[(IDX2 << 2) + 3] = COL3;

	// write mask (mx!=3)
	VU::ps2_arr_mask_write_avx2_lut[(IDX2 << 2) + 0] = WRITE0;
	VU::ps2_arr_mask_write_avx2_lut[(IDX2 << 2) + 1] = WRITE1;
	VU::ps2_arr_mask_write_avx2_lut[(IDX2 << 2) + 2] = WRITE2;
	VU::ps2_arr_mask_write_avx2_lut[(IDX2 << 2) + 3] = WRITE3;

#endif


	init_vu_array_mask_unpack<OFFSET, IDX + 1>();

}

template<> inline constexpr void init_vu_array_mask_unpack<0, (1 << 8)>() {}


template<const int OFFSET = 0, const int IDX = 0>
static inline constexpr void init_vu_array_execute_unpack()
{
	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int VIFCODE_VL = ((IDX2 >> 0) & 3);
	constexpr const int VIFCODE_VN = ((IDX2 >> 2) & 3);

	constexpr const int VIFCODE_M = ((IDX2 >> 4) & 1);

	constexpr const int VIFCODE_USN = ((IDX2 >> 5) & 1);

	constexpr const int VIFREG_MODE = ((IDX2 >> 6) & 3);


#define USE_TEMPLATES_VU_EXECUTE_UNPACK
#ifdef USE_TEMPLATES_VU_EXECUTE_UNPACK

	VU::ps2_arr_execute_unpack_lut[IDX2] = &VU::Execute_UNPACK<VIFREG_MODE, VIFCODE_USN, VIFCODE_M, VIFCODE_VN, VIFCODE_VL>;

#endif


#define USE_TEMPLATES_VU_EXECUTE_UNPACK_AVX2
#ifdef USE_TEMPLATES_VU_EXECUTE_UNPACK_AVX2

	VU::ps2_arr_execute_unpack_avx2_lut[IDX2] = &VU::Execute_UNPACK_AVX2<VIFREG_MODE, VIFCODE_USN, VIFCODE_M, VIFCODE_VN, VIFCODE_VL>;

#endif

	init_vu_array_execute_unpack<OFFSET, IDX + 1>();
}

template<> inline constexpr void init_vu_array_execute_unpack<0, (1 << 8)>() {}


VU::VU ()
{

	cout << "Running VU constructor...\n";
}

void VU::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( VU ) );

	// zero recompiler caches
	//memset ( vu0_recompiler_cache, 0, sizeof( vu0_recompiler_cache ) );
	//memset ( vu1_recompiler_cache, 0, sizeof( vu1_recompiler_cache ) );
	//cout << "\nsizeof( vu_recompiler_cache )= " << dec << sizeof( vu_recompiler_cache );
	//memset ( vu_recompiler_cache, 0, sizeof( vu_recompiler_cache ) );

	
	// initialize random value to zero?
	vi [ VU::REG_R ].u = ( 0x7f << 23 ) | 0;

	// q/p busy until cycle should be set to max since they have not been used
	QBusyUntil_Cycle = -1ull;
	PBusyUntil_Cycle = -1ull;

	// busy until cycle should be at least 1 or ps2 dma might think device is not ready at all
	BusyUntil_Cycle = 1;

}



// actually, you need to start objects after everything has been initialized
void VU::Start ( int iNumber )
{
	cout << "Running VU::Start...\n";
	
	
#ifdef INLINE_DEBUG_ENABLE
	if ( !iNumber )
	{
#ifdef INLINE_DEBUG_SPLIT
	// put debug output into a separate file
	debug.SetSplit ( true );
	debug.SetCombine ( false );
#endif

	debug.Create ( "PS2_VU_Log.txt" );
	}
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering VU::Start";
#endif


	cout << "Resetting VU...\n";

	Reset ();

	cout << "Starting VU Lookup object...\n";
	
	Vu::Instruction::Lookup::Start ();
	
	cout << "Starting VU Print object...\n";
	
	Vu::Instruction::Print::Start ();
	
	// only need to start the VU Execute once, or else there are issues with debugging
	if ( !iNumber )
	{
		Vu::Instruction::Execute::Start ();
	}
	
	// set the vu number
	Number = iNumber;
	
	// set as current GPU object
	if ( Number < c_iMaxInstances )
	{
		_VU [ Number ] = this;
		
		if ( !Number )
		{
			ulMicroMem_Start = c_ulMicroMem0_Start;
			ulMicroMem_Size = c_ulMicroMem0_Size;
			ulMicroMem_Mask = c_ulMicroMem0_Mask;
			ulVuMem_Start = c_ulVuMem0_Start;
			ulVuMem_Size = c_ulVuMem0_Size;
			ulVuMem_Mask = c_ulVuMem0_Mask;
		}
		else
		{
			ulMicroMem_Start = c_ulMicroMem1_Start;
			ulMicroMem_Size = c_ulMicroMem1_Size;
			ulMicroMem_Mask = c_ulMicroMem1_Mask;
			ulVuMem_Start = c_ulVuMem1_Start;
			ulVuMem_Size = c_ulVuMem1_Size;
			ulVuMem_Mask = c_ulVuMem1_Mask;
		}
		
		// initialize zero registers
		_VU [ Number ]->vi [ 0 ].u = 0;
		
		// f0 always 0, 0, 0, 1 ???
		_VU [ Number ]->vf [ 0 ].uLo = 0;
		_VU [ Number ]->vf [ 0 ].uHi = 0;
		_VU [ Number ]->vf [ 0 ].uw = 0x3f800000;


		// setup recompiler caches for VU //

		// set vu recompiler to use either sse/avx2/avx512
		//Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_SSE);
		//Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_SSEX1);

		// if using avx2 encoding max of 1 instruction at a time
		//Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_AVX2);
		//Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_AVX2X1);

		// if using avx2 encoding max of 2 instructions at a time
		Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_AVX2);
		Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_AVX2X2);

		// if using avx512 with 2 instructions at a time
		//Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_AVX512);
		//Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_AVX512X2);

		// if using avx512 with 4 instructions at a time
		//Recompiler::SetVectorType(Recompiler::VECTOR_TYPE_AVX512);
		//Recompiler::SetRecompilerWidth(Recompiler::RECOMPILER_WIDTH_AVX512X4);

#ifdef ENABLE_VU_RECOMPILER
		//Recompiler ( VU* v, u32 NumberOfBlocks, u32 BlockSize_PowerOfTwo, u32 MaxIStep );
		if ( Number )
		{

			// allocate recompiler caches for vu1
			for( int i = 0; i < VU::c_iVURecompilerCache_MaxEntries; i++ )
			{
				vu_recompiler_cache [ i ] [ Number ] = new Recompiler ( this, 0, VU1_CODE_BLOCK_SIZE_SHIFT, 11 );
				//vu_recompiler_cache [ i ] [ Number ]->SetOptimizationLevel ( this, 0 );
				vu_recompiler_cache [ i ] [ Number ]->SetOptimizationLevel ( this, 1 );
				vu_recompiler_cache [ i ] [ Number ]->ullChecksum = -1ull;
			}

			//vrs [ Number ] = new Recompiler ( this, 0, 21, 11 );
			iNextRecompile = 1;
			vrs [ Number ] = vu_recompiler_cache [ 0 ] [ Number ];
			ullRunningChecksum = Vu::Recompiler::Calc_Checksum( this );
			vrs [ Number ]->ullChecksum = ullRunningChecksum;

			// enable vu recompiler cache for vu1
			bRecompilerCacheEnabled = true;
		}
		else
		{

			// allocate recompiler caches for vu0
			for( int i = 0; i < VU::c_iVURecompilerCache_MaxEntries; i++ )
			{
				vu_recompiler_cache [ i ] [ Number ] = new Recompiler ( this, 0, VU0_CODE_BLOCK_SIZE_SHIFT, 9 );
				//vu_recompiler_cache [ i ] [ Number ]->SetOptimizationLevel ( this, 0 );
				vu_recompiler_cache [ i ] [ Number ]->SetOptimizationLevel ( this, 1 );
				vu_recompiler_cache [ i ] [ Number ]->ullChecksum = -1ull;
			}

			//vrs [ Number ] = new Recompiler ( this, 0, 21, 11 );
			iNextRecompile = 1;
			vrs [ Number ] = vu_recompiler_cache [ 0 ] [ Number ];
			ullRunningChecksum = Vu::Recompiler::Calc_Checksum( this );
			vrs [ Number ]->ullChecksum = ullRunningChecksum;

			// enable vu recompiler cache for vu1
			bRecompilerCacheEnabled = true;
		}
		
		//vrs [ Number ]->SetOptimizationLevel ( this, 0 );
		vrs [ Number ]->SetOptimizationLevel ( this, 1 );
		
		// enable recompiler by default
		bEnableRecompiler = true;

		// should be set when code is modified (start out as code modified to force recompile)
		bCodeModified [ Number ] = 1;

#else

		bEnableRecompiler = false;
#endif


#ifdef ENABLE_GUI_DEBUGGER
		cout << "\nVU#" << Number << " breakpoint instance";
		Breakpoints [ Number ] = new Debug_BreakPoints ( NULL, NULL, NULL );
#endif
	}


	// start as not in use
	// if this is -1, then in macro mode will think it is time to update q
	CycleCount = -2ull;	//-1LL;
	eCycleCount = -2ull;	// -1ull;
	//SetNextEvent_Cycle ( -1LL );
	

	// update number of object instances
	iInstance++;


	if ( Number )
	{
		// VU#1 has 16 qw buffer
		ulQWBufferSize = 16;
	}
	else
	{
		// VU#0 has 8 qw buffer
		ulQWBufferSize = 8;
	}
	
	
#ifdef ENABLE_VU_MULTI_THREAD

	// ***TESTING***
	// enable multi-threading for vu#1
	if ( Number )
	{
		// start from beginning of buffer
		ullCommBuf_ReadIdx = 0;
		ullCommBuf_WriteIdx = 0;
		ullCommBuf_TargetIdx2 = 0;

		// one thread maximum?
		//ulThreadCount = 1;
		ulThreadCount = 0;
		
		//bMultiThreadingEnabled = true;
	}

#endif


	// initialize lut for executing unpack
	init_vu_array_execute_unpack<>();
	init_vu_array_mask_unpack<>();
	
	//Playstation2::pAddress = (long*) & ( VU::_VU[1]->VuMem64[ 0x56 << 1 ] );

	cout << "done\n";

#ifdef INLINE_DEBUG
	debug << "->Exiting VU::Start";
#endif

	cout << "Exiting VU::Start...\n";
}








void VU::SetNextEvent ( u64 CycleOffset )
{
	NextEvent_Cycle = CycleOffset + *_DebugCycleCount;
	
	Update_NextEventCycle ();
}


void VU::SetNextEvent_Cycle ( u64 Cycle )
{
	NextEvent_Cycle = Cycle;
	
	Update_NextEventCycle ();
}

//void VU::Update_NextEventCycle ()
//{
//	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
//}

void VU::Update_NextEventCycle ()
{
	//if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) )
	if ( NextEvent_Cycle < *_NextSystemEvent )
	{
		*_NextSystemEvent = NextEvent_Cycle;
		*_NextEventIdx = NextEvent_Idx;
	}
}





void VU::Write_CTC ( u32 Register, u32 Data )
{
	// check if INT reg
	if ( Register < 16 )
	{
		vi [ Register ].sLo = (s16) Data;
	}
	else
	{
		// control register //
		
		switch ( Register )
		{
			// STATUS FLAG
			case 16:
				// lower 6-bits are ignored when writing status flag
				vi [ 16 ].u = ( vi [ 16 ].u & 0x3f ) | ( Data & 0xfc0 );
				break;
				
			// FBRST
			case 28:
				
				// check for VU#0 force break
				if ( Data & 0x1 )
				{
					cout << "\nhps2x64: VU#0: ALERT: FORCE BREAK!!!\n";
					
					// force break VU#0 //
					
					// stop the vu
					_VU [ 0 ]->StopVU ();
					
					// VU0 force break
					VU0::_VU0->vi [ 29 ].uLo |= ( 0x8 << ( 0x0 ) );
				}
				
				// check if we need to reset VU0
				if ( Data & 0x2 )
				{
					// reset VU0
					/*
					// do this for now
					_VU [ 0 ]->lVifIdx = 0;
					_VU [ 0 ]->lVifCodeState = 0;
					
					// set DBF to zero (for VU double buffering)
					//_VU [ 0 ]->VifRegs.STAT.DBF = 0;
					_VU [ 0 ]->VifRegs.STAT.Value = 0;
					_VU [ 0 ]->VifRegs.ERR = 0;
					_VU [ 0 ]->VifRegs.MARK = 0;
					*/
					
					// stop the vu
					_VU [ 0 ]->StopVU ();
					
					// note: this is actually VU FBRST, complete separate from VIF FBRST!!
					_VU [ 0 ]->vi [ VU::REG_STATUSFLAG ].u = 0;
					_VU [ 0 ]->vi [ VU::REG_MACFLAG ].u = 0;
					_VU [ 0 ]->vi [ VU::REG_CLIPFLAG ].u = 0;
					_VU [ 0 ]->vi [ VU::REG_FBRST ].u &= 0xf00;
					_VU [ 0 ]->vi [ VU::REG_VPUSTAT ].u &= 0xff00;

				}
				
				// check for VU#1 force break
				if ( Data & 0x100 )
				{
					cout << "\nhps2x64: VU#1: ALERT: FORCE BREAK!!!\n";
					
					// force break VU#1 //
					
					// stop the vu
					_VU [ 1 ]->StopVU ();
					
					// VU0 force break;
					VU0::_VU0->vi [ 29 ].uLo |= ( 0x8 << ( 0x8 ) );
				}
				
				// check if we need to reset VU1
				if ( Data & 0x200 )
				{
					// reset VU1
					/*
					// do this for now
					_VU [ 1 ]->lVifIdx = 0;
					_VU [ 1 ]->lVifCodeState = 0;
					
					// set DBF to zero (for VU double buffering)
					_VU [ 1 ]->VifRegs.STAT.DBF = 0;
					*/
					
					// stop the vu
					_VU [ 1 ]->StopVU ();
					
					// note: this is actually VU FBRST, complete separate from VIF FBRST!!
					_VU [ 1 ]->vi [ VU::REG_STATUSFLAG ].u = 0;
					_VU [ 1 ]->vi [ VU::REG_MACFLAG ].u = 0;
					_VU [ 1 ]->vi [ VU::REG_CLIPFLAG ].u = 0;
					_VU [ 0 ]->vi [ VU::REG_FBRST ].u &= 0x00f;
					_VU [ 0 ]->vi [ VU::REG_VPUSTAT ].u &= 0x00ff;

				}
				
				// clear bits 0,1 and 8,9
				Data &= ~0x303;
				
				// write register value
				vi [ Register ].u = Data;
				
				break;
				
			// CMSAR1 - runs vu1 from address on write
			case 31:
				cout << "\nhps2x64: ALERT: writing to CMSAR1!\n";
				vi [ Register ].u = Data;
				break;
				
			default:
				vi [ Register ].u = Data;
				break;
		}
	}
	
	
}

u32 VU::Read_CFC ( u32 Register )
{
	switch ( Register )
	{
		// FBRST register
		// bits 0,1,4-7,8,9,12-31 always read zero
		case 28:
			return vi [ Register ].u & ~0xfffff3f3;
			break;

		default:
			return vi [ Register ].u;
			break;
	}
}




u64 VU::Read ( u32 Address, u64 Mask )
{
	//u32 Temp;
	u64 Output = 0;
	u32 lReg;

#ifdef INLINE_DEBUG_READ
	debug << "\r\n\r\nVU::Read; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << " Mask=" << Mask;
	debug << "; VU#" << Number;
	if ( !Number )
	{
	if ( ( ( ( Address & 0xffff ) - 0x3800 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu0
	debug << "; " << VU0RegNames [ ( ( Address & 0xffff ) - 0x3800 ) >> 4 ];
	}
	}
	else
	{
	if ( ( ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu1
	debug << "; " << VU1RegNames [ ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ];
	}
	}
#endif

#ifdef ALLOW_VERBOSE_DEBUG_PS2_VU_READ
	if (bEnableVerboseDebugRead)
	{
		cout << "\nPS2 VU READ ADDR:0x" << hex << Address;

		if ((Address & 0xffff) < 0x4000)
		{
			cout << " " << REG_NAMES[(Address >> 10) & 0x1][(Address >> 4) & 0x1f];
		}
		else if ((Address & 0xffff) < 0x6000)
		{
			cout << " " << REG_NAMES2[(Address >> 13) & 0x1];
		}
		else
		{
			cout << " ERROR";
		}
	}
#endif	// end #ifdef ALLOW_VERBOSE_DEBUG_PS2_VU_READ


	// check if Address is for the registers or the fifo
	if ( ( Address & 0xffff ) < 0x4000 )
	{
		// get the register number
		lReg = ( Address >> 4 ) & 0x1f;
		
		// will set some values here for now
		switch ( lReg )
		{
			default:
				break;
		}
		
		
		if ( lReg < c_iNumVuRegs )
		{
			Output = VifRegs.Regs [ lReg ];
		}

		switch ( lReg )
		{
			// STAT
			case 0x0:
			
				
				break;
				
			default:
				break;
		}
	}
	else
	{
		// ***TODO*** implement READ from VIF1 FIFO!!!
		cout << "\nhps2x64: ALERT: READING VIF1 FIFO!!! ADDR=" << hex << Address << "\n";
		
		// read data from gpu
		GPU::Path2_ReadBlock ( ReadData128, 1 );
		
		Output = (u64) ReadData128;
	}
	
	
#ifdef INLINE_DEBUG_READ
	debug << "; Output=" << hex << Output;
#endif

#ifdef ALLOW_VERBOSE_DEBUG_PS2_VU_READ
	if (bEnableVerboseDebugRead)
	{
		cout << " DATA:0x" << hex << Output << " MASK:0x" << Mask << " PC:0x" << *_DebugPC << " CYCL#" << dec << *_DebugCycleCount;
	}
#endif

	return Output;
}


void VU::Write ( u32 Address, u64 Data, u64 Mask )
{
	u32 lReg;
	u32 QWC_Transferred;
	//u32 ulTempArray [ 4 ];
	
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\n\r\nVU::Write; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data << " Mask=" << Mask;
	debug << "; VU#" << Number;
	if ( !Number )
	{
	if ( ( ( ( Address & 0xffff ) - 0x3800 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu0
	debug << "; " << VU0RegNames [ ( ( Address & 0xffff ) - 0x3800 ) >> 4 ];
	}
	else if ( ( Address & 0xffff ) == 0x4000 )
	{
	debug << "; VIF0FIFO";
	}
	}
	else
	{
	if ( ( ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu1
	debug << "; " << VU1RegNames [ ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ];
	}
	else if ( ( Address & 0xffff ) == 0x5000 )
	{
	debug << "; VIF1FIFO";
	}
	}
#endif

#ifdef ALLOW_VERBOSE_DEBUG_PS2_VU_WRITE
	if (bEnableVerboseDebugWrite)
	{
		cout << "\nPS2 VU WRITE ADDR:0x" << hex << Address;

		if ((Address & 0xffff) < 0x4000)
		{
			cout << " " << REG_NAMES[(Address >> 10) & 0x1][(Address >> 4) & 0x1f];
		}
		else if ((Address & 0xffff) < 0x6000)
		{
			cout << " " << REG_NAMES2[(Address >> 13) & 0x1];
		}
		else
		{
			cout << " ERROR";
		}

		cout << " DATA:0x" << Data << " MASK:0x" << Mask << " PC:0x" << *_DebugPC << " CYCL#" << dec << *_DebugCycleCount;
	}
#endif	// end #ifdef ALLOW_VERBOSE_DEBUG_PS2_VU_WRITE


	if ( ( Address & 0xffff ) < 0x4000 )
	{
		// get register number being written to
		lReg = ( Address >> 4 ) & 0x1f;
		
		// perform actions before write
		switch ( lReg )
		{
			// STAT
			case 0x0:
			
				// this might be getting or'ed with 0x1f000000 ?
				// no, thats no it
				//Data |= 0x1f000000;
				
				// stat is read only
				return;
				
				break;
				
			// FBRST
			case 0x1:
			
				// when 1 is written, it means to reset VU completely
				// bit 0 - reset VU (when writing to memory mapped register. when writing to VU0 int reg 28, then it is bit 1)
				// ***todo*** this looks like reset VIF, not reset VU
				if ( Data & 0x1 )
				{
					// reset VIF //
					//Reset ();
				
					// vu no longer running?
					//Running = 0;
					
					// if vu is not running, then multi-threading obviously isn't active
					//bMultiThreadingActive = 0;
					
					// if no longer running, then no longer multi-threading either
					//ullMultiThread_StartCycle = 0;
					
					
					// do this for now
					// make sure to specify VU Number (was specifying vu#0 ??)
					//_VU [ 0 ]->lVifIdx = 0;
					//_VU [ 0 ]->lVifCodeState = 0;
					lVifIdx = 0;
					lVifCodeState = 0;
					
					// vif no longer stopped if stopped ??
					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

					// set DBF to zero (for VU double buffering)
					//_VU [ 0 ]->VifRegs.STAT.DBF = 0;
					//VifRegs.STAT.DBF = 0;
					// clear stat completely
					VifRegs.STAT.Value = 0;
					VifRegs.FBRST = 0;

					/*
					VifRegs.ERR = 0;
					VifRegs.MARK = 0;
					VifRegs.CYCLE.Value = 0;
					VifRegs.MODE = 0;
					VifRegs.NUM = 0;
					VifRegs.MASK = 0;
					VifRegs.CODE = 0;
					VifRegs.ITOPS = 0;
					VifRegs.BASE = 0;
					VifRegs.OFST = 0;
					VifRegs.TOPS = 0;
					VifRegs.ITOP = 0;
					VifRegs.TOP = 0;
					VifRegs.RES = 0;
					VifRegs.R0 = 0;
					VifRegs.R1 = 0;
					VifRegs.R2 = 0;
					VifRegs.R3 = 0;
					VifRegs.C0 = 0;
					VifRegs.C1 = 0;
					VifRegs.C2 = 0;
					VifRegs.C3 = 0;
					*/

					// clear bit 0
					//Data &= 0x1;
					Data &= ~0x1;

					//if ( ulThreadCount && Number )
					//{
					//	CopyResetTo_CommandBuffer ();
					//}
				}

				// bit 1 - force break
				// ***todo*** this is actually force break on VIF, not force break on VU
				if ( Data & 0x2 )
				{
					cout << "\nhps2x64: VU#" << Number << ": VIF: ALERT: FORCE BREAK!!!\n";
					
					VifRegs.STAT.VFS = 1;
					VifStopped = 1;
					
					// vu should also enter the "stopped" state //
					
					// vu no longer running?
					//Running = 0;
					//VifRegs.STAT.VEW = 0;
					
					// if vu is not running, then multi-threading obviously isn't active
					// ***todo*** !!!lock synchronize with the other thread if multi-threading vu!!!
					//bMultiThreadingActive = 0;
					
					// if no longer running, then no longer multi-threading either
					//ullMultiThread_StartCycle = 0;
				}

				// bit 2 - stop
				// ***todo*** this is actually stop for VIF, not stop for VU
				if ( Data & 0x4 )
				{
					cout << "\nhps2x64: VU#" << Number << ": VIF: ALERT: STOP!!!\n";
					
					VifRegs.STAT.VSS = 1;
					VifStopped = 1;
				}
				
				// writing 1 to STC (bit 3) clears VSS,VFS,VIS,INT,ER0,ER1 in STAT
				// also cancels any stalls
				if ( Data & 0x8 )
				{
					VifRegs.STAT.VSS = 0;
					VifRegs.STAT.VFS = 0;
					VifRegs.STAT.VIS = 0;
					VifRegs.STAT.INT = 0;
					VifRegs.STAT.ER0 = 0;
					VifRegs.STAT.ER1 = 0;
					
					// also should cancel any stalls for VIF
					VifStopped = 0;
					

					// backup cycle - to prevent skips for now
					// don't do this, since it is cool if done during a write/read
					// *_DebugCycleCount--;
					
					// ?? re-start dma after cancelling stall ??
					//Dma::_DMA->Transfer ( Number );
					Dma::_DMA->CheckTransfer ();
					
					// restore cycle - to prevent skips for now
					// *_DebugCycleCount++;
				}
				
				// don't write to fbrst?
				Data = 0;
				
				break;
				
			// MARK
			case 0x3:
			
				// STAT.MRK gets cleared when CPU writes to MARK register
				VifRegs.STAT.MRK = 0;
				break;
				
			// CODE register is probably read-only
			case 0x8:
				return;
				break;
				
				
			default:
				break;
		}

		
		if ( lReg < c_iNumVuRegs )
		{
			VifRegs.Regs [ lReg ] = Data;
		}

		// perform actions after write
		switch ( Address )
		{
			default:
				break;
		}
	}
	else
	{
		// Write to VIF FIFO address //
		
#ifdef INLINE_DEBUG_WRITE
	if (!Mask)
	{
	debug << " DATA128=" << (((u64*) Data)[0]) << " " << (((u64*) Data)[1]);
	}
#endif

		if ( Mask )
		{
			cout << "\nhps2x64: VU: ALERT: NON-128-bit write to VIF FIFO!!! UNIMPLEMENTED!!!\n";
		}

		QWC_Transferred = VIF_FIFO_Execute ( (u32*) Data, 4 );
		
		if ( !QWC_Transferred )
		{
#ifdef INLINE_DEBUG_WRITE
			debug << " QWC_Transferred=" << dec << QWC_Transferred;
			debug << "\r\nhps2x64 ALERT: VU: non-dma transfer did not completely execute\r\n";
			debug << "\r\nCommand=" << hex << ((u32*)Data) [ 0 ] << " " << ((u32*)Data) [ 1 ] << " " << ((u32*)Data) [ 2 ] << " " << ((u32*)Data) [ 3 ];
#endif

			cout << "\nhps2x64 ALERT: VU: non-dma transfer did not completely execute\n";
			cout << "\nCommand=" << hex << ((u32*)Data) [ 0 ] << " " << ((u32*)Data) [ 1 ] << " " << ((u32*)Data) [ 2 ] << " " << ((u32*)Data) [ 3 ];
			cout << " ulQWBufferCount=" << dec << ulQWBufferCount;
			
#ifdef ENABLE_QWBUFFER
			// put the remainder of transfer into QW Buffer
			ullQWBuffer [ ( ulQWBufferCount << 1 ) + 0 ] = ( (u64*) Data ) [ 0 ];
			ullQWBuffer [ ( ulQWBufferCount << 1 ) + 1 ] = ( (u64*) Data ) [ 1 ];
			
			// we now have 1 more QW in QWbuffer
			ulQWBufferCount++;
#endif
		}
	}
}


/*
void VU::Execute_CommandBuffer ( u64 ullTargetIdx )
{
	u64 ullCycleNum;
	u32 ulDataCount;

	u64 ullIndex0, ullIndex1;

	s32 lCount;

	// get the current target id
	while ( iRunning || ( ullCommBuf_ReadIdx != ullTargetIdx ) )
	{
		if ( iRunning )
		{
			do {
				// run vu if it is running
				Run ();
			} while ( iVifStopped );

		//cout << "\n***Running VU***";
		}

		// if vu is not running, then make sure vif is running
		if ( !iRunning )
		{
			iVifStopped = 0;
		}

		if ( !iVifStopped )
		{
#ifdef ENABLE_VERBOSE_MT
		cout << "\n***!iVifStopped*** ullTargetIdx=" << ullTargetIdx << " ullCommBuf_ReadIdx=" << ullCommBuf_ReadIdx;
#endif

			// if there is data in the command buffer, then process it
			if ( ullCommBuf_ReadIdx != ullTargetIdx )
			{
				// get the cycle#
				ullCycleNum = (u64) pCommandBuffer32 [ ullCommBuf_ReadIdx & c_ullCommBufMask ];
				ullCycleNum |= ( (u64) pCommandBuffer32 [ ( ullCommBuf_ReadIdx + 1 ) & c_ullCommBufMask ] ) << 32ull;

				if ( ullCycleNum == -1ull )
				{
					// reset signal
					//iVifIdx = 0;
					iVifCodeState = 0;

					// move to next item in buffer
					ullCommBuf_ReadIdx += 2;
					
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nVIF-MT-RESET READIDX->" << dec << ullCommBuf_ReadIdx;
#endif
				}
				else if ( !iRunning || ( ullCycleNum >= CycleCount ) )
				{

					// move to the data
					ullCommBuf_ReadIdx += 2;

					// get count of data
					ulDataCount = pCommandBuffer32 [ ullCommBuf_ReadIdx & c_ullCommBufMask ];
					ullCommBuf_ReadIdx++;

#ifdef ENABLE_VERBOSE_MT
					cout << "\n***ENTER DATA LOOP*** ulDataCount=" << dec << ulDataCount;
#endif

					lCount = VIF_FIFO_ExecuteMT ( (u32*) pCommandBuffer32, ullCommBuf_ReadIdx, c_ullCommBufMask, ullCycleNum, ulDataCount );

					if ( lCount > 0 )
					{
						ullCommBuf_ReadIdx += lCount;

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " READIDX->" << dec << ullCommBuf_ReadIdx;
#endif
					}	// end if ( lCount > 0 )

					// if the vif is stopped on the other thread, then need to hop out of loop
					if ( iVifStopped )
					{
						ullCommBuf_ReadIdx -= 3;

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " VIF-STOPPED READIDX->" << dec << ullCommBuf_ReadIdx;
#endif
					}	// end if ( iVifStopped )

				}	// end if ( !iRunning || ( ullCycleNum >= CycleCount ) )


#ifdef ENABLE_VERBOSE_MT
				cout << "\n***EXIT DATA LOOP***";
#endif

			}	// end if ( ullCommBuf_ReadIdx != ullTargetIdx )

		}	// end if ( !VifStopped )

	}	// end while ( iRunning || ( ullCommBuf_ReadIdx != ullTargetIdx ) )

	return;
}	// end void VU::Execute_CommandBuffer ( u64 ullTargetIdx )


bool VU::CopyResetTo_CommandBuffer ()
{
	// first I'll drop in the cycle count (64-bits)
	pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = -1;
	ullCommBuf_WriteIdx++;

	pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = -1;
	ullCommBuf_WriteIdx++;


	if ( ( ullCommBuf_WriteIdx - ullCommBuf_ReadIdx ) > c_ullCommBufSize )
	{
		cout << "\nhps2x64: VU: ALERT: Comm buffer overrun! READIDX=" << dec << ullCommBuf_ReadIdx << " WRITEIDX=" << ullCommBuf_WriteIdx;
	}

	return true;
}

bool VU::CopyTo_CommandBuffer ( u32* pData32, u32 iSize32 )
{
	// first I'll drop in the cycle count (64-bits)
	pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = *_DebugCycleCount;
	ullCommBuf_WriteIdx++;

	pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = ( *_DebugCycleCount >> 32 );
	ullCommBuf_WriteIdx++;

	// put in count of 32-bit data
	pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = iSize32;
	ullCommBuf_WriteIdx++;

	for ( int i = 0; i < iSize32; i++ )
	{
		// put in the command
		pCommandBuffer32 [ ullCommBuf_WriteIdx & c_ullCommBufMask ] = *pData32++;
		ullCommBuf_WriteIdx++;
	}

	if ( ( ullCommBuf_WriteIdx - ullCommBuf_ReadIdx ) > c_ullCommBufSize )
	{
		cout << "\nhps2x64: VU: ALERT: Comm buffer overrun! READIDX=" << dec << ullCommBuf_ReadIdx << " WRITEIDX=" << ullCommBuf_WriteIdx;
	}

	return true;
}


u32 VU::VIF_FIFO_ExecuteMT ( u32* pData, u64 iStartIndex, u64 iMask, u64 ullCycleNumber, u32 SizeInWords32 )
{
	u32 ulTemp;
	u32 *DstPtr32;
	
	u32 lWordsToTransfer;
	
	u32 QWC_Transferred;
	s32 UnpackX, UnpackY, UnpackZ, UnpackW;
	//s32 lUnpackTemp;
	union { s32 lUnpackData; float fUnpackData; };
	u32 ulUnpackMask;
	u32 ulUnpackRow, ulUnpackCol;
	u32 ulWriteCycle;
	
	u64 PreviousValue;
	
	// need an index for just the data being passed
	u64 iVifIdx = 0;
	
	u32 lWordsLeft = SizeInWords32;

	static const int c_iSTROW_CommandSize = 5;
	static const int c_iSTCOL_CommandSize = 5;

	// also need to subtract lVifIdx & 0x3 from the words left
	// to take over where we left off at
	//lWordsLeft -= ( lVifIdx & 0x3 );
	
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nVU::VIF_FIFO_ExecuteMT CNT=" << dec << SizeInWords32 << " READID=" << ullCommBuf_ReadIdx << " TARGETID=" << ullCommBuf_TargetIdx2 << " WRITEID=" << ullCommBuf_WriteIdx;
#endif

	// start reading from index offset (may have left off in middle of data QW block)
	//Data = & ( Data [ lVifIdx & 0x3 ] );
	
	
	while ( lWordsLeft )
	{
		
		// check if vif code processing is in progress
		if ( !iVifCodeState )
		{
			// load vif code
			//iVifCode.Value = *Data++;
			iVifCode.Value = pData [ ( iStartIndex + iVifIdx ) & iMask ];
			iVifIdx++;
			
			// vif code is 1 word
			lWordsLeft--;

//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << "\r\nVU::VIF_FIFO_ExecuteMT";
//#endif
#ifdef ENABLE_VERBOSE_MT
	cout << "\nVIFCODE=" << hex << iVifCode.Value;
#endif
		}
		
		// perform the action for the vif code
		switch ( iVifCode.CMD )
		{
			// STCYCL
			case 1:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " STCYCL";
#endif

				iVifRegs.CYCLE.Value = iVifCode.IMMED;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STCYCL***";
#endif
				break;

			// OFFSET
			case 2:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " OFFSET";
#endif

				// wait if vu is running ??
				if ( iRunning )
				{
					// a VU program is in progress already //
					iVifStopped = 1;

					// this command isn't getting executed yet
					iVifIdx--;
					
					// return the number of full quadwords transferred/processed
					return iVifIdx;
				}
				else
				{

					// VU program is NOT in progress //
					
					iVifStopped = 0;

					// Vu1 ONLY
					iVifRegs.OFST = iVifCode.IMMED;
				
					// set DBF Flag to zero
					iVifRegs.STAT.DBF = 0;
				
					// set TOPS to BASE
					// *note* should probably use DBF to see what this gets set to
					iVifRegs.TOPS = iVifRegs.BASE;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***OFFSET***";
#endif
				}

				break;

			// BASE
			case 3:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " BASE";
#endif

				// Vu1 ONLY
				iVifRegs.BASE = iVifCode.IMMED;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***BASE***";
#endif
				break;

			// ITOP
			case 4:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " ITOP";
#endif

				// set data pointer (ITOPS register) - first 10 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				iVifRegs.ITOPS = iVifCode.IMMED;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***ITOPS***";
#endif
				break;

			// STMOD
			case 5:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " STMOD";
#endif

				// set mode of addition decompression (MODE register) - first 2 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				iVifRegs.MODE = iVifCode.IMMED;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STMOD***";
#endif
				break;

			// STMASK
			case 32:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " STMASK";
#endif

				// Vu0 and Vu1
				
				if ( !iVifCodeState )
				{
					// there is more incoming data than just the tag
					iVifCodeState++;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STMASK1***";
#endif
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					// store mask
					//iVifRegs.MASK = *Data++;
					iVifRegs.MASK = pData [ ( iStartIndex + iVifIdx ) & iMask ];

					// update index
					iVifIdx++;
					
					// command size is 1+1 words
					lWordsLeft--;
					
					// command is done
					iVifCodeState = 0;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STMASK2***";
#endif
				}
				
				break;

			// STROW
			case 48:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " STROW";
#endif


				// Vu0 and Vu1
				if ( !iVifCodeState )
				{
					// there is more incoming data than just the tag
					iVifCodeState++;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STROW1***";
#endif
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( iVifCodeState < c_iSTROW_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//iVifRegs.Regs [ c_iRowRegStartIdx + iVifCodeState - 1 ] = *Data++;
						iVifRegs.Regs [ c_iRowRegStartIdx + iVifCodeState - 1 ] = pData [ ( iStartIndex + iVifIdx ) & iMask ];

						// update index
						iVifIdx++;
						
						// move to next data input element
						iVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STROW" << iVifCodeState << "***";
#endif
					}
					
					if ( iVifCodeState >= c_iSTROW_CommandSize )
					{
						// command is done
						iVifCodeState = 0;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STROWX***";
#endif
					}
				}
				
				break;

			// STCOL
			case 49:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " STCOL";
#endif


				// Vu0 and Vu1
				
				if ( !iVifCodeState )
				{
					// there is more incoming data than just the tag
					iVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( iVifCodeState < c_iSTCOL_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//iVifRegs.Regs [ c_iColRegStartIdx + iVifCodeState - 1 ] = *Data++;
						iVifRegs.Regs [ c_iColRegStartIdx + iVifCodeState - 1 ] = pData [ ( iStartIndex + iVifIdx ) & iMask ];
						
						// update index
						iVifIdx++;
						
						// move to next data input element
						iVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;
					}
					
					if ( iVifCodeState >= c_iSTCOL_CommandSize )
					{
						// command is done
						iVifCodeState = 0;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***STCOL***";
#endif
					}
				}
				
				break;

			// FLUSH
			case 16:
			case 17:
			case 19:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " FLUSH";
#endif

				// waits for vu program to finish
				// Vu0 and Vu1
				
				if ( iRunning )
				{
					iVifStopped = 1;
					
					// this command isn't getting executed yet
					iVifIdx--;

					// return the number of full quadwords transferred/processed
					return iVifIdx;
				}
				else
				{
					iVifStopped = 0;

#ifdef ENABLE_VERBOSE_MT
				cout << "\n***FLUSH***";
#endif
				}
				
				break;

			// MSCAL
			case 20:
			// MSCALF
			case 21:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " MSCAL";
#endif

				// waits for end of current vu program and runs new vu program starting at IMM*8
				// Vu0 and Vu1
				
				if ( iRunning )
				{
					// a VU program is in progress already //
					iVifStopped = 1;
					
					// this command isn't getting executed yet
					iVifIdx--;

					// return the number of full quadwords transferred/processed
					return iVifIdx;
				}
				else
				{
					// VU program is NOT in progress //

					iVifStopped = 0;
					
					// VU is now running
					iStartVU ( ullCycleNumber );
					
					// set TOP to TOPS
					iVifRegs.TOP = iVifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					iVifRegs.ITOP = iVifRegs.ITOPS;
					
					// reverse DBF flag
					iVifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					iVifRegs.TOPS = iVifRegs.BASE + ( iVifRegs.OFST * ( (u32) iVifRegs.STAT.DBF ) );

					// set PC = IMM * 8
					PC = iVifCode.IMMED << 3;
					
					// need to set next pc too since this could get called in the middle of VU Main CPU loop
					NextPC = PC;

#ifdef INLINE_DEBUG_VUEXECUTE_MT
					Vu::Instruction::Execute::debug << "\r\n*** MSCAL";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " StartPC=" << hex << PC;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << iVifCode.Value;
					Vu::Instruction::Execute::debug << " ***";
#endif
#ifdef ENABLE_VERBOSE_MT
				cout << "\n***MSCAL***";
#endif
				}

				break;

			// MSCNT
			case 23:
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " MSCNT";
#endif

				if ( iRunning )
				{
					// a VU program is in progress already //
					iVifStopped = 1;
					
					// this command isn't getting executed yet
					iVifIdx--;

					// return the number of full quadwords transferred/processed
					return iVifIdx;
				}
				else
				{
					// VU program is NOT in progress //

					iVifStopped = 0;
					
					// VU is now running
					iStartVU ( ullCycleNumber );
					
					// set TOP to TOPS
					iVifRegs.TOP = iVifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					iVifRegs.ITOP = iVifRegs.ITOPS;
					
					// reverse DBF flag
					iVifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					iVifRegs.TOPS = iVifRegs.BASE + ( iVifRegs.OFST * ( (u32) iVifRegs.STAT.DBF ) );

					// set PC = IMM * 8
					//PC = VifCode.IMMED << 3;
					
					// need to set next pc too since this could get called in the middle of VU Main CPU loop
					//NextPC = PC;

#ifdef INLINE_DEBUG_VUEXECUTE_MT
					Vu::Instruction::Execute::debug << "\r\n*** MSCNT";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " StartPC=" << hex << PC;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << iVifCode.Value;
					Vu::Instruction::Execute::debug << " ***";
#endif
#ifdef ENABLE_VERBOSE_MT
				cout << "\n***MSCNT***";
#endif
				}

				break;

			// MPG
			// MT
			case 74:

				// load following vu program of size NUM*2 into address IMM*8
				// Vu0 and Vu1
				
				// ***TODO*** MPG is supposed to wait for the end of the microprogram before executing



			
				if ( iRunning )
				{
					// VU program is in progress //

					iVifStopped = 1;
					
					// this command isn't getting executed yet
					iVifIdx--;
					
					// return the number of full quadwords transferred/processed
					return iVifIdx;
				}
				else
				{
					// VU program is NOT in progress //

					if ( !iVifCodeState )
					{
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " MPG";
#endif
						// set the command size
						// when zero is specified, it means 256
						iVifCommandSize = 1 + ( ( ( !iVifCode.NUM ) ? 256 : iVifCode.NUM ) << 1 );
						

						// there is more incoming data than just the tag
						iVifCodeState++;

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " MPGSIZE=" << dec << iVifCommandSize;
#endif

#ifdef INLINE_DEBUG_VUEXECUTE_MT
					Vu::Instruction::Execute::debug << "\r\n*** MPG";
					Vu::Instruction::Execute::debug << " VU#" << hex << Number;
					Vu::Instruction::Execute::debug << " PC=" << ( iVifCode.IMMED >> 1 );
					Vu::Instruction::Execute::debug << " VifCode=" << hex << iVifCode.Value;
					Vu::Instruction::Execute::debug << " ***";
#endif

					//cout << "\n***MPG START***";
					}

					iVifStopped = 0;

					// code has been modified for VU
					bCodeModified [ Number ] = 1;

					// if processing next element, also check there is data to process
					if ( lWordsLeft )
					{
//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " MPGWORK=" << dec << iVifCodeState << " OF " << iVifCommandSize;
//#endif

						//DstPtr32 = & MicroMem32 [ ( iVifCode.IMMED << 1 ) + ( iVifCodeState - 1 ) ];
						DstPtr32 = & MicroMem32 [ ( ( iVifCode.IMMED << 1 ) + ( iVifCodeState - 1 ) ) & ( ulVuMem_Mask >> 2 ) ];
						while ( lWordsLeft && ( iVifCodeState < iVifCommandSize ) )
						{
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							// *DstPtr32++ = *Data++;
							// *DstPtr32++ = pData [ ( iStartIndex + iVifIdx ) & iMask ];
							// but if data can wrap, then need to do it this way
							MicroMem32 [ ( ( iVifCode.IMMED << 1 ) + ( iVifCodeState - 1 ) ) & ( ulVuMem_Mask >> 2 ) ] = pData [ ( iStartIndex + iVifIdx ) & iMask ];

							// update index
							iVifIdx++;
							
							// move to next data input element
							iVifCodeState++;
							
							// command size is 1+NUM*2 words
							lWordsLeft--;
						}
						
						
						if ( iVifCodeState >= iVifCommandSize )
						{
							// command is done
							iVifCodeState = 0;
							
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " MPGDONE";
#endif
#ifdef ENABLE_VERBOSE_MT
					cout << "\n***MPG DONE***";
#endif
						}
					}
				}

				break;


			// UNPACK
			default:
			
				if ( ( ( iVifCode.Value >> 29 ) & 0x3 ) == 0x3 )
				{

					// unpacks data into vu data memory
					// Vu0 and Vu1
					if ( !iVifCodeState )
					{
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " UNPACK";
#endif
						// clear the the counter for unpack
						lVifUnpackIndex = 0;
						
						// looks like NUM=0 might be same as 256 ?? even for unpack ??
						iVifCommandSize = ( iVifCode.NUM ? iVifCode.NUM : 256 );
						
						// the actual amount of data to write in 32-bit words is NUM*4
						iVifCommandSize = 1 + ( iVifCommandSize << 2 );
						
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " UNPACKSIZE=" << dec << iVifCommandSize;
#endif

						// there is more incoming data than just the tag
						iVifCodeState++;
						
					// note: if running on another thread want to send command and:
					// TOPS,STMASK,STCYCLE,STCOL,STROW,DATA
					// note: if running on another thread want to send command and:
					// TOP,ITOP
						
						// need to start read offset at zero, and update when reading from dma/memory
						ulReadOffset32 = 0;
						
						// get size of data elements in bytes
						ulUnpackItemWidth = 4 >> iVifCode.vl;
						
						// get number of components to unpack
						ulUnpackNum = iVifCode.vn + 1;
						ulUnpackNumIdx = 0;
						
						// get wl/cl
						ulWL = iVifRegs.CYCLE.WL;
						ulCL = iVifRegs.CYCLE.CL;
						ulWLCycle = 0;
						ulCLCycle = 0;

						
						// initialize count of elements read
						ulReadCount = 0;

						// for UNPACK command, NUM register is set to NUM field
						//VifRegs.NUM = VifCode.NUM;

						// check if FLG bit is set
						if ( iVifCode.IMMED & 0x8000 )
						{
							// ***todo*** update to offset from current data being stored
							//DstPtr32 = & VuMem32 [ ( ( VifCode.IMMED & 0x3ff ) + ( VifRegs.TOPS & 0x3ff ) ) << 2 ];
							
							// where the data is getting stored to has to do with the write offset, which is the same as lVifCodeState-1
							//DstPtr32 += ( lVifCodeState - 1 );
							
							// need to keep track of where writing to in VU memory
							ulWriteOffset32 = ( ( ( iVifCode.IMMED & 0x3ff ) + ( iVifRegs.TOPS & 0x3ff ) ) << 2 ) + ( iVifCodeState - 1 );
						}
						else
						{
							// ***todo*** update to offset from current data being stored
							//DstPtr32 = & VuMem32 [ ( VifCode.IMMED & 0x3ff ) << 2 ];
							//DstPtr32 += ( lVifCodeState - 1 );
							
							// need to keep track of where writing to in VU memory
							ulWriteOffset32 = ( ( ( iVifCode.IMMED & 0x3ff ) ) << 2 ) + ( iVifCodeState - 1 );

						}	// end if ( iVifCode.IMMED & 0x8000 ) else


						
#ifdef ENABLE_VERBOSE_MT
					cout << "\n***UNPACK START***";
#endif
#ifdef INLINE_DEBUG_VUEXECUTE_MT
					Vu::Instruction::Execute::debug << "\r\n*** UNPACK";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << iVifCode.Value;
					Vu::Instruction::Execute::debug << hex << " BASE=" << iVifRegs.BASE << " OFFSET=" << iVifRegs.OFST << " TOP=" << iVifRegs.TOP << " TOPS=" << iVifRegs.TOPS;
					Vu::Instruction::Execute::debug << hex << " m=" << iVifCode.m << " vn=" << iVifCode.vn << " vl=" << iVifCode.vl << " CL=" << iVifRegs.CYCLE.CL << " WL=" << iVifRegs.CYCLE.WL << " FLG=" << ( ( iVifCode.IMMED & 0x8000 ) >> 15 ) << " ADDR=" << hex << ( iVifCode.IMMED & 0x3ff );
					Vu::Instruction::Execute::debug << dec << " unpackedsize=" << iVifCommandSize;
					Vu::Instruction::Execute::debug << " ***";
#endif

					}	// end if ( !iVifCodeState )
					
					//if ( lWordsLeft )
					if ( iVifCodeState )
					{
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " UNPACKWORK=" << dec << iVifCodeState << " OF " << iVifCommandSize;
#endif
						// do this for now
						// add tops register if FLG set
						
						// get dst pointer
						DstPtr32 = & VuMem32 [ ulWriteOffset32 ];
						
						//while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
						while ( iVifCodeState < iVifCommandSize )
						{
//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " ulWLCycle=" << dec << ulWLCycle << " ulWL=" << ulWL;
//#endif

							// first determine where data should be read from //
							// WL must be within range before data is read
							if ( ulWLCycle < ulWL )
							{
								// WL is within range
								// so we must read data from someplace
								
								// get Write Cycle
								// *todo* unsure if the "Write Cycle" is reset after CL or WL or both ??
								ulWriteCycle = ulWLCycle;
								
								// get unpack row/col
								ulUnpackRow = ( ( ulWriteCycle < 4 ) ? ulWriteCycle : 3 );
								ulUnpackCol = ulWriteOffset32 & 3;

//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " ulCLCycle=" << dec << ulCLCycle << " ulCL=" << ulCL;
//	debug << " ulUnpackCol=" << dec << ulUnpackCol << " ulUnpackNum=" << ulUnpackNum;
//#endif

								// to read data from dma/memory, CL must be within range, and Col must be within correct range
								if ( ( ulCLCycle < ulCL ) && ( ulUnpackCol < ulUnpackNum ) )
								{
									// data source is dma/ram //
								
									// make sure that there is data left to read
									if ( lWordsLeft <= 0 )
									{
										// there is no data to read //
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " OUT-OF-DATA";
#endif
										
										// return sequence when lVifCodeState is NOT zero //
										break;
										
									}
									
									// determine data element size
									if ( !ulUnpackItemWidth )
									{
//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " PIXEL";
//#endif

										// must be unpacking pixels //
										// *todo* check for vn!=3 which would mean it is not unpacking pixels but rather an invalid command
										//cout << "\nhps2x64: ALERT: VU: UNPACK: Pixel unpack unsupported!!\n";
										
										// similar to unpacking 16-bit halfwords //
										
										if ( ! ( ulReadCount & 7 ) )
										{
											//ulUnpackBuf = *Data++;
											ulUnpackBuf = pData [ ( iStartIndex + iVifIdx ) & iMask ];
										
											// get the correct element of 32-bit word
											//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
											//lUnpackTemp &= 0xffff;
										}
										
										
										// pull correct component
										if ( ulUnpackCol < 3 )
										{
											// RGB
											lUnpackLastData = ulUnpackBuf & 0x1f;
											lUnpackLastData <<= 3;
											
											ulUnpackBuf >>= 5;
										}
										else
										{
											// A
											lUnpackLastData = ulUnpackBuf & 1;
											lUnpackLastData <<= 7;
											
											ulUnpackBuf >>= 1;
										}
										
										ulReadCount++;
										
										// one pixel gets unpacked per 8 components RGBA
										if ( ! ( ulReadCount & 7 ) )
										{
											//Data++;
											iVifIdx++;
											
											// this is counting the words left in the input
											lWordsLeft--;
											
										}
									}
									else
									{
										// unpacking NON-pixels //
										
//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " NONPIXEL";
//	debug << " ulReadCount=" << dec << ulReadCount;
//#endif
										// check if only 1 element (vn=0) and not first col
										if ( iVifCode.vn || !ulUnpackCol )
										{
											if ( ! ( ulReadCount & ( 7 >> ulUnpackItemWidth ) ) )
											{
												//ulUnpackBuf = *Data++;
												ulUnpackBuf = pData [ ( iStartIndex + iVifIdx ) & iMask ];
											
												// get the correct element of 32-bit word
												//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
												//lUnpackTemp &= 0xffff;
											}
											
											
											lUnpackLastData = ulUnpackBuf & ( 0xffffffffUL >> ( ( 4 - ulUnpackItemWidth ) << 3 ) );
											
											ulUnpackBuf >>= ( ulUnpackItemWidth << 3 );
											
											// check if data item is signed and 1 or 2 bytes long
											if ( ( !iVifCode.USN ) && ( ( ulUnpackItemWidth == 1 ) || ( ulUnpackItemWidth == 2 ) ) )
											{
												// signed data //
												lUnpackLastData <<= ( ( 4 - ulUnpackItemWidth ) << 3 );
												lUnpackLastData >>= ( ( 4 - ulUnpackItemWidth ) << 3 );
											}
											
											ulReadCount++;
											
											if ( ! ( ulReadCount & ( 7 >> ulUnpackItemWidth ) ) )
											{
												//Data++;
												iVifIdx++;
												
												// command size is 1+NUM*2 words
												// this is counting the words left in the input, but should ALSO be counting words left in output
												lWordsLeft--;
												
											}

//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " iVifIdx=" << dec << iVifIdx << " lWordsLeft=" << lWordsLeft;
//#endif

										} // end if ( ulUnpackNum || !ulUnpackCol )
											
									} // end if ( !ulUnpackItemWidth )
									
									
									
								} // end if ( ulCLCycle < ulCL )
								
								
								// data source is determined by m and MASK //
								
								// if m bit is 0, then mask is zero
								ulUnpackMask = 0;
								
								if ( iVifCode.m )
								{
									// mask comes from MASK register //
									
									
									// mask index is (Row*4)+Col
									// but if the row (write cycle??) is 3 or 4 or greater then just use maximum row value
									ulUnpackMask = ( iVifRegs.MASK >> ( ( ( ulUnpackRow << 2 ) + ulUnpackCol ) << 1 ) ) & 3;
									
								}
								
								// determine source of data
								switch ( ulUnpackMask )
								{
									case 0:
										// data comes from dma/ram //
										lUnpackData = lUnpackLastData;
										break;
										
									case 1:
										// data comes from ROW register //
										lUnpackData = iVifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
										break;
										
									case 2:
										// data comes from COL register //
										lUnpackData = iVifRegs.Regs [ c_iColRegStartIdx + ulUnpackRow ];
										break;
										
									case 3:
										// write is masked/not happening/prevented //
										break;
								}
								
								// for adding/totalling with row register, you need either m=0 or m[x]=0
								if ( !iVifCode.m || !ulUnpackMask )
								{
									// check if data should be added or totaled with ROW register
									if ( iVifRegs.MODE & 3 )
									{
										// 1 means add, 2 means total
										
										switch ( iVifRegs.MODE & 3 )
										{
											case 1:
											
												// add to unpacked data //
												
												// I guess the add should be an integer one?
#ifdef ENABLE_FLOAT_ADD_MT
												fUnpackData += iVifRegs.fRegs [ c_iRowRegStartIdx + ulUnpackCol ];
#else
												lUnpackData += iVifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
#endif
												break;
												
											case 2:
#ifdef INLINE_DEBUG_UNPACK_2_MT
	debug << " TOTAL";
#endif

												// add to unpacked data and write result back to ROW register //
												
												// I guess the add/total should be an integer one?
#ifdef ENABLE_FLOAT_TOTAL_MT
												fUnpackData += iVifRegs.fRegs [ c_iRowRegStartIdx + ulUnpackCol ];
												iVifRegs.fRegs [ c_iRowRegStartIdx + ulUnpackCol ] = fUnpackData;
#else
												lUnpackData += iVifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
												iVifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ] = lUnpackData;
#endif
												break;
												
											
											// undocumented ??
											case 3:
#ifdef INLINE_DEBUG_UNPACK_2_MT
	debug << " UNDOCUMENTED";
#endif

												iVifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ] = lUnpackData;
												break;
												
											default:
#ifdef INLINE_DEBUG_UNPACK_MT
	debug << " INVALID";
#endif

												cout << "\nhps2x64: ALERT: VU: UNPACK: Invalid MODE setting:" << dec << VifRegs.MODE << "\n";
												break;

										}	// end switch ( iVifRegs.MODE & 3 )

									}	// end if ( iVifRegs.MODE & 3 )

								} // end if ( !VifCode.m || !ulUnpackMask )
								
								
#ifdef DISABLE_UNPACK_INDETERMINATE_MT
								// only write data if the column is within the amount being written, or it is masked
								if ( ( ulUnpackNum == 1 ) || ( ulUnpackMask ) || ( ulUnpackCol < ulUnpackNum ) )
								{
#endif
								
								// write data to vu memory if write is not masked //
								if ( ulUnpackMask != 3 )
								{
#ifdef INLINE_DEBUG_UNPACK_2_MT
	debug << "\r\nCol#" << ulUnpackCol << " Data=" << hex << lUnpackData << " TO " << ulWriteOffset32 << " Msk=" << ulUnpackMask;
#endif

									*DstPtr32 = lUnpackData;
									
#ifdef INLINE_DEBUG_UNPACK_3_MT
	debug << " VuMem=" << hex << VuMem32 [ ulWriteOffset32 ];
#endif
								}
#ifdef INLINE_DEBUG_UNPACK_2_MT
								else
								{
	debug << "\r\nCol#" << ulUnpackCol << " DATA:" << hex << lUnpackData << " MSKD TO " << hex << ulWriteOffset32 << " Msk=" << ulUnpackMask;
								}	// end if ( ulUnpackMask != 3 ) else
#endif

#ifdef DISABLE_UNPACK_INDETERMINATE_MT
								}	// if ( ( ulUnpackNum == 1 ) || ( ulUnpackMask ) || ( ulUnpackCol < ulUnpackNum ) )
#ifdef INLINE_DEBUG_UNPACK_2_MT
								else
								{
	debug << "\r\nCol#" << ulUnpackCol << " UNDT TO " << hex << ulWriteOffset32;
								}	// end if ( ( ulUnpackNum == 1 ) || ( ulUnpackCol < ulUnpackNum ) ) else
#endif
								
#endif
								
//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " BEFORE=" << dec << iVifCodeState;
//#endif
								// move to next data input element
								// this should update per 32-bit word that is written into VU memory
								// WL is a write cycle
								iVifCodeState++;

//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << " AFTER=" << dec << iVifCodeState;
//#endif

							} // end if ( ulWLCycle < ulWL )
						
						
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							//*DstPtr32++ = *Data++;
							
							// update index
							//lVifIdx++;
							
							// update write offset
							ulWriteOffset32++;
							
							// when write offset is updated, must also update ptr
							DstPtr32++;
							
							// check if going to the next row
							//if ( ( ( lVifCodeState - 1 ) & 3 ) == 3 )
							if ( !( ulWriteOffset32 & 3 ) )
							{
								// if just completed a write cycle, then decrement NUM register
								//if ( ulWLCycle < ulWL )
								//{
								//	// NUM register counts number 128-bit QWs written
								//	VifRegs.NUM--;
								//	VifRegs.NUM &= 0xff;
								//}
								
								// update WL,CL
								ulWLCycle++;
								ulCLCycle++;

							}	// end if ( !( ulWriteOffset32 & 3 ) )
							
							// reset both if they are both outside of range
							if ( ( ulWLCycle >= ulWL ) && ( ulCLCycle >= ulCL ) )
							{
								ulWLCycle = 0;
								ulCLCycle = 0;
							}	// end if ( ( ulWLCycle >= ulWL ) && ( ulCLCycle >= ulCL ) )
							
							
							
#ifdef INLINE_DEBUG_UNPACK_MT
	debug << " State=" << dec << lVifCodeState;
#endif
						}	// end while ( lVifCodeState < lVifCommandSize )
						
						if ( iVifCodeState >= iVifCommandSize )
						{
							// command is done
							iVifCodeState = 0;
							
							// for now, clear buffer at command completion even if temporary
							//VifRegs.STAT.FQC = 0;
							
							
							// it is possible that unpacking is done but that there was padding left in the word
							// in that case need to skip the padding and go to the next word
							if ( !ulUnpackItemWidth )
							{
								if ( ulReadCount & 7 )
								{
									//Data++;
									iVifIdx++;
									
									// this is counting the words left in the input
									lWordsLeft--;

								}	// end if ( ulReadCount & 7 )
							}
							else
							{
								 if ( ulReadCount & ( 7 >> ulUnpackItemWidth ) )
								 {
									//Data++;
									iVifIdx++;
									
									// this is counting the words left in the input
									lWordsLeft--;

								 }	// end if ( ulReadCount & ( 7 >> ulUnpackItemWidth ) )

							}	// end if ( !ulUnpackItemWidth )

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " UNPACKDONE";
#endif

#ifdef ENABLE_VERBOSE_MT
					cout << "\n***UNPACK DONE***";
#endif
						}	// end if ( iVifCodeState >= iVifCommandSize )
					
					}	// end if ( iVifCodeState )

					break;
				}
				
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << " INVALIDMT";
#endif
#ifdef INLINE_DEBUG_INVALID_MT
	debug << "\r\nVU#" << Number << " ErrorMT. VU Thread. Invalid VIF Code=" << hex << iVifCode.Value << " Cycle#" << dec << *_DebugCycleCount;
#endif

				cout << "\nhps2x64: VU#" << Number << " Error. VU Thread. Invalid VIF Code=" << hex << iVifCode.Value;

				break;

		}	// end switch ( iVifCode.CMD )

	}	// end while ( lWordsLeft )

	// return number of 32-bit words processed
	return iVifIdx;
}
*/


template<const int VIFREG_MODE, const int VIFCODE_USN, const int VIFCODE_M, const int VIFCODE_VN, const int VIFCODE_VL>
void VU::Execute_UNPACK_AVX2(VU* v, u8*& Data8, uint32_t& lWordsLeft, uint32_t& ulPackedSize)
{
	constexpr const int UNPACK_ITEM_WIDTH = 4 >> VIFCODE_VL;
	constexpr const int UNPACK_NUM = VIFCODE_VN + 1;

	// number of bytes needed to be loaded to output a qw
	// if vl==3, then it is an rgba value
	constexpr const int LOAD_BYTES = (VIFCODE_VL < 3) ? (UNPACK_ITEM_WIDTH * UNPACK_NUM) : 2;

	long lWordsToTransfer, lUnpackData;
	unsigned long ulUnpackCol, ulUnpackRow, ulUnpackMask, ulWriteCycle;
	u32* DstPtr32;

	// need to know total words that were consumed, so need to know where reading starts
	unsigned long ulStartByteCount8, ulWordsRead32;
	unsigned long ulBytesRead8 = 0;

	//u8* Data8;

	__m128i qwVal, qwRowReg, qwColReg, qwRowMask, qwColMask, qwRowWriteMask, qwWriteMask;

	u32 ulMin, ulMax, ulVal, ulMask;
	u64 ullVal64;

	u32 ulReadCount8;

	lWordsToTransfer = (lWordsLeft <= ulPackedSize) ? lWordsLeft : ulPackedSize;
	//lWordsToTransfer = lWordsLeft;

	// make sure that there is data to transfer
	//if (!lWordsToTransfer)
	//{
	//	// no data to transfer
	//	return;
	//}

	ulPackedSize -= lWordsToTransfer;

	DstPtr32 = &v->VuMem32[v->ulWriteOffset32];

	// get the min btw wl/cl
	ulMin = (v->ulWL < v->ulCL) ? v->ulWL : v->ulCL;
	ulMax = (v->ulWL > v->ulCL) ? v->ulWL : v->ulCL;

	// nothing to read if filling write and cl is zero
	if ((v->VifRegs.CYCLE.CL < v->VifRegs.CYCLE.WL) && !v->VifRegs.CYCLE.CL)
	{
		ulReadCount8 = 0;
	}
	else
	{

		// get number of bytes available to read
		ulReadCount8 = lWordsToTransfer << 2;
	}

	// need to know the start value
	ulStartByteCount8 = ulReadCount8;

	// load row register
	qwRowReg = _mm_load_si128(((const __m128i*) & v->VifRegs.R0));



	// note: as long as WL is not zero shouldn't be an issue //
	//if (!v->VifRegs.CYCLE.WL || !v->VifRegs.CYCLE.CL)
	//{
	//	cout << "\nhps2x64: VIF: UNPACK: ERROR: WL is zero!!!";
	//	cout << "\nALERT: VU: UNPACK: WL:" << v->VifRegs.CYCLE.WL << " CL:" << v->VifRegs.CYCLE.CL << " SKIP:" << (v->VifRegs.CYCLE.CL - v->VifRegs.CYCLE.WL) << " SIZE:" << ulPackedSize;
	//	cout << " m:" << v->VifCode.m << " vn:" << v->VifCode.vn << " vl:" << v->VifCode.vl << " USN:" << v->VifCode.USN << " MODE:" << (v->VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
	//	cout << " lWordsToTransfer: " << dec << lWordsToTransfer << " ulPackedSize:" << ulPackedSize << " ulReadCount8:" << ulReadCount8 << " lWordsLeft:" << lWordsLeft << " LOAD_BYTES:" << LOAD_BYTES;
	//	cout << " Data8:" << (u64)Data8 << " lVifCodeState:" << v->lVifCodeState << " lVifCommandSize:" << v->lVifCommandSize << " ulMin" << ulMin << " ulWL" << v->ulWL << " ulCL:" << v->ulCL << " NUM:" << v->VifRegs.NUM;
	//}


	// process //

//#define ALLOW_VU_UNPACK_MEMCPY
#ifdef ALLOW_VU_UNPACK_MEMCPY
	if (!VIFCODE_M && !VIFCODE_VL && (VIFCODE_VN == 3) && (v->ulCL == v->ulWL) && !VIFREG_MODE)
	{
		u32 ulStartOffset, ulQwCount;

		ulStartOffset = v->ulWriteOffset32;

		memcpy(DstPtr32, Data8, lWordsToTransfer << 2);

		Data8 += (lWordsToTransfer << 2);

		v->lVifIdx += lWordsToTransfer;
		v->lVifCodeState += lWordsToTransfer;
		v->ulWriteOffset32 += lWordsToTransfer;

		lWordsLeft -= lWordsToTransfer;

		// get number of full qws transferred in this instance
		ulQwCount = (v->ulWriteOffset32 - (ulStartOffset & ~0x3)) >> 2;


		// NUM register counts number 128-bit QWs written
		//v->VifRegs.NUM--;
		v->VifRegs.NUM -= ulQwCount;
		v->VifRegs.NUM &= 0xff;

		// update WL,CL
		//v->ulWLCycle++;
		//v->ulCLCycle++;
		//v->ulWLCycle = 0;
		//v->ulCLCycle = 0;

		if (v->lVifCodeState >= v->lVifCommandSize)
		{
			// command is done
			v->lVifCodeState = 0;
		}

		// complete for now
		return;
	}
#endif


	// can't test NUM for this outer loop because it could start at zero (which means 256)
	while (v->lVifCodeState < v->lVifCommandSize)
	{
		// check if writing qws from input stream
		while (v->ulWLCycle < ulMin)
		{
			// qws coming from input //

			// get write cycle
			// todo: check if this gets reset after wl ??
			ulWriteCycle = (v->ulWLCycle < 3) ? v->ulWLCycle : 3;


			//cout << "\nCyc:" << dec << ulWriteCycle << " REMN8:" << ulReadCount8;


			// load data //

			if (VIFCODE_VL == 3)
			{
				// rgba16 //

				ullVal64 = *((u16*)Data8);
				ullVal64 = ((ullVal64 & 0x8000) << 16) | ((ullVal64 & 0x7c00) << 9) | ((ullVal64 & 0x3e0) << 6) | ((ullVal64 & 0x1f) << 3);
				//qwVal = _mm_cvtsi64_si128(ullVal64);
			}
			else if (LOAD_BYTES <= 1)
			{
				ullVal64 = *((u8*)Data8);
				//qwVal = _mm_cvtsi64_si128(ullVal64);
			}
			else if (LOAD_BYTES <= 2)
			{
				ullVal64 = *((u16*)Data8);
				//qwVal = _mm_cvtsi64_si128(ullVal64);
			}
			else if (LOAD_BYTES <= 4)
			{
				ullVal64 = *((u32*)Data8);
				//qwVal = _mm_cvtsi64_si128(ullVal64);
			}
			else if (LOAD_BYTES <= 8)
			{
				ullVal64 = *((u64*)Data8);
				//qwVal = _mm_cvtsi64_si128(ullVal64);
			}
			else
			{
				qwVal = _mm_loadu_si128(((const __m128i*)Data8));
			}

			// go ahead and update pointer
			Data8 += LOAD_BYTES;

			//cout << " data: " << hex << ullVal64 << " loadbytes:" << dec << LOAD_BYTES << " lastreadcnt:" << v->ulLastReadByteCount << " lastdata:" << hex << v->ullLastUnpackData;


			// check for remaining data //
			// note: there could be remaining data when LOAD_BYTES > 2 unless LOAD_BYTES == 4
			//if (LOAD_BYTES > 4)
			{
				// possible remaining data //
				if (v->ulLastReadByteCount)
				{
					// data is remaining //

					//cout << "\n***data remaining***\n";

					if (LOAD_BYTES <= 8)
					{
						ullVal64 = (ullVal64 << (v->ulLastReadByteCount << 3)) | v->ullLastUnpackData;
					}
					else
					{

						switch (v->ulLastReadByteCount >> 2)
						{
							// has 1 dw and needs to load at most 3
						case 1:
							// shift qw to left 32
							qwVal = _mm_blend_epi16(_mm_slli_si128(qwVal, 4), v->qwLastUnpackData, 0x03);
							break;

							// has 2 dws and needs to load at most 2
						case 2:
							// shift qw to left 64
							qwVal = _mm_blend_epi16(_mm_slli_si128(qwVal, 8), v->qwLastUnpackData, 0x0f);
							break;

							// has 3 dws and needs to load at most 1
						case 3:
							// shift qw to left 96
							qwVal = _mm_blend_epi16(_mm_slli_si128(qwVal, 12), v->qwLastUnpackData, 0x3f);
							break;
						}
					}

					// update the number of bytes available for read
					ulReadCount8 += v->ulLastReadByteCount;

					// start value was updated
					ulStartByteCount8 = ulReadCount8;

					// backup the pointer to the next unaligned data to read
					Data8 -= v->ulLastReadByteCount;

					// remaining data handled
					v->ulLastReadByteCount = 0;

				}	// end if (v->ulLastReadByteCount)

			}	// end if (LOAD_BYTES > 4)




			// if not all the bytes were available and loaded, then unable to proceed
			if (ulReadCount8 < LOAD_BYTES)
			{
				// set the number of bytes last loaded
				v->ulLastReadByteCount = ulReadCount8;

				// update number of bytes read
				ulBytesRead8 += ulReadCount8;

				// mask remaining amount
				ullVal64 &= (1ull << (ulReadCount8 << 3)) - 1ull;

				// set the remaining amount
				v->ullLastUnpackData = ullVal64;
				v->qwLastUnpackData = qwVal;

				// number of words read/consumed
				//ulWordsRead32 = (ulBytesRead8 >> 2) + 1;

				// update vif idx with the amount of data read
				// note: all the available words of 32-bit data were transferred in this case
				v->lVifIdx += lWordsToTransfer;

				// update the words left
				lWordsLeft -= lWordsToTransfer;
				//lWordsLeft = 0;

				// done until more data is sent
				return;
			}

			// count number of bytes read
			ulBytesRead8 += LOAD_BYTES;

			// load into qw if needed
			if (LOAD_BYTES <= 8)
			{
				qwVal = _mm_cvtsi64_si128(ullVal64);
			}



			// if not 32-bits, then extend
			switch (VIFCODE_VL)
			{
			case 1:

				if (VIFCODE_USN)
				{
					// unsigned 16-bit //
					qwVal = _mm_cvtepu16_epi32(qwVal);
				}
				else
				{
					// signed 16-bit //
					qwVal = _mm_cvtepi16_epi32(qwVal);
				}

				break;

			case 2:

				if (VIFCODE_USN)
				{
					// unsigned 8-bit //
					qwVal = _mm_cvtepu8_epi32(qwVal);
				}
				else
				{
					// signed 8-bit //
					qwVal = _mm_cvtepi8_epi32(qwVal);
				}

				break;

				// rgba16
			case 3:
				qwVal = _mm_cvtepu8_epi32(qwVal);
				break;
			}

			// if vn==0, then broadcast 32-bits (single element)
			if (!VIFCODE_VN)
			{
				qwVal = _mm_broadcastd_epi32(qwVal);
			}


			// the last data read
			v->qwLastUnpackData = qwVal;


			// todo - handle totals (MODE) //
			if (VIFREG_MODE && (VIFCODE_VL != 3))
			{
				// add with row register //
				qwVal = _mm_add_epi32(qwVal, qwRowReg);
			}

			// todo - handle mask (MASK) //
			if (VIFCODE_M)
			{
				// load mask //
				ulMask = (v->VifRegs.MASK >> (ulWriteCycle << 3)) & 0xff;

				//cout << "\nCyc:" << dec << ulWriteCycle << " MskReg:" << hex << v->VifRegs.MASK << " Msk:" << hex << ulMask << " REMN8:" << ulReadCount8;

				// load column reg for write cycle
				qwColReg = _mm_set1_epi32(v->VifRegs.Regs[c_iColRegStartIdx + ulWriteCycle]);
				
				// todo - load row mask //
				qwRowMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_row_avx2_lut[ulMask << 2]);

				//cout << " RM:" << hex << _mm_extract_epi32(qwRowMask, 0) << " " << _mm_extract_epi32(qwRowMask, 1) << " " << _mm_extract_epi32(qwRowMask, 2) << " " << _mm_extract_epi32(qwRowMask, 3);

				// todo - load col mask //
				qwColMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_col_avx2_lut[ulMask << 2]);

				//cout << " CM:" << hex << _mm_extract_epi32(qwColMask, 0) << " " << _mm_extract_epi32(qwColMask, 1) << " " << _mm_extract_epi32(qwColMask, 2) << " " << _mm_extract_epi32(qwColMask, 3);

				// todo - load write mask //
				qwWriteMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_write_avx2_lut[ulMask << 2]);

				//cout << " WM:" << hex << _mm_extract_epi32(qwWriteMask, 0) << " " << _mm_extract_epi32(qwWriteMask, 1) << " " << _mm_extract_epi32(qwWriteMask, 2) << " " << _mm_extract_epi32(qwWriteMask, 3);

				// todo - make rowwrite mask (set if it writes back to row register [mx=0]) //
				qwRowWriteMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_rowwrite_avx2_lut[ulMask << 2]);

				//cout << " RWM:" << hex << _mm_extract_epi32(qwRowWriteMask, 0) << " " << _mm_extract_epi32(qwRowWriteMask, 1) << " " << _mm_extract_epi32(qwRowWriteMask, 2) << " " << _mm_extract_epi32(qwRowWriteMask, 3);

				//cout << " loadbytes:" << dec << LOAD_BYTES << " width:" << UNPACK_ITEM_WIDTH << " num:" << UNPACK_NUM;
				//cout << " qwVal(before):" << hex << _mm_extract_epi32(qwVal, 0) << " " << _mm_extract_epi32(qwVal, 1) << " " << _mm_extract_epi32(qwVal, 2) << " " << _mm_extract_epi32(qwVal, 3);

				// todo - blend with row register //
				qwVal = _mm_blendv_epi8(qwVal, qwRowReg, qwRowMask);

				// todo - blend with column register //
				qwVal = _mm_blendv_epi8(qwVal, qwColReg, qwColMask);

				//cout << " qwVal(after):" << hex << _mm_extract_epi32(qwVal, 0) << " " << _mm_extract_epi32(qwVal, 1) << " " << _mm_extract_epi32(qwVal, 2) << " " << _mm_extract_epi32(qwVal, 3);
			}

			// todo - handle difference mode write-back //
			if ((VIFREG_MODE == 2) && (VIFCODE_VL != 3))
			{
				if (VIFCODE_M)
				{
					// todo - write-back to row register using mask //
					qwRowReg = _mm_blendv_epi8(qwRowReg, qwVal, qwRowWriteMask);
					_mm_maskstore_epi32((int*)& v->VifRegs.R0, qwRowWriteMask, qwRowReg);
				}
				else
				{
					// write-back to row register //
					qwRowReg = qwVal;
					_mm_store_si128((__m128i*) & v->VifRegs.R0, qwRowReg);
				}
			}

			if (VIFCODE_M)
			{
				// todo - write the data using mask //
				_mm_maskstore_epi32((int*)DstPtr32, qwWriteMask, qwVal);
			}
			else
			{
				// write the data to vu mem //
				_mm_store_si128((__m128i*)DstPtr32, qwVal);
			}


			//cout << " qwVal:" << hex << _mm_extract_epi32(qwVal, 0) << " " << _mm_extract_epi32(qwVal, 1) << " " << _mm_extract_epi32(qwVal, 2) << " " << _mm_extract_epi32(qwVal, 3);
			//cout << " num:" << dec << v->VifRegs.NUM;


			// update remaining bytes to read
			ulReadCount8 -= LOAD_BYTES;

			// update write location of next qw
			DstPtr32 += 4;

			// update offset to write
			v->ulWriteOffset32 += 4;

			// update vif idx (should be the amount of data read)
			// todo
			//v->lVifIdx += 4;

			// update code state (amount of data to write, like NUM)
			v->lVifCodeState += 4;

			// update write cycle
			v->ulWLCycle++;

			// reset cycle once it reaches the end
			if (v->ulWLCycle >= ulMax)
			{
				v->ulWLCycle = 0;
			}

			//ulPackedSize -= 4;

			// update #qws remaining to be written
			v->VifRegs.NUM--;
			v->VifRegs.NUM &= 0xff;

			// todo: if NUM is zero then done
			if (!v->VifRegs.NUM)
			{

				// update vif idx with the amount of data read
				// note: all the words to be read must have been read here
				v->lVifIdx += lWordsToTransfer;

				// update the words left
				lWordsLeft -= lWordsToTransfer;

				// command is done
				v->lVifCodeState = 0;

				// update pointer to next 32-bit data
				Data8 += ulReadCount8;
				//Data8 = (Data8 + 3ull) & ~0x3ull;

				//if (!v->VifRegs.CYCLE.WL || !v->VifRegs.CYCLE.CL)
				//{
				//	cout << "\nhps2x64: VIF: UNPACK: !NUM: !WL || !CL";
				//	cout << "\nALERT: VU: UNPACK: WL:" << v->VifRegs.CYCLE.WL << " CL:" << v->VifRegs.CYCLE.CL << " SKIP:" << (v->VifRegs.CYCLE.CL - v->VifRegs.CYCLE.WL) << " SIZE:" << ulPackedSize;
				//	cout << " m:" << v->VifCode.m << " vn:" << v->VifCode.vn << " vl:" << v->VifCode.vl << " USN:" << v->VifCode.USN << " MODE:" << (v->VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
				//	cout << " lWordsToTransfer: " << dec << lWordsToTransfer << " ulPackedSize:" << ulPackedSize << " ulReadCount8:" << ulReadCount8 << " lWordsLeft:" << lWordsLeft << " LOAD_BYTES:" << LOAD_BYTES;
				//	cout << " Data8:" << (u64)Data8;
				//}

				return;
			}

		}

		while (v->ulWLCycle < v->ulWL)
		{
			// read suspended but data still written //
			// todo - filling write //

			// get write cycle
			// todo: check if this gets reset after wl ??
			ulWriteCycle = (v->ulWLCycle < 3) ? v->ulWLCycle : 3;

			// start with the last data read
			qwVal = v->qwLastUnpackData;


			// todo - handle mask (MASK) //
			if (VIFCODE_M)
			{
				// load mask //
				ulMask = (v->VifRegs.MASK >> (ulWriteCycle << 3)) & 0xff;

				//cout << "\nCyc:" << dec << ulWriteCycle << " MskReg:" << hex << v->VifRegs.MASK << " Msk:" << hex << ulMask << " REMN8:" << ulReadCount8;

				// load column reg for write cycle
				qwColReg = _mm_set1_epi32(v->VifRegs.Regs[c_iColRegStartIdx + ulWriteCycle]);

				// todo - load row mask //
				qwRowMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_row_avx2_lut[ulMask << 2]);

				//cout << " RM:" << hex << _mm_extract_epi32(qwRowMask, 0) << " " << _mm_extract_epi32(qwRowMask, 1) << " " << _mm_extract_epi32(qwRowMask, 2) << " " << _mm_extract_epi32(qwRowMask, 3);

				// todo - load col mask //
				qwColMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_col_avx2_lut[ulMask << 2]);

				//cout << " CM:" << hex << _mm_extract_epi32(qwColMask, 0) << " " << _mm_extract_epi32(qwColMask, 1) << " " << _mm_extract_epi32(qwColMask, 2) << " " << _mm_extract_epi32(qwColMask, 3);

				// todo - load write mask //
				qwWriteMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_write_avx2_lut[ulMask << 2]);

				//cout << " WM:" << hex << _mm_extract_epi32(qwWriteMask, 0) << " " << _mm_extract_epi32(qwWriteMask, 1) << " " << _mm_extract_epi32(qwWriteMask, 2) << " " << _mm_extract_epi32(qwWriteMask, 3);

				// todo - make rowwrite mask (set if it writes back to row register [mx=0]) //
				qwRowWriteMask = _mm_load_si128((const __m128i*) & ps2_arr_mask_rowwrite_avx2_lut[ulMask << 2]);

				//cout << " RWM:" << hex << _mm_extract_epi32(qwRowWriteMask, 0) << " " << _mm_extract_epi32(qwRowWriteMask, 1) << " " << _mm_extract_epi32(qwRowWriteMask, 2) << " " << _mm_extract_epi32(qwRowWriteMask, 3);

				//cout << " loadbytes:" << dec << LOAD_BYTES << " width:" << UNPACK_ITEM_WIDTH << " num:" << UNPACK_NUM;
				//cout << " qwVal(before):" << hex << _mm_extract_epi32(qwVal, 0) << " " << _mm_extract_epi32(qwVal, 1) << " " << _mm_extract_epi32(qwVal, 2) << " " << _mm_extract_epi32(qwVal, 3);

				// todo - blend with row register //
				qwVal = _mm_blendv_epi8(qwVal, qwRowReg, qwRowMask);

				// todo - blend with column register //
				qwVal = _mm_blendv_epi8(qwVal, qwColReg, qwColMask);

				//cout << " qwVal(after):" << hex << _mm_extract_epi32(qwVal, 0) << " " << _mm_extract_epi32(qwVal, 1) << " " << _mm_extract_epi32(qwVal, 2) << " " << _mm_extract_epi32(qwVal, 3);

			}


			// todo - write data //
			if (VIFCODE_M)
			{
				// todo - write the data using mask //
				_mm_maskstore_epi32((int*)DstPtr32, qwWriteMask, qwVal);
			}
			else
			{
				// write the data to vu mem //
				_mm_store_si128((__m128i*)DstPtr32, qwVal);
			}


			// update write location of next qw
			DstPtr32 += 4;

			// update code state (amount of data to write, like NUM)
			v->lVifCodeState += 4;

			// update write cycle
			v->ulWLCycle++;

			//ulPackedSize -= 4;

			// update #qws remaining to be written
			v->VifRegs.NUM--;
			v->VifRegs.NUM &= 0xff;

			// todo: if NUM is zero then done
			if (!v->VifRegs.NUM)
			{
				// update vif idx with the amount of data read
				v->lVifIdx += lWordsToTransfer;

				// update the words left
				lWordsLeft -= lWordsToTransfer;

				// command is done
				v->lVifCodeState = 0;

				// update pointer to next 32-bit data
				Data8 += ulReadCount8;
				//Data8 = (Data8 + 3ull) & ~0x3ull;

				return;
			}
		}


		if (v->ulWLCycle < v->ulCL)
		{
			// skipping write //

			// skip write cycle
			//v->ulWLCycle += v->ulCL - v->ulWL;
			v->ulWLCycle = 0;

			// update write location of qw
			DstPtr32 += (v->ulCL - v->ulWL) << 2;

			// update offset to write
			v->ulWriteOffset32 += (v->ulCL - v->ulWL) << 2;

			// update pointer to next 32-bit data
			//Data8 += ulReadCount8;
			//Data8 = (Data8 + 3ull) & ~0x3ull;
		}

		// reset cycle once it reaches the end
		if (v->ulWLCycle >= ulMax)
		{
			v->ulWLCycle = 0;
		}

	}	// end while(v->lVifCodeState < v->lVifCommandSize)


	// update vif idx with the amount of data read
	v->lVifIdx += lWordsToTransfer;

	// update the words left
	lWordsLeft -= lWordsToTransfer;

	// update pointer to next 32-bit data
	Data8 += ulReadCount8;
	//Data8 = (Data8 + 3ull) & ~0x3ull;

}

// returns lWordsLeft
template<const int VIFREG_MODE, const int VIFCODE_USN, const int VIFCODE_M,const int VIFCODE_VN, const int VIFCODE_VL>
void VU::Execute_UNPACK(VU* v, u32* &Data, uint32_t &lWordsLeft, uint32_t &ulPackedSize)
{
	constexpr const int UNPACK_ITEM_WIDTH = 4 >> VIFCODE_VL;
	constexpr const int UNPACK_NUM = VIFCODE_VN + 1;

	long lWordsToTransfer, lUnpackData;
	unsigned long ulUnpackCol, ulUnpackRow, ulUnpackMask, ulWriteCycle;
	u32* DstPtr32;


	if (v->lVifCodeState)
	{
		lWordsToTransfer = (lWordsLeft <= ulPackedSize) ? lWordsLeft : ulPackedSize;
		ulPackedSize -= lWordsToTransfer;

		//if ( Number && ulThreadCount )
		//{
		//	CopyTo_CommandBuffer ( Data, lWordsToTransfer );
		//	lWordsLeft -= lWordsToTransfer;
		//	lVifIdx += lWordsToTransfer;

		//	Data += lWordsToTransfer;

		//	if ( !ulPackedSize )
		//	{
		//		lVifCodeState = lVifCommandSize;

		//		//ulUnpackItemWidth = 0;
		//		//ulReadCount = 0;
		//	}
		//}
		//else
		{

			// get dst pointer
			DstPtr32 = &v->VuMem32[v->ulWriteOffset32];


			if (!VIFCODE_M && !VIFCODE_VL && (VIFCODE_VN == 3) && (v->ulCL == v->ulWL) && !VIFREG_MODE)
			{
				u32 ulStartOffset, ulQwCount;

				ulStartOffset = v->ulWriteOffset32;

				memcpy(DstPtr32, Data, lWordsToTransfer << 2);

				Data += lWordsToTransfer;

				v->lVifIdx += lWordsToTransfer;
				v->lVifCodeState += lWordsToTransfer;
				v->ulWriteOffset32 += lWordsToTransfer;

				lWordsLeft -= lWordsToTransfer;

				// get number of full qws transferred in this instance
				ulQwCount = (v->ulWriteOffset32 - (ulStartOffset & ~0x3)) >> 2;


				// NUM register counts number 128-bit QWs written
				//v->VifRegs.NUM--;
				v->VifRegs.NUM -= ulQwCount;
				v->VifRegs.NUM &= 0xff;

				// update WL,CL
				//v->ulWLCycle++;
				//v->ulCLCycle++;
				//v->ulWLCycle = 0;
				//v->ulCLCycle = 0;

				if (v->lVifCodeState >= v->lVifCommandSize)
				{
					// command is done
					v->lVifCodeState = 0;
				}

				// complete for now
				return;
			}


			//while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
			while (v->lVifCodeState < v->lVifCommandSize)
			{
				// first determine where data should be read from //
				// WL must be within range before data is read
				if (v->ulWLCycle < v->ulWL)
				{
					// WL is within range
					// so we must read data from someplace

					// get Write Cycle
					// *todo* unsure if the "Write Cycle" is reset after CL or WL or both ??
					ulWriteCycle = v->ulWLCycle;

					// get unpack row/col
					ulUnpackRow = ((ulWriteCycle < 4) ? ulWriteCycle : 3);
					ulUnpackCol = v->ulWriteOffset32 & 3;

					// change#5 - reset unpack count for column at row start
					//if (!ulUnpackCol)
					//{
					//	//ulUnpackCount = UNPACK_NUM;
					//}

					// data source is determined by m and MASK //

					// change#1 - can get the mask sooner
					// if m bit is 0, then mask is zero
					ulUnpackMask = 0;

					if (VIFCODE_M || (v->ulCLCycle >= v->ulCL))
					{
						// mask comes from MASK register //


						// mask index is (Row*4)+Col
						// but if the row (write cycle??) is 3 or 4 or greater then just use maximum row value
						ulUnpackMask = (v->VifRegs.MASK >> (((ulUnpackRow << 2) + ulUnpackCol) << 1)) & 3;

						// determine source of data
						switch (ulUnpackMask)
						{
							//case 0:
							//	// data comes from dma/ram //
							//	lUnpackData = v->lUnpackLastData;
							//	break;

						case 1:
							// data comes from ROW register //
							lUnpackData = v->VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
							break;

						case 2:
							// data comes from COL register //
							lUnpackData = v->VifRegs.Regs[c_iColRegStartIdx + ulUnpackRow];
							break;

							//case 3:
							//	// write is masked/not happening/prevented //
							//	break;
						}


#ifdef INLINE_DEBUG_UNPACK_2
						debug << "\r\nRow#" << ulUnpackRow << " Col#" << ulUnpackCol << " TO " << hex << ulWriteOffset32 << " FullMsk=" << hex << VifRegs.MASK << " Shift=" << dec << (((ulUnpackRow << 2) + ulUnpackCol) << 1);
#endif
					}	// end if (VIFCODE_M || (v->ulCLCycle >= v->ulCL))


					// to read data from dma/memory, CL must be within range, and Col must be within correct range
					if ((v->ulCLCycle < v->ulCL) && (ulUnpackCol < UNPACK_NUM))
					{
						// data source is dma/ram //

						// change#6 - update unpack count for the current row
						//ulUnpackCount--;

						// make sure that there is data left to read
						if (lWordsLeft <= 0)
						{
							// there is no data to read //

							// return sequence when lVifCodeState is NOT zero //
							break;
						}

						// determine data element size
						if (!UNPACK_ITEM_WIDTH)
						{
							// must be unpacking pixels //
							// *todo* check for vn!=3 which would mean it is not unpacking pixels but rather an invalid command

							// similar to unpacking 16-bit halfwords //

							if (!(v->ulReadCount & 7))
							{
								v->ulUnpackBuf = *Data++;

								// get the correct element of 32-bit word
								//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
								//lUnpackTemp &= 0xffff;
							}


							// pull correct component
							if (ulUnpackCol < 3)
							{
								// RGB
								v->lUnpackLastData = v->ulUnpackBuf & 0x1f;
								v->lUnpackLastData <<= 3;

								v->ulUnpackBuf >>= 5;
							}
							else
							{
								// A
								v->lUnpackLastData = v->ulUnpackBuf & 1;
								v->lUnpackLastData <<= 7;

								v->ulUnpackBuf >>= 1;
							}

							v->ulReadCount++;

							// one pixel gets unpacked per 8 components RGBA
							if (!(v->ulReadCount & 7))
							{
								//Data++;
								v->lVifIdx++;

								// this is counting the words left in the input
								lWordsLeft--;

#ifdef INLINE_DEBUG_UNPACK
								debug << " VifIdx=" << dec << lVifIdx << " WordsLeft=" << lWordsLeft;
#endif
							}
						}
						else
						{
							// unpacking NON-pixels //

							// check if only 1 element (vn=0) and not first col
							if (VIFCODE_VN || !ulUnpackCol)
							{
								if (!(v->ulReadCount & (7 >> UNPACK_ITEM_WIDTH)))
								{
									v->ulUnpackBuf = *Data++;

									// get the correct element of 32-bit word
									//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
									//lUnpackTemp &= 0xffff;
								}


								v->lUnpackLastData = v->ulUnpackBuf & (0xffffffffUL >> ((4 - UNPACK_ITEM_WIDTH) << 3));

								v->ulUnpackBuf >>= (UNPACK_ITEM_WIDTH << 3);

								// check if data item is signed and 1 or 2 bytes long
								if ((!VIFCODE_USN) && ((UNPACK_ITEM_WIDTH == 1) || (UNPACK_ITEM_WIDTH == 2)))
								{
									// signed data //
									v->lUnpackLastData <<= ((4 - UNPACK_ITEM_WIDTH) << 3);
									v->lUnpackLastData >>= ((4 - UNPACK_ITEM_WIDTH) << 3);
								}

								v->ulReadCount++;

								if (!(v->ulReadCount & (7 >> UNPACK_ITEM_WIDTH)))
								{
									//Data++;
									v->lVifIdx++;

									// command size is 1+NUM*2 words
									// this is counting the words left in the input, but should ALSO be counting words left in output
									lWordsLeft--;

#ifdef INLINE_DEBUG_UNPACK
									debug << " VifIdx=" << dec << lVifIdx << " WordsLeft=" << lWordsLeft;
#endif
								}

							} // end if (VIFCODE_VN || !ulUnpackCol)

						} // end if ( !ulUnpackItemWidth ) else



					} // end if ((v->ulCLCycle < v->ulCL) && (ulUnpackCol < UNPACK_NUM))




					// for adding/totalling with row register, you need either m=0 or m[x]=0
					if (!VIFCODE_M || !ulUnpackMask)
					{
						// 1 means add, 2 means total

						// data comes from dma/ram //
						lUnpackData = v->lUnpackLastData;

						switch (VIFREG_MODE)
						{
						case 0:
							break;

						case 1:
#ifdef INLINE_DEBUG_UNPACK_2
							debug << " VALUE: " << hex << lUnpackData;
							debug << " ADD:" << hex << VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
#endif

							// add to unpacked data //

							// I guess the add should be an integer one?
							lUnpackData += v->VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];

#ifdef INLINE_DEBUG_UNPACK_2
							debug << " RESULT:" << hex << lUnpackData;
#endif

							break;

						case 2:
#ifdef INLINE_DEBUG_UNPACK_2
							debug << " VALUE: " << hex << lUnpackData;
							debug << " TOTAL:" << hex << VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
#endif

							// add to unpacked data and write result back to ROW register //

							// I guess the add/total should be an integer one?
							lUnpackData += v->VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
							v->VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol] = lUnpackData;

#ifdef INLINE_DEBUG_UNPACK_2
							debug << " RESULT:" << hex << lUnpackData;
							debug << " TOTAL:" << hex << VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
#endif

							break;


							// undocumented ??
						case 3:
#ifdef INLINE_DEBUG_UNPACK_2
							debug << " UNDOCUMENTED";
#endif

							v->VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol] = lUnpackData;
							break;

						}	// end switch (VIFREG_MODE)

					} // end if (!VIFCODE_M || !ulUnpackMask)


#ifdef DISABLE_UNPACK_INDETERMINATE
								// only write data if the column is within the amount being written, or it is masked
								// change #3 - always outputs data unless masked
								//if ( ( UNPACK_NUM == 1 ) || ( ulUnpackMask ) || ( ulUnpackCol < UNPACK_NUM ) )
								//if ( ( UNPACK_NUM == 1 ) || ( ulUnpackCol < UNPACK_NUM ) )
					{
#endif

						// write data to vu memory if write is not masked //
						if (ulUnpackMask != 3)
						{
#ifdef INLINE_DEBUG_UNPACK_2
							debug << "\r\n";
							if ((ulUnpackCol >= UNPACK_NUM) && (UNPACK_NUM != 1))
							{
								debug << "[INDT] ";
							}
							debug << "Col#" << ulUnpackCol << " Data=" << hex << lUnpackData << " TO " << ulWriteOffset32 << "(SA=" << (ulWriteOffset32 >> 2) << ")" << " Msk=" << ulUnpackMask;
#endif

							*DstPtr32 = lUnpackData;

#ifdef INLINE_DEBUG_UNPACK_3
							debug << " VuMem=" << hex << VuMem32[ulWriteOffset32];
#endif
						}
#ifdef INLINE_DEBUG_UNPACK_2
						else
						{
							//debug << "\r\nCol#" << ulUnpackCol << " MSKD TO " << hex << ulWriteOffset32 << "(SA=" << (ulWriteOffset32>>2) << ")" << " Msk=" << ulUnpackMask;
							debug << "\r\nCol#" << ulUnpackCol << " DATA:" << hex << lUnpackData << " MSKD TO " << hex << ulWriteOffset32 << " Msk=" << ulUnpackMask;
						}
#endif

#ifdef DISABLE_UNPACK_INDETERMINATE
					} // end if ( ( UNPACK_NUM == 1 ) || ( ulUnpackCol < UNPACK_NUM ) )
#ifdef INLINE_DEBUG_UNPACK_2
//								else
//								{
//	debug << "\r\nCol#" << ulUnpackCol << " UNDT TO " << hex << ulWriteOffset32 << "(SA=" << (ulWriteOffset32>>2) << ")";
//								}
#endif

#endif

								// move to next data input element
								// this should update per 32-bit word that is written into VU memory
								// WL is a write cycle
					v->lVifCodeState++;

				} // end if ( ulWLCycle < ulWL )
				//else
				//{
				//	// skip part of skipping write //
				//}


				// store ROW (R0-R3)
				// register offset starts at 16, buf lVifCodeState will start at 1
				//*DstPtr32++ = *Data++;

				// update index
				//lVifIdx++;

				// update write offset
				v->ulWriteOffset32++;

				// when write offset is updated, must also update ptr
				DstPtr32++;

				// check if going to the next row
				//if ( ( ( lVifCodeState - 1 ) & 3 ) == 3 )
				if (!(v->ulWriteOffset32 & 3))
				{
					// if just completed a write cycle, then decrement NUM register
					if (v->ulWLCycle < v->ulWL)
					{
						// NUM register counts number 128-bit QWs written
						v->VifRegs.NUM--;
						v->VifRegs.NUM &= 0xff;
					}

					// update WL,CL
					v->ulWLCycle++;
					v->ulCLCycle++;
				}

				// reset both if they are both outside of range
				if ((v->ulWLCycle >= v->ulWL) && (v->ulCLCycle >= v->ulCL))
				{
					v->ulWLCycle = 0;
					v->ulCLCycle = 0;
				}



#ifdef INLINE_DEBUG_UNPACK
				debug << " State=" << dec << lVifCodeState;
#endif
			}	// end while ( lVifCodeState < lVifCommandSize )

		}	// end if ( Number && ulThreadCount )

		if (v->lVifCodeState >= v->lVifCommandSize)
		{
			// command is done
			v->lVifCodeState = 0;

			// for now, clear buffer at command completion even if temporary
			//VifRegs.STAT.FQC = 0;


			// this next part is only if running on the same thread
			if (!v->Number || !v->ulThreadCount)
			{
				// it is possible that unpacking is done but that there was padding left in the word
				// in that case need to skip the padding and go to the next word
				if (!UNPACK_ITEM_WIDTH)
				{
					if (v->ulReadCount & 7)
					{
						//Data++;
						v->lVifIdx++;

						// this is counting the words left in the input
						lWordsLeft--;
					}
				}
				else
				{
					if (v->ulReadCount & (7 >> UNPACK_ITEM_WIDTH))
					{
						//Data++;
						v->lVifIdx++;

						// this is counting the words left in the input
						lWordsLeft--;
					}
				}
			}

		}	// end if ( lVifCodeState >= lVifCommandSize )

	}	// end if (v->lVifCodeState)

}

// VIF Codes are 32-bits. can always do a cast to 64/128-bits as needed
// need this to return the number of quadwords read and update the offset so it points to correct data for next time
u32 VU::VIF_FIFO_Execute ( u32* Data, u32 SizeInWords32 )
{
	static const int c_iSTROW_CommandSize = 5;
	static const int c_iSTCOL_CommandSize = 5;

	u32 ulTemp;
	u32 *DstPtr32;
	
	u32 lWordsToTransfer;
	
	u32 QWC_Transferred;
	s32 UnpackX, UnpackY, UnpackZ, UnpackW;
	//s32 lUnpackTemp;
	union { s32 lUnpackData; float fUnpackData; };
	u32 ulUnpackMask;
	u32 ulUnpackRow, ulUnpackCol;
	u32 ulWriteCycle;
	
	u64 PreviousValue;
	u64 NewValue;
	
	// need an index for just the data being passed
	u32 lIndex = 0;
	
	u32 lWordsLeft = SizeInWords32;

#ifdef INLINE_DEBUG_VUCOM
	debug << "\r\nVU#" << Number << "::VIF_FIFO_Execute ";
#endif

	// also need to subtract lVifIdx & 0x3 from the words left
	// to take over where we left off at
	lWordsLeft -= ( lVifIdx & 0x3 );
	
#ifdef INLINE_DEBUG_VUCOM
	debug << " lVifIdx=" << hex << lVifIdx;
#endif

	// start reading from index offset (may have left off in middle of data QW block)
	Data = & ( Data [ lVifIdx & 0x3 ] );
	
	// TESTING
	// this should mean that VIF is busy
	//VifRegs.STAT.VPS = 0x2;
	
	while ( lWordsLeft )
	{
		// for now, update amount of data left in buffer
		//VifRegs.STAT.FQC = ( lWordsLeft + 3 ) >> 2;
		
		// check if vif code processing is in progress
		if ( !lVifCodeState )
		{
			// load vif code
			//VifCode.Value = Data [ lIndex++ ];
			VifCode.Value = *Data++;
			
			// store to CODE register
			VifRegs.CODE = VifCode.Value;
			
			// is now busy transferring data after vif code
			VifRegs.STAT.VPS = 3;
			
			// update index
			lVifIdx++;
			
			// vif code is 1 word
			lWordsLeft--;
		}
		
		// perform the action for the vif code
		switch ( VifCode.CMD )
		{
			// NOP
			case 0:
#ifdef INLINE_DEBUG_VUCOM
	debug << " NOP";
#endif

				if (BusyUntil_Cycle < *_DebugCycleCount)
				{
					BusyUntil_Cycle = *_DebugCycleCount;
				}

				BusyUntil_Cycle += 1ull << 0;

				break;
			
			// STCYCL
			case 1:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STCYCL";
#endif

				//if ( Number && ulThreadCount )
				//{
				//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
				//}

				// set CYCLE register value - first 8 bits are CL, next 8-bits are WL
				// set from IMM
				// Vu0 and Vu1
				VifRegs.CYCLE.Value = VifCode.IMMED;

				break;

			// OFFSET
			case 2:
#ifdef INLINE_DEBUG_VUCOM
	debug << " OFFSET";
#endif

				// wait if vu is running ??
				if ( Running )
				{
					// a VU program is in progress already //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;

					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (OFFSET_PENDING)";
#endif

					// there is a pending MSCAL Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
				
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					// transfer to vif is no longer stopped
					VifStopped = 0;

					// vif no longer waiting for end of program
					VifWaiting = 0;

					//if ( Number && ulThreadCount )
					//{
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}

					// set OFFSET register value - first 10 bits of IMM
					// set from IMM
					// Vu1 ONLY
					VifRegs.OFST = VifCode.IMMED;
				
					// set DBF Flag to zero
					VifRegs.STAT.DBF = 0;
					
				
					// set TOPS to BASE
					// *note* should probably use DBF to see what this gets set to
					VifRegs.TOPS = VifRegs.BASE;


#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (OFFSET=" << VifRegs.OFST << " TOPS=" << VifRegs.TOPS << ")";
#endif

				}

				break;

			// BASE
			case 3:
#ifdef INLINE_DEBUG_VUCOM
	debug << " BASE";
#endif

				//if ( Number && ulThreadCount )
				//{
				//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
				//}

				// set BASE register value - first 10 bits of IMM
				// set from IMM
				// Vu1 ONLY
				VifRegs.BASE = VifCode.IMMED;


#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (BASE=" << VifRegs.BASE << ")";
#endif
				break;

			// ITOP
			case 4:
#ifdef INLINE_DEBUG_VUCOM
	debug << " ITOP";
#endif

				//if ( Number && ulThreadCount )
				//{
				//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
				//}

				// set data pointer (ITOPS register) - first 10 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				VifRegs.ITOPS = VifCode.IMMED;


				break;

			// STMOD
			case 5:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STMOD";
#endif

				//if ( Number && ulThreadCount )
				//{
				//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
				//}

				// set mode of addition decompression (MODE register) - first 2 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				VifRegs.MODE = VifCode.IMMED;


				break;

			// MSKPATH3
			case 6:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSKPATH3";
#endif

				// set mask of path3 - bit 15 - 0: enable path3 transfer, 1: disable path3 transfer
				// set from IMM
				// Vu1 ONLY
				
				PreviousValue = GPU::_GPU->GIFRegs.STAT.M3P;
				
				// set bit 15 of mskpath3 to bit 1 of gif_stat (M3P)
				//GPU::_GPU->GIFRegs.STAT.M3P = ( ( VifCode.Value & ( 1 << 15 ) ) >> 15 );
				NewValue = ( ( VifCode.Value & ( 1 << 15 ) ) >> 15 );
				
				//if ( PreviousValue )
				if (!NewValue)
				{
#ifdef INLINE_DEBUG_VUCOM
					debug << "\r\n***ALLOW PATH3 M3P=0***";
#endif

					//if (!NewValue)
					{
						// set new value (zero)
						GPU::_GPU->GIFRegs.STAT.M3P = 0;

						// check for a transition from 1 to 0
						if (!GPU::_GPU->GIFRegs.STAT.M3R)
						{
							// path 3 mask is being disabled //
#ifdef INLINE_DEBUG_VUCOM
							debug << "\r\n*** PATH3 BEING UN-MASKED VIA VU ***\r\n";
#endif
#ifdef VERBOSE_MSKPATH3
							cout << "\n*** PATH3 BEING UN-MASKED VIA VU ***\n";
#endif

							// set new value (zero)

							// as long as path 1 is not in progress ??
							if (GPU::_GPU->GIFRegs.STAT.APATH != 1)
							{
								// obviously path 3 is not in queue
								GPU::_GPU->GIFRegs.STAT.P3Q = 0;

								// obviously path 3 not currently interrupted
								GPU::_GPU->GIFRegs.STAT.IP3 = 0;
							}

							// this affects gpu since data in the buffer can now process
							GPU::Update_Device();

							// check transfer
							Dma::_DMA->CheckTransfer();

						}	// end if ( !GPU::_GPU->GIFRegs.STAT.M3R && !GPU::_GPU->GIFRegs.STAT.M3P )

					}


				}	// end if ( PreviousValue )
				
				//if ( !PreviousValue )
				else
				{
#ifdef INLINE_DEBUG_VUCOM
					debug << "\r\n***MASK PATH3 M3P=1 P2E=" << GPU::_GPU->EndOfPacket[2] << " P3E=" << GPU::_GPU->EndOfPacket[3] << " DMA#2.STR=" << Dma::pRegData[2]->CHCR.STR << " ***";
#endif

					//if ( GPU::_GPU->GIFRegs.STAT.M3P )
					if (NewValue)
					{
#ifdef INLINE_DEBUG_VUCOM
						debug << "\r\n*** PATH3 BEING MASKED VIA VU ***\r\n";
#endif
#ifdef VERBOSE_MSKPATH3
						cout << "\n*** PATH3 BEING MASKED VIA VU ***\n";
#endif


						GPU::_GPU->GIFRegs.STAT.M3P = 1;

						// vif now running
						VifStopped = 0;

#ifndef DISABLE_PATH3_MASK

						/*
						// if path 3 is masked (need to clear condition when m3p cleared)
						//if ( GPU::_GPU->GIFRegs.STAT.M3P )
						//if ( GPU::_GPU->GIFRegs.STAT.M3R || NewValue )
						if (GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P)
						{

#ifndef ENABLE_PATH3_MASK_NO_CONDITION

							// if path 3 is currently in progress and not at the end of a packet
							// (need to clear condition on end of dma#2 or end of path 3 packet)
							//if ( Dma::pRegData [ 2 ]->CHCR.STR )
							//if ( Dma::pRegData [ 2 ]->CHCR.STR && ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
							//if (Dma::pRegData[2]->CHCR.STR && (!GPU::_GPU->EndOfPacket[3]) && (!GPU::_GPU->GIFRegs.MODE.IMT || GPU::_GPU->GIFTag0[3].FLG != 2))
#ifdef ENABLE_PATH3_MASK_CONDITION_END_TRANSFER
							if (Dma::pRegData[2]->CHCR.STR)
#endif
#ifdef ENABLE_PATH3_MASK_CONDITION_END_PACKET
							if (!GPU::_GPU->EndOfPacket[3])
#endif
							{
#ifdef INLINE_DEBUG_VUCOM
								debug << " (VIFSTOP)";
#endif


								VifStopped = 1;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSKPATH3_PENDING)";
#endif

								// there is a pending Command -> for now just back up index
								lVifIdx--;

								// just return for now //

								// get the number of full quadwords completely processed
								QWC_Transferred = lVifIdx >> 2;

								// get start index of remaining data to process in current block
								lVifIdx &= 0x3;

								// return the number of full quadwords transferred/processed
								return QWC_Transferred;

							}	// end if ( Dma::pRegData [ 2 ]->CHCR.STR )

#endif	// end ENABLE_PATH3_MASK_NO_CONDITION


						// vif now running
							VifStopped = 0;

						}	// end if ( GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P )
						*/

#endif // end DISABLE_PATH3_MASK


					}

				}	// end if ( !PreviousValue )


				
				break;

			// MARK
			case 7:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MARK";
#endif

#ifdef VERBOSE_VU_MARK
				// debug
				cout << "\nhps2x64 ALERT: VU: Mark encountered!!!\n";
#endif
				
				// set mark register
				// set from IMM
				// Vu0 and Vu1
				VifRegs.MARK = VifCode.IMMED;
				
				// also need to set MRK bit in STAT
				// this gets cleared when CPU writes to MARK register
				VifRegs.STAT.MRK = 1;
				
				break;

			// FLUSHE
			case 16:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSHE";
#endif

				// waits for vu program to finish
				// Vu0 and Vu1
				
				if ( Running )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (FLUSHE_PENDING)";
#endif

					// there is a pending FLUSHE Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;

					// vif no longer waiting for end of program
					VifWaiting = 0;

					//if ( Number && ulThreadCount )
					//{
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}
				}
				
				break;

			// FLUSH
			case 17:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSH";
#endif

				// waits for end of vu program and end of path1/path2 transfer
				// Vu1 ONLY
				// ***todo*** wait for end of path1/path2 transfer
				if ( (Running) || (GPU::_GPU->GIFRegs.STAT.APATH == 1) )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (FLUSH_PENDING)";
#endif

					// there is a pending FLUSH Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;

					// vif no longer waiting for end of program
					VifWaiting = 0;

					//if ( Number && ulThreadCount )
					//{
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}

#ifdef ENABLE_SET_VGW
					// not waiting for end of GIF transfer currently
					VifRegs.STAT.VGW = 0;
#endif


				}
				
				break;

			// FLUSHA
			case 19:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSHA";
#endif

				// waits for end of vu program and end of transfer to gif
				// Vu1 ONLY
				// ***todo*** wait for end of transfer to gif
				//if ( Running || ( Dma::cbReady [ 2 ] () ) )
				//if ( Running || ( Dma::pRegData [ 2 ]->CHCR.STR && !( GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P ) ) )
				
	
	
	
				//if ( ( Running ) || ( Dma::pRegData [ 2 ]->CHCR.STR ) || ( GPU::_GPU->GIFRegs.STAT.APATH == 1 ) )
				//if ( (Running) || ( Dma::pRegData [ 2 ]->CHCR.STR && GPU::DMA_Write_Ready() ) || (GPU::_GPU->GIFRegs.STAT.APATH == 1) )
				//if ( Running || ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
				//if (Running || (Dma::pRegData[2]->CHCR.STR && !GPU::_GPU->EndOfPacket[3]) || (GPU::_GPU->GIFRegs.STAT.APATH == 1))
				//if (Running || (GPU::_GPU->ulTransferCount[3] && Dma::pRegData[2]->CHCR.STR) || (GPU::_GPU->GIFRegs.STAT.APATH == 1))
				if (Running || (!GPU::_GPU->EndOfPacket[3]) || (GPU::_GPU->GIFRegs.STAT.APATH == 1))
				//if ( Running || ( GPU::_GPU->ulTransferCount [ 3 ] && Dma::pRegData [ 2 ]->CHCR.STR ) || ( GPU::_GPU->GIFRegs.STAT.APATH == 1 ) )
				//if ((Running) || (GPU::_GPU->GIFRegs.STAT.APATH == 1) || (GPU::_GPU->GIFRegs.STAT.APATH == 3))
				//if ( ( Running )
				//	|| (GPU::_GPU->GIFRegs.STAT.APATH == 1)
				//	|| ()
				//	|| ((GPU::_GPU->GIFRegs.STAT.APATH == 3) && !( (GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P) && (GPU::_GPU->EndOfPacket[3]) ) )
				//	)
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

#ifdef ENABLE_SET_VGW
					//if ( Dma::pRegData [ 2 ]->CHCR.STR && !( GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P ) )
					if ( GPU::_GPU->ulTransferCount [ 3 ] )
					//if ( !GPU::_GPU->EndOfPacket [ 3 ] )
					{
						// when vif is stalled via DIRECTHL or FLUSHA, need to set STAT.VGW
						VifRegs.STAT.VGW = 1;
					}
#endif
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (FLUSHA_PENDING)";
	debug << " P3C=" << hex << GPU::_GPU->ulTransferCount [ 3 ];
	debug << " VU1Running=" << hex << Running;
#endif

					// there is a pending FLUSHA Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

					//if ( Number && ulThreadCount )
					//{
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}

#ifdef ENABLE_SET_VGW
					// not waiting for end of GIF transfer currently
					VifRegs.STAT.VGW = 0;
#endif


				}
				
				break;

			// MSCAL
			// MT
			case 20:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCAL";
#endif

				// waits for end of current vu program and runs new vu program starting at IMM*8
				// Vu0 and Vu1
				
				if ( Running )
				{
					// a VU program is in progress already //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCAL_PENDING)";
#endif

					// there is a pending MSCAL Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //

#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// VU is now running
					StartVU ();
					
					// if just starting vu, then no longer multi-threading either
					//ullMultiThread_StartCycle = -1LL;
					
#ifdef ENABLE_PATH1_BUFFER
					//GPU::Path1_ProcessBlocks ();
#endif
					
					// start multi-threading from when vu is started
					//ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					
					// ***todo*** also set VPU STAT COP2 r29 ??

					// set TOP to TOPS
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );

					// note: if running on another thread want to send command and:
					//if ( Number && ulThreadCount )
					//{
					//	// vu events are handled differently if the command is in the cmd buffer
					//	bCurrentVuRunInCmdBuffer = true;
					//
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}
					//else
					{
						// vu events are handled differently if the command is in the cmd buffer
						//bCurrentVuRunInCmdBuffer = false;

						// set PC = IMM * 8
						PC = VifCode.IMMED << 3;
						
						// need to set next pc too since this could get called in the middle of VU Main CPU loop
						NextPC = PC;

#ifdef ENABLE_VU_MULTI_EXECUTE
						ulRunStartAddress = NextPC;
#endif

						// set needed iVifRegs since running on the same thread
						iVifRegs.TOP = VifRegs.TOP;
						iVifRegs.ITOP = VifRegs.ITOP;

						//CycleCount = eCycleCount;
					}

#ifdef VERBOSE_MSCAL
					// debugging
					cout << "\nhps2x64: VU#" << Number << ": MSCAL";
					cout << " StartPC=" << hex << PC;
#endif
					
#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** MSCAL";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " StartPC=" << hex << PC;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << VifCode.Value;
					Vu::Instruction::Execute::debug << " Cycle#" << hex << *_DebugCycleCount;
					Vu::Instruction::Execute::debug << " ***";
#endif
				}
				
				
				break;

			// MSCALF
			// MT
			case 21:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCALF";
#endif

				// waits for end of current vu program and gif path1/path2 transfer, then runs program starting from IMM*8
				// Vu0 and Vu1
				
				// *** TODO *** wait for end of PATH1/PATH2 transfer
				if ( Running )
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;

					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCALF_PENDING)";
#endif

					// there is a pending MSCALF Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

#ifdef ENABLE_SET_VGW
					// not waiting for end of GIF transfer currently
					VifRegs.STAT.VGW = 0;
#endif
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// set PC = IMM * 8
					//PC = VifCode.IMMED << 3;
					
					// need to set next pc too since this could get called in the middle of VU Main CPU loop
					//NextPC = PC;
					
					// VU is now running
					StartVU ();
					
					// if just starting vu, then no longer multi-threading either
					//ullMultiThread_StartCycle = -1LL;
					
#ifdef ENABLE_PATH1_BUFFER
					//GPU::Path1_ProcessBlocks ();
#endif
					

					
					// start multi-threading from when vu is started
					//ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					
					
					
					// set TOPS to TOP
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOPS to ITOP ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );

					// note: if running on another thread want to send command and:
					// TOP,ITOP
					//if ( Number && ulThreadCount )
					//{
					//	// vu events are handled differently if the command is in the cmd buffer
					//	bCurrentVuRunInCmdBuffer = true;
					//
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}
					//else
					{
						// vu events are handled differently if the command is in the cmd buffer
						//bCurrentVuRunInCmdBuffer = false;

						// set PC = IMM * 8
						PC = VifCode.IMMED << 3;
						
						// need to set next pc too since this could get called in the middle of VU Main CPU loop
						NextPC = PC;

#ifdef ENABLE_VU_MULTI_EXECUTE
						ulRunStartAddress = NextPC;
#endif

						// set needed iVifRegs since running on the same thread
						iVifRegs.TOP = VifRegs.TOP;
						iVifRegs.ITOP = VifRegs.ITOP;
					}

#ifdef VERBOSE_MSCALF
					// debugging
					cout << "\nhps2x64: VU#" << Number << ": MSCALF";
					cout << " StartPC=" << hex << PC;
#endif
					
#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** MSCALF";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " StartPC=" << hex << PC;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << VifCode.Value;
					Vu::Instruction::Execute::debug << " Cycle#" << hex << *_DebugCycleCount;
					Vu::Instruction::Execute::debug << " ***";
#endif
				}
				
				break;
				
				
			// MSCNT
			// MT
			case 23:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCNT";
#endif

				// waits for end of current vu program and starts next one where it left off
				// Vu0 and Vu1
				
				if ( Running )
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCNT_PENDING)";
#endif

					// there is a pending MSCNT Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// VU is now running
					StartVU ();

#ifdef ENABLE_VU_MULTI_EXECUTE
					ulRunStartAddress = NextPC;
#endif


					// if just starting vu, then no longer multi-threading either
					//ullMultiThread_StartCycle = -1LL;
					
#ifdef ENABLE_PATH1_BUFFER
					//GPU::Path1_ProcessBlocks ();
#endif
					
					// start multi-threading from when vu is started
					//ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;

					// set TOP to TOPS
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );

					// note: if running on another thread want to send command and:
					// TOP,ITOP
					//if ( Number && ulThreadCount )
					//{
					//	// vu events are handled differently if the command is in the cmd buffer
					//	bCurrentVuRunInCmdBuffer = true;

					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//}
					//else
					{
						// vu events are handled differently if the command is in the cmd buffer
						//bCurrentVuRunInCmdBuffer = false;

						// set PC = IMM * 8
						//PC = VifCode.IMMED << 3;
						
						// need to set next pc too since this could get called in the middle of VU Main CPU loop
						//NextPC = PC;

						// set needed iVifRegs since running on the same thread
						iVifRegs.TOP = VifRegs.TOP;
						iVifRegs.ITOP = VifRegs.ITOP;
					}



#ifdef VERBOSE_MSCNT
					// debugging
					cout << "\nhps2x64: VU#" << Number << ": MSCNT";
					cout << " StartPC=" << hex << PC;
#endif
					
#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** MSCNT";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " PC=" << hex << PC;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << VifCode.Value;
					Vu::Instruction::Execute::debug << " Cycle#" << hex << *_DebugCycleCount;
					Vu::Instruction::Execute::debug << " P0=" << hex << Pipeline_Bitmap.b0 << " P1=" << Pipeline_Bitmap.b1;
					Vu::Instruction::Execute::debug << " ***";
#endif
				}
				
				break;

			// STMASK
			case 32:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STMASK";
#endif

				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					//if ( Number && ulThreadCount )
					//{
					//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
					//	CopyTo_CommandBuffer ( Data, 1 );
					//}

					// store mask
					//VifRegs.MASK = Data [ lIndex++ ];
					VifRegs.MASK = *Data++;
					
					// update index
					lVifIdx++;
					
					// command size is 1+1 words
					lWordsLeft--;
					
					// command is done
					lVifCodeState = 0;
					
				}
				
				break;

			// STROW
			case 48:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STROW";
#endif


				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( lVifCodeState < c_iSTROW_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//VifRegs.Regs [ 15 + lVifCodeState ] = Data [ lIndex++ ];
						VifRegs.Regs [ c_iRowRegStartIdx + lVifCodeState - 1 ] = *Data++;
						
						// update index
						lVifIdx++;
						
						// move to next data input element
						lVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;
					}
					
					if ( lVifCodeState >= c_iSTROW_CommandSize )
					{
						//if ( Number && ulThreadCount )
						//{
						//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
						//	CopyTo_CommandBuffer ( &VifRegs.Regs [ c_iRowRegStartIdx ], 4 );
						//}

						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;

			// STCOL
			case 49:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STCOL";
#endif


				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( lVifCodeState < c_iSTCOL_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//VifRegs.Regs [ 19 + lVifCodeState ] = Data [ lIndex++ ];
						VifRegs.Regs [ c_iColRegStartIdx + lVifCodeState - 1 ] = *Data++;
						
						// update index
						lVifIdx++;
						
						// move to next data input element
						lVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;
					}
					
					if ( lVifCodeState >= c_iSTCOL_CommandSize )
					{
						//if ( Number && ulThreadCount )
						//{
						//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
						//	CopyTo_CommandBuffer ( &VifRegs.Regs [ c_iColRegStartIdx ], 4 );
						//}

						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;

			// MPG
			case 74:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MPG";
#endif

				// load following vu program of size NUM*2 into address IMM*8
				// Vu0 and Vu1
				
				// ***TODO*** MPG is supposed to wait for the end of the microprogram before executing

					if ( !lVifCodeState )
					{
						//if ( Number && ulThreadCount )
						//{
						//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
						//}

						// set the command size
						// when zero is specified, it means 256
						lVifCommandSize = 1 + ( ( ( !VifCode.NUM ) ? 256 : VifCode.NUM ) << 1 );
						
						// for MPG command, NUM register is set to NUM field
						VifRegs.NUM = VifCode.NUM;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

						// there is more incoming data than just the tag
						lVifCodeState++;
						

						// if multi-threading, put in the command
						
						
#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** MPG";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << VifCode.Value;
					Vu::Instruction::Execute::debug << " ***";
					// looks like sometimes the VifCode can be "unaligned"
					// it would be unaligned if it were not zero ??
					if ( lVifIdx & 3 )
					{
						Vu::Instruction::Execute::debug << "(UNALIGNED lVifIdx_Offset=" << (lVifIdx & 3) << ")";
					}
#endif
					}


				
				if ( Running )
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// vif is not only stopped, but also waiting for the end of microprogram
					VifWaiting = 1;

					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MPG_PENDING)";
#endif

					// there is a pending DIRECT Command -> for now just back up index
					//lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					// note: if running on another thread want to send command and:
					// DATA


					VifStopped = 0;
					
					// vif no longer waiting for end of program
					VifWaiting = 0;

#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNMPG)";
#endif

					// if multi-threading, but in just the data into buffer
					//if ( Number && ulThreadCount )
					//{
					//	if ( lWordsLeft )
					//	{
					//	ulTemp = lVifCommandSize - lVifCodeState;
					//	lWordsToTransfer = ( lWordsLeft <= ulTemp ) ? lWordsLeft : ulTemp;
					//	CopyTo_CommandBuffer ( Data, lWordsToTransfer );
					//	lWordsLeft -= lWordsToTransfer;
					//	lVifIdx += lWordsToTransfer;
					//	lVifCodeState += lWordsToTransfer;

					//	Data += lWordsToTransfer;
					//	}
					//}
					//else
					{
						// code has been modified for VU
						bCodeModified [ Number ] = 1;

						// if processing next element, also check there is data to process
						if ( lWordsLeft )
						{
							// vu address can wrap ?
							//DstPtr32 = & MicroMem32 [ ( VifCode.IMMED << 1 ) + ( lVifCodeState - 1 ) ];
							//DstPtr32 = & MicroMem32 [ ( ( VifCode.IMMED << 1 ) + ( lVifCodeState - 1 ) ) & ( ulVuMem_Mask >> 2 ) ];

							while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
							{
#ifdef ENABLE_RUNNING_CHECKSUM
								ullRunningChecksum -= ( (u64) ( *DstPtr32 ) ) * (( VifCode.IMMED << 1 ) + ( lVifCodeState ));
								ullRunningChecksum += ( (u64) ( *Data ) ) * (( VifCode.IMMED << 1 ) + ( lVifCodeState ));
#endif

								// store ROW (R0-R3)
								// register offset starts at 16, buf lVifCodeState will start at 1
								// *DstPtr32++ = *Data++;
								// but if transfer can wrap, need to do it this way
								MicroMem32 [ ( ( VifCode.IMMED << 1 ) + ( lVifCodeState - 1 ) ) & ( ulVuMem_Mask >> 2 ) ] = *Data++;
								
								// update index
								lVifIdx++;
								
								// move to next data input element
								lVifCodeState++;
								
								// command size is 1+NUM*2 words
								lWordsLeft--;

							}	// end while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
							
							// update NUM register with amount of data left
							// state counts 32-bit words, so shifted right to count 64-bit values
							VifRegs.NUM = ( ( lVifCommandSize - lVifCodeState ) >> 1 ) & 0xff;

						}	// end if ( lWordsLeft )

					}	// end if ( Number && ulThreadCount ) else

					if ( lVifCodeState >= lVifCommandSize )
					{
						// command is done
						lVifCodeState = 0;
						
					}	// end if ( lVifCodeState >= lVifCommandSize )

				}	// end if ( Running ) else

				break;

			// DIRECT
			case 80:
#ifdef INLINE_DEBUG_VUCOM
	debug << " DIRECT";
	debug << ",APATH=" << dec << GPU::_GPU->GIFRegs.STAT.APATH;
#endif

				// transfers data to GIF via path2 (correct GIF Tag required)
				// size 1+IMM*4, but IMM is 65536 when it is zero
				// Vu1 ONLY
				
				// don't perform DIRECT/DIRECTHL command yet while vu1 (path1?) is executing ??

				
					if ( !lVifCodeState )
					{
						// set the command size
						lVifCommandSize = 1 + ( ( !VifCode.IMMED ? 65536 : VifCode.IMMED ) << 2 );
						
						// looks like it might be possible to send unaligned graphics data ??
						// need to assume that direct command always begins with a new packet due to this for now ??
						// note: it does not reset these to zero when direct command is run since it could be transfering graphics data
						// but still need to figure out the unaligned direct/directhl transfers
						//GPU::_GPU->ulTransferCount [ 2 ] = 0;
						//GPU::_GPU->ulTransferSize [ 2 ] = 0;
						
						// for DIRECT command, NUM register is set to value of the IMMEDIATE field
						VifRegs.NUM = VifCode.IMMED;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

						// there is more incoming data than just the tag
						lVifCodeState++;
						
						// direct command is now transferring data
						// gets priority over path 3 for this
						bTransferringDirectViaPath2 = true;
					}
				
				
#ifdef HALT_DIRECT_WHILE_VU_RUNNING
				//if ( Running || ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
				//if ( Running )
				if (
					// if path 1 is in queue and end of packet for path 2
					( ( GPU::_GPU->GIFRegs.STAT.P1Q ) && ( GPU::_GPU->EndOfPacket [ 2 ] ) )
					
					// if path 1 is running
					|| ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
					
					// if path 3 is running (meaning not masked, or not masked yet) and can't be interrupted AND not the end of a packet
					//|| ( ( GPU::_GPU->GIFRegs.STAT.APATH == 3 ) && ( ( !GPU::_GPU->GIFRegs.STAT.IMT ) || ( GPU::_GPU->GIFTag0 [ 3 ].FLG != 2 ) ) && ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
					|| ( ( ( !GPU::_GPU->GIFRegs.STAT.IMT ) || ( GPU::_GPU->GIFTag0 [ 3 ].FLG != 2 ) ) && ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
					
					// if path 3 and not end of a packet
					// || ( ( GPU::_GPU->GIFRegs.STAT.APATH == 3 ) && ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
					//|| ((GPU::_GPU->GIFRegs.STAT.M3R || GPU::_GPU->GIFRegs.STAT.M3P) && (!GPU::_GPU->EndOfPacket[3]))
					)
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif


					// path 2 in queue
					GPU::_GPU->GIFRegs.STAT.P2Q = 1;
					

					
					VifStopped = 1;
					
					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (DIRECT_PENDING)";
	debug << " P1Q=" << GPU::_GPU->GIFRegs.STAT.P1Q;
	debug << " APATH=" << GPU::_GPU->GIFRegs.STAT.APATH;
	debug << " IMT=" << GPU::_GPU->GIFRegs.STAT.IMT;
	debug << " FLG=" << GPU::_GPU->GIFTag0 [ 3 ].FLG;
	debug << " EP2=" << GPU::_GPU->EndOfPacket [ 2 ];
	debug << " EP3=" << GPU::_GPU->EndOfPacket [ 3 ];
#endif

					// there is a pending DIRECT Command -> for now just back up index
					//lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
#endif
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif


					// path 2 no longer in queue
					GPU::_GPU->GIFRegs.STAT.P2Q = 0;

					
					// check if interrupting path 3
					if ( GPU::_GPU->GIFRegs.STAT.APATH == 3 )
					{
#ifdef INLINE_DEBUG_VUCOM
	debug << " SET-IP3";
#endif
						GPU::_GPU->GIFRegs.STAT.IP3 = 1;
					}

					// this is path 2
					GPU::_GPU->GIFRegs.STAT.APATH = 2;
					
					
					VifStopped = 0;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNDIRECT)";
#endif

					// if processing next element, also check there is data to process
					if ( lWordsLeft )
					{
#ifdef INLINE_DEBUG_VUCOM
	debug << " lWordsLeft=" << dec << lWordsLeft;
	debug << " lVifCodeState=" << dec << lVifCodeState;
	debug << " lVifCommandSize=" << dec << lVifCommandSize;
#endif
						while ( ( lWordsLeft > 0 ) && ( lVifCodeState < lVifCommandSize ) )
						{
							// get amount to transfer from this block
							lWordsToTransfer = lVifCommandSize - lVifCodeState;
							if ( lWordsToTransfer > lWordsLeft ) lWordsToTransfer = lWordsLeft;
							
							// ***TODO*** send graphics data to GIF
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							//GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 2 );
							//GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 1 );
							GPU::Path2_WriteBlock ( Data, lWordsToTransfer );
							
							//Data += 4;
							Data += lWordsToTransfer;
							
							// update index
							lVifIdx += lWordsToTransfer;
							
							// move to next data input element
							//lVifCodeState++;
							//lVifCodeState += 4;
							lVifCodeState += lWordsToTransfer;
							
							// command size is 1+NUM*2 words
							//lWordsLeft--;
							//lWordsLeft -= 4;
							lWordsLeft -= lWordsToTransfer;
						}
						
						// for DIRECT command, NUM register is number of 128-bit QWs left
						VifRegs.NUM = ( ( lVifCommandSize - lVifCodeState ) >> 2 ) & 0xffff;
						
						if ( lVifCodeState >= lVifCommandSize )
						{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (DIRECT-COMPLETED)";
#endif

							// command is done
							lVifCodeState = 0;


							// done in path 2
							GPU::_GPU->GIFRegs.STAT.APATH = 0;
							
							// also need to clear/flush the path2 pipeline
							GPU::_GPU->ulPath2_DataWaiting = 0;
							
							// Direct command no longer transferring data
							bTransferringDirectViaPath2 = false;
							
							// for now, clear buffer at command completion even if temporary
							//VifRegs.STAT.FQC = 0;
							
						}
					}
					
				}
				
				break;

			// DIRECTHL
			case 81:
#ifdef INLINE_DEBUG_VUCOM
	debug << " DIRECTHL";
#endif

				// transfers data to GIF via path2 (correct GIF Tag required)
				// size 1+IMM*4, but IMM is 65536 when it is zero
				// stalls until PATH3 transfer is complete
				// Vu1 ONLY
				
				// don't perform DIRECT/DIRECTHL command yet while vu1 (path1?) is executing ??

				
					if ( !lVifCodeState )
					{
						// set the command size
						lVifCommandSize = 1 + ( ( !VifCode.IMMED ? 65536 : VifCode.IMMED ) << 2 );
					
						// looks like it might be possible to send unaligned graphics data ??
						// need to assume that direct command always begins with a new packet due to this for now ??
						// note: it does not reset these to zero when directhl command is run since it could be transfering graphics data
						// but still need to figure out the unaligned direct/directhl transfers
						//GPU::_GPU->ulTransferCount [ 2 ] = 0;
						//GPU::_GPU->ulTransferSize [ 2 ] = 0;
						
						// for DIRECTHL command, NUM register is set to value of the IMMEDIATE field
						VifRegs.NUM = VifCode.IMMED;

						
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

						// there is more incoming data than just the tag
						lVifCodeState++;
					}
					
				
				//if ( Running || ( GPU::_GPU->ulTransferCount [ 3 ] ) )
#ifdef HALT_DIRECTHL_WHILE_VU_RUNNING
				//if ( Running || ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
				if (
					// if path 1 is in queue and end of packet for path 2
					( ( GPU::_GPU->GIFRegs.STAT.P1Q ) && ( GPU::_GPU->EndOfPacket [ 2 ] ) )
					
					// if path 1 is running
					|| ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
					
					// if path 3 is running (meaning not masked, or not masked yet)
					// || ( GPU::_GPU->GIFRegs.STAT.APATH == 3 )
					// if path 3 is running and it is not the end of a packet
					//|| ( ( GPU::_GPU->GIFRegs.STAT.APATH == 3 ) && ( !GPU::_GPU->EndOfPacket [ 3 ] ) )
					|| ( !GPU::_GPU->EndOfPacket [ 3 ] )
					
					// if path 3 is in queue and not masked and path 2 is not already running
					// || ( ( GPU::_GPU->GIFRegs.STAT.APATH != 2 ) && ( GPU::_GPU->GIFRegs.STAT.P3Q ) && ( !GPU::_GPU->GIFRegs.STAT.M3R ) && ( !GPU::_GPU->GIFRegs.STAT.M3P ) )
				)
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					// for now, need to indicate that directhl isn't waiting to process if path 3 is running
					if ( GPU::_GPU->GIFRegs.STAT.APATH != 3 )
					{
						// path 2 in queue
						GPU::_GPU->GIFRegs.STAT.P2Q = 1;
					}
					

					

					VifStopped = 1;
					
					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
					
					
#ifdef ENABLE_SET_VGW
					//if ( GPU::_GPU->ulTransferCount [ 3 ] )
					//if ( !GPU::_GPU->EndOfPacket [ 3 ] )
					//{
						// when vif is stalled via DIRECTHL or FLUSHA, need to set STAT.VGW
						VifRegs.STAT.VGW = 1;
					//}
#endif
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (DIRECTHL_PENDING)";
	debug << " APATH=" << GPU::_GPU->GIFRegs.STAT.APATH;
	debug << " P1Q=" << GPU::_GPU->GIFRegs.STAT.P1Q;
	debug << " P3Q=" << GPU::_GPU->GIFRegs.STAT.P3Q;
#endif

					// there is a pending DIRECT Command -> for now just back up index
					//lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
#endif
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif


					// path 2 no longer in queue
					GPU::_GPU->GIFRegs.STAT.P2Q = 0;

					// check if interrupting path 3
					if ( GPU::_GPU->GIFRegs.STAT.APATH == 3 )
					{
						GPU::_GPU->GIFRegs.STAT.IP3 = 1;
					}

					// this is path 2
					GPU::_GPU->GIFRegs.STAT.APATH = 2;
					

					VifStopped = 0;
					
#ifdef ENABLE_SET_VGW
					// not waiting for end of GIF transfer currently
					VifRegs.STAT.VGW = 0;
#endif
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNDIRECTHL)";
#endif


					// if processing next element, also check there is data to process
					if ( lWordsLeft )
					{
						while ( ( lWordsLeft > 0 ) && ( lVifCodeState < lVifCommandSize ) )
						{
							// get amount to transfer from this block
							lWordsToTransfer = lVifCommandSize - lVifCodeState;
							if ( lWordsToTransfer > lWordsLeft ) lWordsToTransfer = lWordsLeft;
							
							// ***TODO*** send graphics data to GIF
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							//GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 2 );
							//GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 1 );
							GPU::Path2_WriteBlock ( Data, lWordsToTransfer );
							
							//Data += 4;
							Data += lWordsToTransfer;
							
							// update index
							lVifIdx += lWordsToTransfer;
							
							// move to next data input element
							//lVifCodeState++;
							//lVifCodeState += 4;
							lVifCodeState += lWordsToTransfer;
							
							// command size is 1+NUM*2 words
							//lWordsLeft--;
							//lWordsLeft -= 4;
							lWordsLeft -= lWordsToTransfer;
						}
						
						// for DIRECTHL command, NUM register is number of 128-bit QWs left
						VifRegs.NUM = ( ( lVifCommandSize - lVifCodeState ) >> 2 ) & 0xffff;
						
						if ( lVifCodeState >= lVifCommandSize )
						{
							// command is done
							lVifCodeState = 0;
							
							// done in path 2
							GPU::_GPU->GIFRegs.STAT.APATH = 0;
							
							// also need to clear/flush the path2 pipeline
							GPU::_GPU->ulPath2_DataWaiting = 0;
							
							// for now, clear buffer at command completion even if temporary
							//VifRegs.STAT.FQC = 0;
						}
					}
					
				}
				
				break;
				
			// UNPACK
			default:
			
				if ( ( ( VifCode.Value >> 29 ) & 0x3 ) == 0x3 )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " UNPACK";
#endif

					// unpacks data into vu data memory
					// Vu0 and Vu1
					if ( !lVifCodeState )
					{
						u32 uWL, uCL;

						// clear the the counter for unpack
						lVifUnpackIndex = 0;

						ulUnpackLutIndex = ((VifCode.Value >> 24) & 0x1f) | (VifCode.USN << 5) | ((VifRegs.MODE & 3) << 6);


						// looks like NUM=0 might be same as 256 ?? even for unpack ??
						lVifCommandSize = ( VifCode.NUM ? VifCode.NUM : 256 );
						//lVifCommandSize = VifCode.NUM;

						//uWL = (VifRegs.CYCLE.WL) ? VifRegs.CYCLE.WL : 256;
						//uCL = (VifRegs.CYCLE.CL) ? VifRegs.CYCLE.CL : 256;
						//uWL = VifRegs.CYCLE.WL;
						//uCL = VifRegs.CYCLE.CL;

						// get size of packet to receive
						// WL <= CL
						if ( VifRegs.CYCLE.WL <= VifRegs.CYCLE.CL )
						//if (uWL <= uCL)
						{
							// skipping write //

							uWL = (VifRegs.CYCLE.WL) ? VifRegs.CYCLE.WL : 256;
							uCL = (VifRegs.CYCLE.CL) ? VifRegs.CYCLE.CL : 256;

							ulPackedSize = ( ( 32 >> VifCode.vl ) * ( VifCode.vn + 1 ) ) * lVifCommandSize;
							ulPackedSize = ( ulPackedSize >> 5 ) + ( ( ulPackedSize & 0x1f ) ? 1 : 0 );

							//cout << "\nALERT: VU: UNPACK: skipping write: WL:" << VifRegs.CYCLE.WL << " CL:" << VifRegs.CYCLE.CL << " SKIP:" << (VifRegs.CYCLE.CL - VifRegs.CYCLE.WL) << " SIZE:" << ulPackedSize;
							//cout << " m:" << VifCode.m << " vn:" << VifCode.vn << " vl:" << VifCode.vl << " USN:" << VifCode.USN << " MODE:" << (VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
						}
						else
						{
							// filling write //

							uWL = VifRegs.CYCLE.WL;
							uCL = VifRegs.CYCLE.CL;

							//ulTemp = VifRegs.CYCLE.CL * ( lVifCommandSize / VifRegs.CYCLE.WL );
							ulTemp = uCL * (lVifCommandSize / uWL);
							//ulTemp += std::min( lVifCommandSize % VifRegs.CYCLE.WL, VifRegs.CYCLE.CL );
							ulTemp += std::min(lVifCommandSize % uWL, uCL);
							ulPackedSize = ( ( 32 >> VifCode.vl ) * ( VifCode.vn + 1 ) ) * ulTemp;
							ulPackedSize = ( ulPackedSize >> 5 ) + ( ( ulPackedSize & 0x1f ) ? 1 : 0 );

							//cout << "\nALERT: VU: UNPACK: filling write: WL:" << VifRegs.CYCLE.WL << " CL:" << VifRegs.CYCLE.CL << " FILL:" << (VifRegs.CYCLE.WL - VifRegs.CYCLE.CL) << " SIZE:" << ulPackedSize;
							//cout << " m:" << VifCode.m << " vn:" << VifCode.vn << " vl:" << VifCode.vl << " USN:" << VifCode.USN << " MODE:" << (VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
						}


//#define VERBOSE_WL_CL_ZERO
#ifdef VERBOSE_WL_CL_ZERO
						if (!VifRegs.CYCLE.WL || !VifRegs.CYCLE.CL)
						{
							cout << "\nhps2x64: VU: ALERT: UNPACK !WL||!CL !!!";
							cout << "\nALERT: VU: UNPACK: WL:" << VifRegs.CYCLE.WL << " CL:" << VifRegs.CYCLE.CL << " NUM:" << VifCode.NUM << " SIZE:" << ulPackedSize;
							cout << " m:" << VifCode.m << " vn:" << VifCode.vn << " vl:" << VifCode.vl << " USN:" << VifCode.USN << " MODE:" << (VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
						}
#endif	// end #ifdef VERBOSE_WL_CL_ZERO


						// the actual amount of data to write in 32-bit words is NUM*4
						//lVifCommandSize = 1 + ( VifCode.NUM << 2 );
						lVifCommandSize = 1 + ( lVifCommandSize << 2 );
						
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

#ifdef VERBOSE_UNPACK
						// alert for values of m, vn, and vl for now
						// m=0, vn=3, vl=0
						cout << "\nhps2x64: VU#" << Number << ": m=" << dec << VifCode.m << " vn=" << VifCode.vn << " vl=" << VifCode.vl << " CL=" << VifRegs.CYCLE.CL << " WL=" << VifRegs.CYCLE.WL << " FLG=" << ( ( VifCode.IMMED & 0x8000 ) >> 15 ) << " ADDR=" << hex << ( VifCode.IMMED & 0x3ff );
#endif

						// there is more incoming data than just the tag
						lVifCodeState++;
						

						// note: if running on another thread want to send command and:
						// TOPS,STMASK,STCYCLE,STCOL,STROW,DATA
						// note: if running on another thread want to send command and:
						// TOP,ITOP
						//if ( Number && ulThreadCount )
						//{
						//	CopyTo_CommandBuffer ( &VifCode.Value, 1 );
						//}
						//else
						{
							
							// need to start read offset at zero, and update when reading from dma/memory
							ulReadOffset32 = 0;
							
							// get size of data elements in bytes
							ulUnpackItemWidth = 4 >> VifCode.vl;
							
							// get number of components to unpack
							ulUnpackNum = VifCode.vn + 1;
							ulUnpackNumIdx = 0;
							
							// get wl/cl
							//ulWL = VifRegs.CYCLE.WL;
							//ulCL = VifRegs.CYCLE.CL;
							ulWL = uWL;
							ulCL = uCL;
							ulWLCycle = 0;
							ulCLCycle = 0;

#ifdef VERBOSE_FILLING_WRITE
						// need to find and debug filling writes ??
						if ( ulCL < ulWL )
						{
							cout << "\nhps2x64: ALERT: VIF: UNPACK: Filling Write. CL=" << dec << ulCL << " WL=" << ulWL << "\n";
						}
#endif
							
							// initialize count of elements read
							ulReadCount = 0;

							// for UNPACK command, NUM register is set to NUM field
							VifRegs.NUM = VifCode.NUM;

							// check if FLG bit is set or if this is vu0
							if ( !( VifCode.IMMED & 0x8000 ) || ( !Number ) )
							{
								if ( !Number )
								{
									// vu0 //

									// need to keep track of where writing to in VU memory
									ulWriteOffset32 = ( ( ( VifCode.IMMED & 0xff ) ) << 2 ) + ( lVifCodeState - 1 );
								}
								else
								{
									// vu1 //

									// need to keep track of where writing /to in VU memory
									ulWriteOffset32 = ( ( ( VifCode.IMMED & 0x3ff ) ) << 2 ) + ( lVifCodeState - 1 );
								}
							}
							else
							{
								// need to keep track of where writing to in VU memory
								ulWriteOffset32 = ( ( ( VifCode.IMMED & 0x3ff ) + ( VifRegs.TOPS & 0x3ff ) ) << 2 ) + ( lVifCodeState - 1 );
							}

						}

						
#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (BASE=" << VifRegs.BASE << " OFFSET=" << VifRegs.OFST << " TOP=" << VifRegs.TOP << " TOPS=" << VifRegs.TOPS << ")";
#endif

#ifdef INLINE_DEBUG_VUEXECUTE_VERBOSE
					cout << "\r\n*** UNPACK";
					cout << " VU#" << Number;
					cout << " VifCode=" << hex << VifCode.Value;
					cout << hex << " BASE=" << VifRegs.BASE << " OFFSET=" << VifRegs.OFST << " TOP=" << VifRegs.TOP << " TOPS=" << VifRegs.TOPS;
					cout << hex << " m=" << VifCode.m << " vn=" << VifCode.vn << " vl=" << VifCode.vl << " CL=" << VifRegs.CYCLE.CL << " WL=" << VifRegs.CYCLE.WL << " FLG=" << ( ( VifCode.IMMED & 0x8000 ) >> 15 ) << " ADDR=" << hex << ( VifCode.IMMED & 0x3ff );
					cout << dec << " packedsize=" << ulPackedSize;
					cout << dec << " unpackedsize=" << (lVifCommandSize-1);
					cout << " ***";
#endif

#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** UNPACK";
					Vu::Instruction::Execute::debug << " VU#" << Number;
					Vu::Instruction::Execute::debug << " VifCode=" << hex << VifCode.Value;
					Vu::Instruction::Execute::debug << hex << " BASE=" << VifRegs.BASE << " OFFSET=" << VifRegs.OFST << " TOP=" << VifRegs.TOP << " TOPS=" << VifRegs.TOPS;
					Vu::Instruction::Execute::debug << hex << " m=" << VifCode.m << " vn=" << VifCode.vn << " vl=" << VifCode.vl << " CL=" << VifRegs.CYCLE.CL << " WL=" << VifRegs.CYCLE.WL << " FLG=" << ( ( VifCode.IMMED & 0x8000 ) >> 15 ) << " ADDR=" << hex << ( VifCode.IMMED & 0x3ff );
					Vu::Instruction::Execute::debug << dec << " packedsize=" << ulPackedSize;
					Vu::Instruction::Execute::debug << dec << " unpackedsize=" << (lVifCommandSize-1);
					Vu::Instruction::Execute::debug << " ***";
					// looks like sometimes the VifCode can be "unaligned"
					// it would be unaligned if it were not zero ??
					if ( lVifIdx & 3 )
					{
						Vu::Instruction::Execute::debug << "(UNALIGNED lVifIdx_Offset=" << (lVifIdx & 3) << ")";
					}
#endif

					}	// end if ( !lVifCodeState )
					
					//if ( lWordsLeft )
					if ( lVifCodeState )
					{

#define USE_TEMPLATES_LUT_UNPACK_VU
#ifdef USE_TEMPLATES_LUT_UNPACK_VU

#define USE_TEMPLATES_LUT_UNPACK_VU_AVX2
#ifdef USE_TEMPLATES_LUT_UNPACK_VU_AVX2
						//if ((ulWL == ulCL) && (!VifCode.m) && (VifCode.vl==1) && (VifCode.vn == 2))
						//if((ulWL == ulCL) && (!VifCode.m) && (VifCode.vn == 1) && (VifCode.vl == 0))
						{
							//cout << "\nALERT: VU: UNPACK: skipping write: WL:" << VifRegs.CYCLE.WL << " CL:" << VifRegs.CYCLE.CL << " SKIP:" << (VifRegs.CYCLE.CL - VifRegs.CYCLE.WL) << " SIZE:" << ulPackedSize;
							//cout << " m:" << VifCode.m << " vn:" << VifCode.vn << " vl:" << VifCode.vl << " USN:" << VifCode.USN << " MODE:" << (VifRegs.MODE & 3) << " LEFT:" << lWordsLeft;
							ps2_arr_execute_unpack_avx2_lut[ulUnpackLutIndex](this, (u8*&)Data, lWordsLeft, ulPackedSize);
						}
						//else
#endif
						{
							//ps2_arr_execute_unpack_lut[ulUnpackLutIndex](this, Data, lWordsLeft, ulPackedSize);
						}

#else
						lWordsToTransfer = ( lWordsLeft <= ulPackedSize ) ? lWordsLeft : ulPackedSize;
						ulPackedSize -= lWordsToTransfer;

						//if ( Number && ulThreadCount )
						//{
						//	CopyTo_CommandBuffer ( Data, lWordsToTransfer );
						//	lWordsLeft -= lWordsToTransfer;
						//	lVifIdx += lWordsToTransfer;

						//	Data += lWordsToTransfer;

						//	if ( !ulPackedSize )
						//	{
						//		lVifCodeState = lVifCommandSize;

						//		//ulUnpackItemWidth = 0;
						//		//ulReadCount = 0;
						//	}
						//}
						//else
						{
						
						// get dst pointer
						DstPtr32 = & VuMem32 [ ulWriteOffset32 ];
						
						//while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
						while ( lVifCodeState < lVifCommandSize )
						{
							// first determine where data should be read from //
							// WL must be within range before data is read
							if ( ulWLCycle < ulWL )
							{
								// WL is within range
								// so we must read data from someplace
								
								// get Write Cycle
								// *todo* unsure if the "Write Cycle" is reset after CL or WL or both ??
								ulWriteCycle = ulWLCycle;
								
								// get unpack row/col
								ulUnpackRow = ( ( ulWriteCycle < 4 ) ? ulWriteCycle : 3 );
								ulUnpackCol = ulWriteOffset32 & 3;

								// change#5 - reset unpack count for column at row start
								if ( !ulUnpackCol )
								{
									//ulUnpackCount = ulUnpackNum;
								}

								// data source is determined by m and MASK //
								
								// change#1 - can get the mask sooner
								// if m bit is 0, then mask is zero
								ulUnpackMask = 0;
								
								if ( VifCode.m || ( ulCLCycle >= ulCL ) )
								{
									// mask comes from MASK register //
									
									
									// mask index is (Row*4)+Col
									// but if the row (write cycle??) is 3 or 4 or greater then just use maximum row value
									ulUnpackMask = ( VifRegs.MASK >> ( ( ( ulUnpackRow << 2 ) + ulUnpackCol ) << 1 ) ) & 3;
									
									// determine source of data
									switch (ulUnpackMask)
									{
										//case 0:
										//	// data comes from dma/ram //
										//	lUnpackData = lUnpackLastData;
										//	break;

									case 1:
										// data comes from ROW register //
										lUnpackData = VifRegs.Regs[c_iRowRegStartIdx + ulUnpackCol];
										break;

									case 2:
										// data comes from COL register //
										lUnpackData = VifRegs.Regs[c_iColRegStartIdx + ulUnpackRow];
										break;

										//case 3:
										//	// write is masked/not happening/prevented //
										//	break;

									}	// end switch (ulUnpackMask)


#ifdef INLINE_DEBUG_UNPACK_2
	debug << "\r\nRow#" << ulUnpackRow << " Col#" << ulUnpackCol << " TO " << hex << ulWriteOffset32 << " FullMsk=" << hex << VifRegs.MASK << " Shift=" << dec << ( ( ( ulUnpackRow << 2 ) + ulUnpackCol ) << 1 );
#endif
								}	// end if ( VifCode.m || ( ulCLCycle >= ulCL ) )


								// to read data from dma/memory, CL must be within range, and Col must be within correct range
								if ( ( ulCLCycle < ulCL ) && ( ulUnpackCol < ulUnpackNum ) )
								// change#2 - don't update input pointer unless mask is zero
								//if ( ( ulCLCycle < ulCL ) && ( ulUnpackCol < ulUnpackNum ) && ( !ulUnpackMask ) )
								// change#6 - use a count to determine number of max input per row
								//if ( ( ulCLCycle < ulCL ) && ( ulUnpackCount ) && ( !ulUnpackMask ) )
								{
									// data source is dma/ram //

									// change#6 - update unpack count for the current row
									//ulUnpackCount--;
								
									// make sure that there is data left to read
									if ( lWordsLeft <= 0 )
									{
										// there is no data to read //
										
										// return sequence when lVifCodeState is NOT zero //
										break;
										
									}
									
									// determine data element size
									if ( !ulUnpackItemWidth )
									{
										// must be unpacking pixels //
										// *todo* check for vn!=3 which would mean it is not unpacking pixels but rather an invalid command
										//cout << "\nhps2x64: ALERT: VU: UNPACK: Pixel unpack unsupported!!\n";
										
										// similar to unpacking 16-bit halfwords //
										
										if ( ! ( ulReadCount & 7 ) )
										{
											ulUnpackBuf = *Data++;
										
											// get the correct element of 32-bit word
											//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
											//lUnpackTemp &= 0xffff;
										}
										
										
										// pull correct component
										if ( ulUnpackCol < 3 )
										{
											// RGB
											lUnpackLastData = ulUnpackBuf & 0x1f;
											lUnpackLastData <<= 3;
											
											ulUnpackBuf >>= 5;
										}
										else
										{
											// A
											lUnpackLastData = ulUnpackBuf & 1;
											lUnpackLastData <<= 7;
											
											ulUnpackBuf >>= 1;
										}
										
										ulReadCount++;
										
										// one pixel gets unpacked per 8 components RGBA
										if ( ! ( ulReadCount & 7 ) )
										{
											//Data++;
											lVifIdx++;
											
											// this is counting the words left in the input
											lWordsLeft--;
											
#ifdef INLINE_DEBUG_UNPACK
	debug << " VifIdx=" << dec << lVifIdx << " WordsLeft=" << lWordsLeft;
#endif
										}
									}
									else
									{
										// unpacking NON-pixels //
										
										// check if only 1 element (vn=0) and not first col
										if ( VifCode.vn || !ulUnpackCol )
										{
											if ( ! ( ulReadCount & ( 7 >> ulUnpackItemWidth ) ) )
											{
												ulUnpackBuf = *Data++;
											
												// get the correct element of 32-bit word
												//lUnpackTemp >>= ( ( ulReadCount & 4 ) << 2 );
												//lUnpackTemp &= 0xffff;
											}
											
											
											lUnpackLastData = ulUnpackBuf & ( 0xffffffffUL >> ( ( 4 - ulUnpackItemWidth ) << 3 ) );
											
											ulUnpackBuf >>= ( ulUnpackItemWidth << 3 );
											
											// check if data item is signed and 1 or 2 bytes long
											if ( ( !VifCode.USN ) && ( ( ulUnpackItemWidth == 1 ) || ( ulUnpackItemWidth == 2 ) ) )
											{
												// signed data //
												lUnpackLastData <<= ( ( 4 - ulUnpackItemWidth ) << 3 );
												lUnpackLastData >>= ( ( 4 - ulUnpackItemWidth ) << 3 );
											}
											
											ulReadCount++;
											
											if ( ! ( ulReadCount & ( 7 >> ulUnpackItemWidth ) ) )
											{
												//Data++;
												lVifIdx++;
												
												// command size is 1+NUM*2 words
												// this is counting the words left in the input, but should ALSO be counting words left in output
												lWordsLeft--;
												
#ifdef INLINE_DEBUG_UNPACK
	debug << " VifIdx=" << dec << lVifIdx << " WordsLeft=" << lWordsLeft;
#endif
											}
											
										} // end if ( VifCode.vn || !ulUnpackCol )
											
									} // end if ( !ulUnpackItemWidth ) else
									
									
								} // end if ( ( ulCLCycle < ulCL ) && ( ulUnpackCol < ulUnpackNum ) )
								
								
								// for adding/totalling with row register, you need either m=0 or m[x]=0
								if ( !VifCode.m || !ulUnpackMask )
								{
									// data comes from dma/ram //
									lUnpackData = lUnpackLastData;

									// check if data should be added or totaled with ROW register
									if ( VifRegs.MODE & 3 )
									{
										// 1 means add, 2 means total
										
										switch ( VifRegs.MODE & 3 )
										{
											case 1:
#ifdef INLINE_DEBUG_UNPACK_2
	debug << " VALUE: " << hex << lUnpackData;
	debug << " ADD:" << hex << VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
#endif
											
												// add to unpacked data //
												
												// I guess the add should be an integer one?
												lUnpackData += VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];

#ifdef INLINE_DEBUG_UNPACK_2
	debug << " RESULT:" << hex << lUnpackData;
#endif

												break;
												
											case 2:
#ifdef INLINE_DEBUG_UNPACK_2
	debug << " VALUE: " << hex << lUnpackData;
	debug << " TOTAL:" << hex << VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
#endif

												// add to unpacked data and write result back to ROW register //
												
												// I guess the add/total should be an integer one?
												lUnpackData += VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
												VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ] = lUnpackData;

#ifdef INLINE_DEBUG_UNPACK_2
	debug << " RESULT:" << hex << lUnpackData;
	debug << " TOTAL:" << hex << VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ];
#endif

												break;
												
											
											// undocumented ??
											case 3:
#ifdef INLINE_DEBUG_UNPACK_2
	debug << " UNDOCUMENTED";
#endif

												VifRegs.Regs [ c_iRowRegStartIdx + ulUnpackCol ] = lUnpackData;
												break;
												
											default:
#ifdef INLINE_DEBUG_UNPACK
	debug << " INVALID";
#endif

												cout << "\nhps2x64: ALERT: VU: UNPACK: Invalid MODE setting:" << dec << VifRegs.MODE << "\n";
												break;

										}	// end switch ( VifRegs.MODE & 3 )

									}	// end if ( VifRegs.MODE & 3 )

								} // end if ( !VifCode.m || !ulUnpackMask )
								
								
#ifdef DISABLE_UNPACK_INDETERMINATE
								// only write data if the column is within the amount being written, or it is masked
								// change #3 - always outputs data unless masked
								//if ( ( ulUnpackNum == 1 ) || ( ulUnpackMask ) || ( ulUnpackCol < ulUnpackNum ) )
								//if ( ( ulUnpackNum == 1 ) || ( ulUnpackCol < ulUnpackNum ) )
								{
#endif
								
								// write data to vu memory if write is not masked //
								if ( ulUnpackMask != 3 )
								{
#ifdef INLINE_DEBUG_UNPACK_2
	debug << "\r\n";
if ( ( ulUnpackCol >= ulUnpackNum ) && ( ulUnpackNum != 1 ) )
{
	debug << "[INDT] ";
}
	debug << "Col#" << ulUnpackCol << " Data=" << hex << lUnpackData << " TO " << ulWriteOffset32 << "(SA=" << (ulWriteOffset32>>2) << ")" << " Msk=" << ulUnpackMask;
#endif

									*DstPtr32 = lUnpackData;
									
#ifdef INLINE_DEBUG_UNPACK_3
	debug << " VuMem=" << hex << VuMem32 [ ulWriteOffset32 ];
#endif
								}
#ifdef INLINE_DEBUG_UNPACK_2
								else
								{
	//debug << "\r\nCol#" << ulUnpackCol << " MSKD TO " << hex << ulWriteOffset32 << "(SA=" << (ulWriteOffset32>>2) << ")" << " Msk=" << ulUnpackMask;
	debug << "\r\nCol#" << ulUnpackCol << " DATA:" << hex << lUnpackData << " MSKD TO " << hex << ulWriteOffset32 << " Msk=" << ulUnpackMask;
								}
#endif

#ifdef DISABLE_UNPACK_INDETERMINATE
								} // end if ( ( ulUnpackNum == 1 ) || ( ulUnpackCol < ulUnpackNum ) )
#ifdef INLINE_DEBUG_UNPACK_2
//								else
//								{
//	debug << "\r\nCol#" << ulUnpackCol << " UNDT TO " << hex << ulWriteOffset32 << "(SA=" << (ulWriteOffset32>>2) << ")";
//								}
#endif
								
#endif
								
								// move to next data input element
								// this should update per 32-bit word that is written into VU memory
								// WL is a write cycle
								lVifCodeState++;
								
							} // end if ( ulWLCycle < ulWL )
							//else
							//{
							//	// skip part of skipping write //
							//}
						
						
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							//*DstPtr32++ = *Data++;
							
							// update index
							//lVifIdx++;
							
							// update write offset
							ulWriteOffset32++;
							
							// when write offset is updated, must also update ptr
							DstPtr32++;
							
							// check if going to the next row
							//if ( ( ( lVifCodeState - 1 ) & 3 ) == 3 )
							if ( !( ulWriteOffset32 & 3 ) )
							{
								// if just completed a write cycle, then decrement NUM register
								if ( ulWLCycle < ulWL )
								{
									// NUM register counts number 128-bit QWs written
									VifRegs.NUM--;
									VifRegs.NUM &= 0xff;
								}
								
								// update WL,CL
								ulWLCycle++;
								ulCLCycle++;
							}
							
							// reset both if they are both outside of range
							if ( ( ulWLCycle >= ulWL ) && ( ulCLCycle >= ulCL ) )
							{
								ulWLCycle = 0;
								ulCLCycle = 0;
							}
							
							
							
#ifdef INLINE_DEBUG_UNPACK
	debug << " State=" << dec << lVifCodeState;
#endif
						}	// end while ( lVifCodeState < lVifCommandSize )

						}	// end if ( Number && ulThreadCount )
						
						if ( lVifCodeState >= lVifCommandSize )
						{
							// command is done
							lVifCodeState = 0;
							
							// for now, clear buffer at command completion even if temporary
							//VifRegs.STAT.FQC = 0;
							

							// this next part is only if running on the same thread
							if ( !Number || !ulThreadCount )
							{
								// it is possible that unpacking is done but that there was padding left in the word
								// in that case need to skip the padding and go to the next word
								if ( !ulUnpackItemWidth )
								{
									if ( ulReadCount & 7 )
									{
										//Data++;
										lVifIdx++;
										
										// this is counting the words left in the input
										lWordsLeft--;
									}
								}
								else
								{
									if ( ulReadCount & ( 7 >> ulUnpackItemWidth ) )
									{
										//Data++;
										lVifIdx++;
										
										// this is counting the words left in the input
										lWordsLeft--;
									}
								}
							}
							
						}	// end if ( lVifCodeState >= lVifCommandSize )

#endif
					
					}	// end if ( lVifCodeState )

					break;

				}	// end if ( ( ( VifCode.Value >> 29 ) & 0x3 ) == 0x3 )
				
#ifdef INLINE_DEBUG_VUCOM
	debug << " INVALID";
#endif
#ifdef INLINE_DEBUG_INVALID
	debug << "\r\nVU#" << Number << " Error. CPU Thread. Invalid VIF Code=" << hex << VifCode.Value << " Cycle#" << dec << *_DebugCycleCount << " WRITEID=" << dec << ullCommBuf_WriteIdx;
#endif

#ifdef ENABLE_VU_MULTI_THREAD

				cout << "\nhps2x64: VU#" << Number << " Error. CPU Thread. Invalid VIF Code=" << hex << VifCode.Value << " WRITEID=" << dec << ullCommBuf_WriteIdx;

#endif

				break;

		}
	}
	
	// get the number of full quadwords completely processed
	QWC_Transferred = lVifIdx >> 2;
	
	// get start index of remaining data to process in current block
	lVifIdx &= 0x3;
	
	// if vif code state is zero, then check if the previous code had an interrupt, and trigger if so
	if ( !lVifCodeState )
	{
		// TESTING
		// command is done so set stat to vif idle?
		VifRegs.STAT.VPS = 0x0;
		
		// check for interrupt
		if ( VifCode.INT )
		{
#ifdef INLINE_DEBUG_VUINT
			debug << " ***INT***";
#endif
#ifdef VERBOSE_VUINT
			cout << "\n***VUINT***\n";
#endif
			
			// check if vif interrupt is enabled (means vif.err&1=0)
			if ( ! ( VifRegs.ERR & 1 ) )
			{
			// ***todo*** need to send interrupt signal and stop transfer to Vif //
			SetInterrupt_VIF ();
			
			// set STAT.INT
			VifRegs.STAT.INT = 1;
			
			// start reading from index offset (may have left off in middle of data QW block)
			Data = & ( Data [ lVifIdx & 0x3 ] );
			
			// check if next is MARK (don't stall on Vif Code MARK) or if there is no more data
			// if the next Vif Code is mark, then go ahead and execute it for now
			// note: it is possible this could be missed if more data is needed to be loaded ***NEEDS FIXING***
			if ( lWordsLeft )
			{
				if ( ( ( Data [ 0 ] >> 24 ) & 0x7f ) == 0x7 )
				{
					// VIF Code MARK after a vif code with I-bit set //
					
					// load vif code
					//VifCode.Value = Data [ lIndex++ ];
					VifCode.Value = *Data++;
					
					// store to CODE register
					VifRegs.CODE = VifCode.Value;
					
					// update index
					lVifIdx++;
					
					// vif code is 1 word
					lWordsLeft--;
					
					// MARK //
					
					// debug
					cout << "\nhps2x64 ALERT: VU: Mark encountered!!!\n";
					
					// set mark register
					// set from IMM
					// Vu0 and Vu1
					VifRegs.MARK = VifCode.IMMED;
					
					// also need to set MRK bit in STAT
					// this gets cleared when CPU writes to MARK register
					VifRegs.STAT.MRK = 1;
				}
			}
			
			
			// ***todo*** stop vif transfer
#ifdef VERBOSE_VUINT
			cout << "\nhps2x64: VU: stopping VIF transfer due to interrupt bit set!!!\n";
#endif

			VifStopped = 1;
			
			
					// if vu multi-threading is enabled...
					//if ( bMultiThreadingEnabled )
					//{
					//	// if vif is waiting for vu, then get ready to start multi-threading
					//	ullMultiThread_StartCycle = CycleCount + c_ullMultiThread_Cycles;
					//}
			
			
			// set STAT.VIS due to interrupt stall ??
			VifRegs.STAT.VIS = 1;
			
			}	// if ( ! ( VifRegs.ERR & 1 ) )
#ifdef VERBOSE_VUINT
			else
			{
				cout << "\nhps2x64: VU: VIF interrupt detected but was MASKED!!!\n";
			}
#endif
			
		}
	}
	
	// return the number of full quadwords transferred/processed
	return QWC_Transferred;
}






void VU::DMA_Read ( u32* Data, int ByteReadCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_Read " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}




void VU::DMA_Write ( u32* Data, int ByteWriteCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_Write " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}


u32 VU::DMA_WriteBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\nVU#" << Number << "::DMA_WriteBlock";
	debug << " " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount;
	debug << " APATH=" << GPU::_GPU->GIFRegs.STAT.APATH;
	debug << hex << " DATA= ";
	for ( int i = 0; i < ( QuadwordCount * 2 ); i++ ) debug << " " << Data [ i ];
	debug << "\r\n";
#endif

	u32 QWC_Transferred;
	u32 QWC_Remaining;
	
	u32 ulQWBuffer_Left;


	
	
	// check if QWC is zero
	if ( !QuadwordCount )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << " QWC_IS_ZERO";
#endif

		// if it transfers zero, then buffer must be empty
		VifRegs.STAT.FQC = 0;
		
		return 0;
	}

	
	// data being input
	VifRegs.STAT.FDR = 0;

	// set amount of data in buffer
	//VifRegs.STAT.FQC = QuadwordCount;

	
	// send data to VIF FIFO
	QWC_Transferred = VIF_FIFO_Execute ( (u32*) Data, QuadwordCount << 2 );
	
	
	if ( QWC_Transferred == QuadwordCount )
	{
		// transfer complete //
		lVifIdx = 0;
		
		//VifRegs.STAT.FQC = QuadwordCount;
	}

	// if it transfers zero, then buffer must be empty
	VifRegs.STAT.FQC = QuadwordCount - QWC_Transferred;
	
	// return the amount of quadwords processed
	return QWC_Transferred;

}




u32 VU::DMA_ReadBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_ReadBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	debug << "; VU#" << Number;
	debug << "\r\n";
#endif


	// data being output
	VifRegs.STAT.FDR = 1;
	
	// set amount of data in buffer
	VifRegs.STAT.FQC = QuadwordCount;

	
	// read data from gpu
	GPU::Path2_ReadBlock ( Data, QuadwordCount );
	
	return QuadwordCount;
}


u64 VU::DMA_Read_Ready ()
{
#ifdef INLINE_DEBUG_DMA_READ_READY
	debug << "\r\nVU#" << Number << "::DMA_Read_Ready";
	debug << " GPUDIR=" << GPU::_GPU->GIFRegs.STAT.DIR;
	debug << " BUSDIR=" << GPU::_GPU->GPURegs1.BUSDIR;
	debug << " XDIR=" << GPU::_GPU->GPURegsGp.TRXDIR.XDIR;
	debug << " P3E=" << GPU::_GPU->EndOfPacket [ 3 ];
	debug << " P3C=" << GPU::_GPU->ulTransferCount [ 3 ];
#endif

	// do this for now
	// ***todo*** check if read from VU1 is ready
	//return !VifStopped;
	
	/*
	if ( ( GPU::_GPU->GPURegs1.BUSDIR & 1 ) == 1 )
	{
		// data is going out of gpu //
		return true;
	}
	*/
	
	// can't read xdir unless we sychronize with gpu for now (cuz it could be multi-threaded)...
	// actually can't read anything period unless synchronized
	//GPU::Finish();
	
	// can't read the data if the transfer isn't ready
	if ( GPU::_GPU->GPURegsGp.TRXDIR.XDIR != 1 )
	{
#ifdef INLINE_DEBUG_DMA_READ_READY
		debug << " XDIR!=1";
#endif

		return 0;
	}
	
	if ( ( !GPU::_GPU->ulTransferCount [ 3 ] ) || ( !Dma::pRegData [ 2 ]->CHCR.STR ) )
	{
#ifdef INLINE_DEBUG_DMA_READ_READY
		debug << " !P3C||!STR2";
#endif

		// check if buffer is going in opposite direction
		if ( GPU::_GPU->GIFRegs.STAT.DIR )
		{
#ifdef INLINE_DEBUG_DMA_READ_READY
			debug << " DIR";
#endif

			return 1;
		}
	}

#ifdef INLINE_DEBUG_DMA_READ_READY
	debug << " OTHER";
#endif

	return 0;

}

u64 VU::DMA_Write_Ready ()
{
#ifdef INLINE_DEBUG_DMA_WRITE_READY
	debug << "\r\nVU#" << Number << "::DMA_Write_Ready";
	debug << " VifStopped=" << VifStopped;
	debug << " P3Q=" << GPU::_GPU->GIFRegs.STAT.P3Q;
#endif

	// if the VIF has been stopped, or if there is a packet in progress in path 3, then do not run dma1??
	//if ( ( VifStopped ) || ( GPU::_GPU->ulTransferCount [ 3 ] ) )
	if ( ( VifStopped ) )
	{
#ifdef ENABLE_QWBUFFER_DMA
		if ( ulQWBufferCount < ulQWBufferSize )
		{
			return 1;
		}
#endif

#ifdef INLINE_DEBUG_DMA_WRITE_READY
	debug << " NOT-READY-VIFSTOPPED";
#endif

		return 0;
	}
	
	/*
	if ( Number )
	{
		// if path 3 packet transfer is in progress and IMT is set to transfer in continuous mode
		if ( ( GPU::_GPU->ulTransferCount [ 3 ] ) && ( Dma::pRegData [ 2 ]->CHCR.STR ) && ( !GPU::_GPU->GIFRegs.MODE.IMT ) )
		{
			return 0;
		}
	}
	*/
	
	// check if stalled
	if ( VifRegs.STAT.VSS || VifRegs.STAT.VFS || VifRegs.STAT.VIS )
	{
#ifdef INLINE_DEBUG_DMA_WRITE_READY
	debug << " NOT-READY-STALL";
#endif

		return 0;
	}
	
	/*
	if ( bTransferringDirectViaPath2 && !GPU::_GPU->EndOfPacket [ 3 ] )
	{
		return 0;
	}
	*/
	
	// next, make sure this is not vu#0
	/*
	if ( Number )
	{
		// if path 3 packet transfer is in progress and IMT is set to transfer in continuous mode
		if ( ( GPU::_GPU->ulTransferCount [ 3 ] ) && ( !GPU::_GPU->GIFRegs.MODE.IMT ) )
		{
			return 0;
		}
	}
	*/

#ifdef INLINE_DEBUG_DMA_WRITE_READY
	debug << " READY-OK";
#endif

	//return 1;
	return BusyUntil_Cycle;
}



void VU::Update_QWBuffer ()
{
#ifdef INLINE_DEBUG_DMA_WRITE
		debug << "\r\nUPDATE-QWBUFFER";
#endif

	u32 QWC_Transferred;
	
	if ( !VifStopped )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
		debug << "\r\nVIF-RUNNING";
		debug << " (before)ulQWBufferCount=" << dec << ulQWBufferCount;
#endif

		if ( ulQWBufferCount )
		{
			QWC_Transferred = VIF_FIFO_Execute ( (u32*) ullQWBuffer, ulQWBufferCount << 2 );
			
			ulQWBufferCount -= QWC_Transferred;
		}
		
#ifdef INLINE_DEBUG_DMA_WRITE
		debug << " (after)ulQWBufferCount=" << dec << ulQWBufferCount;
#endif
	}
	else
	{
#ifdef INLINE_DEBUG_DMA_WRITE
		debug << "\r\nVIF-STOPPED";
#endif
	}
}



int VU::ExceptionFilter(LPEXCEPTION_POINTERS pEp)
{
	CONTEXT* ctx = pEp->ContextRecord;
	ULONG_PTR* info = pEp->ExceptionRecord->ExceptionInformation;
	UINT_PTR addr = info[1];
	DWORD dummy;

	u64 ullFunctionCode;
	u64 ullInstrPtr;
	u32 uCurVuNumber;

	u32 op1, op2, op3, result;
	u32 instr_opcode, instr_func, instr_func2;

	Vu::Instruction::Format instr;

	//bIsFloatException = false;
	bIsInvalidException = false;


	ullFunctionCode = ((u64)ctx->R8) >> 32;
	uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;

	uCurVuNumber = (u32)ctx->R9;

	instr_opcode = ullFunctionCode >> 26;
	instr_func = ullFunctionCode & 0x3f;

	instr_func2 = ullFunctionCode & 0x7ff;

	ullInstrPtr = ctx->Rip;
	instr.Value = ullFunctionCode;


	switch (pEp->ExceptionRecord->ExceptionCode)
	{
		/*
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
		*/

	case STATUS_FLOAT_INVALID_OPERATION:
#define VERBOSE_EXCEPTION_FILTER_INVALID
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID
		printf("\nhps2x64: status vu float invalid operation: pc=%x op1=%016llx %016llx op2=%016llx %016llx res=%016llx %016llx",
			uExceptionPC64, ctx->Xmm0.High, ctx->Xmm0.Low, ctx->Xmm1.High, ctx->Xmm1.Low, ctx->Xmm3.High, ctx->Xmm3.Low);
#endif

		//bIsFloatException = true;
		bIsInvalidException = true;

		//uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		//return EXCEPTION_EXECUTE_HANDLER;

		// interpret the instruction since it led to an exception
		Instruction::Execute::ExecuteInstructionHI(VU::_VU[uCurVuNumber], instr);

		// resume from the next instruction
		ctx->Rip = uExceptionPC64;

		// resume from the next instruction
		return EXCEPTION_CONTINUE_EXECUTION;
		
		break;

	case STATUS_FLOAT_OVERFLOW:
#define VERBOSE_EXCEPTION_FILTER_INVALID_OVERFLOW
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID_OVERFLOW
		printf("\nhps2x64: status vu float overflow: pc=%x",
			uExceptionPC64);
#endif

		//bIsFloatException = true;
		//uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		//return EXCEPTION_EXECUTE_HANDLER;

		// interpret the instruction since it led to an exception
		Instruction::Execute::ExecuteInstructionHI(VU::_VU[uCurVuNumber], instr);

		// resume from the next instruction
		ctx->Rip = uExceptionPC64;

		// resume from the next instruction
		return EXCEPTION_CONTINUE_EXECUTION;

		break;

	case STATUS_FLOAT_UNDERFLOW:
#define VERBOSE_EXCEPTION_FILTER_INVALID_UNDERFLOW
#ifdef VERBOSE_EXCEPTION_FILTER_INVALID_UNDERFLOW
		printf("\nhps2x64: status vu float underflow: pc=%x",
			uExceptionPC64);
#endif

		//bIsFloatException = true;
		//uExceptionPC64 = ((u64)ctx->R8) & 0xffffffffull;
		//return EXCEPTION_EXECUTE_HANDLER;

		// interpret the instruction since it led to an exception
		Instruction::Execute::ExecuteInstructionHI(VU::_VU[uCurVuNumber], instr);

		// resume from the next instruction
		ctx->Rip = uExceptionPC64;

		// resume from the next instruction
		return EXCEPTION_CONTINUE_EXECUTION;

		break;

	default:
		break;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void VU::Run ()
{
	u32 Index;
	int iMatch;

	// making these part of object rather than local function
	//Instruction::Format CurInstLO;
	//Instruction::Format CurInstHI;
	

	/////////////////////////////////
	// VU components:
	// 1. Instruction Execute Unit
	// 2. Delay Slot Unit
	// 3. Multiply/Divide Unit
	/////////////////////////////////
	
	
	// if VU is not running, update vu cycle and then return
	if ( Number && ulThreadCount )
	{
		// currently running vu#1 on a separate thread
		// uses a different variable to determine if running
		if ( !iRunning )
		{
			// if this is -1, then in macro mode will think it is time to update q
			CycleCount = -2ull;	// -1LL;
			return;
		}
	}
	else if ( !Running )
	{
		//CycleCount = *_DebugCycleCount;
		// if this is -1, then in macro mode will think it is time to update q
		CycleCount = -2ull;	// -1LL;
		eCycleCount = -2ull;	// -1ull;
		return;
	}

	
	
	if ( !Number )
	{
		// check watch dog
		if ( ( CycleCount - ullStartCycle ) > c_ullVU0WD_Cycles )
		{
			// stop vu
			StopVU ();
			
			// interrupt
			SetInterrupt_VU0WD ();
			
			// done
			return;
		}
	}
	
	

#ifdef INLINE_DEBUG
	debug << "\r\n->PC = " << hex << setw( 8 ) << PC << dec;
	debug << " VU#" << dec << Number;
#endif



#ifdef ENABLE_XGKICK_WAIT
	// if another path is in progress, then don't start transfer yet
	//if ( ( GPU::_GPU->GIFRegs.STAT.APATH ) && ( GPU::_GPU->GIFRegs.STAT.APATH != 1 ) )
	if ( Status.XgKick_Wait )
	{
		// make sure path 3 is not in an interruptible image transfer
		if ( ( GPU::_GPU->GIFRegs.STAT.APATH != 3 ) || ( !GPU::_GPU->GIFRegs.STAT.IMT ) || ( GPU::_GPU->GIFTag0 [ 3 ].FLG != 2 ) )
		{
			// path 1 in queue
			//GPU::_GPU->GIFRegs.STAT.P1Q = 1;
			
			// waiting
			//v->Status.XgKick_Wait = 1;
			
			// wait until transfer via the other path is complete
			//v->NextPC = v->PC;
			CycleCount++;
			return;
		}
		

		// if there is an xgkick instruction in progress, then need to complete it immediately
		/*
		if ( Status.XgKickDelay_Valid )
		{
			// looks like this is only supposed to write one 128-bit value to PATH1
			// no, this actually writes an entire gif packet to path1
			// the address should only be a maximum of 10-bits, so must mask
			//GPU::Path1_WriteBlock ( & v->VuMem64 [ ( v->XgKick_Address & 0x3ff ) << 1 ] );
			GPU::Path1_WriteBlock ( VuMem64, XgKick_Address & 0x3ff );
			
			// the previous xgkick is done with
			//v->Status.XgKickDelay_Valid = 0;
		}
		*/

		// no longer waiting
		Status.XgKick_Wait = 0;
		
		// path 1 no longer in queue
		GPU::_GPU->GIFRegs.STAT.P1Q = 0;

		// now transferring via path 1
		GPU::_GPU->GIFRegs.STAT.APATH = 1;

		
		// need to execute xgkick instruction, but it doesn't execute completely immediately and vu keeps going
		// so for now will delay execution for an instruction or two
		Status.XgKickDelay_Valid = 0x2;
		//XgKick_Address = v->vi [ i.is & 0xf ].uLo;
	}
#endif



	//cout << "\nVU -> running=" << Running;


	// check that static analysis has been run //

		// check that address block is encoded
		//if ( ! vrs [ Number ]->isRecompiled ( PC ) )
		if ( bCodeModified [ Number ] )
		{
#ifdef INLINE_DEBUG
	debug << ";NOT Recompiled";
#endif
			// VU code memory has been modified since the last time it was recompiled //

			if ( bRecompilerCacheEnabled )
			{
				// VU code caching ENABLED for recompiler //

				// get/calculate checksum for current contents of vu code memory
				ullRunningChecksum = Vu::Recompiler::Calc_Checksum( this );

				if ( vrs[ Number ]->ullChecksum != ullRunningChecksum )
				{
					// check if checksum for those vu code memory contents are already recompiled (todo: bloom filter??)
					for ( iMatch = 0; iMatch < VU::c_iVURecompilerCache_MaxEntries; iMatch++ )
					{
						// check if checksum matches for any vu code that has already been recompiled
						if ( vu_recompiler_cache [ iMatch ] [ Number ]->ullChecksum == ullRunningChecksum )
						{
							break;
						}
					}

					if ( iMatch < VU::c_iVURecompilerCache_MaxEntries)
					{
						// VU code memory contents were already cached //

						vrs [ Number ] = vu_recompiler_cache [ iMatch ] [ Number ];

						pStaticInfo [ Number ] = vrs [ Number ]->LUT_StaticInfo;
						pLUT_StaticInfo = pStaticInfo [ Number ];

#ifdef VERBOSE_CHECKSUM
			cout << "\nhps2x64: VU#" << Number << ": SUCCESS: found checksum match= " << hex << ullRunningChecksum;
#endif

#ifdef INLINE_DEBUG_VURUN
			debug << "\r\n***VU#" << Number << ": SUCCESS: found checksum match= " << hex << ullRunningChecksum << " iMatch=" << dec << iMatch;
#endif

					}
					else
					{
						// VU code memory contents NOT already cached, so need to recompile //

						// make this the current recompiled code that the vu is going to use
						vrs [ Number ] = vu_recompiler_cache [ iNextRecompile & VU::c_iVURecompilerCache_Mask] [ Number ];

						// perform static analysis
						vrs [ Number ]->StaticAnalysis ( this );

						// print results
						vrs[Number]->Print_StaticAnalysis(this);

						pStaticInfo [ Number ] = vrs [ Number ]->LUT_StaticInfo;
						pLUT_StaticInfo = pStaticInfo [ Number ];

						// perform recompile
						vrs [ Number ]->Recompile( this, PC );
						//vrs [ Number ]->Recompile2( this, PC );

						// set checksum for the recompiled code
						vrs [ Number ]->ullChecksum = ullRunningChecksum;



						// use a different cache block next time we do the recompile
						iNextRecompile++;

#ifdef VERBOSE_CHECKSUM
			cout << "\nhps2x64: VU#" << Number << ": FAIL: no checksum match= " << hex << ullRunningChecksum;
#endif

#ifdef INLINE_DEBUG_VURUN
			debug << "\r\n***VU#" << Number << ": FAIL: no checksum match= " << hex << ullRunningChecksum;
#endif

					}	// end if ( iMatch < 32 ) else

				}	// end if ( vrs[ Number ]->ullChecksum != ullRunningChecksum )

			}
			else
			{
				// VU code caching DISABLED for recompiler //

				// perform static analysis
				vrs [ Number ]->StaticAnalysis ( this );

				// print static analysis
				vrs[Number]->Print_StaticAnalysis(this);

				pStaticInfo [ Number ] = vrs [ Number ]->LUT_StaticInfo;
				pLUT_StaticInfo = pStaticInfo [ Number ];

				// recompile block
				vrs [ Number ]->Recompile ( this, PC );
				//vrs [ Number ]->Recompile2( this, PC );



				// this way, if caching gets re-enabled it will start all over again
				vrs [ Number ]->ullChecksum = -1ull;

			}	// end if ( bRecompilerCacheEnabled ) else
			
			// code has been recompiled so only need to recompile again if modified
			bCodeModified [ Number ] = 0;

#ifdef VERBOSE_RECOMPILE
			cout << "\nhps2x64: VU: ALERT: code has been recompiled.";
#endif
#ifdef VERBOSE_CHECKSUM
			cout << "\nhps2x64: VU#" << Number << ": ALERT: checksum for new code= " << hex << Vu::Recompiler::Calc_Checksum( this );
			//cout << "\nhps2x64: VU#" << Number << ": ALERT: running checksum for new code= " << hex << ullRunningChecksum;
#endif

		}	// end if ( bCodeModified [ Number ] )

	
	/////////////////////////
	// Execute Instruction //
	/////////////////////////
	
	
#ifdef INLINE_DEBUG
	debug << ";Execute";
#endif

	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	//GPR [ 0 ].u = 0;
	vi [ 0 ].u = 0;
	
	// f0 always 0, 0, 0, 1 ???
	vf [ 0 ].uLo = 0;
	vf [ 0 ].uHi = 0;
	vf [ 0 ].uw = 0x3f800000;

	// update instruction
	NextPC = PC + 8;

#ifdef ENABLE_RECOMPILER_VU
	if ( ( Status.Value ) || ( !bEnableRecompiler ) )
	{
#endif

	// VU recompiler is DISABLED //


	//cout << "\nVU -> load instruction";
	
	// get the instruction to execute
	CurInstLOHI.ValueLoHi = MicroMem64 [ PC >> 3 ];
	
	
	
	// alert if D-bit set //
	if ( CurInstLOHI.D )
	{
		// register #28 is the FBRST register
		// the de bit says if the d-bit is enabled or not
		// de0 register looks to be bit 2
		// de1 register looks to be bit 10
		if ( !Number )
		{
			// check de0
			if ( vi [ 28 ].u & ( 1 << 2 ) )
			{
				cout << "\nhps2x64: ALERT: VU#" << Number << " D-bit is set!\n";
			}
		}
		else
		{
			// check de1
			if ( vi [ 28 ].u & ( 1 << 10 ) )
			{
				cout << "\nhps2x64: ALERT: VU#" << Number << " D-bit is set!\n";
			}
		}
	}

	// alert if T-bit set //
	if ( CurInstLOHI.T )
	{
		cout << "\nhps2x64: ALERT: VU#" << Number << " T-bit is set!\n";
	}


	u32 RecompileCount;
	RecompileCount = ( PC & ulVuMem_Mask ) >> 3;


	

	// check if E-bit is set (means end of execution after E-bit delay slot)
	if (CurInstLOHI.E)
	{
#ifdef INLINE_DEBUG
		debug << "; ***E-BIT SET***";
#endif

		Status.EBitDelaySlot_Valid |= 0x2;
	}


#ifdef VERBOSE_RECOMPILE
	// alert if the instruction is parallel that is being interpreted
	if (pLUT_StaticInfo[RecompileCount] & Vu::Recompiler::STATIC_PARALLEL_EXE_MASK)
	{
		cout << "\nHPS2X64: VU: EXECUTION: ALERT: Interpreted parallel instruction at index#" << ((pLUT_StaticInfo[RecompileCount] & Vu::Recompiler::STATIC_PARALLEL_INDEX_MASK) >> Vu::Recompiler::STATIC_PARALLEL_INDEX_SHIFT);
	}
#endif

	
	// execute HI instruction first ??
	
	// check if Immediate or End of execution bit is set
	if ( CurInstLOHI.I )
	{
		// lower instruction contains an immediate value //
		
		// *important* MUST execute the HI instruction BEFORE storing the immediate
		Instruction::Execute::ExecuteInstructionHI ( this, CurInstLOHI.Hi );
		
		// load immediate regiser with LO instruction
		vi [ 21 ].u = CurInstLOHI.Lo.Value;
	}
	else
	{
		// execute lo/hi instruction normally //
		// unsure of order

#ifdef USE_NEW_RECOMPILE2_EXEORDER


		// check if instruction order got swapped (analysis bit 20) //
		if (pLUT_StaticInfo[ RecompileCount ] & Vu::Recompiler::STATIC_REVERSE_EXE_ORDER)
		{
#ifdef INLINE_DEBUG_RECOMPILE2
	debug << "\r\n>EXE-ORDER-SWAP";
#endif

			// execute HI instruction
			Instruction::Execute::ExecuteInstructionHI ( this, CurInstLOHI.Hi );
		}


		// check if should ignore lower instruction (analysis bit 4)
		if ( !( pLUT_StaticInfo[ RecompileCount ] & Vu::Recompiler::STATIC_SKIP_EXEC_LOWER) )
		{
			// execute LO instruction since it is an instruction rather than an immediate value
			Instruction::Execute::ExecuteInstructionLO ( this, CurInstLOHI.Lo );
		}
		
		if ( !( pLUT_StaticInfo[ RecompileCount ] & Vu::Recompiler::STATIC_REVERSE_EXE_ORDER) )
		{
			// execute HI instruction
			Instruction::Execute::ExecuteInstructionHI ( this, CurInstLOHI.Hi );
		}



#else

		Instruction::Execute::ExecuteInstructionLO ( this, CurInstLOHI.Lo );
		Instruction::Execute::ExecuteInstructionHI ( this, CurInstLOHI.Hi );
#endif

		
	}


	


#ifdef ENABLE_RECOMPILER_VU
	}	// end if ( ( Status.Value ) || ( !bEnableRecompiler ) )
	else
	{
		// VU recompiler is ENABLED //


		u32 RecompileCount;
		RecompileCount = ( PC & ulVuMem_Mask ) >> 3;


#ifdef INLINE_DEBUG
	debug << ";Recompiled";
	debug << ";PC=" << hex << PC;
	debug << " Status=" << hex << Status.Value;
#endif

		// clear branches
		//Recompiler::Status_BranchDelay = 0;
		Recompiler_EnableFlags = 0;
		
		// get the block index
		Index = vrs [ Number ]->Get_Index ( PC );

#ifdef INLINE_DEBUG
	debug << ";Index(dec)=" << dec << Index;
	//debug << ";Index(hex)=" << hex << Index;
	debug << " CntOffset=" << dec << vrs [ Number ]->CycleCount [ Index ];
	debug << " codeptr=" << hex << ((u64) vrs [ Number ]->pCodeStart [ Index ]);
#endif


#ifdef INLINE_DEBUG_RECOMPILE2
	debug << " InstrCount=" << dec << InstrCount << " " << hex << InstrCount;
	debug << " RecCycleCnt=" << dec << vrs [ Number ]->CycleCount [ Index ] << " " << hex << vrs [ Number ]->CycleCount [ Index ];
	debug << " Index=" << dec << Index;
	debug << " Number=" << dec << Number;
#endif


		// offset cycles before the run, so that it updates to the correct value
		// note: needs to be commented out if updating cyclecount in recompiler
		//CycleCount -= vrs [ Number ]->CycleCount [ Index ];

		// offset instruction count
		InstrCount -= vrs [ Number ]->CycleCount [ Index ];

		__try
		{
			// already checked that is was either in cache, or etc
			// execute from address
			//cout << "\nCalling vu code at address: " << hex << (u64)(vrs[Number]->pCodeStart[Index]) << " PC=" << PC;
			((func2)(vrs[Number]->pCodeStart[Index])) ();
		}
		__except (ExceptionFilter(GetExceptionInformation()))
		{
			u32 uSrcAddr;

			//u32 uBlockStartAddr;
			u64 uSrcCycles;

			// get addr that block starts at
			//u32 uBlockStartAddr = PC & ~0x3f;


			//#define VERBOSE_VIRTUAL_MACHINE
#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\nhpsx64: EXCEPTION: Exception code: " << hex << GetExceptionCode();
			cout << "\nException read/write address: " << hex << uExceptionRWAddr64 << " from code at address: " << uExceptionInstrAddr64;
			cout << "\nVU PC: " << hex << PC << " NextPC: " << NextPC << " LastPC: " << LastPC << " Status:" << Status.Value;
			cout << "\nCycle#" << CycleCount << " NextEvent:" << *_NextSystemEvent;
			//cout << "\nhpsx64: EXCEPTION: Accessing address: " << hex << addr;
#endif


			// get the source ps1 address that affected R3000A instruction is at
			uSrcAddr = uExceptionPC64 & ulVuMem_Mask;

			// get the cycles to update after exception
			//uSrcCycles = uExceptionCycles64;
			//uSrcCycles = vrs[Number]->get_cycle_count(uSrcAddr);

#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << " R8PC=" << hex << uExceptionPC64 << dec << " R8Cycles=" << ((s32)uExceptionCycles64) << " StartCycle:" << rs->CycleCount[Index] << " EndCycle:" << (u64)rs->get_cycle_count(uSrcAddr);
#endif

#ifdef VERBOSE_VIRTUAL_MACHINE
			//u32 Instr0 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) - 1];
			//u32 Instr1 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) + 0];
			//u32 Instr2 = _SYSTEM._CPU.Bus->MainMemory.b32[((uSrcAddr & _SYSTEM._CPU.Bus->MainMemory_Mask) >> 2) + 1];
			u32 Instr0 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) - 1];
			u32 Instr1 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 0];
			u32 Instr2 = Bus->BIOS.b32[((uSrcAddr & Bus->BIOS_Mask) >> 2) + 1];

			cout << "\nthe instructions are:";
			cout << "\n#0: " << hex << Instr0;
			cout << "\n#1: " << hex << Instr1;
			cout << "\n#2: " << hex << Instr2;

#endif

			// update the cycles before interpreting instruction
			//CycleCount += uSrcCycles;

			// instruction that was interrupted needs to be interpreted //
			PC = uSrcAddr;
			NextPC = PC + 8;

			// get the block index
			Index = vrs[Number]->Get_Index(PC);

			InstrCount += vrs[Number]->CycleCount[Index];


			RecompileCount = (PC & ulVuMem_Mask) >> 3;

			// get the instruction to execute
			CurInstLOHI.ValueLoHi = MicroMem64[PC >> 3];

			if (CurInstLOHI.E)
			{
				Status.EBitDelaySlot_Valid |= 0x2;
			}

			// check if Immediate or End of execution bit is set
			if (CurInstLOHI.I)
			{
				// lower instruction contains an immediate value //

				// *important* MUST execute the HI instruction BEFORE storing the immediate
				Instruction::Execute::ExecuteInstructionHI(this, CurInstLOHI.Hi);

				// load immediate regiser with LO instruction
				vi[21].u = CurInstLOHI.Lo.Value;
			}
			else
			{
				// execute lo/hi instruction normally //

				// check if instruction order got swapped (analysis bit 20) //
				if (pLUT_StaticInfo[RecompileCount] & Vu::Recompiler::STATIC_REVERSE_EXE_ORDER)
				{
#ifdef INLINE_DEBUG_RECOMPILE2
					debug << "\r\n>EXE-ORDER-SWAP";
#endif

					// execute HI instruction
					Instruction::Execute::ExecuteInstructionHI(this, CurInstLOHI.Hi);

					// check if should ignore lower instruction
					if (!(pLUT_StaticInfo[RecompileCount] & Vu::Recompiler::STATIC_SKIP_EXEC_LOWER))
					{
						// execute LO instruction since it is an instruction rather than an immediate value
						Instruction::Execute::ExecuteInstructionLO(this, CurInstLOHI.Lo);
					}
				}
				//if (!(pLUT_StaticInfo[RecompileCount] & Vu::Recompiler::STATIC_REVERSE_EXE_ORDER))
				else
				{
					// execute HI instruction
					Instruction::Execute::ExecuteInstructionHI(this, CurInstLOHI.Hi);
				}



			}




#ifdef VERBOSE_VIRTUAL_MACHINE
			cout << "\nStart addr for the block: " << hex << uBlockStartAddr;
			cout << "\nPS2 Source Address for affected instruction: " << uSrcAddr;
#endif

			if (bIsInvalidException)
			{
				// mark the problem instruction in the bitmap //
				vrs[Number]->bitset_hwrw_bitmap(uSrcAddr);

				// recompile block
				vrs[Number]->Recompile(this, PC);

				// this way, if caching gets re-enabled it will start all over again
				//vrs[Number]->ullChecksum = -1ull;
			}

		}	// end __try __except (ExceptionFilter(GetExceptionInformation()))


		//cout << "\nReturning from vu code.";

#ifdef INLINE_DEBUG
	debug << "\r\n->RecompilerReturned";
	debug << " VU#" << dec << Number;
#endif

#ifdef INLINE_DEBUG_RECOMPILE2
	debug << " ->InstrCount=" << dec << InstrCount << " " << hex << InstrCount;
#endif

			
	}
#endif


	
	///////////////////////////////////////////////////
	// Check if there is anything else going on

#ifdef ENABLE_XGKICK_TIMING

	// check if there is data transferring via path 1
	if ( bXgKickActive )
	{
		// update ongoing transfer via path 1
		Execute_XgKick();
	}

#endif

	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Check delay slots
	// need to do this before executing instruction because load delay slot might stall pipeline

	// check if anything is going on with the delay slots
	// *note* by doing this before execution of instruction, pipeline can be stalled as needed
	if ( Status.Value )
	{
		// clear status/clip set flags
		Status.SetStatus_Flag = 0;
		Status.SetClip_Flag = 0;

		/////////////////////////////////////////////
		// check for anything in delay slots

#ifdef ENABLE_INTDELAYSLOT
		if ( Status.IntDelayValid )
		{
#ifdef INLINE_DEBUG_INTDELAYSLOT
			debug << "\r\nVU::RUN->Status.IntDelayValid=" << (u32)Status.IntDelayValid;
#endif

			Status.IntDelayValid >>= 1;
			
			if ( !Status.IntDelayValid )
			{
#ifdef INLINE_DEBUG_INTDELAYSLOT
			debug << " IntDelayReg=" << IntDelayReg << " IntDelayValue=" << IntDelayValue;
#endif

				vi [ IntDelayReg ].u = IntDelayValue;
			}
		}
#endif
		


		
#ifdef INLINE_DEBUG
		debug << ";DelaySlotValid";
		debug << "; Status=" << hex << Status.Value;
#endif
		
		//if ( Status.DelaySlot_Valid & 1 )
		if ( Status.DelaySlot_Valid )
		{
#ifdef INLINE_DEBUG
			debug << ";Delay1.Value";
#endif
			
			Status.DelaySlot_Valid >>= 1;
			
			if ( !Status.DelaySlot_Valid )
			{
				// clear instr count when branching
				InstrCount = -1;

				ProcessBranchDelaySlot ();
			}
			
			///////////////////////////////////
			// move delay slot
			NextDelaySlotIndex ^= 1;
		}
		

		// *** TODO: update to include path3 checks and better path1 processing etc *** //
		//if ( ( Status.XgKickDelay_Valid ) && ( GPU::_GPU->EndOfPacket [ 2 ] ) )
		if ( Status.XgKickDelay_Valid )
		{
			Status.XgKickDelay_Valid >>= 1;
			
			if ( !Status.XgKickDelay_Valid )
			{
				Execute_XgKick ();
			}
		}
		
		// check for end of execution for VU
		// *TODO* - this might not work for two e-bit delay slots next to each other
		if ( Status.EBitDelaySlot_Valid )
		{
#ifdef INLINE_DEBUG_VURUN
			debug << "\r\nEBitDelaySlot_Valid; VU#" << dec << Number;
			debug << " @Cycle#" << dec << *_DebugCycleCount;
			debug << " EBitDelaySlot_Valid=" << hex << (u32) Status.EBitDelaySlot_Valid;
			debug << " PC=" << hex << PC;
			debug << " P1BufSize=" << GPU::_GPU->ullP1Buf_WriteIndex;
#endif

			Status.EBitDelaySlot_Valid >>= 1;

			// need to stop the vu from executing the last instruction
			// so it is like it is running until the cycle# it was supposed to stop at
			//bMultiThreadingActive = 0;
			
			if ( !Status.EBitDelaySlot_Valid )
			{
#ifdef INLINE_DEBUG_VURUN
			debug << "\r\nEBitDelaySlot; VU#" << dec << Number << " DONE";
			debug << " @Cycle#" << dec << CycleCount;	//*_DebugCycleCount;
			debug << " RunTime=" << dec << ( CycleCount - ullStartCycle );
			debug << " PC=" << hex << PC;
#endif

#ifdef DISALLOW_XGKICK_IN_DELAY_SLOT
				// not supposed to put xgkick in a delay slot ??
				Status.XgKickDelay_Valid = 0;
#endif

				//if ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
				//{
				//	GPU::_GPU->GIFRegs.STAT.APATH = 0;
				//}


#ifdef DISALLOW_LOADSTORE_IN_DELAY_SLOT
				// ***TODO*** just stop load/store, not move?
				switch( CurInstLOHI.Lo.Opcode )
				{
					// LQ
					case 0:
					// SQ
					case 1:
					// ILW
					case 4:
					// ISW
					case 5:
						Status.EnableLoadMoveDelaySlot = 0;
						break;
						
					case 0x40:
						if ( ( CurInstLOHI.Lo.Fd == 0xd ) && ( ( CurInstLOHI.Lo.Funct & 0x3c ) == 0x3c ) )
						{
							Status.EnableLoadMoveDelaySlot = 0;
						}

						if ( ( CurInstLOHI.Lo.Fd == 0xf ) && ( ( CurInstLOHI.Lo.Funct & 0x3e ) == 0x3e ) )
						{
							Status.EnableLoadMoveDelaySlot = 0;
						}
						
						break;
				}
#endif

#ifdef DISALLOW_BRANCH_IN_DELAY_SLOT
				Status.DelaySlot_Valid = 0;
#endif

				// check if vu is running on another thread
				if ( Number && ulThreadCount )
				{
					// stop vu running on a separate vu thread //
					
					// check if this is vu#1 and running on another thread
					// stop the vu when it is complete
					iStopVU ();

					// if Vif is stopped for some reason, it can resume transfer now
					//iVifStopped = 0;

				}
				else
				{
					// check if this is vu0 that is going to continue rather than stop
					if ( !bContinueVU0 )
					{
						// just a normal full stop of vu0 or vu1 //

						// reset instruction count unless vu0 is going to continue
						InstrCount = -1;

						// make sure q and p are updated properly ??
						WaitQ ();
						WaitP ();

						// also make sure mac/stat/clip fill the buffer
						FlagSave[0].MACFlag = vi[VU::REG_MACFLAG].s;
						FlagSave[1].MACFlag = vi[VU::REG_MACFLAG].s;
						FlagSave[2].MACFlag = vi[VU::REG_MACFLAG].s;
						FlagSave[3].MACFlag = vi[VU::REG_MACFLAG].s;

						FlagSave[0].StatusFlag = vi[VU::REG_STATUSFLAG].s;
						FlagSave[1].StatusFlag = vi[VU::REG_STATUSFLAG].s;
						FlagSave[2].StatusFlag = vi[VU::REG_STATUSFLAG].s;
						FlagSave[3].StatusFlag = vi[VU::REG_STATUSFLAG].s;

						FlagSave[0].ClipFlag = vi[VU::REG_CLIPFLAG].s;
						FlagSave[1].ClipFlag = vi[VU::REG_CLIPFLAG].s;
						FlagSave[2].ClipFlag = vi[VU::REG_CLIPFLAG].s;
						FlagSave[3].ClipFlag = vi[VU::REG_CLIPFLAG].s;

						FlagSave[0].ullBusyUntil_Cycle = 1;
						FlagSave[1].ullBusyUntil_Cycle = 1;
						FlagSave[2].ullBusyUntil_Cycle = 1;
						FlagSave[3].ullBusyUntil_Cycle = 1;
					}
					
					// handled until next time
					bContinueVU0 = 0;

					// check if this is vu#1 and running on another thread
					// stop the vu when it is complete
					StopVU ();


					// for now, path 1 is done
					if ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
					{
						//cout << "\nhps2x64: VU1: ALERT: apath=1 at conclusion of vu1 run\n";

						GPU::_GPU->GIFRegs.STAT.APATH = 0;

						if ( VifStopped )
						{
							// also, need to re-enable the vif when done in path 1 in case it was stopped
							VifStopped = 0;

							// condition for dma changed, so check dma
							Dma::_DMA->CheckTransfer ();
						}
					}


					// if Vif is stopped for some reason, it can resume transfer now
					//VifStopped = 0;

					// ***todo*** also need to notify DMA to restart if needed
					//Dma::_DMA->Transfer ( Number );
					//Dma::_DMA->CheckTransfer ();

					// if vu is not running, then multi-threading obviously isn't active
					//bMultiThreadingActive = 0;
					
					// if no longer running, then no longer multi-threading either
					//ullMultiThread_StartCycle = 0;
				
#ifdef ENABLE_PATH1_BUFFER
					// this looks like the best place for this, at the end of the vu1 run
					//GPU::Path1_ProcessBlocks ();
#endif

#ifdef ENABLE_QWBUFFER
					// update the qw buffer
					Update_QWBuffer();
#endif
				}


				// for VU#1, clear pipelines
				//if ( Number )
				//{
				//	Pipeline_Bitmap = 0;
				//	Int_Pipeline_Bitmap = 0;
				//}
				
				
				


//#ifdef INLINE_DEBUG_VURUN
//			debug << "->DMA Should be restarted";
//#endif

			}
		}

#ifdef ENABLE_STALLS
		// for load/move delay slot (must only write after execution of upper instruction unless writing to integer register
		if ( Status.EnableLoadMoveDelaySlot )
		{
			// this clears the EnableLoadMoveDelaySlot, so no need to do it here
			//Instruction::Execute::Execute_LoadDelaySlot ( this, CurInstLO );
			Instruction::Execute::Execute_LoadDelaySlot ( this, CurInstLOHI.Lo );
		}
#endif

		//cout << hex << "\n" << DelaySlot1.Value << " " << DelaySlot1.Value2 << " " << DelaySlot0.Value << " " << DelaySlot0.Value2;
		
	}	// end if ( Status.Value )


	/////////////////////////////////////
	// Update Program Counter
	LastPC = PC;
	PC = NextPC;

#ifdef USE_NEW_RECOMPILE2

	// only update the cycle count and instruction count
	//CycleCount++;
	InstrCount++;

	AdvanceCycle ();

	// ***testing*** add an update q before returning
	//UpdateQ();

#else

#ifdef ENABLE_STALLS

	// advance the vu cyclecount/pipeline, etc
	AdvanceCycle ();
#else
	// this counts the bus cycles, not R5900 cycles
	CycleCount++;

	// update q and p registers here for now
	UpdateQ ();
	UpdateP ();

	// update the flags here for now
	UpdateFlags ();

#endif	// end #ifdef ENABLE_STALLS

#endif	// end #ifdef USE_NEW_RECOMPILE2

	// if not multi-threading, update the externally viewable cycle#
	if ( !Number || !ulThreadCount )
	{
		eCycleCount = CycleCount;
	}

}



void VU::ProcessBranchDelaySlot ()
{
	Vu::Instruction::Format i;
	u32 Address;
	
	DelaySlot *d = & ( DelaySlots [ NextDelaySlotIndex ] );

	
#ifdef ENABLE_PIPELINE_CLEAR_ON_BRANCH
	// clear the pipeline when branching?
	ClearFullPipeline ();
	
	// cycle time to refill the pipeline ?
	//CycleCount += 6;
#endif
	
	//i = DelaySlot1.Instruction;
	i = d->Instruction;
	
	
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
		
			NextPC = PC + ( i.Imm11 << 3 );
			
			// make sure that address is not outside range (no memory management on VU)
			NextPC &= ulVuMem_Mask;
			
			break;
			
			
		// JALR
		case 0x25:
		
		// JR
		case 0x24:
		
			// it must be multiplying the register address by 8 before jumping to it
			//NextPC = d->Data;
			NextPC = d->Data << 3;
			
			// make sure that address is not outside range (no memory management on VU)
			NextPC &= ulVuMem_Mask;
			
			break;
	}

#ifdef ENABLE_VU_MULTI_EXECUTE
	ulRunStartAddress = NextPC;
#endif
	
}



u32* VU::GetMemPtr ( u32 Address32 )
{
	u32 Reg;
	
#ifdef INLINE_DEBUG_GETMEMPTR
	debug << "; Address32=" << hex << Address32;
	debug << "; Number=" << Number;
#endif

	// check if this is VU#0
	if ( !Number )
	{
		// VU#0 //
		
		// the upper part of address should probably be 0x4000 exactly to access vu1 regs
#ifdef ALL_VU0_UPPER_ADDRS_ACCESS_VU1
		if ( Address32 & ( 0x4000 >> 2 ) )
#else
		if ( ( Address32 & ( 0xf000 >> 2 ) ) == ( 0x4000 >> 2 ) )
#endif
		{
			// get the register number
			Reg = ( Address32 >> 2 ) & 0x1f;
			
			// check if these are float/int/etc registers being accessed
			// note: makes more sense to shift over one more and check
			// unknown what addresses might be mirrored here
			//switch ( ( Address32 >> 6 ) & 0xf )
			//switch ( ( Address32 >> 7 ) & 0x1f )
			switch ( ( Address32 >> 7 ) & 0x1 )
			{
				// float
				case 0:
				//case 1:
					return & VU1::_VU1->vf [ Reg ].uw0;
					break;
					
				// int/control
				//case 2:
				//case 3:
				case 1:
					return & VU1::_VU1->vi [ Reg ].u;
					break;
					
				default:
#ifdef INLINE_DEBUG_GETMEMPTR_INVALID
	debug << "\r\nERROR: VU0: referencing VU1 reg outside of range. Address=" << hex << ( Address32 << 2 );
#endif

					cout << "\nhps2x64: ERROR: VU0: referencing VU1 reg outside of range. Address=" << hex << ( Address32 << 2 );
					break;
			}
		}
		
		return & ( VuMem32 [ Address32 & ( c_ulVuMem0_Mask >> 2 ) ] );
	}
	
	return & ( VuMem32 [ Address32 & ( c_ulVuMem1_Mask >> 2 ) ] );
}


void VU::Perform_UnJump ( VU* v, Instruction::Format CurInstHI )
{
	u32 RecompileCount;
	RecompileCount = ( v->PC & v->ulVuMem_Mask ) >> 3;

	// check if instr count is zero
	if ( ( v->InstrCount + RecompileCount ) == 0 )
	{
		cout << "\nPerform_UnJump: ALERT: InstrCount is ZERO!!!\n";
	}

	// clear instruction count
	v->InstrCount = -1;

	// update cycle count
	// note: plus 5 cycles to branch ??
	//CycleCount += 5;

	// notify vu that branch has been hit
	v->Status.DelaySlot_Valid = 1;


	// if int was put into delay slot, might need to handle this

	// if the e-bit is set, MUST handle it after landing from jump
	if ( v->pLUT_StaticInfo[ RecompileCount + 1 ] & ( 1 << 17 ) )
	{
		v->Status.EBitDelaySlot_Valid |= 0x2;
	}
}


void VU::UpdateQ_Micro2 ( VU* v )
{
	// check if div/sqrt unit is done
	if ( v->CycleCount >= v->QBusyUntil_Cycle )
	{
		// get next q value
		v->vi [ VU::REG_Q ].u = v->NextQ.lu;

		// get next q flag
		v->vi [ VU::REG_STATUSFLAG ].u &= ~0x30;
		v->vi [ VU::REG_STATUSFLAG ].u |= v->NextQ_Flag;

		// clear nextq flag after it has been updated
		//NextQ_Flag = 0;
		v->QBusyUntil_Cycle = -1ull;
	}
}

void VU::WaitQ_Micro2 ( VU* v )
{
	// make sure there is a value in the pipeline
	if ( v->QBusyUntil_Cycle != -1 )
	{
		// check if div/sqrt unit is done
		if ( v->CycleCount < v->QBusyUntil_Cycle )
		{
			v->CycleCount = v->QBusyUntil_Cycle;
		}

		// get next q value
		v->vi [ VU::REG_Q ].u = v->NextQ.lu;

		// get next q flag
		v->vi [ VU::REG_STATUSFLAG ].u &= ~0x30;
		v->vi [ VU::REG_STATUSFLAG ].u |= v->NextQ_Flag;

		// clear nextq flag after it has been updated
		//NextQ_Flag = 0;
		v->QBusyUntil_Cycle = -1ull;
	}
}


u32 VU::Get_MacFlag2 ( VU* v )
{
	//u32 RecompileCount;
	//RecompileCount = ( v->PC & v->ulVuMem_Mask ) >> 3;

	v->Current_MacFlag = v->History_MacStatFlag [ v->iMacStatIndex & 0x3 ].MacFlag;
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 1 ) & 0x3 ] )
	{
		v->Current_MacFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 1 ) & 0x3 ].MacFlag;
	}
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 2 ) & 0x3 ] )
	{
		v->Current_MacFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 2 ) & 0x3 ].MacFlag;
	}
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 3 ) & 0x3 ] )
	{
		v->Current_MacFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 3 ) & 0x3 ].MacFlag;
	}

	return v->Current_MacFlag;
}

u32 VU::Get_StatFlag2 ( VU* v )
{
	//u32 RecompileCount;
	//RecompileCount = ( v->PC & v->ulVuMem_Mask ) >> 3;

	v->Current_StatFlag = v->History_MacStatFlag [ v->iMacStatIndex & 0x3 ].StatFlag;
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 1 ) & 0x3 ] )
	{
		v->Current_StatFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 1 ) & 0x3 ].StatFlag;
	}
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 2 ) & 0x3 ] )
	{
		v->Current_StatFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 2 ) & 0x3 ].StatFlag;
	}
	if ( ( v->CycleCount - 4 ) >= v->History_MacStatCycle [ ( v->iMacStatIndex + 3 ) & 0x3 ] )
	{
		v->Current_StatFlag = v->History_MacStatFlag [ ( v->iMacStatIndex + 3 ) & 0x3 ].StatFlag;
	}

	return v->Current_StatFlag;
}

u32 VU::Get_ClipFlag2 ( VU* v )
{
	//u32 RecompileCount;
	//RecompileCount = ( v->PC & v->ulVuMem_Mask ) >> 3;

	v->Current_ClipFlag = v->History_ClipFlag [ v->iClipIndex & 0x3 ];
	if ( ( v->CycleCount - 4 ) >= v->History_ClipCycle [ ( v->iClipIndex + 1 ) & 0x3 ] )
	{
		v->Current_ClipFlag = v->History_ClipFlag [ ( v->iClipIndex + 1 ) & 0x3 ];
	}
	if ( ( v->CycleCount - 4 ) >= v->History_ClipCycle [ ( v->iClipIndex + 2 ) & 0x3 ] )
	{
		v->Current_ClipFlag = v->History_ClipFlag [ ( v->iClipIndex + 2 ) & 0x3 ];
	}
	if ( ( v->CycleCount - 4 ) >= v->History_ClipCycle [ ( v->iClipIndex + 3 ) & 0x3 ] )
	{
		v->Current_ClipFlag = v->History_ClipFlag [ ( v->iClipIndex + 3 ) & 0x3 ];
	}

	return v->Current_ClipFlag;
}



void VU::PipelineWait_FMAC ()
{
	// FMAC pipeline isn't more than 4 cycles long, so this guards against an infinite loop
	static const u32 c_CycleTimeout = 3;
	
	u32 Count;
	
	for ( Count = 0; Count < c_CycleTimeout; Count++ )
	{
		// advance to next pipeline stage/next cycle
		AdvanceCycle ();
		
		// check if pipeline is still stalled
		if ( !TestStall () )
		{
			// the registers that were needed are now ready for use
			return;
		}
		
	}
	
	// time out, which should never happen theoretically!!!
	cout << "\nhps2x64: VU" << dec << Number << ": SERIOUS ERROR: FMAC Pipeline wait timeout!!! Should never happen!\n";
	
#ifdef INLINE_DEBUG_PIPELINE
	debug << "\r\nhps2x64: VU#" << dec << Number << ": SERIOUS ERROR: FMAC Pipeline wait timeout!!! Should never happen! P0=" << hex << Pipeline_Bitmap.b0 << " P1=" << Pipeline_Bitmap.b1 << " S0=" << SrcRegs_Bitmap.b0 << " S1=" << SrcRegs_Bitmap.b1 << " PC=" << PC << " Cycle#" << dec << CycleCount << "\r\n";
	debug << " fi=" << (( iFlagSave_Index - 4 ) & c_lFlag_Delay_Mask);
	debug << " fb0= " << hex << FlagSave [ 0 ].Bitmap.b0 << " " << FlagSave [ 0 ].Bitmap.b1;
	debug << " fb1= " << FlagSave [ 1 ].Bitmap.b0 << " " << FlagSave [ 1 ].Bitmap.b1;
	debug << " fb2= " << FlagSave [ 2 ].Bitmap.b0 << " " << FlagSave [ 2 ].Bitmap.b1;
	debug << " fb3= " << FlagSave [ 3 ].Bitmap.b0 << " " << FlagSave [ 3 ].Bitmap.b1;
#endif
}

void VU::PipelineWait_INT ()
{
	// Integer pipeline isn't more than 4 cycles long, so this guards against an infinite loop
	static const u32 c_CycleTimeout = 3;
	
	u32 Count;
	
	for ( Count = 0; Count < c_CycleTimeout; Count++ )
	{
		// advance to next pipeline stage/next cycle
		AdvanceCycle ();
		
		// check if pipeline is still stalled
		if ( !TestStall_INT () )
		{
			// the registers that were needed are now ready for use
			return;
		}
		
	}
	
	// time out, which should never happen theoretically!!!
	cout << "\nhps2x64: VU: SERIOUS ERROR: INT Pipeline wait timeout!!! Should never happen!\n";
	
#ifdef INLINE_DEBUG_PIPELINE
	debug << "\r\nhps2x64: VU: SERIOUS ERROR: INT Pipeline wait timeout!!! Should never happen!\r\n";
#endif
}


/*
// force pipeline to wait for 1 register to be ready before executing Upper instruction
void VU::PipelineWaitFMAC1 ( u32 UpperInstruction, u32 Register0 )
{
	u32 xyzw32;
	Bitmap128 WaitForBitmap;
	
	// get the xyzw
	xyzw32 = ( UpperInstruction >> 21 ) & 0xf;
	
	// create bitmap
	CreateBitmap ( WaitForBitmap, xyzw32, Register0 );
	
	PipelineWaitFMAC ( WaitForBitmap );
}

// force pipeline to wait for 2 registers to be ready before executing Upper instruction
void VU::PipelineWaitFMAC2 ( u32 UpperInstruction, u32 Register0, u32 Register1 )
{
	u32 xyzw32;
	Bitmap128 WaitForBitmap;
	
	// get the xyzw
	xyzw32 = ( UpperInstruction >> 21 ) & 0xf;
	
	// create bitmap
	CreateBitmap ( WaitForBitmap, xyzw32, Register0 );
	AddBitmap ( WaitForBitmap, xyzw32, Register1 );
	
	PipelineWaitFMAC ( WaitForBitmap );
}
*/


void VU::PipelineWaitCycle ( u64 WaitUntil_Cycle )
{
	// FMAC pipeline isn't more than 4 cycles long, so this guards against an infinite loop
	static const u32 c_CycleTimeout = 3;
	
	u32 Count;
	u64 Diff;
	
	Diff = WaitUntil_Cycle - CycleCount;
	
	if ( Diff > c_CycleTimeout )
	{
		Diff = c_CycleTimeout;
	}
	
	
	/*
	// check if the P register was set in the meantime too
	if ( CycleCount >= ( PBusyUntil_Cycle - 1 ) )
	{
		SetP ();
	}
	
	// first check that the current cycle is not past the busy until cycle
	if ( CycleCount >= WaitUntil_Cycle )
	{
		// no need to wait
		return;
	}
	*/
	
	// exaust the fmac pipeline for a maximum of 4 cycles
	//for ( Count = 0; Count < c_CycleTimeout; Count++ )
	for ( Count = 0; Count < Diff; Count++ )
	{
		AdvanceCycle ();
		
		// check if we are at the correct cycle yet
		//if ( CycleCount >= WaitUntil_Cycle ) return;
	}
	
	// at this point can just set the cycle# to the correct value
	// since there is nothing else to wait for
	CycleCount = WaitUntil_Cycle;
	
	/*
	// check if the Q register was set in the meantime too
	if ( CycleCount >= QBusyUntil_Cycle )
	{
		SetQ ();
	}
	
	// check if the P register was set in the meantime too
	if ( CycleCount >= ( PBusyUntil_Cycle - 1 ) )
	{
		SetP ();
	}
	*/
}

// force pipeline to wait for the Q register
void VU::PipelineWaitQ ()
{
	PipelineWaitCycle ( QBusyUntil_Cycle );
	
	// done waiting for Q register
	//QBusyUntil_Cycle = 0LL;
	
	if ( QBusyUntil_Cycle != -1LL )
	{
		SetQ ();
	}
}

// force pipeline to wait for the P register
void VU::PipelineWaitP ()
{
	// note: must wait only until one cycle before the P register is supposed to be free
	// because "PBusyUntil_Cycle" actually specifies the cycle the P register should be updated for "MFP" instruction
	// any stalls actually only stall until one cycle before that
	PipelineWaitCycle ( PBusyUntil_Cycle );
	//PipelineWaitCycle ( PBusyUntil_Cycle - 1 );
	
	// done waiting for P register
	//PBusyUntil_Cycle = 0LL;
	
	SetP ();
}


// whenever the pipeline advances, there's a bunch of stuff that must be updated
// or at least theoretically
// this will advance to the next cycle in VU
void VU::AdvanceCycle ()
{
	// testing
	//Check_QReg();

	// update the flags here for now
	// add the flags before updating the cycle count
	UpdateFlags();

	// this counts the bus cycles, not R5900 cycles
	CycleCount++;

	

#ifdef INLINE_DEBUG_ADVANCE_CYCLE
	debug << "\r\nhps2x64: VU#" << dec << Number << ": ADVANCE_CYCLE: P0=" << hex << Pipeline_Bitmap.b0 << " P1=" << Pipeline_Bitmap.b1 << " S0=" << SrcRegs_Bitmap.b0 << " S1=" << SrcRegs_Bitmap.b1 << " PC=" << PC << " Cycle#" << dec << CycleCount << "\r\n";
	debug << " fi=" << (( iFlagSave_Index - 4 ) & c_lFlag_Delay_Mask);
	debug << " fb0= " << hex << FlagSave [ 0 ].Bitmap.b0 << " " << FlagSave [ 0 ].Bitmap.b1;
	debug << " fb1= " << FlagSave [ 1 ].Bitmap.b0 << " " << FlagSave [ 1 ].Bitmap.b1;
	debug << " fb2= " << FlagSave [ 2 ].Bitmap.b0 << " " << FlagSave [ 2 ].Bitmap.b1;
	debug << " fb3= " << FlagSave [ 3 ].Bitmap.b0 << " " << FlagSave [ 3 ].Bitmap.b1;
#endif

}

// can modify this later to include the R5900 cycle number, to keep more in sync if needed in macro mode
void VU::MacroMode_AdvanceCycle ( u32 Instruction )
{
	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	vi [ 0 ].u = 0;
	
	// f0 always 0, 0, 0, 1 ???
	vf [ 0 ].uLo = 0;
	vf [ 0 ].uHi = 0;
	vf [ 0 ].uw = 0x3f800000;

	/*
	// process load delay slot
	if ( Status.EnableLoadMoveDelaySlot )
	{
		// need to make sure delay slot does not get cancelled in macro mode
		//FlagSave [ iFlagSave_Index & VU::c_lFlag_Delay_Mask ].Int_Bitmap = 0;
		
		// this clears the EnableLoadMoveDelaySlot, so no need to do it here
		Instruction::Execute::Execute_LoadDelaySlot_MacroMode ( this, (Vu::Instruction::Format&) Instruction );
	}

#ifdef ENABLE_INTDELAYSLOT	
	// go ahead and set the integer delay slot result
	//Execute_IntDelaySlot ();
	if ( Status.IntDelayValid )
	{
		vi [ IntDelayReg ].u = IntDelayValue;
		//Status.IntDelayValid = 0;
	}
#endif
	*/

	// clear the status flag here for macro mode (no longer needed)
	//Status.Value = 0;

	//SetCurrentFlags ();
	
}


void VU::Flush_XgKick ()
{
	u64 ullDrawCycle;

	if ( GPU::ulThreadedGPU )
	{
		ullDrawCycle = ( CycleCount < ullMaxCycle ) ? CycleCount : ullMaxCycle;

		GPU::_GPU->OutputTo_P1Buffer ( VuMem64, XgKick_Address & 0x3ff, ullDrawCycle );
	}
	else
	{
		GPU::Path1_WriteBlock ( VuMem64, XgKick_Address & 0x3ff );
	}
}


void VU::Execute_XgKick ()
{
	u64 ullDrawCycle;
	u32 ulCount;
	u32 bDone;

	/*
	if ( GPU::ulThreadedGPU )
	{

#ifdef ENABLE_XGKICK_TIMING

		if ( bXgKickActive )
		{
			if ( CycleCount > ullXgKickCycle )
			{
				ullDrawCycle = ( CycleCount < ullMaxCycle ) ? CycleCount : ullMaxCycle;

				// get amount of path1 data to transfer
				ulCount = CycleCount - ullXgKickCycle;

				bDone = GPU::_GPU->OutputTo_P1Buffer2 ( VuMem64, XgKick_Address & 0x3ff, ullDrawCycle, ulCount );

				// update xgkick cycle
				ullXgKickCycle = CycleCount;

				// update address
				XgKick_Address += ulCount;

				// check if done
				if ( bDone )
				{
					// stop transferring when done
					bXgKickActive = 0;
					ullXgKickCycle = -1ull;
				}
			}
		}
		
#else

		ullDrawCycle = ( CycleCount < ullMaxCycle ) ? CycleCount : ullMaxCycle;

		GPU::_GPU->OutputTo_P1Buffer ( VuMem64, XgKick_Address & 0x3ff, ullDrawCycle );

#endif
	}
	else
	*/
	{

#ifdef ENABLE_XGKICK_TIMING

		if ( bXgKickActive )
		{
			if ( CycleCount > ullXgKickCycle )
			{
				// get amount of path1 data to transfer
				ulCount = CycleCount - ullXgKickCycle;

				// need to transfer only a certain amount of data here
				GPU::Path1_WriteBlock ( VuMem64, XgKick_Address & 0x3ff, ulCount );

				// update xgkick cycle
				ullXgKickCycle = CycleCount;

				// update address
				XgKick_Address += ulCount;

				// check if done
				if ( GPU::_GPU->EndOfPacket [ 1 ] )
				{
					// stop transferring when done
					bXgKickActive = 0;
					ullXgKickCycle = -1ull;
				}
			}
		}

#else
		// clear the path 1 count before writing block
		GPU::_GPU->ulTransferCount [ 1 ] = 0;

		// looks like this is only supposed to write one 128-bit value to PATH1
		// no, this actually writes an entire gif packet to path1
		// the address should only be a maximum of 10-bits, so must mask
		//GPU::Path1_WriteBlock ( & VuMem64 [ ( XgKick_Address & 0x3ff ) << 1 ] );
		GPU::Path1_WriteBlock ( VuMem64, XgKick_Address & 0x3ff );
#endif
		
		// for now, path 1 is done
		/*
		if ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
		{
			GPU::_GPU->GIFRegs.STAT.APATH = 0;

			if ( VifStopped )
			{
				// also, need to re-enable the vif when done in path 1 in case it was stopped
//debug << "\r\n***VIF STOPPED - WANT TO RESTART***\r\n";
				VifStopped = 0;

				// condition for dma changed, so check dma
				Dma::_DMA->CheckTransfer ();
			}
		}
		*/
	}
}

void VU::iStartVU ( u64 ullCycleNumber )
{
	iRunning = 1;
	ullStartCycle = ullCycleNumber;
	CycleCount = ullStartCycle + 1;

	// max cycle# for drawing objects from this run of VU
	ullMaxCycle = ullStartCycle + c_ullMultiThread_Cycles;

	// VUx is not stopped anymore
	//VU0::_VU0->vi [ 29 ].uLo &= ~( 0xe << ( Number << 3 ) );
	
	// set VBSx in VPU STAT to 1 (running)
	//VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( Number << 3 ) );
	
	// also set VIFx STAT to indicate program is running
	//VifRegs.STAT.VEW = 1;
	
	//SetNextEvent_Cycle ( CycleCount );
}

void VU::iStopVU ()
{
	iRunning = 0;

	// but might want to check cycle# it stopped at
	//CycleCount = -1LL;

	// the vif on the other thread can try to proceed now
	iVifStopped = 0;

	//VifRegs.STAT.VEW = 0;
	//VU0::_VU0->vi [ 29 ].uLo &= ~( 1 << ( Number << 3 ) );
}


void VU::StartVU ()
{
	Running = 1;

	// VUx is not stopped anymore
	VU0::_VU0->vi [ 29 ].uLo &= ~( 0xe << ( Number << 3 ) );
	
	// set VBSx in VPU STAT to 1 (running)
	VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( Number << 3 ) );
	
	// also set VIFx STAT to indicate program is running
	VifRegs.STAT.VEW = 1;

	// set the externally viewable cycle#
 	eCycleCount = *_DebugCycleCount + 1;
	
	// if not multi-threading the vu, then set the internal variables
	if ( !Number || !ulThreadCount )
	{
		// running vu on the same thread //

		// set the cycle# vu is starting at for watchdog
		//ullStartCycle = *_DebugCycleCount;
		ullStartCycle = *_DebugCycleCount;

		// vu is running on same thread, so set Cycle# directly
		CycleCount = eCycleCount;

		// no max cycle if running vu on the main thread
		ullMaxCycle = -1ull;
	}
	else
	{
		// running vu on another thread at another time //

		// pretend to run vu#1 for x number of cycles
		eCycleCount += c_ullMultiThread_Cycles;
		//eCycleCount = R5900::Cpu::_CPU->CycleCount + c_ullMultiThread_Cycles;
	}

	//SetNextEvent_Cycle ( CycleCount );
	SetNextEvent_Cycle ( eCycleCount );
}

void VU::StopVU ()
{
	Running = 0;
	VifRegs.STAT.VEW = 0;
	VU0::_VU0->vi [ 29 ].uLo &= ~( 1 << ( Number << 3 ) );

	// vif can try to proceed now
	VifStopped = 0;

	// vif no longer waiting for end of program
	VifWaiting = 0;

	// VifStopped is a condition used for some transfers, so have dma check transfers
	Dma::_DMA->CheckTransfer ();

	// vu no longer running
	// but might want to check cycle# it stopped at
	//eCycleCount = -1LL;

	// if not multi-threading the vu, then set the internal variables
	if ( !Number || !ulThreadCount )
	{
		// vu is running on same thread, so set Cycle# directly
		// but might want to check cycle# it stopped at
		//CycleCount = -1LL;
	}
}





void VU::Start_Frame ()
{
	//cout << "\nVU::Start_Frame";

	/*
	//if ( VU1::_VU1->bMultiThreadingEnabled )
	//{
		//cout << "\nVU1::_VU1->bMultiThreadingEnabled";
		
		if ( ulThreadCount )
		{
			//cout << "\nVU1::_VU1->ulThreadCount";

			// start from beginning of buffer
			ullCommBuf_ReadIdx = 0;
			ullCommBuf_WriteIdx = 0;
			ullCommBuf_TargetIdx2 = 0;

			// clear gpu mt vars
			GPU::ulTBufferIndex = 0;
			GPU::ulP1BufferIndex = 0;

			GPU::ulInputBuffer_WriteIndex = 0;
			GPU::ulInputBuffer_TargetIndex = 0;
			GPU::ulInputBuffer_TargetIndex2 = 0;
			GPU::ulInputBuffer_ReadIndex = 0;

			GPU::ullP1Buf_WriteIndex = 0;
			GPU::ullP1Buf_TargetIndex = 0;
			GPU::ullP1Buf_TargetIndex2 = 0;
			GPU::ullP1Buf_ReadIndex = 0;


			// make sure to copy over offset,top,tops,itop,itops,base,strow,stcol,mode,mask,cycle
			VU1::_VU1->iVifCode = VU1::_VU1->VifCode;
			VU1::_VU1->iVifCodeState = VU1::_VU1->lVifCodeState;
			VU1::_VU1->iVifRegs.CYCLE.Value = VU1::_VU1->VifRegs.CYCLE.Value;
			VU1::_VU1->iVifRegs.MODE = VU1::_VU1->VifRegs.MODE;
			VU1::_VU1->iVifRegs.MASK = VU1::_VU1->VifRegs.MASK;
			VU1::_VU1->iVifRegs.BASE = VU1::_VU1->VifRegs.BASE;
			VU1::_VU1->iVifRegs.TOP = VU1::_VU1->VifRegs.TOP;
			VU1::_VU1->iVifRegs.TOPS = VU1::_VU1->VifRegs.TOPS;
			VU1::_VU1->iVifRegs.ITOP = VU1::_VU1->VifRegs.ITOP;
			VU1::_VU1->iVifRegs.ITOPS = VU1::_VU1->VifRegs.ITOPS;
			VU1::_VU1->iVifRegs.STAT.DBF = VU1::_VU1->VifRegs.STAT.DBF;

			VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 0 ] = VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 0 ];
			VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 1 ] = VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 1 ];
			VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 2 ] = VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 2 ];
			VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 3 ] = VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 3 ];

			VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 0 ] = VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 0 ];
			VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 1 ] = VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 1 ];
			VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 2 ] = VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 2 ];
			VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 3 ] = VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 3 ];

			// don't kill the vu thread
			VU::ulKillVuThread = 0;
			
			// data not ready yet from thread
			VU1::_VU1->bMultiThreadVUDataReady = 0;
			
			// create event for triggering gpu thread UPDATE
			ghEvent_PS2VU1_Update = CreateEvent(
				NULL,			// default security
				false,			// auto-reset event (true for manually reset event)
				false,			// initially not set
				NULL			// name of event object
			);
			
			if ( !ghEvent_PS2VU1_Update )
			{
				cout << "\nERROR: Unable to create PS2 VU1 START event. " << GetLastError ();
			}
			
			ghEvent_PS2VU1_Running = CreateEvent(
				NULL,			// default security
				false,			// auto-reset event (true for manually reset event)
				false,			// initially not set
				NULL			// name of event object
			);
			
			if ( !ghEvent_PS2VU1_Running )
			{
				cout << "\nERROR: Unable to create PS2 VU1 RUNNING event. " << GetLastError ();
			}

			ghEvent_PS2VU1_Finish = CreateEvent(
				NULL,			// default security
				true,			// auto-reset event (true for manually reset event)
				false,			// initially not set
				NULL			// name of event object
			);
			
			if ( !ghEvent_PS2VU1_Finish )
			{
				cout << "\nERROR: Unable to create PS2 VU1 RUNNING event. " << GetLastError ();
			}

			ulThreadCount_Created = 0;
			for ( int i = 0; i < ulThreadCount; i++ )
			{
				//cout << "\nCreating VU thread#" << dec << i;
				
				// create thread
				VUThreads [ i ] = new Api::Thread( Start_VUThread, (void*) NULL, false );
				
				ulThreadCount_Created++;
				
				//cout << "\nCreated VU thread#" << dec << i << " ThreadStarted=" << VUThreads[ i ]->ThreadStarted;
				//cout << "\nCREATING: NEW VU THREAD: " << dec << ulThreadCount_Created;
			}

		// wait for thread to be ready
		WaitForSingleObject ( ghEvent_PS2VU1_Finish, INFINITE );

		//}
	}
	*/
}

void VU::End_Frame ()
{
#ifdef ENABLE_VU_MULTI_THREAD
	int iRet;
	
	if ( ulThreadCount_Created )
	{
		// wait for vu thread to finish
		Finish ();


		// kill vu thread
		//x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) VU::ulKillVuThread, 1 );
		x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) ullCommBuf_TargetIdx2, -1ull );
		SetEvent ( ghEvent_PS2VU1_Update );
		
		
		for ( int i = 0; i < ulThreadCount_Created; i++ )
		{
			//cout << "\nKilling GPU thread#" << dec << i << " ThreadStarted=" << GPUThreads[ i ]->ThreadStarted;
			
			// create thread
			iRet = VUThreads [ i ]->Join();
			
			//cout << "\nThreadStarted=" << GPUThreads[ i ]->ThreadStarted;
			
			if ( iRet )
			{
				cout << "\nhps2x64: VU: ALERT: Problem with completion of VU thread#" << dec << i << " iRet=" << iRet;
			}
			
			delete VUThreads [ i ];
			
			//cout << "\nKilled VU thread#" << dec << i << " iRet=" << iRet;
		}

		// no more vu threads now
		ulThreadCount_Created = 0;
		
		// close event handles
		CloseHandle ( ghEvent_PS2VU1_Update );
		CloseHandle ( ghEvent_PS2VU1_Running );
		CloseHandle ( ghEvent_PS2VU1_Finish );

		// after a full sync, need to copy the variables back
		VU1::_VU1->VifCode = VU1::_VU1->iVifCode;
		VU1::_VU1->lVifCodeState = VU1::_VU1->iVifCodeState;
		VU1::_VU1->VifRegs.CYCLE.Value = VU1::_VU1->iVifRegs.CYCLE.Value;
		VU1::_VU1->VifRegs.MODE = VU1::_VU1->iVifRegs.MODE;
		VU1::_VU1->VifRegs.MASK = VU1::_VU1->iVifRegs.MASK;
		VU1::_VU1->VifRegs.BASE = VU1::_VU1->iVifRegs.BASE;
		VU1::_VU1->VifRegs.TOP = VU1::_VU1->iVifRegs.TOP;
		VU1::_VU1->VifRegs.TOPS = VU1::_VU1->iVifRegs.TOPS;
		VU1::_VU1->VifRegs.ITOP = VU1::_VU1->iVifRegs.ITOP;
		VU1::_VU1->VifRegs.ITOPS = VU1::_VU1->iVifRegs.ITOPS;
		VU1::_VU1->VifRegs.STAT.DBF = VU1::_VU1->iVifRegs.STAT.DBF;

		VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 0 ] = VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 0 ];
		VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 1 ] = VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 1 ];
		VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 2 ] = VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 2 ];
		VU1::_VU1->VifRegs.Regs [ c_iRowRegStartIdx + 3 ] = VU1::_VU1->iVifRegs.Regs [ c_iRowRegStartIdx + 3 ];

		VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 0 ] = VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 0 ];
		VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 1 ] = VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 1 ];
		VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 2 ] = VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 2 ];
		VU1::_VU1->VifRegs.Regs [ c_iColRegStartIdx + 3 ] = VU1::_VU1->iVifRegs.Regs [ c_iColRegStartIdx + 3 ];
	}

#endif
}


// run from main thread
void VU1::sRun ()
{
#ifdef ALLOW_VU1_MULTITHREADING
	if ( _VU1->ulThreadCount && _VU1->bCurrentVuRunInCmdBuffer )
	{
		// the REAL vu run is actually on another thread somewhere //

#ifdef INLINE_DEBUG_VUEXECUTE
					Vu::Instruction::Execute::debug << "\r\n*** LAST CALL END";
					Vu::Instruction::Execute::debug << " VU#" << _VU1->Number;
					Vu::Instruction::Execute::debug << " Cycle#" << hex << *VU::_DebugCycleCount;
					Vu::Instruction::Execute::debug << " ***";
#endif


		// stop the vu
		_VU1->StopVU ();

		// don't need to know cycle# it stopped at, just need to stop the events
		// if this is -1, then in macro mode will think it is time to update q
		_VU1->eCycleCount = -2ull;	// -1ull;
	}
	else
	{
		// vu is running normally on the same thread //

		_VU1->Run ();
	}
#else
	_VU1->Run ();
#endif
}


// run from another thread
void VU1::sRun2 ()
{
	/*
	// only run if multi-threading is active
	// when running from main thread
	if ( _VU1->bMultiThreadingEnabled )
	{
		//cout << "\nVUMT: ENABLED";
		
		// check for signal to start running multi-threaded VU
		if ( x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) _VU1->bMultiThreadVUDataStart, 0 ) )
		{
			//cout << "\nVUMT: START";
			
			_VU1->ullMultiThread_NextCycle = _VU1->CycleCount;
			
#ifdef VERBOSE_VU_THREAD
			cout << "\nSTART: VU THREAD" << " VUCycle#" << dec << _VU1->CycleCount << " SystemCycle#" << ( *VU::_DebugCycleCount ) << " RDY=" << _VU1->bMultiThreadVUDataReady << " ACTIVE=" << _VU1->bMultiThreadingActive;
#endif
			
			while ( _VU1->bMultiThreadingActive )
			{
				_VU1->ullMultiThread_NextCycle += VU::c_ullMultiThread_NextCycles;
				
				do
				{
					_VU1->Run ();
					
				// run until it's time to synchronize or it's done running
				} while ( ( _VU1->CycleCount < _VU1->ullMultiThread_NextCycle ) && ( _VU1->bMultiThreadingActive ) );
				
				
				if ( ! _VU1->bMultiThreadingActive )
				{
#ifdef VERBOSE_VU_THREAD
					cout << "\nDATA READY: VU THREAD";
#endif
					
					// flag when done
					x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) _VU1->bMultiThreadVUDataReady, 1 );
				}
				else
				{
					// *** todo *** lock syncrhonize the cycle count with main thread
					x64ThreadSafe::Utilities::Lock_ExchangeAdd64 ( (long long&) _VU1->CycleCount, 0 );
				}
				
#ifdef VERBOSE_VU_THREAD
				cout << "\nSIGNAL: VU THREAD" << " VUCycle#" << dec << _VU1->CycleCount << " SystemCycle#" << ( *VU::_DebugCycleCount ) << " RDY=" << _VU1->bMultiThreadVUDataReady << " ACTIVE=" << _VU1->bMultiThreadingActive;
#endif
				
#ifdef ENABLE_PASSIVE_WAIT
				
				// if main thread is waiting, then wake it up
				SetEvent ( ghEvent_PS2VU1_Running );
#endif
			}
			
			
		}
	}
	*/

}


/*
int VU::Start_VUThread( void* Param )
{
	
//cout << "\nSTARTING: VU THREAD";

	volatile u64 ullVUTargetIdx;
	volatile u64 ullGPUTargetIdx;

	volatile u64 ullGPUReadIdx = 0;

	GPU::ulTBufferIndex = 0;
	GPU::ulP1BufferIndex = 0;

	//ullVUTargetIdx = ullCommBuf_TargetIdx2;
	//ullGPUTargetIdx = GPU::ulInputBuffer_TargetIndex2;
	//ullGPUReadIdx = GPU::ulInputBuffer_ReadIndex;

	while ( 1 )
	{
		
		//cout << "\nWaiting for next VU1 event.";

		ullVUTargetIdx = Lock_ExchangeAdd64( (long long&) VU::ullCommBuf_TargetIdx2, 0 );
		ullGPUTargetIdx = Lock_ExchangeAdd64( (long long&) GPU::ulInputBuffer_TargetIndex2, 0 );

		while ( ( ullVUTargetIdx <= ullCommBuf_ReadIdx )
				&& ( ullGPUTargetIdx <= ullGPUReadIdx ) )
		{
			if ( !SetEvent ( ghEvent_PS2VU1_Finish ) )
			{
				cout << "\nUnable to set VU finish event. " << GetLastError ();
			}

			if ( GPU::ulThreadedGPU && !GPU::ulNumberOfThreads )
			{
				if ( !SetEvent ( GPU::ghEvent_PS2GPU_Finish ) )
				{
					cout << "\nUnable to set GPU finish event. " << GetLastError ();
				}
			}

			WaitForSingleObject( ghEvent_PS2VU1_Update, INFINITE );

			//cout << "\nVU1 Thread triggered while waiting.";

			ullVUTargetIdx = Lock_ExchangeAdd64( (long long&) VU::ullCommBuf_TargetIdx2, 0 );
			ullGPUTargetIdx = Lock_ExchangeAdd64( (long long&) GPU::ulInputBuffer_TargetIndex2, 0 );
		}

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nSTART->VU-THREAD-LOOP";
#endif

		//cout << "\nVU1 Thread triggered.";
		
		if ( !ResetEvent ( ghEvent_PS2VU1_Finish ) )
		{
			cout << "\nUnable to reset VU finish event. " << GetLastError ();
		}

		if ( GPU::ulThreadedGPU && !GPU::ulNumberOfThreads )
		{
			if ( !ResetEvent ( GPU::ghEvent_PS2GPU_Finish ) )
			{
				cout << "\nUnable to rese GPU finish event. " << GetLastError ();
			}
		}

		if ( ullVUTargetIdx == -1ull )
		{
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nVU-THREAD-KILL";
#endif

			// signal to kill thread
			break;
		}

		//cout << "\nullVUTargetIdx=" << dec << ullVUTargetIdx << " VUReadIdx=" << ullCommBuf_ReadIdx << " Target2=" << VU::ullCommBuf_TargetIdx2;
		//cout << " ulInputBuffer_TargetIndex=" << dec << GPU::_GPU->ulInputBuffer_TargetIndex2;

		if ( ullVUTargetIdx > ullCommBuf_ReadIdx )
		{
#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nCALLING->Execute_CommandBuffer";
#endif
		//cout << "\nExecuting VU command buffer. targetidx=" << dec << ullVUTargetIdx << " readidx=" << ullCommBuf_ReadIdx;

			VU1::_VU1->Execute_CommandBuffer ( ullVUTargetIdx );

#ifdef INLINE_DEBUG_VUCOM_MT
	debug << "\r\nDONE->Execute_CommandBuffer";
#endif
		}

		//cout << "\nCommand buffer executed.";

//#ifdef INLINE_DEBUG_VUCOM_MT
//	debug << "\r\nSEND-TO-GPU VUREADID=" << dec << ullCommBuf_ReadIdx << " VUTARGETID=" << ullVUTargetIdx;
//	debug << " GPUREADID="
//#endif


		GPU::_GPU->ullP1Buf_TargetIndex = GPU::_GPU->ullP1Buf_WriteIndex;
		GPU::_GPU->ulInputBuffer_TargetIndex = ullGPUTargetIdx;
		Lock_Exchange64( (long long&) ullCommBuf_ReadIdx, ullCommBuf_ReadIdx );


		if ( GPU::ulNumberOfThreads )
		{
			//cout << "Triggerring GPU thread. gputargetidx=" << dec << GPU::_GPU->ulInputBuffer_TargetIndex << " gpureadidx=" << GPU::_GPU->ulInputBuffer_ReadIndex;

			// trigger gpu thread with the data
			if ( !SetEvent ( GPU::ghEvent_PS2GPU_Update ) )
			{
				cout << "\nUnable to set gpu update event from vu event. " << GetLastError ();
			}
			
		}
		else
		{
			GPU::Process_GPUCommandsMT ( ullGPUTargetIdx, GPU::_GPU->ullP1Buf_WriteIndex );
		}


		// data was sent
		ullGPUReadIdx = ullGPUTargetIdx;
	}

	return 0;
}
*/



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VU::DebugWindow_Enable ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static constexpr const char* COP0_Names [] = { "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "Reserved",
								"BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause", "EPC", "PRId",
								"Config", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "BadPAddr",
								"Debug", "Perf", "Reserved", "Reserved", "TagLo", "TagHi", "ErrorEPC", "Reserved" };
								
	static constexpr const char* DisAsm_Window_ColumnHeadings [] = { "Address", "@", ">", "Instruction", "Inst (hex)" };
								
	static constexpr const char* FontName = "Courier New";
	static constexpr int FontSize = 6;
	
	static constexpr const char* DebugWindow_Caption = "VU Debug Window";
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
	
	if ( !DebugWindow_Enabled [ Number ] )
	{
		// create the main debug window
		DebugWindow [ Number ] = new WindowClass::Window ();
		DebugWindow [ Number ]->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow [ Number ]->Set_Font ( DebugWindow [ Number ]->CreateFontObject ( FontSize, FontName ) );
		DebugWindow [ Number ]->DisableCloseButton ();
		
		// create "value lists"
		GPR_ValueList [ Number ] = new DebugValueList<float> ();
		COP0_ValueList [ Number ] = new DebugValueList<u32> ();
		COP2_CPCValueList [ Number ] = new DebugValueList<u32> ();
		COP2_CPRValueList [ Number ] = new DebugValueList<u32> ();
		
		// create the value lists
		GPR_ValueList [ Number ]->Create ( DebugWindow [ Number ], GPRList_X, GPRList_Y, GPRList_Width, GPRList_Height );
		COP0_ValueList [ Number ]->Create ( DebugWindow [ Number ], COP1List_X, COP1List_Y, COP1List_Width, COP1List_Height );
		COP2_CPCValueList [ Number ]->Create ( DebugWindow [ Number ], COP2_CPCList_X, COP2_CPCList_Y, COP2_CPCList_Width, COP2_CPCList_Height );
		COP2_CPRValueList [ Number ]->Create ( DebugWindow [ Number ], COP2_CPRList_X, COP2_CPRList_Y, COP2_CPRList_Width, COP2_CPRList_Height );
		
		GPR_ValueList [ Number ]->EnableVariableEdits ();
		COP0_ValueList [ Number ]->EnableVariableEdits ();
		COP2_CPCValueList [ Number ]->EnableVariableEdits ();
		COP2_CPRValueList [ Number ]->EnableVariableEdits ();
	
		// add variables into lists
		for ( i = 0; i < 32; i++ )
		{
			ss.str ("");
			ss << "vf" << dec << i << "_x";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw0) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fx) );
			ss.str ("");
			ss << "vf" << dec << i << "_y";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw1) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fy) );
			ss.str ("");
			ss << "vf" << dec << i << "_z";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw2) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fz) );
			ss.str ("");
			ss << "vf" << dec << i << "_w";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw3) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fw) );
			
			//COP0_ValueList->AddVariable ( COP0_Names [ i ], &(_CPU->CPR0.Regs [ i ]) );

			if ( i < 16 )
			{
				ss.str("");
				ss << "vi" << dec << i;
				COP2_CPCValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vi [ i ].u) );
				
				//ss.str("");
				//ss << "VI_" << dec << ( ( i << 1 ) + 1 );
				//COP2_CPCValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ ( i << 1 ) + 1 ]) );
			}
			else
			{
				ss.str("");
				ss << "CPC2_" << dec << i;
				COP2_CPRValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vi [ i ].u) );
				
				//ss.str("");
				//ss << "CPR2_" << dec << ( ( ( i - 16 ) << 1 ) + 1 );
				//COP2_CPRValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( ( i - 16 ) << 1 ) + 1 ]) );
			}
		}
		
		
		// also add PC and CycleCount
		ss.str("");
		ss << "V" << Number << "PC";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->PC) );
		ss.str("");
		ss << "V" << Number << "NextPC";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->NextPC) );
		ss.str("");
		ss << "V" << Number << "LastPC";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->LastPC) );
		ss.str("");
		ss << "V" << Number << "CycleLO";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), (u32*) &(_VU [ Number ]->CycleCount) );
		
		ss.str("");
		ss << "V" << Number << "LastReadAddress";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->Last_ReadAddress) );
		ss.str("");
		ss << "V" << Number << "LastWriteAddress";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->Last_WriteAddress) );
		ss.str("");
		ss << "V" << Number << "LastReadWriteAddress";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->Last_ReadWriteAddress) );
		
		if ( Number )
		{
			// also add in the count for path2 transfers for debugging vu#1 direct/directhl etc
			COP0_ValueList [ Number ]->AddVariable ( "Path2Cnt", &(GPU::_GPU->ulTransferCount [ 2 ]) );
			COP0_ValueList [ Number ]->AddVariable ( "Path2Sze", &(GPU::_GPU->ulTransferSize [ 2 ]) );
		}
		
		// need to add in load delay slot values
		//GPR_ValueList->AddVariable ( "D0_INST", &(_VU [ Number ]->DelaySlot0.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D0_VAL", &(_VU [ Number ]->DelaySlot0.Data) );
		//GPR_ValueList->AddVariable ( "D1_INST", &(_VU [ Number ]->DelaySlot1.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D1_VAL", &(_VU [ Number ]->DelaySlot1.Data) );
		
		//GPR_ValueList->AddVariable ( "SPUCC", (u32*) _SpuCycleCount );
		//GPR_ValueList->AddVariable ( "Trace", &TraceValue );

		// add some things to the cop0 value list
		ss.str("");
		ss << "V" << Number << "RUN";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->Running) );
		ss.str("");
		ss << "V" << Number << "VifStop";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->VifStopped) );
		ss.str("");
		ss << "V" << Number << "VUSTAT";
		COP0_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->VifRegs.STAT.Value) );
		
		COP0_ValueList [ Number ]->AddVariable ( "GPUSTAT", &(GPU::_GPU->GIFRegs.STAT.Value) );

		// create the disassembly window
		DisAsm_Window [ Number ] = new Debug_DisassemblyViewer ( Breakpoints [ Number ] );
		DisAsm_Window [ Number ]->Create ( DebugWindow [ Number ], DisAsm_X, DisAsm_Y, DisAsm_Width, DisAsm_Height, (Debug_DisassemblyViewer::DisAsmFunction) Vu::Instruction::Print::PrintInstructionLO, (Debug_DisassemblyViewer::DisAsmFunction) Vu::Instruction::Print::PrintInstructionHI );
		
		DisAsm_Window [ Number ]->Add_MemoryDevice ( "MicroMem", _VU [ Number ]->ulMicroMem_Start, _VU [ Number ]->ulMicroMem_Size, (u8*) _VU [ Number ]->MicroMem8 );
		
		DisAsm_Window [ Number ]->SetProgramCounter ( &_VU [ Number ]->PC );
		
		
		// create window area for breakpoints
		Breakpoint_Window [ Number ] = new Debug_BreakpointWindow ( Breakpoints [ Number ] );
		Breakpoint_Window [ Number ]->Create ( DebugWindow [ Number ], BkptViewer_X, BkptViewer_Y, BkptViewer_Width, BkptViewer_Height );
		
		// create the viewer for D-Cache scratch pad
		ScratchPad_Viewer [ Number ] = new Debug_MemoryViewer ();
		
		ScratchPad_Viewer [ Number ]->Create ( DebugWindow [ Number ], MemoryViewer_X, MemoryViewer_Y, MemoryViewer_Width, MemoryViewer_Height, MemoryViewer_Columns );
		ScratchPad_Viewer [ Number ]->Add_MemoryDevice ( "VuMem", _VU [ Number ]->ulVuMem_Start, _VU [ Number ]->ulVuMem_Size, (u8*) _VU [ Number ]->VuMem8 );
		
		cout << "\nDebug Enable";
		
		// mark debug as enabled now
		DebugWindow_Enabled [ Number ] = true;
		
		cout << "\nUpdate Start";
		
		// update the value lists
		DebugWindow_Update ( Number );
	}
	
#endif

}

void VU::DebugWindow_Disable ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled [ Number ] )
	{
		delete DebugWindow [ Number ];
		delete GPR_ValueList [ Number ];
		delete COP0_ValueList [ Number ];
		delete COP2_CPCValueList [ Number ];
		delete COP2_CPRValueList [ Number ];

		delete DisAsm_Window [ Number ];
		
		delete Breakpoint_Window [ Number ];
		delete ScratchPad_Viewer [ Number ];
		
	
		// disable debug window
		DebugWindow_Enabled [ Number ] = false;
	}
	
#endif

}

void VU::DebugWindow_Update ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled [ Number ] )
	{
		GPR_ValueList [ Number ]->Update();
		COP0_ValueList [ Number ]->Update();
		COP2_CPCValueList [ Number ]->Update();
		COP2_CPRValueList [ Number ]->Update();
		DisAsm_Window [ Number ]->GoTo_Address ( _VU [ Number ]->PC );
		DisAsm_Window [ Number ]->Update ();
		Breakpoint_Window [ Number ]->Update ();
		ScratchPad_Viewer [ Number ]->Update ();
	}
	
#endif

}



void VU::Flush ()
{
//cout << "\nVU::Flush";

#ifdef ENABLE_VU_MULTI_THREAD

	if ( ulThreadCount_Created )
	{

		if ( GPU::ulThreadedGPU )
		{
			if ( !SetEvent ( GPU::ghEvent_PS2GPU_Update ) )
			{
				cout << "\nUnable to set PS2 GPU UPDATE event. " << GetLastError ();
			}
		}

//cout << " ulThreadCount_Created ullCommBuf_WriteIdx=" << ullCommBuf_WriteIdx << " ulInputBuffer_WriteIndex=" << GPU::ulInputBuffer_WriteIndex;
		if ( ( ullCommBuf_TargetIdx2 != ullCommBuf_WriteIdx )
			|| ( GPU::ulInputBuffer_TargetIndex2 != GPU::ulInputBuffer_WriteIndex )
		)
		{
//cout << " Triggering VU1 Event";
			// send gpu buffer data with vu buffer data
			GPU::ulInputBuffer_TargetIndex2 = GPU::ulInputBuffer_WriteIndex;
			x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) ullCommBuf_TargetIdx2, ullCommBuf_WriteIdx );
			
		//cout << "\nSending ullCommBuf_WriteIdx=" << ullCommBuf_WriteIdx;
			// send the command to the other thread
			//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) ulInputBuffer_TargetIndex, ulInputBuffer_WriteIndex );
			//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) _GPU->ullP1Buf_TargetIndex, _GPU->ullP1Buf_WriteIndex );
			
			// trigger event
			if ( !SetEvent ( ghEvent_PS2VU1_Update ) )
			{
				cout << "\nUnable to set PS2 VU1 UPDATE event. " << GetLastError ();
			}


			//while ( ( ulInputBuffer_WriteIndex - x64ThreadSafe::Utilities::Lock_ExchangeAdd64 ( (long long&) ulInputBuffer_ReadIndex, 0 ) ) > ( c_ulInputBuffer_Size - c_ulRequiredBuffer ) );
//cout << "\nSENDING: " << dec << ulInputBuffer_WriteIndex;
		}
	}

#endif


}

void VU::Finish ()
{
#ifdef ENABLE_VU_MULTI_THREAD

	if ( ulThreadCount_Created )
	{
		if ( GPU::ulThreadedGPU )
		{
			if ( !SetEvent ( GPU::ghEvent_PS2GPU_Update ) )
			{
				cout << "\nUnable to set PS2 GPU UPDATE event. " << GetLastError ();
			}
		}

		//if ( ( ullCommBuf_WriteIdx != ullCommBuf_ReadIdx )
		//	|| ( GPU::ulInputBuffer_WriteIndex != GPU::ulInputBuffer_ReadIndex )
		//)
		//{
			
			if ( ( ullCommBuf_TargetIdx2 != ullCommBuf_WriteIdx )
				|| ( GPU::ulInputBuffer_TargetIndex2 != GPU::ulInputBuffer_WriteIndex )
			)
			{
				// send gpu buffer data with vu buffer data
				GPU::ulInputBuffer_TargetIndex2 = GPU::ulInputBuffer_WriteIndex;
				x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) ullCommBuf_TargetIdx2, ullCommBuf_WriteIdx );
				
				// send the command to the other thread
				//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) ulInputBuffer_TargetIndex, ulInputBuffer_WriteIndex );
				//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&) _GPU->ullP1Buf_TargetIndex, _GPU->ullP1Buf_WriteIndex );
				
				if ( !ResetEvent ( ghEvent_PS2VU1_Finish ) )
				{
					cout << "\nUnable to reset finish event before update. " << GetLastError ();
				}
				
				// trigger event
				if ( !SetEvent ( ghEvent_PS2VU1_Update ) )
				{
					cout << "\nUnable to set PS2 VU1 UPDATE event. " << GetLastError ();
				}
				
			}
			
			// wait for the other thread to complete
			//while ( ulInputBuffer_WriteIndex != x64ThreadSafe::Utilities::Lock_ExchangeAdd64 ( (long long&)ulInputBuffer_ReadIndex, 0 ) )
			//{
//#ifdef ENABLE_WAIT_DURING_SPIN_FINISH
				//MsgWaitForMultipleObjectsEx( NULL, NULL, 1, QS_ALLINPUT, MWMO_ALERTABLE );
				//SetEvent ( ghEvent_PS2GPU_Update );
				WaitForSingleObject ( ghEvent_PS2VU1_Finish, INFINITE );
//#endif

				//PAUSE ();
			//}
		//}

	}

#endif
}




