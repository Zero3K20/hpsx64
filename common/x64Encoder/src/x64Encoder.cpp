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


#ifndef _WINDOWS_H_
#define _WINDOWS_H_
#include <windows.h>
#endif


#include "x64Encoder.h"
#include <iostream>

using namespace std;


#define USE_BACKEND_RECOMPILER_CLASS


#define FAST_ENCODE_VALUE16
#define FAST_ENCODE_VALUE32
#define FAST_ENCODE_VALUE64


x64Encoder::x64Encoder(int32_t SizePerCodeBlock_PowerOfTwo, int32_t CountOfCodeBlocks)
	: RecompilerBackend(SizePerCodeBlock_PowerOfTwo, CountOfCodeBlocks)
{
	int32_t i;
	int32_t lSpaceToAllocate;
	int64_t Ret;

	// we're not ready yet
	isEncoding = true;
	isReadyForUse = false;

	lNumberOfCodeBlocks = CountOfCodeBlocks;
	//lCodeBlockSize = SizePerCodeBlock;
	lCodeBlockSize_PowerOfTwo = SizePerCodeBlock_PowerOfTwo;
	lCodeBlockSize = 1 << SizePerCodeBlock_PowerOfTwo;
	lCodeBlockSize_Mask = lCodeBlockSize - 1;


	// get the amount of space we need to allocate
	lSpaceToAllocate = lCodeBlockSize * lNumberOfCodeBlocks;


#ifdef VERBOSE_X64ENCODER_ALLOC
	// need to allocate executable memory area
	cout << "\nX64: RECOMPILER: Allocatting virtual space (bytes): " << dec << lSpaceToAllocate;
#endif


#ifdef USE_BACKEND_RECOMPILER_CLASS

	LiveCodeArea = reinterpret_cast<char*>(GetBlockBase(0));
#else

	LiveCodeArea = (char*)VirtualAlloc(NULL, lSpaceToAllocate, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	VirtualLock((LPVOID)LiveCodeArea, lSpaceToAllocate);
#endif


	if (!LiveCodeArea)
	{
		cout << "\nx64Encoder: ERROR: There was an error allocating executable area.\n";
	}
	else
	{

#ifdef VERBOSE_X64ENCODER_ALLOC
		cout << "\nx64Encoder: Executable area allocated successfully. Address=" << hex << (unsigned int64_t) LiveCodeArea << "\n";
#endif

	}

	// set location of where code will be decoded to
	//LiveCodeArea = CodeBlocksStartAddress;

	if (!x64CodeArea)
	{
		// there was an error during initialization
		isReadyForUse = false;
	}
	else
	{
		isReadyForUse = true;
	}

	// set the amount of space allocated
	x64TotalSpaceAllocated = lSpaceToAllocate;

	// clear stuff for jumping
	for (i = 0; i < 256; i++) BranchOffset[i] = -1;
	//	JmpOffset = -1;
	//	BranchOffset = -1;
	//	JmpOffsetEndCount = -1;
	//	BranchOffsetEndCount = -1;


		// we're done with initializing and can use encoder when we need
	isEncoding = false;
}


x64Encoder::~x64Encoder ( void )
{

#ifndef USE_BACKEND_RECOMPILER_CLASS

	// ** Start Platform Dependent Code ** //
	VirtualUnlock ( (LPVOID) LiveCodeArea, lCodeBlockSize * lNumberOfCodeBlocks );
	VirtualFree( (LPVOID) LiveCodeArea, NULL, MEM_RELEASE );
	// ** End Platform Dependent Code ** //
#endif

}

bool x64Encoder::FlushCurrentCodeBlock ( void )
{

#ifndef USE_BACKEND_RECOMPILER_CLASS

	return FlushCodeBlock ( x64CurrentCodeBlockIndex );
#endif

	return true;
}

bool x64Encoder::FlushCodeBlock ( int32_t IndexOfCodeBlock )
{
	int64_t CurrentCodeBlockAddress;
	
	CurrentCodeBlockAddress = ( ( (int64_t) x64CodeArea ) + ( (int64_t) ( IndexOfCodeBlock * lCodeBlockSize ) ) );

	return FlushICache ( CurrentCodeBlockAddress, lCodeBlockSize ); 
}

bool x64Encoder::FlushICache ( int64_t baseAddress, int32_t NumberOfBytes )
{

#ifndef USE_BACKEND_RECOMPILER_CLASS

	// ** Start Platform Dependent Code ** //
	if ( FlushInstructionCache( GetCurrentProcess (), (LPCVOID) baseAddress, NumberOfBytes ) == 0 ) return false;
	// ** End Platform Dependent Code ** //
#endif
	
	return true;
}

int32_t x64Encoder::GetCurrentInstructionOffset ( void )
{
	return x64NextOffset;
}

void x64Encoder::SetCurrentInstructionOffset ( int32_t offset )
{
	x64NextOffset = offset;
}

// get the size of a code block for encoder - should help with determining if there is more space available
int32_t x64Encoder::GetCodeBlockSize ( void )
{
	return lCodeBlockSize;
}


char* x64Encoder::Get_CodeBlock_StartPtr ()
{
	// get offset for start of code block
	return & x64CodeArea [ lCodeBlockSize * x64CurrentCodeBlockIndex ];
}

char* x64Encoder::Get_CodeBlock_EndPtr ()
{
	// get offset for end of code block
	return & x64CodeArea [ ( lCodeBlockSize * ( x64CurrentCodeBlockIndex + 1 ) ) - 1 ];
}

// get the start pointer for an arbitrary code block number
char* x64Encoder::Get_XCodeBlock_StartPtr ( int32_t lCodeBlockIndex )
{
	// make sure code block number is valid
	if ( lCodeBlockIndex < lNumberOfCodeBlocks )
	{
		// get offset for start of code block specified
		return & x64CodeArea [ lCodeBlockSize * lCodeBlockIndex ];
	}
	
	// must be an invalid code block index
	return NULL;
}


char* x64Encoder::Get_CodeBlock_CurrentPtr ()
{
	return & x64CodeArea [ x64NextOffset ];
}



bool x64Encoder::StartCodeBlock ( int32_t lCodeBlockIndex )
{

#ifdef USE_BACKEND_RECOMPILER_CLASS

	// let the backend know encoding has started
	StartEncoding(lCodeBlockIndex);
#endif
	
	// we have started encoding in an x64 code block
	isEncoding = true;
	
	
	// get the index for the code block to put x64 code in
	//lCodeBlockIndex = ( lSourceStartAddress & ( lNumberOfCodeBlocks - 1 ) );
	
	// set entry in hash table
	//x64CodeHashTable [ lCodeBlockIndex ] = lSourceStartAddress;
	
	// set the current source address
	//x64CurrentSourceAddress = lSourceStartAddress;
	
	// get offset for start of code block
	x64NextOffset = lCodeBlockSize * lCodeBlockIndex;
	
	// get offset for start of current instruction
	x64CurrentStartOffset = x64NextOffset;
	
	// also need to set the current code block index
	x64CurrentCodeBlockIndex = lCodeBlockIndex;
	
	// adding this so it works with older code
	x64CodeArea = LiveCodeArea;
	
	
	// clear stuff for jumping
//	JmpOffset = -1;
//	BranchOffset = -1;
//	JmpOffsetEndCount = -1;
//	BranchOffsetEndCount = -1;

	// no problems here
	return true;
}



bool x64Encoder::EndCodeBlock ( void )
{
#ifdef USE_BACKEND_RECOMPILER_CLASS

	// update the encoding position
	SetEncodingPosition(x64CurrentCodeBlockIndex, reinterpret_cast<uint8_t*>(&(x64CodeArea[x64NextOffset])));

	// let the backend know encoding has ended
	EndEncoding(x64CurrentCodeBlockIndex);
#else
	
	// we have to flush the instruction cache for code block before we can use it
	// ??
	FlushCurrentCodeBlock ();
#endif
	
	// not currently encoding a code block
	isEncoding = false;

	return true;
}



int32_t x64Encoder::x64CurrentCodeBlockSpaceRemaining ( void )
{
	int32_t lNextCodeBlockOffset, lRemainingBytes;
	
	lNextCodeBlockOffset = lCodeBlockSize * ( x64CurrentCodeBlockIndex + 1 );
	
	lRemainingBytes = lNextCodeBlockOffset - x64NextOffset;
	
	return lRemainingBytes;
	
}


bool x64Encoder::StartInstructionBlock ( void )
{
	x64CurrentSourceCpuInstructionStartOffset = x64NextOffset;
	
	return true;
}



bool x64Encoder::EndInstructionBlock ()
{
	// advance the source cpu instruction pointer
	//x64CurrentSourceAddress += lSourceCpuInstructionSize;
	
	return true;
}



int32_t x64Encoder::x64GetBookmark8 ( void )
{
	int32_t lBookmark;

	lBookmark = x64NextOffset;

	// also advance to where we will start to insert the next instruction
	x64NextOffset++;
	
	return lBookmark;
}


bool x64Encoder::x64SetBookmark8 ( int32_t Bookmark, char value )
{
	x64CodeArea [ Bookmark ] = value;
	
	return true;
}



bool x64Encoder::UndoInstructionBlock ( void )
{
	x64NextOffset = x64CurrentSourceCpuInstructionStartOffset;
	
	return true;
}

bool x64Encoder::x64EncodeImmediate8 ( char Imm8 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 1 ) return false;

	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( Imm8 );

	return true;
}

bool x64Encoder::x64EncodeImmediate16 ( int16_t Imm16 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

#ifdef FAST_ENCODE_VALUE16
	*((int16_t*)(x64CodeArea+x64NextOffset)) = Imm16;
	x64NextOffset += 2;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm16 & 0xff );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( ( Imm16 >> 8 ) & 0xff );
#endif
	
	return true;
}

bool x64Encoder::x64EncodeImmediate32 ( int32_t Imm32 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 4 ) return false;

#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = Imm32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm32 & 0xff );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( ( Imm32 >> 8 ) & 0xff );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( ( Imm32 >> 16 ) & 0xff );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( ( Imm32 >> 24 ) & 0xff );
#endif
	
	return true;
}

bool x64Encoder::x64EncodeImmediate64 ( int64_t Imm64 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 8 ) return false;

#ifdef FAST_ENCODE_VALUE64
	*((int64_t*)(x64CodeArea+x64NextOffset)) = Imm64;
	x64NextOffset += 8;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 24 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 40 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 48 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Imm64 >> 56 );
#endif
	
	return true;
}

inline bool x64Encoder::x64Encode16Bit ( void )
{
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	x64CodeArea [ x64NextOffset++ ] = PREFIX_16BIT;
	
	return true;
}


inline bool x64Encoder::x64EncodePrefix ( char Prefix )
{
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	x64CodeArea [ x64NextOffset++ ] = Prefix;
	
	return true;
}



bool x64Encoder::x64EncodeRexReg32 ( int32_t DestReg_RM_Base, int32_t SourceReg_Reg_Opcode )
{
	// check if any of the registers require rex opcode
	if ( DestReg_RM_Base > 7 || SourceReg_Reg_Opcode > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, SourceReg_Reg_Opcode, 0, DestReg_RM_Base );
	}
	
	return true;
}

bool x64Encoder::x64EncodeRexReg64 ( int32_t DestReg_RM_Base, int32_t SourceReg_Reg_Opcode )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	x64CodeArea [ x64NextOffset++ ] = MAKE_REX( OPERAND_64BIT, SourceReg_Reg_Opcode, 0, DestReg_RM_Base );
	
	return true;
}


bool x64Encoder::x64EncodeMovImm64 ( int32_t x64DestRegIndex, int64_t Immediate64 )
{
	x64EncodeRexReg64 ( x64DestRegIndex, 0 );
	
	x64EncodeOpcode ( X64BOP_MOV_IMM + ( x64DestRegIndex & 7 ) );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 8 ) return false;
	
#ifdef FAST_ENCODE_VALUE64
	*((int64_t*)(x64CodeArea+x64NextOffset)) = Immediate64;
	x64NextOffset += 8;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 24 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 40 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 48 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate64 >> 56 );
#endif

	return true;
}


bool x64Encoder::x64EncodeMovImm32 ( int32_t x64DestRegIndex, int32_t Immediate32 )
{
	x64EncodeRexReg32 ( x64DestRegIndex, 0 );

	x64EncodeOpcode ( X64BOP_MOV_IMM + ( x64DestRegIndex & 7 ) );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 4 ) return false;
	
#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = Immediate32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate32 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate32 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate32 >> 24 );
#endif
	
	return true;
}

bool x64Encoder::x64EncodeMovImm16 ( int32_t x64DestRegIndex, int16_t Immediate16 )
{
	x64EncodeRexReg32 ( x64DestRegIndex, 0 );

	x64EncodeOpcode ( X64BOP_MOV_IMM + ( x64DestRegIndex & 7 ) );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;
	
#ifdef FAST_ENCODE_VALUE16
	*((int16_t*)(x64CodeArea+x64NextOffset)) = Immediate16;
	x64NextOffset += 2;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate16 >> 8 );
#endif
	
	return true;
}

bool x64Encoder::x64EncodeMovImm8 ( int32_t x64DestRegIndex, int32_t Immediate8 )
{
	x64EncodeRexReg32 ( x64DestRegIndex, 0 );
	
	x64EncodeOpcode ( X64BOP_MOV_IMM8 + ( x64DestRegIndex & 7 ) );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// now we need to drop in the immediate
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( Immediate8 );
	
	return true;
}


bool x64Encoder::x64Encode16 ( int32_t x64InstOpcode )
{
	x64Encode16Bit ();
	return x64EncodeOpcode ( x64InstOpcode );
}

bool x64Encoder::x64Encode32 ( int32_t x64InstOpcode )
{
	return x64EncodeOpcode ( x64InstOpcode );
}

bool x64Encoder::x64Encode64 ( int32_t x64InstOpcode )
{
	x64CodeArea [ x64NextOffset++ ] = CREATE_REX( 1, 0, 0, 0 );
	return x64EncodeOpcode ( x64InstOpcode );
}


bool x64Encoder::x64EncodeReg16 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg )
{
	x64Encode16Bit ();
	return x64EncodeReg32 ( x64InstOpcode, ModRMOpcode, x64Reg );
}

bool x64Encoder::x64EncodeReg32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg )
{
	x64EncodeRexReg32 ( x64Reg, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// now we need to say what registers to use for instruction
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( ModRMOpcode, x64Reg );
	
	return true;
}



bool x64Encoder::x64EncodeReg64 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg )
{
	x64EncodeRexReg64 ( x64Reg, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// now we need to say what registers to use for instruction
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( ModRMOpcode, x64Reg );
	
	return true;
}

bool x64Encoder::x64EncodeRegReg16 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base )
{
	x64Encode16Bit ();
	return x64EncodeRegReg32 ( x64InstOpcode, x64DestReg_Reg_Opcode, x64SourceReg_RM_Base );
}


bool x64Encoder::x64EncodeRegReg32 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base )
{
	// need to reverse source and dest since I used a certain form of regreg instructions
	x64EncodeRexReg32 ( x64SourceReg_RM_Base, x64DestReg_Reg_Opcode );
	//x64EncodeRexReg32 ( x64DestReg_Reg_Opcode, x64SourceReg_RM_Base );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// now we need to say what registers to use for instruction
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( x64DestReg_Reg_Opcode, x64SourceReg_RM_Base );
	
	return true;
}


bool x64Encoder::x64EncodeRegReg64 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base )
{
	// need to reverse source and dest since I used a certain form of regreg instructions
	x64EncodeRexReg64 ( x64SourceReg_RM_Base, x64DestReg_Reg_Opcode );
	//x64EncodeRexReg64 ( x64DestReg_Reg_Opcode, x64SourceReg_RM_Base );

	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// now we need to say what registers to use for instruction
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( x64DestReg_Reg_Opcode, x64SourceReg_RM_Base );

	return true;
}



bool x64Encoder::x64EncodeReturn ( void )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	x64CodeArea [ x64NextOffset++ ] = X64OP_RET;
	
	return true;
}




bool x64Encoder::x64EncodeReg8Imm8(int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int8_t cImmediate8)
{
	x64EncodeRexReg32(x64DestReg, 0);

	x64EncodeOpcode(x64InstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 2) return false;

	// add in modr/m
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGOP(REGREG, x64DestReg, ModRMOpcode);

	// now we need to drop in the immediate, but backwards
	x64CodeArea[x64NextOffset++] = (char)(cImmediate8);

	return true;
}
bool x64Encoder::x64EncodeReg16Imm16 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int16_t cImmediate16 )
{
	x64Encode16Bit ();

	x64EncodeRexReg32 ( x64DestReg, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg, ModRMOpcode );

#ifdef FAST_ENCODE_VALUE16
	*((int16_t*)(x64CodeArea+x64NextOffset)) = cImmediate16;
	x64NextOffset += 2;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate16 >> 8 );
#endif

	return true;
}


bool x64Encoder::x64EncodeReg32Imm32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int32_t cImmediate32 )
{
	x64EncodeRexReg32 ( x64DestReg, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 5 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg, ModRMOpcode );

#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = cImmediate32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 24 );
#endif

	return true;
}


bool x64Encoder::x64EncodeReg64Imm32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int32_t cImmediate32 )
{
	x64EncodeRexReg64 ( x64DestReg, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 5 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg, ModRMOpcode );

#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = cImmediate32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 24 );
#endif
	
	return true;
}


bool x64Encoder::x64EncodeAcc8Imm8(int32_t x64InstOpcode, int8_t cImmediate8)
{
	x64EncodeOpcode(x64InstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 1) return false;

	// now we need to drop in the immediate, but backwards
	x64CodeArea[x64NextOffset++] = (char)(cImmediate8);

	return true;
}
bool x64Encoder::x64EncodeAcc16Imm16 ( int32_t x64InstOpcode, int16_t cImmediate16 )
{
	x64Encode16Bit ();
	x64EncodeOpcode ( x64InstOpcode );
	
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

#ifdef FAST_ENCODE_VALUE16
	*((int16_t*)(x64CodeArea+x64NextOffset)) = cImmediate16;
	x64NextOffset += 2;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate16 >> 8 );
#endif

	return true;
}

bool x64Encoder::x64EncodeAcc32Imm32 ( int32_t x64InstOpcode, int32_t cImmediate32 )
{
	x64EncodeOpcode ( x64InstOpcode );
	
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 4 ) return false;

#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = cImmediate32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 24 );
#endif

	return true;
}

bool x64Encoder::x64EncodeAcc64Imm32 ( int32_t x64InstOpcode, int32_t cImmediate32 )
{
	x64EncodeRexReg64 ( 0, 0 );
	x64EncodeOpcode ( x64InstOpcode );
	
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 4 ) return false;

#ifdef FAST_ENCODE_VALUE32
	*((int32_t*)(x64CodeArea+x64NextOffset)) = cImmediate32;
	x64NextOffset += 4;
#else
	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 8 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 16 );
	x64CodeArea [ x64NextOffset++ ] = ( char ) ( cImmediate32 >> 24 );
#endif

	return true;
}





bool x64Encoder::x64EncodeRegMem16 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	x64Encode16Bit ();
	return x64EncodeRegMem32 ( x64InstOpcode, x64DestReg, BaseAddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::x64EncodeRegMem32 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// check if any of the registers require rex opcode
	if ( x64DestReg > 7 || BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, x64DestReg, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( x64DestReg, BaseAddressReg, IndexReg, Scale, Offset );

	return true;
}

bool x64Encoder::x64EncodeRegMem64 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
	x64CodeArea [ x64NextOffset++ ] = MAKE_REX( OPERAND_64BIT, x64DestReg, IndexReg, BaseAddressReg );
	
	x64EncodeOpcode ( x64InstOpcode );
	
	return x64EncodeMem ( x64DestReg, BaseAddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::x64EncodeMem16Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	x64Encode16Bit ();

	// check if any of the registers require rex opcode
	if ( BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, 0, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( ModRMOpcode, BaseAddressReg, IndexReg, Scale, Offset );
	
	// encode immediate backwards
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::x64EncodeMem32Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	// check if any of the registers require rex opcode
	if ( BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, 0, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( ModRMOpcode, BaseAddressReg, IndexReg, Scale, Offset );

	// encode immediate backwards
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::x64EncodeMem64Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
	x64CodeArea [ x64NextOffset++ ] = MAKE_REX( OPERAND_64BIT, 0, IndexReg, BaseAddressReg );
	
	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( ModRMOpcode, BaseAddressReg, IndexReg, Scale, Offset );

	// encode immediate backwards
	return x64EncodeImmediate8 ( Imm8 );
}



bool x64Encoder::x64EncodeMemImm8 ( int32_t x64InstOpcode, int32_t Mod, char Imm8, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// check if any of the registers require rex opcode
	if ( BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, 0, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( Mod, BaseAddressReg, IndexReg, Scale, Offset );
	
	// encode immediate backwards
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::x64EncodeMemImm16 ( int32_t x64InstOpcode, int32_t Mod, int16_t Imm16, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	x64Encode16Bit ();

	// check if any of the registers require rex opcode
	if ( BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, 0, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( Mod, BaseAddressReg, IndexReg, Scale, Offset );
	
	// encode immediate backwards
	return x64EncodeImmediate16 ( Imm16 );
}

bool x64Encoder::x64EncodeMemImm32 ( int32_t x64InstOpcode, int32_t Mod, int32_t Imm32, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// check if any of the registers require rex opcode
	if ( BaseAddressReg > 7 || IndexReg > 7 )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
		x64CodeArea [ x64NextOffset++ ] = MAKE_REX( 0, 0, IndexReg, BaseAddressReg );
	}

	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( Mod, BaseAddressReg, IndexReg, Scale, Offset );

	// encode immediate backwards
	return x64EncodeImmediate32 ( Imm32 );
}

bool x64Encoder::x64EncodeMemImm64 ( int32_t x64InstOpcode, int32_t Imm32, int32_t Mod, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
	x64CodeArea [ x64NextOffset++ ] = MAKE_REX( OPERAND_64BIT, 0, IndexReg, BaseAddressReg );
	
	x64EncodeOpcode ( x64InstOpcode );
	
	x64EncodeMem ( Mod, BaseAddressReg, IndexReg, Scale, Offset );

	// encode immediate backwards
	return x64EncodeImmediate32 ( Imm32 );
}

bool x64Encoder::x64EncodeRegMem32S ( int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t avxDestSrcReg, int32_t avxBaseReg, int32_t avxIndexReg, int32_t Scale, int32_t Offset )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( avxDestSrcReg, avxBaseReg, avxIndexReg, mmmmm);
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( 0, 0, 0, pp);

	x64EncodeOpcode ( avxInstOpcode );

	x64EncodeMem ( avxDestSrcReg, avxBaseReg, avxIndexReg, Scale, Offset );

	return true;
}

bool x64Encoder::x64EncodeRegMemV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R_Dest, int32_t vvvv, int32_t x64RM_B_Base, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R_Dest, x64IndexReg, x64RM_B_Base, mmmmm);
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, vvvv, L, pp);

	x64EncodeOpcode ( avxInstOpcode );

	x64EncodeMem ( REG_R_Dest, x64RM_B_Base, x64IndexReg, Scale, Offset );

	return true;
}

bool x64Encoder::x64EncodeRegMem256 ( int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t avxDestSrcReg, int32_t avxBaseReg, int32_t avxIndexReg, int32_t Scale, int32_t Offset )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( avxDestSrcReg, avxIndexReg, avxBaseReg, mmmmm);
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( 0, 0, VEX_256BIT, pp);

	x64EncodeOpcode ( avxInstOpcode );

	x64EncodeMem ( avxDestSrcReg, avxBaseReg, avxIndexReg, Scale, Offset );

	return true;
}

bool x64Encoder::x64EncodeReg16Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 )
{
	x64Encode16Bit ();
	
	x64EncodeRexReg32 ( x64DestReg_RM_Base, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg_RM_Base, ModRMOpcode );

	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = cImmediate8;

	return true;
}

bool x64Encoder::x64EncodeReg32Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 )
{
	x64EncodeRexReg32 ( x64DestReg_RM_Base, 0 );
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg_RM_Base, ModRMOpcode );

	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = cImmediate8;

	return true;
}


bool x64Encoder::x64EncodeReg64Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 )
{
	x64EncodeRexReg64 ( x64DestReg_RM_Base, 0 );

	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGOP( REGREG, x64DestReg_RM_Base, ModRMOpcode );

	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = cImmediate8;
	
	return true;
}


bool x64Encoder::x64EncodeRegRegEVImm8(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, char cImmediate8, int32_t z, int32_t k)
{
	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 4) return false;

	// add in evex 4-byte prefix
	x64CodeArea[x64NextOffset++] = EVEX_START_4BYTE;
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P0(REG_R, RM_B, RM_B, mmm);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P1(w, vvvv, pp);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P2(z, L, 0, vvvv, k);

	// encode the opcode
	x64EncodeOpcode(avxInstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 2) return false;

	// now we need to say what registers to use for instruction
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGREG(REG_R, RM_B);

	x64CodeArea[x64NextOffset++] = cImmediate8;

	return true;
}

