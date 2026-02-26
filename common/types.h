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

//#pragma once
#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#if defined(_WIN32) || defined(_WIN64)
#include <intrin.h>
#else
#include <x86intrin.h>
#include <cpuid.h>
#endif

//#define GCC_COMPILER (defined(__GNUC__) && !defined(__clang__))
//#if GCC_COMPILER
#if (defined(__GNUC__) && !defined(__clang__))

#define ALIGN2		__attribute__ ((aligned (2)))
#define ALIGN4		__attribute__ ((aligned (4)))
#define ALIGN8		__attribute__ ((aligned (8)))
#define ALIGN16		__attribute__ ((aligned (16)))
#define ALIGN32		__attribute__ ((aligned (32)))
#define ALIGN64		__attribute__ ((aligned (64)))
#define ALIGN128	__attribute__ ((aligned (128)))

#else

#define ALIGN2		__declspec(align(2))
#define ALIGN4		__declspec(align(4))
#define ALIGN8		__declspec(align(8))
#define ALIGN16		__declspec(align(16))
#define ALIGN32		__declspec(align(32))
#define ALIGN64		__declspec(align(64))
#define ALIGN128	__declspec(align(128))

#endif


// function interfaces

typedef void (*funcVoid) ( void );
typedef uint32_t (*funcVoid1) ( void* );
typedef uint32_t (*funcVoid2) ( void (*)() );
typedef uint32_t (*func2) ();


// types

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

union FloatLong
{
	float f;
	int32_t l;
	uint32_t lu;
} ALIGN4;

union DoubleLong
{
	double d;
	int64_t l;
	uint64_t lu;
} ALIGN8;


struct Reg32
{
	union
	{
		u32 u;
		s32 s;
		
		float f;
		
		u8 Bytes [ 4 ];
		
		struct
		{
			u8 ub8;
			u8 filler0;
			u16 filler1;
		};
		
		struct
		{
			s8 sb8;
			s8 filler2;
			s16 filler3;
		};
		
		struct
		{
			u16 uLo;
			u16 uHi;
		};
		
		struct
		{
			s16 sLo;
			s16 sHi;
		};
		
	};
} ALIGN4;


struct Reg64
{
	union
	{
		struct
		{
			u32 uLo;
			u32 uHi;
		};
		
		struct
		{
			s32 sLo;
			s32 sHi;
		};

		u64 uValue;
		s64 sValue;
		
		u32 uArray [ 2 ];
		s32 sArray [ 2 ];
	};
} ALIGN8;


struct Reg128
{
	union
	{
		struct
		{
			u64 u;
			u64 filler64_0;
		};
		
		struct
		{
			s64 s;
			s64 filler64_1;
		};
		
		struct
		{
			float fx, fy, fz, fw;
		};
		
		struct
		{
			u32 ux, uy, uz, uw;
		};
		
		struct
		{
			s32 sx, sy, sz, sw;
		};
		
		struct
		{
			u64 uq0, uq1;
		};
		
		struct
		{
			s64 sq0, sq1;
		};
		
		struct
		{
			u32 uw0, uw1, uw2, uw3;
		};
		
		struct
		{
			s32 sw0, sw1, sw2, sw3;
		};
		
		struct
		{
			u16 uh0, uh1, uh2, uh3, uh4, uh5, uh6, uh7;
		};
		
		struct
		{
			s16 sh0, sh1, sh2, sh3, sh4, sh5, sh6, sh7;
		};
		
		struct
		{
			u8 ub0, ub1, ub2, ub3, ub4, ub5, ub6, ub7, ub8, ub9, ub10, ub11, ub12, ub13, ub14, ub15;
		};
		
		struct
		{
			s8 sb0, sb1, sb2, sb3, sb4, sb5, sb6, sb7, sb8, sb9, sb10, sb11, sb12, sb13, sb14, sb15;
		};
		
		struct
		{
			u64 uLo;
			u64 uHi;
		};
		
		struct
		{
			s64 sLo;
			s64 sHi;
		};
		
		u8 vub [ 16 ];
		u16 vuh [ 8 ];
		u32 vuw [ 4 ];
		u64 vuq [ 2 ];
		
		s8 vsb [ 16 ];
		s16 vsh [ 8 ];
		s32 vsw [ 4 ];
		s64 vsq [ 2 ];
		
		
		
	};
} ALIGN16;



// need to convert some previously 32-bit registers to 128-bit registers so they can be memory mapped.
// this structure should fix that
struct Reg128x
{
	union
	{
		struct
		{
			u32 u;
			u32 xu;
			u64 filler64_0;
		};
		
		struct
		{
			s32 s;
			s32 xs;
			s64 filler64_1;
		};
		
		struct
		{
			float f, f1, f2, f3;
		};
		
		struct
		{
			float fx, fy, fz, fw;
		};
		
		struct
		{
			u32 ux, uy, uz, uw;
		};
		
		struct
		{
			s32 sx, sy, sz, sw;
		};
		
		struct
		{
			u64 uq0, uq1;
		};
		
