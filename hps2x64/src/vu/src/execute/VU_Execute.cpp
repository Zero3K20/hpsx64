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


// ***TODO***
// 1. if upper and lower instruction have same destination float reg, then determine which executes first or gets cancelled
// 2. recompiler is ok, but interpreter would have to first handle all upper and lower delays before setting wait for dst regs
// 3. need a static flag for when lower instruction sets the clip flag and upper instruction is CLIP
// 4. need a static flag for when lower instruction sets the stat flag in addition to upper instruction setting it


#include "VU_Execute.h"
#include "VU_Print.h"
#include "PS2Float.h"
#include "PS2_Gpu.h"

//#include <cmath>
#include <stdlib.h>

using namespace std;
using namespace Vu::Instruction;
using namespace PS2Float;


#ifdef _DEBUG_VERSION_
Debug::Log Execute::debug;
#endif


//#define ENABLE_TEST_DEBUG_FTOI4



//#define ENABLE_XGKICK_WAIT


#define USE_PACKED_INTRINSICS_MUL_VU
#define USE_PACKED_INTRINSICS_MULI_VU
#define USE_PACKED_INTRINSICS_MULQ_VU
#define USE_PACKED_INTRINSICS_MULX_VU
#define USE_PACKED_INTRINSICS_MULY_VU
#define USE_PACKED_INTRINSICS_MULZ_VU
#define USE_PACKED_INTRINSICS_MULW_VU

#define USE_PACKED_INTRINSICS_MULA_VU
#define USE_PACKED_INTRINSICS_MULAI_VU
#define USE_PACKED_INTRINSICS_MULAQ_VU
#define USE_PACKED_INTRINSICS_MULAX_VU
#define USE_PACKED_INTRINSICS_MULAY_VU
#define USE_PACKED_INTRINSICS_MULAZ_VU
#define USE_PACKED_INTRINSICS_MULAW_VU

#define USE_PACKED_INTRINSICS_ADD_VU
#define USE_PACKED_INTRINSICS_ADDI_VU
#define USE_PACKED_INTRINSICS_ADDQ_VU
#define USE_PACKED_INTRINSICS_ADDX_VU
#define USE_PACKED_INTRINSICS_ADDY_VU
#define USE_PACKED_INTRINSICS_ADDZ_VU
#define USE_PACKED_INTRINSICS_ADDW_VU

#define USE_PACKED_INTRINSICS_ADDA_VU
#define USE_PACKED_INTRINSICS_ADDAI_VU
#define USE_PACKED_INTRINSICS_ADDAQ_VU
#define USE_PACKED_INTRINSICS_ADDAX_VU
#define USE_PACKED_INTRINSICS_ADDAY_VU
#define USE_PACKED_INTRINSICS_ADDAZ_VU
#define USE_PACKED_INTRINSICS_ADDAW_VU

#define USE_PACKED_INTRINSICS_SUB_VU
#define USE_PACKED_INTRINSICS_SUBI_VU
#define USE_PACKED_INTRINSICS_SUBQ_VU
#define USE_PACKED_INTRINSICS_SUBX_VU
#define USE_PACKED_INTRINSICS_SUBY_VU
#define USE_PACKED_INTRINSICS_SUBZ_VU
#define USE_PACKED_INTRINSICS_SUBW_VU

#define USE_PACKED_INTRINSICS_SUBA_VU
#define USE_PACKED_INTRINSICS_SUBAI_VU
#define USE_PACKED_INTRINSICS_SUBAQ_VU
#define USE_PACKED_INTRINSICS_SUBAX_VU
#define USE_PACKED_INTRINSICS_SUBAY_VU
#define USE_PACKED_INTRINSICS_SUBAZ_VU
#define USE_PACKED_INTRINSICS_SUBAW_VU

#define USE_PACKED_INTRINSICS_MADD_VU
#define USE_PACKED_INTRINSICS_MADDI_VU
#define USE_PACKED_INTRINSICS_MADDQ_VU
#define USE_PACKED_INTRINSICS_MADDX_VU
#define USE_PACKED_INTRINSICS_MADDY_VU
#define USE_PACKED_INTRINSICS_MADDZ_VU
#define USE_PACKED_INTRINSICS_MADDW_VU

#define USE_PACKED_INTRINSICS_MADDA_VU
#define USE_PACKED_INTRINSICS_MADDAI_VU
#define USE_PACKED_INTRINSICS_MADDAQ_VU
#define USE_PACKED_INTRINSICS_MADDAX_VU
#define USE_PACKED_INTRINSICS_MADDAY_VU
#define USE_PACKED_INTRINSICS_MADDAZ_VU
#define USE_PACKED_INTRINSICS_MADDAW_VU

#define USE_PACKED_INTRINSICS_MSUB_VU
#define USE_PACKED_INTRINSICS_MSUBI_VU
#define USE_PACKED_INTRINSICS_MSUBQ_VU
#define USE_PACKED_INTRINSICS_MSUBX_VU
#define USE_PACKED_INTRINSICS_MSUBY_VU
#define USE_PACKED_INTRINSICS_MSUBZ_VU
#define USE_PACKED_INTRINSICS_MSUBW_VU

#define USE_PACKED_INTRINSICS_MSUBA_VU
#define USE_PACKED_INTRINSICS_MSUBAI_VU
#define USE_PACKED_INTRINSICS_MSUBAQ_VU
#define USE_PACKED_INTRINSICS_MSUBAX_VU
#define USE_PACKED_INTRINSICS_MSUBAY_VU
#define USE_PACKED_INTRINSICS_MSUBAZ_VU
#define USE_PACKED_INTRINSICS_MSUBAW_VU

#define USE_PACKED_INTRINSICS_OPMULA_VU
#define USE_PACKED_INTRINSICS_OPMSUB_VU


// will need this for accurate operation, and cycle accuracy is required

#define ENABLE_STALLS
#define ENABLE_STALLS_INT

// these are for if not doing execution reorder
/*
#define ENABLE_STALLS_MOVE
#define ENABLE_STALLS_MR32

#define ENABLE_STALLS_LQ
#define ENABLE_STALLS_LQD
#define ENABLE_STALLS_LQI
#define ENABLE_STALLS_RGET
#define ENABLE_STALLS_RNEXT
#define ENABLE_STALLS_MFIR
#define ENABLE_STALLS_MFP
*/


#define ENABLE_INTDELAYSLOT

#define ENABLE_INTDELAYSLOT_CALC


#define ENABLE_INTDELAYSLOT_ILW_BEFORE
#define ENABLE_INTDELAYSLOT_ILWR_BEFORE

#define ENABLE_INTDELAYSLOT_ISW
#define ENABLE_INTDELAYSLOT_ISWR


#define ENABLE_INTDELAYSLOT_LQD_BEFORE
#define ENABLE_INTDELAYSLOT_LQI_BEFORE
#define ENABLE_INTDELAYSLOT_SQD_BEFORE
#define ENABLE_INTDELAYSLOT_SQI_BEFORE


//#define ENABLE_INTDELAYSLOT_LQD_AFTER
//#define ENABLE_INTDELAYSLOT_LQI_AFTER
//#define ENABLE_INTDELAYSLOT_SQD_AFTER
//#define ENABLE_INTDELAYSLOT_SQI_AFTER




#define PERFORM_CLIP_AS_INTEGER



#define ENABLE_SNAPSHOTS


//#define USE_NEW_RECOMPILE2

#define USE_NEW_RECOMPILE2_INTCALC

#define USE_NEW_RECOMPILE2_MOVE
#define USE_NEW_RECOMPILE2_MR32

#define USE_NEW_RECOMPILE2_LQI
#define USE_NEW_RECOMPILE2_LQD
#define USE_NEW_RECOMPILE2_SQI
#define USE_NEW_RECOMPILE2_SQD



// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

//#define INLINE_DEBUG_SPLIT


//#define INLINE_DEBUG_STALLS

//#define INLINE_DEBUG_XGKICK
#define INLINE_DEBUG_INVALID

//#define INLINE_DEBUG_QFLAG

//#define INLINE_DEBUG_ADD
//#define INLINE_DEBUG_IAND

//#define INLINE_DEBUG_MULAX

/*
#define INLINE_DEBUG_SUB
#define INLINE_DEBUG_SUBI
#define INLINE_DEBUG_SUBQ
#define INLINE_DEBUG_SUBBCX
#define INLINE_DEBUG_SUBBCY
#define INLINE_DEBUG_SUBBCW
#define INLINE_DEBUG_SUBBCZ
#define INLINE_DEBUG_SUBA
#define INLINE_DEBUG_SUBAI
#define INLINE_DEBUG_SUBAQ
#define INLINE_DEBUG_SUBAX
#define INLINE_DEBUG_SUBAY
#define INLINE_DEBUG_SUBAW
#define INLINE_DEBUG_SUBAZ
*/

/*
#define INLINE_DEBUG_ADD
#define INLINE_DEBUG_ADDI
#define INLINE_DEBUG_ADDQ
#define INLINE_DEBUG_ADDBCX
#define INLINE_DEBUG_ADDBCY
#define INLINE_DEBUG_ADDBCW
#define INLINE_DEBUG_ADDBCZ
#define INLINE_DEBUG_ADDA
#define INLINE_DEBUG_ADDAI
#define INLINE_DEBUG_ADDAQ
#define INLINE_DEBUG_ADDAX
#define INLINE_DEBUG_ADDAY
#define INLINE_DEBUG_ADDAW
#define INLINE_DEBUG_ADDAZ
*/

/*
#define INLINE_DEBUG_MADD
#define INLINE_DEBUG_MADDI
#define INLINE_DEBUG_MADDQ
#define INLINE_DEBUG_MADDX
#define INLINE_DEBUG_MADDY
#define INLINE_DEBUG_MADDZ
#define INLINE_DEBUG_MADDW

#define INLINE_DEBUG_MADDA
#define INLINE_DEBUG_MADDAI
#define INLINE_DEBUG_MADDAQ
#define INLINE_DEBUG_MADDAX
#define INLINE_DEBUG_MADDAY
#define INLINE_DEBUG_MADDAZ
#define INLINE_DEBUG_MADDAW
*/

//#define INLINE_DEBUG_MFIR
//#define INLINE_DEBUG_MTIR

//#define INLINE_DEBUG_RINIT
//#define INLINE_DEBUG_RGET
//#define INLINE_DEBUG_RNEXT
//#define INLINE_DEBUG_LD

//#define INLINE_DEBUG_VU
//#define INLINE_DEBUG_UNIMPLEMENTED
//#define INLINE_DEBUG_EXT


#endif




const char Execute::XyzwLUT [ 4 ] = { 'x', 'y', 'z', 'w' };
const char* Execute::BCType [ 4 ] = { "F", "T", "FL", "TL" };






void Execute::Start ()
{
	// make sure the lookup object has started (note: this can take a LONG time for R5900 currently)
	Lookup::Start ();
	
#ifdef INLINE_DEBUG_ENABLE

#ifdef INLINE_DEBUG_SPLIT
	// put debug output into a separate file
	debug.SetSplit ( true );
	debug.SetCombine ( false );
#endif

	debug.Create ( "PS2_VU_Execute_Log.txt" );
#endif
}



void Execute::INVALID ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_INVALID || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << "INVALID" << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	cout << "\nhps2x64: ERROR: VU: Invalid instruction encountered. VU#" << v->Number << " PC=" << hex << v->PC << " Code=" << i.Value << " Cycle#" << dec << v->CycleCount;
}


// instruction execution //


template<const int FP>
void Execute::EXECUTE_PS2_DOUBLE_FTOI_PACKED_AVX2(VU* v, Vu::Instruction::Format i)
{
	__m128i iop128_0, iop128_1, iop128_offset, iop128_2, iop128_3, iop128_mask;

	iop128_0 = _mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0);

	if (FP)
	{
		iop128_0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(iop128_0), _mm_castsi128_ps(_mm_set1_epi32((127 + FP) << 23))));
	}


	iop128_1 = _mm_cvtps_epi32(_mm_castsi128_ps(iop128_0));

	//iop128_offset = _mm_sign_epi32(_mm_srli_epi32(_mm_xor_si128(iop128_0, iop128_1), 31), _mm_xor_si128(_mm_srai_epi32(iop128_1, 31), iop128_1));
	iop128_offset = _mm_sign_epi32(_mm_srli_epi32(_mm_xor_si128(iop128_0, iop128_1), 31), iop128_1);

	iop128_0 = _mm_sub_epi32(iop128_1, iop128_offset);

	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));
	_mm_maskstore_epi32((int*)&v->vf[i.Ft].sw0, iop128_mask, iop128_0);
}

template<const int FP>
void Execute::EXECUTE_PS2_DOUBLE_ITOF_PACKED_AVX2(VU* v, Vu::Instruction::Format i)
{
	__m128i iop128_0, iop128_1, iop128_offset, iop128_2, iop128_3, iop128_mask;

	iop128_0 = _mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0);

	iop128_0 = _mm_cvtepi32_ps(iop128_0);

	if (FP)
	{
		iop128_0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(iop128_0), _mm_castsi128_ps(_mm_set1_epi32((127 - FP) << 23))));
	}

	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));
	_mm_maskstore_epi32((int*)&v->vf[i.Ft].sw0, iop128_mask, iop128_0);
}

void Execute::EXECUTE_PS2_DOUBLE_ABS_PACKED_AVX2(VU* v, Vu::Instruction::Format i)
{
	__m128i iop128_0, iop128_1, iop128_offset, iop128_2, iop128_3, iop128_mask;

	iop128_0 = _mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0);

	iop128_0 = _mm_srli_epi32(_mm_slli_epi32(iop128_0, 1), 1);

	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));
	_mm_maskstore_epi32((int*)&v->vf[i.Ft].sw0, iop128_mask, iop128_0);
}

template<const int FTCOMP>
void Execute::EXECUTE_PS2_DOUBLE_MIN_PACKED_AVX2(VU* v, Vu::Instruction::Format i, u32* pFt)
{
	__m128i iop128_0, iop128_1, iop128_offset, iop128_2, iop128_3, iop128_mask;
	//__m256i iop256_0, iop256_1, iop256_2, iop256_offset, iop256_offset0, iop256_offset1, iop256_overflow, iop256_underflow;
	//u32 u_ps2_stat_flags, u_ps2_mac_flags;

	if (FTCOMP)
	{
		//e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
		iop128_1 = _mm_load_si128((const __m128i*) & v->vf[i.Ft].sw0);
	}
	else
	{
		//e->vpbroadcastd1regmem(RCX, (long*)pFt);
		iop128_1 = _mm_set1_epi32(*pFt);
	}

	//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);
	iop128_0 = _mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0);

	iop128_2 = _mm_xor_si128(iop128_0, _mm_srli_epi32(_mm_srai_epi32(iop128_0, 31), 1));
	iop128_3 = _mm_xor_si128(iop128_1, _mm_srli_epi32(_mm_srai_epi32(iop128_1, 31), 1));

	iop128_0 = _mm_blendv_epi8(iop128_0, iop128_1, _mm_cmpgt_epi32(iop128_2, iop128_3));

	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));
	_mm_maskstore_epi32((int*)&v->vf[i.Fd].sw0, iop128_mask, iop128_0);

}

template<const int FTCOMP>
void Execute::EXECUTE_PS2_DOUBLE_MAX_PACKED_AVX2(VU* v, Vu::Instruction::Format i, u32* pFt)
{
	__m128i iop128_0, iop128_1, iop128_offset, iop128_2, iop128_3, iop128_mask;
	//__m256i iop256_0, iop256_1, iop256_2, iop256_offset, iop256_offset0, iop256_offset1, iop256_overflow, iop256_underflow;
	//u32 u_ps2_stat_flags, u_ps2_mac_flags;

	if (FTCOMP)
	{
		//e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
		iop128_1 = _mm_load_si128((const __m128i*) & v->vf[i.Ft].sw0);
	}
	else
	{
		//e->vpbroadcastd1regmem(RCX, (long*)pFt);
		iop128_1 = _mm_set1_epi32(*pFt);
	}

	//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);
	iop128_0 = _mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0);

	iop128_2 = _mm_xor_si128(iop128_0, _mm_srli_epi32(_mm_srai_epi32(iop128_0, 31), 1));
	iop128_3 = _mm_xor_si128(iop128_1, _mm_srli_epi32(_mm_srai_epi32(iop128_1, 31), 1));

	iop128_0 = _mm_blendv_epi8(iop128_0, iop128_1, _mm_cmpgt_epi32(iop128_3, iop128_2));

	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_lut[i.xyzw]));
	_mm_maskstore_epi32((int*)&v->vf[i.Fd].sw0, iop128_mask, iop128_0);

}