bool x64Encoder::x64EncodeRegVImm8 ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, char cImmediate8 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R, 0, RM_B, mmmmm );
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, vvvv, L, pp );
	
	x64EncodeOpcode ( avxInstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	// now we need to drop in the immediate, but backwards
	x64CodeArea [ x64NextOffset++ ] = cImmediate8;
	
	return true;
}

bool x64Encoder::x64EncodeRegMemVImm8 ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R_Dest, int32_t vvvv, int32_t x64RM_B_Base, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R_Dest, x64IndexReg, x64RM_B_Base, mmmmm);
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, vvvv, L, pp);

	x64EncodeOpcode ( avxInstOpcode );

	x64EncodeMem ( REG_R_Dest, x64RM_B_Base, x64IndexReg, Scale, Offset );

	if ( x64CurrentCodeBlockSpaceRemaining() < 1 ) return false;

	x64CodeArea [ x64NextOffset++ ] = Imm8;

	return true;
}


/*
bool x64Encoder::x64EncodeRegMemV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t avxDestReg, int32_t avxSourceReg, int32_t avxBaseReg, int32_t avxIndexReg, int32_t Scale, int32_t Offset )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( avxDestReg, avxIndexReg, 0, mmmmm);
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, avxSourceReg, L, pp);

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	x64EncodeOpcode ( avxInstOpcode );

	x64EncodeMem ( avxDestReg, avxBaseReg, avxIndexReg, Scale, Offset );

	return true;
}
*/

bool x64Encoder::x64EncodeRegRegEV(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, int32_t z, int32_t k, int32_t rnd_enable, int32_t rnd_type)
{
	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 4) return false;

	// add in evex 4-byte prefix
	x64CodeArea[x64NextOffset++] = EVEX_START_4BYTE;
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P0(REG_R, RM_B, RM_B, mmm);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P1(w, vvvv, pp);

	if (!rnd_enable)
	{
		x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P2(z, L, 0, vvvv, k);
	}
	else
	{
		// static rounding enabled //
		x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P2(z, rnd_type, rnd_enable, vvvv, k);

	}

	// encode the opcode
	x64EncodeOpcode(avxInstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() <= 0) return false;

	// now we need to say what registers to use for instruction
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGREG(REG_R, RM_B);

	return true;
}


bool x64Encoder::x64EncodeRegRegV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R, 0, RM_B, mmmmm );
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, vvvv, L, pp);

	// encode the opcode
	x64EncodeOpcode ( avxInstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// now we need to say what registers to use for instruction
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	return true;
}

bool x64Encoder::x64EncodeRipOffsetEV(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, int32_t z, int32_t k, int32_t broadcast)
{
	int32_t Offset;

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 3) return false;

	// add in vex 3-byte prefix
	x64CodeArea[x64NextOffset++] = EVEX_START_4BYTE;
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P0(REG_R, 0, 0, mmm);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P1(w, vvvv, pp);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P2(z, L, broadcast, vvvv, k);
	//x64CodeArea[x64NextOffset++] = VEX_START_3BYTE;
	//x64CodeArea[x64NextOffset++] = VEX_3BYTE_MID(REG_R, 0, 0, mmmmm);
	//x64CodeArea[x64NextOffset++] = VEX_3BYTE_END(w, vvvv, L, pp);

	// encode the opcode
	x64EncodeOpcode(avxInstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() <= 0) return false;

	// now we need to say what registers to use for instruction
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	// add in modr/m
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGMEM(REGMEM_NOOFFSET, REG_R, RMBASE_USINGRIP);

	// get offset to data
	Offset = (int32_t)(DataAddress - (&(x64CodeArea[x64NextOffset + 4])));

	//cout << hex << "\nOffset=" << Offset << "\n";

	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::x64EncodeRipOffsetV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress )
{
	int32_t Offset;
	
	// will leave rex opcode off for now
	/*
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	*/

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() < 3 ) return false;

	// add in vex 3-byte prefix
	x64CodeArea [ x64NextOffset++ ] = VEX_START_3BYTE;
	//x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R, 0, RM_B, mmmmm );
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R, 0, 0, mmmmm );
	x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_END( w, vvvv, L, pp);

	// encode the opcode
	x64EncodeOpcode ( avxInstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// now we need to say what registers to use for instruction
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, REG_R, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 4 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	return x64EncodeImmediate32 ( Offset );
}

bool x64Encoder::x64EncodeRipOffsetEVImm8(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, char Imm8, int32_t z, int32_t k, int32_t broadcast)
{
	int32_t Offset;

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 3) return false;

	// add in vex 3-byte prefix
	x64CodeArea[x64NextOffset++] = EVEX_START_4BYTE;
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P0(REG_R, 0, 0, mmm);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P1(w, vvvv, pp);
	x64CodeArea[x64NextOffset++] = EVEX_4BYTE_P2(z, L, broadcast, vvvv, k);
	//x64CodeArea[x64NextOffset++] = VEX_START_3BYTE;
	//x64CodeArea[x64NextOffset++] = VEX_3BYTE_MID(REG_R, 0, 0, mmmmm);
	//x64CodeArea[x64NextOffset++] = VEX_3BYTE_END(w, vvvv, L, pp);

	// encode the opcode
	x64EncodeOpcode(avxInstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() <= 0) return false;

	// now we need to say what registers to use for instruction
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	// add in modr/m
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGMEM(REGMEM_NOOFFSET, REG_R, RMBASE_USINGRIP);

	// get offset to data
	Offset = (int32_t)(DataAddress - (&(x64CodeArea[x64NextOffset + 5])));

	//cout << hex << "\nOffset=" << Offset << "\n";

	x64EncodeImmediate32(Offset);
	return x64EncodeImmediate8(Imm8);
}

bool x64Encoder::x64EncodeRipOffsetVImm8(int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, char Imm8)
{
	int32_t Offset;

	// will leave rex opcode off for now
	/*
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	*/

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() < 3) return false;

	// add in vex 3-byte prefix
	x64CodeArea[x64NextOffset++] = VEX_START_3BYTE;
	//x64CodeArea [ x64NextOffset++ ] = VEX_3BYTE_MID( REG_R, 0, RM_B, mmmmm );
	x64CodeArea[x64NextOffset++] = VEX_3BYTE_MID(REG_R, 0, 0, mmmmm);
	x64CodeArea[x64NextOffset++] = VEX_3BYTE_END(w, vvvv, L, pp);

	// encode the opcode
	x64EncodeOpcode(avxInstOpcode);

	// make sure there is enough room for this
	if (x64CurrentCodeBlockSpaceRemaining() == 0) return false;

	// now we need to say what registers to use for instruction
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGREG( REG_R, RM_B );

	// add in modr/m
	x64CodeArea[x64NextOffset++] = MAKE_MODRMREGMEM(REGMEM_NOOFFSET, REG_R, RMBASE_USINGRIP);

	// get offset to data
	Offset = (int32_t)(DataAddress - (&(x64CodeArea[x64NextOffset + 5])));

	//cout << hex << "\nOffset=" << Offset << "\n";

	x64EncodeImmediate32(Offset);
	return x64EncodeImmediate8(Imm8);
}






bool x64Encoder::x64EncodeRipOffset16 ( int32_t x64InstOpcode, int32_t x64DestReg, char* DataAddress, bool bIsSourceReg )
{
	x64Encode16Bit ();
	return x64EncodeRipOffset32 ( x64InstOpcode, x64DestReg, DataAddress, bIsSourceReg );
}


// added the "bToMem" argument because it turns out that this was only handling cases where register is the destination register
bool x64Encoder::x64EncodeRipOffset32 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	

	x64EncodeOpcode ( x64InstOpcode );
	
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 4 ] ) ) );
	
	//cout << hex << "\nDataAddress=" << (unsigned int64_t)DataAddress;
	//cout << hex << "\nCodeAreaAddr=" << (unsigned int64_t)(&(x64CodeArea[x64NextOffset + 4]));
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	return x64EncodeImmediate32 ( Offset );
}




bool x64Encoder::x64EncodeRipOffset64 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg64 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg64 ( x64Reg, 0 );
	}
	
	/*
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// add rex opcode - we have to put the value in rex.r here since the register used in mov depends on the opcode
	x64CodeArea [ x64NextOffset++ ] = MAKE_REX( OPERAND_64BIT, x64DestReg, 0, 0 );
	*/

	x64EncodeOpcode ( x64InstOpcode );
	
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 4 ] ) ) );
	
	return x64EncodeImmediate32 ( Offset );
}


bool x64Encoder::x64EncodeRipOffsetImm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate8 ( Imm8 );
	// needs to be fixed
	/*
	int32_t Offset;
	
	x64EncodeRipOffset32 ( x64InstOpcode, x64DestReg, DataAddress );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate8 ( Imm8 );
	*/
}

bool x64Encoder::x64EncodeRipOffsetImm16 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int16_t Imm16, bool bIsSourceReg )
{
	int32_t Offset;
	
	x64Encode16Bit ();
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 6 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate16 ( Imm16 );
}

bool x64Encoder::x64EncodeRipOffsetImm32 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int32_t Imm32, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 8 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate32 ( Imm32 );
}

bool x64Encoder::x64EncodeRipOffsetImm64 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int32_t Imm32, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg64 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg64 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 8 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << "\n";
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate32 ( Imm32 );
}


// for instructions that take an IMM8 argument
bool x64Encoder::x64EncodeRipOffset16Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg )
{
	int32_t Offset;
	
	x64Encode16Bit ();
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate8 ( Imm8 );
}


bool x64Encoder::x64EncodeRipOffset32Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg32 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg32 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::x64EncodeRipOffset64Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg )
{
	int32_t Offset;
	
	if ( bIsSourceReg )
	{
		x64EncodeRexReg64 ( 0, x64Reg );
	}
	else
	{
		x64EncodeRexReg64 ( x64Reg, 0 );
	}
	
	x64EncodeOpcode ( x64InstOpcode );

	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;
	
	// add in modr/m
	x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64Reg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( DataAddress - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	
	x64EncodeImmediate32 ( Offset );
	return x64EncodeImmediate8 ( Imm8 );
}



inline bool x64Encoder::x64EncodeMem ( int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// check the size of the offset
	if ( Offset == 0 )
	{
	
		if ( IndexReg == NO_INDEX && ( BaseAddressReg & 0x7 ) != RMBASE_USINGSIB )
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64DestReg, BaseAddressReg );
		}
		else
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64DestReg, RMBASE_USINGSIB );

			// add in the sib byte
			// with no offset I guess there is no sib byte
			x64CodeArea [ x64NextOffset++ ] = MAKE_SIB( Scale, IndexReg, BaseAddressReg );
		}

	}
	else if ( Offset <= 127 && Offset >= -128 )
	{
		// we can use a single byte for the displacement here
		
		if ( IndexReg == NO_INDEX && ( BaseAddressReg & 0x7 ) != RMBASE_USINGSIB )
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_OFFSET8, x64DestReg, BaseAddressReg );
		}
		else
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_OFFSET8, x64DestReg, RMBASE_USINGSIB );

			// add in the sib byte
			// with no offset I guess there is no sib byte
			x64CodeArea [ x64NextOffset++ ] = MAKE_SIB( Scale, IndexReg, BaseAddressReg );
		}

		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// now we need to drop in the offset, but backwards
		x64CodeArea [ x64NextOffset++ ] = ( char ) ( Offset );

	}
	else
	{
		if ( IndexReg == NO_INDEX && ( BaseAddressReg & 0x7 ) != RMBASE_USINGSIB )
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_OFFSET32, x64DestReg, BaseAddressReg );
		}
		else
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() < 2 ) return false;

			// add in modr/m
			x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_OFFSET32, x64DestReg, RMBASE_USINGSIB );

			// add in the sib byte
			// with no offset I guess there is no sib byte
			x64CodeArea [ x64NextOffset++ ] = MAKE_SIB( Scale, IndexReg, BaseAddressReg );
		}

		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() < 4 ) return false;

		// now we need to drop in the immediate, but backwards
		x64CodeArea [ x64NextOffset++ ] = ( char ) ( Offset );
		x64CodeArea [ x64NextOffset++ ] = ( char ) ( Offset >> 8 );
		x64CodeArea [ x64NextOffset++ ] = ( char ) ( Offset >> 16 );
		x64CodeArea [ x64NextOffset++ ] = ( char ) ( Offset >> 24 );

	}
	
	return true;
}


inline bool x64Encoder::x64EncodeOpcode ( int32_t x64InstOpcode )
{
	// make sure there is enough room for this
	if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

	// add in opcode
	x64CodeArea [ x64NextOffset++ ] = GETOPCODE1( x64InstOpcode );
	
	// if there is another opcode, then add it in
	if ( GETOPCODE2( x64InstOpcode ) )
	{
		// make sure there is enough room for this
		if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

		// add in opcode
		x64CodeArea [ x64NextOffset++ ] = GETOPCODE2( x64InstOpcode );
		
		if ( GETOPCODE3( x64InstOpcode ) )
		{
			// make sure there is enough room for this
			if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

			// add in opcode
			x64CodeArea [ x64NextOffset++ ] = GETOPCODE3( x64InstOpcode );
			
			if ( GETOPCODE4( x64InstOpcode ) )
			{
				// make sure there is enough room for this
				if ( x64CurrentCodeBlockSpaceRemaining() == 0 ) return false;

				// add in opcode
				// note: needed to make 0xff represent 0x00 for the fourth byte in opcode for now
				if ( GETOPCODE4( x64InstOpcode ) == 0xff )
				{
					x64CodeArea [ x64NextOffset++ ] = 0;
				}
				else
				{
					x64CodeArea [ x64NextOffset++ ] = GETOPCODE4( x64InstOpcode );
				}
			}
		}
	}
	
	return true;
}




// **** x64 Instructions **** //


// ** popcnt ** //

bool x64Encoder::PopCnt32 ( int32_t DestReg, int32_t SrcReg )
{
	x64EncodePrefix ( X64OP1_POPCNT );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP2_POPCNT, X64OP3_POPCNT ), DestReg, SrcReg );
}


// ** mov ** //

bool x64Encoder::MovRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	if ( DestReg == SrcReg ) return true;	// if both regs are the same, then no need to encode instruction
	x64Encode16Bit ();
	return x64EncodeRegReg32 ( X64OP_MOV, DestReg, SrcReg );
}

bool x64Encoder::MovRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	if ( DestReg == SrcReg ) return true;	// if both regs are the same, then no need to encode instruction
	return x64EncodeRegReg32 ( X64OP_MOV, DestReg, SrcReg );
}

bool x64Encoder::MovRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	if ( DestReg == SrcReg ) return true;	// if both regs are the same, then no need to encode instruction
	return x64EncodeRegReg64 ( X64OP_MOV, DestReg, SrcReg );
}

bool x64Encoder::MovRegImm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeMovImm8 ( DestReg, Imm8 );
}

bool x64Encoder::MovRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	x64Encode16Bit ();
	return x64EncodeMovImm16 ( DestReg, Imm16 );
}

bool x64Encoder::MovRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeMovImm32 ( DestReg, Imm32 );
}

bool x64Encoder::MovRegImm64 ( int32_t DestReg, int64_t Imm64 )
{
	return x64EncodeMovImm64 ( DestReg, Imm64 );
}

bool x64Encoder::MovReg64Imm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_MOV_IMM, MODRM_MOV_IMM, DestReg, Imm32 );
}



bool x64Encoder::MovReg32ImmX ( int32_t DestReg, int32_t Imm32 )
{
	if ( !Imm32 )
	{
		return XorRegReg32 ( DestReg, DestReg );
	}
	
	return x64EncodeMovImm32 ( DestReg, Imm32 );
}

bool x64Encoder::MovReg64ImmX ( int32_t DestReg, int64_t Imm64 )
{
	if ( !Imm64 )
	{
		return XorRegReg32 ( DestReg, DestReg );
	}
	
	if ( Imm64 <= 0xffffffffULL )
	{
		return MovRegImm32 ( DestReg, Imm64 );
	}
	
	if ( Imm64 >= -0x80000000LL && Imm64 <= 0x7fffffffLL )
	{
		return MovReg64Imm32 ( DestReg, Imm64 );
	}
	
	return x64EncodeMovImm64 ( DestReg, Imm64 );
}


bool x64Encoder::MovRegToMem8 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_MOV_TOMEM8, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegFromMem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_MOV_FROMMEM8, DestReg, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::MovRegToMem16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// add in prefix to make this a 16-bit operation - this must come before REX prefix!
	x64CodeArea [ x64NextOffset++ ] = PREFIX_16BIT;

	return x64EncodeRegMem32 ( X64OP_MOV_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegFromMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	// add in prefix to make this a 16-bit operation - this must come before REX prefix!
	x64CodeArea [ x64NextOffset++ ] = PREFIX_16BIT;

	return x64EncodeRegMem32 ( X64OP_MOV_FROMMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegToMem32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_MOV_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegFromMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_MOV_FROMMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegToMem64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_MOV_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovRegFromMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_MOV_FROMMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}





// *** testing ***
bool x64Encoder::MovRegToMem8 ( char* Address, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_MOV_TOMEM8, SrcReg, (char*)Address, true );
}

bool x64Encoder::MovRegFromMem8 ( int32_t DestReg, char* Address )
{
	return x64EncodeRipOffset32 ( X64OP_MOV_FROMMEM8, DestReg, (char*)Address, true );
}

bool x64Encoder::MovRegToMem16 ( int16_t* Address, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_MOV_TOMEM, SrcReg, (char*) Address, true );
}

bool x64Encoder::MovRegFromMem16 ( int32_t DestReg, int16_t* Address )
{
	return x64EncodeRipOffset16 ( X64OP_MOV_FROMMEM, DestReg, (char*)Address, true );
}

bool x64Encoder::MovRegToMem32 ( int32_t* Address, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_MOV_TOMEM, SrcReg, (char*)Address, true );
}

bool x64Encoder::MovRegFromMem32 ( int32_t DestReg, int32_t* Address )
{
	return x64EncodeRipOffset32 ( X64OP_MOV_FROMMEM, DestReg, (char*)Address, true );
}

bool x64Encoder::MovRegToMem64 ( int64_t* Address, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_MOV_TOMEM, SrcReg, (char*)Address, true );
}

bool x64Encoder::MovRegFromMem64 ( int32_t DestReg, int64_t* Address )
{
	return x64EncodeRipOffset64 ( X64OP_MOV_FROMMEM, DestReg, (char*)Address, true );
}

bool x64Encoder::MovMemImm8 ( char* DestPtr, char Imm8 )
{
	return x64EncodeRipOffsetImm8 ( X64OP_MOV_IMM8, MODRM_MOV_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::MovMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_MOV_IMM, MODRM_MOV_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::MovMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_MOV_IMM, MODRM_MOV_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::MovMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_MOV_IMM, MODRM_MOV_IMM, (char*) DestPtr, Imm32 );
}



bool x64Encoder::MovMemImm8 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm8 ( X64OP_MOV_IMM8, MODRM_MOV_IMM8, Imm8, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_MOV_IMM, MODRM_MOV_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_MOV_IMM, MODRM_MOV_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_MOV_IMM, MODRM_MOV_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_ADD, DestReg, SrcReg );
}

bool x64Encoder::AddRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_ADD, DestReg, SrcReg );
}

bool x64Encoder::AddRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_ADD, DestReg, SrcReg );
}

bool x64Encoder::AddRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_ADD_IMM, MODRM_ADD_IMM, DestReg, Imm16 );
}

bool x64Encoder::AddRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_ADD_IMM, MODRM_ADD_IMM, DestReg, Imm32 );
}

bool x64Encoder::AddRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_ADD_IMM, MODRM_ADD_IMM, DestReg, Imm32 );
}


bool x64Encoder::AddRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_ADD, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_ADD, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_ADD, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_ADD_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_ADD_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_ADD_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_ADD_IMM, MODRM_ADD_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_ADD_IMM, MODRM_ADD_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AddMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_ADD_IMM, MODRM_ADD_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}




bool x64Encoder::AddRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_ADD, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AddRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_ADD, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AddRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_ADD, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AddMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_ADD_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AddMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_ADD_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AddMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_ADD_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AddMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::AddMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::AddMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, Imm32 );
}



bool x64Encoder::AddAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_ADD_IMMA, Imm16 );
}

bool x64Encoder::AddAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_ADD_IMMA, Imm32 );
}

bool x64Encoder::AddAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_ADD_IMMA, Imm32 );
}


bool x64Encoder::AddReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AddReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AddReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AddMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AddMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AddMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AddReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return IncReg16 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return DecReg16 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AddReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AddRegImm16 ( DestReg, ImmX );
	}
	
	return AddAcc16Imm16 ( ImmX );
}

bool x64Encoder::AddReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return IncReg32 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return DecReg32 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AddReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AddRegImm32 ( DestReg, ImmX );
	}
	
	return AddAcc32Imm32 ( ImmX );
}

bool x64Encoder::AddReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return IncReg64 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return DecReg64 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AddReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AddRegImm64 ( DestReg, ImmX );
	}
	
	return AddAcc64Imm32 ( ImmX );
}


bool x64Encoder::AddMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return IncMem16 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return DecMem16 ( DestPtr );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AddMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return IncMem32 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return DecMem32 ( DestPtr );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AddMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	
	if ( ImmX == 1 )
	{
		return IncMem64 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return DecMem64 ( DestPtr );
	}
	
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_ADD_IMM8, MODRM_ADD_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_ADD_IMM, MODRM_ADD_IMM, (char*) DestPtr, ImmX );
}






// ---------------------------


bool x64Encoder::AdcRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_ADC, DestReg, SrcReg );
}

bool x64Encoder::AdcRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_ADC, DestReg, SrcReg );
}

bool x64Encoder::AdcRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_ADC, DestReg, SrcReg );
}

bool x64Encoder::AdcRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_ADC_IMM, MODRM_ADC_IMM, DestReg, Imm16 );
}

bool x64Encoder::AdcRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_ADC_IMM, MODRM_ADC_IMM, DestReg, Imm32 );
}

bool x64Encoder::AdcRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_ADC_IMM, MODRM_ADC_IMM, DestReg, Imm32 );
}


bool x64Encoder::AdcRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_ADC, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_ADC, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_ADC, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_ADC_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_ADC_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_ADC_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_ADC_IMM, MODRM_ADC_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_ADC_IMM, MODRM_ADC_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AdcMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_ADC_IMM, MODRM_ADC_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}




bool x64Encoder::AdcRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_ADC, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AdcRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_ADC, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AdcRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_ADC, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AdcMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_ADC_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AdcMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_ADC_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AdcMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_ADC_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AdcMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::AdcMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::AdcMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, Imm32 );
}



bool x64Encoder::AdcAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_ADC_IMMA, Imm16 );
}

bool x64Encoder::AdcAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_ADC_IMMA, Imm32 );
}

bool x64Encoder::AdcAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_ADC_IMMA, Imm32 );
}


bool x64Encoder::AdcReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AdcReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AdcReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AdcMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AdcMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AdcMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AdcReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AdcReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AdcRegImm16 ( DestReg, ImmX );
	}
	
	return AdcAcc16Imm16 ( ImmX );
}

bool x64Encoder::AdcReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AdcReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AdcRegImm32 ( DestReg, ImmX );
	}
	
	return AdcAcc32Imm32 ( ImmX );
}

bool x64Encoder::AdcReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AdcReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AdcRegImm64 ( DestReg, ImmX );
	}
	
	return AdcAcc64Imm32 ( ImmX );
}


bool x64Encoder::AdcMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AdcMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AdcMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_ADC_IMM8, MODRM_ADC_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_ADC_IMM, MODRM_ADC_IMM, (char*) DestPtr, ImmX );
}








// --------------------------------












bool x64Encoder::BsrRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, SrcReg );
}

bool x64Encoder::BsrRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, SrcReg );
}

bool x64Encoder::BsrRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, SrcReg );
}

bool x64Encoder::BsrRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, (char*) SrcPtr );
}

bool x64Encoder::BsrRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, (char*) SrcPtr );
}

bool x64Encoder::BsrRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP_BSR_OP1, X64OP_BSR_OP2 ), DestReg, (char*) SrcPtr );
}



bool x64Encoder::AndRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_AND, DestReg, SrcReg );
}

bool x64Encoder::AndRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_AND, DestReg, SrcReg );
}

bool x64Encoder::AndRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_AND, DestReg, SrcReg );
}

bool x64Encoder::AndRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_AND_IMM, MODRM_AND_IMM, DestReg, Imm16 );
}

bool x64Encoder::AndRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_AND_IMM, MODRM_AND_IMM, DestReg, Imm32 );
}

bool x64Encoder::AndRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_AND_IMM, MODRM_AND_IMM, DestReg, Imm32 );
}

bool x64Encoder::AndRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_AND, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_AND, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_AND, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_AND_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_AND_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_AND_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_AND_IMM, MODRM_AND_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_AND_IMM, MODRM_AND_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::AndMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_AND_IMM, MODRM_AND_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::AndRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_AND, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AndRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_AND, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AndRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_AND, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::AndMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_AND_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AndMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_AND_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AndMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_AND_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::AndMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::AndMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::AndMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, Imm32 );
}



