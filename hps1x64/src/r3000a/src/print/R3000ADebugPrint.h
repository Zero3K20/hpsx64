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
#ifndef __R3000ADEBUGPRINT_H__
#define __R3000ADEBUGPRINT_H__


#include <sstream>
#include <string>


//#include "R3000A.h"
#include "R3000A_Instruction.h"
#include "R3000A_Lookup.h"
#include "types.h"

#include "MipsOpcode.h"

using namespace std;


namespace R3000A
{

	namespace Instruction
	{
	
		class Print
		{
		public:
			//using namespace std;

			typedef void (*Function) ( stringstream &s, long instruction );
			
			static const Function FunctionList [];

			// this gets the instruction text
			static string PrintInstruction ( long instruction );

			static const char XyzwLUT [ 4 ];	// = { 'x', 'y', 'z', 'w' };
			static const char* BCType [ 4 ];// = { "F", "T", "FL", "TL" };

			// constructor
			//Print ();
			
			// creates lookup table from list of entries
			static void Start ();
		
			// returns true if the instruction has executed its last cycle
			// r - pointer to the Cpu object to execute the instruction on
			// i - the instruction to execute
			// CycleToExecute - the cycle to execute for the instruction. To execute first cycle, this should be zero
			//typedef bool (*Function) ( R3000A::Cpu* r, Instruction::Format i );
			
			//static Entry<Function> Entries [];

			// this actually has to be a 4D array, but I'll make it one array			
			// use Opcode, Rs, Rt, and Funct to lookup value
			//static Function LookupTable [ 64 * 32 * 32 * 64 ];
			
			static void AddInstArgs ( stringstream &strMipsArgs, long Instruction, long InstFormat );
			static void AddVuDestArgs ( stringstream &strVuArgs, long Instruction );

		

			static void Invalid ( stringstream &s, long instruction );

			static void J ( stringstream &s, long instruction );
			static void JAL ( stringstream &s, long instruction );
			static void BEQ ( stringstream &s, long instruction );
			static void BNE ( stringstream &s, long instruction );
			static void BLEZ ( stringstream &s, long instruction );
			static void BGTZ ( stringstream &s, long instruction );
			static void ADDI ( stringstream &s, long instruction );
			static void ADDIU ( stringstream &s, long instruction );
			static void SLTI ( stringstream &s, long instruction );
			static void SLTIU ( stringstream &s, long instruction );
			static void ANDI ( stringstream &s, long instruction );
			static void ORI ( stringstream &s, long instruction );
			static void XORI ( stringstream &s, long instruction );
			static void LUI ( stringstream &s, long instruction );
			static void LB ( stringstream &s, long instruction );
			static void LH ( stringstream &s, long instruction );
			static void LWL ( stringstream &s, long instruction );
			static void LW ( stringstream &s, long instruction );
			static void LBU ( stringstream &s, long instruction );
			static void LHU ( stringstream &s, long instruction );
			
			static void LWR ( stringstream &s, long instruction );
			static void SB ( stringstream &s, long instruction );
			static void SH ( stringstream &s, long instruction );
			static void SWL ( stringstream &s, long instruction );
			static void SW ( stringstream &s, long instruction );
			static void SWR ( stringstream &s, long instruction );
			static void LWC2 ( stringstream &s, long instruction );
			static void SWC2 ( stringstream &s, long instruction );
			static void SLL ( stringstream &s, long instruction );
			static void SRL ( stringstream &s, long instruction );
			static void SRA ( stringstream &s, long instruction );
			static void SLLV ( stringstream &s, long instruction );
			static void SRLV ( stringstream &s, long instruction );
			static void SRAV ( stringstream &s, long instruction );
			static void JR ( stringstream &s, long instruction );
			static void JALR ( stringstream &s, long instruction );
			static void SYSCALL ( stringstream &s, long instruction );
			static void BREAK ( stringstream &s, long instruction );
			static void MFHI ( stringstream &s, long instruction );
			static void MTHI ( stringstream &s, long instruction );

			static void MFLO ( stringstream &s, long instruction );
			static void MTLO ( stringstream &s, long instruction );
			static void MULT ( stringstream &s, long instruction );
			static void MULTU ( stringstream &s, long instruction );
			static void DIV ( stringstream &s, long instruction );
			static void DIVU ( stringstream &s, long instruction );
			static void ADD ( stringstream &s, long instruction );
			static void ADDU ( stringstream &s, long instruction );
			static void SUB ( stringstream &s, long instruction );
			static void SUBU ( stringstream &s, long instruction );
			static void AND ( stringstream &s, long instruction );
			static void OR ( stringstream &s, long instruction );
			static void XOR ( stringstream &s, long instruction );
			static void NOR ( stringstream &s, long instruction );
			static void SLT ( stringstream &s, long instruction );
			static void SLTU ( stringstream &s, long instruction );
			static void BLTZ ( stringstream &s, long instruction );
			static void BGEZ ( stringstream &s, long instruction );
			static void BLTZAL ( stringstream &s, long instruction );
			static void BGEZAL ( stringstream &s, long instruction );

			static void MFC0 ( stringstream &s, long instruction );
			static void MTC0 ( stringstream &s, long instruction );
			static void RFE ( stringstream &s, long instruction );
			static void MFC2 ( stringstream &s, long instruction );
			static void CFC2 ( stringstream &s, long instruction );
			static void MTC2 ( stringstream &s, long instruction );
			static void CTC2 ( stringstream &s, long instruction );
			static void COP2 ( stringstream &s, long instruction );
			
			static void RTPS ( stringstream &s, long instruction );
			static void NCLIP ( stringstream &s, long instruction );
			static void OP ( stringstream &s, long instruction );
			static void DPCS ( stringstream &s, long instruction );
			static void INTPL ( stringstream &s, long instruction );
			static void MVMVA ( stringstream &s, long instruction );
			static void NCDS ( stringstream &s, long instruction );
			static void CDP ( stringstream &s, long instruction );
			static void NCDT ( stringstream &s, long instruction );
			static void NCCS ( stringstream &s, long instruction );
			static void CC ( stringstream &s, long instruction );
			static void NCS ( stringstream &s, long instruction );
			static void NCT ( stringstream &s, long instruction );
			static void SQR ( stringstream &s, long instruction );
			static void DCPL ( stringstream &s, long instruction );
			static void DPCT ( stringstream &s, long instruction );
			static void AVSZ3 ( stringstream &s, long instruction );
			static void AVSZ4 ( stringstream &s, long instruction );
			static void RTPT ( stringstream &s, long instruction );
			static void GPF ( stringstream &s, long instruction );
			static void GPL ( stringstream &s, long instruction );
			static void NCCT ( stringstream &s, long instruction );

		};
		
	}

}

#endif	// end #ifndef __R3000ADEBUGPRINT_H__
