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
#ifndef __X64ENCODER_H__
#define __X64ENCODER_H__


#include <stdint.h>

#include "x64Instructions.h"

#include "RecompilerBackend.h"


#define INVALIDCODEBLOCK		0xffffffff

// some x64 instructions have more than 1 opcode
#define MAKE2OPCODE(opcode1,opcode2)			( ( ( opcode2 ) << 8 ) | ( opcode1 ) )
#define MAKE3OPCODE(opcode1,opcode2,opcode3)	( ( ( opcode3 ) << 16 ) | ( ( opcode2 ) << 8 ) | ( opcode1 ) )
#define MAKE4OPCODE(opcode1,opcode2,opcode3,opcode4)	( ( ( opcode4 ) << 24 ) | ( ( opcode3 ) << 16 ) | ( ( opcode2 ) << 8 ) | ( opcode1 ) )
#define GETOPCODE1(multiopcode)					( ( multiopcode ) & 0xff )
#define GETOPCODE2(multiopcode)					( ( ( multiopcode ) >> 8 ) & 0xff )
#define GETOPCODE3(multiopcode)					( ( ( multiopcode ) >> 16 ) & 0xff )
#define GETOPCODE4(multiopcode)					( ( ( multiopcode ) >> 24 ) & 0xff )


// defines for vpcmp
enum { VPCMP_EQ = 0, VPCMP_LT = 1, VPCMP_LE = 2, VPCMP_FALSE = 3, VPCMP_NE = 4, VPCMP_GE = 5, VPCMP_GT = 6, VPCMP_TRUE = 7 };


// this not only encodes from a source cpu to a target cpu but also holds the code and runs it
// lNumberOfCodeBlocks must be a power of 2
// the total amount allocated to the x64 code area is lCodeBlockSize*lNumberOfCodeBlocks*2
// lCodeBlockSize is in bytes

class x64Encoder : public RecompilerBackend
{

public:
	
	// this is going to point to the current code area
	char* x64CodeArea;
	
	// this is going to be where all the live cached x64 code is at - no need for this after testing
	char* LiveCodeArea;
	
	int32_t lCodeBlockSize_PowerOfTwo, lCodeBlockSize, lCodeBlockSize_Mask;
	int32_t lNumberOfCodeBlocks;
	
	// I need the index for the current code block
	int32_t x64CurrentCodeBlockIndex;
	
	// the total amount of space allocated for code area
	int32_t x64TotalSpaceAllocated;

	// need to know the address where to start putting the next x64 instruction
	int32_t x64NextOffset;
	
	// need to know the address of the start of the current x64 instruction being encoded
	int32_t x64CurrentStartOffset;
	
	// need to know offset first x64 byte of current source cpu instruction being encoded
	int32_t x64CurrentSourceCpuInstructionStartOffset;
	
	// need to know the Source Cpu Address that is currently being worked on
	int32_t x64CurrentSourceAddress;
	
	// need to keep track of the source cpu instruction size
	int32_t x64SourceCpuInstructionSize;

	// need to know if we are currently encoding or if we're ready to execute
	bool isEncoding;
	
	// we also ned to know that it was set up successfully
	bool isReadyForUse;
	
	
	// constructor
	// CountOfCodeBlocks must be a power of 2, OR ELSE!!!!
	x64Encoder ( int32_t SizePerCodeBlock_PowerOfTwo, int32_t CountOfCodeBlocks );
	
	// destructor
	~x64Encoder ( void );
	
	// flush current code block
	bool FlushCurrentCodeBlock ( void );
	
	// flush dynamic code block - must do this once after creation before you can use it
	bool FlushCodeBlock ( int32_t IndexOfCodeBlock );
	
	// flush instruction cache line for current process - have to use this with dynamic code or else
	bool FlushICache ( int64_t baseAddress, int32_t NumberOfBytes );

	
	// get/set current instruction offset
	int32_t GetCurrentInstructionOffset ( void );
	void SetCurrentInstructionOffset ( int32_t offset );

	// get the size of a code block for encoder - should help with determining if there is more space available
	int32_t GetCodeBlockSize ( void );
	
	// call this before beginning an x64 code block
	bool StartCodeBlock ( int32_t lCodeBlockIndex );

	// call this when you are done with the code block
	bool EndCodeBlock ( void );

	// gets a pointer to the start of current code block
	char* Get_CodeBlock_StartPtr ();
	
	// gets a pointer to the end of current code block (where there SHOULD be return instruction)
	char* Get_CodeBlock_EndPtr ();

	// get the start pointer for an arbitrary code block number
	char* Get_XCodeBlock_StartPtr ( int32_t lCodeBlockIndex );
	
	// get the current pointer in code block
	char* Get_CodeBlock_CurrentPtr ();

	// update the position in the current block
	void Update_CodeBlock_CurrentPtr(uint32_t advance) { x64NextOffset += advance; }

	// call this before starting to encode a single instruction from source cpu
	bool StartInstructionBlock ( void );

	// call this when done encoding a single instructin from source cpu
	bool EndInstructionBlock ( void );

	// takes you to the start of the current source cpu instruction you were encoding - cuz you may need to backtrack
	bool UndoInstructionBlock ( void );
	
	// this returns the amount of space remaining in the current code block
	int32_t x64CurrentCodeBlockSpaceRemaining ( void );
	
	// invalidate a code block that has been overwritten in source cpu memory or is otherwise no int32_ter valid
	bool x64InvalidateCodeBlock ( int32_t lSourceAddress );

	// check if code is already encoded and ready to run
	bool x64IsEncodedAndReady ( int32_t lSourceAddress );

	// executes a code block and returns address of the next instruction
	//int64_t ExecuteCodeBlock ( int32_t lSourceAddress );
	inline int64_t ExecuteCodeBlock ( int32_t lSourceAddress )
	{
		// function will return the address of the next instruction to execute
		typedef int64_t (*asm_function) ( void );
		
		volatile asm_function x64Function;

		//int32_t lCodeBlockIndex;
		
		// get the index for the code block to put x64 code in
		//lCodeBlockIndex = ( lSourceAddress & ( lNumberOfCodeBlocks - 1 ) );
		
		//x64Function = (asm_function) ( & ( x64CodeArea [ lCodeBlockIndex * lNumberOfCodeBlocks ] ) );
		x64Function = (asm_function) ( & ( x64CodeArea [ lSourceAddress << lCodeBlockSize_PowerOfTwo ] ) );

		return x64Function ();
	}



	// **** Functions For Encoding of x64 Instructions **** //
	
	// encode an instruction with no register arguments
	bool x64Encode16 ( int32_t x64InstOpcode );
	bool x64Encode32 ( int32_t x64InstOpcode );
	bool x64Encode64 ( int32_t x64InstOpcode );
	
	// encode a move immediate instruction using fewest bytes possible
	bool x64EncodeMovImm64 ( int32_t x64DestRegIndex, int64_t Immediate64 );
	bool x64EncodeMovImm32 ( int32_t x64DestRegIndex, int32_t Immediate32 );
	bool x64EncodeMovImm16 ( int32_t x64DestRegIndex, int16_t Immediate16 );
	bool x64EncodeMovImm8 ( int32_t x64DestRegIndex, int32_t Immediate8 );

	// encode a single register x64 instruction into code block
	bool x64EncodeReg32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg );
	bool x64EncodeReg64 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg );
	
	// encode a register-register x64 instruction into code block
	bool x64EncodeRegReg16 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base );
	bool x64EncodeRegReg32 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base );
	bool x64EncodeRegReg64 ( int32_t x64InstOpcode, int32_t x64DestReg_Reg_Opcode, int32_t x64SourceReg_RM_Base );

	// encode a x64 Imm8 instruction into code block
	bool x64EncodeReg16Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 );
	bool x64EncodeReg32Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 );
	bool x64EncodeReg64Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg_RM_Base, char cImmediate8 );
	
	// encode x64 Imm8 instruction but with memory
	bool x64EncodeMem16Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char cImmediate8 );
	bool x64EncodeMem32Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char cImmediate8 );
	bool x64EncodeMem64Imm8 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char cImmediate8 );

	
	// encode 16-bit immediate x64 instruction
	bool x64EncodeReg8Imm8(int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int8_t cImmediate16);
	bool x64EncodeReg16Imm16 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int16_t cImmediate16 );

	// encode a reg-immediate 32 bit x64 instruction into code block
	bool x64EncodeReg32Imm32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int32_t cImmediate32 );

	// encode a reg-immediate 64 bit x64 instruction into code block
	bool x64EncodeReg64Imm32 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64DestReg, int32_t cImmediate32 );

	// encode for operation on accumulator
	bool x64EncodeAcc8Imm8(int32_t x64InstOpcode, int8_t cImmediate8);
	bool x64EncodeAcc16Imm16 ( int32_t x64InstOpcode, int16_t cImmediate16 );
	bool x64EncodeAcc32Imm32 ( int32_t x64InstOpcode, int32_t cImmediate32 );
	bool x64EncodeAcc64Imm32 ( int32_t x64InstOpcode, int32_t cImmediate32 );

	
	
	// *** These functions work with memory accesses ***
	// ** NOTE ** Do not use sp/esp/rsp or bp/ebp/rbp with these functions until I read the x64 docs more thoroughly!!!

	// encode an x64 instruction that addresses memory
	bool x64EncodeRegMem16 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeRegMem32 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeRegMem64 ( int32_t x64InstOpcode, int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool x64EncodeMemImm8 ( int32_t x64InstOpcode, int32_t Mod, char Imm8, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeMemImm16 ( int32_t x64InstOpcode, int32_t Mod, int16_t Imm16, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeMemImm32 ( int32_t x64InstOpcode, int32_t Mod, int32_t Imm32, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeMemImm64 ( int32_t x64InstOpcode, int32_t Mod, int32_t Imm32, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// these are for encoding just immediate values
	bool x64EncodeImmediate8 ( char Imm8 );
	bool x64EncodeImmediate16 ( int16_t Imm16 );
	bool x64EncodeImmediate32 ( int32_t Imm32 );
	bool x64EncodeImmediate64 ( int64_t Imm64 );

	// encode a return instruction
	bool x64EncodeReturn ( void );
	
	
	
	// *** testing *** encode RIP-offset addressing
	bool x64EncodeRipOffset16 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, bool bIsSourceReg = false );
	bool x64EncodeRipOffset32 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, bool bIsSourceReg = false );
	bool x64EncodeRipOffset64 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, bool bIsSourceReg = false );
	bool x64EncodeRipOffsetImm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg = false );
	bool x64EncodeRipOffsetImm16 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int16_t Imm16, bool bIsSourceReg = false );
	bool x64EncodeRipOffsetImm32 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int32_t Imm32, bool bIsSourceReg = false );
	bool x64EncodeRipOffsetImm64 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, int32_t Imm32, bool bIsSourceReg = false );
	
	bool x64EncodeRipOffset16Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg = false );
	bool x64EncodeRipOffset32Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg = false );
	bool x64EncodeRipOffset64Imm8 ( int32_t x64InstOpcode, int32_t x64Reg, char* DataAddress, char Imm8, bool bIsSourceReg = false );
	
	
	
	// ** Encoding vector/sse instructions ** //

	// encode an avx instruction (64-bit version) with an immediate byte on the end
	bool x64EncodeRegVImm8 ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, char cImmediate8 );
	bool x64EncodeRegMemVImm8 ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R_Dest, int32_t vvvv, int32_t x64RM_B_Base, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	
	// encode avx instruction for a single 32-bit load store to/from memory
	bool x64EncodeRegMem32S ( int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t avxDestSrcReg, int32_t avxBaseReg, int32_t avxIndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeRegMemV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R_Dest, int32_t vvvv, int32_t x64RM_B_Base, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool x64EncodeRegMem256 ( int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t avxDestSrcReg, int32_t avxBaseReg, int32_t avxIndexReg, int32_t Scale, int32_t Offset );

	// encode an avx-512 instruction
	bool x64EncodeRegRegEV(int32_t L, int32_t w, int32_t pp, int32_t mm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, int32_t z, int32_t k, int32_t rnd_enable = 0, int32_t rnd_type = 0);
	bool x64EncodeRegRegEVImm8(int32_t L, int32_t w, int32_t pp, int32_t mm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B, char cImmediate8, int32_t z, int32_t k);

	// encode an avx instruction (64-bit version) that is just register-register
	bool x64EncodeRegRegV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, int32_t RM_B );

	bool x64EncodeRipOffsetV ( int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress );
	bool x64EncodeRipOffsetVImm8(int32_t L, int32_t w, int32_t pp, int32_t mmmmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, char Imm8);

	// avx512
	bool x64EncodeRipOffsetEV(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, int32_t z, int32_t k, int32_t broadcast = 0);
	bool x64EncodeRipOffsetEVImm8(int32_t L, int32_t w, int32_t pp, int32_t mmm, int32_t avxInstOpcode, int32_t REG_R, int32_t vvvv, char* DataAddress, char Imm8, int32_t z, int32_t k, int32_t broadcast = 0);

	// **** x64 Instuctions **** //
	
	// ** General Purpose Register Instructions ** //
	
	
	// * jump/branch instructions * //
	
	static const int c_iNumOfBranchOffsets = 32768;
	static const int c_iNumOfBranchOffsets_Mask = c_iNumOfBranchOffsets - 1;
	