bool x64Encoder::AndAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_AND_IMMA, Imm16 );
}

bool x64Encoder::AndAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_AND_IMMA, Imm32 );
}

bool x64Encoder::AndAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_AND_IMMA, Imm32 );
}


bool x64Encoder::AndReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AndReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AndReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, DestReg, Imm8 );
}

bool x64Encoder::AndMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AndMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AndMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::AndReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX )
	{
		return XorRegReg16 ( DestReg, DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AndReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AndRegImm16 ( DestReg, ImmX );
	}
	
	return AndAcc16Imm16 ( ImmX );
}

bool x64Encoder::AndReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX )
	{
		return XorRegReg32 ( DestReg, DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AndReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AndRegImm32 ( DestReg, ImmX );
	}
	
	return AndAcc32Imm32 ( ImmX );
}

bool x64Encoder::AndReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX )
	{
		return XorRegReg64 ( DestReg, DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return AndReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return AndRegImm64 ( DestReg, ImmX );
	}
	
	return AndAcc64Imm32 ( ImmX );
}


bool x64Encoder::AndMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AndMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::AndMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_AND_IMM8, MODRM_AND_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_AND_IMM, MODRM_AND_IMM, (char*) DestPtr, ImmX );
}




bool x64Encoder::BtRegReg16 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, Reg );
}

bool x64Encoder::BtRegReg32 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, Reg );
}

bool x64Encoder::BtRegReg64 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, Reg );
}

bool x64Encoder::BtRegImm16 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, Reg, Imm8 );
}

bool x64Encoder::BtRegImm32 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, Reg, Imm8 );
}

bool x64Encoder::BtRegImm64 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, Reg, Imm8 );
}


bool x64Encoder::BtMemImm16 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, (char*) DestPtr, Imm8 );
}

bool x64Encoder::BtMemImm32 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, (char*) DestPtr, Imm8 );
}

bool x64Encoder::BtMemImm64 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, (char*) DestPtr, Imm8 );
}



bool x64Encoder::BtMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE ( X64OP1_BT, X64OP2_BT ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem16Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem32Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem64Imm8 ( MAKE2OPCODE ( X64OP1_BT_IMM, X64OP2_BT_IMM ), MODRM_BT_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

// btc

bool x64Encoder::BtcRegReg16 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, Reg );
}

bool x64Encoder::BtcRegReg32 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, Reg );
}

bool x64Encoder::BtcRegReg64 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, Reg );
}

bool x64Encoder::BtcRegImm16 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, Reg, Imm8 );
}

bool x64Encoder::BtcRegImm32 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, Reg, Imm8 );
}

bool x64Encoder::BtcRegImm64 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, Reg, Imm8 );
}

bool x64Encoder::BtcMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtcMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtcMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE ( X64OP1_BTC, X64OP2_BTC ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtcMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem16Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtcMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem32Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtcMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem64Imm8 ( MAKE2OPCODE ( X64OP1_BTC_IMM, X64OP2_BTC_IMM ), MODRM_BTC_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}


bool x64Encoder::BtcMemImm16(int16_t* DestPtr, char Imm8)
{
	return x64EncodeRipOffset16Imm8(MAKE2OPCODE(X64OP1_BTC_IMM, X64OP2_BTC_IMM), MODRM_BTC_IMM, (char*)DestPtr, Imm8);
}
bool x64Encoder::BtcMemImm32(int32_t* DestPtr, char Imm8)
{
	return x64EncodeRipOffset32Imm8(MAKE2OPCODE(X64OP1_BTC_IMM, X64OP2_BTC_IMM), MODRM_BTC_IMM, (char*)DestPtr, Imm8);
}
bool x64Encoder::BtcMemImm64(int64_t* DestPtr, char Imm8)
{
	return x64EncodeRipOffset64Imm8(MAKE2OPCODE(X64OP1_BTC_IMM, X64OP2_BTC_IMM), MODRM_BTC_IMM, (char*)DestPtr, Imm8);
}



// btr

bool x64Encoder::BtrRegReg16 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, Reg );
}

bool x64Encoder::BtrRegReg32 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, Reg );
}

bool x64Encoder::BtrRegReg64 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, Reg );
}

bool x64Encoder::BtrRegImm16 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, Reg, Imm8 );
}

bool x64Encoder::BtrRegImm32 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, Reg, Imm8 );
}

bool x64Encoder::BtrRegImm64 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, Reg, Imm8 );
}

bool x64Encoder::BtrMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtrMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtrMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE ( X64OP1_BTR, X64OP2_BTR ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::BtrMem32Imm ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffsetImm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, (char*) DestPtr, Imm8 );
}

bool x64Encoder::BtrMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem16Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtrMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem32Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtrMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem64Imm8 ( MAKE2OPCODE ( X64OP1_BTR_IMM, X64OP2_BTR_IMM ), MODRM_BTR_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

// bts

bool x64Encoder::BtsRegReg16 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, Reg );
}

bool x64Encoder::BtsRegReg32 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, Reg );
}

bool x64Encoder::BtsRegReg64 ( int32_t Reg, int32_t BitSelectReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, Reg );
}

bool x64Encoder::BtsRegImm16 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, Reg, Imm8 );
}

bool x64Encoder::BtsRegImm32 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, Reg, Imm8 );
}

bool x64Encoder::BtsRegImm64 ( int32_t Reg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, Reg, Imm8 );
}

bool x64Encoder::BtsMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtsMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtsMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE ( X64OP1_BTS, X64OP2_BTS ), BitSelectReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::BtsMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem16Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtsMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem32Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtsMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMem64Imm8 ( MAKE2OPCODE ( X64OP1_BTS_IMM, X64OP2_BTS_IMM ), MODRM_BTS_IMM, AddressReg, IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::BtsMemImm32(int32_t* DestPtr, char Imm8)
{
	return x64EncodeRipOffsetImm8(MAKE2OPCODE(X64OP1_BTS_IMM, X64OP2_BTS_IMM), MODRM_BTS_IMM, (char*)DestPtr, Imm8);
}


bool x64Encoder::Cbw ( void )
{
	return x64Encode16 ( X64OP_CBW );
}

bool x64Encoder::Cwde ( void )
{
	return x64Encode32 ( X64OP_CWDE );
}

bool x64Encoder::Cdqe ( void )
{
	return x64Encode64 ( X64OP_CDQE );
}



bool x64Encoder::Cwd ( void )
{
	return x64Encode16 ( X64OP_CWD );
}

bool x64Encoder::Cdq ( void )
{
	return x64Encode32 ( X64OP_CDQ );
}

bool x64Encoder::Cqo ( void )
{
	return x64Encode64 ( X64OP_CQO );
}



bool x64Encoder::CmovERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, SrcReg );
}

bool x64Encoder::CmovNERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, SrcReg );
}

bool x64Encoder::CmovBRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, SrcReg );
}

bool x64Encoder::CmovBERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, SrcReg );
}

bool x64Encoder::CmovARegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, SrcReg );
}

bool x64Encoder::CmovAERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, SrcReg );
}

bool x64Encoder::CmovLRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVL, X64OP2_CMOVL ), DestReg, SrcReg );
}

bool x64Encoder::CmovORegReg16(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg16(MAKE2OPCODE(X64OP1_CMOVO, X64OP2_CMOVO), DestReg, SrcReg);
}

bool x64Encoder::CmovERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, SrcReg );
}

bool x64Encoder::CmovNERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, SrcReg );
}

bool x64Encoder::CmovBRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, SrcReg );
}

bool x64Encoder::CmovBERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, SrcReg );
}

bool x64Encoder::CmovARegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, SrcReg );
}

bool x64Encoder::CmovAERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, SrcReg );
}


bool x64Encoder::CmovLRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVL, X64OP2_CMOVL ), DestReg, SrcReg );
}

bool x64Encoder::CmovORegReg32(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg32(MAKE2OPCODE(X64OP1_CMOVO, X64OP2_CMOVO), DestReg, SrcReg);
}

bool x64Encoder::CmovERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, SrcReg );
}

bool x64Encoder::CmovNERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, SrcReg );
}

bool x64Encoder::CmovBRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, SrcReg );
}

bool x64Encoder::CmovBERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, SrcReg );
}

bool x64Encoder::CmovARegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, SrcReg );
}

bool x64Encoder::CmovAERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, SrcReg );
}

bool x64Encoder::CmovLRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVL, X64OP2_CMOVL ), DestReg, SrcReg );
}

bool x64Encoder::CmovORegReg64(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg64(MAKE2OPCODE(X64OP1_CMOVO, X64OP2_CMOVO), DestReg, SrcReg);
}


bool x64Encoder::CmovERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovNERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovNERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovNERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovBRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::CmovAERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovAERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmovAERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, AddressReg, IndexReg, Scale, Offset );
}



bool x64Encoder::CmovERegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovNERegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBERegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovARegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovAERegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovERegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovNERegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBERegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovARegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovAERegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, (char*) SrcPtr, true );
}


bool x64Encoder::CmovERegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVE, X64OP2_CMOVE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovNERegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVNE, X64OP2_CMOVNE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVB, X64OP2_CMOVB ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovBERegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVBE, X64OP2_CMOVBE ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovARegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVA, X64OP2_CMOVA ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovAERegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVAE, X64OP2_CMOVAE ), DestReg, (char*) SrcPtr, true );
}



bool x64Encoder::CmovLERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVLE, X64OP2_CMOVLE ), DestReg, SrcReg );
}

bool x64Encoder::CmovGRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVG, X64OP2_CMOVG ), DestReg, SrcReg );
}

bool x64Encoder::CmovGERegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVGE, X64OP2_CMOVGE ), DestReg, SrcReg );
}

bool x64Encoder::CmovLERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVLE, X64OP2_CMOVLE ), DestReg, SrcReg );
}


bool x64Encoder::CmovGRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVG, X64OP2_CMOVG ), DestReg, SrcReg );
}

bool x64Encoder::CmovGERegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVGE, X64OP2_CMOVGE ), DestReg, SrcReg );
}

bool x64Encoder::CmovLERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVLE, X64OP2_CMOVLE ), DestReg, SrcReg );
}

bool x64Encoder::CmovGRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVG, X64OP2_CMOVG ), DestReg, SrcReg );
}

bool x64Encoder::CmovGERegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVGE, X64OP2_CMOVGE ), DestReg, SrcReg );
}


bool x64Encoder::CmovSRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, SrcReg );
}

bool x64Encoder::CmovSRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, SrcReg );
}

bool x64Encoder::CmovSRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, SrcReg );
}

bool x64Encoder::CmovSRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovSRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovSRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVS, X64OP2_CMOVS ), DestReg, (char*) SrcPtr, true );
}



bool x64Encoder::CmovNSRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, SrcReg );
}

bool x64Encoder::CmovNSRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, SrcReg );
}

bool x64Encoder::CmovNSRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, SrcReg );
}

bool x64Encoder::CmovNSRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovNSRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmovNSRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_CMOVNS, X64OP2_CMOVNS ), DestReg, (char*) SrcPtr, true );
}






bool x64Encoder::CmpRegReg16 ( int32_t SrcReg1, int32_t SrcReg2 )
{
	return x64EncodeRegReg16 ( X64OP_CMP, SrcReg1, SrcReg2 );
}

bool x64Encoder::CmpRegReg32 ( int32_t SrcReg1, int32_t SrcReg2 )
{
	return x64EncodeRegReg32 ( X64OP_CMP, SrcReg1, SrcReg2 );
}

bool x64Encoder::CmpRegReg64 ( int32_t SrcReg1, int32_t SrcReg2 )
{
	return x64EncodeRegReg64 ( X64OP_CMP, SrcReg1, SrcReg2 );
}

bool x64Encoder::CmpRegImm8(int32_t SrcReg, int8_t Imm8)
{
	return x64EncodeReg8Imm8(X64OP_CMP8_IMM8, MODRM_CMP8_IMM8, SrcReg, Imm8);
}
bool x64Encoder::CmpRegImm16 ( int32_t SrcReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_CMP_IMM, MODRM_CMP_IMM, SrcReg, Imm16 );
}

bool x64Encoder::CmpRegImm32 ( int32_t SrcReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_CMP_IMM, MODRM_CMP_IMM, SrcReg, Imm32 );
}

bool x64Encoder::CmpRegImm64 ( int32_t SrcReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_CMP_IMM, MODRM_CMP_IMM, SrcReg, Imm32 );
}

bool x64Encoder::CmpRegMem8 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_CMP8, SrcReg, AddressReg, IndexReg, Scale, Offset );
}
bool x64Encoder::CmpRegMem16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_CMP, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpRegMem32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_CMP, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpRegMem64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_CMP, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_CMP_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_CMP_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_CMP_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_CMP_IMM, MODRM_CMP_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_CMP_IMM, MODRM_CMP_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::CmpMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_CMP_IMM, MODRM_CMP_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}




bool x64Encoder::CmpRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_CMP, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmpRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_CMP, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmpRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_CMP, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::CmpMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_CMP_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::CmpMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_CMP_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::CmpMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_CMP_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::CmpMemImm8 ( char* DestPtr, char Imm8 )
{
	return x64EncodeRipOffsetImm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::CmpMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::CmpMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::CmpMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, Imm32 );
}



bool x64Encoder::CmpAcc8Imm8(int8_t Imm8)
{
	return x64EncodeAcc8Imm8(X64OP_CMP8_IMMA, Imm8);
}
bool x64Encoder::CmpAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_CMP_IMMA, Imm16 );
}

bool x64Encoder::CmpAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_CMP_IMMA, Imm32 );
}

bool x64Encoder::CmpAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_CMP_IMMA, Imm32 );
}


bool x64Encoder::CmpReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, DestReg, Imm8 );
}

bool x64Encoder::CmpReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, DestReg, Imm8 );
}

bool x64Encoder::CmpReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, DestReg, Imm8 );
}

bool x64Encoder::CmpMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::CmpMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::CmpMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::CmpReg8ImmX(int32_t DestReg, int8_t ImmX)
{
	if (DestReg)
	{
		return CmpRegImm8(DestReg, ImmX);
	}

	return CmpAcc8Imm8(ImmX);
}
bool x64Encoder::CmpReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return CmpReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return CmpRegImm16 ( DestReg, ImmX );
	}
	
	return CmpAcc16Imm16 ( ImmX );
}

bool x64Encoder::CmpReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return CmpReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return CmpRegImm32 ( DestReg, ImmX );
	}
	
	return CmpAcc32Imm32 ( ImmX );
}

bool x64Encoder::CmpReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return CmpReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return CmpRegImm64 ( DestReg, ImmX );
	}
	
	return CmpAcc64Imm32 ( ImmX );
}


bool x64Encoder::CmpMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::CmpMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::CmpMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_CMP_IMM8, MODRM_CMP_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_CMP_IMM, MODRM_CMP_IMM, (char*) DestPtr, ImmX );
}






bool x64Encoder::DivRegReg16 ( int32_t SrcReg )
{
	return x64EncodeReg16 ( X64OP_DIV, MODRM_DIV, SrcReg );
}

bool x64Encoder::DivRegReg32 ( int32_t SrcReg )
{
	return x64EncodeReg32 ( X64OP_DIV, MODRM_DIV, SrcReg );
}

bool x64Encoder::DivRegReg64 ( int32_t SrcReg )
{
	return x64EncodeReg64 ( X64OP_DIV, MODRM_DIV, SrcReg );
}

bool x64Encoder::DivRegMem16 ( int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_DIV, MODRM_DIV, (char*) SrcPtr );
}

bool x64Encoder::DivRegMem32 ( int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_DIV, MODRM_DIV, (char*) SrcPtr );
}

bool x64Encoder::DivRegMem64 ( int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_DIV, MODRM_DIV, (char*) SrcPtr );
}



bool x64Encoder::DivRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_DIV, MODRM_DIV, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::DivRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_DIV, MODRM_DIV, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::DivRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_DIV, MODRM_DIV, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::IdivRegReg16 ( int32_t SrcReg )
{
	return x64EncodeReg16 ( X64OP_IDIV, MODRM_IDIV, SrcReg );
}

bool x64Encoder::IdivRegReg32 ( int32_t SrcReg )
{
	return x64EncodeReg32 ( X64OP_IDIV, MODRM_IDIV, SrcReg );
}

bool x64Encoder::IdivRegReg64 ( int32_t SrcReg )
{
	return x64EncodeReg64 ( X64OP_IDIV, MODRM_IDIV, SrcReg );
}

bool x64Encoder::IdivRegMem16 ( int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_IDIV, MODRM_IDIV, (char*) SrcPtr );
}

bool x64Encoder::IdivRegMem32 ( int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_IDIV, MODRM_IDIV, (char*) SrcPtr );
}

bool x64Encoder::IdivRegMem64 ( int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_IDIV, MODRM_IDIV, (char*) SrcPtr );
}


bool x64Encoder::IdivRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_IDIV, MODRM_IDIV, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::IdivRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_IDIV, MODRM_IDIV, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::IdivRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_IDIV, MODRM_IDIV, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::ImulRegReg16 ( int32_t SrcReg )
{
	return x64EncodeReg16 ( X64OP_IMUL, MODRM_IMUL, SrcReg );
}

bool x64Encoder::ImulRegReg32 ( int32_t SrcReg )
{
	return x64EncodeReg32 ( X64OP_IMUL, MODRM_IMUL, SrcReg );
}

bool x64Encoder::ImulRegReg64 ( int32_t SrcReg )
{
	return x64EncodeReg64 ( X64OP_IMUL, MODRM_IMUL, SrcReg );
}

bool x64Encoder::ImulRegMem16 ( int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_IMUL, MODRM_IMUL, (char*) SrcPtr );
}

bool x64Encoder::ImulRegMem32 ( int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_IMUL, MODRM_IMUL, (char*) SrcPtr );
}

bool x64Encoder::ImulRegMem64 ( int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_IMUL, MODRM_IMUL, (char*) SrcPtr );
}


bool x64Encoder::ImulRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_IMUL, MODRM_IMUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::ImulRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_IMUL, MODRM_IMUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::ImulRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_IMUL, MODRM_IMUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::LeaRegRegReg16 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg )
{
	return x64EncodeRegMem16 ( X64OP_LEA, DestReg, Src1Reg, Src2Reg, SCALE_NONE, 0 );
}

bool x64Encoder::LeaRegRegReg32 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg )
{
	return x64EncodeRegMem32 ( X64OP_LEA, DestReg, Src1Reg, Src2Reg, SCALE_NONE, 0 );
}

bool x64Encoder::LeaRegRegReg64 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg )
{
	return x64EncodeRegMem64 ( X64OP_LEA, DestReg, Src1Reg, Src2Reg, SCALE_NONE, 0 );
}

bool x64Encoder::LeaRegMem64 ( int32_t DestReg, void* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_LEA, DestReg, (char*) SrcPtr, true );
}


bool x64Encoder::LeaRegRegImm16 ( int32_t DestReg, int32_t SrcReg, int32_t Imm16 )
{
	return x64EncodeRegMem16 ( X64OP_LEA, DestReg, SrcReg, NO_INDEX, SCALE_NONE, Imm16 );
}

bool x64Encoder::LeaRegRegImm32 ( int32_t DestReg, int32_t SrcReg, int32_t Imm32 )
{
	return x64EncodeRegMem32 ( X64OP_LEA, DestReg, SrcReg, NO_INDEX, SCALE_NONE, Imm32 );
}

bool x64Encoder::LeaRegRegImm64 ( int32_t DestReg, int32_t SrcReg, int32_t Imm32 )
{
	return x64EncodeRegMem64 ( X64OP_LEA, DestReg, SrcReg, NO_INDEX, SCALE_NONE, Imm32 );
}




bool x64Encoder::MovsxReg16Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, SrcReg );
}

bool x64Encoder::MovsxReg32Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, SrcReg );
}

bool x64Encoder::MovsxReg64Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, SrcReg );
}





//

bool x64Encoder::MovsxReg32Reg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, SrcReg );
}

bool x64Encoder::MovsxReg64Reg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, SrcReg );
}



bool x64Encoder::MovsxReg16Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovsxReg32Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovsxReg64Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}




bool x64Encoder::MovsxReg16Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovsxReg32Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovsxReg64Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVSXB, X64OP2_MOVSXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovsxReg32Mem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovsxReg64Mem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, (char*)SrcPtr, true );
}






bool x64Encoder::MovsxdReg64Reg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_MOVSXD, DestReg, SrcReg );
}

bool x64Encoder::MovsxdReg64Mem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_MOVSXD, DestReg, (char*) SrcPtr, true );
}







bool x64Encoder::MovsxReg32Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovsxReg64Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_MOVSXH, X64OP2_MOVSXH ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovsxdReg64Mem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_MOVSXD, DestReg, AddressReg, IndexReg, Scale, Offset );
}



// -----------------------------------------

bool x64Encoder::MovzxReg16Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, SrcReg );
}

bool x64Encoder::MovzxReg32Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, SrcReg );
}

bool x64Encoder::MovzxReg64Reg8 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, SrcReg );
}



bool x64Encoder::MovzxReg16Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovzxReg32Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovzxReg64Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, AddressReg, IndexReg, Scale, Offset );
}



bool x64Encoder::MovzxReg32Reg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, SrcReg );
}

bool x64Encoder::MovzxReg64Reg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, SrcReg );
}



bool x64Encoder::MovzxReg32Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MovzxReg64Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, AddressReg, IndexReg, Scale, Offset );
}





bool x64Encoder::MovzxReg16Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovzxReg32Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovzxReg64Mem8 ( int32_t DestReg, char* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVZXB, X64OP2_MOVZXB ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovzxReg32Mem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, (char*)SrcPtr, true );
}

bool x64Encoder::MovzxReg64Mem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVZXH, X64OP2_MOVZXH ), DestReg, (char*)SrcPtr, true );
}






bool x64Encoder::MulRegReg16 ( int32_t SrcReg )
{
	return x64EncodeReg16 ( X64OP_MUL, MODRM_MUL, SrcReg );
}

bool x64Encoder::MulRegReg32 ( int32_t SrcReg )
{
	return x64EncodeReg32 ( X64OP_MUL, MODRM_MUL, SrcReg );
}

bool x64Encoder::MulRegReg64 ( int32_t SrcReg )
{
	return x64EncodeReg64 ( X64OP_MUL, MODRM_MUL, SrcReg );
}

bool x64Encoder::MulRegMem16 ( int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_MUL, MODRM_MUL, (char*) SrcPtr );
}

bool x64Encoder::MulRegMem32 ( int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_MUL, MODRM_MUL, (char*) SrcPtr );
}

bool x64Encoder::MulRegMem64 ( int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_MUL, MODRM_MUL, (char*) SrcPtr );
}


bool x64Encoder::MulRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_MUL, MODRM_MUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MulRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_MUL, MODRM_MUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::MulRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_MUL, MODRM_MUL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NegReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_NEG, MODRM_NEG, DestReg );
}

bool x64Encoder::NegReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_NEG, MODRM_NEG, DestReg );
}

bool x64Encoder::NegReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_NEG, MODRM_NEG, DestReg );
}

bool x64Encoder::NegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_NEG, MODRM_NEG, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_NEG, MODRM_NEG, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_NEG, MODRM_NEG, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NegMem16 ( int16_t* DestPtr )
{
	return x64EncodeRipOffset16 ( X64OP_NEG, MODRM_NEG, (char*) DestPtr );
}

bool x64Encoder::NegMem32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_NEG, MODRM_NEG, (char*) DestPtr );
}

bool x64Encoder::NegMem64 ( int64_t* DestPtr )
{
	return x64EncodeRipOffset64 ( X64OP_NEG, MODRM_NEG, (char*) DestPtr );
}




bool x64Encoder::NotReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_NOT, MODRM_NOT, DestReg );
}

bool x64Encoder::NotReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_NOT, MODRM_NOT, DestReg );
}

bool x64Encoder::NotReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_NOT, MODRM_NOT, DestReg );
}

bool x64Encoder::NotMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_NOT, MODRM_NOT, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NotMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_NOT, MODRM_NOT, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NotMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_NOT, MODRM_NOT, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::NotMem16 ( int16_t* DestPtr )
{
	return x64EncodeRipOffset16 ( X64OP_NOT, MODRM_NOT, (char*) DestPtr );
}

bool x64Encoder::NotMem32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_NOT, MODRM_NOT, (char*) DestPtr );
}

bool x64Encoder::NotMem64 ( int64_t* DestPtr )
{
	return x64EncodeRipOffset64 ( X64OP_NOT, MODRM_NOT, (char*) DestPtr );
}




bool x64Encoder::IncReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_INC, MODRM_INC, DestReg );
}

bool x64Encoder::IncReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_INC, MODRM_INC, DestReg );
}

bool x64Encoder::IncReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_INC, MODRM_INC, DestReg );
}

bool x64Encoder::IncMem16 ( int16_t* DestPtr )
{
	return x64EncodeRipOffset16 ( X64OP_INC, MODRM_INC, (char*) DestPtr );
}

bool x64Encoder::IncMem32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_INC, MODRM_INC, (char*) DestPtr );
}