		struct
		{
			s64 sq0, sq1;
		};
		
		struct
		{
			u32 uw0, uw1, uw2, uw3;
		};
		
		struct
		{
			s32 sw0, sw1, sw2, sw3;
		};
		
		struct
		{
			u16 uh0, uh1, uh2, uh3, uh4, uh5, uh6, uh7;
		};
		
		struct
		{
			s16 sh0, sh1, sh2, sh3, sh4, sh5, sh6, sh7;
		};
		
		struct
		{
			u8 ub0, ub1, ub2, ub3, ub4, ub5, ub6, ub7, ub8, ub9, ub10, ub11, ub12, ub13, ub14, ub15;
		};
		
		struct
		{
			s8 sb0, sb1, sb2, sb3, sb4, sb5, sb6, sb7, sb8, sb9, sb10, sb11, sb12, sb13, sb14, sb15;
		};
		
		struct
		{
			u16 uLo;
			u16 xuLo;
			u32 xxuLo;
			u64 uHi;
		};
		
		struct
		{
			s16 sLo;
			s16 xsLo;
			s32 xxsLo;
			s64 sHi;
		};
		
		u8 vub [ 16 ];
		u16 vuh [ 8 ];
		u32 vuw [ 4 ];
		u64 vuq [ 2 ];
		
		s8 vsb [ 16 ];
		s16 vsh [ 8 ];
		s32 vsw [ 4 ];
		s64 vsq [ 2 ];
		
		
		
	};
} ALIGN16;



union MultiPtr
{
	u8* b8;
	u16* b16;
	u32* b32;
	u64* b64;
} ALIGN8;


inline static bool check_avx512_support() {
#if defined(_WIN32) || defined(_WIN64)
	int cpuInfo[4];
	__cpuidex(cpuInfo, 7, 0);
	return (cpuInfo[1] & (1 << 16)) != 0;
#else
	unsigned int eax, ebx, ecx, edx;
	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	return (ebx & (1 << 16)) != 0;
#endif
}

inline static bool check_avx2_support() {
#if defined(_WIN32) || defined(_WIN64)
	int cpuInfo[4];
	__cpuidex(cpuInfo, 7, 0);
	return (cpuInfo[1] & (1 << 5)) != 0; // Check for bit 5 in EBX for AVX2
#else
	unsigned int eax, ebx, ecx, edx;
	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	return (ebx & (1 << 5)) != 0;
#endif
}


//inline static bool check_avx2_support() {
//	unsigned int eax, ebx, ecx, edx;
//	if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
//		return (ebx & (1 << 5)) != 0; // Check for bit 5 in EBX for AVX2
//	}
//	return false;
//}

//#define GCC_OR_CLANG_COMPILER (defined(__GNUC__) || defined(__clang__))
//#if GCC_OR_CLANG_COMPILER
#if (defined(__GNUC__) || defined(__clang__))

#define clz32(x) __builtin_clz(x)
#define ctz32(x) __builtin_ctz(x)
#define clz64(x) __builtin_clz(x)
#define ctz64(x) __builtin_ctz(x)
#define popcnt32(x) __builtin_popcount(x)
#define popcnt64(x) __builtin_popcount(x)

#else

#include <intrin.h>

static unsigned long __inline popcnt32(uint32_t x)
{
	return _mm_popcnt_u32(x);
	//x -= ((x >> 1) & 0x55555555);
	//x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	//x = (((x >> 4) + x) & 0x0f0f0f0f);
	//x += (x >> 8);
	//x += (x >> 16);
	//return x & 0x0000003f;
}
static unsigned long __inline popcnt64(uint64_t x)
{
	return _mm_popcnt_u64(x);
	//x -= ((x >> 1) & 0x55555555);
	//x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	//x = (((x >> 4) + x) & 0x0f0f0f0f);
	//x += (x >> 8);
	//x += (x >> 16);
	//return x & 0x0000003f;
}
static unsigned long __inline clz32(uint32_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return 32 - popcnt32(x);
}
static unsigned long __inline clz64(uint64_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return 64 - popcnt64(x);
}
static unsigned long __inline ctz32(uint32_t x)
{
	return popcnt32((x & -x) - 1);
}
static unsigned long __inline ctz64(uint64_t x)
{
	return popcnt64((x & -x) - 1);
}

#endif


template <typename T, size_t N>
inline constexpr size_t countof(T(&)[N]) noexcept {
	return N;
}


// this is for any code used from mame/mess
//typedef long long INT64;
//typedef long INT32;
//typedef short INT16;
//typedef char INT8;
//typedef unsigned long long UINT64;
//typedef unsigned long UINT32;
//typedef unsigned short UINT16;
//typedef unsigned char UINT8;

// this is for code ripped from pcsx2
#if !defined(__linux__) || defined(__MINGW32__)
typedef unsigned long uint;
#endif
#define __fi inline
#define __ri inline
#define __aligned16 ALIGN16

#endif	// end #ifndef __TYPES_H__