//	int32_t JmpOffset [ 256 ];
	int32_t BranchOffset [ c_iNumOfBranchOffsets ];
/*
	int32_t JmpOffsetEndCount;
+++++	int32_t JmpOffset_End [ 512 ];

	int32_t BranchOffsetEndCount;
	int32_t BranchOffset_End [ 512 ];
*/

	// jump to address in 64-bit register
	bool JmpReg64 ( int32_t x64Reg );
	bool JmpMem64 ( int64_t* SrcPtr );

	// these are used to make a int16_t term hop while encoding x64 instructions
	bool Jmp ( int32_t Offset, uint32_t Label );
	bool Jmp_NE ( int32_t Offset, uint32_t Label );
	bool Jmp_E ( int32_t Offset, uint32_t Label );
	bool Jmp_L ( int32_t Offset, uint32_t Label );
	bool Jmp_LE ( int32_t Offset, uint32_t Label );
	bool Jmp_G ( int32_t Offset, uint32_t Label );
	bool Jmp_GE ( int32_t Offset, uint32_t Label );
	bool Jmp_B ( int32_t Offset, uint32_t Label );
	bool Jmp_BE ( int32_t Offset, uint32_t Label );
	bool Jmp_A ( int32_t Offset, uint32_t Label );
	bool Jmp_AE ( int32_t Offset, uint32_t Label );
	bool Jmp_S(int32_t Offset, uint32_t Label);
	bool Jmp_NS(int32_t Offset, uint32_t Label);

	
	// jump int16_t
	bool Jmp8 ( char Offset, uint32_t Label );
	
	// jump int16_t if equal
	bool Jmp8_E ( char Offset, uint32_t Label );
	
	// jump int16_t if not equal
	bool Jmp8_NE ( char Offset, uint32_t Label );
	
	// jump int16_t if unsigned above (carry flag=0 and zero flag=0)
	bool Jmp8_A ( char Offset, uint32_t Label );

	// jump int16_t if unsigned above or equal (carry flag=0)
	bool Jmp8_AE ( char Offset, uint32_t Label );
	
	// jump int16_t if unsigned below (carry flag=1)
	bool Jmp8_B ( char Offset, uint32_t Label );

	// jump int16_t if unsigned below or equal (carry flag=1 or zero flag=1)
	bool Jmp8_BE ( char Offset, uint32_t Label );

	bool Jmp8_G ( char Offset, uint32_t Label );
	bool Jmp8_GE ( char Offset, uint32_t Label );
	bool Jmp8_L ( char Offset, uint32_t Label );
	bool Jmp8_LE ( char Offset, uint32_t Label );
	
	// jump int16_t if not overflow
	bool Jmp8_NO ( char Offset, uint32_t Label );

	// jump if CX/ECX/RCX is zero
	bool Jmp8_CXZ ( char Offset, uint32_t Label );
	bool Jmp8_ECXZ ( char Offset, uint32_t Label );
	bool Jmp8_RCXZ ( char Offset, uint32_t Label );

	

	// these are used to set the target address of jump once you find out what it is
	bool SetJmpTarget ( uint32_t Label );
	
	// set jump target address for a int16_t jump
	bool SetJmpTarget8 ( uint32_t Label );
	
	// self-optimizing unconditional jump
	bool JMP ( void* Target );
	
	// self-optimizing jump on equal
	bool JMP_E ( void* Target );
	
	// self-optimizing jump on not-equal
	bool JMP_NE ( void* Target );

	bool JMP_LE(void* Target);
	bool JMP_G(void* Target);
	bool JMP_L(void* Target);
	bool JMP_GE(void* Target);
	bool JMP_B(void* Target);
	bool JMP_AE(void* Target);
	bool JMP_O(void* Target);

	
	// absolute jump with 32-bit rip-offset
	bool Jmp ( void* Target );
	
	
	// absolute call with 32-bit rip-offset
	bool Call ( const void* Target );

//	bool SetJmpTarget_End ( void );

	// mov
	bool MovRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool MovRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool MovRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool MovRegImm8 ( int32_t DestReg, char Imm8 );
	bool MovRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool MovRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool MovRegImm64 ( int32_t DestReg, int64_t Imm64 );
	bool MovReg64Imm32 ( int32_t DestReg, int32_t Imm32 );
	bool MovRegToMem8 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegFromMem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegToMem16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegFromMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegToMem32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegFromMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegToMem64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovRegFromMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	bool MovMemImm8 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// self-optimizing instructions
	bool MovReg16ImmX ( int32_t DestReg, int16_t Imm16 );
	bool MovReg32ImmX ( int32_t DestReg, int32_t Imm32 );
	bool MovReg64ImmX ( int32_t DestReg, int64_t Imm64 );

	
	// *** testing ***
	bool MovRegToMem8 ( char* Address, int32_t SrcReg );
	bool MovRegFromMem8 ( int32_t DestReg, char* Address );
	bool MovRegToMem16 ( int16_t* Address, int32_t SrcReg );
	bool MovRegFromMem16 ( int32_t DestReg, int16_t* Address );
	bool MovRegToMem32 ( int32_t* Address, int32_t SrcReg );
	bool MovRegFromMem32 ( int32_t DestReg, int32_t* Address );
	bool MovRegToMem64 ( int64_t* Address, int32_t SrcReg );
	bool MovRegFromMem64 ( int32_t DestReg, int64_t* Address );
	
	inline bool MovMemReg8 ( char* Address, int32_t SrcReg ) { return MovRegToMem8 ( Address, SrcReg ); }
	inline bool MovRegMem8 ( int32_t DestReg, char* Address ) { return MovRegFromMem8 ( DestReg, Address ); }
	inline bool MovMemReg16 ( int16_t* Address, int32_t SrcReg ) { return MovRegToMem16 ( Address, SrcReg ); }
	inline bool MovRegMem16 ( int32_t DestReg, int16_t* Address ) { return MovRegFromMem16 ( DestReg, Address ); }
	inline bool MovMemReg32 ( int32_t* Address, int32_t SrcReg ) { return MovRegToMem32 ( Address, SrcReg ); }
	inline bool MovRegMem32 ( int32_t DestReg, int32_t* Address ) { return MovRegFromMem32 ( DestReg, Address ); }
	inline bool MovMemReg64 ( int64_t* Address, int32_t SrcReg ) { return MovRegToMem64 ( Address, SrcReg ); }
	inline bool MovRegMem64 ( int32_t DestReg, int64_t* Address ) { return MovRegFromMem64 ( DestReg, Address ); }

	bool MovMemImm8 ( char* DestPtr, char Imm8 );
	bool MovMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool MovMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool MovMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// load variables from memory //
	inline bool LoadMem8 ( int32_t DstReg, void* MemoryAddress ) { return MovRegFromMem8 ( DstReg, (char*) MemoryAddress ); }
	inline bool LoadMem16 ( int32_t DstReg, void* MemoryAddress ) { return MovRegFromMem16 ( DstReg, (int16_t*) MemoryAddress ); }
	inline bool LoadMem32 ( int32_t DstReg, void* MemoryAddress ) { return MovRegFromMem32 ( DstReg, (int32_t*) MemoryAddress ); }
	inline bool LoadMem64 ( int32_t DstReg, void* MemoryAddress ) { return MovRegFromMem64 ( DstReg, (int64_t*) MemoryAddress ); }
	
	// store variables to memory //
	inline bool StoreMem8 ( void* MemoryAddress, int32_t SrcReg ) { return MovRegToMem8 ( (char*) MemoryAddress, SrcReg ); }
	inline bool StoreMem16 ( void* MemoryAddress, int32_t SrcReg ) { return MovRegToMem16 ( (int16_t*) MemoryAddress, SrcReg ); }
	inline bool StoreMem32 ( void* MemoryAddress, int32_t SrcReg ) { return MovRegToMem32 ( (int32_t*) MemoryAddress, SrcReg ); }
	inline bool StoreMem64 ( void* MemoryAddress, int32_t SrcReg ) { return MovRegToMem64 ( (int64_t*) MemoryAddress, SrcReg ); }
	
	// set register to constant //
	//inline bool LoadImm8 ( int32_t DstReg, char Imm8 ) { return MovRegImm8 ( DstReg, Imm8 ); }
	inline bool LoadImm16 ( int32_t DstReg, int16_t Imm16 ) { return MovRegImm16 ( DstReg, Imm16 ); }
	inline bool LoadImm32 ( int32_t DstReg, int32_t Imm32 ) { return MovRegImm32 ( DstReg, Imm32 ); }
	inline bool LoadImm64 ( int32_t DstReg, int64_t Imm64 ) { return MovRegImm64 ( DstReg, Imm64 ); }

	// set memory to constant //
	//inline bool StoreImm8 ( void* MemoryAddress, char Imm8 ) { return MovMemImm8 ( (char*) MemoryAddress, Imm8 ); }
	inline bool StoreImm16 ( void* MemoryAddress, int16_t Imm16 ) { return MovMemImm16 ( (int16_t*) MemoryAddress, Imm16 ); }
	inline bool StoreImm32 ( void* MemoryAddress, int32_t Imm32 ) { return MovMemImm32 ( (int32_t*) MemoryAddress, Imm32 ); }
	inline bool StoreImm64 ( void* MemoryAddress, int32_t Imm32 ) { return MovMemImm64 ( (int64_t*) MemoryAddress, Imm32 ); }
	
	