bool x64Encoder::IncMem64 ( int64_t* DestPtr )
{
	return x64EncodeRipOffset64 ( X64OP_INC, MODRM_INC, (char*) DestPtr );
}



bool x64Encoder::DecReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_DEC, MODRM_DEC, DestReg );
}

bool x64Encoder::DecReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_DEC, MODRM_DEC, DestReg );
}

bool x64Encoder::DecReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_DEC, MODRM_DEC, DestReg );
}

bool x64Encoder::DecMem16 ( int16_t* DestPtr )
{
	return x64EncodeRipOffset16 ( X64OP_DEC, MODRM_DEC, (char*) DestPtr );
}

bool x64Encoder::DecMem32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_DEC, MODRM_DEC, (char*) DestPtr );
}

bool x64Encoder::DecMem64 ( int64_t* DestPtr )
{
	return x64EncodeRipOffset64 ( X64OP_DEC, MODRM_DEC, (char*) DestPtr );
}




bool x64Encoder::OrRegReg8(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg32(X64OP_OR_FROMMEM8, DestReg, SrcReg);
}
bool x64Encoder::OrRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_OR, DestReg, SrcReg );
}

bool x64Encoder::OrRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_OR, DestReg, SrcReg );
}

bool x64Encoder::OrRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_OR, DestReg, SrcReg );
}

bool x64Encoder::OrRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_OR_IMM, MODRM_OR_IMM, DestReg, Imm16 );
}

bool x64Encoder::OrRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_OR_IMM, MODRM_OR_IMM, DestReg, Imm32 );
}

bool x64Encoder::OrRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_OR_IMM, MODRM_OR_IMM, DestReg, Imm32 );
}

bool x64Encoder::OrRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_OR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_OR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_OR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_OR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_OR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_OR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_OR_IMM, MODRM_OR_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_OR_IMM, MODRM_OR_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_OR_IMM, MODRM_OR_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::OrRegMem8(int32_t DestReg, char* SrcPtr)
{
	return x64EncodeRipOffset32(X64OP_OR_FROMMEM8, DestReg, (char*)SrcPtr);
}

bool x64Encoder::OrRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_OR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::OrRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_OR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::OrRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_OR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::OrMemReg8 ( char* DestPtr, int SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_OR_TOMEM8, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::OrMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_OR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::OrMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_OR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::OrMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_OR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::OrMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::OrMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::OrMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, Imm32 );
}


bool x64Encoder::OrAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_OR_IMMA, Imm16 );
}

bool x64Encoder::OrAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_OR_IMMA, Imm32 );
}

bool x64Encoder::OrAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_OR_IMMA, Imm32 );
}


bool x64Encoder::OrReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::OrReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::OrReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::OrMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::OrMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::OrMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::OrReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return OrReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return OrRegImm16 ( DestReg, ImmX );
	}
	
	return OrAcc16Imm16 ( ImmX );
}

bool x64Encoder::OrReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return OrReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return OrRegImm32 ( DestReg, ImmX );
	}
	
	return OrAcc32Imm32 ( ImmX );
}

bool x64Encoder::OrReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return OrReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return OrRegImm64 ( DestReg, ImmX );
	}
	
	return OrAcc64Imm32 ( ImmX );
}


bool x64Encoder::OrMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::OrMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::OrMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_OR_IMM8, MODRM_OR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_OR_IMM, MODRM_OR_IMM, (char*) DestPtr, ImmX );
}








bool x64Encoder::PopReg64 ( int32_t SrcReg )
{
	x64EncodeRexReg64 ( 0, SrcReg );
	return x64EncodeOpcode ( X64BOP_POP + ( SrcReg & 7 ) );
}
	
bool x64Encoder::PushReg64 ( int32_t SrcReg )
{
	x64EncodeRexReg64 ( 0, SrcReg );
	return x64EncodeOpcode ( X64BOP_PUSH + ( SrcReg & 7 ) );
}

bool x64Encoder::PushImm8 ( char Imm8 )
{
	x64EncodeOpcode ( X64OP_PUSH_IMM8 );

	if ( x64CurrentCodeBlockSpaceRemaining () <= 0 ) return false;
	
	x64CodeArea [ x64NextOffset++ ] = Imm8;
	
	return true;
}

bool x64Encoder::PushImm16 ( int16_t Imm16 )
{
	x64Encode16Bit ();
	x64EncodeOpcode ( X64OP_PUSH_IMM );
	return x64EncodeImmediate16 ( Imm16 );
}

bool x64Encoder::PushImm32 ( int32_t Imm32 )
{
	x64EncodeOpcode ( X64OP_PUSH_IMM );
	return x64EncodeImmediate32 ( Imm32 );
}

bool x64Encoder::Ret ( void )
{
	return x64EncodeReturn ();
}

// seta - set if unsigned above
bool x64Encoder::Set_A (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETA, X64OP2_SETA ), 0, DestReg );
}

bool x64Encoder::Set_A ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETA, X64OP2_SETA ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_A ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETA, X64OP2_SETA ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setae - set if unsigned above or equal
bool x64Encoder::Set_AE (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETAE, X64OP2_SETAE ), 0, DestReg );
}

bool x64Encoder::Set_AE ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETAE, X64OP2_SETAE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_AE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETAE, X64OP2_SETAE ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setb - set if unsigned below
bool x64Encoder::Set_B (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETB, X64OP2_SETB ), 0, DestReg );
}

bool x64Encoder::Set_B ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETB, X64OP2_SETB ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_B ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETB, X64OP2_SETB ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setbe - set if unsigned below or equal
bool x64Encoder::Set_BE (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETBE, X64OP2_SETBE ), 0, DestReg );
}

bool x64Encoder::Set_BE ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETBE, X64OP2_SETBE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_BE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETBE, X64OP2_SETBE ), 0, AddressReg, IndexReg, Scale, Offset );
}

// sete - set if equal
bool x64Encoder::Set_E (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETE, X64OP2_SETE ), 0, DestReg );
}

bool x64Encoder::Set_E ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETE, X64OP2_SETE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_E ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETE, X64OP2_SETE ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setne - set if equal
bool x64Encoder::Set_NE (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETNE, X64OP2_SETNE ), 0, DestReg );
}

bool x64Encoder::Set_NE ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETNE, X64OP2_SETNE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_NE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETNE, X64OP2_SETNE ), 0, AddressReg, IndexReg, Scale, Offset );
}

// signed versions

// setg - set if signed greater
bool x64Encoder::Set_G ( int DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETG, X64OP2_SETG ), 0, DestReg );
}

bool x64Encoder::Set_G ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETG, X64OP2_SETG ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_G ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETG, X64OP2_SETG ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setge - set if signed greater
bool x64Encoder::Set_GE (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETGE, X64OP2_SETGE ), 0, DestReg );
}

bool x64Encoder::Set_GE ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETGE, X64OP2_SETGE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_GE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETGE, X64OP2_SETGE ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setl - set if signed greater
bool x64Encoder::Set_L (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETL, X64OP2_SETL ), 0, DestReg );
}

bool x64Encoder::Set_L ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETL, X64OP2_SETL ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_L ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETL, X64OP2_SETL ), 0, AddressReg, IndexReg, Scale, Offset );
}

// setle - set if signed greater
bool x64Encoder::Set_LE (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETLE, X64OP2_SETLE ), 0, DestReg );
}

bool x64Encoder::Set_LE ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETLE, X64OP2_SETLE ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_LE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETLE, X64OP2_SETLE ), 0, AddressReg, IndexReg, Scale, Offset );
}


// sets - set if signed
bool x64Encoder::Set_S (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETS, X64OP2_SETS ), 0, DestReg );
}

bool x64Encoder::Set_S ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETS, X64OP2_SETS ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_S ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETS, X64OP2_SETS ), 0, AddressReg, IndexReg, Scale, Offset );
}


// setpo - set if parity odd
bool x64Encoder::Set_PO (int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETPO, X64OP2_SETPO ), 0, DestReg );
}

bool x64Encoder::Set_PO ( void* DestPtr8 )
{
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_SETPO, X64OP2_SETPO ), 0, (char*) DestPtr8 );
}

bool x64Encoder::Set_PO ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETPO, X64OP2_SETPO ), 0, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::Setb ( int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETB, X64OP2_SETB ), 0, DestReg );
}

bool x64Encoder::Setl ( int32_t DestReg )
{
	return x64EncodeReg32 ( MAKE2OPCODE( X64OP1_SETL, X64OP2_SETL ), 0, DestReg );
}

bool x64Encoder::Setl ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( MAKE2OPCODE( X64OP1_SETL, X64OP2_SETL ), 0, AddressReg, IndexReg, Scale, Offset );
}



bool x64Encoder::RcrReg16(int32_t DestReg)
{
	return x64EncodeReg16(X64OP_RCR_1, MODRM_RCR_1, DestReg);
}

bool x64Encoder::RcrReg32(int32_t DestReg)
{
	return x64EncodeReg32(X64OP_RCR_1, MODRM_RCR_1, DestReg);
}

bool x64Encoder::RcrReg64(int32_t DestReg)
{
	return x64EncodeReg64(X64OP_RCR_1, MODRM_RCR_1, DestReg);
}



bool x64Encoder::SarRegReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_SAR, MODRM_SAR, DestReg );
}

bool x64Encoder::SarRegReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_SAR, MODRM_SAR, DestReg );
}

bool x64Encoder::SarRegReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_SAR, MODRM_SAR, DestReg );
}

bool x64Encoder::SarRegImm16 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg16Imm8 ( X64OP_SAR_IMM, MODRM_SAR_IMM, DestReg, Imm8 );
}

bool x64Encoder::SarRegImm32 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg32Imm8 ( X64OP_SAR_IMM, MODRM_SAR_IMM, DestReg, Imm8 );
}

bool x64Encoder::SarRegImm64 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg64Imm8 ( X64OP_SAR_IMM, MODRM_SAR_IMM, DestReg, Imm8 );
}

bool x64Encoder::SarMemReg32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_SAR, MODRM_SAR, (char*) DestPtr );
}


bool x64Encoder::ShlRegReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_SHL, MODRM_SHL, DestReg );
}

bool x64Encoder::ShlRegReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_SHL, MODRM_SHL, DestReg );
}

bool x64Encoder::ShlRegReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_SHL, MODRM_SHL, DestReg );
}

bool x64Encoder::ShlRegImm16 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg16Imm8 ( X64OP_SHL_IMM, MODRM_SHL_IMM, DestReg, Imm8 );
}

bool x64Encoder::ShlRegImm32 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg32Imm8 ( X64OP_SHL_IMM, MODRM_SHL_IMM, DestReg, Imm8 );
}

bool x64Encoder::ShlRegImm64 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg64Imm8 ( X64OP_SHL_IMM, MODRM_SHL_IMM, DestReg, Imm8 );
}

bool x64Encoder::ShlMemReg32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_SHL, MODRM_SHL, (char*) DestPtr );
}


bool x64Encoder::ShlMemReg16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_SHL, MODRM_SHL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::ShlMemReg32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_SHL, MODRM_SHL, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::ShlMemReg64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_SHL, MODRM_SHL, AddressReg, IndexReg, Scale, Offset );
}

//bool x64Encoder::ShlMemImm16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
//{
//}

//bool x64Encoder::ShlMemImm32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
//{
//}

//bool x64Encoder::ShlMemImm64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
//{
//}







bool x64Encoder::ShrRegReg16 ( int32_t DestReg )
{
	return x64EncodeReg16 ( X64OP_SHR, MODRM_SHR, DestReg );
}

bool x64Encoder::ShrRegReg32 ( int32_t DestReg )
{
	return x64EncodeReg32 ( X64OP_SHR, MODRM_SHR, DestReg );
}

bool x64Encoder::ShrRegReg64 ( int32_t DestReg )
{
	return x64EncodeReg64 ( X64OP_SHR, MODRM_SHR, DestReg );
}

bool x64Encoder::ShrRegImm16 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg16Imm8 ( X64OP_SHR_IMM, MODRM_SHR_IMM, DestReg, Imm8 );
}

bool x64Encoder::ShrRegImm32 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg32Imm8 ( X64OP_SHR_IMM, MODRM_SHR_IMM, DestReg, Imm8 );
}

bool x64Encoder::ShrRegImm64 ( int32_t DestReg, char Imm8 )
{
	if ( !Imm8 ) return true;
	
	return x64EncodeReg64Imm8 ( X64OP_SHR_IMM, MODRM_SHR_IMM, DestReg, Imm8 );
}


bool x64Encoder::ShrMemReg32 ( int32_t* DestPtr )
{
	return x64EncodeRipOffset32 ( X64OP_SHR, MODRM_SHR, (char*) DestPtr );
}





bool x64Encoder::SubRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_SUB, DestReg, SrcReg );
}

bool x64Encoder::SubRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_SUB, DestReg, SrcReg );
}

bool x64Encoder::SubRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_SUB, DestReg, SrcReg );
}

bool x64Encoder::SubRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_SUB_IMM, MODRM_SUB_IMM, DestReg, Imm16 );
}

bool x64Encoder::SubRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_SUB_IMM, MODRM_SUB_IMM, DestReg, Imm32 );
}

bool x64Encoder::SubRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_SUB_IMM, MODRM_SUB_IMM, DestReg, Imm32 );
}

bool x64Encoder::SubRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_SUB, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_SUB, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_SUB, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_SUB_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_SUB_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_SUB_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_SUB_IMM, MODRM_SUB_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_SUB_IMM, MODRM_SUB_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::SubMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_SUB_IMM, MODRM_SUB_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}



bool x64Encoder::SubRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_SUB, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::SubRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_SUB, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::SubRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_SUB, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::SubMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_SUB_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::SubMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_SUB_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::SubMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_SUB_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::SubMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::SubMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::SubMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, Imm32 );
}


bool x64Encoder::SubAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_SUB_IMMA, Imm16 );
}

bool x64Encoder::SubAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_SUB_IMMA, Imm32 );
}

bool x64Encoder::SubAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_SUB_IMMA, Imm32 );
}


bool x64Encoder::SubReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, DestReg, Imm8 );
}

bool x64Encoder::SubReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, DestReg, Imm8 );
}

bool x64Encoder::SubReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, DestReg, Imm8 );
}

bool x64Encoder::SubMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::SubMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::SubMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::SubReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecReg16 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return IncReg16 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return SubReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return SubRegImm16 ( DestReg, ImmX );
	}
	
	return SubAcc16Imm16 ( ImmX );
}

bool x64Encoder::SubReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecReg32 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return IncReg32 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return SubReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return SubRegImm32 ( DestReg, ImmX );
	}
	
	return SubAcc32Imm32 ( ImmX );
}

bool x64Encoder::SubReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecReg64 ( DestReg );
	}

	if ( ImmX == -1 )
	{
		return IncReg64 ( DestReg );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return SubReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return SubRegImm64 ( DestReg, ImmX );
	}
	
	return SubAcc64Imm32 ( ImmX );
}


bool x64Encoder::SubMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecMem16 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return IncMem16 ( DestPtr );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::SubMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecMem32 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return IncMem32 ( DestPtr );
	}

	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::SubMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX == 1 )
	{
		return DecMem64 ( DestPtr );
	}

	if ( ImmX == -1 )
	{
		return IncMem64 ( DestPtr );
	}
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_SUB_IMM8, MODRM_SUB_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_SUB_IMM, MODRM_SUB_IMM, (char*) DestPtr, ImmX );
}


bool x64Encoder::SbbRegReg16(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg16(X64OP_SBB, DestReg, SrcReg);
}

bool x64Encoder::SbbRegReg32(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg32(X64OP_SBB, DestReg, SrcReg);
}

bool x64Encoder::SbbRegReg64(int32_t DestReg, int32_t SrcReg)
{
	return x64EncodeRegReg64(X64OP_SBB, DestReg, SrcReg);
}


bool x64Encoder::SbbReg16Imm8(int32_t DestReg, char Imm8)
{
	return x64EncodeReg16Imm8(X64OP_SBB_IMM8, MODRM_SBB_IMM8, DestReg, Imm8);
}

bool x64Encoder::SbbReg32Imm8(int32_t DestReg, char Imm8)
{
	return x64EncodeReg32Imm8(X64OP_SBB_IMM8, MODRM_SBB_IMM8, DestReg, Imm8);
}

bool x64Encoder::SbbReg64Imm8(int32_t DestReg, char Imm8)
{
	return x64EncodeReg64Imm8(X64OP_SBB_IMM8, MODRM_SBB_IMM8, DestReg, Imm8);
}



bool x64Encoder::TestRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_TEST_TOMEM, DestReg, SrcReg );
}

bool x64Encoder::TestRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_TEST_TOMEM, DestReg, SrcReg );
}

bool x64Encoder::TestRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_TEST_TOMEM, DestReg, SrcReg );
}

bool x64Encoder::TestRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_TEST_IMM, MODRM_TEST_IMM, DestReg, Imm16 );
}

bool x64Encoder::TestRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_TEST_IMM, MODRM_TEST_IMM, DestReg, Imm32 );
}

bool x64Encoder::TestRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_TEST_IMM, MODRM_TEST_IMM, DestReg, Imm32 );
}

bool x64Encoder::TestMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::TestMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::TestMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, Imm32 );
}


bool x64Encoder::TestRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_TEST_TOMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}
bool x64Encoder::TestRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_TEST_TOMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}
bool x64Encoder::TestRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_TEST_TOMEM, DestReg, AddressReg, IndexReg, Scale, Offset );
}


bool x64Encoder::TestAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_TEST_IMMA, Imm16 );
}

bool x64Encoder::TestAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_TEST_IMMA, Imm32 );
}

bool x64Encoder::TestAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_TEST_IMMA, Imm32 );
}


bool x64Encoder::TestReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, DestReg, Imm8 );
}

bool x64Encoder::TestReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, DestReg, Imm8 );
}

bool x64Encoder::TestReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, DestReg, Imm8 );
}

bool x64Encoder::TestMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::TestMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::TestMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::TestReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return TestReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return TestRegImm16 ( DestReg, ImmX );
	}
	
	return TestAcc16Imm16 ( ImmX );
}

bool x64Encoder::TestReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return TestReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return TestRegImm32 ( DestReg, ImmX );
	}
	
	return TestAcc32Imm32 ( ImmX );
}

bool x64Encoder::TestReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return TestReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return TestRegImm64 ( DestReg, ImmX );
	}
	
	return TestAcc64Imm32 ( ImmX );
}


bool x64Encoder::TestMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::TestMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::TestMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_TEST_IMM8, MODRM_TEST_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_TEST_IMM, MODRM_TEST_IMM, (char*) DestPtr, ImmX );
}





bool x64Encoder::XchgRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_XCHG, DestReg, SrcReg );
}

bool x64Encoder::XchgRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_XCHG, DestReg, SrcReg );
}

bool x64Encoder::XchgRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_XCHG, DestReg, SrcReg );
}


bool x64Encoder::XorRegReg16 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg16 ( X64OP_XOR, DestReg, SrcReg );
}

bool x64Encoder::XorRegReg32 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg32 ( X64OP_XOR, DestReg, SrcReg );
}

bool x64Encoder::XorRegReg64 ( int32_t DestReg, int32_t SrcReg )
{
	return x64EncodeRegReg64 ( X64OP_XOR, DestReg, SrcReg );
}

bool x64Encoder::XorRegImm16 ( int32_t DestReg, int16_t Imm16 )
{
	return x64EncodeReg16Imm16 ( X64OP_XOR_IMM, MODRM_XOR_IMM, DestReg, Imm16 );
}

bool x64Encoder::XorRegImm32 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg32Imm32 ( X64OP_XOR_IMM, MODRM_XOR_IMM, DestReg, Imm32 );
}

bool x64Encoder::XorRegImm64 ( int32_t DestReg, int32_t Imm32 )
{
	return x64EncodeReg64Imm32 ( X64OP_XOR_IMM, MODRM_XOR_IMM, DestReg, Imm32 );
}

bool x64Encoder::XorRegMem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_XOR8, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_XOR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_XOR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_XOR, DestReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem16 ( X64OP_XOR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem32 ( X64OP_XOR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMem64 ( X64OP_XOR_TOMEM, SrcReg, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemImm8 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm8 ( X64OP_XOR8_IMM8, MODRM_XOR8_IMM8, Imm8, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm16 ( X64OP_XOR_IMM, MODRM_XOR_IMM, Imm16, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm32 ( X64OP_XOR_IMM, MODRM_XOR_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}

bool x64Encoder::XorMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeMemImm64 ( X64OP_XOR_IMM, MODRM_XOR_IMM, Imm32, AddressReg, IndexReg, Scale, Offset );
}



bool x64Encoder::XorRegMem16 ( int32_t DestReg, int16_t* SrcPtr )
{
	return x64EncodeRipOffset16 ( X64OP_XOR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::XorRegMem32 ( int32_t DestReg, int32_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( X64OP_XOR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::XorRegMem64 ( int32_t DestReg, int64_t* SrcPtr )
{
	return x64EncodeRipOffset64 ( X64OP_XOR, DestReg, (char*) SrcPtr, true );
}

bool x64Encoder::XorMemReg16 ( int16_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset16 ( X64OP_XOR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::XorMemReg32 ( int32_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset32 ( X64OP_XOR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::XorMemReg64 ( int64_t* DestPtr, int32_t SrcReg )
{
	return x64EncodeRipOffset64 ( X64OP_XOR_TOMEM, SrcReg, (char*) DestPtr, true );
}

bool x64Encoder::XorMemImm16 ( int16_t* DestPtr, int16_t Imm16 )
{
	return x64EncodeRipOffsetImm16 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, Imm16 );
}

bool x64Encoder::XorMemImm32 ( int32_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm32 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, Imm32 );
}

bool x64Encoder::XorMemImm64 ( int64_t* DestPtr, int32_t Imm32 )
{
	return x64EncodeRipOffsetImm64 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, Imm32 );
}


bool x64Encoder::XorAcc16Imm16 ( int16_t Imm16 )
{
	return x64EncodeAcc16Imm16 ( X64OP_XOR_IMMA, Imm16 );
}

bool x64Encoder::XorAcc32Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc32Imm32 ( X64OP_XOR_IMMA, Imm32 );
}

bool x64Encoder::XorAcc64Imm32 ( int32_t Imm32 )
{
	return x64EncodeAcc64Imm32 ( X64OP_XOR_IMMA, Imm32 );
}


bool x64Encoder::XorReg16Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg16Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::XorReg32Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg32Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::XorReg64Imm8 ( int32_t DestReg, char Imm8 )
{
	return x64EncodeReg64Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, DestReg, Imm8 );
}

bool x64Encoder::XorMem16Imm8 ( int16_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset16Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::XorMem32Imm8 ( int32_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset32Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::XorMem64Imm8 ( int64_t* DestPtr, char Imm8 )
{
	return x64EncodeRipOffset64Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, Imm8 );
}

bool x64Encoder::XorReg16ImmX ( int32_t DestReg, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return XorReg16Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return XorRegImm16 ( DestReg, ImmX );
	}
	
	return XorAcc16Imm16 ( ImmX );
}

bool x64Encoder::XorReg32ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return XorReg32Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return XorRegImm32 ( DestReg, ImmX );
	}
	
	return XorAcc32Imm32 ( ImmX );
}

bool x64Encoder::XorReg64ImmX ( int32_t DestReg, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return XorReg64Imm8 ( DestReg, ImmX );
	}
	
	if ( DestReg )
	{
		return XorRegImm64 ( DestReg, ImmX );
	}
	
	return XorAcc64Imm32 ( ImmX );
}


bool x64Encoder::XorMem16ImmX ( int16_t* DestPtr, int16_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset16Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm16 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::XorMem32ImmX ( int32_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset32Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm32 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, ImmX );
}

bool x64Encoder::XorMem64ImmX ( int64_t* DestPtr, int32_t ImmX )
{
	if ( !ImmX ) return true;
	
	if ( ImmX >= -128 && ImmX <= 127 )
	{
		return x64EncodeRipOffset64Imm8 ( X64OP_XOR_IMM8, MODRM_XOR_IMM8, (char*) DestPtr, ImmX );
	}
	
	return x64EncodeRipOffsetImm64 ( X64OP_XOR_IMM, MODRM_XOR_IMM, (char*) DestPtr, ImmX );
}






// ** sse instruction definitions ** //


bool x64Encoder::stmxcsr(int32_t* pDstPtr)
{
	return x64EncodeRipOffset64(MAKE2OPCODE(0x0f, 0xae), 0x3, (char*)pDstPtr);
}

bool x64Encoder::ldmxcsr(int32_t* pSrcPtr)
{
	return x64EncodeRipOffset64(MAKE2OPCODE(0x0f, 0xae), 0x2, (char*)pSrcPtr);
}



bool x64Encoder::movhlps2 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVHLPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::movlhps2 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVLHPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::pinsrb1regreg ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRB, sseDestReg, sseSrcReg1, x64SrcReg2, Imm8 );
}

bool x64Encoder::pinsrw1regreg ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PINSRW, sseDestReg, sseSrcReg1, x64SrcReg2, Imm8 );
}

