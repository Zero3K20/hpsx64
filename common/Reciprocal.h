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
#ifndef __RECIPROCAL_H__
#define __RECIPROCAL_H__

namespace Math
{
	namespace Reciprocal
	{
		// take reciprocal of double
		static inline double INVD ( double a ) { return 1.0L / a; }
		
		// take integer part of double-precision floating point value
		static inline long long INTD ( double a ) { return (long long) a; }
		
		// attempt to speed up modulo via reciprocal multiplication
		static inline double RMOD ( double numerator, double denominator, double reciprocal )
		{
			double quotient;
			quotient = numerator * reciprocal;
			return denominator * ( quotient - (long long) quotient );
		}
		
		// take ceil of double
		static inline long long CEILD ( double a )
		{
			long long integer;
			integer = (long long) a;
			return ( ( ( a - integer ) == 0.0L ) ? integer : ( integer + 1 ) );
		}
		
	};
};

#endif	// end #ifndef __RECIPROCAL_H__
