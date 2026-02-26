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
#ifndef __PS2FLOAT_H__
#define __PS2FLOAT_H__

#include "types.h"
#include <math.h>

#include <iostream>
using namespace std;





// todo
// 1. fix sqrt, rsqrt, div


#define ENABLE_FLAG_CHECK
//#define USE_DOUBLE
//#define CALC_WITH_INF


// use double-precision to implement PS2 floating point
#define USE_DOUBLE_MATH_ADD
#define USE_DOUBLE_MATH_SUB
#define USE_DOUBLE_MATH_MUL
#define USE_DOUBLE_MATH_MADD
#define USE_DOUBLE_MATH_MSUB
#define USE_DOUBLE_MATH_DIV
#define USE_DOUBLE_MATH_SQRT
#define USE_DOUBLE_MATH_RSQRT



// use integer math when more simplistic
//#define USE_INTEGER_MATH
//#define USE_INTEGER_MATH_ADD
//#define USE_INTEGER_MATH_SUB
//#define USE_INTEGER_MATH_MUL
//#define USE_INTEGER_MATH_MADD
//#define USE_INTEGER_MATH_MSUB


// this is for testing negation of the msub multiply result
#define NEGATE_MSUB_MULTIPLY_RESULT


#define ENABLE_FLAG_CHECK_MADD


//#define IGNORE_DENORMAL_MAXMIN
#define COMPUTE_MINMAX_AS_INTEGER


// chops off last bit before multiply of one of the multiply operands
//#define ENABLE_MUL_PRECISION_CHOP


// should signed zero set the signed flag too, or just the zero flag ??
#define DISABLE_SIGN_FLAG_WHEN_ZERO


// on multiply underflow store the accumulator or previous value for MADD/MSUB ??
//#define ENABLE_PREVIOUS_VALUE_ON_MULTIPY_UNDERFLOW


// output of these defines should be LONG LONG
#define CvtLongLongToDouble(ll) ( ( (ll) & 0x7f800000 ) ? ( ( ( (( ( (ll) >> 23 ) & 0xff ) + 896) ) << 52 ) | ( ( (ll) & 0x7fffff ) << 29 ) | ( ( (ll) & 0x80000000ULL ) << 32 ) /* | ( 0x1fffffffULL ) */ ) : ( ( (ll) & 0x80000000 ) << 32 ) )
#define CvtPS2FloatToDouble(f) ( CvtLongLongToDouble( ((long long&)f) ) )
#define CvtLongToDouble(l) ( CvtLongLongToDouble( ((long long)l) ) )

// output of these defines should be LONG
//#define CvtLongLongToPS2Float(ll) ( ( ll & 0x7fffffffffffffffLL ) ? ( ( ( ( ( (ll) >> 52 ) & 0x7ff ) - 896 ) << 23 ) | ( ( (ll) >> 29 ) & 0x7fffff ) | ( ( (ll) >> 32 ) & 0x80000000 ) ) : ( ( (ll) >> 32 ) & 0x80000000 ) )
#define CvtLongLongToPS2Float(ll) ( ( ll & 0x7ff0000000000000ULL ) ? ( ( ( ( ( (ll) >> 52 ) & 0x7ff ) - 896 ) << 23 ) | ( ( (ll) >> 29 ) & 0x7fffff ) | ( ( (ll) >> 32 ) & 0x80000000 ) ) : ( ( (ll) >> 32 ) & 0x80000000 ) )
#define CvtDoubleToPS2Float(d) ( CvtLongLongToPS2Float( ((unsigned long long&)d) ) )

// these are absolute max values and represent a PS2 overflow
// so if result is greater OR equal to these, then you have an overflow
#define PosMaxLongLongPS2 ((896LL+256LL)<<52)
#define NegMaxLongLongPS2 ( ((896LL+256LL)<<52) | ( 1LL << 63 ) )

// these are absolute min values and represent zero before calculation and underflow after calculation
// so if result is strictly below these (but NOT equal to), then you have an underflow
#define PosMinLongLongPS2 ((896LL+1LL)<<52)
#define NegMinLongLongPS2 ( ((896LL+1LL)<<52) | ( 1LL << 63 ) )


namespace PS2Float
{
	//static const long long c_llPS2DoubleMax = ( 1151ULL << 52 );
	static const long long c_llPS2DoubleMax = ( 1152ULL << 52 );
	static const long long c_llPS2DoubleMin = ( 897ULL << 52 );
	//static const long long c_llPS2DoubleMin = ( 1ULL << 63 ) | ( 1151ULL << 52 );
	static const long long c_llDoubleINF = ( 0x7ffULL << 52 );
	static const long long c_llDoubleAbsMask = -( 1LL << 63 );
	static const long long c_llDoubleSignMask = -( 1LL << 63 );
	

	static const long c_lFloatINF = ( 0xff << 23 );
	static const long c_lFloat_SignExpMask = ( 0x1ff << 23 );
	static const long c_lFloat_SignMask = 0x80000000;
	static const long c_lFloat_ExpMask = ( 0xff << 23 );
	static const long c_lFloat_MantissaMask = 0x7fffff;
	
	static const long c_lFloat_ValidMax = 0x7f7fffff;
	
	// difference between max ps2 float and next value down
	static const long c_lFloatMaxDiff = 0x73800000;
	
	
	// maximum positive 64-bit integer
	static const long long c_llMaxPosInt64 = 0x7fffffffffffffffLL;
	
	// maximum positive 32-bit integer
	static const long c_lMaxPosInt32 = 0x7fffffff;
	
	
	
	static const long long c_llPS2DoubleMaxVal = CvtLongToDouble ( c_lMaxPosInt32 );
	
	
	
	// these 3 functions are from: http://cottonvibes.blogspot.com/2010/07/testing-for-nan-and-infinity-values.html
	//inline bool isNaN(float x) { return x != x; }
	//inline bool isInf(float x) { return fabs(x) == numeric_limits<float>::infinity(); }
	static inline bool isNaNorInf(float x) { return ((long&)x & 0x7fffffff) >= 0x7f800000; }
	//inline bool isNaN_d (double x) { return x != x; }
	//inline bool isInf_d (double x) { return fabs(x) == numeric_limits<float>::infinity(); }
	static inline bool isNaNorInf_d (double x) { return ((long long&)x & 0x7fffffffffffffffULL) >= 0x7ff0000000000000ULL; }
	
	
	inline static void FlushPS2DoubleToZero ( double& d )
	{
		//static const long long c_ullMinPS2Value = PosMinLongLongPS2;

#ifdef USE_INTEGER_MATH
		long long ll;
		
		ll = (long long&) d;
		
		// if less than the min value, then flush to zero
		if ( ( ll & c_llMaxPosInt64 ) < c_llPS2DoubleMin ) ll &= c_llDoubleSignMask;
		
		d = (double&) ll;
#else
		// if less than the min value, then flush to zero
		if ( d < ( (double&) c_llPS2DoubleMin ) ) d = 0.0L;
#endif
	}
	
	
	// forces double to be within the range ps2 max and min
	inline static void PS2DblMaxMin ( double& d )
	{
		long long ll;
		
		ll = (long long&) d;
		
		// if less than the min value, then flush to zero
		if ( ( ll & c_llMaxPosInt64 ) < c_llPS2DoubleMin ) ll &= c_llDoubleSignMask;
		
		// greater than max value, then set to +/-max
		else if ( ( ll & c_llMaxPosInt64 ) >= c_llPS2DoubleMax ) ll = ( ll & c_llDoubleSignMask ) | c_llPS2DoubleMaxVal;
		
		d = (double&) ll;
	}