bool x64Encoder::pinsrd1regreg ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRD, sseDestReg, sseSrcReg1, x64SrcReg2, Imm8 );
}

bool x64Encoder::pinsrq1regreg ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, OPERAND_64BIT, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRQ, sseDestReg, sseSrcReg1, x64SrcReg2, Imm8 );
}


bool x64Encoder::pinsrb1regmem(int32_t sseDestReg, int32_t sseSrcReg1, char* pSrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRB, sseDestReg, sseSrcReg1, (char*)pSrcPtr, Imm8);
}
bool x64Encoder::pinsrw1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int16_t* pSrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PINSRW, sseDestReg, sseSrcReg1, (char*)pSrcPtr, Imm8);
}
bool x64Encoder::pinsrd1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int32_t* pSrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRD, sseDestReg, sseSrcReg1, (char*)pSrcPtr, Imm8);
}
bool x64Encoder::pinsrq1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int64_t* pSrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(0, OPERAND_64BIT, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PINSRQ, sseDestReg, sseSrcReg1, (char*)pSrcPtr, Imm8);
}



bool x64Encoder::vinserti128regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8)
{
	//VEX.256.66.0F3A.W0 38 / r ib VINSERTI128 ymm1, ymm2, xmm3 / m128, imm8
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8);
}
bool x64Encoder::vinserti128regmem(int32_t sseDestReg, int32_t sseSrcReg1, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, sseDestReg, sseSrcReg1, (char*)SrcPtr, Imm8);
}

bool x64Encoder::vinserti32x4_rri256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vinserti32x4_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, dst, src0, src1, Imm8, z, mask);
}

bool x64Encoder::vinserti32x8_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3a, dst, src0, src1, Imm8, z, mask);
}

bool x64Encoder::vinserti32x4_rmi256(int32_t dst, int32_t src0, void* SrcPtr, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, dst, src0, (char*)SrcPtr, Imm8, z, mask);
}
bool x64Encoder::vinserti32x4_rmi512(int32_t dst, int32_t src0, void* SrcPtr, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x38, dst, src0, (char*)SrcPtr, Imm8, z, mask);
}


bool x64Encoder::vinsertb_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x20, dst, src0, x64reg, Imm8, 0, 0);
}
bool x64Encoder::vinsertd_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x22, dst, src0, x64reg, Imm8, 0, 0);
}
bool x64Encoder::vinsertq_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x22, dst, src0, x64reg, Imm8, 0, 0);
}


bool x64Encoder::vinsertb_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x20, dst, src0, (char*)SrcPtr, Imm8, 0, 0);
}
bool x64Encoder::vinsertd_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x22, dst, src0, (char*)SrcPtr, Imm8, 0, 0);
}
bool x64Encoder::vinsertq_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x22, dst, src0, (char*)SrcPtr, Imm8, 0, 0);
}


bool x64Encoder::pextrb1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PEXTRB, sseSrcReg, 0, x64DestReg, Imm8 );
}

bool x64Encoder::pextrw1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PEXTRW, sseSrcReg, 0, x64DestReg, Imm8 );
}

bool x64Encoder::pextrd1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PEXTRD, sseSrcReg, 0, x64DestReg, Imm8 );
}

bool x64Encoder::pextrq1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, OPERAND_64BIT, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PEXTRQ, sseSrcReg, 0, x64DestReg, Imm8 );
}


bool x64Encoder::pextrd1memreg(int32_t* pDstPtr, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, sseSrcReg, 0, (char*)pDstPtr, Imm8);
}

bool x64Encoder::pextrq1memreg(int64_t* pDstPtr, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, sseSrcReg, 0, (char*)pDstPtr, Imm8);
}



bool x64Encoder::vextracti128regreg(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, sseSrcReg, 0, sseDestReg, Imm8);
}

bool x64Encoder::vextracti128memreg(void* DstPtr, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, sseSrcReg, 0, (char*)DstPtr, Imm8);
}


bool x64Encoder::vextracti32x4_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, src0, 0, dst, Imm8, z, mask);
}
bool x64Encoder::vextracti32x4_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, src0, 0, dst, Imm8, z, mask);
}

bool x64Encoder::vextracti64x4_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3b, src0, 0, dst, Imm8, z, mask);
}


bool x64Encoder::vextracti32x4_mri256(void* DstPtr, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, src0, 0, (char*)DstPtr, Imm8, z, mask);
}
bool x64Encoder::vextracti32x4_mri512(void* DstPtr, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x39, src0, 0, (char*)DstPtr, Imm8, z, mask);
}


bool x64Encoder::vextractb_rri128(int32_t x64reg, int32_t src0, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x14, src0, 0, x64reg, Imm8, 0, 0);
}
bool x64Encoder::vextractd_rri128(int32_t x64reg, int32_t src0, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, src0, 0, x64reg, Imm8, 0, 0);
}
bool x64Encoder::vextractq_rri128(int32_t x64reg, int32_t src0, char Imm8)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, src0, 0, x64reg, Imm8, 0, 0);
}


bool x64Encoder::vextractb_mri128(void* DstPtr, int32_t src0, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x14, src0, 0, (char*)DstPtr, Imm8, 0, 0);
}
bool x64Encoder::vextractd_mri128(void* DstPtr, int32_t src0, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, src0, 0, (char*)DstPtr, Imm8, 0, 0);
}
bool x64Encoder::vextractq_mri128(void* DstPtr, int32_t src0, char Imm8)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x16, src0, 0, (char*)DstPtr, Imm8, 0, 0);
}



bool x64Encoder::vpblendmd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}
bool x64Encoder::vpblendmd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}
bool x64Encoder::vpblendmd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}


bool x64Encoder::vpblendmq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}
bool x64Encoder::vpblendmq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}
bool x64Encoder::vpblendmq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_VPBLENDMD, dst, src0, src1, z, mask);
}




/*
bool x64Encoder::blendvps ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, int32_t x64SrcReg3 )
{
	return x64EncodeRegVImm8 ( 0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_BLENDVPS, sseDestReg, sseSrcReg1, sseSrcReg2, ( x64SrcReg3 << 4 ) );
}
*/


bool x64Encoder::pblendw1regregimm ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8 )
{
	return x64EncodeRegVImm8 (VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PBLENDW, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8 );
}
bool x64Encoder::pblendw2regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PBLENDW, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8);
}

bool x64Encoder::pblendw1regmemimm(int32_t sseDestReg, int32_t sseSrcReg1, void* SrcPtr, char Imm8)
{
	//return x64EncodeRegVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PBLENDW, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8);
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_PBLENDW, sseDestReg, sseSrcReg1, (char*)SrcPtr, Imm8);
}


bool x64Encoder::pblendwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	x64EncodePrefix ( 0x66 );
	x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x3a, 0x0e ), sseDestReg, sseSrcReg );
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::pblendwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32Imm8 ( MAKE3OPCODE( 0x0f, 0x3a, 0x0e ), sseDestReg, (char*) SrcPtr, Imm8 );
	//return x64EncodeImmediate8 ( Imm8 );
}


bool x64Encoder::pblendvb1regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, int32_t sseSrcReg3)
{
	return x64EncodeRegVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x4c, sseDestReg, sseSrcReg1, sseSrcReg2, (char) (sseSrcReg3 << 4));
}

bool x64Encoder::pblendvb2regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, int32_t sseSrcReg3)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x4c, sseDestReg, sseSrcReg1, sseSrcReg2, (char)(sseSrcReg3 << 4));
}

//bool x64Encoder::pblendvb1regmem(int32_t sseDestReg, void* SrcPtr)
//{
//}


bool x64Encoder::pblendvbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x10 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pblendvbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x10 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::knotb(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x44, dst, 0, src0);
}
bool x64Encoder::knotw(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x44, dst, 0, src0);
}
bool x64Encoder::knotd(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegV(0, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x44, dst, 0, src0);
}
bool x64Encoder::knotq(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegV(0, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x44, dst, 0, src0);
}


bool x64Encoder::kxnorb(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x46, dst, src0, src1);
}
bool x64Encoder::kxnorw(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x46, dst, src0, src1);
}
bool x64Encoder::kxnord(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x46, dst, src0, src1);
}
bool x64Encoder::kxnorq(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x46, dst, src0, src1);
}


bool x64Encoder::korb(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x45, dst, src0, src1);
}
bool x64Encoder::korw(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x45, dst, src0, src1);
}
bool x64Encoder::kord(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x45, dst, src0, src1);
}
bool x64Encoder::korq(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x45, dst, src0, src1);
}


bool x64Encoder::kandb(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x41, dst, src0, src1);
}
bool x64Encoder::kandw(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x41, dst, src0, src1);
}
bool x64Encoder::kandd(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x41, dst, src0, src1);
}
bool x64Encoder::kandq(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x41, dst, src0, src1);
}


bool x64Encoder::kandnb(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x42, dst, src0, src1);
}
bool x64Encoder::kandnw(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x42, dst, src0, src1);
}
bool x64Encoder::kandnd(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x42, dst, src0, src1);
}
bool x64Encoder::kandnq(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x42, dst, src0, src1);
}


bool x64Encoder::kmovbmskreg(int32_t dstmsk, int32_t srcreg0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x92, dstmsk, 0, srcreg0);
}
bool x64Encoder::kmovbregmsk(int32_t dstreg, int32_t srcmsk0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x93, dstreg, 0, srcmsk0);
}
bool x64Encoder::kmovbmskmem(int32_t dstmsk, char* srcptr0)
{
	return x64EncodeRipOffsetV(0, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x90, dstmsk, 0, (char*)srcptr0);
}

bool x64Encoder::kmovwmskreg(int32_t dstmsk, int32_t srcreg0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x92, dstmsk, 0, srcreg0);
}
bool x64Encoder::kmovwregmsk(int32_t dstreg, int32_t srcmsk0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x93, dstreg, 0, srcmsk0);
}
bool x64Encoder::kmovwmskmem(int32_t dstmsk, int16_t* srcptr0)
{
	return x64EncodeRipOffsetV(0, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x90, dstmsk, 0, (char*)srcptr0);
}

bool x64Encoder::kmovdmskreg(int32_t dstmsk, int32_t srcreg0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIXF2, VEX_PREFIX0F, 0x92, dstmsk, 0, srcreg0);
}
bool x64Encoder::kmovdregmsk(int32_t dstreg, int32_t srcmsk0)
{
	return x64EncodeRegRegV(0, 0, VEX_PREFIXF2, VEX_PREFIX0F, 0x93, dstreg, 0, srcmsk0);
}
bool x64Encoder::kmovdmskmem(int32_t dstmsk, int32_t* srcptr0)
{
	return x64EncodeRipOffsetV(0, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x90, dstmsk, 0, (char*)srcptr0);
}
bool x64Encoder::kmovdmemmsk(int32_t* dstptr0, int32_t srcmsk0)
{
	return x64EncodeRipOffsetV(0, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x91, srcmsk0, 0, (char*)dstptr0);
}

bool x64Encoder::kmovqmskreg(int32_t dstmsk, int32_t srcreg0)
{
	return x64EncodeRegRegV(0, 1, VEX_PREFIXF2, VEX_PREFIX0F, 0x92, dstmsk, 0, srcreg0);
}
bool x64Encoder::kmovqregmsk(int32_t dstreg, int32_t srcmsk0)
{
	return x64EncodeRegRegV(0, 1, VEX_PREFIXF2, VEX_PREFIX0F, 0x93, dstreg, 0, srcmsk0);
}
bool x64Encoder::kmovqmskmem(int32_t dstmsk, int64_t* srcptr0)
{
	return x64EncodeRipOffsetV(0, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x90, dstmsk, 0, (char*)srcptr0);
}


bool x64Encoder::kshiftrb(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x30, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftrw(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x30, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftrd(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x31, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftrq(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x31, dst, 0, src0, Imm8);
}


bool x64Encoder::kshiftlb(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x32, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftlw(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x32, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftld(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x33, dst, 0, src0, Imm8);
}
bool x64Encoder::kshiftlq(int32_t dst, int32_t src0, char Imm8)
{
	return x64EncodeRegVImm8(0, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x33, dst, 0, src0, Imm8);
}


bool x64Encoder::kunpackbw(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x4b, dst, src0, src1);
}
bool x64Encoder::kunpackwd(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x4b, dst, src0, src1);
}
bool x64Encoder::kunpackdq(int32_t dst, int32_t src0, int32_t src1)
{
	return x64EncodeRegRegV(1, 1, VEX_PREFIXNONE, VEX_PREFIX0F, 0x4b, dst, src0, src1);
}


bool x64Encoder::movaps128 ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movaps_to_mem128 ( void* DestPtr, int32_t sseSrcReg )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	//return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, AVXOP_MOVAPS_TOMEM ), sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, AVXOP_MOVAPS_TOMEM ), sseSrcReg, (char*) DestPtr );
}

bool x64Encoder::movaps_from_mem128 ( int32_t sseDestReg, void* SrcPtr )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	//return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, AVXOP_MOVAPS_TOMEM ), sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, AVXOP_MOVAPS_FROMMEM ), sseDestReg, (char*) SrcPtr );
}





bool x64Encoder::movdqa_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x6f ), sseDestReg, sseSrcReg );
}


bool x64Encoder::movdqa_memreg ( void* DestPtr, int32_t sseSrcReg )
{
	//cout << hex << "\nDestPtr=" << (unsigned int64_t)DestPtr;
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x7f ), sseSrcReg, (char*) DestPtr );
}

bool x64Encoder::movdqa_regmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x6f ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::movdqa_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, 0x7f ), sseSrcReg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqa_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, 0x6f ), sseDestReg, x64BaseReg, x64IndexReg, Scale, Offset );
}




bool x64Encoder::movdqu_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x6f ), sseDestReg, sseSrcReg );
}


bool x64Encoder::movdqu_memreg ( void* DestPtr, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x7f ), sseSrcReg, (char*) DestPtr );
}

bool x64Encoder::movdqu_regmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x6f ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::movdqu_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, 0x7f ), sseSrcReg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqu_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, 0x6f ), sseDestReg, x64BaseReg, x64IndexReg, Scale, Offset );
}








bool x64Encoder::movaps_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, AVXOP_MOVAPS_TOMEM ), sseSrcReg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movaps_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	//return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
	return x64EncodeRegMem32 ( MAKE2OPCODE ( 0x0f, AVXOP_MOVAPS_FROMMEM ), sseDestReg, x64BaseReg, x64IndexReg, Scale, Offset );
}



bool x64Encoder::movdqa1_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movdqa1_regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::movdqa1_memreg ( void* DestPtr, int32_t sseSrcReg )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, (char*) DestPtr );
}
bool x64Encoder::movdqa1_memreg ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}
bool x64Encoder::movdqa1_regmem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::movdqu1_memreg(void* DestPtr, int32_t sseSrcReg)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x7f, sseSrcReg, 0, (char*)DestPtr);
}
bool x64Encoder::movdqu1_memreg(int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x7f, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset);
}
bool x64Encoder::movdqu1_regmem(int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x6f, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset);
}


bool x64Encoder::movdqa2_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movdqa2_regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::movdqa2_memreg ( void* DestPtr, int32_t sseSrcReg )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, (char*) DestPtr );
}

bool x64Encoder::movdqa2_memreg ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqa2_regmem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::movsd1_regmem(int32_t sseDstReg, int64_t* pSrcPtr)
{
	return x64EncodeRipOffsetV(0, 1, VEX_PREFIXF2, VEX_PREFIX0F, 0x10, sseDstReg, 0, (char*)pSrcPtr);
}
bool x64Encoder::movsd1_memreg(int64_t* pDstPtr, int32_t sseSrcReg)
{
	return x64EncodeRipOffsetV(0, 1, VEX_PREFIXF2, VEX_PREFIX0F, 0x11, sseSrcReg, 0, (char*)pDstPtr);
}



/*
bool x64Encoder::movdqa_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqa_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqa256 ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movdqa_to_mem256 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movdqa_from_mem256 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MOVDQA_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}
*/

// movss
bool x64Encoder::movss_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_MOVSS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movss_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_MOVSS_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::movdqa32_rm512(int32_t avxDstReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::movdqa32_rm256(int32_t avxDstReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::movdqa32_rm128(int32_t avxDstReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}

bool x64Encoder::movdqa32_mr512(void* pDstPtr, int32_t avxDstReg, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, avxDstReg, 0, (char*)pDstPtr, z, mask);
}
bool x64Encoder::movdqa32_mr256(void* pDstPtr, int32_t avxDstReg, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, avxDstReg, 0, (char*)pDstPtr, z, mask);
}
bool x64Encoder::movdqa32_mr128(void* pDstPtr, int32_t avxDstReg, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_TOMEM, avxDstReg, 0, (char*)pDstPtr, z, mask);
}

bool x64Encoder::movdqa32_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}
bool x64Encoder::movdqa32_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}
bool x64Encoder::movdqa32_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}


bool x64Encoder::movdqa64_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}
bool x64Encoder::movdqa64_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}
bool x64Encoder::movdqa64_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, PP_VMOVDQA, MMMMM_VMOVDQA, AVXOP_MOVDQA_FROMMEM, dst, 0, src0, z, mask);
}




bool x64Encoder::pabsd_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, dst, 0, src0, z, mask);
}
bool x64Encoder::pabsd_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, dst, 0, src0, z, mask);
}
bool x64Encoder::pabsd_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, dst, 0, src0, z, mask);
}



bool x64Encoder::pabsb1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSB, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pabsw1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSW, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pabsd1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, sseDestReg, 0, sseSrcReg );
}


bool x64Encoder::pabsb1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSB, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pabsw1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSW, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pabsd1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, sseDestReg, 0, (char*) SrcPtr );
}



bool x64Encoder::pabsb2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSB, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::pabsw2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSW, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::pabsd2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::pabsb2regmem(int32_t sseDestReg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSB, sseDestReg, 0, (char*)SrcPtr);
}

bool x64Encoder::pabsw2regmem(int32_t sseDestReg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSW, sseDestReg, 0, (char*)SrcPtr);
}

bool x64Encoder::pabsd2regmem(int32_t sseDestReg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, PP_VPABS, MMMMM_VPABS, AVXOP_PABSD, sseDestReg, 0, (char*)SrcPtr);
}






bool x64Encoder::pabswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x1d ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pabswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x1d ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pabsdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x1e ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pabsdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x1e ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::paddd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::paddd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::paddd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask);
}

bool x64Encoder::paddq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::paddq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::paddq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask);
}



bool x64Encoder::paddd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::paddd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::paddd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}

bool x64Encoder::paddq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::paddq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::paddq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}


bool x64Encoder::paddd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, src1, z, mask);
}
bool x64Encoder::paddd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, src1, z, mask);
}
bool x64Encoder::paddd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, dst, src0, src1, z, mask);
}


bool x64Encoder::paddq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, src1, z, mask);
}
bool x64Encoder::paddq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, src1, z, mask);
}
bool x64Encoder::paddq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, dst, src0, src1, z, mask);
}

bool x64Encoder::paddb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::paddw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::paddd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::paddq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::paddb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::paddw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::paddd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::paddq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDQ, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::paddb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::paddw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::paddd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::paddq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xd4, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::paddbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xfc ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xfc ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::paddwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xfd ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xfd ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::padddregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xfe ), sseDestReg, sseSrcReg );
}



bool x64Encoder::padddregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xfe ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::paddqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xd4 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xd4 ), sseDestReg, (char*) SrcPtr );
}



bool x64Encoder::paddusb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDUSB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::paddusw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDUSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::paddusb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDUSB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::paddusw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDUSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::paddsb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDSB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::paddsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::paddsb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDSB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::paddsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PADDSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::paddsbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xec ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddsbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xec ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::paddswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xed ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xed ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::paddusbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xdc ), sseDestReg, sseSrcReg );
}

bool x64Encoder::paddusbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xdc ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::padduswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xdd ), sseDestReg, sseSrcReg );
}

bool x64Encoder::padduswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xdd ), sseDestReg, (char*) SrcPtr );
}



bool x64Encoder::pand_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, src1, z, mask);
}
bool x64Encoder::pand_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, src1, z, mask);
}
bool x64Encoder::pand_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, src1, z, mask);
}

bool x64Encoder::pandd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pandd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pandd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}

bool x64Encoder::pandq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pandq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pandq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask);
}


bool x64Encoder::pandd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pandd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pandd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}

bool x64Encoder::pandq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pandq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pandq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, dst, src0, (char*)pSrcPtr, z, mask, 1);
}


bool x64Encoder::pand1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::pand1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::pand2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PAND, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pandregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xdb ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pandregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xdb ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pandn_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PANDN, dst, src0, src1, z, mask);
}
bool x64Encoder::pandn_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PANDN, dst, src0, src1, z, mask);
}
bool x64Encoder::pandn_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PANDN, dst, src0, src1, z, mask);
}


bool x64Encoder::pandn1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PANDN, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pandn2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PANDN, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pandnregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xdf ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pandnregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xdf ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::vpcmpqeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask);
}

bool x64Encoder::vpcmpqeq_rrb128(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask, 1);
}
bool x64Encoder::vpcmpqeq_rrb256(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask, 1);
}
bool x64Encoder::vpcmpqeq_rrb512(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask, 1);
}


bool x64Encoder::vpcmpqeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpqne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpqlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpqle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpqgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpqge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}


bool x64Encoder::vpcmpqeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpqne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpqlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpqle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpqgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpqge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}


bool x64Encoder::vpcmpqeq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpqne_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpqlt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpqle_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpqgt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpqge_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}



bool x64Encoder::vpcmpdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}


bool x64Encoder::vpcmpdeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpdne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpdlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpdle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpdgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpdge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}


bool x64Encoder::vpcmpdeq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpdne_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpdlt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpdle_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpdgt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpdge_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPD, kdst, src0, src1, VPCMP_GE, 0, mask);
}



bool x64Encoder::vpcmpudeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpudne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpudlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpudle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpudgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpudge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPCMPUD, kdst, src0, src1, VPCMP_GE, 0, mask);
}

bool x64Encoder::vpcmpubeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::vpcmpubne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::vpcmpublt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::vpcmpuble_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_LE, 0, mask);
}
bool x64Encoder::vpcmpubgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::vpcmpubge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x3f, kdst, src0, src1, VPCMP_GE, 0, mask);
}


bool x64Encoder::pcmpeqb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pcmpeqw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pcmpeqd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::pcmpeqq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x29, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::pcmpeqb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pcmpeqw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pcmpeqd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::pcmpeqb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pcmpeqw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pcmpeqd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPEQD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::pcmpeqq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x29, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::pcmpeqbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x74 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpeqbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x74 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pcmpeqwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x75 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpeqwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x75 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pcmpeqdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x76 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpeqdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x76 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pcmpgtb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pcmpgtw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pcmpgtd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::pcmpgtq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x37, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::pcmpgtb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pcmpgtw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pcmpgtd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::pcmpgtb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pcmpgtw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pcmpgtd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PCMPGTD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::pcmpgtq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x37, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::pcmpgtbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x64 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpgtbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x64 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pcmpgtwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x65 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpgtwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x65 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pcmpgtdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x66 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pcmpgtdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x66 ), sseDestReg, (char*) SrcPtr );
}

// --------------


bool x64Encoder::pmaxud_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXUD, dst, src0, src1, z, mask);
}
bool x64Encoder::pmaxud_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXUD, dst, src0, src1, z, mask);
}
bool x64Encoder::pmaxud_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXUD, dst, src0, src1, z, mask);
}


bool x64Encoder::pmaxuw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3e, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxuw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3e, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxuwregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x3e), sseDestReg, sseSrcReg);
}

bool x64Encoder::pmaxuwregmem(int32_t sseDestReg, void* SrcPtr)
{
	x64EncodePrefix(0x66);
	return x64EncodeRipOffset32(MAKE3OPCODE(0x0f, 0x38, 0x3e), sseDestReg, (char*)SrcPtr);
}


bool x64Encoder::pmaxud1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3f, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxud2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3f, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxudregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x3f), sseDestReg, sseSrcReg);
}

bool x64Encoder::pmaxudregmem(int32_t sseDestReg, void* SrcPtr)
{
	x64EncodePrefix(0x66);
	return x64EncodeRipOffset32(MAKE3OPCODE(0x0f, 0x38, 0x3f), sseDestReg, (char*)SrcPtr);
}


// --------------

bool x64Encoder::pmaxsd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, dst, src0, src1, z, mask);
}
bool x64Encoder::pmaxsd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, dst, src0, src1, z, mask);
}
bool x64Encoder::pmaxsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, dst, src0, src1, z, mask);
}


bool x64Encoder::pmaxsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMAXSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pmaxsw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMAXSW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xee ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmaxswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xee ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmaxsd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::pmaxsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMAXSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pmaxsd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::pmaxsd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMAXSD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pmaxsdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x3d ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmaxsdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x3d ), sseDestReg, (char*) SrcPtr );
}

// -----------------

bool x64Encoder::pminud_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINUD, dst, src0, src1, z, mask);
}
bool x64Encoder::pminud_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINUD, dst, src0, src1, z, mask);
}
bool x64Encoder::pminud_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINUD, dst, src0, src1, z, mask);
}