template<const int ACCUM, const int FTCOMP, const int FSCOMP>
void Execute::EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2(VU* v, Vu::Instruction::Format i, u32* pFt)
{
	static constexpr long long mantissa_double_mask = 0x000fffffe0000000ull;

	__m128i iop128_0, iop128_1, iop128_offset, iop128_sign, iop128_zero, iop128_overflow, iop128_underflow, iop128_sign_zero, iop128_ovf_udf, iop128_flags, iop128_mask;
	__m256i iop256_0, iop256_1, iop256_2, iop256_offset, iop256_offset0, iop256_offset1, iop256_overflow, iop256_underflow;
	u32 u_ps2_stat_flags, u_ps2_mac_flags;

	if (FTCOMP)
	{
		//e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
		iop128_1 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Ft].sw0), FTCOMP);
	}
	else
	{
		//e->vpbroadcastd1regmem(RCX, (long*)pFt);
		iop128_1 = _mm_set1_epi32(*pFt);
	}

	//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);
	iop128_0 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0), FSCOMP);

	// load args
	//iop128_0 = _mm_load_si128((const __m128i*) & vs.uw0);
	iop256_0 = _mm256_inserti128_si256(_mm256_castsi128_si256(iop128_0), iop128_1, 1);

	// convert to ps2 float/double

	// get offset
	//iop256_offset = _mm256_srli_epi32(_mm256_castps_si256(_mm256_sub_ps(_mm256_castsi256_ps(iop256_0), _mm256_castsi256_ps(iop256_0))), 7);
	iop256_offset = _mm256_slli_epi32(_mm256_srli_epi32(_mm256_slli_epi32(iop256_0, 1), 31), 23);

	// sub offset
	iop256_0 = _mm256_sub_epi32(iop256_0, iop256_offset);

	// convert to double
	iop256_1 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_castsi256_si128(iop256_offset)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_0))))
	);

	iop256_2 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_extracti128_si256(iop256_offset, 1)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_extracti128_si256(iop256_0, 1))))
	);

	// do x*y adjustment
	iop256_2 = _mm256_and_si256(iop256_2,
		_mm256_add_epi64(iop256_2,
			_mm256_cmpeq_epi64(_mm256_and_si256(iop256_2,
				_mm256_set1_epi64x(mantissa_double_mask)), _mm256_set1_epi64x(mantissa_double_mask))));

	// do the double multiply
	iop256_0 = _mm256_castpd_si256(_mm256_mul_pd(
		_mm256_castsi256_pd(iop256_1),
		_mm256_castsi256_pd(iop256_2)
	));


	// get offset
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(_mm256_slli_epi64(iop256_0, 1), 63), 52);

	// convert to raw ps
	iop128_0 = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_castsi256_pd(_mm256_sub_epi64(iop256_0, iop256_offset))));

	// convert back to clamped pd
	iop256_1 = _mm256_add_epi64(_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(iop128_0))), iop256_offset);

	// overflow is if clamped and unclamped masked pd don't match
	iop256_overflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_castsi256_pd(_mm256_and_si256(iop256_0, iop256_1)), _mm256_castsi256_pd(iop256_1), _CMP_NEQ_OQ)
	);

	// underflow
	iop256_underflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_cmp_pd(_mm256_castsi256_pd(iop256_0), _mm256_setzero_pd(), _CMP_EQ_OQ), _mm256_castsi256_pd(iop256_1), _CMP_EQ_OQ)
	);

	// sign/zero flag from raw un-offset ps
	iop128_zero = _mm_castps_si128(_mm_cmp_ps(_mm_castsi128_ps(iop128_0), _mm_setzero_ps(), _CMP_EQ_OQ));
	//iop128_sign = _mm_castps_si128(_mm_cmp_ps(_mm_setzero_ps(), _mm_castsi128_ps(iop128_0), _CMP_GT_OQ));
	iop128_sign = _mm_cmpgt_epi32(_mm_setzero_si128(), iop128_0);

	// get 128-bit overflow/underflow
	iop128_overflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_overflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_overflow, 1)), 0x88));
	iop128_underflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_underflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_underflow, 1)), 0x88));

	// convert offset and offset ps
	iop256_offset = _mm256_srli_epi64(iop256_offset, 29);
	iop128_offset = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_offset)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_offset, 1)), 0x88));

	// offset ps2 float
	iop128_0 = _mm_add_epi32(iop128_0, iop128_offset);

	// iop128_0 -> ps result
	// iop256_0 -> unclamped pd
	// iop256_1 -> clamped pd/pd result

	iop128_mask = _mm_set1_epi32(vec_mask8_lut[i.xyzw]);
	
	// get flags
	// could combine sign(r4) | zero(rcx) here for simd ->rcx
	//e->packsdw1regreg(RCX, RCX, 4);
	//e->packsdw1regreg(RAX, RAX, RBX);
	iop128_sign_zero = _mm_packs_epi32(iop128_zero, iop128_sign);
	iop128_ovf_udf = _mm_packs_epi32(iop128_underflow, iop128_overflow);



	// flags
	//if (i.xyzw != 0xf)
	//{
		//e->pblendw1regregimm(RCX, RCX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//e->pblendw1regregimm(RAX, RAX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_sign_zero = _mm_blend_epi16(iop128_sign_zero, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_ovf_udf = _mm_blend_epi16(iop128_ovf_udf, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
	//}

	//e->packswb1regreg(RAX, RCX, RAX);
	iop128_flags = _mm_packs_epi16(iop128_sign_zero, iop128_ovf_udf);

	// mask the unwanted flags
	iop128_flags = _mm_and_si128(iop128_flags, iop128_mask);

	// now get stat
	//e->psrld1regimm(RCX, RAX, 7);
	//e->pcmpgtd1regreg(RCX, RCX, 5);

	// now pull mac(RAX)->RCX and stat(RCX)->RAX
	//e->movmskb1regreg(RCX, RAX);
	//e->movmskps1regreg(RAX, RCX);
	u_ps2_stat_flags = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(_mm_srli_epi32(iop128_flags, 7), _mm_setzero_si128())));
	u_ps2_mac_flags = _mm_movemask_epi8(iop128_flags);

	// put in the sticky flags
	u_ps2_stat_flags |= u_ps2_stat_flags << 6;

	v->vi[VU::REG_MACFLAG].s = u_ps2_mac_flags;
	if (!v->Status.SetStatus_Flag)
	{
		v->vi[VU::REG_STATUSFLAG].s = (v->vi[VU::REG_STATUSFLAG].s & ~0xf) | u_ps2_stat_flags;
	}

	// store result
	//_mm_store_si128((__m128i*) & vd.uw0, iop128_0);
	iop128_0 = _mm_shuffle_epi32(iop128_0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

	// make mask
	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));

	//if (i.xyzw != 0xf)
	//{
		//if (pFd)
		if(ACCUM)
		{
			//e->pblendw1regmemimm(RDX, RDX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->dACC[0].l, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//e->movdqa1_memreg(pFd, RDX);
			//_mm_store_si128((__m128i*) & v->dACC[0].l, iop128_0);
			_mm_maskstore_epi32((int*)&v->dACC[0].l, iop128_mask, iop128_0);
		}
		else
		{
			if (i.Fd)
			{
				//e->pblendw1regmemimm(RDX, RDX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
				//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
				//_mm_store_si128((__m128i*) & v->vf[i.Fd].sw0, iop128_0);
				_mm_maskstore_epi32((int*)&v->vf[i.Fd].sw0, iop128_mask, iop128_0);
			}
		}
	//}

	//if (pFd)
	//{
	//	//e->movdqa1_memreg(pFd, RDX);
	//	_mm_store_si128((__m128i*) pFd, iop128_0);
	//}
	//else
	//{
	//	if (i.Fd)
	//	{
	//		//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
	//		_mm_store_si128((__m128i*) & v->vf[i.Fd].sw0, iop128_0);
	//	}
	//}

}



template<const int BSUB, const int ACCUM, const int FTCOMP, const int FSCOMP>
void Execute::EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2(VU* v, Vu::Instruction::Format i, u32* pFt)
{
	static constexpr long long mantissa_double_mask = 0x000fffffe0000000ull;

	__m128i iop128_0, iop128_1, iop128_offset, iop128_sign, iop128_zero, iop128_overflow, iop128_underflow, iop128_sign_zero, iop128_ovf_udf, iop128_flags, iop128_mask;
	__m256i iop256_0, iop256_1, iop256_2, iop256_offset, iop256_offset0, iop256_offset1, iop256_overflow, iop256_underflow;
	u32 u_ps2_stat_flags, u_ps2_mac_flags;

	if (FTCOMP)
	{
		//e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
		iop128_1 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Ft].sw0), FTCOMP);
	}
	else
	{
		//e->vpbroadcastd1regmem(RCX, (long*)pFt);
		iop128_1 = _mm_set1_epi32(*pFt);
	}

	//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));
	iop128_0 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0), FSCOMP);

	// load args
	//iop128_0 = _mm_load_si128((const __m128i*) & vs.uw0);
	iop256_0 = _mm256_inserti128_si256(_mm256_castsi128_si256(iop128_0), iop128_1, 1);

	// convert to ps2 float/double

	// get offset
	//iop256_offset = _mm256_srli_epi32(_mm256_castps_si256(_mm256_sub_ps(_mm256_castsi256_ps(iop256_0), _mm256_castsi256_ps(iop256_0))), 7);
	iop256_offset = _mm256_slli_epi32(_mm256_srli_epi32(_mm256_slli_epi32(iop256_0, 1), 31), 23);

	// sub offset
	iop256_0 = _mm256_sub_epi32(iop256_0, iop256_offset);

	// convert to double
	iop256_1 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_castsi256_si128(iop256_offset)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_0))))
	);

	iop256_2 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_extracti128_si256(iop256_offset, 1)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_extracti128_si256(iop256_0, 1))))
	);

	// get guard mask
	//e->pxor2regreg(RBX, RAX, RCX);
	iop256_offset = _mm256_xor_si256(iop256_1, iop256_2);

	if (BSUB)
	{
		//e->pand2regreg(RBX, RAX, RCX);
		iop256_offset = _mm256_xor_si256(iop256_offset, _mm256_set1_epi32(-1));
	}

	//e->psrlq2regimm(RBX, RBX, 63);
	//e->psllq2regimm(RBX, RBX, 27);
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(iop256_offset, 63), 27);


	//e->vaddpd2(RAX, RAX, RCX);
	if (BSUB)
	{
		// sub
		//e->vsubpd2(RAX, RAX, RCX);
		// do the double add
		iop256_0 = _mm256_castpd_si256(_mm256_sub_pd(
			_mm256_castsi256_pd(iop256_1),
			_mm256_castsi256_pd(iop256_2)
		));
	}
	else
	{
		// add
		//e->vaddpd2(RAX, RAX, RCX);
		// do the double add
		iop256_0 = _mm256_castpd_si256(_mm256_add_pd(
			_mm256_castsi256_pd(iop256_1),
			_mm256_castsi256_pd(iop256_2)
		));
	}


	// offset with mask
	//e->paddq2regreg(RAX, RAX, RBX);
	iop256_0 = _mm256_add_epi64(iop256_0, iop256_offset);

	// get offset
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(_mm256_slli_epi64(iop256_0, 1), 63), 52);

	// convert to raw ps
	iop128_0 = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_castsi256_pd(_mm256_sub_epi64(iop256_0, iop256_offset))));

	// convert back to clamped pd
	iop256_1 = _mm256_add_epi64(_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(iop128_0))), iop256_offset);

	// overflow is if clamped and unclamped masked pd don't match
	iop256_overflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_castsi256_pd(_mm256_and_si256(iop256_0, iop256_1)), _mm256_castsi256_pd(iop256_1), _CMP_NEQ_OQ)
	);

	// underflow
	iop256_underflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_cmp_pd(_mm256_castsi256_pd(iop256_0), _mm256_setzero_pd(), _CMP_EQ_OQ), _mm256_castsi256_pd(iop256_1), _CMP_EQ_OQ)
	);

	// sign/zero flag from raw un-offset ps
	iop128_zero = _mm_castps_si128(_mm_cmp_ps(_mm_castsi128_ps(iop128_0), _mm_setzero_ps(), _CMP_EQ_OQ));
	//iop128_sign = _mm_castps_si128(_mm_cmp_ps(_mm_setzero_ps(), _mm_castsi128_ps(iop128_0), _CMP_GT_OQ));
	iop128_sign = _mm_cmpgt_epi32(_mm_setzero_si128(), iop128_0);

	// get 128-bit overflow/underflow
	iop128_overflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_overflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_overflow, 1)), 0x88));
	iop128_underflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_underflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_underflow, 1)), 0x88));

	// convert offset and offset ps
	iop256_offset = _mm256_srli_epi64(iop256_offset, 29);
	iop128_offset = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_offset)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_offset, 1)), 0x88));

	// offset ps2 float
	iop128_0 = _mm_add_epi32(iop128_0, iop128_offset);

	// iop128_0 -> ps result
	// iop256_0 -> unclamped pd
	// iop256_1 -> clamped pd/pd result

	iop128_mask = _mm_set1_epi32(vec_mask8_lut[i.xyzw]);


	// get flags
	// could combine sign(r4) | zero(rcx) here for simd ->rcx
	//e->packsdw1regreg(RCX, RCX, 4);
	//e->packsdw1regreg(RAX, RAX, RBX);
	iop128_sign_zero = _mm_packs_epi32(iop128_zero, iop128_sign);
	iop128_ovf_udf = _mm_packs_epi32(iop128_underflow, iop128_overflow);

	// flags
	//if (i.xyzw != 0xf)
	//{
		//e->pblendw1regregimm(RCX, RCX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//e->pblendw1regregimm(RAX, RAX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_sign_zero = _mm_blend_epi16(iop128_sign_zero, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_ovf_udf = _mm_blend_epi16(iop128_ovf_udf, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
	//}

	//e->packswb1regreg(RAX, RCX, RAX);
	iop128_flags = _mm_packs_epi16(iop128_sign_zero, iop128_ovf_udf);

	// now get stat
	//e->psrld1regimm(RCX, RAX, 7);
	//e->pcmpgtd1regreg(RCX, RCX, 5);

	// mask the unwanted flags
	iop128_flags = _mm_and_si128(iop128_flags, iop128_mask);

	// now pull mac(RAX)->RCX and stat(RCX)->RAX
	//e->movmskb1regreg(RCX, RAX);
	//e->movmskps1regreg(RAX, RCX);
	u_ps2_stat_flags = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(_mm_srli_epi32(iop128_flags, 7), _mm_setzero_si128())));
	u_ps2_mac_flags = _mm_movemask_epi8(iop128_flags);

	// put in the sticky flags
	u_ps2_stat_flags |= u_ps2_stat_flags << 6;

	v->vi[VU::REG_MACFLAG].s = u_ps2_mac_flags;
	if (!v->Status.SetStatus_Flag)
	{
		v->vi[VU::REG_STATUSFLAG].s = (v->vi[VU::REG_STATUSFLAG].s & ~0xf) | u_ps2_stat_flags;
	}

	// store result
	//_mm_store_si128((__m128i*) & vd.uw0, iop128_0);
	iop128_0 = _mm_shuffle_epi32(iop128_0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

	// make mask
	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));

	//if (i.xyzw != 0xf)
	//{
		//if (pFd)
	if (ACCUM)
	{
		//e->pblendw1regmemimm(RDX, RDX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
		//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->dACC[0].l, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
		//e->movdqa1_memreg(pFd, RDX);
		//_mm_store_si128((__m128i*) & v->dACC[0].l, iop128_0);
		_mm_maskstore_epi32((int*)&v->dACC[0].l, iop128_mask, iop128_0);
	}
	else
	{
		if (i.Fd)
		{
			//e->pblendw1regmemimm(RDX, RDX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
			//_mm_store_si128((__m128i*) & v->vf[i.Fd].sw0, iop128_0);
			_mm_maskstore_epi32((int*)&v->vf[i.Fd].sw0, iop128_mask, iop128_0);
		}
	}
	//}


	/*
	if (pFd)
	{
		//e->movdqa1_memreg(pFd, RDX);
		_mm_store_si128((__m128i*) pFd, iop128_0);
	}
	else
	{
		if (i.Fd)
		{
			//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
			_mm_store_si128((__m128i*) & v->vf[i.Fd].sw0, iop128_0);
		}
	}
	*/

}