//	bool MovRegToMem32S ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
//	bool MovRegFromMem32S ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// add
	bool AddRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool AddRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool AddRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool AddRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool AddRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool AddRegImm64 ( int32_t DestReg, int32_t Imm32 );
	
	bool AddRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AddMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool AddRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool AddRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool AddRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool AddMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool AddMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool AddMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool AddMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool AddMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool AddMemImm64 ( int64_t* DestPtr, int32_t Imm32 );
	

	// 8-bit immediate
	bool AddReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool AddReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool AddReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool AddMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool AddMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool AddMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool AddAcc16Imm16 ( int16_t Imm16 );
	bool AddAcc32Imm32 ( int32_t Imm32 );
	bool AddAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool AddReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool AddReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool AddReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool AddMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool AddMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool AddMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// adc - add with carry
	
	bool AdcRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool AdcRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool AdcRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool AdcRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool AdcRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool AdcRegImm64 ( int32_t DestReg, int32_t Imm32 );
	
	bool AdcRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AdcMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool AdcRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool AdcRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool AdcRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool AdcMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool AdcMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool AdcMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool AdcMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool AdcMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool AdcMemImm64 ( int64_t* DestPtr, int32_t Imm32 );
	

	// 8-bit immediate
	bool AdcReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool AdcReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool AdcReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool AdcMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool AdcMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool AdcMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool AdcAcc16Imm16 ( int16_t Imm16 );
	bool AdcAcc32Imm32 ( int32_t Imm32 );
	bool AdcAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool AdcReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool AdcReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool AdcReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool AdcMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool AdcMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool AdcMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// and
	bool AndRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool AndRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool AndRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool AndRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool AndRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool AndRegImm64 ( int32_t DestReg, int32_t Imm32 );

	bool AndRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool AndMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	bool AndRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool AndRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool AndRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool AndMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool AndMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool AndMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool AndMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool AndMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool AndMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// 8-bit immediate
	bool AndReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool AndReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool AndReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool AndMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool AndMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool AndMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool AndAcc16Imm16 ( int16_t Imm16 );
	bool AndAcc32Imm32 ( int32_t Imm32 );
	bool AndAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool AndReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool AndReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool AndReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool AndMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool AndMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool AndMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// bsr - bit scan reverse - leading zero count
	bool BsrRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool BsrRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool BsrRegReg64 ( int32_t DestReg, int32_t SrcReg );
	
	bool BsrRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool BsrRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool BsrRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	
	
	// bt - bit test - stores specified bit of value in carry flag
	bool BtRegReg16 ( int32_t Reg, int32_t BitSelectReg );
	bool BtRegReg32 ( int32_t Reg, int32_t BitSelectReg );
	bool BtRegReg64 ( int32_t Reg, int32_t BitSelectReg );
	bool BtRegImm16 ( int32_t Reg, char Imm8 );
	bool BtRegImm32 ( int32_t Reg, char Imm8 );
	bool BtRegImm64 ( int32_t Reg, char Imm8 );
	
	// with memory
	bool BtMemImm16 ( int16_t* DestPtr, char Imm8 );
	bool BtMemImm32 ( int32_t* DestPtr, char Imm8 );
	bool BtMemImm64 ( int64_t* DestPtr, char Imm8 );
	bool BtMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// btc - bit test and compliment
	bool BtcRegReg16 ( int32_t Reg, int32_t BitSelectReg );
	bool BtcRegReg32 ( int32_t Reg, int32_t BitSelectReg );
	bool BtcRegReg64 ( int32_t Reg, int32_t BitSelectReg );
	bool BtcRegImm16 ( int32_t Reg, char Imm8 );
	bool BtcRegImm32 ( int32_t Reg, char Imm8 );
	bool BtcRegImm64 ( int32_t Reg, char Imm8 );
	
	// with memory
	bool BtcMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtcMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtcMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtcMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtcMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtcMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool BtcMemImm16(int16_t* DestPtr, char Imm8);
	bool BtcMemImm32(int32_t* DestPtr, char Imm8);
	bool BtcMemImm64(int64_t* DestPtr, char Imm8);

	// btr - bit test and reset
	bool BtrRegReg16 ( int32_t Reg, int32_t BitSelectReg );
	bool BtrRegReg32 ( int32_t Reg, int32_t BitSelectReg );
	bool BtrRegReg64 ( int32_t Reg, int32_t BitSelectReg );
	bool BtrRegImm16 ( int32_t Reg, char Imm8 );
	bool BtrRegImm32 ( int32_t Reg, char Imm8 );
	bool BtrRegImm64 ( int32_t Reg, char Imm8 );
	
	// with memory
	bool BtrMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtrMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtrMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtrMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtrMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtrMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool BtrMem32Imm ( int32_t* DestPtr, char Imm8 );
	
	// bts - bit test and set
	bool BtsRegReg16 ( int32_t Reg, int32_t BitSelectReg );
	bool BtsRegReg32 ( int32_t Reg, int32_t BitSelectReg );
	bool BtsRegReg64 ( int32_t Reg, int32_t BitSelectReg );
	bool BtsRegImm16 ( int32_t Reg, char Imm8 );
	bool BtsRegImm32 ( int32_t Reg, char Imm8 );
	bool BtsRegImm64 ( int32_t Reg, char Imm8 );

	// with memory
	bool BtsMemReg16 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtsMemReg32 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtsMemReg64 ( int32_t BitSelectReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtsMemImm16 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtsMemImm32 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool BtsMemImm64 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	bool BtsMemImm32(int32_t* DestPtr, char Imm8);

	// cbw, cwde, cdqe - sign extention for RAX - from byte to word, word to double word, or double word to quadword respectively
	bool Cbw ( void );
	bool Cwde ( void );
	bool Cdqe ( void );
	
	// cwd, cdq, cqo - sign extension RAX into RAX:RDX
	bool Cwd ( void );
	bool Cdq ( void );
	bool Cqo ( void );

	// cmov - conditional mov
	bool CmovERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovNERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovARegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovAERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovORegReg16(int32_t DestReg, int32_t SrcReg);
	bool CmovERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovNERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovARegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovAERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovORegReg32(int32_t DestReg, int32_t SrcReg);
	bool CmovERegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovNERegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovBERegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovARegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovAERegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovORegReg64(int32_t DestReg, int32_t SrcReg);

	
	// with memory
	bool CmovERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovNERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovNERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovNERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovBRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool CmovAERegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovAERegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmovAERegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool CmovERegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovNERegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovBRegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovBERegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovARegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovAERegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovERegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovNERegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovBRegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovBERegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovARegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovAERegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovERegMem64 ( int32_t DestReg, int64_t* Mem64 );
	bool CmovNERegMem64 ( int32_t DestReg, int64_t* Mem64 );
	bool CmovBRegMem64 ( int32_t DestReg, int64_t* Mem64 );
	bool CmovBERegMem64 ( int32_t DestReg, int64_t* Mem64 );
	bool CmovARegMem64 ( int32_t DestReg, int64_t* Mem64 );
	bool CmovAERegMem64 ( int32_t DestReg, int64_t* Mem64 );

	
	// signed conditional mov
	
	bool CmovLRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovLERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGERegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovLRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovLERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGERegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovLRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovLERegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool CmovGERegReg64 ( int32_t DestReg, int32_t SrcReg );

	bool CmovSRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovSRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovSRegReg64 ( int32_t DestReg, int32_t SrcReg );
	
	bool CmovSRegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovSRegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovSRegMem64 ( int32_t DestReg, int64_t* Mem64 );

	bool CmovNSRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool CmovNSRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool CmovNSRegReg64 ( int32_t DestReg, int32_t SrcReg );
	
	bool CmovNSRegMem16 ( int32_t DestReg, int16_t* Mem16 );
	bool CmovNSRegMem32 ( int32_t DestReg, int32_t* Mem32 );
	bool CmovNSRegMem64 ( int32_t DestReg, int64_t* Mem64 );
	

	// cmp - compare two values
	bool CmpRegReg16 ( int32_t SrcReg1, int32_t SrcReg2 );
	bool CmpRegReg32 ( int32_t SrcReg1, int32_t SrcReg2 );
	bool CmpRegReg64 ( int32_t SrcReg1, int32_t SrcReg2 );
	bool CmpRegImm8(int32_t SrcReg, int8_t Imm8);
	bool CmpRegImm16 ( int32_t SrcReg, int16_t Imm16 );
	bool CmpRegImm32 ( int32_t SrcReg, int32_t Imm32 );
	bool CmpRegImm64 ( int32_t SrcReg, int32_t Imm32 );

	bool CmpRegMem8 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpRegMem16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpRegMem32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpRegMem64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool CmpMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool CmpRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool CmpRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool CmpRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool CmpMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool CmpMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool CmpMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool CmpMemImm8 ( char* DestPtr, char Imm8 );
	bool CmpMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool CmpMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool CmpMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// 8-bit immediate
	bool CmpReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool CmpReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool CmpReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool CmpMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool CmpMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool CmpMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool CmpAcc8Imm8(int8_t Imm8);
	bool CmpAcc16Imm16 ( int16_t Imm16 );
	bool CmpAcc32Imm32 ( int32_t Imm32 );
	bool CmpAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool CmpReg8ImmX(int32_t DestReg, int8_t ImmX);
	bool CmpReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool CmpReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool CmpReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool CmpMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool CmpMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool CmpMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// div - unsigned divide - divides d:a by register, puts quotient in a, remainder in d
	bool DivRegReg16 ( int32_t SrcReg );
	bool DivRegReg32 ( int32_t SrcReg );
	bool DivRegReg64 ( int32_t SrcReg );

	bool DivRegMem16 ( int16_t* SrcPtr );
	bool DivRegMem32 ( int32_t* SrcPtr );
	bool DivRegMem64 ( int64_t* SrcPtr );

	bool DivRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool DivRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool DivRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// idiv - signed divide
	bool IdivRegReg16 ( int32_t SrcReg );
	bool IdivRegReg32 ( int32_t SrcReg );
	bool IdivRegReg64 ( int32_t SrcReg );

	bool IdivRegMem16 ( int16_t* SrcPtr );
	bool IdivRegMem32 ( int32_t* SrcPtr );
	bool IdivRegMem64 ( int64_t* SrcPtr );

	bool IdivRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool IdivRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool IdivRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// imul - signed multiply
	bool ImulRegReg16 ( int32_t SrcReg );
	bool ImulRegReg32 ( int32_t SrcReg );
	bool ImulRegReg64 ( int32_t SrcReg );

	bool ImulRegMem16 ( int16_t* SrcPtr );
	bool ImulRegMem32 ( int32_t* SrcPtr );
	bool ImulRegMem64 ( int64_t* SrcPtr );

	bool ImulRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ImulRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ImulRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// lea - can be used as 3 operand add instruction
	bool LeaRegRegReg16 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg );
	bool LeaRegRegReg32 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg );
	bool LeaRegRegReg64 ( int32_t DestReg, int32_t Src1Reg, int32_t Src2Reg );
	bool LeaRegRegImm16 ( int32_t DestReg, int32_t SrcReg, int32_t Imm16 );
	bool LeaRegRegImm32 ( int32_t DestReg, int32_t SrcReg, int32_t Imm32 );
	bool LeaRegRegImm64 ( int32_t DestReg, int32_t SrcReg, int32_t Imm32 );

	// loads the address into register using a 4-byte offset
	//bool LeaRegMem32 ( int32_t DestReg, void* SrcPtr );
	bool LeaRegMem64 ( int32_t DestReg, void* SrcPtr );

	
	// movsx - move with sign extension
	bool MovsxReg16Reg8 ( int32_t DestReg, int32_t SrcReg );
	bool MovsxReg32Reg8 ( int32_t DestReg, int32_t SrcReg );
	bool MovsxReg64Reg8 ( int32_t DestReg, int32_t SrcReg );

	bool MovsxReg16Mem8 ( int32_t DestReg, char* SrcPtr );
	bool MovsxReg32Mem8 ( int32_t DestReg, char* SrcPtr );
	bool MovsxReg64Mem8 ( int32_t DestReg, char* SrcPtr );

	bool MovsxReg16Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovsxReg32Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovsxReg64Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool MovsxReg32Reg16 ( int32_t DestReg, int32_t SrcReg );
	bool MovsxReg64Reg16 ( int32_t DestReg, int32_t SrcReg );
	
	bool MovsxReg32Mem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool MovsxReg64Mem16 ( int32_t DestReg, int16_t* SrcPtr );
	
	
	bool MovsxdReg64Reg32 ( int32_t DestReg, int32_t SrcReg );

	bool MovsxdReg64Mem32 ( int32_t DestReg, int32_t* SrcPtr );
	
	bool MovsxReg32Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovsxReg64Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovsxdReg64Mem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	
	// movzx - move with zero extension
	bool MovzxReg16Reg8 ( int32_t DestReg, int32_t SrcReg );
	bool MovzxReg32Reg8 ( int32_t DestReg, int32_t SrcReg );
	bool MovzxReg64Reg8 ( int32_t DestReg, int32_t SrcReg );

	bool MovzxReg16Mem8 ( int32_t DestReg, char* SrcPtr );
	bool MovzxReg32Mem8 ( int32_t DestReg, char* SrcPtr );
	bool MovzxReg64Mem8 ( int32_t DestReg, char* SrcPtr );

	bool MovzxReg16Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovzxReg32Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovzxReg64Mem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool MovzxReg32Reg16 ( int32_t DestReg, int32_t SrcReg );
	bool MovzxReg64Reg16 ( int32_t DestReg, int32_t SrcReg );

	bool MovzxReg32Mem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool MovzxReg64Mem16 ( int32_t DestReg, int16_t* SrcPtr );

	bool MovzxReg32Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MovzxReg64Mem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	
	// mul - unsigned multiply
	bool MulRegReg16 ( int32_t SrcReg );
	bool MulRegReg32 ( int32_t SrcReg );
	bool MulRegReg64 ( int32_t SrcReg );
	
	bool MulRegMem16 ( int16_t* SrcPtr );
	bool MulRegMem32 ( int32_t* SrcPtr );
	bool MulRegMem64 ( int64_t* SrcPtr );
	
	bool MulRegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MulRegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool MulRegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// neg
	bool NegReg16 ( int32_t DestReg );
	bool NegReg32 ( int32_t DestReg );
	bool NegReg64 ( int32_t DestReg );
	
	bool NegMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool NegMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool NegMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool NegMem16 ( int16_t* DestPtr );
	bool NegMem32 ( int32_t* DestPtr );
	bool NegMem64 ( int64_t* DestPtr );

	// not 
	bool NotReg16 ( int32_t DestReg );
	bool NotReg32 ( int32_t DestReg );
	bool NotReg64 ( int32_t DestReg );

	bool NotMem16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool NotMem32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool NotMem64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool NotMem16 ( int16_t* DestPtr );
	bool NotMem32 ( int32_t* DestPtr );
	bool NotMem64 ( int64_t* DestPtr );
	
	
	// inc
	bool IncReg16 ( int32_t DestReg );
	bool IncReg32 ( int32_t DestReg );
	bool IncReg64 ( int32_t DestReg );

	bool IncMem16 ( int16_t* DestPtr );
	bool IncMem32 ( int32_t* DestPtr );
	bool IncMem64 ( int64_t* DestPtr );
	
	
	// dec
	bool DecReg16 ( int32_t DestReg );
	bool DecReg32 ( int32_t DestReg );
	bool DecReg64 ( int32_t DestReg );

	bool DecMem16 ( int16_t* DestPtr );
	bool DecMem32 ( int32_t* DestPtr );
	bool DecMem64 ( int64_t* DestPtr );

	
	// or
	bool OrRegReg8(int32_t DestReg, int32_t SrcReg);
	bool OrRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool OrRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool OrRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool OrRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool OrRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool OrRegImm64 ( int32_t DestReg, int32_t Imm32 );

	bool OrRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool OrMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool OrRegMem8(int32_t DestReg, char* SrcPtr);
	bool OrRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool OrRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool OrRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool OrMemReg8 ( char* DestPtr, int SrcReg );
	bool OrMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool OrMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool OrMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool OrMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool OrMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool OrMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// 8-bit immediate
	bool OrReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool OrReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool OrReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool OrMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool OrMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool OrMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool OrAcc16Imm16 ( int16_t Imm16 );
	bool OrAcc32Imm32 ( int32_t Imm32 );
	bool OrAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool OrReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool OrReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool OrReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool OrMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool OrMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool OrMem64ImmX ( int64_t* DestPtr, int32_t ImmX );

	
	// popcnt - population count
	bool PopCnt32 ( int32_t DestReg, int32_t SrcReg );

	
	// pop - pop register from stack
	bool PopReg16 ( int32_t DestReg );
	bool PopReg32 ( int32_t DestReg );
	bool PopReg64 ( int32_t DestReg );
	
	// push - push register onto stack
	bool PushReg16 ( int32_t SrcReg );
	bool PushReg32 ( int32_t SrcReg );
	bool PushReg64 ( int32_t SrcReg );

	bool PushImm8 ( char Imm8 );
	bool PushImm16 ( int16_t Imm16 );
	bool PushImm32 ( int32_t Imm32 );

	// ret
	bool Ret ( void );
	
	// set - set byte instructions
	
	// unsigned versions
	
	// seta - set if unsigned above
	bool Set_A (int32_t DestReg );
	bool Set_A ( void* DestPtr8 );
	bool Set_A ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// setae - set if unsigned above or equal
	bool Set_AE (int32_t DestReg );
	bool Set_AE ( void* DestPtr8 );
	bool Set_AE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// setb - set if unsigned below
	bool Set_B (int32_t DestReg );
	bool Set_B ( void* DestPtr8 );
	bool Set_B ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// setbe - set if unsigned below or equal
	bool Set_BE (int32_t DestReg );
	bool Set_BE ( void* DestPtr8 );
	bool Set_BE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// sete - set if equal
	bool Set_E (int32_t DestReg );
	bool Set_E ( void* DestPtr8 );
	bool Set_E ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// setne - set if NOT equal
	bool Set_NE (int32_t DestReg );
	bool Set_NE ( void* DestPtr8 );
	bool Set_NE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// signed versions

	// setg - set if signed greater
	bool Set_G ( int DestReg );
	bool Set_G ( void* DestPtr8 );
	bool Set_G ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// setge - set if signed greater
	bool Set_GE (int32_t DestReg );
	bool Set_GE ( void* DestPtr8 );
	bool Set_GE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// setl - set if signed greater
	bool Set_L (int32_t DestReg );
	bool Set_L ( void* DestPtr8 );
	bool Set_L ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// setle - set if signed greater
	bool Set_LE (int32_t DestReg );
	bool Set_LE ( void* DestPtr8 );
	bool Set_LE ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// sets - set if signed
	bool Set_S (int32_t DestReg );
	bool Set_S ( void* DestPtr8 );
	bool Set_S ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	// setpo - set if parity odd
	bool Set_PO (int32_t DestReg );
	bool Set_PO ( void* DestPtr8 );
	bool Set_PO ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	
	// setb - set if unsigned below
	bool Setb ( int32_t DestReg );
	
	// setl - set if unsigned less than
	bool Setl ( int32_t DestReg );
	bool Setl ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );


	// rcr - rotate right through carry

	// rotate right through carry once
	bool RcrReg16(int32_t DestReg);
	bool RcrReg32(int32_t DestReg);
	bool RcrReg64(int32_t DestReg);


	// shl - shift left logical - these shift by the value in register c or by an immediate 8-bit value
	bool ShlRegReg16 ( int32_t DestReg );
	bool ShlRegReg32 ( int32_t DestReg );
	bool ShlRegReg64 ( int32_t DestReg );
	bool ShlRegImm16 ( int32_t DestReg, char Imm8 );
	bool ShlRegImm32 ( int32_t DestReg, char Imm8 );
	bool ShlRegImm64 ( int32_t DestReg, char Imm8 );
	
	bool ShlMemReg32 ( int32_t* DestPtr );
	
	// with memory
	bool ShlMemReg16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShlMemReg32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShlMemReg64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShlMemImm16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool ShlMemImm32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool ShlMemImm64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );

	// sar - shift right arithemetic - these shift by the value in register c or by an immediate 8-bit value
	bool SarRegReg16 ( int32_t DestReg );
	bool SarRegReg32 ( int32_t DestReg );
	bool SarRegReg64 ( int32_t DestReg );
	bool SarRegImm16 ( int32_t DestReg, char Imm8 );
	bool SarRegImm32 ( int32_t DestReg, char Imm8 );
	bool SarRegImm64 ( int32_t DestReg, char Imm8 );

	bool SarMemReg32 ( int32_t* DestPtr );
	
	// with memory
	bool SarMemReg16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SarMemReg32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SarMemReg64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SarMemImm16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool SarMemImm32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool SarMemImm64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );

	// shr - shift right logical - these shift by the value in register c or by an immediate 8-bit value
	bool ShrRegReg16 ( int32_t DestReg );
	bool ShrRegReg32 ( int32_t DestReg );
	bool ShrRegReg64 ( int32_t DestReg );
	bool ShrRegImm16 ( int32_t DestReg, char Imm8 );
	bool ShrRegImm32 ( int32_t DestReg, char Imm8 );
	bool ShrRegImm64 ( int32_t DestReg, char Imm8 );

	bool ShrMemReg32 ( int32_t* DestPtr );
	
	// ***todo***
	//bool ShrMemImm32 ( int32_t* DestPtr, char Imm8 );
	//bool ShrMemImm32 ( int32_t* DestPtr, char Imm8 );
	
	// with memory
	bool ShrMemReg16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShrMemReg32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShrMemReg64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool ShrMemImm16 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool ShrMemImm32 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool ShrMemImm64 ( int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset, char Imm8 );

	// sub
	bool SubRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool SubRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool SubRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool SubRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool SubRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool SubRegImm64 ( int32_t DestReg, int32_t Imm32 );

	bool SubRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool SubMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool SubRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool SubRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool SubRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool SubMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool SubMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool SubMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool SubMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool SubMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool SubMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// 8-bit immediate
	bool SubReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool SubReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool SubReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool SubMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool SubMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool SubMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool SubAcc16Imm16 ( int16_t Imm16 );
	bool SubAcc32Imm32 ( int32_t Imm32 );
	bool SubAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool SubReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool SubReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool SubReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool SubMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool SubMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool SubMem64ImmX ( int64_t* DestPtr, int32_t ImmX );

	// sbb - subtract with borrow
	bool SbbRegReg16(int32_t DestReg, int32_t SrcReg);
	bool SbbRegReg32(int32_t DestReg, int32_t SrcReg);
	bool SbbRegReg64(int32_t DestReg, int32_t SrcReg);

	bool SbbReg16Imm8(int32_t DestReg, char Imm8);
	bool SbbReg32Imm8(int32_t DestReg, char Imm8);
	bool SbbReg64Imm8(int32_t DestReg, char Imm8);

	
	// test - perform and without writing result and just setting the flags

	bool TestRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool TestRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool TestRegReg64 ( int32_t DestReg, int32_t SrcReg );
	
	bool TestRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool TestRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool TestRegImm64 ( int32_t DestReg, int32_t Imm32 );
	
	bool TestMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool TestMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool TestMemReg64 ( int64_t* DestPtr, int32_t SrcReg );

	bool TestMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool TestMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool TestMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	bool TestRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool TestRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool TestRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	
	// 8-bit immediate
	bool TestReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool TestReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool TestReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool TestMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool TestMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool TestMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// test with accumulator
	bool TestAcc16Imm16 ( int16_t Imm16 );
	bool TestAcc32Imm32 ( int32_t Imm32 );
	bool TestAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool TestReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool TestReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool TestReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool TestMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool TestMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool TestMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// xchg - exchange register with register or memory
	bool XchgRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool XchgRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool XchgRegReg64 ( int32_t DestReg, int32_t SrcReg );

	bool XchgRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XchgRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XchgRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	// xor
	bool XorRegReg16 ( int32_t DestReg, int32_t SrcReg );
	bool XorRegReg32 ( int32_t DestReg, int32_t SrcReg );
	bool XorRegReg64 ( int32_t DestReg, int32_t SrcReg );
	bool XorRegImm16 ( int32_t DestReg, int16_t Imm16 );
	bool XorRegImm32 ( int32_t DestReg, int32_t Imm32 );
	bool XorRegImm64 ( int32_t DestReg, int32_t Imm32 );

	bool XorRegMem8 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorRegMem16 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorRegMem32 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorRegMem64 ( int32_t DestReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemReg16 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemReg32 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemReg64 ( int32_t SrcReg, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemImm8 ( char Imm8, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemImm16 ( int16_t Imm16, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemImm32 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	bool XorMemImm64 ( int32_t Imm32, int32_t AddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );

	bool XorRegMem16 ( int32_t DestReg, int16_t* SrcPtr );
	bool XorRegMem32 ( int32_t DestReg, int32_t* SrcPtr );
	bool XorRegMem64 ( int32_t DestReg, int64_t* SrcPtr );
	bool XorMemReg16 ( int16_t* DestPtr, int32_t SrcReg );
	bool XorMemReg32 ( int32_t* DestPtr, int32_t SrcReg );
	bool XorMemReg64 ( int64_t* DestPtr, int32_t SrcReg );
	bool XorMemImm16 ( int16_t* DestPtr, int16_t Imm16 );
	bool XorMemImm32 ( int32_t* DestPtr, int32_t Imm32 );
	bool XorMemImm64 ( int64_t* DestPtr, int32_t Imm32 );

	// 8-bit immediate
	bool XorReg16Imm8 ( int32_t DestReg, char Imm8 );
	bool XorReg32Imm8 ( int32_t DestReg, char Imm8 );
	bool XorReg64Imm8 ( int32_t DestReg, char Imm8 );

	bool XorMem16Imm8 ( int16_t* DestPtr, char Imm8 );
	bool XorMem32Imm8 ( int32_t* DestPtr, char Imm8 );
	bool XorMem64Imm8 ( int64_t* DestPtr, char Imm8 );
	
	// add with accumulator
	bool XorAcc16Imm16 ( int16_t Imm16 );
	bool XorAcc32Imm32 ( int32_t Imm32 );
	bool XorAcc64Imm32 ( int32_t Imm32 );
	
	// self-optimizing versions
	bool XorReg16ImmX ( int32_t DestReg, int16_t ImmX );
	bool XorReg32ImmX ( int32_t DestReg, int32_t ImmX );
	bool XorReg64ImmX ( int32_t DestReg, int32_t ImmX );
	
	// self-optimizing
	bool XorMem16ImmX ( int16_t* DestPtr, int16_t ImmX );
	bool XorMem32ImmX ( int32_t* DestPtr, int32_t ImmX );
	bool XorMem64ImmX ( int64_t* DestPtr, int32_t ImmX );
	
	
	// ** SSE/AVX Register Instructions ** //


	// mxcsr
	bool stmxcsr(int32_t* pDstPtr);
	bool vstmxcsr(int32_t* pDstPtr);

	bool ldmxcsr(int32_t* pDstPtr);
	bool vldmxcsr(int32_t* pDstPtr);

	// movhlps - combine high quadwords of xmm registers, 3rd operand on bottom

	// avx
	bool movhlps2 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	// movlhps - combine low quadwords of xmm registers, 3rd operand on top

	// avx
	bool movlhps2 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	// pinsr

	// avx
	bool pinsrb1regreg ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 );
	bool pinsrw1regreg( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 );
	bool pinsrd1regreg( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 );
	bool pinsrq1regreg( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, char Imm8 );

	bool pinsrb1regmem(int32_t sseDestReg, int32_t sseSrcReg1, char* pSrcPtr, char Imm8);
	bool pinsrw1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int16_t* pSrcPtr, char Imm8);
	bool pinsrd1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int32_t* pSrcPtr, char Imm8);
	bool pinsrq1regmem(int32_t sseDestReg, int32_t sseSrcReg1, int64_t* pSrcPtr, char Imm8);

	bool vinserti128regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8);
	bool vinserti128regmem(int32_t sseDestReg, int32_t sseSrcReg1, void* SrcPtr, char Imm8);

	// avx512
	bool vinserti32x4_rri256(int32_t avxDestReg, int32_t avxSrcReg1, int32_t sseSrcReg2, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vinserti32x4_rri512(int32_t avxDestReg, int32_t avxSrcReg1, int32_t sseSrcReg2, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vinserti32x8_rri512(int32_t avxDestReg, int32_t avxSrcReg1, int32_t sseSrcReg2, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vinserti32x4_rmi256(int32_t dst, int32_t src0, void* SrcPtr, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vinserti32x4_rmi512(int32_t dst, int32_t src0, void* SrcPtr, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vinsertb_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8);
	bool vinsertd_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8);
	bool vinsertq_rri128(int32_t dst, int32_t src0, int32_t x64reg, char Imm8);

	bool vinsertb_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8);
	bool vinsertd_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8);
	bool vinsertq_rmi128(int32_t dst, int32_t src0, void* SrcPtr, char Imm8);

	// pextr

	// avx
	bool pextrb1regreg ( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 );
	bool pextrw1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 );
	bool pextrd1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 );
	bool pextrq1regreg( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 );

	bool pextrd1memreg(int32_t* pDstPtr, int32_t sseSrcReg, char Imm8);
	bool pextrq1memreg(int64_t* pDstPtr, int32_t sseSrcReg, char Imm8);

	bool vextracti128regreg(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	bool vextracti128memreg(void* DstPtr, int32_t sseSrcReg, char Imm8);

	// avx512
	bool vextracti32x4_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vextracti32x4_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vextracti64x4_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vextracti32x4_mri256(void* DstPtr, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vextracti32x4_mri512(void* DstPtr, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vextractb_rri128(int32_t x64reg, int32_t src0, char Imm8);
	bool vextractd_rri128(int32_t x64reg, int32_t src0, char Imm8);
	bool vextractq_rri128(int32_t x64reg, int32_t src0, char Imm8);

	bool vextractb_mri128(void* DstPtr, int32_t src0, char Imm8);
	bool vextractd_mri128(void* DstPtr, int32_t src0, char Imm8);
	bool vextractq_mri128(void* DstPtr, int32_t src0, char Imm8);

	// blendvps
	//bool blendvps ( int32_t sseDestReg, int32_t sseSrcReg1, int32_t x64SrcReg2, int32_t x64SrcReg3 );

	// vpblendmd - blend 32-bit double-words using mask

	// avx512
	
	bool vpblendmd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vpblendmd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vpblendmd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// vpblendmq - blend 64-bit quad-words using mask

	// avx512

	bool vpblendmq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vpblendmq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vpblendmq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);



	// pblendw

	// avx
	bool pblendw1regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8);
	bool pblendw2regregimm (int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8);
	
	bool pblendw1regmemimm(int32_t sseDestReg, int32_t sseSrcReg1, void* SrcPtr, char Imm8);

	// sse
	bool pblendwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pblendwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 );
	
	// pblendvb

	// avx
	bool pblendvb1regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, int32_t sseSrcReg3);
	bool pblendvb2regreg(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, int32_t sseSrcReg3);
	//bool pblendvb1regmem(int32_t sseDestReg, void* SrcPtr);


	// sse
	bool pblendvbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pblendvbregmem ( int32_t sseDestReg, void* SrcPtr );


	// knot
	bool knotb(int32_t dst, int32_t src0);
	bool knotw(int32_t dst, int32_t src0);
	bool knotd(int32_t dst, int32_t src0);
	bool knotq(int32_t dst, int32_t src0);

	// kxnor
	bool kxnorb(int32_t dst, int32_t src0, int32_t src1);
	bool kxnorw(int32_t dst, int32_t src0, int32_t src1);
	bool kxnord(int32_t dst, int32_t src0, int32_t src1);
	bool kxnorq(int32_t dst, int32_t src0, int32_t src1);

	// kxor
	bool korb(int32_t dst, int32_t src0, int32_t src1);
	bool korw(int32_t dst, int32_t src0, int32_t src1);
	bool kord(int32_t dst, int32_t src0, int32_t src1);
	bool korq(int32_t dst, int32_t src0, int32_t src1);

	// kand
	bool kandb(int32_t dst, int32_t src0, int32_t src1);
	bool kandw(int32_t dst, int32_t src0, int32_t src1);
	bool kandd(int32_t dst, int32_t src0, int32_t src1);
	bool kandq(int32_t dst, int32_t src0, int32_t src1);

	// kandn
	bool kandnb(int32_t dst, int32_t src0, int32_t src1);
	bool kandnw(int32_t dst, int32_t src0, int32_t src1);
	bool kandnd(int32_t dst, int32_t src0, int32_t src1);
	bool kandnq(int32_t dst, int32_t src0, int32_t src1);

	// kmov
	bool kmovbmskreg(int32_t dstmsk, int32_t srcreg0);
	bool kmovbregmsk(int32_t dstreg, int32_t srcmsk0);
	bool kmovbmskmem(int32_t dstmsk, char* srcptr0);

	bool kmovwmskreg(int32_t dstmsk, int32_t srcreg0);
	bool kmovwregmsk(int32_t dstreg, int32_t srcmsk0);
	bool kmovwmskmem(int32_t dstmsk, int16_t* srcptr0);

	bool kmovdmskreg(int32_t dstmsk, int32_t srcreg0);
	bool kmovdregmsk(int32_t dstreg, int32_t srcmsk0);
	bool kmovdmskmem(int32_t dstmsk, int32_t* srcptr0);
	bool kmovdmemmsk(int32_t* dstptr0, int32_t srcmsk0);

	bool kmovqmskreg(int32_t dstmsk, int32_t srcreg0);
	bool kmovqregmsk(int32_t dstreg, int32_t srcmsk0);
	bool kmovqmskmem(int32_t dstmsk, int64_t* srcptr0);


	// kshiftr - shift right k register
	bool kshiftrb(int32_t dst, int32_t src0, char Imm8);
	bool kshiftrw(int32_t dst, int32_t src0, char Imm8);
	bool kshiftrd(int32_t dst, int32_t src0, char Imm8);
	bool kshiftrq(int32_t dst, int32_t src0, char Imm8);

	// kshiftl - shift right k register
	bool kshiftlb(int32_t dst, int32_t src0, char Imm8);
	bool kshiftlw(int32_t dst, int32_t src0, char Imm8);
	bool kshiftld(int32_t dst, int32_t src0, char Imm8);
	bool kshiftlq(int32_t dst, int32_t src0, char Imm8);

	// kunpack - src1 goes on top, src2 on the bottom
	bool kunpackbw(int32_t dst, int32_t src0, int32_t src1);
	bool kunpackwd(int32_t dst, int32_t src0, int32_t src1);
	bool kunpackdq(int32_t dst, int32_t src0, int32_t src1);

	// movaps
	bool movaps128 ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movaps_to_mem128 ( void* DestPtr, int32_t sseSrcReg );
	bool movaps_from_mem128 ( int32_t sseDestReg, void* SrcPtr );
	bool movaps_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movaps_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	

	// movdqa

	// avx 128-bit
	bool movdqa1_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movdqa1_regmem ( int32_t sseDestReg, void* SrcPtr );
	bool movdqa1_memreg ( void* DestPtr, int32_t sseSrcReg );
	bool movdqa1_memreg ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movdqa1_regmem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	bool movdqu1_memreg(void* DestPtr, int32_t sseSrcReg);
	bool movdqu1_memreg(int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	bool movdqu1_regmem(int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);

	// avx 256-bit
	bool movdqa2_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movdqa2_regmem ( int32_t sseDestReg, void* SrcPtr );
	bool movdqa2_memreg ( void* DestPtr, int32_t sseSrcReg );
	bool movdqa2_memreg ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movdqa2_regmem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// movsd - move single 32-bit value from memory or to memory
	bool movsd1_regmem(int32_t sseDstReg, int64_t* pSrcPtr);
	bool movsd1_memreg(int64_t* pDstPtr, int32_t sseSrcReg);


	//bool movdqa_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	//bool movdqa_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	//bool movdqa256 ( int32_t sseDestReg, int32_t sseSrcReg );
	//bool movdqa_to_mem256 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	//bool movdqa_from_mem256 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// sse
	bool movdqa_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movdqa_memreg ( void* DestPtr, int32_t sseSrcReg );
	bool movdqa_regmem ( int32_t sseDestReg, void* SrcPtr );
	bool movdqa_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movdqa_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	bool movdqu_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movdqu_memreg ( void* DestPtr, int32_t sseSrcReg );
	bool movdqu_regmem ( int32_t sseDestReg, void* SrcPtr );
	bool movdqu_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movdqu_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// move 32-bit value into lower part of xmm register (zero upper bits)
	//bool movd_regmem ( int32_t sseDestReg, int32_t* SrcPtr );

	// move 64-bit value into lower part of xmm register (zero upper bits)
	//bool movq_regmem ( int32_t sseDestReg, int64_t* SrcPtr );
	
	// movss - move single 32-bit value from memory or to memory
	bool movss_to_mem128 ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movss_from_mem128 ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );


	// avx512
	bool movdqa32_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	bool movdqa64_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool movdqa64_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool movdqa64_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	bool movdqa32_rm512(int32_t dst, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_rm256(int32_t dst, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_rm128(int32_t dst, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool movdqa32_mr512(void* pDstPtr, int32_t dst, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_mr256(void* pDstPtr, int32_t dst, int32_t mask = 0, int32_t z = 0);
	bool movdqa32_mr128(void* pDstPtr, int32_t dst, int32_t mask = 0, int32_t z = 0);

	// pabs

	// avx512
	bool pabsd_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool pabsd_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool pabsd_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	// avx 128-bit
	bool pabsb1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pabsw1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pabsd1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	
	bool pabsb1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pabsw1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pabsd1regmem ( int32_t sseDestReg, void* SrcPtr );

	// avx 256-bit
	bool pabsb2regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool pabsw2regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool pabsd2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool pabsb2regmem(int32_t sseDestReg, void* SrcPtr);
	bool pabsw2regmem(int32_t sseDestReg, void* SrcPtr);
	bool pabsd2regmem(int32_t sseDestReg, void* SrcPtr);

	// sse
	bool pabswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pabswregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pabsdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pabsdregmem ( int32_t sseDestReg, void* SrcPtr );

	// packs

	// avx512

	// rrr - register-register-register
	bool packsdw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packsdw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packsdw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool packswb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packswb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packswb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);


	// sse
	bool packsdwregreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool packswbregreg(int32_t sseDestReg, int32_t sseSrcReg);

	// avx128
	bool packsdw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool packswb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// avx2 - 256
	bool packsdw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool packswb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// packus

	// avx512

	// rrr - register-register-register
	bool packusdw_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packusdw_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packusdw_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool packuswb_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packuswb_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool packuswb_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx128
	bool packusdw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool packuswb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool packsswb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrcReg);
	bool packsswb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr);
	bool packssdw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrcReg);
	bool packssdw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr);

	// avx2 - 256
	bool packusdw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool packuswb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool packuswbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool packuswbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool packusdwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool packusdwregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool packsswbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool packsswbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool packssdwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool packssdwregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pmovzx

	// avx
	bool pmovzxbw1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbw1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbd1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbd1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbq1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbq1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwd1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwd1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwq1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwq1regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxdq1regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxdq1regmem ( int32_t sseDestReg, void* SrcPtr );

	bool pmovzxbw2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbw2regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbd2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbd2regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbq2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbq2regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwd2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwd2regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwq2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwq2regmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxdq2regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxdq2regmem ( int32_t sseDestReg, void* SrcPtr );
	
	
	
	// sse
	bool pmovzxbwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbdregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxbqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxbqregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwdregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxwqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxwqregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmovzxdqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovzxdqregmem ( int32_t sseDestReg, void* SrcPtr );
	

	// padd

	// avx512

	// rrm - register-register-memory
	bool paddd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool paddq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	// rrb - register-register-broadcast memory
	bool paddd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool paddq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	// rrr - register-register-register
	bool paddd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool paddd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool paddq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool paddq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);


	// avx 128-bit
	bool paddb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool paddw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool paddd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool paddq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool paddb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool paddw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool paddd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool paddq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// avx 256-bit
	bool paddb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool paddw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool paddd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool paddq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool paddbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool paddwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool padddregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool paddqregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool paddbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool paddwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool padddregmem ( int32_t sseDestReg, void* SrcPtr );
	bool paddqregmem ( int32_t sseDestReg, void* SrcPtr );

	// paddus

	// avx 128-bit
	bool paddusb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool paddusw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool paddusb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool paddusw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// sse
	bool paddusbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool paddusbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool padduswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool padduswregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// padds - add packed signed values with saturation

	// avx 128-bit
	bool paddsb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool paddsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool paddsb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool paddsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// sse
	bool paddsbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool paddsbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool paddswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool paddswregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pand

	// avx512

	// rrr - register-register-register
	bool pand_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pand_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pand_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// rrm - register-register-memory
	bool pandd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool pandq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	// rrb - register-register-broadcast memory
	bool pandd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool pandq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pandq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);


	// avx 128-bit
	bool pand1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool pand1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// avx 256-bit
	bool pand2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pandregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pandregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pandn

	// avx512

	// rrr - register-register-register
	bool pandn_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pandn_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pandn_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx 128-bit
	bool pandn1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	// avx 256-bit
	bool pandn2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pandnregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pandnregmem ( int32_t sseDestReg, void* SrcPtr );

	// vpcmpq - signed compare packed quad-word (64-bit)

	// avx512

	bool vpcmpqeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	bool vpcmpqeq_rrb128(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0);
	bool vpcmpqeq_rrb256(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0);
	bool vpcmpqeq_rrb512(int32_t kdst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0);

	bool vpcmpqeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vpcmpqeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vpcmpqeq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqne_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqlt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqle_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqgt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpqge_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);


	// vpcmpd - signed compare packed double-word

	// avx512

	// rrr - register-register-register
	bool vpcmpdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vpcmpdeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vpcmpdeq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdne_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdlt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdle_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdgt_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpdge_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	// vpcmpud - unsigned compare packed double-word

	// avx512

	// rrr - register-register-register
	bool vpcmpudeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpudne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpudlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpudle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpudgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpudge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	// vpcmpub - unsigned compare packed double-word

	// avx512

	// rrr - register-register-register
	bool vpcmpubeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpubne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpublt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpuble_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpubgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vpcmpubge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	// pcmpeq - compare packed integers - sets destination to all 1s if equal, sets to all 0s otherwise

	// avx
	bool pcmpeqb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpeqw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpeqd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpeqq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool pcmpeqb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pcmpeqw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pcmpeqd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pcmpeqb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpeqw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpeqd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpeqq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pcmpeqbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pcmpeqwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pcmpeqdregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool pcmpeqbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pcmpeqwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pcmpeqdregmem ( int32_t sseDestReg, void* SrcPtr );

	// pcmpgt - compare packed signed integers for greater than


	// avx
	bool pcmpgtb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpgtw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpgtd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pcmpgtq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool pcmpgtb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pcmpgtw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pcmpgtd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pcmpgtb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpgtw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpgtd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pcmpgtq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pcmpgtbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pcmpgtwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pcmpgtdregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool pcmpgtbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pcmpgtwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pcmpgtdregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pmaxu - get maximum of unsigned integers

	// avx512

	// rrr - register-register-register
	bool pmaxud_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pmaxud_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pmaxud_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pmaxuw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmaxud1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool pmaxuw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmaxud2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pmaxuwregreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool pmaxudregreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool pmaxuwregmem(int32_t sseDestReg, void* SrcPtr);
	bool pmaxudregmem(int32_t sseDestReg, void* SrcPtr);

	// pmaxs - get maximum of signed integers

	// avx512

	// rrr - register-register-register
	bool pmaxsd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pmaxsd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pmaxsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pmaxsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pmaxsd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool pmaxsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pmaxsd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pmaxsw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmaxsd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pmaxswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmaxsdregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool pmaxswregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pmaxsdregmem ( int32_t sseDestReg, void* SrcPtr );

	// pminu - get minimum of unsigned integers

	// avx512

	// rrr - register-register-register
	bool pminud_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pminud_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pminud_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pminuw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pminud1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool pminuw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pminud1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pminuw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pminud2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pminuwregreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool pminudregreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool pminuwregmem(int32_t sseDestReg, void* SrcPtr);
	bool pminudregmem(int32_t sseDestReg, void* SrcPtr);

	// pmins - get minimum of signed integers

	// avx512

	// rrr - register-register-register
	bool pminsd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pminsd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pminsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pminsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pminsd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool pminsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool pminsd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pminsw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pminsd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pminswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pminsdregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool pminswregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pminsdregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pmovsxdq - move and sign extend lower 2 32-bit packed integers into 2 64-bit packed integers

	// avx
	bool pmovsxdq1regreg ( int32_t sseDestReg, int32_t sseSrcReg );

	// this extends lower 4 32-bit packed integers in 128-bit register into 4 64-bit packed integers in 256-bit register
	bool pmovsxdq2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	// sse
	bool pmovsxdqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmovsxdqregmem ( int32_t sseDestReg, void* SrcPtr );
	

	// por - packed logical or of integers

	// avx512

	// rrr - register-register-register
	bool por_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool por_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool por_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool pord_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pord_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pord_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool pord_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pord_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pord_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool porq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool porq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool porq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool porq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool porq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool porq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool porq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool porq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool porq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);


	// avx
	bool por1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool por1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool por2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool porregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool porregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pshuf - shuffle packed values

	// avx512

	// rr - register-register-immediate
	bool pshufd_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pshufd_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pshufd_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool pshufb_rrm512(int32_t dst, int32_t src0, void* SrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pshufb_rrm256(int32_t dst, int32_t src0, void* SrcPtr, int32_t mask = 0, int32_t z = 0);
	bool pshufb_rrm128(int32_t dst, int32_t src0, void* SrcPtr, int32_t mask = 0, int32_t z = 0);

	// rm - register-memory-immediate
	bool pshufd_rmi512(int32_t dst, void* pSrcPtr, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pshufd_rmi256(int32_t dst, void* pSrcPtr, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pshufd_rmi128(int32_t dst, void* pSrcPtr, char Imm8, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pshufb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pshufb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr);

	bool pshufb2regmem(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);

	bool pshufd1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pshufd2regregimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	bool pshufd1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8);
	bool pshufd2regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8);

	bool pshufps1regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8);
	bool pshufps2regregimm(int32_t sseDestReg, int32_t sseSrcReg1, int32_t sseSrcReg2, char Imm8);

	// sse
	bool pshufbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pshufbregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool pshufdregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	
	// ***todo*** this instruction crashes for some reason at the moment ***needs fixing***
	bool pshufdregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 );
	
	// pshufhw - packed shuffle high words

	// avx
	bool pshufhw1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pshufhw1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8);

	// sse
	bool pshufhwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	
	// ***todo*** this instruction crashes for some reason at the moment ***needs fixing***
	bool pshufhwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 );
	
	// pshuflw - packed shuffle low words

	// avx
	bool pshuflw1regregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pshuflw1regmemimm(int32_t sseDestReg, void* SrcPtr, char Imm8);

	// sse
	bool pshuflwregregimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pshuflwregmemimm ( int32_t sseDestReg, void* SrcPtr, char Imm8 );
	
	// psll - packed shift logical left integers

	// avx512

	// rri - register-register-immediate
	bool pslld_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pslld_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool pslld_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool psllq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psllq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psllq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);


	// avx
	bool pslldq1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool psllw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psllw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool pslld1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pslld1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool psllq1regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	bool pslldq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psllw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psllw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool pslld2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pslld2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	bool psllq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	// sse
	bool psllwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psllwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psllwregimm ( int32_t sseDestReg, char Imm8 );

	bool pslldregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pslldregmem ( int32_t sseDestReg, void* SrcPtr );
	bool pslldregimm ( int32_t sseDestReg, char Imm8 );
	
	bool psllqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psllqregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psllqregimm ( int32_t sseDestReg, char Imm8 );

	// psra - packed shift arithemetic right integers

	// avx512

	// rri - register-register-immediate
	bool psrad_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrad_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrad_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool psraq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psraq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psraq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	// avx2
	bool psraw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psraw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool psrad1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psrad1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );

	bool psraw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psraw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psrad2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psrad2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	// sse
	bool psrawregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psrawregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psrawregimm ( int32_t sseDestReg, char Imm8 );

	bool psradregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psradregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psradregimm ( int32_t sseDestReg, char Imm8 );
	
	// psrl - packed shift logical right integers

	// avx512

	// rri - register-register-immediate
	bool psrld_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrld_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrld_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool psrlq_rri512(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrlq_rri256(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool psrlq_rri128(int32_t dst, int32_t src0, char Imm8, int32_t mask = 0, int32_t z = 0);

	// avx
	bool psrldq1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool psrlq1regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psrlw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psrlw1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );
	bool psrld1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psrld1regimm ( int32_t sseDestReg, int32_t sseSrcReg, char Imm8 );

	bool psrldq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psrlq2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psrlw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psrlw2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);
	bool psrld2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psrld2regimm(int32_t sseDestReg, int32_t sseSrcReg, char Imm8);

	// sse
	bool psrlwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psrlwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psrlwregimm ( int32_t sseDestReg, char Imm8 );

	bool psrldregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psrldregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psrldregimm ( int32_t sseDestReg, char Imm8 );
	
	bool psrlqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psrlqregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psrlqregimm ( int32_t sseDestReg, char Imm8 );
	
	// psub - subtraction of packed integers

	// avx512

	// rrm - register-register-memory
	bool psubd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool psubq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0, int32_t z = 0);


	// rrb - register-register-broadcast memory
	bool psubd_rrb512(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrb256(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrb128(int32_t dst, int32_t src0, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool psubq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	// rrr - register-register-register
	bool psubd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool psubd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool psubq_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool psubq_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool psubb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psubw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psubd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psubq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool psubb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool psubw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool psubd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool psubb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psubw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psubd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psubq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool psubbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubqregreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool psubbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psubwregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psubdregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psubqregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// psubs - subtract packed integers with signed saturation

	// avx
	bool psubsb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psubsw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool psubsb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool psubsw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);


	// psubus - subtract packed integers with unsigned saturation

	// avx
	bool psubusb1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool psubusw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool psubusb1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool psubusw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);


	// sse
	bool psubsbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubsbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psubswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubswregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool psubusbregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubusbregmem ( int32_t sseDestReg, void* SrcPtr );
	bool psubuswregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool psubuswregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// punpckh - unpack from high - first source on bottom

	// avx
	bool punpckhbw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpckhwd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpckhdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpckhqdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool punpckhbw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpckhwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpckhdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpckhqdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// sse
	bool punpckhbwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpckhbwregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool punpckhwdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpckhwdregmem ( int32_t sseDestReg, void* SrcPtr );

	bool punpckhdqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpckhdqregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool punpckhqdqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpckhqdqregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// punpckl - unpack from low - first source on bottom


	// avx
	bool punpcklbw1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpcklwd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpckldq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool punpcklqdq1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool punpcklbw1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpcklwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpckldq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool punpcklqdq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// sse
	bool punpcklbwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpcklbwregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool punpcklwdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpcklwdregmem ( int32_t sseDestReg, void* SrcPtr );

	bool punpckldqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpckldqregmem ( int32_t sseDestReg, void* SrcPtr );
	
	bool punpcklqdqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool punpcklqdqregmem ( int32_t sseDestReg, void* SrcPtr );


	// pxor

	// avx512

	// rrr - register-register-register
	bool pxor_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pxor_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool pxor_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx
	bool pxor1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool pxor1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool pxor2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);


	// sse
	bool pxorregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pxorregmem ( int32_t sseDestReg, void* SrcPtr );

	// vpmov b/w/d/q 2m - convert vector register to mask register

	bool vpmovb2m_rr512(int32_t dst, int32_t src0);
	bool vpmovb2m_rr256(int32_t dst, int32_t src0);
	bool vpmovb2m_rr128(int32_t dst, int32_t src0);

	bool vpmovd2m_rr512(int32_t dst, int32_t src0);
	bool vpmovd2m_rr256(int32_t dst, int32_t src0);
	bool vpmovd2m_rr128(int32_t dst, int32_t src0);


	// vpmovm2b/w/d/q - convert mask register to vector register

	bool vpmovm2b_rr512(int32_t dst, int32_t src0);
	bool vpmovm2b_rr256(int32_t dst, int32_t src0);
	bool vpmovm2b_rr128(int32_t dst, int32_t src0);

	bool vpmovm2w_rr512(int32_t dst, int32_t src0);
	bool vpmovm2w_rr256(int32_t dst, int32_t src0);
	bool vpmovm2w_rr128(int32_t dst, int32_t src0);

	bool vpmovm2d_rr512(int32_t dst, int32_t src0);
	bool vpmovm2d_rr256(int32_t dst, int32_t src0);
	bool vpmovm2d_rr128(int32_t dst, int32_t src0);

	bool vpmovm2q_rr512(int32_t dst, int32_t src0);
	bool vpmovm2q_rr256(int32_t dst, int32_t src0);
	bool vpmovm2q_rr128(int32_t dst, int32_t src0);


	// vptestmb/w/d/q - do logical AND and set bit in mask if result is non-zero

	// avx512

	// rr - register-register
	bool vptestmb_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmb_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmb_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestmw_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmw_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmw_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestmd_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmd_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmd_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestmd_rrm512(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestmd_rrm256(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestmd_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	bool vptestmd_rrb512(int32_t kdst, int32_t src0, int32_t* pSrcPtr32, int32_t mask = 0);
	bool vptestmd_rrb256(int32_t kdst, int32_t src0, int32_t* pSrcPtr32, int32_t mask = 0);
	bool vptestmd_rrb128(int32_t kdst, int32_t src0, int32_t* pSrcPtr32, int32_t mask = 0);

	bool vptestmq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestmq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestmq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestmq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestmq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	bool vptestmq_rrb512(int32_t dst, int32_t src0, int64_t* pSrcPtr64, int32_t mask = 0);
	bool vptestmq_rrb256(int32_t dst, int32_t src0, int64_t* pSrcPtr64, int32_t mask = 0);
	bool vptestmq_rrb128(int32_t dst, int32_t src0, int64_t* pSrcPtr64, int32_t mask = 0);

	// vptestnmb/w/d/q - do logical AND and set bit in mask if result is non-zero

	bool vptestnmb_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmb_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmb_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestnmw_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmw_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmw_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestnmd_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmd_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmd_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestnmd_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestnmd_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestnmd_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	bool vptestnmq_rrr512(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool vptestnmq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool vptestnmq_rrm512(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestnmq_rrm256(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool vptestnmq_rrm128(int32_t dst, int32_t src0, void* pSrcPtr, int32_t mask = 0);


	// ptest

	// avx
	bool ptest1regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool ptest2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	// sse
	bool ptestregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool ptestregmem ( int32_t sseDestReg, void* SrcPtr );

	
	// pmuldq - multiplies 2 signed 32-bit values (index 0 and 2) and stores the results as two 64-bit values

	// sse
	bool pmuldqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmuldqregmem ( int32_t sseDestReg, void* SrcPtr );

	// avx
	bool pmuldq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmuldq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr);

	// pmuludq - unsigned version of pmuldq

	// avx
	bool pmuludq1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmuludq1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* SrcPtr);

	bool pmuludq2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool pmuludqregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmuludqregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// pmulld - multiplies 4 signed 32-bit values and stores low result
	bool pmulldregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmulldregmem ( int32_t sseDestReg, void* SrcPtr );
	
	
	// pmullw - multiply packed signed 16-bit halfwords and store low result

	// sse
	bool pmullwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmullwregmem ( int32_t sseDestReg, void* SrcPtr );

	// avx
	bool pmullw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// pmulhw - multiply packed signed 16-bit halfwords and store high result

	// sse
	bool pmulhwregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmulhwregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// avx
	bool pmulhw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// pmaddwd - mulitply halfwords and add adjacent results

	// sse
	bool pmaddwdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool pmaddwdregmem ( int32_t sseDestReg, void* SrcPtr );

	// avx
	bool pmaddwd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool pmaddwd1regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// psign - make result zero if arg zero, negate result if arg negative, otherwise leave result alone

	// avx
	bool psignb1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psignw1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psignd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool psignb2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psignw2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool psignd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	// sse
	bool psignbregreg (int32_t sseDestReg, int32_t sseSrcReg);
	bool psignwregreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool psigndregreg(int32_t sseDestReg, int32_t sseSrcReg);


	// vpshiftv - variable shift

	// vpsllv - variable shift left logical

	// avx
	bool vpsllvd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vpsllvd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);


	// vpsrav - variable shift right arithmetic

	// avx
	bool vpsravd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vpsravd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);


	// vpsrlv - variable shift right logical

	// avx
	bool vpsrlvd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vpsrlvd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);


	//** SSE Floating Point Instructions **//
	
	// extractps
	bool extractps ( int32_t x64DestReg, int32_t sseSrcReg, char Imm8 );

	// movmskb

	// avx
	bool movmskb1regreg(int32_t x64DestReg, int32_t sseSrcReg);
	bool movmskb2regreg(int32_t x64DestReg, int32_t sseSrcReg);

	// sse
	bool movmskbregreg(int32_t x64DestReg, int32_t sseSrcReg);


	// movmskps

	// avx
	bool movmskps1regreg(int32_t x64DestReg, int32_t sseSrcReg);
	bool movmskps2regreg ( int32_t x64DestReg, int32_t sseSrcReg );
	
	// sse
	bool movmskpsregreg ( int32_t x64DestReg, int32_t sseSrcReg );

	// movmskpd

	// avx
	bool movmskpd1regreg(int32_t x64DestReg, int32_t sseSrcReg);
	bool movmskpd2regreg(int32_t x64DestReg, int32_t sseSrcReg);


	// vcvtdq2ps

	// avx512
	bool cvtdq2ps_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtdq2ps_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtdq2ps_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	// avx
	bool cvtdq2ps1regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool cvtdq2ps2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool cvtsi2ss1regreg(int32_t sseDestReg, int32_t sseSrcReg, int32_t x64SrcReg);
	bool cvtsi2ss1regmem(int32_t sseDestReg, int32_t sseSrcReg, int32_t* pSrcPtr);

	// sse
	bool cvtdq2psregreg(int32_t sseDestReg, int32_t sseSrcReg);


	// cvtsd2ss

	// avx
	bool cvtsd2ss1regreg(int32_t sseDestReg, int32_t sseSrcReg);

	
	// movaps
	bool movaps ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movapd ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movapstomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movapsfrommem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movapdtomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool movapdfrommem ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	

	// vbroadcastss
	bool vbroadcastss ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vbroadcastsd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// vmaskmov
	bool vmaskmovpstomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseMaskReg );
	bool vmaskmovpdtomem ( int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseMaskReg );

	bool vmaskmovd1_memreg(int32_t sseMaskReg,int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	bool vmaskmovd1_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaskmovq1_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaskmovq1_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);

	bool vmaskmovd2_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	bool vmaskmovd2_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaskmovq2_memreg(void* pDestPtr, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaskmovq2_memreg(int32_t sseMaskReg, int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);

	// blendvp

	// avx
	bool blendvps1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg );
	bool blendvpd1regreg ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg );

	bool blendvps2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg);
	bool blendvpd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, int32_t sseSrc3Reg);

	bool blendps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool blendpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );

	bool blendvps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseSrc3Reg );
	bool blendvpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, int32_t sseSrc3Reg );
	bool blendps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool blendpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );


	// vpmovsxbd - up convert 4/8 bytes into mask for xmm/ymm

	// avx2
	bool vpmovsxbd1regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool vpmovsxbd2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool vpmovsxbd1regmem(int32_t sseDestReg, int32_t* SrcPtr);
	bool vpmovsxbd2regmem(int32_t sseDestReg, int64_t* SrcPtr);

	// vpmovsxdq - up convert 32-bit value to 64-bit value with sign-extension

	// avx512

	// rr - register-register
	bool vpmovsxdq_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovsxdq_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovsxdq_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	// vpmovzxdq - up convert 32-bit value to 64-bit value with zero-extension

	// avx512

	// rr - register-register
	bool vpmovzxdq_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovzxdq_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovzxdq_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	// vpmovqd - down convert 64-bit value to 32-bit value with truncation

	// avx512

	// rr - register-register
	bool vpmovqd_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovqd_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool vpmovqd_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);


	// vpbroadcastd

	// avx2
	bool vpbroadcastd1regreg(int32_t sseDstReg, int32_t sseSrcReg);
	bool vpbroadcastd1regmem(int32_t sseDstReg, int32_t* SrcPtr);
	bool vpbroadcastd2regreg(int32_t sseDstReg, int32_t sseSrcReg);
	bool vpbroadcastd2regmem(int32_t sseDstReg, int32_t* SrcPtr);

	// avx512

	// rr - register-register
	bool vpbroadcastd_rx512(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastd_rx256(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastd_rx128(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);

	bool vpbroadcastd_rm512(int32_t dst, int32_t* SrcPtr, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastd_rm256(int32_t dst, int32_t* SrcPtr, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastd_rm128(int32_t dst, int32_t* SrcPtr, int32_t mask = 0, int32_t z = 0);

	// vpbroadcastq

	// avx2
	bool vpbroadcastq1regreg(int32_t sseDstReg, int32_t sseSrcReg);
	bool vpbroadcastq1regmem(int32_t sseDstReg, int64_t* SrcPtr);
	bool vpbroadcastq2regreg(int32_t sseDstReg, int32_t sseSrcReg);
	bool vpbroadcastq2regmem(int32_t sseDstReg, int64_t* SrcPtr);

	// avx512

	// rr - register-register
	bool vpbroadcastq_rr512(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastq_rr256(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);
	bool vpbroadcastq_rr128(int32_t dst, int32_t x64reg, int32_t mask = 0, int32_t z = 0);


	// vpternlogd - 32-bit ternary instructions

	// avx512

	bool vpternlogd_rrr512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vpternlogd_rrr256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vpternlogd_rrr128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vpternlogq_rrr512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vpternlogq_rrr256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vpternlogq_rrr128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);


	// *** SSE instructions *** //

	// avx
	bool movd1_regmem(int32_t sseDestReg, int32_t* SrcPtr);
	bool movd1_memreg(int32_t* DstPtr, int32_t sseSrcReg);
	bool movq1_regmem(int32_t sseDestReg, int64_t* SrcPtr);
	bool movq1_memreg(int64_t* DstPtr, int32_t sseSrcReg);

	//bool movd1_memreg(int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	//bool movd1_regmem(int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	//bool movq1_memreg(int32_t sseSrcReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);
	//bool movq1_regmem(int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset);

	bool movd_to_sse128(int32_t sseDestReg, int32_t x64SrcReg);
	bool movd_from_sse128(int32_t x64DestReg, int32_t sseSrcReg);

	bool movq_to_sse128(int32_t sseDestReg, int32_t x64SrcReg);
	bool movq_from_sse128(int32_t x64DestReg, int32_t sseSrcReg);

	// avx512
	bool movd_to_sse_rr128(int32_t dst, int32_t x64reg);
	bool movd_to_x64_rr128(int32_t x64reg, int32_t src0);
	bool movd_to_sse_rm128(int32_t dst, int32_t* pSrcPtr);
	bool movd_to_x64_mr128(int32_t* pDstPtr, int32_t src0);

	bool movq_to_sse_rr128(int32_t dst, int32_t x64reg);
	bool movq_to_x64_rr128(int32_t x64reg, int32_t src0);
	bool movq_to_sse_rm128(int32_t dst, int64_t* pSrcPtr);
	bool movq_to_x64_mr128(int64_t* pDstPtr, int32_t src0);


	// sse
	bool movd_to_sse ( int32_t sseDestReg, int32_t x64SrcReg );
	bool movd_from_sse ( int32_t x64DestReg, int32_t sseSrcReg );
	
	bool movq_to_sse ( int32_t sseDestReg, int32_t x64SrcReg );
	bool movq_from_sse ( int32_t x64DestReg, int32_t sseSrcReg );

	bool movd_regmem ( int32_t sseDestReg, int32_t* SrcPtr );
	bool movd_memreg ( int32_t* DstPtr, int32_t sseSrcReg );
	bool movq_regmem ( int32_t sseDestReg, int32_t* SrcPtr );
	bool movq_memreg ( int32_t* DstPtr, int32_t sseSrcReg );


	
	// move and duplicate double floating point values
	bool movddup_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool movddup_regmem ( int32_t sseDestReg, int64_t* SrcPtr );
	
	

	
	// *** SSE floating point instructions *** //
	

	bool addsd ( int32_t sseDestReg, int32_t sseSrcReg );
	bool subsd ( int32_t sseDestReg, int32_t sseSrcReg );
	bool mulsd ( int32_t sseDestReg, int32_t sseSrcReg );
	bool divsd ( int32_t sseDestReg, int32_t sseSrcReg );
	bool sqrtsd ( int32_t sseDestReg, int32_t sseSrcReg );
	
	bool mulss(int32_t sseDestReg, int32_t sseSrcReg);

	bool addpsregreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool mulpsregreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool addpdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool addpdregmem ( int32_t sseDestReg, void* SrcPtr );

	bool mulpdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool mulpdregmem ( int32_t sseDestReg, void* SrcPtr );
	
	// convert with truncation scalar single precision floating point value to general purpose register signed doubleword

	// avx512
	bool cvttps2dq_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvttps2dq_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvttps2dq_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	bool cvttss2si_rr128(int32_t dst, int32_t src0);

	// avx
	bool cvttps2dq1regreg(int32_t sseDestReg, int32_t sseSrcReg);
	bool cvttps2dq2regreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool cvttss2si1regreg(int32_t sseDestReg, int32_t sseSrcReg);

	bool cvtss2sd1regreg(int32_t sseDestReg, int32_t sseSrcReg);

	// sse
	bool cvttss2si ( int32_t x64DestReg, int32_t sseSrcReg );
	bool cvttps2dq_regreg ( int32_t sseDestReg, int32_t sseSrcReg );
	
	// convert doubleword integer to scalar double precision floating point value
	bool cvtsi2sd ( int32_t sseDestReg, int32_t x64SrcReg );
	bool cvtdq2pd_regreg ( int32_t sseDestReg, int32_t sseSrcReg );

	bool cvtss2sdregreg(int32_t sseDestReg, int32_t sseSrcReg);

	// avx 512

	bool cvtss2sd_rr32(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtss2sd_rm32(int32_t avxDstReg, int32_t* pSrcPtr, int32_t mask = 0, int32_t z = 0);

	bool cvtsd2ss_rr64(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	bool cvtps2pd_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtps2pd_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtps2pd_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);

	bool cvtpd2ps_rr512(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtpd2ps_rr256(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);
	bool cvtpd2ps_rr128(int32_t dst, int32_t src0, int32_t mask = 0, int32_t z = 0);


	// *** AVX floating point instructions *** //
	
	// addp

	// avx512
	bool vaddpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vaddpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vaddpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vaddps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0, int32_t rnd_enable = 0, int32_t rnd_type = 0);
	bool vaddps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vaddps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vaddss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0, int32_t rnd_enable = 0, int32_t rnd_type = 0);
	bool vaddsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx

	bool vaddps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vaddps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vaddpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vaddpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vaddss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vaddsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );


	bool vaddps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vaddpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vaddss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vaddsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// andp
	bool andps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool andpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool andps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool andpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// andnp
	bool andnps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool andnpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool andnps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool andnpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// cmpxxp

	// avx512
	bool cmppseq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppsne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppsgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppsge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppslt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppsle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool cmpsseq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpssne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpssgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpssge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsslt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpssle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);


	bool cmpsdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmpsdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool cmpsdeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmpsdne_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmpsdgt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmpsdge_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmpsdlt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmpsdle_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	bool cmppdeq_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdne_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdgt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdge_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdlt_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdle_rrr128(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool cmppdeq_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdne_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdgt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdge_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdlt_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);
	bool cmppdle_rrr256(int32_t kdst, int32_t src0, int32_t src1, int32_t mask = 0);

	bool cmppdeq_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmppdne_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmppdgt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmppdge_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmppdlt_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);
	bool cmppdle_rrm128(int32_t kdst, int32_t src0, void* pSrcPtr, int32_t mask = 0);

	// avx2
	bool cmpps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool cmppd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool cmpss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool cmpsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool cmpeqps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpeqpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpeqss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpeqsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpneps(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool cmpnepd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool cmpness(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool cmpnesd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool cmpltps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpltpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpltss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpltsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgtps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgtpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgtss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgtsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpleps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmplepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpless ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmplesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgeps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgess ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpgesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpunordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpunordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpunordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpunordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool cmpordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool cmpps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool cmppd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool cmpss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool cmpsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool cmpeqps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpeqpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpeqss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpeqsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpltps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpltpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpltss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpltsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgtps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgtpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgtss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgtsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpleps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmplepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpless ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmplesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgeps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgepd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgess ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpgesd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpunordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpunordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpunordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpunordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpordps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpordpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpordss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cmpordsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// cvtp

	bool cvtdq2pd ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool cvtdq2ps ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool cvtpd2dq ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool cvtps2dq ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool cvtps2pd ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool cvtpd2ps ( int32_t sseDestReg, int32_t sseSrc1Reg );
	
	bool cvtdq2pd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cvtdq2ps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cvtpd2dq ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cvtps2dq ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cvtps2pd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool cvtpd2ps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// avx 2

	bool cvtpd2ps1regreg(int32_t sseDestReg, int32_t sseSrc1Reg);
	bool cvtpd2ps2regreg(int32_t sseDestReg, int32_t sseSrc1Reg);

	bool cvtps2pd1regreg(int32_t sseDestReg, int32_t sseSrc1Reg);
	bool cvtps2pd2regreg(int32_t sseDestReg, int32_t sseSrc1Reg);

	// divp
	bool vdivps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vdivpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vdivss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vdivsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool vdivps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vdivpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vdivss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vdivsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	//maxp
	bool maxps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool maxpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool maxss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool maxsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool maxps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool maxpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool maxss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool maxsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// avx2
	bool vmaxsd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaxpd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vmaxpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vmaxps1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vmaxpd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vmaxss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vmaxsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	// minp
	bool minps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool minpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool minss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool minsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool minps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool minpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool minss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool minsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );

	// avx2
	bool vminsd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vminpd(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vminpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vminps1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vminpd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vminss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vminsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);


	// range

	bool vrangesd_rri64(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangesd_rmi64(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangess_rri32(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangess_rmi32(int32_t dst, int32_t src0, void* pSrcPtr32, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangeps_rri128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangeps_rri256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangeps_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangeps_rmi128(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangeps_rmi256(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangeps_rmi512(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangepd_rri128(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rri256(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rri512(int32_t dst, int32_t src0, int32_t src1, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangepd_rmi128(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rmi256(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rmi512(int32_t dst, int32_t src0, void* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);

	bool vrangepd_rbi128(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rbi256(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);
	bool vrangepd_rbi512(int32_t dst, int32_t src0, int64_t* pSrcPtr64, char Imm8, int32_t mask = 0, int32_t z = 0);


	// mulp

	// avx512
	bool vmulpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vmulpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vmulpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vmulps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vmulps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vmulps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vmulss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vmulsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// avx2
	bool vmulps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vmulps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vmulpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vmulpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vmulss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vmulsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool vmulss1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);
	bool vmulsd1_regmem(int32_t sseDestReg, int32_t sseSrc1Reg, void* pSrcPtr);

	bool vmulps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vmulpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vmulss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vmulsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );


	// orp

	// avx2
	bool vorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );

	bool vorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );


	// shufp

	// avx2
	bool vshufps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	bool vshufpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );

	bool vshufps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	bool vshufpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset, char Imm8 );
	
	// rsqrtp - I don't think this is used
	
	// sqrtp

	// avx2
	bool vsqrtps ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool vsqrtpd ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool vsqrtss ( int32_t sseDestReg, int32_t sseSrc1Reg );
	bool vsqrtsd ( int32_t sseDestReg, int32_t sseSrc1Reg );
	
	bool vsqrtps ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsqrtpd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsqrtss ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsqrtsd ( int32_t sseDestReg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// subp

	// sse
	bool subpsregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	bool subpdregreg ( int32_t sseDestReg, int32_t sseSrcReg );
	
	// avx2
	bool vsubps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vsubpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vsubss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vsubsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool vsubps2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);
	bool vsubpd2(int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg);

	bool vsubps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsubpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsubss ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vsubsd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	// avx-512
	bool vsubps_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0, int32_t rnd_enable = 0, int32_t rnd_type = 0);
	bool vsubps_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vsubps_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vsubpd_rrr512(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vsubpd_rrr256(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);
	bool vsubpd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	bool vsubss_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0, int32_t rnd_enable = 0, int32_t rnd_type = 0);

	bool vsubsd_rrr128(int32_t dst, int32_t src0, int32_t src1, int32_t mask = 0, int32_t z = 0);

	// vperm2f128

	// avx2
	bool vperm2f128 ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg, char Imm8 );
	
	// xorp

	// avx2
	bool vxorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	bool vxorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t sseSrc2Reg );
	
	bool vxorps ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	bool vxorpd ( int32_t sseDestReg, int32_t sseSrc1Reg, int32_t x64BaseReg, int32_t x64IndexReg, int32_t Scale, int32_t Offset );
	
	
	// **** bookmarking **** //
	
	// if you have a jump and don't know how far you'll need to jump, you'll need to store the jump amount later
	// this will return a bookmark for the current byte and advance to the next byte so you can insert the next instruction
	int32_t x64GetBookmark8 ( void );
	
	// this is where you can write to where you put the bookmark - this will be used for int16_t jumps
	bool x64SetBookmark8 ( int32_t Bookmark, char value );

private:

	bool x64EncodeReg16 ( int32_t x64InstOpcode, int32_t ModRMOpcode, int32_t x64Reg );

	// general encode of sib and immediates and stuff for register-memory instructions
	inline bool x64EncodeMem ( int32_t x64DestReg, int32_t BaseAddressReg, int32_t IndexReg, int32_t Scale, int32_t Offset );
	
	inline bool x64Encode16Bit ( void );
	inline bool x64EncodePrefix ( char Prefix );
	
	bool x64EncodeRexReg32 ( int32_t DestReg_RM_Base, int32_t SourceReg_Reg_Opcode );
	bool x64EncodeRexReg64 ( int32_t DestReg_RM_Base, int32_t SourceReg_Reg_Opcode );

	// general encoding of opcode(s)
	inline bool x64EncodeOpcode ( int32_t x64InstOpcode );
};

#endif	// end #ifndef __X64ENCODER_H__