bool x64Encoder::pminuw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3a, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminuw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3a, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminuwregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x3a), sseDestReg, sseSrcReg);
}

bool x64Encoder::pminuwregmem(int32_t sseDestReg, void* SrcPtr)
{
	x64EncodePrefix(0x66);
	return x64EncodeRipOffset32(MAKE3OPCODE(0x0f, 0x38, 0x3a), sseDestReg, (char*)SrcPtr);
}


bool x64Encoder::pminud1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3b, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminuw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3a, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pminud1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3b, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}



bool x64Encoder::pminud2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x3b, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminudregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x3b), sseDestReg, sseSrcReg);
}

bool x64Encoder::pminudregmem(int32_t sseDestReg, void* SrcPtr)
{
	x64EncodePrefix(0x66);
	return x64EncodeRipOffset32(MAKE3OPCODE(0x0f, 0x38, 0x3b), sseDestReg, (char*)SrcPtr);
}



// ----------

bool x64Encoder::pminsd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, dst, src0, src1, z, mask);
}
bool x64Encoder::pminsd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, dst, src0, src1, z, mask);
}
bool x64Encoder::pminsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, dst, src0, src1, z, mask);
}


bool x64Encoder::pminsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMINSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pminsw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMINSW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xea ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pminswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xea ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pminsd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::pminsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PMINSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::pminsd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::pminsd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMINSD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pminsdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x39 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pminsdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x39 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmovsxdq1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMOVSXDQ, sseDestReg, 0, sseSrcReg );
}
bool x64Encoder::pmovsxdq2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PMOVSXDQ, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::pmovsxdqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x25 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovsxdqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x25 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::por_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}
bool x64Encoder::por_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}
bool x64Encoder::por_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}


bool x64Encoder::pord_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pord_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pord_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}


bool x64Encoder::pord_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pord_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::pord_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}


bool x64Encoder::porq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}
bool x64Encoder::porq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}
bool x64Encoder::porq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, src1, z, mask);
}

bool x64Encoder::porq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::porq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::porq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask);
}


bool x64Encoder::porq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::porq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::porq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, dst, src0, (char*)pSrcPtr, z, mask, 1);
}



bool x64Encoder::por1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::por1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::por2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_POR, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::porregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xeb ), sseDestReg, sseSrcReg );
}

bool x64Encoder::porregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xeb ), sseDestReg, (char*) SrcPtr );
}



bool x64Encoder::packsdw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSDW, dst, src0, src1, z, mask);
}
bool x64Encoder::packsdw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSDW, dst, src0, src1, z, mask);
}
bool x64Encoder::packsdw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSDW, dst, src0, src1, z, mask);
}


bool x64Encoder::packswb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSWB, dst, src0, src1, z, mask);
}
bool x64Encoder::packswb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSWB, dst, src0, src1, z, mask);
}
bool x64Encoder::packswb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSWB, dst, src0, src1, z, mask);
}



bool x64Encoder::packsdwregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x6b), sseDestReg, sseSrcReg);
}

bool x64Encoder::packswbregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x63), sseDestReg, sseSrcReg);
}

bool x64Encoder::packsdw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSDW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::packswb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSWB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::packsdw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSDW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::packswb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKSWB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::packusdw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PACKUSDW, dst, src0, src1, z, mask);
}
bool x64Encoder::packusdw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PACKUSDW, dst, src0, src1, z, mask);
}
bool x64Encoder::packusdw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PACKUSDW, dst, src0, src1, z, mask);
}

bool x64Encoder::packusdw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PACKUSDW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::packuswb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKUSWB, dst, src0, src1, z, mask);
}
bool x64Encoder::packuswb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKUSWB, dst, src0, src1, z, mask);
}
bool x64Encoder::packuswb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKUSWB, dst, src0, src1, z, mask);
}

bool x64Encoder::packuswb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKUSWB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::packsswb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x63, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::packsswb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x63, sseDestReg, sseSrc1Reg, (char*)SrcPtr);
}
bool x64Encoder::packssdw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6b, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::packssdw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6b, sseDestReg, sseSrc1Reg, (char*)SrcPtr);
}



bool x64Encoder::packusdw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PACKUSDW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::packuswb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PACKUSWB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::packuswbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x67 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::packuswbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x67 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::packusdwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x2b ), sseDestReg, sseSrcReg );
}

bool x64Encoder::packusdwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x2b ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::packsswbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x63 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::packsswbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x63 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::packssdwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x6b ), sseDestReg, sseSrcReg );
}

bool x64Encoder::packssdwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x6b ), sseDestReg, (char*) SrcPtr );
}







bool x64Encoder::pmovzxbw1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBW, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxbd1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBD, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxbq1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBQ, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxwd1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWD, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxwq1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWQ, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxdq1regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXDQ, sseDestReg, 0, sseSrcReg );
}


bool x64Encoder::pmovzxbw1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBW, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbd1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBD, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbq1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBQ, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwd1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWD, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwq1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWQ, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxdq1regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_128BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXDQ, sseDestReg, 0, (char*) SrcPtr );
}




bool x64Encoder::pmovzxbw2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBW, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxbd2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBD, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxbq2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBQ, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxwd2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWD, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxwq2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWQ, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::pmovzxdq2regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXDQ, sseDestReg, 0, sseSrcReg );
}


bool x64Encoder::pmovzxbw2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBW, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbd2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBD, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbq2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXBQ, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwd2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWD, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwq2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXWQ, sseDestReg, 0, (char*) SrcPtr );
}

bool x64Encoder::pmovzxdq2regmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffsetV ( VEX_256BIT, 0, PP_VPMOVZX, MMMMM_VPMOVZX, AVXOP_PMOVZXDQ, sseDestReg, 0, (char*) SrcPtr );
}








bool x64Encoder::pmovzxbwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x30 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxbwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x30 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x31 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxbdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x31 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pmovzxbqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x32 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxbqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x32 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x33 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxwdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x33 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pmovzxwqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x34 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxwqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x34 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pmovzxdqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x35 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmovzxdqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x35 ), sseDestReg, (char*) SrcPtr );
}






bool x64Encoder::pmuludq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, PP_PMULUDQ, MMMMM_PMULUDQ, AVXOP_PMULUDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::pmuludq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_PMULUDQ, MMMMM_PMULUDQ, AVXOP_PMULUDQ, sseDestReg, sseSrc1Reg, (char*)SrcPtr);
}


bool x64Encoder::pmuludq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_PMULUDQ, MMMMM_PMULUDQ, AVXOP_PMULUDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::pshufd_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, dst, 0, src0, Imm8, z, mask);
}
bool x64Encoder::pshufd_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, dst, 0, src0, Imm8, z, mask);
}
bool x64Encoder::pshufd_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, dst, 0, src0, Imm8, z, mask);
}

bool x64Encoder::pshufb_rrm512(int32_t avxDstReg, int32_t avxSrcReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x00, avxDstReg, avxSrcReg, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pshufb_rrm256(int32_t avxDstReg, int32_t avxSrcReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x00, avxDstReg, avxSrcReg, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::pshufb_rrm128(int32_t avxDstReg, int32_t avxSrcReg, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x00, avxDstReg, avxSrcReg, (char*)pSrcPtr, z, mask);
}


bool x64Encoder::pshufd_rmi512(int32_t avxDstReg, void* pSrcPtr, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, avxDstReg, 0, (char*)pSrcPtr, Imm8, z, mask);
}
bool x64Encoder::pshufd_rmi256(int32_t avxDstReg, void* pSrcPtr, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, avxDstReg, 0, (char*)pSrcPtr, Imm8, z, mask);
}
bool x64Encoder::pshufd_rmi128(int32_t avxDstReg, void* pSrcPtr, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, avxDstReg, 0, (char*)pSrcPtr, Imm8, z, mask);
}



bool x64Encoder::pshufb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PSHUFB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pshufb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PSHUFB, sseDestReg, sseSrc1Reg, (char*)SrcPtr);
}


bool x64Encoder::pshufb2regmem(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, AVXOP_PSHUFB, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset);
}


bool x64Encoder::pshufd1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, sseDestReg, 0, sseSrcReg, Imm8 );
}

bool x64Encoder::pshufd2regregimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, sseDestReg, 0, sseSrcReg, Imm8);
}

bool x64Encoder::pshufd1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, sseDestReg, 0, (char*) SrcPtr, Imm8);
}

bool x64Encoder::pshufd2regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSHUFD, sseDestReg, 0, (char*)SrcPtr, Imm8);
}


bool x64Encoder::pshufps1regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8)
{
	return x64EncodeRegVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0xc6, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8);
}
bool x64Encoder::pshufps2regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0xc6, sseDestReg, sseSrcReg1, sseSrcReg2, Imm8);
}



bool x64Encoder::pshufbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	// note: fourth byte in opcode is actually zero, but I didn't expect this when I started the project
	return x64EncodeRegReg32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0xff ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pshufbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	// note: fourth byte in opcode is actually zero, but I didn't expect this when I started the project
	return x64EncodeRipOffset32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0xff ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pshufdregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	x64EncodePrefix ( 0x66 );
	x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, sseSrcReg );
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::pshufdregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32Imm8 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, (char*) SrcPtr, Imm8 );
	//return x64EncodeImmediate8 ( Imm8 );
}




bool x64Encoder::pshufhw1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_PSHUFHW, sseDestReg, 0, sseSrcReg, Imm8 );
}
bool x64Encoder::pshufhw1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_PSHUFHW, sseDestReg, 0, (char*)SrcPtr, Imm8);
}


bool x64Encoder::pshufhwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	x64EncodePrefix ( 0xf3 );
	x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, sseSrcReg );
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::pshufhwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRipOffset32Imm8 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, (char*) SrcPtr, Imm8 );
	//return x64EncodeImmediate8 ( Imm8 );
}


bool x64Encoder::pshuflw1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_PSHUFLW, sseDestReg, 0, sseSrcReg, Imm8 );
}
bool x64Encoder::pshuflw1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8)
{
	return x64EncodeRipOffsetVImm8(VEX_128BIT, 0, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_PSHUFLW, sseDestReg, 0, (char*)SrcPtr, Imm8);
}

bool x64Encoder::pshuflwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	x64EncodePrefix ( 0xf2 );
	x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, sseSrcReg );
	return x64EncodeImmediate8 ( Imm8 );
}

bool x64Encoder::pshuflwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 )
{
	x64EncodePrefix ( 0xf2 );
	return x64EncodeRipOffset32Imm8 ( MAKE2OPCODE( 0x0f, 0x70 ), sseDestReg, (char*) SrcPtr, Imm8 );
	//return x64EncodeImmediate8 ( Imm8 );
}



bool x64Encoder::psrld_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD_IMM, MODRM_PSRLD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrld_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD_IMM, MODRM_PSRLD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrld_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD_IMM, MODRM_PSRLD_IMM, dst, src0, Imm8, z, mask);
}


bool x64Encoder::psrlq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLQ_IMM, MODRM_PSRLQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrlq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLQ_IMM, MODRM_PSRLQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrlq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLQ_IMM, MODRM_PSRLQ_IMM, dst, src0, Imm8, z, mask);
}



bool x64Encoder::pslld_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD_IMM, MODRM_PSLLD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::pslld_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD_IMM, MODRM_PSLLD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::pslld_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD_IMM, MODRM_PSLLD_IMM, dst, src0, Imm8, z, mask);
}


bool x64Encoder::psllq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLQ_IMM, MODRM_PSLLQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psllq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLQ_IMM, MODRM_PSLLQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psllq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLQ_IMM, MODRM_PSLLQ_IMM, dst, src0, Imm8, z, mask);
}


bool x64Encoder::pslldq1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLDQ, MODRM_PSLLDQ, sseDestReg, sseSrcReg, Imm8 );
}

bool x64Encoder::psllw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psllw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLW_IMM, MODRM_PSLLW_IMM, sseDestReg, sseSrcReg, Imm8 );
}

bool x64Encoder::pslld1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::pslld1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD_IMM, MODRM_PSLLD_IMM, sseDestReg, sseSrcReg, Imm8 );
}
bool x64Encoder::psllq1regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x73, 0x6, sseDestReg, sseSrcReg, Imm8);
}



bool x64Encoder::pslldq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLDQ, MODRM_PSLLDQ, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::psllw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psllw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLW_IMM, MODRM_PSLLW_IMM, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::pslld2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pslld2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSLLD_IMM, MODRM_PSLLD_IMM, sseDestReg, sseSrcReg, Imm8);
}
bool x64Encoder::psllq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x73, 0x6, sseDestReg, sseSrcReg, Imm8);
}



bool x64Encoder::psllwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psllwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psllwregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x71 ), 0x6, sseDestReg, Imm8 );
}

bool x64Encoder::pslldregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf2 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pslldregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf2 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::pslldregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x72 ), 0x6, sseDestReg, Imm8 );
}

bool x64Encoder::psllqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xf3 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psllqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xf3 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psllqregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	x64EncodePrefix ( 0x66 );
	return x64EncodeReg32Imm8 ( MAKE2OPCODE( 0x0f, 0x73 ), 0x6, sseDestReg, Imm8 );
}



bool x64Encoder::psrad_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD_IMM, MODRM_PSRAD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrad_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD_IMM, MODRM_PSRAD_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psrad_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD_IMM, MODRM_PSRAD_IMM, dst, src0, Imm8, z, mask);
}


bool x64Encoder::psraq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAQ_IMM, MODRM_PSRAQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psraq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAQ_IMM, MODRM_PSRAQ_IMM, dst, src0, Imm8, z, mask);
}
bool x64Encoder::psraq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAQ_IMM, MODRM_PSRAQ_IMM, dst, src0, Imm8, z, mask);
}



bool x64Encoder::psraw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psraw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAW_IMM, MODRM_PSRAW_IMM, sseDestReg, sseSrcReg, Imm8 );
}

bool x64Encoder::psrad1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psrad1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD_IMM, MODRM_PSRAD_IMM, sseDestReg, sseSrcReg, Imm8 );
}


bool x64Encoder::psraw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psraw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAW_IMM, MODRM_PSRAW_IMM, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::psrad2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psrad2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRAD_IMM, MODRM_PSRAD_IMM, sseDestReg, sseSrcReg, Imm8);
}




bool x64Encoder::psrawregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe1 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psrawregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe1 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psrawregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x71 ), 0x4, sseDestReg, Imm8 );
}

bool x64Encoder::psradregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe2 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psradregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe2 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psradregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x72 ), 0x4, sseDestReg, Imm8 );
}




	
bool x64Encoder::psrldq1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLDQ_IMM, MODRM_PSRLDQ_IMM, sseDestReg, sseSrcReg, Imm8 );
}
bool x64Encoder::psrlq1regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLQ_IMM, MODRM_PSRLQ_IMM, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::psrlw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psrlw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLW_IMM, MODRM_PSRLW_IMM, sseDestReg, sseSrcReg, Imm8 );
}

bool x64Encoder::psrld1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psrld1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD_IMM, MODRM_PSRLD_IMM, sseDestReg, sseSrcReg, Imm8 );
}



bool x64Encoder::psrldq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLDQ_IMM, MODRM_PSRLDQ_IMM, sseDestReg, sseSrcReg, Imm8);
}
bool x64Encoder::psrlq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLQ_IMM, MODRM_PSRLQ_IMM, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::psrlw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psrlw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLW_IMM, MODRM_PSRLW_IMM, sseDestReg, sseSrcReg, Imm8);
}

bool x64Encoder::psrld2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psrld2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSRLD_IMM, MODRM_PSRLD_IMM, sseDestReg, sseSrcReg, Imm8);
}



bool x64Encoder::psrlwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd1 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psrlwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd1 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psrlwregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x71 ), 0x2, sseDestReg, Imm8 );
}

bool x64Encoder::psrldregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd2 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psrldregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd2 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psrldregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x72 ), 0x2, sseDestReg, Imm8 );
}

bool x64Encoder::psrlqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd3 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psrlqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd3 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psrlqregimm ( int32_t sseDestReg, char Imm8 )
{
	//return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf1 ), sseDestReg, sseSrcReg );
	return x64EncodeReg32Imm8 ( MAKE3OPCODE( 0x66, 0x0f, 0x73 ), 0x2, sseDestReg, Imm8 );
}


bool x64Encoder::psubd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::psubd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::psubd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask);
}


bool x64Encoder::psubq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::psubq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::psubq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask);
}



bool x64Encoder::psubd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::psubd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::psubd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, (char*)pSrcPtr, z, mask, 1);
}

bool x64Encoder::psubq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::psubq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}
bool x64Encoder::psubq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, (char*)pSrcPtr, z, mask, 1);
}



bool x64Encoder::psubd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, src1, z, mask);
}
bool x64Encoder::psubd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, src1, z, mask);
}
bool x64Encoder::psubd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, dst, src0, src1, z, mask);
}


bool x64Encoder::psubq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, src1, z, mask);
}
bool x64Encoder::psubq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, src1, z, mask);
}
bool x64Encoder::psubq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBQ, dst, src0, src1, z, mask);
}


bool x64Encoder::psubb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psubw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psubd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::psubq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xfb, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::psubb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::psubw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::psubd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::psubb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBB, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psubw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBW, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psubd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::psubq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xfb, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::psubbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf8 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf8 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psubwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf9 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf9 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psubdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xfa ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xfa ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::psubqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xfb ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xfb ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::psubusb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBUSB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psubusw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBUSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::psubusb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBUSB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::psubusw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBUSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::psubsb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBSB, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::psubsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBSW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::psubsb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBSB, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::psubsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PSUBSW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::psubsbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe8 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubsbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe8 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psubswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe9 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe9 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::psubusbregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd8 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubusbregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd8 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::psubuswregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd9 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::psubuswregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd9 ), sseDestReg, (char*) SrcPtr );
}




bool x64Encoder::punpckhbw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHBW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::punpckhwd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHWD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::punpckhdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::punpckhqdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHQDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::punpckhbw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHBW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpckhwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHWD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpckhdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHDQ, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpckhqdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKHQDQ, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::punpckhbwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x68 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpckhbwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x68 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::punpckhwdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x69 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpckhwdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x69 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::punpckhdqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6a ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpckhdqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6a ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::punpckhqdqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6d ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpckhqdqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6d ), sseDestReg, (char*) SrcPtr );
}





bool x64Encoder::punpcklbw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLBW, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::punpcklwd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLWD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::punpckldq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::punpcklqdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLQDQ, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::punpcklbw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLBW, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpcklwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLWD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpckldq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLDQ, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::punpcklqdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PUNPCKLQDQ, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}



bool x64Encoder::punpcklbwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x60 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpcklbwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x60 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::punpcklwdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x61 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpcklwdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x61 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::punpckldqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x62 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpckldqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x62 ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::punpcklqdqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6c ), sseDestReg, sseSrcReg );
}

bool x64Encoder::punpcklqdqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0x6c ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pxor_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, dst, src0, src1, z, mask);
}
bool x64Encoder::pxor_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, dst, src0, src1, z, mask);
}
bool x64Encoder::pxor_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, dst, src0, src1, z, mask);
}


bool x64Encoder::pxor1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::pxor1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}

bool x64Encoder::pxor2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_PXOR, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::pxorregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0xef ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pxorregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0xef ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::vpmovb2m_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x29, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovb2m_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x29, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovb2m_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x29, dst, 0, src0, 0, 0);
}


bool x64Encoder::vpmovd2m_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x39, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovd2m_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x39, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovd2m_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x39, dst, 0, src0, 0, 0);
}


bool x64Encoder::vpmovm2b_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2b_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2b_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}

bool x64Encoder::vpmovm2w_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2w_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2w_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x28, dst, 0, src0, 0, 0);
}

bool x64Encoder::vpmovm2d_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2d_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2d_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}

bool x64Encoder::vpmovm2q_rr512(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2q_rr256(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}
bool x64Encoder::vpmovm2q_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x38, dst, 0, src0, 0, 0);
}




bool x64Encoder::vptestmb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}

bool x64Encoder::vptestmw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}


bool x64Encoder::vptestmd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}


bool x64Encoder::vptestmd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestmd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestmd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}


bool x64Encoder::vptestmd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr32, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr32, 0, mask, 1);
}
bool x64Encoder::vptestmd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr32, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr32, 0, mask, 1);
}
bool x64Encoder::vptestmd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr32, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr32, 0, mask, 1);
}


bool x64Encoder::vptestmq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestmq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}


bool x64Encoder::vptestmq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestmq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestmq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}


bool x64Encoder::vptestmq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask, 1);
}
bool x64Encoder::vptestmq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask, 1);
}
bool x64Encoder::vptestmq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask, 1);
}


bool x64Encoder::vptestnmb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}

bool x64Encoder::vptestnmw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x26, dst, src0, src1, 0, mask);
}

bool x64Encoder::vptestnmd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}

bool x64Encoder::vptestnmd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestnmd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestnmd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}


bool x64Encoder::vptestnmq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}
bool x64Encoder::vptestnmq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, src1, 0, mask);
}

bool x64Encoder::vptestnmq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestnmq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}
bool x64Encoder::vptestnmq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIXF3, VEX_PREFIX0F38, 0x27, dst, src0, (char*)pSrcPtr, 0, mask);
}



bool x64Encoder::ptest1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	//x64EncodePrefix(0x66);
	//return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x17), sseDestReg, sseSrcReg);
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x17, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::ptest2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	//x64EncodePrefix(0x66);
	//return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x17), sseDestReg, sseSrcReg);
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x17, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::ptestregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x0f, 0x38, 0x17 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::ptestregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x0f, 0x38, 0x17 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmuldqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x28 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmuldqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x28 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmuldq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x28, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::pmuldq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x28, sseDestReg, sseSrc1Reg, (char*)SrcPtr);
}



bool x64Encoder::pmuludqregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf4 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmuludqregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf4 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmulldregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x40 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmulldregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE4OPCODE( 0x66, 0x0f, 0x38, 0x40 ), sseDestReg, (char*) SrcPtr );
}



bool x64Encoder::pmullwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd5 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmullwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xd5 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmullw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xd5, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::pmulhwregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe5 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmulhwregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xe5 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmulhw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xe5, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::pmaddwdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf5 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::pmaddwdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	return x64EncodeRipOffset32 ( MAKE3OPCODE( 0x66, 0x0f, 0xf5 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::pmaddwd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xf5, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::pmaddwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0xf5, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}



bool x64Encoder::psignb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x08, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psignw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x09, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psignd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x0a, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::psignb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x08, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psignw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x09, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::psignd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x0a, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::psignbregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x08), sseDestReg, sseSrcReg);
}

bool x64Encoder::psignwregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x09), sseDestReg, sseSrcReg);
}

bool x64Encoder::psigndregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0x66);
	return x64EncodeRegReg32(MAKE3OPCODE(0x0f, 0x38, 0x0a), sseDestReg, sseSrcReg);
}



bool x64Encoder::vpsllvd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x47, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vpsravd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x46, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vpsrlvd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x45, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::vpsllvd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x47, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vpsravd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x46, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vpsrlvd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x45, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}



bool x64Encoder::movd1_regmem(int32_t sseDestReg, int32_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, sseDestReg, 0, (char*)SrcPtr);
}
bool x64Encoder::movd1_memreg(int32_t* DstPtr,int32_t sseSrcReg)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, sseSrcReg, 0, (char*)DstPtr);
}

bool x64Encoder::movq1_regmem(int32_t sseDestReg, int64_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, sseDestReg, 0, (char*)SrcPtr);
}
bool x64Encoder::movq1_memreg(int64_t* DstPtr, int32_t sseSrcReg)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, sseSrcReg, 0, (char*)DstPtr);
}


bool x64Encoder::movd_to_sse128(int32_t sseDestReg, int32_t x64SrcReg)
{
	//return x64EncodeRegReg32(MAKE3OPCODE(0x66, X64OP1_MOVD_FROMMEM, X64OP2_MOVD_FROMMEM), sseDestReg, x64SrcReg);
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, sseDestReg, 0, x64SrcReg);
}
bool x64Encoder::movd_from_sse128(int32_t x64DestReg, int32_t sseSrcReg)
{
	//return x64EncodeRegReg32(MAKE3OPCODE(0x66, X64OP1_MOVD_TOMEM, X64OP2_MOVD_TOMEM), x64DestReg, sseSrcReg);
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, sseSrcReg, 0, x64DestReg);
}


bool x64Encoder::movq_to_sse128(int32_t sseDestReg, int32_t x64SrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_MOVQ_FROMMEM, sseDestReg, 0, x64SrcReg);
}
bool x64Encoder::movq_from_sse128(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_MOVQ_TOMEM, sseSrcReg, 0, x64DestReg);
}


bool x64Encoder::movd_to_sse_rr128(int32_t dst, int32_t x64reg)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, dst, 0, x64reg, 0, 0);
}
bool x64Encoder::movd_to_x64_rr128(int32_t x64reg, int32_t dst)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, dst, 0, x64reg, 0, 0);
}