template<const int BSUB, const int ACCUM, const int FTCOMP, const int FSCOMP>
void Execute::EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2(VU* v, Vu::Instruction::Format i, u32* pFt)
{
	static constexpr long long mantissa_double_mask = 0x000fffffe0000000ull;
	static constexpr long long max_double_float = 0x47ffffffe0000000ull;

	__m128i iop128_0, iop128_1, iop128_offset, iop128_sign, iop128_zero, iop128_overflow, iop128_underflow, iop128_sign_zero, iop128_ovf_udf, iop128_flags, iop128_mask;
	__m256i iop256_0, iop256_1, iop256_2, iop256_offset, iop256_offset0, iop256_offset1, iop256_overflow, iop256_underflow;
	u32 u_ps2_stat_flags, u_ps2_mac_flags, u_ps2_stat_sticky_flags;

	if (FTCOMP)
	{
		//e->pshufd1regmemimm(RCX, &v->vf[i.Ft].sw0, FtComponentp);
		iop128_1 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Ft].sw0), FTCOMP);
	}
	else
	{
		//e->vpbroadcastd1regmem(RCX, (long*)pFt);
		iop128_1 = _mm_set1_epi32(*pFt);
	}

	//e->pshufd1regmemimm(RAX, &v->vf[i.Fs].sw0, FsComponentp);
	iop128_0 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->vf[i.Fs].sw0), FSCOMP);

	// load args
	//iop128_0 = _mm_load_si128((const __m128i*) & vs.uw0);
	iop256_0 = _mm256_inserti128_si256(_mm256_castsi128_si256(iop128_0), iop128_1, 1);

	// convert to ps2 float/double

	// get offset
	//iop256_offset = _mm256_srli_epi32(_mm256_castps_si256(_mm256_sub_ps(_mm256_castsi256_ps(iop256_0), _mm256_castsi256_ps(iop256_0))), 7);
	iop256_offset = _mm256_slli_epi32(_mm256_srli_epi32(_mm256_slli_epi32(iop256_0, 1), 31), 23);

	// sub offset
	iop256_0 = _mm256_sub_epi32(iop256_0, iop256_offset);

	// convert to double
	iop256_1 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_castsi256_si128(iop256_offset)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_0))))
	);

	iop256_2 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(_mm256_extracti128_si256(iop256_offset, 1)), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(_mm256_extracti128_si256(iop256_0, 1))))
	);


	// do x*y adjustment
	iop256_2 = _mm256_and_si256(iop256_2,
		_mm256_add_epi64(iop256_2,
			_mm256_cmpeq_epi64(_mm256_and_si256(iop256_2,
				_mm256_set1_epi64x(mantissa_double_mask)), _mm256_set1_epi64x(mantissa_double_mask))));

	// do the double multiply
	iop256_0 = _mm256_castpd_si256(_mm256_mul_pd(
		_mm256_castsi256_pd(iop256_1),
		_mm256_castsi256_pd(iop256_2)
	));


	// get offset
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(_mm256_slli_epi64(iop256_0, 1), 63), 52);

	// convert to raw ps
	iop128_0 = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_castsi256_pd(_mm256_sub_epi64(iop256_0, iop256_offset))));

	// convert back to clamped pd
	iop256_1 = _mm256_add_epi64(_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(iop128_0))), iop256_offset);

	// overflow is if clamped and unclamped masked pd don't match
	iop256_overflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_castsi256_pd(_mm256_and_si256(iop256_0, iop256_1)), _mm256_castsi256_pd(iop256_1), _CMP_NEQ_OQ)
	);

	// underflow
	iop256_underflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_cmp_pd(_mm256_castsi256_pd(iop256_0), _mm256_setzero_pd(), _CMP_EQ_OQ), _mm256_castsi256_pd(iop256_1), _CMP_EQ_OQ)
	);

	// get sticky underflow flag
	u_ps2_stat_sticky_flags = (_mm256_movemask_pd(_mm256_castsi256_pd(iop256_underflow)) & i.xyzw) ? 0x100 : 0;


	// negate result if msub
	if (BSUB)
	{
		iop256_1 = _mm256_castpd_si256(_mm256_sub_pd(_mm256_setzero_pd(), _mm256_castsi256_pd(iop256_1)));
	}

	// get accumulator //
	//iop128_1 = _mm_load_si128((const __m128i*) & v->dACC[0].l);
	iop128_1 = _mm_shuffle_epi32(_mm_load_si128((const __m128i*) & v->dACC[0].l), (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

	// get offset
	//iop128_offset = _mm_srli_epi32(_mm_castps_si128(_mm_sub_ps(_mm_castsi128_ps(iop128_1), _mm_castsi128_ps(iop128_1))), 7);
	iop128_offset = _mm_slli_epi32(_mm_srli_epi32(_mm_slli_epi32(iop128_1, 1), 31), 23);

	// sub offset
	iop128_1 = _mm_sub_epi32(iop128_1, iop128_offset);

	// convert to double
	iop256_2 = _mm256_add_epi64(
		_mm256_slli_epi64(_mm256_cvtepi32_epi64(iop128_offset), 29),
		_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(iop128_1)))
	);

	// handle madd overflow //

	// send mul overflow to acc value
	iop256_2 = _mm256_blendv_epi8(iop256_2, iop256_1, iop256_overflow);

	// check for acc overflow
	//e->pand2regreg(5, 4, RCX);
	//e->pcmpeqq2regreg(RDX, 4, 5);
	// copy acc(r5) to result(rbx) on overflow (r4) -> rax
	//e->pblendvb2regreg(RAX, RBX, RCX, RDX);

	iop256_1 = _mm256_blendv_epi8(
		iop256_1, iop256_2,
		_mm256_cmpeq_epi64(_mm256_and_si256(iop256_2, _mm256_set1_epi64x(max_double_float)), _mm256_set1_epi64x(max_double_float))
	);

	// get guard mask
	//e->pxor2regreg(RBX, RAX, RCX);
	iop256_offset = _mm256_xor_si256(iop256_1, iop256_2);

	//e->psrlq2regimm(RBX, RBX, 63);
	//e->psllq2regimm(RBX, RBX, 27);
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(iop256_offset, 63), 27);


	//e->vaddpd2(RAX, RAX, RCX);
	// add
	//e->vaddpd2(RAX, RAX, RCX);
	// do the double add
	iop256_0 = _mm256_castpd_si256(_mm256_add_pd(
		_mm256_castsi256_pd(iop256_1),
		_mm256_castsi256_pd(iop256_2)
	));


	// offset with mask
	//e->paddq2regreg(RAX, RAX, RBX);
	iop256_0 = _mm256_add_epi64(iop256_0, iop256_offset);

	// get offset
	iop256_offset = _mm256_slli_epi64(_mm256_srli_epi64(_mm256_slli_epi64(iop256_0, 1), 63), 52);

	// convert to raw ps
	iop128_0 = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_castsi256_pd(_mm256_sub_epi64(iop256_0, iop256_offset))));

	// convert back to clamped pd
	iop256_1 = _mm256_add_epi64(_mm256_castpd_si256(_mm256_cvtps_pd(_mm_castsi128_ps(iop128_0))), iop256_offset);

	// overflow is if clamped and unclamped masked pd don't match
	iop256_overflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_castsi256_pd(_mm256_and_si256(iop256_0, iop256_1)), _mm256_castsi256_pd(iop256_1), _CMP_NEQ_OQ)
	);

	// underflow
	iop256_underflow = _mm256_castpd_si256(
		_mm256_cmp_pd(_mm256_cmp_pd(_mm256_castsi256_pd(iop256_0), _mm256_setzero_pd(), _CMP_EQ_OQ), _mm256_castsi256_pd(iop256_1), _CMP_EQ_OQ)
	);

	// sign/zero flag from raw un-offset ps
	iop128_zero = _mm_castps_si128(_mm_cmp_ps(_mm_castsi128_ps(iop128_0), _mm_setzero_ps(), _CMP_EQ_OQ));
	//iop128_sign = _mm_castps_si128(_mm_cmp_ps(_mm_setzero_ps(), _mm_castsi128_ps(iop128_0), _CMP_GT_OQ));
	iop128_sign = _mm_cmpgt_epi32(_mm_setzero_si128(), iop128_0);

	// get 128-bit overflow/underflow
	iop128_overflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_overflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_overflow, 1)), 0x88));
	iop128_underflow = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_underflow)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_underflow, 1)), 0x88));

	// convert offset and offset ps
	iop256_offset = _mm256_srli_epi64(iop256_offset, 29);
	iop128_offset = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_mm256_castsi256_si128(iop256_offset)), _mm_castsi128_ps(_mm256_extracti128_si256(iop256_offset, 1)), 0x88));

	// offset ps2 float
	iop128_0 = _mm_add_epi32(iop128_0, iop128_offset);

	// iop128_0 -> ps result
	// iop256_0 -> unclamped pd
	// iop256_1 -> clamped pd/pd result

	iop128_mask = _mm_set1_epi32(vec_mask8_lut[i.xyzw]);


	// get flags
	// could combine sign(r4) | zero(rcx) here for simd ->rcx
	//e->packsdw1regreg(RCX, RCX, 4);
	//e->packsdw1regreg(RAX, RAX, RBX);
	iop128_sign_zero = _mm_packs_epi32(iop128_zero, iop128_sign);
	iop128_ovf_udf = _mm_packs_epi32(iop128_underflow, iop128_overflow);

	// flags
	//if (i.xyzw != 0xf)
	//{
		//e->pblendw1regregimm(RCX, RCX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//e->pblendw1regregimm(RAX, RAX, 5, ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_sign_zero = _mm_blend_epi16(iop128_sign_zero, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
		//iop128_ovf_udf = _mm_blend_epi16(iop128_ovf_udf, _mm_setzero_si128(), ~((i.destx * 0x88) | (i.desty * 0x44) | (i.destz * 0x22) | (i.destw * 0x11)));
	//}

	//e->packswb1regreg(RAX, RCX, RAX);
	iop128_flags = _mm_packs_epi16(iop128_sign_zero, iop128_ovf_udf);

	// now get stat
	//e->psrld1regimm(RCX, RAX, 7);
	//e->pcmpgtd1regreg(RCX, RCX, 5);

	// mask the unwanted flags
	iop128_flags = _mm_and_si128(iop128_flags, iop128_mask);

	// now pull mac(RAX)->RCX and stat(RCX)->RAX
	//e->movmskb1regreg(RCX, RAX);
	//e->movmskps1regreg(RAX, RCX);
	u_ps2_stat_flags = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(_mm_srli_epi32(iop128_flags, 7), _mm_setzero_si128())));
	u_ps2_mac_flags = _mm_movemask_epi8(iop128_flags);

	// put in the sticky flags
	u_ps2_stat_flags |= u_ps2_stat_flags << 6;

	v->vi[VU::REG_MACFLAG].s = u_ps2_mac_flags;
	if (!v->Status.SetStatus_Flag)
	{
		v->vi[VU::REG_STATUSFLAG].s = (v->vi[VU::REG_STATUSFLAG].s & ~0xf) | u_ps2_stat_flags | u_ps2_stat_sticky_flags;
	}

	// store result
	//_mm_store_si128((__m128i*) & vd.uw0, iop128_0);
	iop128_0 = _mm_shuffle_epi32(iop128_0, (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0));

	// make mask
	iop128_mask = _mm_cvtepi8_epi32(_mm_cvtsi32_si128(vec_mask8_rev_lut[i.xyzw]));

	//if (i.xyzw != 0xf)
	//{
		//if (pFd)
	if (ACCUM)
	{
		//e->pblendw1regmemimm(RDX, RDX, pFd, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
		//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->dACC[0].l, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
		//e->movdqa1_memreg(pFd, RDX);
		//_mm_store_si128((__m128i*) & v->dACC[0].l, iop128_0);
		_mm_maskstore_epi32((int*)&v->dACC[0].l, iop128_mask, iop128_0);
	}
	else
	{
		if (i.Fd)
		{
			//e->pblendw1regmemimm(RDX, RDX, &v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//iop128_0 = _mm_blend_epi16(iop128_0, *(__m128i*) & v->vf[i.Fd].sw0, ~((i.destx * 0x03) | (i.desty * 0x0c) | (i.destz * 0x30) | (i.destw * 0xc0)));
			//e->movdqa1_memreg(&v->vf[i.Fd].sw0, RDX);
			//_mm_store_si128((__m128i*) & v->vf[i.Fd].sw0, iop128_0);
			_mm_maskstore_epi32((int*)&v->vf[i.Fd].sw0, iop128_mask, iop128_0);
		}
	}
	//}

}




//// *** UPPER instructions *** ////


// ABS //

void Execute::ABS ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ABS || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);
	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);


	if ( i.destx )
	{
		v->vf [ i.Ft ].uw0 = v->vf [ i.Fs ].uw0 & ~0x80000000;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].uw1 = v->vf [ i.Fs ].uw1 & ~0x80000000;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].uw2 = v->vf [ i.Fs ].uw2 & ~0x80000000;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].uw3 = v->vf [ i.Fs ].uw3 & ~0x80000000;
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none

#if defined INLINE_DEBUG_ABS || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}





// ADD //

void Execute::ADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_ADD_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	// fd = fs + ft
	VuUpperOp ( v, i, PS2_Float_Add );
	
#endif

#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

	// fd = fs + I
	
#ifdef USE_PACKED_INTRINSICS_ADDI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vi[VU::REG_I].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpI ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

	// fd = fs + Q
	
#ifdef USE_PACKED_INTRINSICS_ADDQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vi[VU::REG_Q].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpQ ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
#ifdef USE_PACKED_INTRINSICS_ADDX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpX ( v, i, PS2_Float_Add );

#endif

#if defined INLINE_DEBUG_ADDBCX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
#ifdef USE_PACKED_INTRINSICS_ADDY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpY ( v, i, PS2_Float_Add );

#endif

#if defined INLINE_DEBUG_ADDBCY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
#ifdef USE_PACKED_INTRINSICS_ADDZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpZ ( v, i, PS2_Float_Add );

#endif
	

#if defined INLINE_DEBUG_ADDBCZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc

#ifdef USE_PACKED_INTRINSICS_ADDW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpW ( v, i, PS2_Float_Add );

#endif
	

#if defined INLINE_DEBUG_ADDBCW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}




// SUB //

void Execute::SUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs - ft

#ifdef USE_PACKED_INTRINSICS_SUB_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOp ( v, i, PS2_Float_Sub );
	
#endif

#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vi[VU::REG_I].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpI ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vi[VU::REG_Q].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpQ ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpX ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpY ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpZ ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpW ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}




// MADD //

void Execute::MADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADD_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOp ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vi[VU::REG_I].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpI ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vi[VU::REG_Q].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpQ ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpX ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpY ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << std::fixed << std::setprecision(0) << v->vi [ VU::REG_STATUSFLAG ].uLo << " STATF=" << v->vi [ VU::REG_MACFLAG ].uLo;
#endif
}

void Execute::MADDBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	//debug << " Fs(hex)= x=" << hex << v->vf [ i.Fs ].ux << " y=" << v->vf [ i.Fs ].uy << " z=" << v->vf [ i.Fs ].uz << " w=" << v->vf [ i.Fs ].uw;
	//debug << " Ft(hex)= x=" << hex << v->vf [ i.Ft ].ux << " y=" << v->vf [ i.Ft ].uy << " z=" << v->vf [ i.Ft ].uz << " w=" << v->vf [ i.Ft ].uw;
	//debug << " ACC(hex)= x=" << v->dACC [ 0 ].l << " y=" << v->dACC [ 1 ].l << " z=" << v->dACC [ 2 ].l << " w=" << v->dACC [ 3 ].l;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpZ ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	//debug << " Output(hex): Fd=" << " vfx=" << hex << v->vf [ i.Fd ].ux << " vfy=" << v->vf [ i.Fd ].uy << " vfz=" << v->vf [ i.Fd ].uz << " vfw=" << v->vf [ i.Fd ].uw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpW ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



// MSUB //

void Execute::MSUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUB_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOp ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vi[VU::REG_I].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpI ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vi[VU::REG_Q].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpQ ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpX ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpY ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpZ ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpW ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



// MUL //

void Execute::MUL ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MUL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MUL_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOp ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MUL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_MULI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vi[VU::REG_I].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpI ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_MULQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vi[VU::REG_Q].u);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpQ ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_MULQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpX ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpY ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpZ ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<false, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOpW ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_MULW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



// MAX //

void Execute::MAX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// set the source register(s)
	//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MAXi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);
	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAXI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MAXBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fx;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);
	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	// don't want to overwrite and propagate...
	fx = v->vf [ i.Ft ].fx;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, fx );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, fx );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, fx );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAXX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MAXBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fy;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	fy = v->vf [ i.Ft ].fy;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, fy );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, fy );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, fy );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MAXBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fz;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	fz = v->vf [ i.Ft ].fz;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, fz );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, fz );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, fz );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MAXBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fw;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	fw = v->vf [ i.Ft ].fw;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, fw );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, fw );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, fw );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, fw );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MAXW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}



// MINI //

void Execute::MINI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// set the source register(s)
	//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);
	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MINIi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINII || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINII || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MINIBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fx;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);


	fx = v->vf [ i.Ft ].fx;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, fx );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, fx );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, fx );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINIX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MINIBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fy;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	fy = v->vf [ i.Ft ].fy;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, fy );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, fy );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, fy );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINIY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MINIBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fz;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	fz = v->vf [ i.Ft ].fz;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, fz );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, fz );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, fz );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINIZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

void Execute::MINIBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fw;

	// set the source register(s)
	//v->Set_SrcRegsBC ( i.Value, i.Fs, i.Ft );
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Fd );
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	fw = v->vf [ i.Ft ].fw;

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, fw );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, fw );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, fw );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, fw );
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;
	
#if defined INLINE_DEBUG_MINIW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}




// ITOF //

void Execute::ITOF0 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].sx << " vfy=" << v->vf [ i.Fs ].sy << " vfz=" << v->vf [ i.Fs ].sz << " vfw=" << v->vf [ i.Fs ].sw;
#endif

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);


	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = (float) v->vf [ i.Fs ].sw0;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = (float) v->vf [ i.Fs ].sw1;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = (float) v->vf [ i.Fs ].sw2;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = (float) v->vf [ i.Fs ].sw3;
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none

#if defined INLINE_DEBUG_ITOF0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::FTOI0 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = PS2_Float_ToInteger ( v->vf [ i.Fs ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = PS2_Float_ToInteger ( v->vf [ i.Fs ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = PS2_Float_ToInteger ( v->vf [ i.Fs ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = PS2_Float_ToInteger ( v->vf [ i.Fs ].fw );
	}
	
	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].sx << " vfy=" << v->vf [ i.Ft ].sy << " vfz=" << v->vf [ i.Ft ].sz << " vfw=" << v->vf [ i.Ft ].sw;
#endif
}