	inline static void SetFlagsOnResultDF_d ( DoubleLong& dResult, DoubleLong dlACC, DoubleLong dProd, DoubleLong dSum, double fd, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		// set STICKY sign flag based on multiplication
		*StatusFlag |= ( dProd.l >> 63 ) & 0x80;
		
		// set ALL sign flag based on ADD
		*StatusFlag |= ( dSum.l >> 63 ) & 0x82;
		*MACFlag |= ( dSum.l >> 63 ) & ( 1 << ( index + 4 ) );
		
		// set STICKY zero based on MUL
		if ( ( dProd.l & c_llMaxPosInt64 ) < c_llPS2DoubleMin )
		{
			// set zero sticky flag based on multiply result
			*StatusFlag |= 0x040;
			
			// set STICKY underflow based on MUL
			if ( dProd.l & c_llMaxPosInt64 )
			{
				// *note* ONLY set underflow sticky flag on underflow
				*StatusFlag |= 0x100;
				
#ifdef ENABLE_PREVIOUS_VALUE_ON_MULTIPY_UNDERFLOW
				// set preliminary result to previous value ?? (or to ACC ??)
				dResult.d = fd;
#else
				// set preliminary result to ACC ??
				dResult.d = dlACC.d;
#endif
			}
		}
		
		// set ALL zero based on ADD
		if ( ( dSum.l & c_llMaxPosInt64 ) < c_llPS2DoubleMin )
		{
			// set all zero flags
			*StatusFlag |= 0x041;
			*MACFlag |= ( 1 << ( index + 0 ) );
		}

		// set ALL overflow based on MUL
		if ( ( dProd.l & c_llMaxPosInt64 ) >= c_llPS2DoubleMax )
		{
			// overflow in MADD //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// set result to +/-max (must use sign of addition result)
			// actually, should be sign of multiplication (MSUB has sign change)
			dResult.l = ( dProd.l & c_llDoubleSignMask ) | c_llPS2DoubleMaxVal;
		}
		
		// otherwise check if ACC already in overflow state
		//else if ( ( dlACC.l & c_llMaxPosInt64 ) >= CvtLongToDouble ( c_lMaxPosInt32 ) )
		else if ( ( dlACC.l & c_llMaxPosInt64 ) >= CvtLongToDouble ( c_lFloat_ExpMask ) )
		{
			// overflow in MADD //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// set result to +/-max (here just return ACC)
			//dResult.l = dlACC.l;
			dResult.l = ( dlACC.l & c_llDoubleSignMask ) | c_llPS2DoubleMaxVal;
		}
		
		// also overflow if the add has overflow
		else if ( ( dSum.l & c_llMaxPosInt64 ) >= c_llPS2DoubleMax )
		{
			// overflow in MADD //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// set result to +/-max (must use sign of addition result)
			dResult.l = ( dSum.l & c_llDoubleSignMask ) | c_llPS2DoubleMaxVal;
		}
		
	}
	
	
	inline static void SetFlagsOnResult_d ( double& dResult, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		long long llResult;
		
		llResult = (long long&) dResult;
		
		// set zero flags
		//if ( ! ( llResult & c_llMaxPosInt64 ) )
		if ( ( llResult & c_llMaxPosInt64 ) < c_llPS2DoubleMin )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#ifdef DISABLE_SIGN_FLAG_WHEN_ZERO
		else
#endif
		// set sign flags
		// note: only set this strictly when calculation result is negative, not on signed zero
		//if ( Result < 0.0f )
		if ( llResult < 0 )
		{

			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}


		// check for overflow
		if ( ( llResult & c_llMaxPosInt64 ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// set to +/-max
			//lResult = c_lFloat_MantissaMask | lResult;
			llResult = ( llResult & c_llDoubleSignMask ) | CvtLongLongToDouble( 0x7fffffffLL );
			dResult = (double&) llResult;
		}
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( ( llResult & c_llMaxPosInt64 ) < c_llPS2DoubleMin ) && ( llResult & c_llMaxPosInt64 ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set to +/-0
			//lResult = c_lFloat_SignMask & lResult;
			llResult &= c_llDoubleSignMask;
			dResult = (double&) llResult;
			
			// since value has been set to zero, set the zero flag
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}


		
		/*
		//----------------------
		// set sign flags
		if ( Dd < 0.0L )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		

		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( ( (long long&) Dd ) & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( ( (long long&) Dd ) & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( ( (long long&) Dd ) & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Dd == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
		*/
	}
	
	
	inline static void SetFlagsOnResult_f ( float& Result, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		long lResult;
		
		lResult = (long&) Result;
		
		// set zero flags
		//if ( Result == 0.0f )
		if ( ! ( lResult & 0x7fffffff ) )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#ifdef DISABLE_SIGN_FLAG_WHEN_ZERO
		else
#endif
		// set sign flags
		// note: only set this strictly when calculation result is negative, not on signed zero
		//if ( Result < 0.0f )
		if ( lResult < 0 )
		{

			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		// check for overflow
		if ( ( lResult & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// set to +/-max
			lResult = c_lFloat_MantissaMask | lResult;
		}
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( lResult & c_lFloat_MantissaMask ) && !( lResult & c_lFloat_ExpMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set to +/-0
			lResult = c_lFloat_SignMask & lResult;
			
			// since value has been set to zero, set the zero flag
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
		
		Result = (float&) lResult;
	}



	//inline static void FlushDenormal_d ( double& d1 )
	//{
	//}

	inline static void FlushDenormal_f ( float& f1 )
	{
		long lValue;
		
		lValue = (long&) f1;
		
		// check if exponent is zero
		if ( ! ( lValue & c_lFloat_ExpMask ) )
		{
			// exponent is zero, so flush value to zero //
			
			lValue &= c_lFloat_SignMask;
			
			f1 = (float&) lValue;
		}
	}

	
	inline static void FlushDenormal2_f ( float& f1, float& f2 )
	{
		FlushDenormal_f ( f1 );
		FlushDenormal_f ( f2 );
	}

	
	// flush denormals to zero and convert negative float to comparable negative integer
	inline static long FlushConvertToComparableInt_f ( float& f1 )
	{
		long lf1;
		
		lf1 = (long&) f1;
		
		// if exponent is zero, then set it to zero
		if ( ! ( lf1 & c_lFloat_ExpMask ) )
		{
			lf1 = 0;
		}
		
		// if negative, then take two's complement of positive value
		if ( lf1 < 0 )
		{
			lf1 = -( lf1 & 0x7fffffff );
		}
		
		return lf1;
	}
	
	
	inline static void ClampValue_d ( double& d )
	{
		long long ll;
		if ( isNaNorInf_d ( d ) )
		{
			ll = ( c_llPS2DoubleMax | ( c_llDoubleAbsMask & ((long long&) d) ) );
			d = (double&) ll;
		}
	}
	
	inline static void ClampValue_f ( float& f )
	{
		long l;
		
		// ps2 treats denormals as zero
		FlushDenormal_f ( f );
		
		// check for not a number and infinity
		if ( isNaNorInf ( f ) )
		{
#ifdef CALC_WITH_INF
			//Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
			//fs = (float&) ( c_lFloat_SignExpMask & ( c_lFloatINF | (long&) fs ) );
			// or could just clear mantissa, or set to next valid value
			l = ( c_lFloat_SignExpMask & ( (long&) f ) );
#else
			// or set to next valid value
			l = ( c_lFloat_ValidMax | ( c_lFloat_SignMask & ( (long&) f ) ) );
#endif

			f = (float&) l;
		}
		
	}


	inline static void ClampValue2_d ( double& d1, double& d2 )
	{
		ClampValue_d ( d1 );
		ClampValue_d ( d2 );
	}

	inline static void ClampValue2_f ( float& f1, float& f2 )
	{
		ClampValue_f ( f1 );
		ClampValue_f ( f2 );
		
		// should also flush denormals to zero
		// this is handle by clamp function now
		//FlushDenormal2_f ( f1, f2 );
	}
	
	
	inline static double CvtPS2FloatToDbl ( float f1 )
	{
		DoubleLong d;
		d.l = CvtPS2FloatToDouble(f1);
		return d.d;
	}

	inline static float CvtDblToPS2Float ( double d1 )
	{
		FloatLong f;
		f.l = CvtDoubleToPS2Float(d1);
		return f.f;
	}


	// add float as integer
	inline static long addfloat_int(long fs, long ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag)
	{
		static const long c_iHiddenBits = 1;
		long es, et, ed;
		long ss, st, sd;

		long temp;

		long ms, mt, md;

		long ext;

		long sign;
				 

		long fd;
			 

		// if exponent is zero, then value is zero
		if (!(fs & 0x7f800000)) fs &= 0x80000000;
		if (!(ft & 0x7f800000)) ft &= 0x80000000;

		// get the magnitude
		ms = fs & 0x7fffffff;
		mt = ft & 0x7fffffff;

		// sort the values by magnitude
		if (ms < mt)
		{
			temp = fs;
			fs = ft;
			ft = temp;
		}
		else if (ms == mt)
		{
			// if values are the same, then positive one is larger
			if (fs < ft)
			{
				temp = fs;
				fs = ft;
				ft = temp;
			}
		}

		// get the exponents
		es = (fs >> 23) & 0xff;
		et = (ft >> 23) & 0xff;
		ed = es - et;

		// debug
		//e1 = es;
		//e2 = et;
		//e3 = ed;
	 

		// get the signs
		ss = fs >> 31;
		st = ft >> 31;

		// get the mantissa and add the hidden bit
		ms = fs & 0x007fffff;
		mt = ft & 0x007fffff;

		// add hidden bit
		ms |= 0x00800000;
		mt |= 0x00800000;

		//cout << "\nAfter hidden bit ms=" << hex << ms << " mt=" << mt;

		// apply the zeros
		if (!es) ms = 0;
		if (!et) mt = 0;

		// apply the signs
		//ms = ( ms ^ ss ) - ss;
		//mt = ( mt ^ st ) - st;

		// do the shift
		//if (ed > (24 + c_iHiddenBits))
		if (ed > 24)
		{
			//ed = 24;
			ed = (24 + c_iHiddenBits);
		}

		// shift up so we don't lose precision
		ms <<= c_iHiddenBits;
		mt <<= c_iHiddenBits;

		// shift smaller value down
		mt >>= ed;

		//cout << "\nAfter shift down mt=" << hex << mt;

		// apply the signs
		ms = (ms ^ ss) - ss;
		mt = (mt ^ st) - st;

		//cout << "\nAfter signs ms=" << hex << ms << " mt=" << mt;

		// do the addition
		md = ms + mt;

		// shift back down result
		//md >>= 4;
									  

		// debug
		//e4 = ms;
		//e5 = mt;
		//e6 = md;

					   
					   

		//cout << "\nAfter addition md=" << hex << md;


		// get the sign
		sd = md >> 31;

		// remove sign
		md = (md ^ sd) - sd;


		//cout << "\nAfter removing sign md=" << hex << md;


		// get new shift amount
		//ed = 8 - __builtin_clz( md );
		//ed = 8 - clz32(md);
		ed = (8 - c_iHiddenBits) - clz32(md);




		// update exponent
		ed += es;


		// debug
		//e7 = es;
		//e8 = ed;
		//e9 = md;


		// make zero if the result is zero
		if (!md)
		{
			ed = 0;
		}
		else
		{
			// get rid of the leading one (hidden bit)
			//md <<= ( __builtin_clz( md ) + 1 );
			md <<= (clz32(md) + 1);
			md = ((unsigned long)md) >> 9;
		}


		//cout << "\nAfter removing leading one md=" << hex << md;


		// remove hidden bit
		//md &= 0x7fffff;




		// check for zero
		if (ed <= 0)
		{
			md = 0;

					   

			// set zero flag //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			// check for underflow
			//if((fs ^ ft) & 0x7fffffff)
			if (ed < 0)
			{
				// set underflow flag //
				//statflag = 0x4008;
						

				*MACFlag |= (1 << (index + 8));
				*StatusFlag |= 0x104;
			}

			sd = ss;

			ed = 0;
		}

		// check for overflow
		if (ed > 255)
		{
			// set overflow flag //
			//statflag = 0x8010;
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;

			md = 0x7fffff;
			ed = 255;
		}

		// set sign flag //
		*MACFlag |= sd & (1 << (index + 4));
		*StatusFlag |= sd & 0x82;


		// debug
		//e10 = md;
		//e11 = ed;
		//e12 = sd;


		// add in exponent
		md += (ed << 23);


		//cout << "\nAfter adding exponent md=" << hex << md;


		// add in sign
		fd = md + (sd << 31);

		//cout << "\nAfter adding sign fd=" << hex << fd;

		return fd;
	}

	
	// add float as float
	inline static long addfloat_flt(long fs, long ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag)
	{
		long es, et, ed;

		long ext;

		FloatLong fld, fls, flt;
		FloatLong fld2;

		fls.l = fs;
		flt.l = ft;

		// sort values
		if ((flt.l & 0x7fffffff) > (fls.l & 0x7fffffff))
		{
			fld.l = flt.l;
			flt.l = fls.l;
			fls.l = fld.l;
		}

		// get exponents
		es = (fls.l >> 23) & 0xff;
		et = (flt.l >> 23) & 0xff;

		//if ((es - et) > 24)
		//{
		//	flt.l = 0;
		//}

		//ext = (fls.l & 0x40000000) >> 6;
		ext = (es - 25) << 23;

		if (es) fls.l -= ext;
		fld.l = flt.l;
		if (et) flt.l -= ext;

		if ((fld.l ^ flt.l) >> 31) flt.l = 0;


		// testing
		//e1 = fls.l;
		//e2 = flt.l;


		// float add
		flt.f = fls.f + flt.f;




		ed = (flt.l >> 23) & 0xff;

		fld.l = flt.l;
		if (ed) fld.l = flt.l + ext;

		//ed = fld.l;
		//fld.l += ext;
		if ((fld.l ^ flt.l) >> 31) ed = ext;

		if (ed > 255)
		{
			// overflow //

			fld.l ^= 0x80000000;
			fld.l |= 0x7fffffff;

			// set overflow flag //
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;
		}


		if (ed <= 0)
		{
			// zero flag //

			fld.l &= 0x80000000;

			// set zero flag //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			// check for underflow
			if (ed < 0)
			{
				// set underflow flag //

				fld.l ^= 0x80000000;

				*MACFlag |= (1 << (index + 8));
				*StatusFlag |= 0x104;
			}
		}

		// set sign flag
		*MACFlag |= (fld.l >> 31) & (1 << (index + 4));
		*StatusFlag |= (fld.l >> 31) & 0x82;

		return fld.l;
	}


	// multiply-add ps2 float as double
	inline static long maddfloat_dbl(long sub, long fs, long ft, long acc, int index, unsigned short* StatusFlag, unsigned short* MACFlag)
	{
		DoubleLong dls, dlt, dld;
		FloatLong fls, flt, fld;

		unsigned long long sign;

		// get sign
		sign = fs ^ ft;

		// 1*x
		if ((ft & 0x7fffff) == 0x7fffff) ft &= ~0x1;

		dls.l = fs;
		dlt.l = ft;

		dls.lu = (dls.lu << 33) >> 4;
		dlt.lu = (dlt.lu << 33) >> 4;

		dld.lu = (2046ull) << 52;
		dlt.d *= dld.d;
		dls.d *= dlt.d;

		dls.lu >>= 29;

		// if not zero sub 127 from exponent
		if (dls.lu) dls.lu -= 127ull << 23;

		// check for underflow
		if (dls.l < 0)
		{
			// underflow //
			*StatusFlag |= 0x100;

			dls.l = 0ull;
		}

		// check for overflow
		if (((long)dls.l) < 0)
		{
			// overflow
			dls.lu = 0x7fffffffull;
		}

		// check for zero exponent
		if (!(dls.lu >> 23))
		{
			// zero //
			dls.l = 0ull;
		}

		// check for sign
		// sign //
		dls.lu |= sign & 0x80000000ull;


		fs = acc;
		ft = (long)dls.l;

		// toggle sign for sub
		if (sub)
		{
			ft ^= -0x80000000;
		}

		if ((ft & 0x7fffffff) == 0x7fffffff)
		{
			fs = ft;
		}

		if ((fs & 0x7fffffff) == 0x7fffffff)
		{
			ft = fs;
		}

		sign = fs ^ ft;

		dls.l = fs;
		dlt.l = ft;

		dls.lu = (dls.lu << 29);
		dlt.lu = (dlt.lu << 29);

		dls.lu &= ~(0x7ull << 60);
		dlt.lu &= ~(0x7ull << 60);

		dld.lu = (1023ull + 127ull) << 52;
		dls.d *= dld.d;
		dlt.d *= dld.d;

		sign = ((sign << 32) >> 63) << 27;
		dls.d += dlt.d;
		dls.lu += sign;

		sign = dls.lu;

		dls.lu = (dls.lu << 1) >> 30;

		// if not zero sub 127 from exponent
		if (dls.lu) dls.lu -= 127ull << 23;

		// check for underflow
		if (dls.l < 0)
		{
			// underflow //
			*MACFlag |= (1 << (index + 8));
			*StatusFlag |= 0x104;

			dls.l = 0ull;
		}

		// check for overflow
		if (((long)dls.l) < 0)
		{
			// overflow
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;

			dls.lu = 0x7fffffffull;
		}

		// check for zero exponent
		if (!(dls.lu >> 23))
		{
			// zero //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			dls.l = 0ull;
		}
		// check for sign (on negative value including negative zero)
		else if (((long long)sign) < 0)
		{
			// sign //
			*MACFlag |= (1 << (index + 4));
			*StatusFlag |= 0x82;

			dls.lu |= 0x80000000ull;
		}

		return (long)dls.l;
	}

	// add ps2 float as double
	inline static long addfloat_dbl(long sub, long fs, long ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag)
	{
		DoubleLong dls, dlt, dld;
		FloatLong fls, flt, fld;

		unsigned long long sign;

		if (sub)
		{
			ft ^= -0x80000000;
		}

		sign = fs ^ ft;

		dls.l = fs;
		dlt.l = ft;

		//dls.lu = (dls.lu << 29);
		//dlt.lu = (dlt.lu << 29);
		dls.l = (dls.l << 32) >> 3;
		dlt.l = (dlt.l << 32) >> 3;

		dls.l &= ~(0x7ull << 60);
		dlt.l &= ~(0x7ull << 60);

		//dld.l = (1023ull + 127ull) << 52;
		dld.l = 1150ull << 52;
		dls.d *= dld.d;
		dlt.d *= dld.d;

		sign = ((sign << 32) >> 63) << 27;
		dls.d += dlt.d;
		dls.lu += sign;

		sign = dls.lu;

		dls.lu = (dls.lu << 1) >> 30;

		// if not zero sub 127 from exponent
		if (dls.lu) dls.lu -= 127ull << 23;

		// check for underflow
		if (dls.l < 0)
		{
			// underflow //
			*MACFlag |= (1 << (index + 8));
			*StatusFlag |= 0x104;

			dls.l = 0ull;
		}

		// check for overflow
		if (((long)dls.l) < 0)
		{
			// overflow
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;

			dls.lu = 0x7fffffffull;
		}

		// check for zero exponent
		if (!(dls.lu >> 23))
		{
			// zero //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			dls.l = 0ull;
		}
		// check for sign (on negative numbers including negative zero)
		else if (((long long)sign) < 0)
		{
			// sign //
			*MACFlag |= (1 << (index + 4));
			*StatusFlag |= 0x82;

			dls.lu |= 0x80000000ull;
		}

		return (long)dls.l;
	}

	// mulitply ps2 float as double
	inline static long mulfloat_dbl(long fs, long ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag)
	{
		DoubleLong dls, dlt, dld;
		FloatLong fls, flt, fld;

		long sign;

		// get sign
		sign = fs ^ ft;

		// 1*x
		if ((ft & 0x7fffff) == 0x7fffff) ft &= ~0x1;

		dls.l = fs;
		dlt.l = ft;

		dls.lu = (dls.lu << 33) >> 4;
		dlt.lu = (dlt.lu << 33) >> 4;

		dld.lu = (2046ull) << 52;
		dlt.d *= dld.d;
		dls.d *= dlt.d;

		dls.lu >>= 29;

		// if not zero sub 127 from exponent
		if (dls.lu) dls.lu -= 127ull << 23;

		// check for underflow
		if (dls.l < 0)
		{
			// underflow //
			*MACFlag |= (1 << (index + 8));
			*StatusFlag |= 0x104;

			dls.l = 0ull;
		}

		// check for overflow
		if (dls.lu > 0x7fffffffull)
		{
			// overflow
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;

			dls.lu = 0x7fffffffull;
		}

		// check for zero exponent
		if (!(dls.lu >> 23))
		{
			// zero //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			dls.l = 0ull;
		}
		// check for sign (on negative value including negative zero)
		else if (sign < 0)
		{
			// sign //
			*MACFlag |= (1 << (index + 4));
			*StatusFlag |= 0x82;

		}

		dls.lu |= sign & 0x80000000ull;

		return (long)dls.l;
	}

	// mulitply float as integer
	inline static long multfloat_int ( long fs, long ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		long es, et, ed;
		long sd;
		
		unsigned long long ms, mt, md;
		
		long ext;
		
		long sign;
		
		long fd;
		
		// sign is always a xor
		fd = ( fs ^ ft ) & -0x80000000;

		// set sign flag //
		sd = fd >> 31;
		*MACFlag |= sd & ( 1 << ( index + 4 ) );
		*StatusFlag |= sd & 0x82;
		
		// get the initial exponent
		es = ( fs >> 23 ) & 0xff;
		et = ( ft >> 23 ) & 0xff;
		ed = es + et - 127;
		
		// get the mantissa and add the hidden bit
		ms = fs & 0x007fffff;
		mt = ft & 0x007fffff;
		
		// optionally mask top bit from bottom ?? (need to test results)
		//mt &= ~(mt >> 22);
		if (mt == 0x007fffff) mt = 0x007ffffe;

		// add hidden bit
		ms |= 0x00800000;
		mt |= 0x00800000;
		
		// do the multiply
		md = ms * mt;
		
		
		// debug
		//ll12 = md;
		
		
		// get bit 47
		ext = md >> 47;
		
		// get the result
		md >>= ( 23 + ext );
		
		// remove the hidden bit
		md &= 0x7fffff;
		
		// update exponent
		ed += ext;
		
		if ( ed > 0xff )
		{
			fd |= 0x7fffffff;
			
			// set overflow flag //
			//statflag = 0x8010;
			*MACFlag |= ( 1 << ( index + 12 ) );
			*StatusFlag |= 0x208;
			
		}
		else if ( ( ed > 0 ) && es && et )
		{
			// put in exponent
			fd |= ( ed << 23 ) | md;
		}
		else
		{
			// set zero flag //
			*MACFlag |= ( 1 << ( index + 0 ) );
			*StatusFlag |= 0x41;
			
			// check for underflow
			if ( ( ed < 0 ) && es && et )
			{
				// set underflow flag //
				//statflag = 0x4008;
				*MACFlag |= ( 1 << ( index + 8 ) );
				*StatusFlag |= 0x104;
			}
		}
		
		return fd;
	}

	
	// mulitply float as integer
	// sign=1 for multsub, 0 for multadd
	inline static long multaddfloat_int ( long fs, long ft, long acc, long sign, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		long es, et, ed;

		long ss, st, sd;
		
		long temp;
		
		long ms, mt, md;
		unsigned long long md64;
		
		//unsigned long long ms, mt, md;
		
		long ext;
		
		//long sign;
		
		long fd;
		
		// sign is always a xor
		fd = ( fs ^ ft ) & -0x80000000;

		// set sign flag //
		//sd = fd >> 31;
		// *MACFlag |= sd & ( 1 << ( index + 4 ) );
		// *StatusFlag |= sd & 0x82;
		
		// get the initial exponent
		es = ( fs >> 23 ) & 0xff;
		et = ( ft >> 23 ) & 0xff;
		ed = es + et - 127;
		
		// get the mantissa and add the hidden bit
		ms = fs & 0x007fffff;
		mt = ft & 0x007fffff;
		
		// optionally mask top bit from bottom ?? (need to test results)
		mt &= ~(mt >> 22);
		
		// add hidden bit
		ms |= 0x00800000;
		mt |= 0x00800000;
		
		// do the multiply
		md64 = (u64)ms * (u64)mt;
		
		
		// debug
		//ll12 = md;
		
		
		// get bit 47
		ext = md64 >> 47;
		
		// get the result
		md64 >>= ( 23 + ext );
		
		// remove the hidden bit
		md64 &= 0x7fffff;
		
		// update exponent
		ed += ext;
		
		if ( ed > 0xff )
		{
			fd |= 0x7fffffff;
			
			
			// set overflow flag //
			// *MACFlag |= ( 1 << ( index + 12 ) );
			// *StatusFlag |= 0x208;
			
		}
		else if ( ( ed > 0 ) && es && et )
		{
			// put in exponent
			fd |= ( ed << 23 ) | md64;
		}
		else
		{
			// set zero flag //
			// *MACFlag |= ( 1 << ( index + 0 ) );
			// *StatusFlag |= 0x41;
			
			// check for underflow
			if ( ( ed < 0 ) && es && et )
			{
				// set underflow flag //
				//statflag = 0x4008;
				// *MACFlag |= ( 1 << ( index + 8 ) );
				// *StatusFlag |= 0x104;
				// set underflow sticky flag ONLY
				*StatusFlag |= 0x100;

				// todo: calculation should stop here and return fd (previous value) ??
				// 
			}
			
			// this is tricky, because if acc is +/-max then set zero sticky flag ??
			if ( ( acc & 0x7fffffff ) == 0x7fffffff )
			{
				// set zero sticky flag ONLY
				*StatusFlag |= 0x40;
			}
			
		}
		
		
		// sign
		fd = ( fd ^ ( sign << 31 ) );
		
		// now, before doing the add, if fd is +/-max then set to acc
		if ( ( fd & 0x7fffffff ) == 0x7fffffff )
		{
			acc = fd;
		}
		
		// now, before doing the add, if acc is +/-max then set to fd
		//if ( ( acc & 0x7f800000 ) == 0x7f800000 )
		if ( ( acc & 0x7fffffff ) == 0x7fffffff )
		{
			fd = acc;
		}
		
		// now do the add part //
		fs = fd;
		ft = acc;

		// get the magnitude
		// todo: or just sort the values by exponent ?
		ms = fs & 0x7fffffff;
		mt = ft & 0x7fffffff;
		
		// sort the values by magnitude
		if ( ms < mt )
		{
			temp = fs;
			fs = ft;
			ft = temp;
		}
		
		// get the exponents
		es = ( fs >> 23 ) & 0xff;
		et = ( ft >> 23 ) & 0xff;
		ed = es - et;
		
		// debug
		//e1 = es;
		//e2 = et;
		//e3 = ed;
		
		// get the signs
		ss = fs >> 31;
		st = ft >> 31;
		
		// get the mantissa and add the hidden bit
		ms = fs & 0x007fffff;
		mt = ft & 0x007fffff;
		
		// add hidden bit
		ms |= 0x00800000;
		mt |= 0x00800000;
		
		// apply the zeros
		if ( !es ) ms = 0;
		if ( !et ) mt = 0;
		
		// do the shift
		if ( ed > 23 )
		{
			ed = 24;
		}
		
		mt >>= ed;
		
		
		// apply the signs
		ms = ( ms ^ ss ) - ss;
		mt = ( mt ^ st ) - st;
		
		
		
		// do the addition
		md = ms + mt;


		// debug
		//e4 = ms;
		//e5 = mt;
		//e6 = md;
		
		
		// get the sign
		sd = md >> 31;
		
		// remove sign
		md = ( md ^ sd ) - sd;

		

		
		// get new shift amount
		//ed = 8 - __builtin_clz( md );
		ed = 8 - clz32(md);

		
		
		
		// update exponent
		ed += es;

		
		// debug
		//e7 = es;
		//e8 = ed;
		//e9 = md;

		
		// make zero if the result is zero
		if ( !md )
		{
			ed = 0;
		}
		else
		{
			// get rid of the leading one (hidden bit)
			//md <<= ( __builtin_clz( md ) + 1 );
			md <<= (clz32(md) + 1);
			md = ( (unsigned long) md ) >> 9;
		}
		
		
		
		
		// remove hidden bit
		//md &= 0x7fffff;



		
		// check for zero
		if ( ed <= 0 )
		{
			md = 0;
			
			// set zero flag //
			*MACFlag |= ( 1 << ( index + 0 ) );
			*StatusFlag |= 0x41;
			
			// check for underflow
			if ( ed < 0 )
			{
				// set underflow flag //
				//statflag = 0x4008;
				*MACFlag |= ( 1 << ( index + 8 ) );
				*StatusFlag |= 0x104;
			}
			
			ed = 0;
			
			// get sign of zero
			sd = ss & st;
		}
		
		// check for overflow
		if ( ed > 255 )
		{
			// set overflow flag //
			//statflag = 0x8010;
			*MACFlag |= ( 1 << ( index + 12 ) );
			*StatusFlag |= 0x208;
			
			md = 0x7fffff;
			ed = 255;
		}
		
		
		// set sign flag //
		*MACFlag |= ( sd & ( 1 << ( index + 4 ) ) );
		*StatusFlag |= ( sd & 0x82 );

		
		// debug
		//e10 = md;
		//e11 = ed;
		//e12 = sd;

		
		// add in exponent
		md += ( ed << 23 );
		
		
		// add in sign
		fd = md + ( sd << 31 );
		
		return fd;
	}

	// mulitply-add float as float
	// sign=1 for multsub, 0 for multadd
	inline static long multaddfloat_flt ( long fs, long ft, long acc, long sign, int index, unsigned short* StatusFlag, unsigned short* MACFlag )
	{
		long es, et, ed;

		long ss, st, sd;
		
		long temp;
		
		long ms, mt, md;
		unsigned long long md64;
		
		//unsigned long long ms, mt, md;
		
		long ext;
		
		//long sign;
		
		long fd;
		
		FloatLong fld, fls, flt;

		// sign is always a xor
		fd = ( fs ^ ft ) & -0x80000000;

		// set sign flag //
		//sd = fd >> 31;
		// *MACFlag |= sd & ( 1 << ( index + 4 ) );
		// *StatusFlag |= sd & 0x82;
		
		// get the initial exponent
		es = ( fs >> 23 ) & 0xff;
		et = ( ft >> 23 ) & 0xff;
		ed = es + et - 127;
		
		// get the mantissa and add the hidden bit
		ms = fs & 0x007fffff;
		mt = ft & 0x007fffff;
		
		// optionally mask top bit from bottom ?? (need to test results)
		mt &= ~(mt >> 22);
		
		// add hidden bit
		ms |= 0x00800000;
		mt |= 0x00800000;
		
		// do the multiply
		md64 = (u64)ms * (u64)mt;
		
		
		// debug
		//ll12 = md;
		
		
		// get bit 47
		ext = md64 >> 47;
		
		// get the result
		md64 >>= ( 23 + ext );
		
		// remove the hidden bit
		md64 &= 0x7fffff;
		
		// update exponent
		ed += ext;
		
		if ( ed > 0xff )
		{
			fd |= 0x7fffffff;
			
			
			// set overflow flag //
			// *MACFlag |= ( 1 << ( index + 12 ) );
			// *StatusFlag |= 0x208;
			
		}
		else if ( ( ed > 0 ) && es && et )
		{
			// put in exponent
			fd |= ( ed << 23 ) | md64;
		}
		else
		{
			// set zero flag //
			// *MACFlag |= ( 1 << ( index + 0 ) );
			// *StatusFlag |= 0x41;
			
			// check for underflow
			if ( ( ed < 0 ) && es && et )
			{
				// set underflow flag //
				//statflag = 0x4008;
				// *MACFlag |= ( 1 << ( index + 8 ) );
				// *StatusFlag |= 0x104;
				// set underflow sticky flag ONLY
				*StatusFlag |= 0x100;

				// todo: calculation should stop here and return fd (previous value) ??
				// 
			}
			
			// this is tricky, because if acc is +/-max then set zero sticky flag ??
			if ( ( acc & 0x7fffffff ) == 0x7fffffff )
			{
				// set zero sticky flag ONLY
				*StatusFlag |= 0x40;
			}
			
		}
		
		
		// sign
		fd = ( fd ^ ( sign << 31 ) );
		
		// now, before doing the add, if fd is +/-max then set to acc
		if ( ( fd & 0x7fffffff ) == 0x7fffffff )
		{
			acc = fd;
		}
		
		// now, before doing the add, if acc is +/-max then set to fd
		//if ( ( acc & 0x7f800000 ) == 0x7f800000 )
		if ( ( acc & 0x7fffffff ) == 0x7fffffff )
		{
			fd = acc;
		}
		
		// now do the add part //
		fs = fd;
		ft = acc;

		fls.l = fs;
		flt.l = ft;

		// sort values
		if ((flt.l & 0x7fffffff) > (fls.l & 0x7fffffff))
		{
			fld.l = flt.l;
			flt.l = fls.l;
			fls.l = fld.l;
		}

		// get exponents
		es = (fls.l >> 23) & 0xff;
		et = (flt.l >> 23) & 0xff;

		//if ((es - et) > 24)
		//{
		//	flt.l = 0;
		//}

		//ext = (fls.l & 0x40000000) >> 6;
		ext = (es - 25) << 23;

		if (es) fls.l -= ext;
		fld.l = flt.l;
		if (et) flt.l -= ext;

		if ((fld.l ^ flt.l) >> 31) flt.l = 0;


		// testing
		//e1 = fls.l;
		//e2 = flt.l;


		// float add
		flt.f = fls.f + flt.f;




		ed = (flt.l >> 23) & 0xff;

		fld.l = flt.l;
		if (ed) fld.l = flt.l + ext;

		//ed = fld.l;
		//fld.l += ext;
		if ((fld.l ^ flt.l) >> 31) ed = ext;

		if (ed > 255)
		{
			// overflow //

			fld.l ^= 0x80000000;
			fld.l |= 0x7fffffff;

			// set overflow flag //
			*MACFlag |= (1 << (index + 12));
			*StatusFlag |= 0x208;
		}


		if (ed <= 0)
		{
			// zero flag //

			fld.l &= 0x80000000;

			// set zero flag //
			*MACFlag |= (1 << (index + 0));
			*StatusFlag |= 0x41;

			// check for underflow
			if (ed < 0)
			{
				// set underflow flag //

				fld.l ^= 0x80000000;

				*MACFlag |= (1 << (index + 8));
				*StatusFlag |= 0x104;
			}
		}

		// set sign flag
		*MACFlag |= (fld.l >> 31) & (1 << (index + 4));
		*StatusFlag |= (fld.l >> 31) & 0x82;

		return fld.l;
	}



	
	// PS2 floating point ADD
	inline static float PS2_Float_Add ( float fs, float ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )		//long* zero, long* sign, long* overflow, long* underflow,
									//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs + ft
		
		FloatLong Result;
		
		//short sflag1 = 0, sflag2 = 0, mflag1 = 0, mflag2 = 0;
#ifdef USE_INTEGER_MATH_ADD
		FloatLong fls, flt;
		fls.f = fs;
		flt.f = ft;

		Result.l = addfloat_flt ( fls.l, flt.l, index, StatusFlag, MACFlag );
		
		
#else

#ifdef USE_DOUBLE_MATH_ADD

		Result.l = addfloat_dbl(0, (long&)fs, (long&)ft, index, StatusFlag, MACFlag);

#else
	

		ClampValue2_f ( fs, ft );
		
		Result.f = fs + ft;

#ifdef ENABLE_FLAG_CHECK

		SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
		
#endif

#endif

#endif
		
		// done?
		return Result.f;
	}

	// PS2 floating point SUB
	inline static float PS2_Float_Sub ( float fs, float ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )		//long* zero, long* sign, long* overflow, long* underflow,
										//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs - ft
		
		FloatLong Result;

#ifdef USE_INTEGER_MATH_SUB
		FloatLong fls, flt;
		fls.f = fs;
		flt.f = ft;
		
		flt.l ^= -0x80000000;

		//Result.l = addfloat_int ( fls.l, flt.l, index, StatusFlag, MACFlag );
		Result.l = addfloat_flt(fls.l, flt.l, index, StatusFlag, MACFlag);

		
#else

#ifdef USE_DOUBLE_MATH_SUB

		Result.l = addfloat_dbl(1, (long&)fs, (long&)ft, index, StatusFlag, MACFlag);

#else

		ClampValue2_f ( fs, ft );
		
		Result.f = fs - ft;

		
#ifdef ENABLE_FLAG_CHECK


		SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );

#endif

#endif	// #ifdef USE_DOUBLE_MATH_SUB

#endif	// #ifdef USE_INTEGER_MATH_SUB
		
		// done?
		return Result.f;
	}

	// PS2 floating point MUL
	inline static float PS2_Float_Mul ( float fs, float ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs * ft

		
		FloatLong Result;
		FloatLong flf;

#ifdef USE_INTEGER_MATH_MUL
		FloatLong fls, flt;
		fls.f = fs;
		flt.f = ft;

		Result.l = multfloat_int ( fls.l, flt.l, index, StatusFlag, MACFlag );
		
		
#else


#ifdef USE_DOUBLE_MATH_MUL

		Result.l = mulfloat_dbl((long&)fs, (long&)ft, index, StatusFlag, MACFlag);

#else

		ClampValue2_f ( fs, ft );
		
		
#ifdef ENABLE_MUL_PRECISION_CHOP
		// multiply does not use full precision ??
		flf.f = ft;
		flf.l &= 0xfffffffe;
		
		Result.f = fs * flf.f;
#else
		Result.f = fs * ft;
#endif

		
#ifdef ENABLE_FLAG_CHECK

		SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
		
#endif

#endif	// #ifdef USE_DOUBLE_MATH_MUL

#endif	// #ifdef USE_INTEGER_MATH_MUL
		
		// done?
		return Result.f;
	}

	// PS2 floating point MADD
	inline static float PS2_Float_Madd ( float dACC, float fd, float fs, float ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = ACC + fs * ft
		
		FloatLong Result;
		FloatLong ACC;
		FloatLong flf;

#ifdef USE_INTEGER_MATH_MADD
		FloatLong fls, flt;
		fls.f = fs;
		flt.f = ft;
		ACC.f = dACC;

		//Result.l = multaddfloat_int ( fls.l, flt.l, ACC.l, 0, index, StatusFlag, MACFlag );
		Result.l = multaddfloat_flt(fls.l, flt.l, ACC.l, 0, index, StatusFlag, MACFlag);

#else

#ifdef USE_DOUBLE_MATH_MADD

		Result.l = maddfloat_dbl(0, (long&)fs, (long&)ft, (long&)dACC, index, StatusFlag, MACFlag);

#else

		ClampValue2_f ( fs, ft );
		
		// also need to clamp accumulator
		// no, actually, you don't
		//ClampValue_f ( dACC );

		
#ifdef ENABLE_MUL_PRECISION_CHOP
		// multiply does not use full precision ??
		flf.f = ft;
		flf.l &= 0xfffffffe;
		
		Result.f = fs * flf.f;
#else
		Result.f = fs * ft;
#endif

		
#ifdef ENABLE_FLAG_CHECK_MADD

	// check for multiply overflow
	if ( ( Result.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
	{
		// multiply overflow in MADD //
		*StatusFlag |= 0x208;
		*MACFlag |= ( 1 << ( index + 12 ) );
		
		// sign flag
		if ( Result.l >> 31 )
		{
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		// set to +/-max
		Result.l = c_lFloat_MantissaMask | Result.l;
		
		// return result
		return Result.f;
	}
	
	// check for ACC overflow
	ACC.f = dACC;
	if ( ( ACC.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
	{
		// multiply overflow in MADD //
		*StatusFlag |= 0x208;
		*MACFlag |= ( 1 << ( index + 12 ) );
		
		// sign flag
		if ( ACC.l >> 31 )
		{
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		// set to +/-max
		ACC.l = c_lFloat_MantissaMask | ACC.l;
		
		// check for multiply underflow, and set sticky underflow ONLY if so
		if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
		{
			// multiply underflow in MADD //
			
			// *note* ONLY set underflow sticky flag on underflow
			*StatusFlag |= 0x100;
			// *MACFlag |= ( 1 << ( index + 8 ) );

		}
		
		// return result
		return ACC.f;
	}
	
	
	// check for multiply underflow
	if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
	{
		// multiply underflow in MADD //
		
		// *note* ONLY set underflow sticky flag on underflow
		*StatusFlag |= 0x100;
		// *MACFlag |= ( 1 << ( index + 8 ) );

		// set result to previous value ??
		//Result.f = fd;
		
		// flag check based on ACC ??
		SetFlagsOnResult_f ( ACC.f, index, StatusFlag, MACFlag );
		
#ifdef ENABLE_PREVIOUS_VALUE_ON_MULTIPY_UNDERFLOW
		// return previous value ??
		return fd;
#else
		return ACC.f;
#endif
	}
	
	// if not multiply overflow on final check, then do the add and check flags
	
	// perform the addition
	Result.f += dACC;
	
	SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
	
	return Result.f;

#else
		
#ifdef ENABLE_FLAG_CHECK

		//SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
		
		
		// if multiply overflow, then set flags and return result
		// for now, just set flags
		if ( ( Result.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
		{
			// multiply overflow in MADD //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// sign flag
			if ( Result.l >> 31 )
			{
				*StatusFlag |= 0x82;
				*MACFlag |= ( 1 << ( index + 4 ) );
			}
			
			// set to +/-max
			Result.l = c_lFloat_MantissaMask | Result.l;
			
			// return result
			return Result.f;
		}
		
		
		
		// if multiply underflow, then only store ACC if it is +/-MAX, otherwise keep previous value
		// for now, just set sticky flags
		if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
		{
			// multiply underflow in MADD //
			
			// *note* ONLY set underflow sticky flag on underflow
			*StatusFlag |= 0x100;
			// *MACFlag |= ( 1 << ( index + 8 ) );

		}
		
		
#endif

#endif

		// perform the addition
		Result.f += dACC;


#ifdef ENABLE_FLAG_CHECK

		SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );

#endif

#endif

#endif	// #ifdef USE_INTEGER_MATH_MADD
		
		// done?
		return Result.f;
	}

	
	// PS2 floating point MSUB
	inline static float PS2_Float_Msub ( float dACC, float fd, float fs, float ft, int index, unsigned short* StatusFlag, unsigned short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = ACC + fs * ft
		
		FloatLong Result;
		FloatLong ACC;
		FloatLong flf;
		
		FloatLong fResultTest;
		
		DoubleLong dfs, dft;
		FloatLong fProd;
		
		short sflag1 = 0, sflag2 = 0, mflag1 = 0, mflag2 = 0;

#ifdef USE_INTEGER_MATH_MSUB
		FloatLong fls, flt;
		fls.f = fs;
		flt.f = ft;
		ACC.f = dACC;

		//Result.l = multaddfloat_int ( fls.l, flt.l, ACC.l, 1, index, StatusFlag, MACFlag );
		Result.l = multaddfloat_flt(fls.l, flt.l, ACC.l, 1, index, StatusFlag, MACFlag);

#else

#ifdef USE_DOUBLE_MATH_MSUB

		Result.l = maddfloat_dbl(1, (long&)fs, (long&)ft, (long&)dACC, index, StatusFlag, MACFlag);

#else
		

		ClampValue2_f ( fs, ft );
		
		// also need to clamp accumulator
		// no, actually, you don't
		//ClampValue_f ( dACC );

		
#ifdef ENABLE_MUL_PRECISION_CHOP
		// multiply does not use full precision ??
		flf.f = ft;
		flf.l &= 0xfffffffe;
		
#ifdef NEGATE_MSUB_MULTIPLY_RESULT
		Result.f = -( fs * flf.f );
#else
		Result.f = fs * flf.f;
#endif

#else

		// probably does -fs times ft, then adds with the accumulator
#ifdef NEGATE_MSUB_MULTIPLY_RESULT
		Result.f = -( fs * ft );
#else
		Result.f = fs * ft;
#endif

#endif	// #ifdef ENABLE_MUL_PRECISION_CHOP


// testing
//fProd.f = Result.f;


#ifdef ENABLE_FLAG_CHECK_MADD

	// check for multiply overflow
	if ( ( Result.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
	{
		// multiply overflow in MADD //
		*StatusFlag |= 0x208;
		*MACFlag |= ( 1 << ( index + 12 ) );
		
		// sign flag
		if ( Result.l >> 31 )
		{
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		// set to +/-max
		Result.l = c_lFloat_MantissaMask | Result.l;
		
		// return result
		return Result.f;
	}
	
	// check for ACC overflow
	ACC.f = dACC;
	if ( ( ACC.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
	{
		// multiply overflow in MADD //
		*StatusFlag |= 0x208;
		*MACFlag |= ( 1 << ( index + 12 ) );
		
		// sign flag
		if ( ACC.l >> 31 )
		{
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		// set to +/-max
		ACC.l = c_lFloat_MantissaMask | ACC.l;
		
		// check for multiply underflow, and set sticky underflow ONLY if so
		if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
		{
			// multiply underflow in MADD //
			
			// *note* ONLY set underflow sticky flag on underflow
			*StatusFlag |= 0x100;
			// *MACFlag |= ( 1 << ( index + 8 ) );

		}
		
		// return result
		return ACC.f;
	}
	
	
	// check for multiply underflow
	if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
	{
		// multiply underflow in MADD //
		
		// *note* ONLY set underflow sticky flag on underflow
		*StatusFlag |= 0x100;
		// *MACFlag |= ( 1 << ( index + 8 ) );

		// set result to previous value ??
		//Result.f = fd;
		
		// flag check based on ACC ??
		SetFlagsOnResult_f ( ACC.f, index, StatusFlag, MACFlag );
		
#ifdef ENABLE_PREVIOUS_VALUE_ON_MULTIPY_UNDERFLOW
		// return previous value ??
		return fd;
#else
		return ACC.f;
#endif
	}
	
	// if not multiply overflow on final check, then do the add and check flags
	
	// perform the addition
	Result.f += dACC;
	
	SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
	
//if ( ( ( Result.l - fResultTest.l ) > 1 ) || ( ( Result.l - fResultTest.l ) < -1 ) )
//{
//	cout << "\nResultTest failed: Result.l=" << hex << Result.l << " " << Result.f << " fResultTest=" << hex << fResultTest.l << " " << fResultTest.f;
//	cout << " fs=" << fs << " ft=" << ft << " acc=" << dACC;
//	cout << " dfs=" << ds.l << " " << ds.d << " dft=" << dt.l << " " << dt.d << " dResult=" << dResult.l << " " << dResult.d;
//	cout << " fProd=" << fProd.l << " " << fProd.f << " dProd=" << dProd.l << " " << dProd.d;
//	cout << " dlACC=" << dlACC.l << " " << dlACC.d;
//	cout << " fd=" << fd << " " << dd.l << " " << dd.d;
//}

//if ( sflag1 != *StatusFlag )
//{
//	cout << "\nmismatch stat flag1=" << hex << " " << sflag1 << " " << *StatusFlag;
//}

//if ( mflag1 != *MACFlag )
//{
//	cout << "\nmismatch mac flag1=" << hex << " " << mflag1 << " " << *MACFlag;
//}


	// *StatusFlag = sflag1;
	// *MACFlag = mflag1;
	//return fResultTest.f;
	return Result.f;

#else
		
#ifdef ENABLE_FLAG_CHECK

		//SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );
		
		
		// if multiply overflow, then set flags and return result
		// for now, just set flags
		if ( ( Result.l & c_lFloat_ExpMask ) == c_lFloat_ExpMask )
		{
			// multiply overflow in MADD //
			
			// overflow flag
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
			
			// sign flag
			if ( Result.l >> 31 )
			{
				*StatusFlag |= 0x82;
				*MACFlag |= ( 1 << ( index + 4 ) );
			}
			
			// set to +/-max
			Result.l = c_lFloat_MantissaMask | Result.l;
			
			// return result
			return Result.f;
		}
		
		
		
		// if multiply underflow, then only store ACC if it is +/-MAX, otherwise keep previous value
		// for now, just set sticky flags
		if ( ( Result.l & c_lFloat_MantissaMask ) && !( Result.l & c_lFloat_ExpMask ) )
		{
			// multiply underflow in MADD //
			
			// *note* ONLY set underflow sticky flag on underflow
			*StatusFlag |= 0x100;
			// *MACFlag |= ( 1 << ( index + 8 ) );

		}
		
#endif

#endif

		
		// perform the addition
#ifdef NEGATE_MSUB_MULTIPLY_RESULT
		Result.f = dACC + Result.f;
#else
		Result.f = dACC - Result.f;
#endif



#ifdef ENABLE_FLAG_CHECK

		SetFlagsOnResult_f ( Result.f, index, StatusFlag, MACFlag );

#endif

#endif

#endif	// #ifdef USE_INTEGER_MATH_MSUB


		// done?
		return Result.f;
	}


	
	inline static long PS2_Float_ToInteger ( float fs )
	{
		//FloatLong Result;
		long lResult;
		
		//if ( isNaNorInf ( fs ) ) (long&) fs = ( ( (long&) fs ) & 0xff800000 );
		//if ( isNaNorInf ( ft ) ) (long&) ft = ( ( (long&) ft ) & 0xff800000 );
		
		lResult = (long&) fs;
		
		// get max
		//fResult = ( ( fs < ft ) ? fs : ft );
		if ( ( lResult & 0x7f800000 ) <= 0x4e800000 )
		{
			lResult = (long) fs;
		}
		else if ( lResult & 0x80000000 )
		{
			// set to negative integer max
			lResult = 0x80000000;
		}
		else
		{
			// set to positive integer max
			lResult = 0x7fffffff;
		}
		
		// MIN does NOT affect any flags
		
		// done?
		return lResult;
	}


	inline static float PS2_Float_Max ( float fs, float ft )
	{
		//FloatLong Result;
		float fResult;
		
#ifdef COMPUTE_MINMAX_AS_INTEGER
		long lfs, lft;
		
		lfs = (long&) fs;
		lft = (long&) ft;
		
		// take two's complement before comparing if negative
		// note: actually, should be one's complement for ps2 float values
		//lfs = ( ( lfs >> 31 ) ^ ( lfs & 0x7fffffff ) ) + ( ( lfs >> 31 ) & 1 );
		//lft = ( ( lft >> 31 ) ^ ( lft & 0x7fffffff ) ) + ( ( lft >> 31 ) & 1 );
		lfs = (lfs >> 31) ^ (lfs & 0x7fffffff);
		lft = (lft >> 31) ^ (lft & 0x7fffffff);

		// compare as integer and return original value?
		fResult = ( ( lfs > lft ) ? fs : ft );
		
#else

#ifdef IGNORE_DENORMAL_MAXMIN
		if ( isNaNorInf ( fs ) ) (long&) fs = ( ( (long&) fs ) & 0xff800000 );
		if ( isNaNorInf ( ft ) ) (long&) ft = ( ( (long&) ft ) & 0xff800000 );
#else
		ClampValue2_f ( fs, ft );
#endif
		
		// get max
		fResult = ( ( fs > ft ) ? fs : ft );
		
#endif

		// MAX does NOT affect any flags
		
		// done?
		return fResult;
	}

	inline static float PS2_Float_Min ( float fs, float ft )
	{
		//FloatLong Result;
		float fResult;

#ifdef COMPUTE_MINMAX_AS_INTEGER		
		long lfs, lft;
		
		lfs = (long&) fs;
		lft = (long&) ft;
		
		// take two's complement before comparing if negative
		// note: actually, should be one's complement for ps2 float values
		//lfs = ( ( lfs >> 31 ) ^ ( lfs & 0x7fffffff ) ) + ( ( lfs >> 31 ) & 1 );
		//lft = ( ( lft >> 31 ) ^ ( lft & 0x7fffffff ) ) + ( ( lft >> 31 ) & 1 );
		lfs = (lfs >> 31) ^ (lfs & 0x7fffffff);
		lft = (lft >> 31) ^ (lft & 0x7fffffff);

		// compare as integer and return original value?
		fResult = ( ( lfs < lft ) ? fs : ft );
		
#else
		
#ifdef IGNORE_DENORMAL_MAXMIN
		if ( isNaNorInf ( fs ) ) (long&) fs = ( ( (long&) fs ) & 0xff800000 );
		if ( isNaNorInf ( ft ) ) (long&) ft = ( ( (long&) ft ) & 0xff800000 );
#else
		ClampValue2_f ( fs, ft );
#endif
		
		// get max
		fResult = ( ( fs < ft ) ? fs : ft );

#endif		
		// MIN does NOT affect any flags
		
		// done?
		return fResult;
	}

	

	// PS2 floating point SQRT
	inline static float PS2_Float_Sqrt ( float ft, unsigned short* StatusFlag )	//long* invalid_negative, long* invalid_zero,
										//long* divide_sticky, long* invalid_negative_sticky, long* invalid_zero_sticky )
	{
		FloatLong Result;
		long l;
		float f;
		
#ifdef USE_DOUBLE_MATH_SQRT
		// Q = sqrt ( ft )
		DoubleLong Dd, Ds, Dt;


		Ds.l = (long&)ft;

		Ds.lu = (Ds.lu << 33) >> 4;

		// multiply with multiplier
		Dd.lu = (1023ull + 896ull) << 52;
		Ds.d *= Dd.d;

		// sqrt
		Dd.d = sqrt(Ds.d);
		Dd.lu += 3ull << 25;

		// convert to float
		Result.f = Dd.d;

		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;

		// set i-flag if arg is negative and not zero
		if ((((long&)ft) < 0) && (ft != 0.0f))
		{
			*StatusFlag |= 0x410;
		}

		return Result.f;
#else

		ClampValue_f ( ft );
		
		// absolute value of ft
		l = ( 0x7fffffff & (long&) ft );
		f = (float&) l;
		
		Result.f = sqrt ( f );

#endif
		
		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;
		
		// check zero division flag -> set to zero if divide by zero
		// write zero division flag -> set to zero for SQRT
		//*divide = -1;
		
		// write invalid flag (SQRT of negative number or 0/0)
		//*invalid_negative = (long&) ft;
		if ( ft < 0.0f )
		{
			*StatusFlag |= 0x410;
		}
		
		// write zero divide/invalid sticky flags
		// leave divide by zero sticky flag alone, since it did not accumulate
		//*divide_stickyflag &= -1;
		//*invalid_negative_sticky |= (long&) ft;
		
		// invalid zero is ok, since there is no divide here
		// leave sticky flag alone
		//*invalid_zero = 0;
		//*invalid_zero_sticky -> leave alone
		
		// done?
		return Result.f;
	}

	
	// PS2 floating point RSQRT
	inline static float PS2_Float_RSqrt ( float fs, float ft, unsigned short* StatusFlag )
	{
		FloatLong Result;
		long l;
		float f;
		
#ifdef USE_DOUBLE_MATH_RSQRT

		DoubleLong Dd, Ds, Dt, Dc;
		
		long temp1, temp2;
		
		Ds.l = (long&)ft;

		Ds.lu = (Ds.lu << 33) >> 4;

		// multiply with multiplier
		Dd.lu = (1023ull + 896ull) << 52;
		Ds.d *= Dd.d;

		// sqrt
		Dd.d = sqrt(Ds.d);
		Dd.lu += 3ull << 25;

		// convert to float
		Result.f = Dd.d;

		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;

		// set i-flag if arg is negative and not zero
		if (!(((long&)ft) & 0x7f800000))
		{
			*StatusFlag |= 0x820;

			if (!(((long&)fs) & 0x7f800000))
			{
				// need to make this 1/0 and not 0/0
				// so that you get the divide by zero exception instead of invalid exception
				fs = 1.0f;
			}
		}
		else if (((long&)ft) < 0)
		{
			*StatusFlag |= 0x410;
		}

		Ds.l = (long&)fs;
		Dt.l = Result.l;

		Ds.lu = (Ds.lu << 33) >> 4;
		Dt.lu = (Dt.lu << 33) >> 4;

		Dd.d = Ds.d / Dt.d;
		//Dd.d *= Dc.d;

		// adjust result
		//Dd.lu += 1ull << 28;

		// get result
		Dd.lu >>= 29;

		Dc.lu = 896ull << 23;
		Dd.lu -= Dc.lu;

		// zero on underflow or zero
		if (Dd.l < 0) Dd.l = 0;

		// maximize on overflow
		if (Dd.lu > 0x7fffffffull) Dd.lu = 0x7fffffffull;

		// put sign in
		if (((long&)fs) < 0) Dd.lu |= 0x80000000ull;

		Result.l = (long)Dd.l;

		return Result.f;
#else

		ClampValue2_f ( fs, ft );
		
		// absolute value of ft
		l = ( 0x7fffffff & (long&) ft );
		f = (float&) l;
		
		Result.f = fs / sqrt ( f );

#endif
		
		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;
		
		// write invalid flag (SQRT of negative number or 0/0)
		//*invalid_negative = (long&) ft;
		//temp1 = (long&) fs;
		//temp2 = (long&) ft;
		//*invalid_zero = temp1 | temp2;
		// *** todo ***
		//*invalid_zero = (long&) fs | (long&) ft;
		if ( ( ft < 0.0f ) || ( fs == 0.0f && ft == 0.0f ) )
		{
			*StatusFlag |= 0x410;
			
			if ( fs == 0.0f )
			{
				// make sure result is zero??
				Result.l &= 0x80000000;
			}
		}
		
		
		// write zero division flag -> set to zero for SQRT
		// write denominator
		//*divide = (long&) ft;
		if ( fs != 0.0f && ft == 0.0f )
		{
			*StatusFlag |= 0x820;
			
			// set result to +max/-max ??
			Result.l |= 0x7fffffff;
		}
		
		
		// done?
		return Result.f;
	}


	// PS2 floating point DIV
	inline static float PS2_Float_Div(float fs, float ft, unsigned short* StatusFlag)
	{
		//FloatLong Result;

		FloatLong Result;
		FloatLong fls, flt, fld;

		long es, et, ed;
		long sign;


#ifdef USE_DOUBLE_MATH_DIV
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt, Dc;

		sign = (long&)fs ^ (long&)ft;

		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;

		// check for 0/0
		if (!(((long&)ft) & 0x7f800000))
		{
			if (!(((long&)fs) & 0x7f800000))
			{
				// 0/0
				*StatusFlag |= 0x410;

				// need to make this 1/0 and not 0/0
				// so that you get the divide by zero exception instead of invalid exception
				fs = 1.0f;
			}
			else
			{
				// x/0
				*StatusFlag |= 0x820;
			}
		}

		Ds.l = (long&)fs;
		Dt.l = (long&)ft;

		Ds.lu = (Ds.lu << 33) >> 4;
		Dt.lu = (Dt.lu << 33) >> 4;


		Dd.d = Ds.d / Dt.d;

		//Dd.d *= Dc.d;

		// adjust result
		Dd.lu += 1ull << 28;

		// get result
		Dd.lu >>= 29;

		//Dc.lu = 127ull << 52;
		Dc.lu = 896ull << 23;
		Dd.lu -= Dc.lu;

		// zero on underflow or zero
		if (Dd.l < 0) Dd.l = 0;

		// maximize on overflow
		if (Dd.lu > 0x7fffffffull) Dd.lu = 0x7fffffffull;

		// put sign in
		if (sign < 0) Dd.lu |= 0x80000000ull;

		Result.l = (long)Dd.l;

		//return (float&)Dd.lu;
		//return Result.f;
#else

		ClampValue2_f(fs, ft);

		Result.f = fs / ft;

		// clear affected non-sticky flags
		// note: mind as well clear the sticky flag area too since this shouldn't set the actual flag
		// *StatusFlag &= ~0x30;
		*StatusFlag &= ~0xc30;

		// write zero division flag -> set to zero for SQRT
		// write denominator
		if (ft == 0.0f)
		{
			// also set result to +max or -max
			Result.l |= 0x7fffffff;

			if (fs != 0.0f)
			{
				// set divide by zero flag //
				*StatusFlag |= 0x820;
			}
			else
			{
				// set invalid flag //
				*StatusFlag |= 0x410;
	}
		}

#endif

		// done?
		return Result.f;
	}

}

#endif	// end #ifndef __PS2FLOAT_H__