bool x64Encoder::movd_to_sse_rm128(int32_t dst, int32_t* pSrcPtr)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, dst, 0, (char*)pSrcPtr, 0, 0);
}
bool x64Encoder::movd_to_x64_mr128(int32_t* pDstPtr, int32_t src0)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, src0, 0, (char*)pDstPtr, 0, 0);
}



bool x64Encoder::movq_to_sse_rr128(int32_t dst, int32_t x64reg)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, dst, 0, x64reg, 0, 0);
}
bool x64Encoder::movq_to_x64_rr128(int32_t x64reg, int32_t dst)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, dst, 0, x64reg, 0, 0);
}

bool x64Encoder::movq_to_sse_rm128(int32_t dst, int64_t* pSrcPtr)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x6e, dst, 0, (char*)pSrcPtr, 0, 0);
}
bool x64Encoder::movq_to_x64_mr128(int64_t* pDstPtr, int32_t src0)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, 0x7e, src0, 0, (char*)pDstPtr, 0, 0);
}



// sse floating point instructions

// *** SSE instructions *** //

bool x64Encoder::movd_to_sse ( int32_t sseDestReg, int32_t x64SrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0x66, X64OP1_MOVD_FROMMEM, X64OP2_MOVD_FROMMEM ), sseDestReg, x64SrcReg );
}

bool x64Encoder::movd_from_sse ( int32_t x64DestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32(MAKE3OPCODE(0x66, X64OP1_MOVD_TOMEM, X64OP2_MOVD_TOMEM), sseSrcReg, x64DestReg);
}

bool x64Encoder::movq_to_sse ( int32_t sseDestReg, int32_t x64SrcReg )
{
	x64CodeArea [ x64NextOffset++ ] = 0x66;
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVQ_FROMMEM, X64OP2_MOVQ_FROMMEM ), sseDestReg, x64SrcReg );
}

bool x64Encoder::movq_from_sse ( int32_t x64DestReg, int32_t sseSrcReg )
{
	x64CodeArea [ x64NextOffset++ ] = 0x66;
	return x64EncodeRegReg64 ( MAKE2OPCODE( X64OP1_MOVQ_TOMEM, X64OP2_MOVQ_TOMEM ), x64DestReg, sseSrcReg );
}


bool x64Encoder::movd_regmem ( int32_t sseDestReg, int32_t* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVD_FROMMEM, X64OP2_MOVD_FROMMEM ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::movd_memreg ( int32_t* DstPtr, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVD_TOMEM, X64OP2_MOVD_TOMEM ), sseSrcReg, (char*) DstPtr );
}

bool x64Encoder::movq_regmem ( int32_t sseDestReg, int32_t* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVQ_FROMMEM, X64OP2_MOVQ_FROMMEM ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::movq_memreg ( int32_t* DstPtr, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset64 ( MAKE2OPCODE( X64OP1_MOVQ_TOMEM, X64OP2_MOVQ_TOMEM ), sseSrcReg, (char*) DstPtr );
}

/*
bool x64Encoder::movd_regmem ( int32_t sseDestReg, int32_t* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x6e ), sseDestReg, (char*) SrcPtr );
}

bool x64Encoder::movq_regmem ( int32_t sseDestReg, int64_t* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset64 ( MAKE2OPCODE( 0x0f, 0x6e ), sseDestReg, (char*) SrcPtr );
}
*/


// *** SSE floating point instructions *** //

bool x64Encoder::mulss(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0xf3);
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x59), sseDestReg, sseSrcReg);
}

bool x64Encoder::addpsregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x58), sseDestReg, sseSrcReg);
}

bool x64Encoder::mulpsregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x59), sseDestReg, sseSrcReg);
}


bool x64Encoder::addsd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf2, X64OP1_ADDSD, X64OP2_ADDSD ), sseDestReg, sseSrcReg );
}

bool x64Encoder::subsd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf2, X64OP1_SUBSD, X64OP2_SUBSD ), sseDestReg, sseSrcReg );
}

bool x64Encoder::mulsd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf2, X64OP1_MULSD, X64OP2_MULSD ), sseDestReg, sseSrcReg );
}

bool x64Encoder::divsd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf2, X64OP1_DIVSD, X64OP2_DIVSD ), sseDestReg, sseSrcReg );
}

bool x64Encoder::sqrtsd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf2, X64OP1_SQRTSD, X64OP2_SQRTSD ), sseDestReg, sseSrcReg );
}


bool x64Encoder::addpdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x58 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::addpdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x58 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::mulpdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, 0x59 ), sseDestReg, sseSrcReg );
}

bool x64Encoder::mulpdregmem ( int32_t sseDestReg, void* SrcPtr )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( 0x0f, 0x59 ), sseDestReg, (char*) SrcPtr );
}


bool x64Encoder::cvttss2si ( int32_t x64DestReg, int32_t sseSrcReg )
{
	return x64EncodeRegReg32 ( MAKE3OPCODE( 0xf3, X64OP1_CVTTSS2SI, X64OP2_CVTTSS2SI ), x64DestReg, sseSrcReg );
}

bool x64Encoder::cvttps2dq_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTTPS2DQ, dst, 0, src0, z, mask);
}
bool x64Encoder::cvttps2dq_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTTPS2DQ, dst, 0, src0, z, mask);
}
bool x64Encoder::cvttps2dq_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTTPS2DQ, dst, 0, src0, z, mask);
}

bool x64Encoder::cvttss2si_rr128(int32_t dst, int32_t src0)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x2c, dst, 0, src0, 0, 0);
}

bool x64Encoder::cvttps2dq1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV ( VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTTPS2DQ, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::cvttps2dq2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTTPS2DQ, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::cvttss2si1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x2c, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::cvtss2sd1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x5a, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::cvttps2dq_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CVTTPS2DQ, X64OP2_CVTTPS2DQ ), sseDestReg, sseSrcReg );
}

bool x64Encoder::cvtss2sdregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	x64EncodePrefix(0xf3);
	return x64EncodeRegReg32(MAKE2OPCODE(X64OP1_CVTSS2SD, X64OP2_CVTSS2SD), sseDestReg, sseSrcReg);
}

bool x64Encoder::cvtss2sd_rr32(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtss2sd_rm32(int32_t avxDstReg, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, X64OP2_CVTSS2SD, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}

bool x64Encoder::cvtsd2ss_rr64(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}

bool x64Encoder::cvtps2pd_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtps2pd_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtps2pd_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}


bool x64Encoder::cvtpd2ps_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtpd2ps_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtpd2ps_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_CVTSS2SD, dst, 0, src0, z, mask);
}


bool x64Encoder::cvtsi2sd ( int32_t sseDestReg, int32_t x64SrcReg )
{
	x64EncodePrefix ( 0xf2 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CVTSI2SD, X64OP2_CVTSI2SD ), sseDestReg, x64SrcReg );
}

bool x64Encoder::cvtdq2pd_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0xf3 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_CVTDQ2PD, X64OP2_CVTDQ2PD ), sseDestReg, sseSrcReg );
}


bool x64Encoder::movddup_regreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0xf2 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_MOVDDUP, X64OP2_MOVDDUP ), sseDestReg, sseSrcReg );
}

bool x64Encoder::movddup_regmem ( int32_t sseDestReg, int64_t* SrcPtr )
{
	x64EncodePrefix ( 0xf2 );
	return x64EncodeRipOffset32 ( MAKE2OPCODE( X64OP1_MOVDDUP, X64OP2_MOVDDUP ), sseDestReg, (char*) SrcPtr );
}




// *** AVX instructions ***//



bool x64Encoder::blendvps1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg )
{
	return x64EncodeRegVImm8 (VEX_128BIT, 0, PP_BLENDVPS, MMMMM_BLENDVPS, AVXOP_BLENDVPS, sseDestReg, sseSrc1Reg, sseSrc2Reg,  ( sseSrc3Reg << 4 ) );
}
bool x64Encoder::blendvps2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg)
{
	return x64EncodeRegVImm8(VEX_256BIT, 0, PP_BLENDVPS, MMMMM_BLENDVPS, AVXOP_BLENDVPS, sseDestReg, sseSrc1Reg, sseSrc2Reg, (sseSrc3Reg << 4));
}

bool x64Encoder::blendvpd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg)
{
	return x64EncodeRegVImm8(VEX_128BIT, 0, PP_BLENDVPD, MMMMM_BLENDVPD, AVXOP_BLENDVPD, sseDestReg, sseSrc1Reg, sseSrc2Reg, (sseSrc3Reg << 4));
}
bool x64Encoder::blendvpd2regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg )
{
	return x64EncodeRegVImm8 (VEX_256BIT, 0, PP_BLENDVPD, MMMMM_BLENDVPD, AVXOP_BLENDVPD, sseDestReg, sseSrc1Reg, sseSrc2Reg,  ( sseSrc3Reg << 4 ) );
}

bool x64Encoder::blendps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, PP_BLENDPS, MMMMM_BLENDPS, AVXOP_BLENDPS, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::blendpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 1, 0, PP_BLENDPD, MMMMM_BLENDPD, AVXOP_BLENDPD, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::blendvps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseSrc3Reg )
{
	return x64EncodeRegMemVImm8 ( 0, 0, PP_BLENDPS, MMMMM_BLENDPS, AVXOP_BLENDPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ( sseSrc3Reg << 4 ) );
}

bool x64Encoder::blendvpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseSrc3Reg )
{
	return x64EncodeRegMemVImm8 ( 1, 0, PP_BLENDPD, MMMMM_BLENDPD, AVXOP_BLENDPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ( sseSrc3Reg << 4 ) );
}

bool x64Encoder::blendps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 0, 0, PP_BLENDPS, MMMMM_BLENDPS, AVXOP_BLENDPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::blendpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 1, 0, PP_BLENDPD, MMMMM_BLENDPD, AVXOP_BLENDPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}


bool x64Encoder::vpmovsxbd1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x21, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::vpmovsxbd2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x21, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::vpmovsxbd1regmem(int32_t sseDestReg, int32_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x21, sseDestReg, 0, (char*)SrcPtr);
}
bool x64Encoder::vpmovsxbd2regmem(int32_t sseDestReg, int64_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x21, sseDestReg, 0, (char*)SrcPtr);
}


bool x64Encoder::vaddpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_ADDPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vaddpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_ADDPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vaddpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_ADDPD, dst, src0, src1, z, mask);
}


bool x64Encoder::vaddps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z, int32_t rnd_enable, int32_t rnd_type)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_ADDPS, dst, src0, src1, z, mask, rnd_enable, rnd_type);
}
bool x64Encoder::vaddps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_ADDPS, dst, src0, src1, z, mask);
}
bool x64Encoder::vaddps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_ADDPS, dst, src0, src1, z, mask);
}

bool x64Encoder::vaddss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z, int32_t rnd_enable, int32_t rnd_type)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x58, dst, src0, src1, z, mask, rnd_enable, rnd_type);
}

bool x64Encoder::vaddsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, 0x58, dst, src0, src1, z, mask);
}


bool x64Encoder::vaddps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 0, PP_ADDPS, MMMMM_ADDPS, AVXOP_ADDPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::vaddps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_ADDPS, MMMMM_ADDPS, AVXOP_ADDPS, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vaddpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 0, PP_ADDPD, MMMMM_ADDPD, AVXOP_ADDPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::vaddpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_ADDPD, MMMMM_ADDPD, AVXOP_ADDPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vaddss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_ADDSS, MMMMM_ADDSS, AVXOP_ADDSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vaddsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 1, PP_ADDSD, MMMMM_ADDSD, AVXOP_ADDSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vaddps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV (VEX_128BIT, 0, PP_ADDPS, MMMMM_ADDPS, AVXOP_ADDPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vaddpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV (VEX_128BIT, 1, PP_ADDPD, MMMMM_ADDPD, AVXOP_ADDPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vaddss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_ADDSS, MMMMM_ADDSS, AVXOP_ADDSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vaddsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 1, PP_ADDSD, MMMMM_ADDSD, AVXOP_ADDSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::andps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_ANDPS, MMMMM_ANDPS, AVXOP_ANDPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::andpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_ANDPD, MMMMM_ANDPD, AVXOP_ANDPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::andps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_ANDPS, MMMMM_ANDPS, AVXOP_ANDPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::andpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_ANDPD, MMMMM_ANDPD, AVXOP_ANDPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::andnps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_ANDNPS, MMMMM_ANDNPS, AVXOP_ANDNPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::andnpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_ANDNPD, MMMMM_ANDNPD, AVXOP_ANDNPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::andnps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_ANDNPS, MMMMM_ANDNPS, AVXOP_ANDNPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::andnpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_ANDNPD, MMMMM_ANDNPD, AVXOP_ANDNPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}






bool x64Encoder::cvtdq2pd ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_CVTDQ2PD, MMMMM_CVTDQ2PD, AVXOP_CVTDQ2PD, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::cvtdq2ps ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_CVTDQ2PS, MMMMM_CVTDQ2PS, AVXOP_CVTDQ2PS, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::cvtpd2dq ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_CVTPD2DQ, MMMMM_CVTPD2DQ, AVXOP_CVTPD2DQ, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::cvtps2dq ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_CVTPS2DQ, MMMMM_CVTPS2DQ, AVXOP_CVTPS2DQ, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::cvtps2pd ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_CVTPS2PD, MMMMM_CVTPS2PD, AVXOP_CVTPS2PD, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::cvtpd2ps ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_CVTPD2PS, MMMMM_CVTPD2PS, AVXOP_CVTPD2PS, sseDestReg, 0, sseSrc1Reg );
}


bool x64Encoder::cvtdq2pd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_CVTDQ2PD, MMMMM_CVTDQ2PD, AVXOP_CVTDQ2PD, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::cvtdq2ps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_CVTDQ2PS, MMMMM_CVTDQ2PS, AVXOP_CVTDQ2PS, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::cvtpd2dq ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_CVTPD2DQ, MMMMM_CVTPD2DQ, AVXOP_CVTPD2DQ, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::cvtps2dq ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_CVTPS2DQ, MMMMM_CVTPS2DQ, AVXOP_CVTPS2DQ, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::cvtps2pd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_CVTPS2PD, MMMMM_CVTPS2PD, AVXOP_CVTPS2PD, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::cvtpd2ps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_CVTPD2PS, MMMMM_CVTPD2PS, AVXOP_CVTPD2PS, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::cvtpd2ps1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_CVTSS2SD, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::cvtpd2ps2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F, X64OP2_CVTSS2SD, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::cvtps2pd1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, X64OP2_CVTSS2SD, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::cvtps2pd2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, X64OP2_CVTSS2SD, sseDestReg, 0, sseSrcReg);
}



bool x64Encoder::vdivps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_DIVPS, MMMMM_DIVPS, AVXOP_DIVPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vdivpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_DIVPD, MMMMM_DIVPD, AVXOP_DIVPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vdivss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_DIVSS, MMMMM_DIVSS, AVXOP_DIVSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vdivsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_DIVSD, MMMMM_DIVSD, AVXOP_DIVSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vdivps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_DIVPS, MMMMM_DIVPS, AVXOP_DIVPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vdivpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_DIVPD, MMMMM_DIVPD, AVXOP_DIVPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vdivss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_DIVSS, MMMMM_DIVSS, AVXOP_DIVSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vdivsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_DIVSD, MMMMM_DIVSD, AVXOP_DIVSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::maxps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MAXPS, MMMMM_MAXPS, AVXOP_MAXPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::maxpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MAXPD, MMMMM_MAXPD, AVXOP_MAXPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::maxss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MAXSS, MMMMM_MAXSS, AVXOP_MAXSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::maxsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MAXSD, MMMMM_MAXSD, AVXOP_MAXSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::maxps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MAXPS, MMMMM_MAXPS, AVXOP_MAXPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::maxpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MAXPD, MMMMM_MAXPD, AVXOP_MAXPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::maxss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MAXSS, MMMMM_MAXSS, AVXOP_MAXSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::maxsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MAXSD, MMMMM_MAXSD, AVXOP_MAXSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmaxsd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, PP_MAXSD, MMMMM_MAXSD, AVXOP_MAXSD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::vmaxpd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, PP_MAXPD, MMMMM_MAXPD, AVXOP_MAXPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::vmaxpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_MAXPD, MMMMM_MAXPD, AVXOP_MAXPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::vmaxps1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_MAXPS, MMMMM_MAXPS, AVXOP_MAXPS, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vmaxpd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, PP_MAXPD, MMMMM_MAXPD, AVXOP_MAXPD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vmaxss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_MAXSS, MMMMM_MAXSS, AVXOP_MAXSS, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vmaxsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, PP_MAXSD, MMMMM_MAXSD, AVXOP_MAXSD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}



bool x64Encoder::minps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MINPS, MMMMM_MINPS, AVXOP_MINPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::minpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MINPD, MMMMM_MINPD, AVXOP_MINPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::minss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MINSS, MMMMM_MINSS, AVXOP_MINSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::minsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MINSD, MMMMM_MINSD, AVXOP_MINSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::minps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MINPS, MMMMM_MINPS, AVXOP_MINPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::minpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MINPD, MMMMM_MINPD, AVXOP_MINPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::minss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MINSS, MMMMM_MINSS, AVXOP_MINSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::minsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MINSD, MMMMM_MINSD, AVXOP_MINSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vminsd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, PP_MINSD, MMMMM_MINSD, AVXOP_MINSD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::vminpd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, PP_MINPD, MMMMM_MINPD, AVXOP_MINPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::vminpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_MINPD, MMMMM_MINPD, AVXOP_MINPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::vminps1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_MINPS, MMMMM_MINPS, AVXOP_MINPS, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vminpd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, PP_MINPD, MMMMM_MINPD, AVXOP_MINPD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vminss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_MINSS, MMMMM_MINSS, AVXOP_MINSS, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vminsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, PP_MINSD, MMMMM_MINSD, AVXOP_MINSD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::vrangesd_rri64(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x51, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangesd_rmi64(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x51, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}

bool x64Encoder::vrangess_rri32(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x51, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangess_rmi32(int32_t dst, int32_t src0, void* pSrcPtr32, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x51, dst, src0, (char*)pSrcPtr32, Imm8, z, mask);
}

bool x64Encoder::vrangepd_rri128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangepd_rri256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangepd_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}

bool x64Encoder::vrangepd_rmi128(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}
bool x64Encoder::vrangepd_rmi256(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}
bool x64Encoder::vrangepd_rmi512(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}

bool x64Encoder::vrangepd_rbi128(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask, 1);
}
bool x64Encoder::vrangepd_rbi256(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask, 1);
}
bool x64Encoder::vrangepd_rbi512(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask, 1);
}



bool x64Encoder::vrangeps_rri128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangeps_rri256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vrangeps_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, src1, Imm8, z, mask);
}

bool x64Encoder::vrangeps_rmi128(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}
bool x64Encoder::vrangeps_rmi256(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}
bool x64Encoder::vrangeps_rmi512(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, 0x50, dst, src0, (char*)pSrcPtr64, Imm8, z, mask);
}


bool x64Encoder::vmulpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MULPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vmulpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MULPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vmulpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_MULPD, dst, src0, src1, z, mask);
}


bool x64Encoder::vmulps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MULPS, dst, src0, src1, z, mask);
}
bool x64Encoder::vmulps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MULPS, dst, src0, src1, z, mask);
}
bool x64Encoder::vmulps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_MULPS, dst, src0, src1, z, mask);
}

bool x64Encoder::vmulss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x59, dst, src0, src1, z, mask);
}

bool x64Encoder::vmulsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, PP_MULSD, MMMMM_MULSD, AVXOP_MULSD, dst, src0, src1, z, mask);
}


bool x64Encoder::vmulps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MULPS, MMMMM_MULPS, AVXOP_MULPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vmulps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_MULPS, MMMMM_MULPS, AVXOP_MULPS, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vmulpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MULPD, MMMMM_MULPD, AVXOP_MULPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}
bool x64Encoder::vmulpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_MULPD, MMMMM_MULPD, AVXOP_MULPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}

bool x64Encoder::vmulss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MULSS, MMMMM_MULSS, AVXOP_MULSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vmulsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MULSD, MMMMM_MULSD, AVXOP_MULSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}


bool x64Encoder::vmulss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, PP_MULSS, MMMMM_MULSS, AVXOP_MULSS, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}
bool x64Encoder::vmulsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, PP_MULSD, MMMMM_MULSD, AVXOP_MULSD, sseDestReg, sseSrc1Reg, (char*)pSrcPtr);
}


bool x64Encoder::vmulps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MULPS, MMMMM_MULPS, AVXOP_MULPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmulpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MULPD, MMMMM_MULPD, AVXOP_MULPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmulss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MULSS, MMMMM_MULSS, AVXOP_MULSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmulsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MULSD, MMMMM_MULSD, AVXOP_MULSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}



bool x64Encoder::vorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_ORPS, MMMMM_ORPS, AVXOP_ORPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_ORPD, MMMMM_ORPD, AVXOP_ORPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_ORPS, MMMMM_ORPS, AVXOP_ORPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_ORPD, MMMMM_ORPD, AVXOP_ORPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::vshufps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, PP_SHUFPS, MMMMM_SHUFPS, AVXOP_SHUFPS, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::vshufpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 1, 0, PP_SHUFPD, MMMMM_SHUFPD, AVXOP_SHUFPD, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::vshufps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 0, 0, PP_SHUFPS, MMMMM_SHUFPS, AVXOP_SHUFPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::vshufpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 1, 0, PP_SHUFPD, MMMMM_SHUFPD, AVXOP_SHUFPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}




bool x64Encoder::vsqrtps ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 0, PP_SQRTPS, MMMMM_SQRTPS, AVXOP_SQRTPS, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::vsqrtpd ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 1, PP_SQRTPD, MMMMM_SQRTPD, AVXOP_SQRTPD, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::vsqrtss ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_SQRTSS, MMMMM_SQRTSS, AVXOP_SQRTSS, sseDestReg, 0, sseSrc1Reg );
}

bool x64Encoder::vsqrtsd ( int32_t sseDestReg, int32_t sseSrc1Reg )
{
	return x64EncodeRegRegV ( 0, 1, PP_SQRTSD, MMMMM_SQRTSD, AVXOP_SQRTSD, sseDestReg, 0, sseSrc1Reg );
}


bool x64Encoder::vsqrtps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV (VEX_128BIT, 0, PP_SQRTPS, MMMMM_SQRTPS, AVXOP_SQRTPS, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsqrtpd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV (VEX_128BIT, 1, PP_SQRTPD, MMMMM_SQRTPD, AVXOP_SQRTPD, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsqrtss ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_SQRTSS, MMMMM_SQRTSS, AVXOP_SQRTSS, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsqrtsd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 1, PP_SQRTSD, MMMMM_SQRTSD, AVXOP_SQRTSD, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::subpsregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	//x64EncodePrefix ( 0xf2 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_SUBPS, X64OP2_SUBPS ), sseDestReg, sseSrcReg );
}

bool x64Encoder::subpdregreg ( int32_t sseDestReg, int32_t sseSrcReg )
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( X64OP1_SUBPD, X64OP2_SUBPD ), sseDestReg, sseSrcReg );
}


bool x64Encoder::vsubps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vsubpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV (VEX_128BIT, 0, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vsubss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_SUBSS, MMMMM_SUBSS, AVXOP_SUBSS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vsubsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV (0, 0, PP_SUBSD, MMMMM_SUBSD, AVXOP_SUBSD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vsubps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}
bool x64Encoder::vsubpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, sseDestReg, sseSrc1Reg, sseSrc2Reg);
}


bool x64Encoder::vsubps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsubpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV (VEX_128BIT, 0, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsubss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_SUBSS, MMMMM_SUBSS, AVXOP_SUBSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsubsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_SUBSD, MMMMM_SUBSD, AVXOP_SUBSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vsubps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z, int32_t rnd_enable, int32_t rnd_type)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, dst, src0, src1, z, mask, rnd_enable, rnd_type);
}
bool x64Encoder::vsubps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, dst, src0, src1, z, mask);
}
bool x64Encoder::vsubps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, PP_SUBPS, MMMMM_SUBPS, AVXOP_SUBPS, dst, src0, src1, z, mask);
}


bool x64Encoder::vsubpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vsubpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, dst, src0, src1, z, mask);
}
bool x64Encoder::vsubpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, PP_SUBPD, MMMMM_SUBPD, AVXOP_SUBPD, dst, src0, src1, z, mask);
}


bool x64Encoder::vsubss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z, int32_t rnd_enable, int32_t rnd_type)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, PP_SUBSS, MMMMM_SUBSS, AVXOP_SUBSS, dst, src0, src1, z, mask, rnd_enable, rnd_type);
}

bool x64Encoder::vsubsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, PP_SUBSD, MMMMM_SUBSS, AVXOP_SUBSS, dst, src0, src1, z, mask);
}










bool x64Encoder::vxorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 0, 0, PP_XORPS, MMMMM_XORPS, AVXOP_XORPS, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vxorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return x64EncodeRegRegV ( 1, 0, PP_XORPD, MMMMM_XORPD, AVXOP_XORPD, sseDestReg, sseSrc1Reg, sseSrc2Reg );
}

bool x64Encoder::vxorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_XORPS, MMMMM_XORPS, AVXOP_XORPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vxorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_XORPD, MMMMM_XORPD, AVXOP_XORPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset );
}