void Execute::ITOF4 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].sx << " vfy=" << v->vf [ i.Fs ].sy << " vfz=" << v->vf [ i.Fs ].sz << " vfw=" << v->vf [ i.Fs ].sw;
#endif

	static const float c_fMultiplier = 1.0f / 16.0f;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::FTOI4 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 16.0f;

	float fProdTemp;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);


	if ( i.destx )
	{
		fProdTemp = v->vf[i.Fs].fx;
		if ((v->vf[i.Fs].uw0 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw0 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.desty )
	{
		fProdTemp = v->vf[i.Fs].fy;
		if ((v->vf[i.Fs].uw1 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw1 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destz )
	{
		fProdTemp = v->vf[i.Fs].fz;
		if ((v->vf[i.Fs].uw2 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw2 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destw )
	{
		fProdTemp = v->vf[i.Fs].fw;
		if ((v->vf[i.Fs].uw3 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw3 = PS2_Float_ToInteger(fProdTemp);
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;

	// flags affected: none

#if defined INLINE_DEBUG_FTOI4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].sx << " vfy=" << v->vf [ i.Ft ].sy << " vfz=" << v->vf [ i.Ft ].sz << " vfw=" << v->vf [ i.Ft ].sw;
#endif
}

void Execute::ITOF12 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].sx << " vfy=" << v->vf [ i.Fs ].sy << " vfz=" << v->vf [ i.Fs ].sz << " vfw=" << v->vf [ i.Fs ].sw;
#endif

	static const float c_fMultiplier = 1.0f / 4096.0f;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::FTOI12 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 4096.0f;
	
	float fProdTemp;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		fProdTemp = v->vf[i.Fs].fx;
		if ((v->vf[i.Fs].uw0 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw0 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.desty )
	{
		fProdTemp = v->vf[i.Fs].fy;
		if ((v->vf[i.Fs].uw1 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw1 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destz )
	{
		fProdTemp = v->vf[i.Fs].fz;
		if ((v->vf[i.Fs].uw2 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw2 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destw )
	{
		fProdTemp = v->vf[i.Fs].fw;
		if ((v->vf[i.Fs].uw3 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw3 = PS2_Float_ToInteger(fProdTemp);
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].sx << " vfy=" << v->vf [ i.Ft ].sy << " vfz=" << v->vf [ i.Ft ].sz << " vfw=" << v->vf [ i.Ft ].sw;
#endif
}

void Execute::ITOF15 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].sx << " vfy=" << v->vf [ i.Fs ].sy << " vfz=" << v->vf [ i.Fs ].sz << " vfw=" << v->vf [ i.Fs ].sw;
#endif

	static const float c_fMultiplier = 1.0f / 32768.0f;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::FTOI15 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 32768.0f;
	
	float fProdTemp;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	//v->Set_DestReg_Upper ( i.Value, i.Ft );
	v->Set_FDstReg_Wait(i.Value, i.Ft);

	if ( i.destx )
	{
		fProdTemp = v->vf[i.Fs].fx;
		if ((v->vf[i.Fs].uw0 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw0 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.desty )
	{
		fProdTemp = v->vf[i.Fs].fy;
		if ((v->vf[i.Fs].uw1 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw1 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destz )
	{
		fProdTemp = v->vf[i.Fs].fz;
		if ((v->vf[i.Fs].uw2 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw2 = PS2_Float_ToInteger(fProdTemp);
	}
	
	if ( i.destw )
	{
		fProdTemp = v->vf[i.Fs].fw;
		if ((v->vf[i.Fs].uw3 & 0x7fffffff) < 0x70000000) fProdTemp *= c_fMultiplier;
		v->vf[i.Ft].sw3 = PS2_Float_ToInteger(fProdTemp);
	}

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Ft;
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].sx << " vfy=" << v->vf [ i.Ft ].sy << " vfz=" << v->vf [ i.Ft ].sz << " vfw=" << v->vf [ i.Ft ].sw;
#endif
}





// ADDA //

void Execute::ADDA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ft

#ifdef USE_PACKED_INTRINSICS_ADDA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true>(v, i);

#else

	VuUpperOp_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vi[VU::REG_I].u);

#else

	VuUpperOpI_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vi[VU::REG_Q].u);

#else

	VuUpperOpQ_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpX_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpY_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpZ_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::ADDABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_ADDAW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpW_A ( v, i, PS2_Float_Add );

#endif
	
#if defined INLINE_DEBUG_ADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}




// SUBA //

void Execute::SUBA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true>(v, i);

#else

	VuUpperOp_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vi[VU::REG_I].u);

#else

	VuUpperOpI_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vi[VU::REG_Q].u);

#else

	VuUpperOpQ_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpX_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpY_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpZ_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::SUBABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_SUBAW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_ADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpW_A ( v, i, PS2_Float_Sub );

#endif
	
#if defined INLINE_DEBUG_SUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



// MADDA //

void Execute::MADDA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true>(v, i);

#else

	VuUpperOp_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vi[VU::REG_I].u);

#else

	VuUpperOpI_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vi[VU::REG_Q].u);

#else

	VuUpperOpQ_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpX_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpY_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpZ_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MADDABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MADDAW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<false, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpW_A ( v, i, PS2_Float_Madd );

#endif
	
#if defined INLINE_DEBUG_MADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}


// MSUBA //

void Execute::MSUBA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true>(v, i);

#else

	VuUpperOp_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vi[VU::REG_I].u);

#else

	VuUpperOpI_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vi[VU::REG_Q].u);

#else

	VuUpperOpQ_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpX_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpY_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpZ_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MSUBABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

#ifdef USE_PACKED_INTRINSICS_MSUBAW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpW_A ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_MSUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



// MULA //

void Execute::MULA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true>(v, i);

#else

	VuUpperOp_A ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f << " " << v->vi [ 21 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAI_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vi[VU::REG_I].u);

#else

	VuUpperOpI_A ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_MULAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Q=" << hex << v->vi [ 22 ].f << " " << v->vi [ 22 ].u;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAQ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);

	// if div/unit is in use, check if it is done yet
	v->UpdateQ();

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vi[VU::REG_Q].u);

#else

	VuUpperOpQ_A ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_MULAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAX_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpX_A ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_MULAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
	//debug << " XMAC=" << hex << v->FlagSave[(v->iFlagSave_Index + 1) & v->c_lFlag_Delay_Mask].MACFlag;
	//debug << " XSTAT=" << hex << v->FlagSave[(v->iFlagSave_Index + 1) & v->c_lFlag_Delay_Mask].StatusFlag;
#endif
}

void Execute::MULABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAY_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpY_A ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAZ_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpZ_A ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::MULABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef USE_PACKED_INTRINSICS_MULAW_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0>(v, i, &v->vf[i.Ft].vuw[i.bc]);

#else

	VuUpperOpW_A ( v, i, PS2_Float_Mul );

#endif
	
#if defined INLINE_DEBUG_MULAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}







// other upper instructions //

void Execute::CLIP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_CLIP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

#ifdef PERFORM_CLIP_AS_INTEGER
	long lx, ly, lz, lw, lw_plus, lw_minus;
#else
	FloatLong fw_plus, fw_minus, fw;
	float fx, fy, fz;
#endif


	// set the source register(s)
	//v->Set_SrcRegs ( i.Value, i.Fs, i.Ft );

	// uses fs.xyz
	v->Wait_FSrcReg(i.Value, i.Fs);

	// uses ft.w
	//v->Wait_FSrcReg(i.Value, i.Ft);
	v->Wait_FSrcReg(1 << 21, i.Ft);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap


#ifdef PERFORM_CLIP_AS_INTEGER
	lx = v->vf [ i.Fs ].sx;
	ly = v->vf [ i.Fs ].sy;
	lz = v->vf [ i.Fs ].sz;
	lw = v->vf [ i.Ft ].sw;
	
	lw_plus = ( lw & 0x7fffffff );
	
	lx = ( lx >> 31 ) ^ ( lx & 0x7fffffff );
	ly = ( ly >> 31 ) ^ ( ly & 0x7fffffff );
	lz = ( lz >> 31 ) ^ ( lz & 0x7fffffff );
	
	lw_minus = lw_plus ^ 0xffffffff;
	
	v->ClippingFlag.Value = 0;
	if ( lx > lw_plus ) v->ClippingFlag.x_plus0 = 1; else if ( lx < lw_minus ) v->ClippingFlag.x_minus0 = 1;
	if ( ly > lw_plus ) v->ClippingFlag.y_plus0 = 1; else if ( ly < lw_minus ) v->ClippingFlag.y_minus0 = 1;
	if ( lz > lw_plus ) v->ClippingFlag.z_plus0 = 1; else if ( lz < lw_minus ) v->ClippingFlag.z_minus0 = 1;
	
#else
	fx = v->vf [ i.Fs ].fx;
	fy = v->vf [ i.Fs ].fy;
	fz = v->vf [ i.Fs ].fz;
	fw.f = v->vf [ i.Ft ].fw;
	
	ClampValue_f ( fx );
	ClampValue_f ( fy );
	ClampValue_f ( fz );
	ClampValue_f ( fw.f );

	fw_plus.l = fw.l & 0x7fffffff;
	fw_minus.l = fw.l | 0x80000000;
	
	
	
	// read clipping flag
	//v->ClippingFlag.Value = v->vi [ 18 ].u;
	//v->ClippingFlag.Value <<= 6;
	v->ClippingFlag.Value = 0;
	
	//if ( v->vf [ i.Fs ].fx > fw_plus.f ) v->ClippingFlag.x_plus0 = 1; else if ( v->vf [ i.Fs ].fx < fw_minus.f ) v->ClippingFlag.x_minus0 = 1;
	//if ( v->vf [ i.Fs ].fy > fw_plus.f ) v->ClippingFlag.y_plus0 = 1; else if ( v->vf [ i.Fs ].fy < fw_minus.f ) v->ClippingFlag.y_minus0 = 1;
	//if ( v->vf [ i.Fs ].fz > fw_plus.f ) v->ClippingFlag.z_plus0 = 1; else if ( v->vf [ i.Fs ].fz < fw_minus.f ) v->ClippingFlag.z_minus0 = 1;
	
	if ( fx > fw_plus.f ) v->ClippingFlag.x_plus0 = 1; else if ( fx < fw_minus.f ) v->ClippingFlag.x_minus0 = 1;
	if ( fy > fw_plus.f ) v->ClippingFlag.y_plus0 = 1; else if ( fy < fw_minus.f ) v->ClippingFlag.y_minus0 = 1;
	if ( fz > fw_plus.f ) v->ClippingFlag.z_plus0 = 1; else if ( fz < fw_minus.f ) v->ClippingFlag.z_minus0 = 1;
#endif
	


	//if ( !v->Status.SetClip_Flag )
	//{
		v->vi [ VU::REG_CLIPFLAG ].u = ( ( v->vi [ VU::REG_CLIPFLAG ].u << 6 ) | ( v->ClippingFlag.Value & 0x3f ) ) & 0xffffff;
	//}



#if defined INLINE_DEBUG_CLIP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " CLIP=" << hex << v->ClippingFlag.Value << " CFLAG=" << hex << v->vi[VU::REG_CLIPFLAG].u;
#endif
}



void Execute::OPMULA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_OPMULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// ACC_x = fs_y * ft_z
	// ACC_y = fs_z * ft_x
	// ACC_z = fs_x * ft_y

#ifdef USE_PACKED_INTRINSICS_OPMULA_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	EXECUTE_PS2_DOUBLE_MUL_PACKED_AVX2<true, 0x84, 0x60>(v, i);

#else

	VuUpperOp_OPMULA ( v, i, PS2_Float_Mul );

#endif

#if defined INLINE_DEBUG_OPMULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}

void Execute::OPMSUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_OPMSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].f << " y=" << v->dACC [ 1 ].f << " z=" << v->dACC [ 2 ].f << " w=" << v->dACC [ 3 ].f;
#endif

	
	// fd_x = ACC_x - fs_y * ft_z
	// fd_y = ACC_y - fs_x * ft_z
	// fd_z = ACC_z - fs_x * ft_y

#ifdef USE_PACKED_INTRINSICS_OPMSUB_VU

	// set the source register(s)
	v->Wait_FSrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Ft);

	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap
	v->Set_FDstReg_Wait(i.Value, i.Fd);

	EXECUTE_PS2_DOUBLE_MADD_PACKED_AVX2<true, false, 0x84, 0x60>(v, i);

	// the accompanying lower instruction can't modify the same register
	v->LastModifiedRegister = i.Fd;

#else

	VuUpperOp_MSUB ( v, i, PS2_Float_Msub );

#endif
	
#if defined INLINE_DEBUG_OPMULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
	debug << " MAC=" << v->vi [ VU::REG_MACFLAG ].uLo << " STATF=" << v->vi [ VU::REG_STATUSFLAG ].uLo;
#endif
}



void Execute::NOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_NOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
#endif


}






//// *** LOWER instructions *** ////




// branch/jump instructions //

void Execute::B ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_B || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;
}

void Execute::BAL ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_BAL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

// here we probably don't want the link register overwritten
// but can toggle execution of the int delay slot here for testing
#ifdef ENABLE_INTDELAYSLOT_BAL
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		
		// should probably store updated program counter divided by 8 it looks like, unless the PC is already divided by 8??
		//v->vi [ i.it ].uLo = v->PC + 16;
		v->vi [ i.it ].uLo = ( v->PC + 16 ) >> 3;
}

void Execute::IBEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " it=" << v->vi [ i.it ].uLo << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
	{
#if defined INLINE_DEBUG_IBEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif


		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;
	}
}

void Execute::IBNE ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBNE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " it=" << v->vi [ i.it ].uLo << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	if ( v->vi [ i.it ].uLo != v->vi [ i.is ].uLo )
	{
#if defined INLINE_DEBUG_IBNE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;

	}
}

void Execute::IBLTZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBLTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);

	if ( v->vi [ i.is ].sLo < 0 )
	{
#if defined INLINE_DEBUG_IBLTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;
	}
}

void Execute::IBGTZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBGTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);

	if ( v->vi [ i.is ].sLo > 0 )
	{
#if defined INLINE_DEBUG_IBGTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;

		d->Instruction = i;
		//d->cb = r->_cb_Branch;
	}
}

void Execute::IBLEZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBLEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);

	if ( v->vi [ i.is ].sLo <= 0 )
	{
#if defined INLINE_DEBUG_IBLEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;

		d->Instruction = i;
		//d->cb = r->_cb_Branch;
	}
}

void Execute::IBGEZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBGEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << v->vi [ i.is ].uLo;
#endif

	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);

	if ( v->vi [ i.is ].sLo >= 0 )
	{
#if defined INLINE_DEBUG_IBGEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " BRANCH";
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		v->Status.DelaySlot_Valid |= 0x2;


		d->Instruction = i;
		//d->cb = r->_cb_Branch;
	}
}

void Execute::JR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_JR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is(hex)=" << v->vi [ i.is ].uLo;
#endif


	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT_JR
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif
	
	// next instruction is in the branch delay slot
	VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
	v->Status.DelaySlot_Valid |= 0x2;

	d->Instruction = i;
	//d->cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 3-bits of jump address are not zero
	// will clear out lower three bits of address for now
	//d->Data = v->vi [ i.is ].uLo & ~7;
	d->Data = v->vi [ i.is & 0xf ].uLo;
	
}

void Execute::JALR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is(hex)=" << v->vi [ i.is ].uLo;
#endif


	// set the source integer register
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT_JALR
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif
	
	// next instruction is in the branch delay slot
	VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
	v->Status.DelaySlot_Valid |= 0x2;


	d->Instruction = i;
	//d->cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 3-bits of jump address are not zero
	// will clear out lower three bits of address for now
	//d->Data = v->vi [ i.is ].uLo & ~7;
	d->Data = v->vi [ i.is & 0xf ].uLo;
	
	
	//v->vi [ i.it ].uLo = v->PC + 16;
	v->vi [ i.it & 0xf ].uLo = ( v->PC + 16 ) >> 3;
	
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output: it(hex)=" << v->vi [ i.it ].uLo;
#endif
}









// FC/FM/FS instructions //

void Execute::FCEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " CF=" << v->vi [ VU::REG_CLIPFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].ClipFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].ClipFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].ClipFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].ClipFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurClipFlag;
	CurClipFlag = v->Retrieve_ClipFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


	v->vi[1].u = ((CurClipFlag ^ i.Imm24) & 0xffffff) ? 0 : 1;

	
#if defined INLINE_DEBUG_FCEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XCLIP:" << CurClipFlag;
	debug << hex << " Output:" << " vi1=" << v->vi [ 1 ].u;
#endif
}

void Execute::FCAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " CF=" << v->vi [ VU::REG_CLIPFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].ClipFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].ClipFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].ClipFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].ClipFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif


	u32 CurClipFlag;
	CurClipFlag = v->Retrieve_ClipFlag();


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


	v->vi[1].u = ((CurClipFlag & i.Imm24) ? 1 : 0);

	
#if defined INLINE_DEBUG_FCAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XCLIP:" << CurClipFlag;
	debug << hex << " Output:" << " vi1=" << v->vi [ 1 ].u;
#endif
}

void Execute::FCOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " CF=" << v->vi [ VU::REG_CLIPFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].ClipFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].ClipFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].ClipFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].ClipFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurClipFlag;
	CurClipFlag = v->Retrieve_ClipFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	v->vi[1].u = ((((CurClipFlag & 0xffffff) | i.Imm24) == 0xffffff) ? 1 : 0);

#if defined INLINE_DEBUG_FCOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XCLIP:" << CurClipFlag;
	debug << hex << " Output:" << " vi1=" << v->vi [ 1 ].u;
#endif
}

void Execute::FCGET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " CF=" << v->vi [ VU::REG_CLIPFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].ClipFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].ClipFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].ClipFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].ClipFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurClipFlag;
	CurClipFlag = v->Retrieve_ClipFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	v->vi[i.it & 0xf].uLo = CurClipFlag & 0xfff;

	
#if defined INLINE_DEBUG_FCGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XCLIP:" << CurClipFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FCSET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCSET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	u32 CurClipFlag;
	CurClipFlag = v->Retrieve_ClipFlag();

	v->vi[VU::REG_CLIPFLAG].u = i.Imm24;
	v->Status.SetClip_Flag = 1;

#if defined INLINE_DEBUG_FCGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XCLIP:" << CurClipFlag;
	debug << hex << " Output:" << " cf=" << v->vi[VU::REG_CLIPFLAG].u;
#endif
}

void Execute::FMEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].u << " MAC=" << v->vi [ VU::REG_MACFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].MACFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].MACFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].MACFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].MACFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	// set the source integer register
	//v->Set_Int_SrcReg ( i.is + 32 );
	v->Wait_ISrcReg(i.is & 0xf);

	u32 CurMacFlag;
	CurMacFlag = v->Retrieve_MacFlag();

	// note: no need to set destination register since instruction is on the integer pipeline


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	v->vi[i.it & 0xf].u = ((CurMacFlag ^ v->vi[i.is & 0xf].u) & 0xffff) ? 0 : 1;

	
#if defined INLINE_DEBUG_FMEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XMAC:" << CurMacFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FMAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].u << " MAC=" << v->vi [ VU::REG_MACFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].MACFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].MACFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].MACFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].MACFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif


	// set the source integer register
	//v->Set_Int_SrcReg ( i.is + 32 );
	v->Wait_ISrcReg(i.is & 0xf);

	u32 CurMacFlag;
	CurMacFlag = v->Retrieve_MacFlag();

	// note: no need to set destination register since instruction is on the integer pipeline


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	v->vi[i.it & 0xf].u = CurMacFlag & v->vi[i.is & 0xf].u;


#if defined INLINE_DEBUG_FMAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XMAC:" << CurMacFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FMOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].u << " MAC=" << v->vi [ VU::REG_MACFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].MACFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].MACFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].MACFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].MACFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	// set the source integer register
	//v->Set_Int_SrcReg ( i.is + 32 );
	v->Wait_ISrcReg(i.is & 0xf);

	u32 CurMacFlag;
	CurMacFlag = v->Retrieve_MacFlag();

	// note: no need to set destination register since instruction is on the integer pipeline


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	v->vi[i.it & 0xf].u = CurMacFlag | v->vi[i.is & 0xf].u;

	
#if defined INLINE_DEBUG_FMOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XMAC:" << CurMacFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FSEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " STAT=" << v->vi [ VU::REG_STATUSFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].StatusFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].StatusFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].StatusFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].StatusFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurStatusFlag;

	// stat flag will get updated in advance if there is a jump or stat flag check so q-flag should be in there properly
	CurStatusFlag = v->Retrieve_StatFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// check if stat flags needs updating from pending div/sqrt
	//v->CheckQ();
	v->Check_QReg();

	// mix with the current stat flag (which has the actual q div/sqrt flags)
	CurStatusFlag = (CurStatusFlag & 0x3cf) | (v->vi[VU::REG_STATUSFLAG].u & 0xc30);

	v->vi[i.it & 0xf].u = ((CurStatusFlag ^ ((i.Imm15_1 << 11) | (i.Imm15_0))) & 0xfff) ? 0 : 1;

#if defined INLINE_DEBUG_FSEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XSTAT:" << CurStatusFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FSSET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSSET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// check if stat flags needs updating from pending div/sqrt
	// this one doesn't read the flag, just sets it so just need this here
	//v->CheckQ();
	v->Check_QReg();


	v->vi[VU::REG_STATUSFLAG].u = (v->vi[VU::REG_STATUSFLAG].u & 0x3f) | (((i.Imm15_1 << 11) | (i.Imm15_0)) & 0xfc0);
	
	// also need to inform that we set the clip flag (in case the upper instruction is CLIP)
	v->Status.SetStatus_Flag = 1;

}

void Execute::FSAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " STAT=" << v->vi [ VU::REG_STATUSFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].StatusFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].StatusFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].StatusFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].StatusFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurStatusFlag;

	// stat flag will get updated in advance if there is a jump or stat flag check so q-flag should be in there properly
	CurStatusFlag = v->Retrieve_StatFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


	// check if stat flags needs updating from pending div/sqrt
	//v->CheckQ();
	v->Check_QReg();


	// mix with the current stat flag (which has the actual q div/sqrt flags)
	CurStatusFlag = (CurStatusFlag & 0x3cf) | (v->vi[VU::REG_STATUSFLAG].u & 0xc30);

	v->vi[i.it & 0xf].u = CurStatusFlag & (((i.Imm15_1 << 11) | (i.Imm15_0)) & 0xfff);

	
#if defined INLINE_DEBUG_FSAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XSTAT:" << CurStatusFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}

void Execute::FSOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " STAT=" << v->vi [ VU::REG_STATUSFLAG ].u;
	debug << " [0]=" << hex << v->FlagSave[0].StatusFlag << " #" << dec << v->FlagSave[0].ullBusyUntil_Cycle;
	debug << " [1]=" << hex << v->FlagSave[1].StatusFlag << " #" << dec << v->FlagSave[1].ullBusyUntil_Cycle;
	debug << " [2]=" << hex << v->FlagSave[2].StatusFlag << " #" << dec << v->FlagSave[2].ullBusyUntil_Cycle;
	debug << " [3]=" << hex << v->FlagSave[3].StatusFlag << " #" << dec << v->FlagSave[3].ullBusyUntil_Cycle;
