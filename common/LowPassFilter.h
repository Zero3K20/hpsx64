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
#ifndef __LOWPASSFILTER_H__
#define __LOWPASSFILTER_H__

//#include <iostream>

//using namespace std;


namespace Emulator
{
	namespace Audio
	{
	
		template <const int NumOfFilters, const int BufferSize_PowerOfTwo>
		class LowPassFilter
		{
			uint64_t WriteIndex;
			
			// these are in 1.15.16 fixed point format
			int32_t Filter [ NumOfFilters ];
			
			int32_t CircularBuffer [ BufferSize_PowerOfTwo ];
			
			static const uint32_t c_iMask = BufferSize_PowerOfTwo - 1;
			
		public:
			
			void Reset ()
			{
				WriteIndex = 0;
				for ( int i = 0; i < BufferSize_PowerOfTwo; i++ ) CircularBuffer [ i ] = 0;
			}
			
			LowPassFilter ()
			{
				Reset ();
			}
			
			void SetFilter ( int32_t* Coefficients )
			{
				for ( int i = 0; i < NumOfFilters; i++ ) Filter [ i ] = Coefficients [ i ];
			}
			
			int32_t ApplyFilter ( int32_t Sample )
			{
				int64_t i, j;
				int64_t Output = 0;
				
				CircularBuffer [ WriteIndex++ & c_iMask ] = Sample;
				
				for ( i = 0, j = WriteIndex - NumOfFilters; i < BufferSize_PowerOfTwo; i++, j++ )
				{
					Output += ( ((int64_t) CircularBuffer [ j & c_iMask ]) * ((int64_t) Filter [ i ]) );
				}
				
				Output >>= 16;
				
				return Output;
			}
		};
		
		
	}
}

#endif	// end #ifndef __LOWPASSFILTER_H__
