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
#ifndef __GENERALEMUDATASTRUCTURES_H__
#define __GENERALEMUDATASTRUCTURES_H__

//#include <iostream>

//using namespace std;


namespace Emulator
{
	namespace DataStructures
	{
	
		template <class T, const long long MaxElementsPowerOfTwo>
		class Stack
		{
			long long ReadWriteIndex;
			T Elements [ MaxElementsPowerOfTwo ];
			
			static const unsigned long c_iMask = MaxElementsPowerOfTwo - 1;
			
		public:
			
			void Reset () { ReadWriteIndex = -1; }
			
			Stack ()
			{
				//cout << "\nStack::Stack";
				
				Reset ();
			}
			
			bool IsEmpty () { return ReadWriteIndex == -1; }
			bool IsFull () { return ReadWriteIndex >= ( MaxElementsPowerOfTwo - 1 ); }
			long Size () { return ReadWriteIndex + 1; }
			
			T* Peek ()
			{
				if ( !IsEmpty () )
				{
					return & ( Elements [ ReadWriteIndex & c_iMask ] );
				}
				
				return NULL;
			}
			
			bool Push ( T Element )
			{
				//cout << "\nStack::Push";
				if ( !IsFull () )
				{
					// stack is not full //
					//cout << "; NOT full";
					Elements [ ++ReadWriteIndex & c_iMask ] = Element;
					
					return true;
				}
				
				// stack is full //
				//cout << "; IS full";
				return false;
			}
			
			T Pop ()
			{
				//cout << "\nStack::Pop";
				if ( !IsEmpty () )
				{
					//cout << "; NOT empty";
					return Elements [ ReadWriteIndex-- & c_iMask ];
				}
				
				//cout << "; IS empty";
			}
			
		};
		
		
		template <class T, const unsigned long long MaxElementsPowerOfTwo>
		class FIFO
		{
			unsigned long long ReadIndex, WriteIndex;
			T Elements [ MaxElementsPowerOfTwo ];
			
			static const unsigned long c_iMask = MaxElementsPowerOfTwo - 1;
			
		public:
			
			void Reset () { ReadIndex = 0; WriteIndex = 0; }
			
			FIFO () { Reset (); }
			
			bool IsEmpty () { return ReadIndex >= WriteIndex; }
			bool IsFull () { return ( ( WriteIndex > ReadIndex ) && ( ( WriteIndex - ReadIndex ) >= MaxElementsPowerOfTwo ) ); }
			long Size () { return WriteIndex - ReadIndex; }
			
			T* Peek ()
			{
				if ( !IsEmpty () )
				{
					return & ( Elements [ ReadIndex & c_iMask ] );
				}
				
				return NULL;
			}
			
			T* Add ( T Element )
			{
				T* ptr = 0;
				
				if ( !IsFull () )
				{
					// stack is not full //
					ptr = & (Elements [ WriteIndex & c_iMask ]);	// = Element;
					WriteIndex++;
					
					*ptr = Element;
					
					return ptr;
				}
				
				// stack is full //
				return 0;
			}
			
			T Remove ()
			{
				if ( !IsEmpty () )
				{
					return Elements [ ReadIndex++ & c_iMask ];
				}
			}
		};
	}
}

#endif	// end #ifndef __GENERALEMUDATASTRUCTURES_H__