#endif

	u32 CurStatusFlag;

	// stat flag will get updated in advance if there is a jump or stat flag check so q-flag should be in there properly
	CurStatusFlag = v->Retrieve_StatFlag();

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


	// check if stat flags needs updating from pending div/sqrt
	//v->CheckQ();
	v->Check_QReg();


	// mix with the current stat flag (which has the actual q div/sqrt flags)
	CurStatusFlag = (CurStatusFlag & 0x3cf) | (v->vi[VU::REG_STATUSFLAG].u & 0xc30);

	v->vi[i.it & 0xf].u = CurStatusFlag | (((i.Imm15_1 << 11) | (i.Imm15_0)) & 0xfff);


#if defined INLINE_DEBUG_FSOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " XSTAT:" << CurStatusFlag;
	debug << hex << " Output:" << " it=" << v->vi [ i.it ].u;
#endif
}



// Integer math //


void Execute::IADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

	// id = is + it

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo + v->vi [ i.it & 0xf ].uLo );
	}
	else
	{
		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo + v->vi [ i.it & 0xf ].uLo;
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC

	v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo + v->vi [ i.it & 0xf ].uLo );

#else
	v->vi [ i.id ].u = v->vi [ i.is ].uLo + v->vi [ i.it ].uLo;
#endif

#endif
	
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

void Execute::VIADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo + v->vi [ i.it & 0xf ].uLo;

#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}


void Execute::IADDI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// it = is + Imm5
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo + ( (s32) i.Imm5 ) );
	}
	else
	{
		v->vi [ i.it & 0xf ].u = v->vi [ i.is & 0xf ].uLo + ( (s32) i.Imm5 );
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	// *TODO* ?? adding with an s32 could put a 32-bit value in i.it ??
	v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo + ( (s32) i.Imm5 ) );
	//v->Set_IntDelaySlot ( i.it & 0xf, ( v->vi [ i.is & 0xf ].uLo + ( (s16) i.Imm5 ) ) | ( v->vi [ i.is & 0xf ].u & 0xffff0000 ) );

#else
	v->vi [ i.it ].u = v->vi [ i.is ].uLo + ( (s32) i.Imm5 );
#endif

#endif
	
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

void Execute::VIADDI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif

		v->vi [ i.it & 0xf ].u = v->vi [ i.is & 0xf ].uLo + ( (s32) i.Imm5 );

#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}


void Execute::IADDIU ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADDIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// it = is + Imm15
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IADDIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
	}
	else
	{
		v->vi [ i.it & 0xf ].u = v->vi [ i.is & 0xf ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
	//v->Set_IntDelaySlot ( i.it & 0xf, ( v->vi [ i.is & 0xf ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) ) | ( v->vi [ i.is & 0xf ].u & 0xffff0000 ) );

#else
	v->vi [ i.it ].u = v->vi [ i.is ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
#endif

#endif
	
#if defined INLINE_DEBUG_IADDIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

void Execute::ISUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// id = is - it
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo - v->vi [ i.it & 0xf ].uLo );
	}
	else
	{
		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo - v->vi [ i.it & 0xf ].uLo;
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo - v->vi [ i.it & 0xf ].uLo );

#else
	v->vi [ i.id ].u = v->vi [ i.is ].uLo - v->vi [ i.it ].uLo;
#endif

#endif
	
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

void Execute::VISUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo - v->vi [ i.it & 0xf ].uLo;

#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

void Execute::ISUBIU ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISUBIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// it = is - Imm15
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_ISUBIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
	}
	else
	{
		v->vi [ i.it & 0xf ].u = v->vi [ i.is & 0xf ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.is & 0xf ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) );
	//v->Set_IntDelaySlot ( i.it & 0xf, ( v->vi [ i.is & 0xf ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) ) | ( v->vi [ i.is & 0xf ].u & 0xffff0000 ) );

#else
	v->vi [ i.it ].u = v->vi [ i.is ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
#endif

#endif
	
#if defined INLINE_DEBUG_ISUBIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}


void Execute::IAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// id = is & it
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo & v->vi [ i.it & 0xf ].uLo );
	}
	else
	{
		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo & v->vi [ i.it & 0xf ].uLo;
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo & v->vi [ i.it & 0xf ].uLo );

#else
	v->vi [ i.id ].u = v->vi [ i.is ].uLo & v->vi [ i.it ].uLo;
#endif

#endif
	
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

void Execute::VIAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo & v->vi [ i.it & 0xf ].uLo;

#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

void Execute::IOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif


	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: no need to set destination register since instruction is on the integer pipeline

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// id = is | it
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo | v->vi [ i.it & 0xf ].uLo );
	}
	else
	{
		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo | v->vi [ i.it & 0xf ].uLo;
	}

#else

#ifdef ENABLE_INTDELAYSLOT_CALC
	
	v->Set_IntDelaySlot ( i.id & 0xf, v->vi [ i.is & 0xf ].uLo | v->vi [ i.it & 0xf ].uLo );

#else
	v->vi [ i.id ].u = v->vi [ i.is ].uLo | v->vi [ i.it ].uLo;
#endif

#endif
	
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}


void Execute::VIOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

		v->vi [ i.id & 0xf ].u = v->vi [ i.is & 0xf ].uLo | v->vi [ i.it & 0xf ].uLo;

#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}



// STORE (to integer) instructions //

void Execute::ISWR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// ISWR itdest, (is)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	

	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: don't want to set destination register until after upper instruction is executed!!!

#ifdef ENABLE_INTDELAYSLOT_ISWR
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	StoreAddress = v->vi [ i.is & 0xf ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vi [ i.it & 0xf ].uLo;
	
	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
		}
	}
	
#if defined INLINE_DEBUG_ISWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << v->vi [ i.is ].uLo;
#endif
}


void Execute::ISW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// ISW itdest, Imm11(is)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	

	// set the source integer register
	//v->Set_Int_SrcRegs ( ( i.is & 0xf ) + 32, ( i.it & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// note: don't want to set destination register until after upper instruction is executed!!!

#ifdef ENABLE_INTDELAYSLOT_ISW
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	StoreAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vi [ i.it & 0xf ].uLo;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vi [ i.it & 0xf ].uLo;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
		}
	}
	
#if defined INLINE_DEBUG_ISW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << ( v->vi [ i.is ].sLo + i.Imm11 );
#endif
}


// STORE (to float) instructions //

void Execute::SQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// SQ fsdest, Imm11(it)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	//v->Set_Int_SrcReg((i.it & 0xf) + 32);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	StoreAddress = ( v->vi [ i.it & 0xf ].sLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
		}
	}
	
#if defined INLINE_DEBUG_SQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << ( v->vi [ i.it ].sLo + i.Imm11 );
#endif
}


void Execute::SQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// SQD fsdest, (--it)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	//v->Set_Int_SrcReg((i.it & 0xf) + 32);
	v->Wait_ISrcReg(i.it & 0xf);

	
	// todo: can set the MAC and STATUS flags registers as being modified also if needed later
	// set the destination register(s)
	// note: can only set this once the pipeline is not stalled since it modifies the pipeline stage bitmap

	// pre-decrement
#ifdef ENABLE_INTDELAYSLOT_SQD_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

#ifdef USE_NEW_RECOMPILE2_SQD

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
		v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.it & 0xf ].uLo - 1 );
		StoreAddress = ( v->vi [ i.it & 0xf ].uLo - 1 ) << 2;
	}
	else
	{
		v->vi [ i.it & 0xf ].uLo--;
		StoreAddress = v->vi [ i.it & 0xf ].uLo << 2;
	}

#else

#ifdef ENABLE_INTDELAYSLOT_SQD_AFTER
	v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.it & 0xf ].uLo - 1 );
	StoreAddress = ( v->vi [ i.it & 0xf ].uLo - 1 ) << 2;

#else
	v->vi [ i.it & 0xf ].uLo--;
	
	StoreAddress = v->vi [ i.it & 0xf ].uLo << 2;
#endif

#endif
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
		}
	}
	
#if defined INLINE_DEBUG_SQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << (v->vi [ i.it ].uLo-1);
#endif
}

void Execute::VSQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_VSQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// VSQD fsdest, (--it)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	//v->Wait_FSrcReg(i.Value, i.Fs);

	//v->Set_Int_SrcReg((i.it & 0xf) + 32);
	//v->Wait_ISrcReg(i.it & 0xf);

	v->vi [ i.it & 0xf ].uLo--;
	StoreAddress = v->vi [ i.it & 0xf ].uLo << 2;

	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
		}
	}
	
#if defined INLINE_DEBUG_VSQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << (v->vi [ i.it ].uLo-1);
#endif
}

void Execute::SQI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs(hex)=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
	debug << dec << " fs(dec)=" << v->vf [ i.Fs ].fx << " " << v->vf [ i.Fs ].fy << " " << v->vf [ i.Fs ].fz << " " << v->vf [ i.Fs ].fw;
#endif

	// SQD fsdest, (it++)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	v->Wait_FSrcReg(i.Value, i.Fs);

	//v->Set_Int_SrcReg((i.it & 0xf) + 32);
	v->Wait_ISrcReg(i.it & 0xf);


#ifdef ENABLE_INTDELAYSLOT_SQI_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	StoreAddress = v->vi [ i.it & 0xf ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
			
		}
	}

#ifdef USE_NEW_RECOMPILE2_SQI

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
		v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.it & 0xf ].uLo + 1 );
	}
	else
	{
		v->vi [ i.it & 0xf ].uLo++;
	}

#else

	// post-increment
#ifdef ENABLE_INTDELAYSLOT_SQI_AFTER
	
	v->Set_IntDelaySlot ( i.it & 0xf, v->vi [ i.it & 0xf ].uLo + 1 );

#else
	v->vi [ i.it & 0xf ].uLo++;
#endif

#endif

#if defined INLINE_DEBUG_SQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << v->vi [ i.it ].uLo;
#endif
}

void Execute::VSQI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs(hex)=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
	debug << dec << " fs(dec)=" << v->vf [ i.Fs ].fx << " " << v->vf [ i.Fs ].fy << " " << v->vf [ i.Fs ].fz << " " << v->vf [ i.Fs ].fw;
#endif

	// SQD fsdest, (it++)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;

	// set the source register(s)
	//v->Set_SrcReg ( i.Value, i.Fs );
	//v->Wait_FSrcReg(i.Value, i.Fs);

	//v->Set_Int_SrcReg((i.it & 0xf) + 32);
	//v->Wait_ISrcReg(i.it & 0xf);

	StoreAddress = v->vi [ i.it & 0xf ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;

	// if writing to TPC, then is should also start VU#1
	if ( !v->Number )
	{
		if ( ( StoreAddress << 2 ) == 0x43a0 )
		{
			if ( i.destx )
			{
				VU1::_VU1->PC = v->vf [ i.Fs ].uw0;
				VU1::_VU1->StartVU ();
			}
			
		}
	}

	v->vi [ i.it & 0xf ].uLo++;

#if defined INLINE_DEBUG_SQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " SA=" << v->vi [ i.it ].uLo;
#endif
}


// LOAD (to integer) instructions //

void Execute::ILWR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ILWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo;
#endif

	// ILWR itdest, (is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	v->Wait_ISrcReg(i.is & 0xf);

	//v->Set_DestReg_Lower((i.it & 0xf) + 32);
	v->Set_IDstReg_Wait(i.it & 0xf);


#ifdef ENABLE_INTDELAYSLOT_ILWR_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif
	

	LoadAddress = v->vi [ i.is & 0xf ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vi [ i.it ].uLo = pVuMem32 [ 0 ];
	if ( i.desty ) v->vi [ i.it ].uLo = pVuMem32 [ 1 ];
	if ( i.destz ) v->vi [ i.it ].uLo = pVuMem32 [ 2 ];
	if ( i.destw ) v->vi [ i.it ].uLo = pVuMem32 [ 3 ];


#if defined INLINE_DEBUG_ILWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " LA=" << v->vi [ i.is ].uLo;
	debug << " Output:" << " it=" << v->vi [ i.it ].uLo;
#endif
}


void Execute::ILW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ILW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo;
#endif

	// ILW itdest, Imm11(is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	v->Wait_ISrcReg(i.is & 0xf);

	//v->Set_DestReg_Lower((i.it & 0xf) + 32);
	v->Set_IDstReg_Wait(i.it & 0xf);


#ifdef ENABLE_INTDELAYSLOT_ILW_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif
	
	LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vi [ i.it ].uLo = pVuMem32 [ 0 ];
	if ( i.desty ) v->vi [ i.it ].uLo = pVuMem32 [ 1 ];
	if ( i.destz ) v->vi [ i.it ].uLo = pVuMem32 [ 2 ];
	if ( i.destw ) v->vi [ i.it ].uLo = pVuMem32 [ 3 ];
	
#if defined INLINE_DEBUG_ILW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " LA=" << ( v->vi [ i.is ].sLo + i.Imm11 );
	debug << " Output:" << " it=" << v->vi [ i.it ].uLo;
#endif
}


// LOAD (to float) instructions //

void Execute::Execute_LoadDelaySlot_MacroMode(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n(LoadMoveDelaySlot)" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
#endif

	// disable the quick delay slot
	v->Status.EnableLoadMoveDelaySlot = 0;

	// only perform transfer if the upper instruction does not write to same register as lower instruction
	// also must check the field, because it only cancels if it writes to the same field as upper instruction?? - no, incorrect
	//if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// load not cancelled //

		if (i.destx) v->vf[i.Ft].uw0 = v->LoadMoveDelayReg.uw0;
		if (i.desty) v->vf[i.Ft].uw1 = v->LoadMoveDelayReg.uw1;
		if (i.destz) v->vf[i.Ft].uw2 = v->LoadMoveDelayReg.uw2;
		if (i.destw) v->vf[i.Ft].uw3 = v->LoadMoveDelayReg.uw3;

		// even if this was already done, it just gets overwritten here
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);
	}

#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " ft(hex)=" << v->vf[i.Ft].uw0 << " " << v->vf[i.Ft].uw1 << " " << v->vf[i.Ft].uw2 << " " << v->vf[i.Ft].uw3;
	debug << dec << " ft(dec)=" << v->vf[i.Ft].fx << " " << v->vf[i.Ft].fy << " " << v->vf[i.Ft].fz << " " << v->vf[i.Ft].fw;
#endif
}

void Execute::Execute_LoadDelaySlot ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n(LoadMoveDelaySlot)" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// disable the quick delay slot
	v->Status.EnableLoadMoveDelaySlot = 0;

	// only perform transfer if the upper instruction does not write to same register as lower instruction
	// also must check the field, because it only cancels if it writes to the same field as upper instruction?? - no, incorrect
	//if ( i.Ft != v->LastModifiedRegister )
	//if ( ! ( v->FlagSave [ v->iFlagSave_Index & VU::c_lFlag_Delay_Mask ].Int_Bitmap & ( 1 << i.Ft ) ) )
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// load not cancelled //

		if ( i.destx ) v->vf [ i.Ft ].uw0 = v->LoadMoveDelayReg.uw0;
		if ( i.desty ) v->vf [ i.Ft ].uw1 = v->LoadMoveDelayReg.uw1;
		if ( i.destz ) v->vf [ i.Ft ].uw2 = v->LoadMoveDelayReg.uw2;
		if ( i.destw ) v->vf [ i.Ft ].uw3 = v->LoadMoveDelayReg.uw3;
		
		// even if this was already done, it just gets overwritten here
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);
	}
	else
	{
#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << " CANCELLED ";
#endif

	}


	// must clear this absolute last
	//v->UpperDest_Bitmap = 0;

#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " ft(hex)=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
	debug << dec << " ft(dec)=" << v->vf [ i.Ft ].fx << " " << v->vf [ i.Ft ].fy << " " << v->vf [ i.Ft ].fz << " " << v->vf [ i.Ft ].fw;
#endif
}

void Execute::LQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " (PRE-DELAY)";
	debug << hex << " is=" << v->vi [ i.is ].uLo;
	debug << hex << " LA=" << ( v->vi [ i.is ].sLo + i.Imm11 );
#endif

	// LQ ftdest, Imm11(is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	LoadAddress = ( v->vi [ i.is & 0xf ].sLo + i.Imm11 ) << 2;
	
#if defined INLINE_DEBUG_LQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " (POST-DELAY)";
	debug << hex << " is=" << v->vi [ i.is ].uLo;
	debug << hex << " LA=" << ( v->vi [ i.is ].sLo + i.Imm11 );
#endif

	pVuMem32 = v->GetMemPtr ( LoadAddress );

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		// check if need to use a temporary variable
		if (v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			// must use a temporary variable to complete move //

			// must do this, in case it loads to a float reg used in src of the upper instruction
			if (i.destx) v->LoadMoveDelayReg.uw0 = pVuMem32[0];
			if (i.desty) v->LoadMoveDelayReg.uw1 = pVuMem32[1];
			if (i.destz) v->LoadMoveDelayReg.uw2 = pVuMem32[2];
			if (i.destw) v->LoadMoveDelayReg.uw3 = pVuMem32[3];

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;

			// TODO: this should only happen AFTER upper instruction is executed probably!!!
			// note: another problem could occur if the destination of upper instruction is source of lower instruction
			if (i.destx) v->vf[i.Ft].uw0 = pVuMem32[0];
			if (i.desty) v->vf[i.Ft].uw1 = pVuMem32[1];
			if (i.destz) v->vf[i.Ft].uw2 = pVuMem32[2];
			if (i.destw) v->vf[i.Ft].uw3 = pVuMem32[3];
		}
		else
		{
			// can simply execute as a simple move //
			
			// TODO: this should only happen AFTER upper instruction is executed probably!!!
			if (i.destx) v->vf[i.Ft].uw0 = pVuMem32[0];
			if (i.desty) v->vf[i.Ft].uw1 = pVuMem32[1];
			if (i.destz) v->vf[i.Ft].uw2 = pVuMem32[2];
			if (i.destw) v->vf[i.Ft].uw3 = pVuMem32[3];
		}
	}

#if defined INLINE_DEBUG_LQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " ft(hex)=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
	debug << dec << " ft(dec)=" << v->vf [ i.Ft ].fx << " " << v->vf [ i.Ft ].fy << " " << v->vf [ i.Ft ].fz << " " << v->vf [ i.Ft ].fw;
#endif
}

void Execute::LQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo;
	debug << hex << " LA=" << (v->vi [ i.is ].uLo-1);
#endif

	// LQD ftdest, (--is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	v->Wait_ISrcReg(i.is & 0xf);


	// pre-decrement