bool x64Encoder::cmppseq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmppsne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmppsgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmppsge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmppslt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmppsle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LE, 0, mask);
}


bool x64Encoder::cmpsseq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmpssne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmpssgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmpssge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmpsslt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmpssle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LE, 0, mask);
}




bool x64Encoder::cmpsdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmpsdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmpsdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmpsdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmpsdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmpsdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LE, 0, mask);
}

bool x64Encoder::cmpsdeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmpsdne_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmpsdgt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmpsdge_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmpsdlt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmpsdle_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIXF2, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_LE, 0, mask);
}


bool x64Encoder::cmppdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmppdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmppdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmppdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmppdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmppdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LE, 0, mask);
}


bool x64Encoder::cmppdeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmppdne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmppdgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmppdge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmppdlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmppdle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, src1, VPCMP_LE, 0, mask);
}



bool x64Encoder::cmppdeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_EQ, 0, mask);
}
bool x64Encoder::cmppdne_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_NE, 0, mask);
}
bool x64Encoder::cmppdgt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_GT, 0, mask);
}
bool x64Encoder::cmppdge_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_GE, 0, mask);
}
bool x64Encoder::cmppdlt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_LT, 0, mask);
}
bool x64Encoder::cmppdle_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask)
{
	return x64EncodeRipOffsetEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F, AVXOP_CMPPS, kdst, src0, (char*)pSrcPtr, VPCMP_LE, 0, mask);
}


bool x64Encoder::cmpps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, PP_CMPPS, MMMMM_CMPPS, AVXOP_CMPPS, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::cmppd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 1, 0, PP_CMPPD, MMMMM_CMPPD, AVXOP_CMPPD, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::cmpss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, PP_CMPSS, MMMMM_CMPSS, AVXOP_CMPSS, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}

bool x64Encoder::cmpsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 1, 0, PP_CMPSD, MMMMM_CMPSD, AVXOP_CMPSD, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}




bool x64Encoder::cmpeqps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, EQ_OQ );
}

bool x64Encoder::cmpneps(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return cmpps(sseDestReg, sseSrc1Reg, sseSrc2Reg, NEQ_OQ);
}

bool x64Encoder::cmpltps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LT_OQ );
}

bool x64Encoder::cmpgtps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GT_OQ );
}

bool x64Encoder::cmpleps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LE_OQ );
}

bool x64Encoder::cmpgeps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GE_OQ );
}

bool x64Encoder::cmpunordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, UNORD_Q );
}

bool x64Encoder::cmpordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpps ( sseDestReg, sseSrc1Reg, sseSrc2Reg, ORD_Q );
}

bool x64Encoder::cmpeqpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, EQ_OQ );
}

bool x64Encoder::cmpnepd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return cmppd(sseDestReg, sseSrc1Reg, sseSrc2Reg, NEQ_OQ);
}

bool x64Encoder::cmpltpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LT_OQ );
}

bool x64Encoder::cmpgtpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GT_OQ );
}

bool x64Encoder::cmplepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LE_OQ );
}

bool x64Encoder::cmpgepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GE_OQ );
}

bool x64Encoder::cmpunordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, UNORD_Q );
}

bool x64Encoder::cmpordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmppd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, ORD_Q );
}


bool x64Encoder::cmpeqss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, EQ_OQ );
}

bool x64Encoder::cmpness(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return cmpss(sseDestReg, sseSrc1Reg, sseSrc2Reg, NEQ_OQ);
}

bool x64Encoder::cmpltss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LT_OQ );
}

bool x64Encoder::cmpgtss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GT_OQ );
}

bool x64Encoder::cmpless ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LE_OQ );
}

bool x64Encoder::cmpgess ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GE_OQ );
}

bool x64Encoder::cmpunordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, UNORD_Q );
}

bool x64Encoder::cmpordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpss ( sseDestReg, sseSrc1Reg, sseSrc2Reg, ORD_Q );
}


bool x64Encoder::cmpeqsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, EQ_OQ );
}

bool x64Encoder::cmpnesd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return cmpsd(sseDestReg, sseSrc1Reg, sseSrc2Reg, NEQ_OQ);
}

bool x64Encoder::cmpltsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LT_OQ );
}

bool x64Encoder::cmpgtsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GT_OQ );
}

bool x64Encoder::cmplesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, LE_OQ );
}

bool x64Encoder::cmpgesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, GE_OQ );
}

bool x64Encoder::cmpunordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, UNORD_Q );
}

bool x64Encoder::cmpordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, sseSrc2Reg, ORD_Q );
}








bool x64Encoder::cmpps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 0, 0, PP_CMPPS, MMMMM_CMPPS, AVXOP_CMPPS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::cmppd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 1, 0, PP_CMPPD, MMMMM_CMPPD, AVXOP_CMPPD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::cmpss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 0, 0, PP_CMPSS, MMMMM_CMPSS, AVXOP_CMPSS, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}

bool x64Encoder::cmpsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 )
{
	return x64EncodeRegMemVImm8 ( 1, 0, PP_CMPSD, MMMMM_CMPSD, AVXOP_CMPSD, sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, Imm8 );
}



bool x64Encoder::cmpeqps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, EQ_OQ );
}

bool x64Encoder::cmpltps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LT_OQ );
}

bool x64Encoder::cmpgtps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GT_OQ );
}

bool x64Encoder::cmpleps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LE_OQ );
}

bool x64Encoder::cmpgeps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GE_OQ );
}

bool x64Encoder::cmpunordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, UNORD_Q );
}

bool x64Encoder::cmpordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpps ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ORD_Q );
}


bool x64Encoder::cmpeqpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, EQ_OQ );
}

bool x64Encoder::cmpltpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LT_OQ );
}

bool x64Encoder::cmpgtpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GT_OQ );
}

bool x64Encoder::cmplepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LE_OQ );
}

bool x64Encoder::cmpgepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GE_OQ );
}

bool x64Encoder::cmpunordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, UNORD_Q );
}

bool x64Encoder::cmpordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmppd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ORD_Q );
}


bool x64Encoder::cmpeqss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, EQ_OQ );
}

bool x64Encoder::cmpltss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LT_OQ );
}

bool x64Encoder::cmpgtss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GT_OQ );
}

bool x64Encoder::cmpless ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LE_OQ );
}

bool x64Encoder::cmpgess ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GE_OQ );
}

bool x64Encoder::cmpunordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, UNORD_Q );
}

bool x64Encoder::cmpordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpss ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ORD_Q );
}


bool x64Encoder::cmpeqsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, EQ_OQ );
}

bool x64Encoder::cmpltsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LT_OQ );
}

bool x64Encoder::cmpgtsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GT_OQ );
}

bool x64Encoder::cmplesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, LE_OQ );
}

bool x64Encoder::cmpgesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, GE_OQ );
}

bool x64Encoder::cmpunordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, UNORD_Q );
}

bool x64Encoder::cmpordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return cmpsd ( sseDestReg, sseSrc1Reg, x64BaseReg, x64IndexReg, Scale, Offset, ORD_Q );
}





// movaps
bool x64Encoder::movaps ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( 0, 0, PP_MOVAPS_FROMMEM, MMMMM_MOVAPS_FROMMEM, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movapd ( int32_t sseDestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MOVAPD_FROMMEM, MMMMM_MOVAPD_FROMMEM, AVXOP_MOVAPD_FROMMEM, sseDestReg, 0, sseSrcReg );
}

bool x64Encoder::movapstomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MOVAPS_TOMEM, MMMMM_MOVAPS_TOMEM, AVXOP_MOVAPS_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movapsfrommem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_MOVAPS_FROMMEM, MMMMM_MOVAPS_FROMMEM, AVXOP_MOVAPS_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movapdtomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MOVAPD_TOMEM, MMMMM_MOVAPD_TOMEM, AVXOP_MOVAPD_TOMEM, sseSrcReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::movapdfrommem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_MOVAPD_FROMMEM, MMMMM_MOVAPD_FROMMEM, AVXOP_MOVAPD_FROMMEM, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

// vbroadcastss
bool x64Encoder::vbroadcastss ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 0, 0, PP_VBROADCASTSS, MMMMM_VBROADCASTSS, AVXOP_VBROADCASTSS, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vbroadcastsd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset )
{
	return x64EncodeRegMemV ( 1, 0, PP_VBROADCASTSD, MMMMM_VBROADCASTSD, AVXOP_VBROADCASTSD, sseDestReg, 0, x64BaseReg, x64IndexReg, Scale, Offset );
}


bool x64Encoder::vmaskmovpstomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseMaskReg )
{
	return x64EncodeRegMemV ( 0, 0, PP_VMASKMOVPS_TOMEM, MMMMM_VMASKMOVPS_TOMEM, AVXOP_VMASKMOVPS_TOMEM, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmaskmovpdtomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseMaskReg )
{
	return x64EncodeRegMemV ( 1, 0, PP_VMASKMOVPD_TOMEM, MMMMM_VMASKMOVPD_TOMEM, AVXOP_VMASKMOVPD_TOMEM, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset );
}

bool x64Encoder::vmaskmovd1_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_128BIT, 0, PP_VMASKMOVPS_TOMEM, MMMMM_VMASKMOVPS_TOMEM, 0x8e, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset);
}
bool x64Encoder::vmaskmovd1_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x8e, sseSrc2Reg, sseSrc1Reg, (char*)pDestPtr);
}
bool x64Encoder::vmaskmovq1_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_128BIT, 1, PP_VMASKMOVPS_TOMEM, MMMMM_VMASKMOVPS_TOMEM, 0x8e, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset);
}
bool x64Encoder::vmaskmovq1_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x8e, sseSrc2Reg, sseSrc1Reg, (char*)pDestPtr);
}


bool x64Encoder::vmaskmovd2_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_256BIT, 0, PP_VMASKMOVPS_TOMEM, MMMMM_VMASKMOVPS_TOMEM, 0x8e, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset);
}
bool x64Encoder::vmaskmovd2_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x8e, sseSrc2Reg, sseSrc1Reg, (char*)pDestPtr);
}
bool x64Encoder::vmaskmovq2_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset)
{
	return x64EncodeRegMemV(VEX_256BIT, 1, PP_VMASKMOVPS_TOMEM, MMMMM_VMASKMOVPS_TOMEM, 0x8e, sseSrcReg, sseMaskReg, x64BaseReg, x64IndexReg, Scale, Offset);
}
bool x64Encoder::vmaskmovq2_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x8e, sseSrc2Reg, sseSrc1Reg, (char*)pDestPtr);
}


bool x64Encoder::extractps ( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 0, 0, PP_EXTRACTPS, MMMMM_EXTRACTPS, AVXOP_EXTRACTPS, sseSrcReg, 0, x64DestReg, Imm8 );
}

bool x64Encoder::movmskb1regreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(0, 0, PP_MOVMSKB, MMMMM_MOVMSKB, AVXOP_MOVMSKB, x64DestReg, 0, sseSrcReg);
}
bool x64Encoder::movmskb2regreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(1, 0, PP_MOVMSKB, MMMMM_MOVMSKB, AVXOP_MOVMSKB, x64DestReg, 0, sseSrcReg);
}

bool x64Encoder::movmskbregreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	x64EncodePrefix ( 0x66 );
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, AVXOP_MOVMSKB), x64DestReg, sseSrcReg);
}



bool x64Encoder::movmskps1regreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(0, 0, PP_MOVMSKPS, MMMMM_MOVMSKPS, AVXOP_MOVMSKPS, x64DestReg, 0, sseSrcReg);
}
bool x64Encoder::movmskps2regreg ( int32_t x64DestReg, int32_t sseSrcReg )
{
	return x64EncodeRegRegV ( 1, 0, PP_MOVMSKPS, MMMMM_MOVMSKPS, AVXOP_MOVMSKPS, x64DestReg, 0, sseSrcReg );
}


bool x64Encoder::movmskpsregreg ( int32_t x64DestReg, int32_t sseSrcReg )
{
	//x64EncodePrefix ( 0xf2 );
	return x64EncodeRegReg32 ( MAKE2OPCODE( 0x0f, AVXOP_MOVMSKPS ), x64DestReg, sseSrcReg );
}


bool x64Encoder::movmskpd1regreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(0, 0, PP_MOVMSKPD, MMMMM_MOVMSKPD, AVXOP_MOVMSKPD, x64DestReg, 0, sseSrcReg);
}
bool x64Encoder::movmskpd2regreg(int32_t x64DestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(1, 0, PP_MOVMSKPD, MMMMM_MOVMSKPD, AVXOP_MOVMSKPD, x64DestReg, 0, sseSrcReg);
}


bool x64Encoder::cvtdq2ps_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x5b, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtdq2ps_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x5b, dst, 0, src0, z, mask);
}
bool x64Encoder::cvtdq2ps_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x5b, dst, 0, src0, z, mask);
}

bool x64Encoder::cvtdq2ps1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x5b, sseDestReg, 0, sseSrcReg);
}

bool x64Encoder::cvtdq2ps2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIXNONE, VEX_PREFIX0F, 0x5b, sseDestReg, 0, sseSrcReg);
}


bool x64Encoder::cvtsi2ss1regreg(int32_t sseDestReg, int32_t sseSrcReg, int32_t x64SrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x2a, sseDestReg, sseSrcReg, x64SrcReg);
}

bool x64Encoder::cvtsi2ss1regmem(int32_t sseDestReg, int32_t sseSrcReg, int32_t* pSrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F, 0x2a, sseDestReg, sseSrcReg, (char*)pSrcPtr);
}



bool x64Encoder::cvtdq2psregreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegReg32(MAKE2OPCODE(0x0f, 0x5b), sseDestReg, sseSrcReg);
}


bool x64Encoder::cvtsd2ss1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIXF2, VEX_PREFIX0F, 0x5a, sseDestReg, 0, sseSrcReg);
}



bool x64Encoder::vperm2f128 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 )
{
	return x64EncodeRegVImm8 ( 1, 0, PP_VPERM2F128, MMMMM_VPERM2F128, AVXOP_VPERM2F128, sseDestReg, sseSrc1Reg, sseSrc2Reg, Imm8 );
}



bool x64Encoder::vpmovsxdq_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x25, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovsxdq_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x25, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovsxdq_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x25, dst, 0, src0, z, mask);
}


bool x64Encoder::vpmovzxdq_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovzxdq_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovzxdq_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}



bool x64Encoder::vpmovqd_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovqd_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}
bool x64Encoder::vpmovqd_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIXF3, VEX_PREFIX0F38, 0x35, dst, 0, src0, z, mask);
}

bool x64Encoder::vpbroadcastd1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::vpbroadcastd1regmem(int32_t sseDestReg, int32_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, sseDestReg, 0, (char*)SrcPtr);
}
bool x64Encoder::vpbroadcastd2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::vpbroadcastd2regmem(int32_t sseDestReg, int32_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, sseDestReg, 0, (char*)SrcPtr);
}

bool x64Encoder::vpbroadcastd_rx512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}
bool x64Encoder::vpbroadcastd_rx256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}
bool x64Encoder::vpbroadcastd_rx128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}

bool x64Encoder::vpbroadcastd_rm512(int32_t avxDstReg, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::vpbroadcastd_rm256(int32_t avxDstReg, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}
bool x64Encoder::vpbroadcastd_rm128(int32_t avxDstReg, int32_t* pSrcPtr, int32_t mask, int32_t z)
{
	return x64EncodeRipOffsetEV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x58, avxDstReg, 0, (char*)pSrcPtr, z, mask);
}

bool x64Encoder::vpbroadcastq1regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x59, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::vpbroadcastq1regmem(int32_t sseDestReg, int64_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x59, sseDestReg, 0, (char*)SrcPtr);
}
bool x64Encoder::vpbroadcastq2regreg(int32_t sseDestReg, int32_t sseSrcReg)
{
	return x64EncodeRegRegV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x59, sseDestReg, 0, sseSrcReg);
}
bool x64Encoder::vpbroadcastq2regmem(int32_t sseDestReg, int64_t* SrcPtr)
{
	return x64EncodeRipOffsetV(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F38, 0x59, sseDestReg, 0, (char*)SrcPtr);
}

bool x64Encoder::vpbroadcastq_rr512(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}
bool x64Encoder::vpbroadcastq_rr256(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}
bool x64Encoder::vpbroadcastq_rr128(int32_t dst, int32_t src0, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEV(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F38, 0x7c, dst, 0, src0, z, mask);
}



bool x64Encoder::vpternlogd_rrr512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vpternlogd_rrr256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vpternlogd_rrr128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 0, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}

bool x64Encoder::vpternlogq_rrr512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(EVEX_512BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vpternlogq_rrr256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_256BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}
bool x64Encoder::vpternlogq_rrr128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask, int32_t z)
{
	return x64EncodeRegRegEVImm8(VEX_128BIT, 1, VEX_PREFIX66, VEX_PREFIX0F3A, AVXOP_VPTERNLOGD, dst, src0, src1, Imm8, z, mask);
}



bool x64Encoder::JmpReg64 ( int32_t SrcReg )
{
	return x64EncodeReg32 ( 0xff, 0x4, SrcReg );
}

bool x64Encoder::JmpMem64 ( int64_t* SrcPtr )
{
	return x64EncodeRipOffset32 ( 0xff, 0x4, (char*) SrcPtr );
}

// these are used to make a int16_t term hop while encoding x64 instructions
bool x64Encoder::Jmp ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JMP );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_NE ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_NE, X64OP2_JMP_NE ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_E ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_E, X64OP2_JMP_E ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_L ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_L, X64OP2_JMP_L ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_LE ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_LE, X64OP2_JMP_LE ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_G ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_G, X64OP2_JMP_G ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_GE ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_GE, X64OP2_JMP_GE ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_B ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_B, X64OP2_JMP_B ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_BE ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_BE, X64OP2_JMP_BE ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_A ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_A, X64OP2_JMP_A ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_AE ( int32_t Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_AE, X64OP2_JMP_AE ) );
	result = x64EncodeImmediate32 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}


bool x64Encoder::Jmp_S(int32_t Offset, uint32_t Label)
{
	bool result;
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_S, X64OP2_JMP_S));
	result = x64EncodeImmediate32(Offset);
	BranchOffset[Label] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp_NS(int32_t Offset, uint32_t Label)
{
	bool result;
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_NS, X64OP2_JMP_NS));
	result = x64EncodeImmediate32(Offset);
	BranchOffset[Label] = x64NextOffset;
	return result;
}



// jump int16_t
bool x64Encoder::Jmp8 ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JMP8 );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}


// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_E ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_NE ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JNE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_A ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JA );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}
	
// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_AE ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JAE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_B ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JB );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
bool x64Encoder::Jmp8_BE ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JBE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}


bool x64Encoder::Jmp8_G ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JG );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp8_GE ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JGE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp8_L ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JL );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp8_LE ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JLE );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}


bool x64Encoder::Jmp8_NO ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JNO );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}


bool x64Encoder::Jmp8_CXZ ( char Offset, uint32_t Label )
{
	bool result;
	x64Encode16 ( X64OP_JCX );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp8_ECXZ ( char Offset, uint32_t Label )
{
	bool result;
	x64EncodeOpcode ( X64OP_JECX );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}

bool x64Encoder::Jmp8_RCXZ ( char Offset, uint32_t Label )
{
	bool result;
	x64Encode64 ( X64OP_JRCX );
	result = x64EncodeImmediate8 ( Offset );
	BranchOffset [ Label ] = x64NextOffset;
	return result;
}



// these are used to set the target address of jump once you find out what it is
bool x64Encoder::SetJmpTarget ( uint32_t Label )
{
	int32_t NextOffsetSave, BranchTargetOffset;
	
	if ( BranchOffset [ Label ] == -1 ) return true;
	
	NextOffsetSave = x64NextOffset;
	
//	JmpTargetOffset = x64NextOffset - JmpOffset;
	BranchTargetOffset = x64NextOffset - BranchOffset [ Label ];
/*	
	// set offset for unconditional jump
	x64NextOffset = JmpOffset - 4;
	if ( JmpOffset > 0 ) x64EncodeImmediate32 ( JmpTargetOffset );
*/
	// set offset for conditional jump
	x64NextOffset = BranchOffset [ Label ] - 4;
	x64EncodeImmediate32 ( BranchTargetOffset );
	
	// restore next offset
	x64NextOffset = NextOffsetSave;
	
	// clear stuff for jumping
//	JmpOffset = -1;
	BranchOffset [ Label ] = -1;

	return true;
}

bool x64Encoder::SetJmpTarget8 ( uint32_t Label )
{
	int32_t NextOffsetSave, BranchTargetOffset;
	
	if ( BranchOffset [ Label ] == -1 ) return true;
	
	NextOffsetSave = x64NextOffset;
	
	BranchTargetOffset = x64NextOffset - BranchOffset [ Label ];
	
	// set offset for conditional jump
	x64NextOffset = BranchOffset [ Label ] - 1;
	x64EncodeImmediate8 ( (char) ( BranchTargetOffset & 0xff ) );
	
	// restore next offset
	x64NextOffset = NextOffsetSave;
	
	// make sure the offset is not greater than signed 8-bits
	if ( BranchTargetOffset < -128 || BranchTargetOffset > 127 ) return false;
	
	BranchOffset [ Label ] = -1;

	return true;
}




// self-optimizing unconditional jump
bool x64Encoder::JMP ( void* Target )
{
	int32_t Offset;
	
	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 2 ] ) ) );
	
	if ( Offset >= -128 && Offset <= 127 )
	{
		x64EncodeOpcode ( X64OP_JMP8 );
		return x64EncodeImmediate8 ( Offset );
	}
	
	// int16_t jump not possible, so do the int32_t jump //
	
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 5 ] ) ) );
	x64EncodeOpcode ( X64OP_JMP );
	return x64EncodeImmediate32 ( Offset );
}


// self-optimizing jump on not-equal
bool x64Encoder::JMP_E(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JE);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_E, X64OP2_JMP_E));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_NE ( void* Target )
{
	int32_t Offset;
	
	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 2 ] ) ) );
	
	if ( Offset >= -128 && Offset <= 127 )
	{
		x64EncodeOpcode ( X64OP_JNE );
		return x64EncodeImmediate8 ( Offset );
	}
	
	// int16_t jump not possible, so do the int32_t jump //
	
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 6 ] ) ) );
	x64EncodeOpcode ( MAKE2OPCODE ( X64OP1_JMP_NE, X64OP2_JMP_NE ) );
	return x64EncodeImmediate32 ( Offset );
}

bool x64Encoder::JMP_LE(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JLE);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_LE, X64OP2_JMP_LE));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_G(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JG);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_G, X64OP2_JMP_G));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_GE(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JGE);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_GE, X64OP2_JMP_GE));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_L(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JL);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_L, X64OP2_JMP_L));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_B(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JB);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_B, X64OP2_JMP_B));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_AE(void* Target)
{
	int32_t Offset;

	// try int16_t jump first //

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 2])));

	if (Offset >= -128 && Offset <= 127)
	{
		x64EncodeOpcode(X64OP_JAE);
		return x64EncodeImmediate8(Offset);
	}

	// int16_t jump not possible, so do the int32_t jump //

	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 6])));
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_AE, X64OP2_JMP_AE));
	return x64EncodeImmediate32(Offset);
}

bool x64Encoder::JMP_O(void* Target)
{
	int32_t Offset;

	//x64EncodeOpcode ( X64OP_JMP );
	x64EncodeOpcode(MAKE2OPCODE(X64OP1_JMP_O, X64OP2_JMP_O));

	// add in modr/m
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64DestReg, RMBASE_USINGRIP );

	// get offset to data
	Offset = (int32_t)(((char*)Target) - (&(x64CodeArea[x64NextOffset + 4])));

	//cout << hex << "\nOffset=" << Offset << " Code Address=" << (unsigned int64_t) & ( x64CodeArea [ x64NextOffset + 4 ] ) << "\n";

	return x64EncodeImmediate32(Offset);
}


bool x64Encoder::Jmp ( void* Target )
{
	int32_t Offset;
	
	//x64EncodeRexReg32 ( x64DestReg, 0 );
	x64EncodeOpcode ( X64OP_JMP );

	// add in modr/m
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64DestReg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 4 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << " Code Address=" << (unsigned int64_t) & ( x64CodeArea [ x64NextOffset + 4 ] ) << "\n";
	
	return x64EncodeImmediate32 ( Offset );
}




bool x64Encoder::Call ( const void* Target )
{
	int32_t Offset;
	
	//x64EncodeRexReg32 ( x64DestReg, 0 );
	x64EncodeOpcode ( X64OP_CALL );

	// add in modr/m
	//x64CodeArea [ x64NextOffset++ ] = MAKE_MODRMREGMEM( REGMEM_NOOFFSET, x64DestReg, RMBASE_USINGRIP );
	
	// get offset to data
	Offset = (int32_t) ( ( (char*) Target ) - ( & ( x64CodeArea [ x64NextOffset + 4 ] ) ) );
	
	//cout << hex << "\nOffset=" << Offset << " Code Address=" << (unsigned int64_t) & ( x64CodeArea [ x64NextOffset + 4 ] ) << "\n";
	
	return x64EncodeImmediate32 ( Offset );
}


