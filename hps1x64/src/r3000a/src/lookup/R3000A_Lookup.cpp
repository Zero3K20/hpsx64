
#include "MipsOpcode.h"
#include "R3000A_Lookup.h"

using namespace R3000A;
using namespace R3000A::Instruction;


#ifdef ENABLE_R3000A_LOOKUP_TABLE

bool Lookup::c_bObjectInitialized = false;

alignas(32) u8* Lookup::LookupTable;	// [c_iLookupTable_Size] ;

#endif


// in format: instruction name, opcode, rs, funct, rt
Instruction::Entry Instruction::Lookup::Entries [] = {
{ "BLTZ",		0x1,		ANY_VALUE,		0x0,			ANY_VALUE,		IDX_BLTZ },
{ "BGEZ",		0x1,		ANY_VALUE,		0x1,			ANY_VALUE,		IDX_BGEZ },
{ "BLTZAL",		0x1,		ANY_VALUE,		0x10,			ANY_VALUE,		IDX_BLTZAL },
{ "BGEZAL",		0x1,		ANY_VALUE,		0x11,			ANY_VALUE,		IDX_BGEZAL },
{ "BEQ",		0x4,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BEQ },
{ "BNE",		0x5,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BNE },
{ "BLEZ",		0x6,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BLEZ },
{ "BGTZ",		0x7,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BGTZ },
{ "J",			0x2,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_J },
{ "JAL",		0x3,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_JAL },
{ "JR",			0x0,		ANY_VALUE,		ANY_VALUE,		0x8,			IDX_JR },
{ "JALR", 		0x0,		ANY_VALUE,		ANY_VALUE,		0x9,			IDX_JALR },
{ "LB",			0x20,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LB },
{ "LH",			0x21,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LH },
{ "LWL",		0x22,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWL },
{ "LW",			0x23,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LW },
{ "LBU",		0x24,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LBU },
{ "LHU",		0x25,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LHU },
{ "LWR",		0x26,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWR },
{ "SB",			0x28,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SB },
{ "SH",			0x29,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SH },
{ "SWL",		0x2a,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWL },
{ "SW",			0x2b,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SW },
{ "SWR",		0x2e,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWR },
{ "LWC2",		0x32,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWC2 },
{ "SWC2",		0x3a,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWC2 },
{ "ADDI",		0x8,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ADDI },
{ "ADDIU",		0x9,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ADDIU },
{ "SLTI",		0xa,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SLTI },
{ "SLTIU",		0xb,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SLTIU },
{ "ANDI",		0xc,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ANDI },
{ "ORI",		0xd,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ORI },
{ "XORI",		0xe,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_XORI },
{ "LUI",		0xf,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LUI },
{ "SLL",		0x0,		ANY_VALUE,		ANY_VALUE,		0x0,			IDX_SLL },
{ "SRL",		0x0,		ANY_VALUE,		ANY_VALUE,		0x2,			IDX_SRL },
{ "SRA",		0x0,		ANY_VALUE,		ANY_VALUE,		0x3,			IDX_SRA },
{ "SLLV",		0x0,		ANY_VALUE,		ANY_VALUE,		0x4,			IDX_SLLV },
{ "SRLV",		0x0,		ANY_VALUE,		ANY_VALUE,		0x6,			IDX_SRLV },
{ "SRAV",		0x0,		ANY_VALUE,		ANY_VALUE,		0x7,			IDX_SRAV },
{ "SYSCALL",	0x0,		ANY_VALUE,		ANY_VALUE,		0xc,			IDX_SYSCALL },
{ "BREAK",		0x0,		ANY_VALUE,		ANY_VALUE,		0xd,			IDX_BREAK },
{ "MFHI",		0x0,		ANY_VALUE,		ANY_VALUE,		0x10,			IDX_MFHI },
{ "MTHI",		0x0,		ANY_VALUE,		ANY_VALUE,		0x11,			IDX_MTHI },
{ "MFLO",		0x0,		ANY_VALUE,		ANY_VALUE,		0x12,			IDX_MFLO },
{ "MTLO",		0x0,		ANY_VALUE,		ANY_VALUE,		0x13,			IDX_MTLO },
{ "MULT",		0x0,		ANY_VALUE,		ANY_VALUE,		0x18,			IDX_MULT },
{ "MULTU",		0x0,		ANY_VALUE,		ANY_VALUE,		0x19,			IDX_MULTU },
{ "DIV",		0x0,		ANY_VALUE,		ANY_VALUE,		0x1a,			IDX_DIV },
{ "DIVU",		0x0,		ANY_VALUE,		ANY_VALUE,		0x1b,			IDX_DIVU },
{ "ADD",		0x0,		ANY_VALUE,		ANY_VALUE,		0x20,			IDX_ADD },
{ "ADDU",		0x0,		ANY_VALUE,		ANY_VALUE,		0x21,			IDX_ADDU },
{ "SUB",		0x0,		ANY_VALUE,		ANY_VALUE,		0x22,			IDX_SUB },
{ "SUBU",		0x0,		ANY_VALUE,		ANY_VALUE,		0x23,			IDX_SUBU },
{ "AND",		0x0,		ANY_VALUE,		ANY_VALUE,		0x24,			IDX_AND },
{ "OR",			0x0,		ANY_VALUE,		ANY_VALUE,		0x25,			IDX_OR },
{ "XOR",		0x0,		ANY_VALUE,		ANY_VALUE,		0x26,			IDX_XOR },
{ "NOR",		0x0,		ANY_VALUE,		ANY_VALUE,		0x27,			IDX_NOR },
{ "SLT",		0x0,		ANY_VALUE,		ANY_VALUE,		0x2a,			IDX_SLT },
{ "SLTU",		0x0,		ANY_VALUE,		ANY_VALUE,		0x2b,			IDX_SLTU },
{ "MFC0",		0x10,		0x0,			ANY_VALUE,		ANY_VALUE,		IDX_MFC0 },
{ "MTC0",		0x10,		0x4,			ANY_VALUE,		ANY_VALUE,		IDX_MTC0 },
{ "MFC2",		0x12,		0x0,			ANY_VALUE,		ANY_VALUE,		IDX_MFC2 },
{ "CFC2",		0x12,		0x2,			ANY_VALUE,		ANY_VALUE,		IDX_CFC2 },
{ "MTC2",		0x12,		0x4,			ANY_VALUE,		ANY_VALUE,		IDX_MTC2 },
{ "CTC2",		0x12,		0x6,			ANY_VALUE,		ANY_VALUE,		IDX_CTC2 },


{ "RFE",		0x10,		0x10,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x11,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x12,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x13,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x14,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x15,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x16,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x17,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x18,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x19,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1a,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1b,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1c,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1d,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1e,			ANY_VALUE,		0x10,			IDX_RFE },
{ "RFE",		0x10,		0x1f,			ANY_VALUE,		0x10,			IDX_RFE },

// *** COP 2 Instructions ***

{ "RTPS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x1,			IDX_RTPS },
{ "NCLIP",		0x12,		ANY_VALUE,		ANY_VALUE,		0x6,			IDX_NCLIP },
{ "OP",			0x12,		ANY_VALUE,		ANY_VALUE,		0xc,			IDX_OP },
{ "DPCS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x10,			IDX_DPCS },
{ "INTPL",		0x12,		ANY_VALUE,		ANY_VALUE,		0x11,			IDX_INTPL },
{ "MVMVA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x12,			IDX_MVMVA },
{ "NCDS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x13,			IDX_NCDS },
{ "CDP",		0x12,		ANY_VALUE,		ANY_VALUE,		0x14,			IDX_CDP },
{ "NCDT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x16,			IDX_NCDT },
{ "NCCS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x1b,			IDX_NCCS },
{ "CC",			0x12,		ANY_VALUE,		ANY_VALUE,		0x1c,			IDX_CC },
{ "NCS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x1e,			IDX_NCS },
{ "NCT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x20,			IDX_NCT },
{ "SQR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x28,			IDX_SQR },
{ "DCPL",		0x12,		ANY_VALUE,		ANY_VALUE,		0x29,			IDX_DCPL },
{ "DPCT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x2a,			IDX_DPCT },
{ "AVSZ3",		0x12,		ANY_VALUE,		ANY_VALUE,		0x2d,			IDX_AVSZ3 },
{ "AVSZ4",		0x12,		ANY_VALUE,		ANY_VALUE,		0x2e,			IDX_AVSZ4 },
{ "RTPT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x30,			IDX_RTPT },
{ "GPF",		0x12,		ANY_VALUE,		ANY_VALUE,		0x3d,			IDX_GPF },
{ "GPL",		0x12,		ANY_VALUE,		ANY_VALUE,		0x3e,			IDX_GPL },
{ "NCCT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x3f,			IDX_NCCT }
};






//Debug::Log Lookup::debug;


#ifdef ENABLE_R3000A_LOOKUP_TABLE

void Lookup::Start ()
{
	u32 Opcode, Rs, Funct, Rt, Index, ElementsInExecute, ElementsInBranchLoad1;
	Instruction::Format i;
	
	u32 ulCounter, ulRemainder;

	if ( c_bObjectInitialized ) return;
	
	cout << "Running constructor for Execute class.\n";

#ifdef INLINE_DEBUG_ENABLE	
	debug.Create ( "R3000A_Execute_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "Running constructor for Execute class.\r\n";
#endif


	//LookupTable = new unsigned char[c_iLookupTable_Size];
	LookupTable = (unsigned char*)malloc(c_iLookupTable_Size);

	
	ElementsInExecute = (sizeof(Entries) / sizeof(Entries[0]));
	
	// clear table first
	cout << "\nSize of R3000A lookup table in bytes=" << dec << sizeof( LookupTable );
	for ( Index = 0; Index < (c_iLookupTable_Size >> 3 ); Index++ ) ((u64*)LookupTable) [ Index ] = 0;
	
	for ( Index = ElementsInExecute - 1; Index < ElementsInExecute; Index-- )
	{
		ulRemainder = 0;
		
		Opcode = Entries [ Index ].Opcode;
		Rs = Entries [ Index ].Rs;
		Rt = Entries [ Index ].Rt;
		//Shft = Entries [ Index ].Shift;
		Funct = Entries [ Index ].Funct;
		
		i.Opcode = Opcode;
		i.Rs = Rs;
		i.Rt = Rt;
		//i.Shift = Shft;
		i.Funct = Funct;
		
		for ( ulCounter = 0; ulRemainder == 0; ulCounter++ )
		{
			ulRemainder = ulCounter;
			
			if ( Opcode == 0xff )
			{
				// 6 bits
				i.Opcode = ulRemainder & 0x3f;
				ulRemainder >>= 6;
			}
			
			if ( Rs == 0xff )
			{
				// 5 bits
				i.Rs = ulRemainder & 0x1f;
				ulRemainder >>= 5;
			}
			
			if ( Rt == 0xff )
			{
				// 5 bits
				i.Rt = ulRemainder & 0x1f;
				ulRemainder >>= 5;
			}
			
			//if ( Shft == 0xff )
			//{
			//	// 5 bits
			//	i.Shift = ulRemainder & 0x1f;
			//	ulRemainder >>= 5;
			//}
			
			if ( Funct == 0xff )
			{
				// 6 bits
				i.Funct = ulRemainder & 0x3f;
				ulRemainder >>= 6;
			}
			
			LookupTable [ ( ( i.Value >> 16 ) | ( i.Value << 16 ) ) & c_iLookupTable_Mask ] = Entries [ Index ].InstructionIndex;
		}
	}
	
	// object has now been fully initilized
	c_bObjectInitialized = true;
}

// deallocate the large lookup table arrays
void Lookup::Stop()
{
	if (!c_bObjectInitialized) return;

	delete LookupTable;

	c_bObjectInitialized = false;
}

#endif


u8 Lookup::FindByInstruction(u32 instruction)
{
	Instruction::Format i;
	i.Value = instruction;

	switch (i.Opcode)
	{
	case 0x00:
		switch (i.Funct)
		{
		case 0x00:
			return IDX_SLL;
			break;

		case 0x02:
			return IDX_SRL;
			break;

		case 0x03:
			return IDX_SRA;
			break;

		case 0x04:
			return IDX_SLLV;
			break;

		case 0x06:
			return IDX_SRLV;
			break;

		case 0x07:
			return IDX_SRAV;
			break;

		case 0x08:
			return IDX_JR;
			break;

		case 0x09:
			return IDX_JALR;
			break;

		case 0x0c:
			return IDX_SYSCALL;
			break;

		case 0x0d:
			return IDX_BREAK;
			break;

		case 0x10:
			return IDX_MFHI;
			break;

		case 0x11:
			return IDX_MTHI;
			break;

		case 0x12:
			return IDX_MFLO;
			break;

		case 0x13:
			return IDX_MTLO;
			break;

		case 0x18:
			return IDX_MULT;
			break;

		case 0x19:
			return IDX_MULTU;
			break;

		case 0x1a:
			return IDX_DIV;
			break;

		case 0x1b:
			return IDX_DIVU;
			break;

		case 0x20:
			return IDX_ADD;
			break;

		case 0x21:
			return IDX_ADDU;
			break;

		case 0x22:
			return IDX_SUB;
			break;

		case 0x23:
			return IDX_SUBU;
			break;

		case 0x24:
			return IDX_AND;
			break;

		case 0x25:
			return IDX_OR;
			break;

		case 0x26:
			return IDX_XOR;
			break;

		case 0x27:
			return IDX_NOR;
			break;

		case 0x2a:
			return IDX_SLT;
			break;

		case 0x2b:
			return IDX_SLTU;
			break;
		}
		break;

	case 0x01:
		switch (i.Rt)
		{
		case 0x00:
			return IDX_BLTZ;
			break;

		case 0x01:
			return IDX_BGEZ;
			break;

		case 0x10:
			return IDX_BLTZAL;
			break;

		case 0x11:
			return IDX_BGEZAL;
			break;
		}
		break;

	case 0x02:
		return IDX_J;
		break;

	case 0x03:
		return IDX_JAL;
		break;

	case 0x04:
		return IDX_BEQ;
		break;

	case 0x05:
		return IDX_BNE;
		break;

	case 0x06:
		return IDX_BLEZ;
		break;

	case 0x07:
		return IDX_BGTZ;
		break;

	case 0x08:
		return IDX_ADDI;
		break;

	case 0x09:
		return IDX_ADDIU;
		break;

	case 0x0a:
		return IDX_SLTI;
		break;

	case 0x0b:
		return IDX_SLTIU;
		break;

	case 0x0c:
		return IDX_ANDI;
		break;

	case 0x0d:
		return IDX_ORI;
		break;

	case 0x0e:
		return IDX_XORI;
		break;

	case 0x0f:
		return IDX_LUI;
		break;

	case 0x10:
		switch (i.Rs)
		{
		case 0x00:
			return IDX_MFC0;
			break;

		case 0x04:
			return IDX_MTC0;
			break;
		}

		if ((i.Rs >= 0x10) && (i.Rs <= 0x1f) && (i.Funct == 0x10))
		{
			return IDX_RFE;
		}

		break;

	case 0x12:
		switch (i.Rs)
		{
		case 0x00:
			return IDX_MFC2;
			break;

		case 0x02:
			return IDX_CFC2;
			break;

		case 0x04:
			return IDX_MTC2;
			break;

		case 0x06:
			return IDX_CTC2;
			break;
		}

		switch (i.Funct)
		{
		case 0x01:
			return IDX_RTPS;
			break;

		case 0x06:
			return IDX_NCLIP;
			break;

		case 0x0c:
			return IDX_OP;
			break;

		case 0x10:
			return IDX_DPCS;
			break;

		case 0x11:
			return IDX_INTPL;
			break;

		case 0x12:
			return IDX_MVMVA;
			break;

		case 0x13:
			return IDX_NCDS;
			break;

		case 0x14:
			return IDX_CDP;
			break;

		case 0x16:
			return IDX_NCDT;
			break;

		case 0x1b:
			return IDX_NCCS;
			break;

		case 0x1c:
			return IDX_CC;
			break;

		case 0x1e:
			return IDX_NCS;
			break;

		case 0x20:
			return IDX_NCT;
			break;

		case 0x28:
			return IDX_SQR;
			break;

		case 0x29:
			return IDX_DCPL;
			break;

		case 0x2a:
			return IDX_DPCT;
			break;

		case 0x2d:
			return IDX_AVSZ3;
			break;

		case 0x2e:
			return IDX_AVSZ4;
			break;

		case 0x30:
			return IDX_RTPT;
			break;

		case 0x3d:
			return IDX_GPF;
			break;

		case 0x3e:
			return IDX_GPL;
			break;

		case 0x3f:
			return IDX_NCCT;
			break;
		}

		break;

	case 0x20:
		return IDX_LB;
		break;

	case 0x21:
		return IDX_LH;
		break;

	case 0x22:
		return IDX_LWL;
		break;

	case 0x23:
		return IDX_LW;
		break;

	case 0x24:
		return IDX_LBU;
		break;

	case 0x25:
		return IDX_LHU;
		break;

	case 0x26:
		return IDX_LWR;
		break;

	case 0x28:
		return IDX_SB;
		break;

	case 0x29:
		return IDX_SH;
		break;

	case 0x2a:
		return IDX_SWL;
		break;

	case 0x2b:
		return IDX_SW;
		break;

	case 0x2e:
		return IDX_SWR;
		break;

	case 0x32:
		return IDX_LWC2;
		break;

	case 0x3a:
		return IDX_SWC2;
		break;
	}

	return IDX_INVALID;
}