#ifdef ENABLE_INTDELAYSLOT_LQD_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif


	// check if int calc result needs to be output to delay slot or not
	if (v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 10)) /* STATIC_MOVE_TO_INT_DELAY_SLOT */
	{
		v->Set_IntDelaySlot(i.is & 0xf, v->vi[i.is & 0xf].uLo - 1);
		LoadAddress = (v->vi[i.is & 0xf].uLo - 1) << 2;
	}
	else
	{
		v->vi [ i.is & 0xf ].uLo--;
		LoadAddress = v->vi [ i.is & 0xf ].uLo << 2;
	}

	
	pVuMem32 = v->GetMemPtr ( LoadAddress );

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		// check if need to use a temporary variable
		if (v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			// must use a temporary variable to complete move //

			if (i.destx) v->LoadMoveDelayReg.uw0 = pVuMem32[0];
			if (i.desty) v->LoadMoveDelayReg.uw1 = pVuMem32[1];
			if (i.destz) v->LoadMoveDelayReg.uw2 = pVuMem32[2];
			if (i.destw) v->LoadMoveDelayReg.uw3 = pVuMem32[3];

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// can simply execute as a simple move //

			if (i.destx) v->vf[i.Ft].uw0 = pVuMem32[0];
			if (i.desty) v->vf[i.Ft].uw1 = pVuMem32[1];
			if (i.destz) v->vf[i.Ft].uw2 = pVuMem32[2];
			if (i.destw) v->vf[i.Ft].uw3 = pVuMem32[3];
		}
	}
	
#if defined INLINE_DEBUG_LQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
#endif
}

void Execute::VLQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_VLQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo;
	debug << hex << " LA=" << (v->vi [ i.is ].uLo-1);
#endif

	// VLQD ftdest, (--is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	//v->Wait_ISrcReg(i.is & 0xf);

	//v->Set_DestReg_Upper(i.Value, i.Ft);
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	v->vi [ i.is & 0xf ].uLo--;
	LoadAddress = v->vi [ i.is & 0xf ].uLo << 2;

	pVuMem32 = v->GetMemPtr ( LoadAddress );

	if ( i.destx ) v->vf [ i.Ft ].uw0 = pVuMem32 [ 0 ];
	if ( i.desty ) v->vf [ i.Ft ].uw1 = pVuMem32 [ 1 ];
	if ( i.destz ) v->vf [ i.Ft ].uw2 = pVuMem32 [ 2 ];
	if ( i.destw ) v->vf [ i.Ft ].uw3 = pVuMem32 [ 3 ];
	
#if defined INLINE_DEBUG_VLQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
#endif
}


void Execute::LQI(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi[i.is].uLo;
	debug << hex << " LA=" << v->vi[i.is].uLo;
#endif

	// LQI ftdest, (is++)
	// do Imm11 x16

	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT_LQI_BEFORE
	// execute int delay slot immediately
	v->Execute_IntDelaySlot();
#endif

	LoadAddress = v->vi[i.is & 0xf].uLo << 2;

	pVuMem32 = v->GetMemPtr(LoadAddress);

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		// check if need to use a temporary variable
		if (v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			// must use a temporary variable to complete move //

			if (i.destx) v->LoadMoveDelayReg.uw0 = pVuMem32[0];
			if (i.desty) v->LoadMoveDelayReg.uw1 = pVuMem32[1];
			if (i.destz) v->LoadMoveDelayReg.uw2 = pVuMem32[2];
			if (i.destw) v->LoadMoveDelayReg.uw3 = pVuMem32[3];

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// can simply execute as a simple move //

			if (i.destx) v->vf[i.Ft].uw0 = pVuMem32[0];
			if (i.desty) v->vf[i.Ft].uw1 = pVuMem32[1];
			if (i.destz) v->vf[i.Ft].uw2 = pVuMem32[2];
			if (i.destw) v->vf[i.Ft].uw3 = pVuMem32[3];
		}
	}

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) ) /* STATIC_MOVE_TO_INT_DELAY_SLOT */
	{
		v->Set_IntDelaySlot(i.is & 0xf, v->vi[i.is & 0xf].uLo + 1);
	}
	else
	{
		v->vi[i.is & 0xf].uLo++;
	}
	
#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft(hex)=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
	debug << dec << " ft(dec)=" << v->vf [ i.Ft ].fx << " " << v->vf [ i.Ft ].fy << " " << v->vf [ i.Ft ].fz << " " << v->vf [ i.Ft ].fw;
#endif
}

void Execute::VLQI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo;
	debug << hex << " LA=" << v->vi [ i.is ].uLo;
#endif

	// LQI ftdest, (is++)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;

	//v->Set_Int_SrcReg((i.is & 0xf) + 32);
	//v->Wait_ISrcReg(i.is & 0xf);

	// ***TODO*** does upper or lower instruction take priority here ??
	//v->Set_DestReg_Upper(i.Value, i.Ft);
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	LoadAddress = v->vi [ i.is & 0xf ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );

	if ( i.destx ) v->vf [ i.Ft ].uw0 = pVuMem32 [ 0 ];
	if ( i.desty ) v->vf [ i.Ft ].uw1 = pVuMem32 [ 1 ];
	if ( i.destz ) v->vf [ i.Ft ].uw2 = pVuMem32 [ 2 ];
	if ( i.destw ) v->vf [ i.Ft ].uw3 = pVuMem32 [ 3 ];

	v->vi [ i.is & 0xf ].uLo++;

#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft(hex)=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
	debug << dec << " ft(dec)=" << v->vf [ i.Ft ].fx << " " << v->vf [ i.Ft ].fy << " " << v->vf [ i.Ft ].fz << " " << v->vf [ i.Ft ].fw;
#endif
}








// MOVE (to float) instructions //


void Execute::MFP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MFP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
	debug << " P=" << hex << v->vi [ VU::REG_P ].u << " " << dec << v->vi [ VU::REG_P ].f;
	debug << hex << " NextP=" << v->NextP.l << " (float)" << v->NextP.f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// dest ft = P
	
	// the MFP instruction does NOT wait until the EFU unit is done executing
	// also need to check timing of EFU instructions


	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		// need to make sure P register is updated properly first
		//v->UpdateP();
		v->UpdateP_Micro();

		// check if need to use a temporary variable
		if (v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			// must use a temporary variable to complete move //

			if (i.destx) v->LoadMoveDelayReg.uw0 = v->vi[VU::REG_P].u;
			if (i.desty) v->LoadMoveDelayReg.uw1 = v->vi[VU::REG_P].u;
			if (i.destz) v->LoadMoveDelayReg.uw2 = v->vi[VU::REG_P].u;
			if (i.destw) v->LoadMoveDelayReg.uw3 = v->vi[VU::REG_P].u;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// can simply execute as a simple move //

			// TODO: this should only store to the float registers AFTER upper instruction has been executed probably!!!
			// but only if the instruction does not get cancelled by upper instruction
			if (i.destx) v->vf[i.Ft].uw0 = v->vi[VU::REG_P].u;
			if (i.desty) v->vf[i.Ft].uw1 = v->vi[VU::REG_P].u;
			if (i.destz) v->vf[i.Ft].uw2 = v->vi[VU::REG_P].u;
			if (i.destw) v->vf[i.Ft].uw3 = v->vi[VU::REG_P].u;
		}
	}

	// flags affected: none

#if defined INLINE_DEBUG_MFP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::MOVE ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	if ( i.Fs || i.Ft )
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	//v->Set_SrcReg(i.Value, i.Fs);
	v->Wait_FSrcReg(i.Value, i.Fs);

	// dest ft = fs

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		if (v->pStaticInfo[v->Number][(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			if (i.destx) v->LoadMoveDelayReg.uw0 = v->vf[i.Fs].uw0; else v->LoadMoveDelayReg.uw0 = v->vf[i.Ft].uw0;
			if (i.desty) v->LoadMoveDelayReg.uw1 = v->vf[i.Fs].uw1; else v->LoadMoveDelayReg.uw1 = v->vf[i.Ft].uw1;
			if (i.destz) v->LoadMoveDelayReg.uw2 = v->vf[i.Fs].uw2; else v->LoadMoveDelayReg.uw2 = v->vf[i.Ft].uw2;
			if (i.destw) v->LoadMoveDelayReg.uw3 = v->vf[i.Fs].uw3; else v->LoadMoveDelayReg.uw3 = v->vf[i.Ft].uw3;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;
		}
		else
		{
			if (i.destx) v->vf[i.Ft].uw0 = v->vf[i.Fs].uw0;
			if (i.desty) v->vf[i.Ft].uw1 = v->vf[i.Fs].uw1;
			if (i.destz) v->vf[i.Ft].uw2 = v->vf[i.Fs].uw2;
			if (i.destw) v->vf[i.Ft].uw3 = v->vf[i.Fs].uw3;
		}
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	if ( i.Fs || i.Ft )
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}


void Execute::VMOVE ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	if ( i.Fs || i.Ft )
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	//v->Wait_FSrcReg(i.Value, i.Fs);

	// here it doesn't matter if the move is cancelled since if it is upper and lower are going to the same float reg
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	//if ( i.destx ) v->LoadMoveDelayReg.uw0 = v->vf [ i.Fs ].uw0; else v->LoadMoveDelayReg.uw0 = v->vf [ i.Ft ].uw0;
	//if ( i.desty ) v->LoadMoveDelayReg.uw1 = v->vf [ i.Fs ].uw1; else v->LoadMoveDelayReg.uw1 = v->vf [ i.Ft ].uw1;
	//if ( i.destz ) v->LoadMoveDelayReg.uw2 = v->vf [ i.Fs ].uw2; else v->LoadMoveDelayReg.uw2 = v->vf [ i.Ft ].uw2;
	//if ( i.destw ) v->LoadMoveDelayReg.uw3 = v->vf [ i.Fs ].uw3; else v->LoadMoveDelayReg.uw3 = v->vf [ i.Ft ].uw3;

	if (i.destx) v->vf[i.Ft].uw0 = v->vf[i.Fs].uw0;
	if (i.desty) v->vf[i.Ft].uw1 = v->vf[i.Fs].uw1;
	if (i.destz) v->vf[i.Ft].uw2 = v->vf[i.Fs].uw2;
	if (i.destw) v->vf[i.Ft].uw3 = v->vf[i.Fs].uw3;

	// enable the quick delay slot
	//v->Status.EnableLoadMoveDelaySlot = 1;
	
	// put the instruction in the delay slot (for recompiler since it would not be there)
	//v->CurInstLOHI.Lo.Value = i.Value;
	
	// clear last modified register to detect if it should be cancelled
	//v->LastModifiedRegister = 0;


	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	if ( i.Fs || i.Ft )
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::MR32 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// dest ft = fs
	
	u32 temp;

	//v->Set_SrcReg(((i.Value << 1) & (0xe << 21)) | ((i.Value >> 3) & (1 << 21)), i.Fs);
	v->Wait_FSrcReg(((i.Value << 1) & (0xe << 21)) | ((i.Value >> 3) & (1 << 21)), i.Fs);


	// TODO: this should only store to the float registers AFTER upper instruction has been executed probably!!!
	// but only if the instruction does not get cancelled by upper instruction
	
	// must do this or data can get overwritten
	temp = v->vf [ i.Fs ].ux;

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		if (v->pStaticInfo[v->Number][(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			if (i.destx) v->LoadMoveDelayReg.uw0 = v->vf[i.Fs].uy; else v->LoadMoveDelayReg.uw0 = v->vf[i.Ft].uw0;
			if (i.desty) v->LoadMoveDelayReg.uw1 = v->vf[i.Fs].uz; else v->LoadMoveDelayReg.uw1 = v->vf[i.Ft].uw1;
			if (i.destz) v->LoadMoveDelayReg.uw2 = v->vf[i.Fs].uw; else v->LoadMoveDelayReg.uw2 = v->vf[i.Ft].uw2;
			if (i.destw) v->LoadMoveDelayReg.uw3 = temp; else v->LoadMoveDelayReg.uw3 = v->vf[i.Ft].uw3;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;
		}
		else
		{
			if (i.destx) v->vf[i.Ft].ux = v->vf[i.Fs].uy;
			if (i.desty) v->vf[i.Ft].uy = v->vf[i.Fs].uz;
			if (i.destz) v->vf[i.Ft].uz = v->vf[i.Fs].uw;
			if (i.destw) v->vf[i.Ft].uw = temp;
		}
	}

	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

void Execute::VMR32 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_VMR32 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	u32 temp;

	// must do this or data can get overwritten
	temp = v->vf [ i.Fs ].ux;

	//if ( i.destx ) v->LoadMoveDelayReg.uw0 = v->vf [ i.Fs ].uy; else v->LoadMoveDelayReg.uw0 = v->vf [ i.Ft ].uw0;
	//if ( i.desty ) v->LoadMoveDelayReg.uw1 = v->vf [ i.Fs ].uz; else v->LoadMoveDelayReg.uw1 = v->vf [ i.Ft ].uw1;
	//if ( i.destz ) v->LoadMoveDelayReg.uw2 = v->vf [ i.Fs ].uw; else v->LoadMoveDelayReg.uw2 = v->vf [ i.Ft ].uw2;
	//if ( i.destw ) v->LoadMoveDelayReg.uw3 = temp; else v->LoadMoveDelayReg.uw3 = v->vf [ i.Ft ].uw3;

	if (i.destx) v->vf[i.Ft].uw0 = v->vf[i.Fs].uy;
	if (i.desty) v->vf[i.Ft].uw1 = v->vf[i.Fs].uz;
	if (i.destz) v->vf[i.Ft].uw2 = v->vf[i.Fs].uw;
	if (i.destw) v->vf[i.Ft].uw3 = temp;


	// enable the quick delay slot
	//v->Status.EnableLoadMoveDelaySlot = 1;
	
	// put the instruction in the delay slot (for recompiler since it would not be there)
	//v->CurInstLOHI.Lo.Value = i.Value;
	
	// clear last modified register to detect if it should be cancelled
	//v->LastModifiedRegister = 0;


	// flags affected: none

#if defined INLINE_DEBUG_VMR32 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}


void Execute::VMFIR(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_VMFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
	debug << " vi=" << hex << v->vi[i.is].uLo;
#endif

	// dest ft = is

	// TODO: this should only store to the float registers AFTER upper instruction has been executed probably!!!
	// but only if the instruction does not get cancelled by upper instruction

	//v->Set_Int_SrcReg(i.is + 32);
	//v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	//v->Execute_IntDelaySlot();
#endif

	// move is not cancelled //

	// this can only happen if move not cancelled
	//v->Set_DestReg_Upper(i.Value, i.Ft);
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	if (i.destx) v->vf[i.Ft].sw0 = (s32)v->vi[i.is].sLo;
	if (i.desty) v->vf[i.Ft].sw1 = (s32)v->vi[i.is].sLo;
	if (i.destz) v->vf[i.Ft].sw2 = (s32)v->vi[i.is].sLo;
	if (i.destw) v->vf[i.Ft].sw3 = (s32)v->vi[i.is].sLo;

	// flags affected: none

#if defined INLINE_DEBUG_VMFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf[i.Ft].sw0 << " vfy=" << v->vf[i.Ft].sw1 << " vfz=" << v->vf[i.Ft].sw2 << " vfw=" << v->vf[i.Ft].sw3;
#endif

}

void Execute::MFIR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vi=" << hex << v->vi [ i.is ].uLo;
#endif

	// dest ft = is
	
	// TODO: this should only store to the float registers AFTER upper instruction has been executed probably!!!
	// but only if the instruction does not get cancelled by upper instruction

	//v->Set_Int_SrcReg(i.is + 32);
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		if (v->pStaticInfo[v->Number][(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			if (i.destx) v->LoadMoveDelayReg.sw0 = (s32)v->vi[i.is].sLo;
			if (i.desty) v->LoadMoveDelayReg.sw1 = (s32)v->vi[i.is].sLo;
			if (i.destz) v->LoadMoveDelayReg.sw2 = (s32)v->vi[i.is].sLo;
			if (i.destw) v->LoadMoveDelayReg.sw3 = (s32)v->vi[i.is].sLo;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// TODO: destination register should only be set AFTER upper instruction has executed
			if (i.destx) v->vf[i.Ft].sw0 = (s32)v->vi[i.is].sLo;
			if (i.desty) v->vf[i.Ft].sw1 = (s32)v->vi[i.is].sLo;
			if (i.destz) v->vf[i.Ft].sw2 = (s32)v->vi[i.is].sLo;
			if (i.destw) v->vf[i.Ft].sw3 = (s32)v->vi[i.is].sLo;
		}
	}

	// flags affected: none

#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].sw0 << " vfy=" << v->vf [ i.Ft ].sw1 << " vfz=" << v->vf [ i.Ft ].sw2 << " vfw=" << v->vf [ i.Ft ].sw3;
#endif
}


// MOVE (to integer) instructions //

void Execute::MTIR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MTIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
#endif

	// fsf it = fs

	//v->Set_SrcRegBC(i.Value, i.Fs);
	v->Wait_FSrcRegBC(i.Value, i.Fs);

	
	// todo: determine if integer register can be used by branch immediately or if you need int delay slot
#ifdef USE_NEW_RECOMPILE2_INTCALC

	// check if int calc result needs to be output to delay slot or not
	if ( v->pLUT_StaticInfo [ ( v->PC & v->ulVuMem_Mask ) >> 3 ] & ( 1 << 10 ) )
	{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT
	debug << ">INT-DELAY-SLOT";
#endif
		v->Set_IntDelaySlot ( i.it & 0xf, (u16) v->vf [ i.Fs ].vuw [ i.fsf ] );
	}
	else
	{
		v->vi [ i.it & 0xf ].uLo = (u16) v->vf [ i.Fs ].vuw [ i.fsf ];
	}

#else

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
	
	v->Set_IntDelaySlot ( i.it & 0xf, (u16) v->vf [ i.Fs ].vuw [ i.fsf ] );
#else
	// note: this should be ok, since this is a lower instruction and integer instructions are lower instructions
	// note: this instruction happens immediately, with NO stall possible
	v->vi [ i.it ].uLo = (u16) v->vf [ i.Fs ].vuw [ i.fsf ];
#endif

#endif

	// flags affected: none

#if defined INLINE_DEBUG_MTIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

void Execute::VMTIR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MTIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
#endif

		v->vi [ i.it & 0xf ].uLo = (u16) v->vf [ i.Fs ].vuw [ i.fsf ];

#if defined INLINE_DEBUG_MTIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}



// Random Number instructions //

void Execute::VRGET(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_VRGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
	debug << " R=" << hex << v->vi[VU::REG_R].u;
#endif

	// move is not cancelled //

	// this can only happen if move not cancelled
	//v->Set_DestReg_Upper(i.Value, i.Ft);
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	if (i.destx) v->vf[i.Ft].uw0 = v->vi[VU::REG_R].u;
	if (i.desty) v->vf[i.Ft].uw1 = v->vi[VU::REG_R].u;
	if (i.destz) v->vf[i.Ft].uw2 = v->vi[VU::REG_R].u;
	if (i.destw) v->vf[i.Ft].uw3 = v->vi[VU::REG_R].u;

	// flags affected: none

#if defined INLINE_DEBUG_VRGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf[i.Ft].fx << " vfy=" << v->vf[i.Ft].fy << " vfz=" << v->vf[i.Ft].fz << " vfw=" << v->vf[i.Ft].fw;
#endif
}

void Execute::RGET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " R=" << hex << v->vi [ VU::REG_R ].u;
#endif


	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		if (v->pStaticInfo[v->Number][(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			if (i.destx) v->LoadMoveDelayReg.uw0 = v->vi[VU::REG_R].u;
			if (i.desty) v->LoadMoveDelayReg.uw1 = v->vi[VU::REG_R].u;
			if (i.destz) v->LoadMoveDelayReg.uw2 = v->vi[VU::REG_R].u;
			if (i.destw) v->LoadMoveDelayReg.uw3 = v->vi[VU::REG_R].u;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// TODO: destination register should only be set AFTER upper instruction has executed
			if (i.destx) v->vf[i.Ft].uw0 = v->vi[VU::REG_R].u;
			if (i.desty) v->vf[i.Ft].uw1 = v->vi[VU::REG_R].u;
			if (i.destz) v->vf[i.Ft].uw2 = v->vi[VU::REG_R].u;
			if (i.destw) v->vf[i.Ft].uw3 = v->vi[VU::REG_R].u;
		}
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_RGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}


void Execute::VRNEXT(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_VRNEXT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
#endif

	static const unsigned long c_ulRandMask = 0x7ffb18;

	unsigned long bit;
	unsigned long reg;

	reg = v->vi[VU::REG_R].u;
	//bit = __builtin_popcount ( reg & c_ulRandMask ) & 1;
	bit = popcnt32(reg & c_ulRandMask) & 1;
	v->vi[VU::REG_R].u = (0x7f << 23) | ((reg << 1) & 0x007fffff) | bit;

	// move is not cancelled //

	// this can only happen if move not cancelled
	//v->Set_DestReg_Upper(i.Value, i.Ft);
	//v->Set_FDstReg_Wait(i.Value, i.Ft);

	if (i.destx) v->vf[i.Ft].uw0 = v->vi[VU::REG_R].u;
	if (i.desty) v->vf[i.Ft].uw1 = v->vi[VU::REG_R].u;
	if (i.destz) v->vf[i.Ft].uw2 = v->vi[VU::REG_R].u;
	if (i.destw) v->vf[i.Ft].uw3 = v->vi[VU::REG_R].u;

#if defined INLINE_DEBUG_VRNEXT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf[i.Ft].fx << " vfy=" << v->vf[i.Ft].fy << " vfz=" << v->vf[i.Ft].fz << " vfw=" << v->vf[i.Ft].fw;
	debug << " R=" << hex << v->vi[VU::REG_R].u;
#endif
}

void Execute::RNEXT(VU* v, Instruction::Format i)
{
#if defined INLINE_DEBUG_RNEXT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw(8) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO(i.Value).c_str() << "; " << hex << i.Value;
#endif

	static const unsigned long c_ulRandMask = 0x7ffb18;

	unsigned long bit;
	unsigned long reg;

	reg = v->vi[VU::REG_R].u;
	//bit = __builtin_popcount ( reg & c_ulRandMask ) & 1;
	bit = popcnt32(reg & c_ulRandMask) & 1;
	v->vi[VU::REG_R].u = (0x7f << 23) | ((reg << 1) & 0x007fffff) | bit;

	// make sure move not cancelled
	if (!(v->pLUT_StaticInfo[(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 4))) /* STATIC_SKIP_EXEC_LOWER */
	{
		// move is not cancelled //

		// this can only happen if move not cancelled
		//v->Set_DestReg_Upper(i.Value, i.Ft);
		v->Set_FDstReg_Wait(i.Value, i.Ft);

		if (v->pStaticInfo[v->Number][(v->PC & v->ulVuMem_Mask) >> 3] & (1 << 5)) /* STATIC_COMPLETE_FLOAT_MOVE */
		{
			if (i.destx) v->LoadMoveDelayReg.uw0 = v->vi[VU::REG_R].u;
			if (i.desty) v->LoadMoveDelayReg.uw1 = v->vi[VU::REG_R].u;
			if (i.destz) v->LoadMoveDelayReg.uw2 = v->vi[VU::REG_R].u;
			if (i.destw) v->LoadMoveDelayReg.uw3 = v->vi[VU::REG_R].u;

			// enable the quick delay slot
			v->Status.EnableLoadMoveDelaySlot = 1;

			// put the instruction in the delay slot (for recompiler since it would not be there)
			v->CurInstLOHI.Lo.Value = i.Value;

			// clear last modified register to detect if it should be cancelled
			v->LastModifiedRegister = 0;
		}
		else
		{
			// TODO: destination register should only be set AFTER upper instruction has executed
			if (i.destx) v->vf[i.Ft].uw0 = v->vi[VU::REG_R].u;
			if (i.desty) v->vf[i.Ft].uw1 = v->vi[VU::REG_R].u;
			if (i.destz) v->vf[i.Ft].uw2 = v->vi[VU::REG_R].u;
			if (i.destw) v->vf[i.Ft].uw3 = v->vi[VU::REG_R].u;
		}
	}
	
#if defined INLINE_DEBUG_RNEXT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
	debug << " R=" << hex << v->vi [ VU::REG_R ].u;
#endif
}

void Execute::RINIT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RINIT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fsfsf=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
#endif

	//cout << "\nhps2x64: ERROR: VU: Instruction not implemented: RINIT";
	
	v->vi [ VU::REG_R ].u = ( 0x7f << 23 ) | ( v->vf [ i.Fs ].vuw [ i.fsf ] & 0x7fffff );
	
	// seed random number generator for now
	//srand ( v->vi [ VU::REG_R ].u );

#if defined INLINE_DEBUG_RINIT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: R=" << hex << v->vi [ VU::REG_R ].u;
#endif
}

void Execute::RXOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RXOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fsfsf=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
	debug << " R=" << hex << v->vi [ VU::REG_R ].u;
#endif

	//cout << "\nhps2x64: ERROR: VU: Instruction not implemented: RXOR";
	
	v->vi [ VU::REG_R ].u = ( 0x7f << 23 ) | ( ( v->vf [ i.Fs ].vuw [ i.fsf ] ^ v->vi [ VU::REG_R ].u ) & 0x7fffff );
	
#if defined INLINE_DEBUG_RXOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: R=" << hex << v->vi [ VU::REG_R ].u;
#endif
}










// X instructions //

void Execute::XGKICK ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XGKICK || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif


	// set the source integer register
	//v->Set_Int_SrcReg ( ( i.is & 0xf ) + 32 );
	v->Wait_ISrcReg(i.is & 0xf);


#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

#if defined INLINE_DEBUG_XGKICK || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " vi=" << hex << v->vi [ i.is ].uLo;
#endif

#ifdef ENABLE_XGKICK_WAIT
	// if another path is in progress, then don't start transfer yet
	if ( ( GPU::_GPU->GIFRegs.STAT.APATH ) && ( GPU::_GPU->GIFRegs.STAT.APATH != 1 ) )
	{
		// make sure path 3 is not in an interruptible image transfer
		if ( ( GPU::_GPU->GIFRegs.STAT.APATH != 3 ) || ( !GPU::_GPU->GIFRegs.STAT.IMT ) || ( GPU::_GPU->GIFTag0 [ 3 ].FLG != 2 ) )
		{
			// path 1 in queue
			GPU::_GPU->GIFRegs.STAT.P1Q = 1;
			
			// waiting
			v->Status.XgKick_Wait = 1;
			
			// wait until transfer via the other path is complete
			//v->NextPC = v->PC;
			
			// ??
			v->XgKick_Address = v->vi [ i.is & 0xf ].uLo;
			return;
		}
	}
#endif

#ifdef ENABLE_XGKICK_TIMING

	if ( v->bXgKickActive )
	{
		v->Flush_XgKick ();
	}

	// clear the path 1 count before start writing block
	GPU::_GPU->ulTransferCount [ 1 ] = 0;

	v->ullXgKickCycle = v->CycleCount;
	v->bXgKickActive = 1;

#else

	// if there is an xgkick instruction in progress, then need to complete it immediately
	if ( v->Status.XgKickDelay_Valid )
	{
		// looks like this is only supposed to write one 128-bit value to PATH1
		// no, this actually writes an entire gif packet to path1
		// the address should only be a maximum of 10-bits, so must mask
		//GPU::Path1_WriteBlock ( v->VuMem64, v->XgKick_Address & 0x3ff );
		v->Execute_XgKick ();
		
		// the previous xgkick is done with
		//v->Status.XgKickDelay_Valid = 0;
	}
	
	// now transferring via path 1
	// but, not if running from another thread or putting data into a buffer
	// probably best to comment this one out for now
	if ( !GPU::_GPU->GIFRegs.STAT.APATH )
	{
		GPU::_GPU->GIFRegs.STAT.APATH = 1;
	}
	else if ( GPU::_GPU->GIFRegs.STAT.APATH )
	{
#ifdef ENABLE_APATH_ALERTS
		//if ( GPU::_GPU->GIFRegs.STAT.APATH == 1 )
		//{
		//	cout << "\nALERT: PATH1 running while PATH1 is already running\n";
		//}
		//else
		if ( GPU::_GPU->GIFRegs.STAT.APATH == 2 )
		{
			cout << "\nALERT: PATH1 running while PATH2 is already running\n";
		}
		else if ( GPU::_GPU->GIFRegs.STAT.APATH == 3 )
		{
			if ( ( !GPU::_GPU->GIFRegs.STAT.IMT ) || ( GPU::_GPU->GIFTag0 [ 3 ].FLG != 2 ) )
			{
				cout << "\nALERT: PATH1 running while PATH3 is already running (IMT=" << GPU::_GPU->GIFRegs.STAT.IMT << " FLG=" << GPU::_GPU->GIFTag0 [ 3 ].FLG << ")\n";
			}
		}
#endif
	}

	// need to execute xgkick instruction, but it doesn't execute completely immediately and vu keeps going
	// so for now will delay execution for an instruction or two
	v->Status.XgKickDelay_Valid = 0x2;
#endif

	// no longer waiting
	v->Status.XgKick_Wait = 0;
	
	// path 1 no longer in queue
	GPU::_GPU->GIFRegs.STAT.P1Q = 0;

	v->XgKick_Address = v->vi [ i.is & 0xf ].uLo;
}


void Execute::XTOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XTOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " TOP(hex)=" << v->iVifRegs.TOP;
#endif

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	// it = TOP
	// this instruction is VU1 only
	v->vi [ i.it & 0xf ].uLo = v->iVifRegs.TOP & 0x3ff;

#if defined INLINE_DEBUG_XTOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " it(hex)=" << v->vi [ i.it ].uLo;
#endif
}

void Execute::XITOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XITOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " ITOP(hex)=" << v->iVifRegs.ITOP;
#endif

#ifdef ENABLE_INTDELAYSLOT
	// execute int delay slot immediately
	v->Execute_IntDelaySlot ();
#endif

	if ( !v->Number )
	{
		// VU0 //
		v->vi [ i.it & 0xf ].uLo = v->iVifRegs.ITOP & 0xff;
	}
	else
	{
		// VU1 //
		v->vi [ i.it & 0xf ].uLo = v->iVifRegs.ITOP & 0x3ff;
	}
	
#if defined INLINE_DEBUG_XITOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " it(hex)=" << v->vi [ i.it ].uLo;
#endif
}





// WAIT instructions //

void Execute::WAITP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_WAITP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->Wait_PReg();
}

void Execute::WAITQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_WAITQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->Wait_QReg();
}



// lower float math //

void Execute::DIV ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " fs=" << ( (float&) v->vf [ i.Fs ].vuw [ i.fsf ] );
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	// todo: update cycle count when div has to wait for a previous div (where Q=NextQ)

	static const u64 c_CycleTime = 7;

	float fs;
	float ft;
	

	//v->Set_SrcRegBC ( i.ftf, i.Ft );
	//v->Add_SrcRegBC ( i.fsf, i.Fs );
	v->Wait_FSrcRegBC(i.ftf, i.Ft);
	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for q-register pipeline to be ready
	v->Wait_QReg();

	
	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->vi [ 22 ].f = PS2_Float_Div ( fs, ft, & v->vi [ 16 ].sLo );
	v->NextQ.f = PS2_Float_Div ( fs, ft, (unsigned short*) & v->NextQ_Flag);
	v->QBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
	debug << dec << " NextQ=" << v->NextQ.f;
#endif
}

void Execute::RSQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " fs=" << ( (float&) v->vf [ i.Fs ].vuw [ i.fsf ] );
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	static const u64 c_CycleTime = 13;

	float fs;
	float ft;
	

	//v->Set_SrcRegBC ( i.ftf, i.Ft );
	//v->Add_SrcRegBC ( i.fsf, i.Fs );
	v->Wait_FSrcRegBC(i.ftf, i.Ft);
	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for q-register pipeline to be ready
	v->Wait_QReg();
	
	
	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->vi [ 22 ].f = PS2_Float_RSqrt ( fs, ft, & v->vi [ 16 ].sLo );
	v->NextQ.f = PS2_Float_RSqrt ( fs, ft, (unsigned short*) & v->NextQ_Flag );
	v->QBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_RSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
	debug << dec << " NextQ=" << v->NextQ.f;
#endif
}

void Execute::SQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	static const u64 c_CycleTime = 7;

	float ft;
	

	//v->Set_SrcRegBC ( i.ftf, i.Ft );
	v->Wait_FSrcRegBC(i.ftf, i.Ft);

	// wait for q-register pipeline to be ready
	v->Wait_QReg();

	
	ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->vi [ 22 ].f = PS2_Float_Sqrt ( ft, & v->vi [ 16 ].sLo );
	v->NextQ.f = PS2_Float_Sqrt ( ft, (unsigned short*) & v->NextQ_Flag );
	v->QBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_SQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
	debug << dec << " NextQ=" << v->NextQ.f;
#endif
}




// External unit //

void Execute::EATAN ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATAN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 53/54 cycles
	static const u64 c_CycleTime = 53;	//54;
	
	// I'll make c0 = pi/4
	static const long c_fC0 = 0x3f490fdb;
	
	static const long c_fC1 = 0x3f7ffff5;
	static const long c_fC2 = 0xbeaaa61c;
	static const long c_fC3 = 0x3e4c40a6;
	static const long c_fC4 = 0xbe0e6c63;
	static const long c_fC5 = 0x3dc577df;
	static const long c_fC6 = 0xbd6501c4;
	static const long c_fC7 = 0x3cb31652;
	static const long c_fC8 = 0xbb84d7e7;

	u16 NoFlags;
	
	float fs;
	float fy1, fy2, fy3, fy4, fy5;
	float ft, ft2, ft3, ft5, ft7, ft9, ft11, ft13, ft15;

	
	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	
	// get (x-1)/(x+1)
	ft = PS2_Float_Div ( PS2_Float_Sub ( fs, 1.0f, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fs, 1.0f, 0, & NoFlags, & NoFlags ), & NoFlags );
	
	ft2 = PS2_Float_Mul ( ft, ft, 0, & NoFlags, & NoFlags );
	ft3 = PS2_Float_Mul ( ft, ft2, 0, & NoFlags, & NoFlags );
	ft5 = PS2_Float_Mul ( ft3, ft2, 0, & NoFlags, & NoFlags );
	ft7 = PS2_Float_Mul ( ft5, ft2, 0, & NoFlags, & NoFlags );
	ft9 = PS2_Float_Mul ( ft7, ft2, 0, & NoFlags, & NoFlags );
	ft11 = PS2_Float_Mul ( ft9, ft2, 0, & NoFlags, & NoFlags );
	ft13 = PS2_Float_Mul ( ft11, ft2, 0, & NoFlags, & NoFlags );
	ft15 = PS2_Float_Mul ( ft13, ft2, 0, & NoFlags, & NoFlags );
	
	// (pi/4) + ( c1 * ft + c2 * ft^3 + c3 * ft^5 + c4 * ft^7 + c5 * ft^9 + c6 * ft^11 + c7 * ft^13 + c8 * ft^15 )
	fy1 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC1, ft, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC2, ft3, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy2 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC3, ft5, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC4, ft7, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy3 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC5, ft9, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC6, ft11, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy4 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC7, ft13, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC8, ft15, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy5 = PS2_Float_Add ( PS2_Float_Add ( fy1, fy2, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fy3, fy4, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	v->NextP.f = PS2_Float_Add ( fy5, (float&) c_fC0, 0, & NoFlags, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_EATAN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::EATANxy ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATANXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 53/54 cycles
	static const u64 c_CycleTime = 53;
	
	// I'll make c0 = pi/4
	static const long c_fC0 = 0x3f490fdb;
	
	static const long c_fC1 = 0x3f7ffff5;
	static const long c_fC2 = 0xbeaaa61c;
	static const long c_fC3 = 0x3e4c40a6;
	static const long c_fC4 = 0xbe0e6c63;
	static const long c_fC5 = 0x3dc577df;
	static const long c_fC6 = 0xbd6501c4;
	static const long c_fC7 = 0x3cb31652;
	static const long c_fC8 = 0xbb84d7e7;

	u16 NoFlags;
	
	float fx, fy;
	float fy1, fy2, fy3, fy4, fy5;
	float ft, ft2, ft3, ft5, ft7, ft9, ft11, ft13, ft15;

	
	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fx = v->vf [ i.Fs ].fx;
	fy = v->vf [ i.Fs ].fy;
	
	// get (y-1)/(x+y)
	ft = PS2_Float_Div ( PS2_Float_Sub ( fy, fx, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fx, fy, 0, & NoFlags, & NoFlags ), & NoFlags );
	
	ft2 = PS2_Float_Mul ( ft, ft, 0, & NoFlags, & NoFlags );
	ft3 = PS2_Float_Mul ( ft, ft2, 0, & NoFlags, & NoFlags );
	ft5 = PS2_Float_Mul ( ft3, ft2, 0, & NoFlags, & NoFlags );
	ft7 = PS2_Float_Mul ( ft5, ft2, 0, & NoFlags, & NoFlags );
	ft9 = PS2_Float_Mul ( ft7, ft2, 0, & NoFlags, & NoFlags );
	ft11 = PS2_Float_Mul ( ft9, ft2, 0, & NoFlags, & NoFlags );
	ft13 = PS2_Float_Mul ( ft11, ft2, 0, & NoFlags, & NoFlags );
	ft15 = PS2_Float_Mul ( ft13, ft2, 0, & NoFlags, & NoFlags );
	
	// (pi/4) + ( c1 * ft + c2 * ft^3 + c3 * ft^5 + c4 * ft^7 + c5 * ft^9 + c6 * ft^11 + c7 * ft^13 + c8 * ft^15 )
	fy1 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC1, ft, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC2, ft3, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy2 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC3, ft5, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC4, ft7, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy3 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC5, ft9, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC6, ft11, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy4 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC7, ft13, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC8, ft15, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy5 = PS2_Float_Add ( PS2_Float_Add ( fy1, fy2, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fy3, fy4, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	v->NextP.f = PS2_Float_Add ( fy5, (float&) c_fC0, 0, & NoFlags, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_EATANXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::EATANxz ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATANXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 53/54 cycles
	static const u64 c_CycleTime = 53;
	
	// I'll make c0 = pi/4
	static const long c_fC0 = 0x3f490fdb;
	
	static const long c_fC1 = 0x3f7ffff5;
	static const long c_fC2 = 0xbeaaa61c;
	static const long c_fC3 = 0x3e4c40a6;
	static const long c_fC4 = 0xbe0e6c63;
	static const long c_fC5 = 0x3dc577df;
	static const long c_fC6 = 0xbd6501c4;
	static const long c_fC7 = 0x3cb31652;
	static const long c_fC8 = 0xbb84d7e7;

	u16 NoFlags;
	
	float fx, fz;
	float fy1, fy2, fy3, fy4, fy5;
	float ft, ft2, ft3, ft5, ft7, ft9, ft11, ft13, ft15;

	
	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fx = v->vf [ i.Fs ].fx;
	fz = v->vf [ i.Fs ].fz;
	
	// get (z-x)/(x+z)
	ft = PS2_Float_Div ( PS2_Float_Sub ( fz, fx, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fx, fz, 0, & NoFlags, & NoFlags ), & NoFlags );
	
	ft2 = PS2_Float_Mul ( ft, ft, 0, & NoFlags, & NoFlags );
	ft3 = PS2_Float_Mul ( ft, ft2, 0, & NoFlags, & NoFlags );
	ft5 = PS2_Float_Mul ( ft3, ft2, 0, & NoFlags, & NoFlags );
	ft7 = PS2_Float_Mul ( ft5, ft2, 0, & NoFlags, & NoFlags );
	ft9 = PS2_Float_Mul ( ft7, ft2, 0, & NoFlags, & NoFlags );
	ft11 = PS2_Float_Mul ( ft9, ft2, 0, & NoFlags, & NoFlags );
	ft13 = PS2_Float_Mul ( ft11, ft2, 0, & NoFlags, & NoFlags );
	ft15 = PS2_Float_Mul ( ft13, ft2, 0, & NoFlags, & NoFlags );
	
	// (pi/4) + ( c1 * ft + c2 * ft^3 + c3 * ft^5 + c4 * ft^7 + c5 * ft^9 + c6 * ft^11 + c7 * ft^13 + c8 * ft^15 )
	fy1 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC1, ft, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC2, ft3, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy2 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC3, ft5, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC4, ft7, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy3 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC5, ft9, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC6, ft11, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy4 = PS2_Float_Add ( PS2_Float_Mul ( (float&) c_fC7, ft13, 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( (float&) c_fC8, ft15, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	fy5 = PS2_Float_Add ( PS2_Float_Add ( fy1, fy2, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( fy3, fy4, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	v->NextP.f = PS2_Float_Add ( fy5, (float&) c_fC0, 0, & NoFlags, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_EATANXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::EEXP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EEXP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 43/44 cycles
	static const u64 c_CycleTime = 43;	//44;
	
	//static const float c_fC0 = (const float&) ((const long&) 0x3f800000);
	static const long c_fC1 = 0x3e7fffa8;
	static const long c_fC2 = 0x3d0007f4;
	static const long c_fC3 = 0x3b29d3ff;
	static const long c_fC4 = 0x3933e553;
	static const long c_fC5 = 0x36b63510;
	static const long c_fC6 = 0x353961ac;

	u16 NoFlags;
	
	float fs;
	float fx2, fx3, fx4, fx5, fx6;
	float fy, fy2, fy4;
	float ft1, ft2, ft3, ft4, ft5, ft6;
	float fc12, fc34, fc56, fc1234, fc123456;

	
	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();



	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	
	fx2 = PS2_Float_Mul ( fs, fs, 0, & NoFlags, & NoFlags );
	fx3 = PS2_Float_Mul ( fs, fx2, 0, & NoFlags, & NoFlags );
	fx4 = PS2_Float_Mul ( fs, fx3, 0, & NoFlags, & NoFlags );
	fx5 = PS2_Float_Mul ( fs, fx4, 0, & NoFlags, & NoFlags );
	fx6 = PS2_Float_Mul ( fs, fx5, 0, & NoFlags, & NoFlags );
	
	// 1 / ( ( 1 + c1 * x + c2 * x^2 + c3 * x^3 + c4 * x^4 + c5 * x^5 + c6 * x^6 ) ^ 4 )
	// *TODO*: should use fast floating point ?
	//fy = 1.0f + ( c_fC1 * fs ) + ( c_fC2 * fx2 ) + ( c_fC3 * fx3 ) + ( c_fC4 * fx4 ) + ( c_fC5 * fx5 ) + ( c_fC6 * fx6 );

	ft1 = PS2_Float_Mul ( (float&) c_fC1, fs, 0, & NoFlags, & NoFlags );
	ft2 = PS2_Float_Mul ( (float&) c_fC2, fx2, 0, & NoFlags, & NoFlags );
	ft3 = PS2_Float_Mul ( (float&) c_fC3, fx3, 0, & NoFlags, & NoFlags );
	ft4 = PS2_Float_Mul ( (float&) c_fC4, fx4, 0, & NoFlags, & NoFlags );
	ft5 = PS2_Float_Mul ( (float&) c_fC5, fx5, 0, & NoFlags, & NoFlags );
	ft6 = PS2_Float_Mul ( (float&) c_fC6, fx6, 0, & NoFlags, & NoFlags );
	
	fc12 = PS2_Float_Add ( ft1, ft2, 0, & NoFlags, & NoFlags );
	fc34 = PS2_Float_Add ( ft3, ft4, 0, & NoFlags, & NoFlags );
	fc56 = PS2_Float_Add ( ft5, ft6, 0, & NoFlags, & NoFlags );
	fc1234 = PS2_Float_Add ( fc12, fc34, 0, & NoFlags, & NoFlags );
	fc123456 = PS2_Float_Add ( fc1234, fc56, 0, & NoFlags, & NoFlags );
	fy = PS2_Float_Add ( 1.0f, fc123456, 0, & NoFlags, & NoFlags );
	
	fy2 = PS2_Float_Mul ( fy, fy, 0, & NoFlags, & NoFlags );
	fy4 = PS2_Float_Mul ( fy2, fy2, 0, & NoFlags, & NoFlags );
	v->NextP.f = PS2_Float_Div ( 1.0f, fy4, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_EEXP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::ESIN ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESIN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 28/29 cycles
	static const u64 c_CycleTime = 28;	//29;
	
	static const long c_fC1 = 0x3f800000;
	static const long c_fC2 = 0xbe2aaaa4;
	static const long c_fC3 = 0x3c08873e;
	static const long c_fC4 = 0xb94fb21f;
	static const long c_fC5 = 0x362e9c14;

	u16 NoFlags;
	
	float fs;
	float fx2, fx3, fx5, fx7, fx9;
	float fc1, fc2, fc3, fc4, fc5, fc12, fc34, fc1234, fy;

	
	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	
	fx2 = PS2_Float_Mul ( fs, fs, 0, & NoFlags, & NoFlags );
	fx3 = PS2_Float_Mul ( fs, fx2, 0, & NoFlags, & NoFlags );
	fx5 = PS2_Float_Mul ( fx3, fx2, 0, & NoFlags, & NoFlags );
	fx7 = PS2_Float_Mul ( fx5, fx2, 0, & NoFlags, & NoFlags );
	fx9 = PS2_Float_Mul ( fx7, fx2, 0, & NoFlags, & NoFlags );
	
	fc1 = PS2_Float_Mul ( (float&) c_fC1, fs, 0, & NoFlags, & NoFlags );
	fc2 = PS2_Float_Mul ( (float&) c_fC2, fx3, 0, & NoFlags, & NoFlags );
	fc3 = PS2_Float_Mul ( (float&) c_fC3, fx5, 0, & NoFlags, & NoFlags );
	fc4 = PS2_Float_Mul ( (float&) c_fC4, fx7, 0, & NoFlags, & NoFlags );
	fc5 = PS2_Float_Mul ( (float&) c_fC5, fx9, 0, & NoFlags, & NoFlags );
	
	fc12 = PS2_Float_Add ( fc1, fc2, 0, & NoFlags, & NoFlags );
	fc34 = PS2_Float_Add ( fc3, fc4, 0, & NoFlags, & NoFlags );
	fc1234 = PS2_Float_Add ( fc12, fc34, 0, & NoFlags, & NoFlags );
	fy = PS2_Float_Add ( fc1234, fc5, 0, & NoFlags, & NoFlags );
	
	// c1 * x + c2 * x^3 + c3 * x^5 + c4 * x^7 + c5 * x^9
	// *TODO*: should use fast floating point ?
	//v->NextP.f = ( c_fC1 * fs ) + ( c_fC2 * fx3 ) + ( c_fC3 * fx5 ) + ( c_fC4 * fx7 ) + ( c_fC5 * fx9 );
	v->NextP.f = fy;
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ESIN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::ERSQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " i.Fs=" << dec << i.Fs << " i.fsf=" << i.fsf << " vf[i.Fs]=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	//cout << "\nhps2x64: ERROR: VU: Instruction not implemented: ERSQRT";
	
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	u16 NoFlags;
	float fs;


	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];

#if defined INLINE_DEBUG_ERSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " (float)" << fs;
#endif
	
	v->NextP.f = PS2_Float_RSqrt ( 1.0f, fs, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ERSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::ERCPR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERCPR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " i.Fs=" << dec << i.Fs << " i.fsf=" << i.fsf << " vf[i.Fs]=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	u16 NoFlags;
	float fs;
	

	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	
#if defined INLINE_DEBUG_ERCPR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " fs= (float)" << fs;
#endif

	v->NextP.f = PS2_Float_Div ( 1.0f, fs, & NoFlags );

	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ERCPR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}


void Execute::ESQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " i.Fs=" << dec << i.Fs << " i.fsf=" << i.fsf << " vf[i.Fs]=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	
	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	u16 NoFlags;
	float fs;
	

	v->Wait_FSrcRegBC(i.fsf, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	
#if defined INLINE_DEBUG_ESQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " (float)" << fs;
#endif

	v->NextP.f = PS2_Float_Sqrt ( fs, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ESQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::ESADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif


	// 10/11 cycles
	static const u64 c_CycleTime = 10;

	u16 NoFlags;
	

	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	v->NextP.f = PS2_Float_Add ( PS2_Float_Add ( PS2_Float_Mul ( v->vf [ i.Fs ].fx, v->vf [ i.Fs ].fx, 0, & NoFlags, & NoFlags ) , PS2_Float_Mul ( v->vf [ i.Fs ].fy, v->vf [ i.Fs ].fy, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( v->vf [ i.Fs ].fz, v->vf [ i.Fs ].fz, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ESADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}

void Execute::ERSADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERSADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	u16 NoFlags;


	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	v->NextP.f = PS2_Float_Div ( 1.0f, PS2_Float_Add ( PS2_Float_Add ( PS2_Float_Mul ( v->vf [ i.Fs ].fx, v->vf [ i.Fs ].fx, 0, & NoFlags, & NoFlags ) , PS2_Float_Mul ( v->vf [ i.Fs ].fy, v->vf [ i.Fs ].fy, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( v->vf [ i.Fs ].fz, v->vf [ i.Fs ].fz, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), & NoFlags );

	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ERSADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}


void Execute::ESUM ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESUM || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	
	// 11/12 cycles
	static const u64 c_CycleTime = 11;

	u16 NoFlags;
	

	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	v->NextP.f = PS2_Float_Add ( PS2_Float_Add ( v->vf [ i.Fs ].fx, v->vf [ i.Fs ].fy, 0, & NoFlags, & NoFlags ), PS2_Float_Add ( v->vf [ i.Fs ].fz, v->vf [ i.Fs ].fw, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ESUM || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}


void Execute::ELENG ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ELENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	
	// 17/18 cycles
	static const u64 c_CycleTime = 17;

	u16 NoFlags;
	

	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	v->NextP.f = PS2_Float_Sqrt ( PS2_Float_Add ( PS2_Float_Add ( PS2_Float_Mul ( v->vf [ i.Fs ].fx, v->vf [ i.Fs ].fx, 0, & NoFlags, & NoFlags ) , PS2_Float_Mul ( v->vf [ i.Fs ].fy, v->vf [ i.Fs ].fy, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( v->vf [ i.Fs ].fz, v->vf [ i.Fs ].fz, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ELENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}


void Execute::ERLENG ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERLENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << hex << " (before)NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
	debug << dec << " PBusyUntil=" << v->PBusyUntil_Cycle;
#endif

	
	// 23/24 cycles
	static const u64 c_CycleTime = 23;

	u16 NoFlags;
	

	v->Wait_FSrcReg(i.Value, i.Fs);

	// wait for p-register pipeline to be ready
	v->Wait_PReg();


	v->NextP.f = PS2_Float_RSqrt ( 1.0f, PS2_Float_Add ( PS2_Float_Add ( PS2_Float_Mul ( v->vf [ i.Fs ].fx, v->vf [ i.Fs ].fx, 0, & NoFlags, & NoFlags ) , PS2_Float_Mul ( v->vf [ i.Fs ].fy, v->vf [ i.Fs ].fy, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), PS2_Float_Mul ( v->vf [ i.Fs ].fz, v->vf [ i.Fs ].fz, 0, & NoFlags, & NoFlags ), 0, & NoFlags, & NoFlags ), & NoFlags );
	v->PBusyUntil_Cycle = v->CycleCount + c_CycleTime;
	
#if defined INLINE_DEBUG_ERLENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_EXT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " (after) NextP=" << v->NextP.l << " (float)" << v->NextP.f << " CurrentP=" << v->vi [ VU::REG_P ].u << " (float)" << v->vi [ VU::REG_P ].f;
#endif
}





const Execute::Function Execute::FunctionList []
{
	Execute::INVALID,
	
	// VU macro mode instructions //
	
	//Execute::COP2
	//Execute::QMFC2_NI, Execute::QMFC2_I, Execute::QMTC2_NI, Execute::QMTC2_I, Execute::LQC2, Execute::SQC2,
	//Execute::CALLMS, Execute::CALLMSR,
	
	// upper instructions //
	
	Execute::ABS,
	Execute::ADD, Execute::ADDi, Execute::ADDq, Execute::ADDBCX, Execute::ADDBCY, Execute::ADDBCZ, Execute::ADDBCW,
	Execute::ADDA, Execute::ADDAi, Execute::ADDAq, Execute::ADDABCX, Execute::ADDABCY, Execute::ADDABCZ, Execute::ADDABCW,
	Execute::CLIP,
	Execute::FTOI0, Execute::FTOI4, Execute::FTOI12, Execute::FTOI15,
	Execute::ITOF0, Execute::ITOF4, Execute::ITOF12, Execute::ITOF15,
	
	Execute::MADD, Execute::MADDi, Execute::MADDq, Execute::MADDBCX, Execute::MADDBCY, Execute::MADDBCZ, Execute::MADDBCW,
	Execute::MADDA, Execute::MADDAi, Execute::MADDAq, Execute::MADDABCX, Execute::MADDABCY, Execute::MADDABCZ, Execute::MADDABCW,
	Execute::MAX, Execute::MAXi, Execute::MAXBCX, Execute::MAXBCY, Execute::MAXBCZ, Execute::MAXBCW,
	Execute::MINI, Execute::MINIi, Execute::MINIBCX, Execute::MINIBCY, Execute::MINIBCZ, Execute::MINIBCW,
	
	Execute::MSUB, Execute::MSUBi, Execute::MSUBq, Execute::MSUBBCX, Execute::MSUBBCY, Execute::MSUBBCZ, Execute::MSUBBCW,
	Execute::MSUBA, Execute::MSUBAi, Execute::MSUBAq, Execute::MSUBABCX, Execute::MSUBABCY, Execute::MSUBABCZ, Execute::MSUBABCW,
	Execute::MUL, Execute::MULi, Execute::MULq, Execute::MULBCX, Execute::MULBCY, Execute::MULBCZ, Execute::MULBCW,
	Execute::MULA, Execute::MULAi, Execute::MULAq, Execute::MULABCX, Execute::MULABCY, Execute::MULABCZ, Execute::MULABCW,
	Execute::NOP, Execute::OPMSUB, Execute::OPMULA,
	Execute::SUB, Execute::SUBi, Execute::SUBq, Execute::SUBBCX, Execute::SUBBCY, Execute::SUBBCZ, Execute::SUBBCW,
	Execute::SUBA, Execute::SUBAi, Execute::SUBAq, Execute::SUBABCX, Execute::SUBABCY, Execute::SUBABCZ, Execute::SUBABCW,
	
	// lower instructions //
	
	Execute::DIV,
	Execute::IADD, Execute::IADDI, Execute::IAND,
	Execute::ILWR,
	Execute::IOR, Execute::ISUB,
	Execute::ISWR,
	Execute::LQD, Execute::LQI,
	Execute::MFIR, Execute::MOVE, Execute::MR32, Execute::MTIR,
	Execute::RGET, Execute::RINIT, Execute::RNEXT,
	Execute::RSQRT,
	Execute::RXOR,
	Execute::SQD, Execute::SQI,
	Execute::SQRT,
	Execute::WAITQ,

	// instructions not in macro mode //
	
	Execute::B, Execute::BAL,
	Execute::FCAND, Execute::FCEQ, Execute::FCGET, Execute::FCOR, Execute::FCSET,
	Execute::FMAND, Execute::FMEQ, Execute::FMOR,
	Execute::FSAND, Execute::FSEQ, Execute::FSOR, Execute::FSSET,
	Execute::IADDIU,
	Execute::IBEQ, Execute::IBGEZ, Execute::IBGTZ, Execute::IBLEZ, Execute::IBLTZ, Execute::IBNE,
	Execute::ILW,
	Execute::ISUBIU, Execute::ISW,
	Execute::JALR, Execute::JR,
	Execute::LQ,
	Execute::MFP,
	Execute::SQ,
	Execute::WAITP,
	Execute::XGKICK, Execute::XITOP, Execute::XTOP,

	// External Unit //

	Execute::EATAN, Execute::EATANxy, Execute::EATANxz, Execute::EEXP, Execute::ELENG, Execute::ERCPR, Execute::ERLENG, Execute::ERSADD,
	Execute::ERSQRT, Execute::ESADD, Execute::ESIN, Execute::ESQRT, Execute::ESUM
};





