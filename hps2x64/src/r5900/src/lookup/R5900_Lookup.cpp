
#include "R5900_Lookup.h"

using namespace R5900;
using namespace R5900::Instruction;


#ifdef ENABLE_R5900_LOOKUP_TABLE

bool Lookup::c_bObjectInitialized = false;

u16* Lookup::LookupTable;	// [c_iLookupTable_Size] ;

#endif


// used opcodes: 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
// used opcodes: 0x10, 0x11, 0x12, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1e, 0x1f
// used opcodes: 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f
// used opcodes: 0x31, 0x33, 0x36, 0x37, 0x39, 0x3e, 0x3f

// unused opcodes: 0x13, 0x1d, 0x32 (PS2 Only), 0x34, 0x35, 0x38, 0x3a, 0x3b, 0x3c, 0x3d

// in format: instruction name, opcode, rs, rt, shift, funct, index/id
const Instruction::Entry Instruction::Lookup::Entries [] = {

// unused opcodes //
//{ "INVALID",	0x13,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x1d,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x32,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x34,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x35,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x38,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x3a,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x3b,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x3c,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },
//{ "INVALID",	0x3d,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid },


// used opcodes //

{ "BLTZ",		0x1,		ANY_VALUE,		0x0,			ANY_VALUE,		ANY_VALUE,		IDX_BLTZ },
{ "BGEZ",		0x1,		ANY_VALUE,		0x1,			ANY_VALUE,		ANY_VALUE,		IDX_BGEZ },
{ "BLTZAL",		0x1,		ANY_VALUE,		0x10,			ANY_VALUE,		ANY_VALUE,		IDX_BLTZAL },
{ "BGEZAL",		0x1,		ANY_VALUE,		0x11,			ANY_VALUE,		ANY_VALUE,		IDX_BGEZAL },
{ "BEQ",		0x4,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BEQ },
{ "BNE",		0x5,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BNE },
{ "BLEZ",		0x6,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BLEZ },
{ "BGTZ",		0x7,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BGTZ },
{ "J",			0x2,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_J },
{ "JAL",		0x3,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_JAL },
{ "JR",			0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x8,			IDX_JR },
{ "JALR", 		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x9,			IDX_JALR },
{ "LB",			0x20,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LB },
{ "LH",			0x21,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LH },
{ "LWL",		0x22,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWL },
{ "LW",			0x23,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LW },
{ "LBU",		0x24,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LBU },
{ "LHU",		0x25,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LHU },
{ "LWR",		0x26,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWR },
{ "SB",			0x28,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SB },
{ "SH",			0x29,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SH },
{ "SWL",		0x2a,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWL },
{ "SW",			0x2b,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SW },
{ "SWR",		0x2e,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWR },
{ "ADDI",		0x8,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ADDI },
{ "ADDIU",		0x9,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ADDIU },
{ "SLTI",		0xa,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SLTI },
{ "SLTIU",		0xb,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SLTIU },
{ "ANDI",		0xc,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ANDI },
{ "ORI",		0xd,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_ORI },
{ "XORI",		0xe,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_XORI },
{ "LUI",		0xf,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LUI },
{ "SLL",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0,			IDX_SLL },
{ "SRL",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2,			IDX_SRL },
{ "SRA",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x3,			IDX_SRA },
{ "SLLV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x4,			IDX_SLLV },
{ "SRLV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x6,			IDX_SRLV },
{ "SRAV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x7,			IDX_SRAV },
{ "SYSCALL",	0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0xc,			IDX_SYSCALL },
{ "BREAK",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0xd,			IDX_BREAK },
{ "MFHI",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x10,			IDX_MFHI },
{ "MTHI",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x11,			IDX_MTHI },
{ "MFLO",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x12,			IDX_MFLO },
{ "MTLO",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x13,			IDX_MTLO },
{ "MULT",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x18,			IDX_MULT },
{ "MULTU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x19,			IDX_MULTU },
{ "DIV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1a,			IDX_DIV },
{ "DIVU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1b,			IDX_DIVU },
{ "ADD",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x20,			IDX_ADD },
{ "ADDU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x21,			IDX_ADDU },
{ "SUB",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x22,			IDX_SUB },
{ "SUBU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x23,			IDX_SUBU },
{ "AND",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x24,			IDX_AND },
{ "OR",			0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x25,			IDX_OR },
{ "XOR",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x26,			IDX_XOR },
{ "NOR",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x27,			IDX_NOR },
{ "SLT",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2a,			IDX_SLT },
{ "SLTU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2b,			IDX_SLTU },
{ "MFC0",		0x10,		0x0,			ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_MFC0 },
{ "MTC0",		0x10,		0x4,			ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_MTC0 },

// on the R5900, this could have the I-bit set for interlocking, or NOT set for non-interlocking
{ "CFC2_I",		0x12,		0x2,			ANY_VALUE,		ANY_VALUE,		0x1,			IDX_CFC2_I },
{ "CTC2_I",		0x12,		0x6,			ANY_VALUE,		ANY_VALUE,		0x1,			IDX_CTC2_I },
{ "CFC2_NI",	0x12,		0x2,			ANY_VALUE,		ANY_VALUE,		0x0,			IDX_CFC2_NI },
{ "CTC2_NI",	0x12,		0x6,			ANY_VALUE,		ANY_VALUE,		0x0,			IDX_CTC2_NI },

// THERE ARE NO RFE OR GTE INSTRUCTIONS ON PS2 R5900 //
/*
{ "LWC2",		0x32,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWC2 },
{ "SWC2",		0x3a,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWC2 },
{ "MFC2",		0x12,		0x0,			ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_MFC2 },
{ "MTC2",		0x12,		0x4,			ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_MTC2 },
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
*/


// *** New instructions for PS2 *** //
// Instruction, Opcode, Rs, Rt, Shift, Funct

	// branch instructions //
{ "BGEZL",		0x1,		ANY_VALUE,		0x3,			ANY_VALUE,		ANY_VALUE,		IDX_BGEZL },
{ "BLTZL",		0x1,		ANY_VALUE,		0x2,			ANY_VALUE,		ANY_VALUE,		IDX_BLTZL },
{ "BGEZALL",	0x1,		ANY_VALUE,		0x13,			ANY_VALUE,		ANY_VALUE,		IDX_BGEZALL },
{ "BLTZALL",	0x1,		ANY_VALUE,		0x12,			ANY_VALUE,		ANY_VALUE,		IDX_BLTZALL },

{ "BEQL",		0x14,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BEQL },
{ "BNEL",		0x15,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_BNEL },
{ "BLEZL",		0x16,		ANY_VALUE,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		IDX_BLEZL },
{ "BGTZL",		0x17,		ANY_VALUE,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		IDX_BGTZL },

	// arithemetic instructions w/o immediate //
{ "DADD",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x2c,			IDX_DADD },
{ "DADDU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x2d,			IDX_DADDU },
{ "DSUB",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x2e,			IDX_DSUB },
{ "DSUBU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x2f,			IDX_DSUBU },

{ "DSLL",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x38,			IDX_DSLL },
{ "DSRL",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3a,			IDX_DSRL },
{ "DSRA",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3b,			IDX_DSRA },
{ "DSLL32",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3c,			IDX_DSLL32 },
{ "DSRL32",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3e,			IDX_DSRL32 },
{ "DSRA32",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3f,			IDX_DSRA32 },

{ "DSLLV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x14,			IDX_DSLLV },
{ "DSRLV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x16,			IDX_DSRLV },
{ "DSRAV",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x17,			IDX_DSRAV },

	// arithemetic instructions w/ immediate //
{ "DADDI",		0x18,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_DADDI },
{ "DADDIU",		0x19,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_DADDIU },

	// load/store instructions //
{ "PREF",		0x33,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_PREF },

{ "LWU",		0x27,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWU },

{ "LD",			0x37,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LD },
{ "SD",			0x3f,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SD },

{ "LDL",		0x1a,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LDL },
{ "LDR",		0x1b,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LDR },
{ "SDL",		0x2c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SDL },
{ "SDR",		0x2d,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SDR },

{ "LQ",			0x1e,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LQ },
{ "SQ",			0x1f,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SQ },

	// trap instructions w/o immediate //
{ "TGE",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x30,			IDX_TGE },
{ "TGEU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x31,			IDX_TGEU },
{ "TLT",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x32,			IDX_TLT },
{ "TLTU",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x33,			IDX_TLTU },
{ "TEQ",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x34,			IDX_TEQ },
{ "TNE",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x36,			IDX_TNE },

	// trap instructions w/ immediate //
{ "TGEI",		0x1,		ANY_VALUE,		0x8,			ANY_VALUE,		ANY_VALUE,		IDX_TGEI },
{ "TGEIU",		0x1,		ANY_VALUE,		0x9,			ANY_VALUE,		ANY_VALUE,		IDX_TGEIU },
{ "TLTI",		0x1,		ANY_VALUE,		0xa,			ANY_VALUE,		ANY_VALUE,		IDX_TLTI },
{ "TLTIU",		0x1,		ANY_VALUE,		0xb,			ANY_VALUE,		ANY_VALUE,		IDX_TLTIU },
{ "TEQI",		0x1,		ANY_VALUE,		0xc,			ANY_VALUE,		ANY_VALUE,		IDX_TEQI },
{ "TNEI",		0x1,		ANY_VALUE,		0xe,			ANY_VALUE,		ANY_VALUE,		IDX_TNEI },

	// data movement instructions //
{ "MOVZ",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0xa,			IDX_MOVZ },
{ "MOVN",		0x0,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0xb,			IDX_MOVN },

	// ps2 specific instructions //
{ "MULT1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x18,			IDX_MULT1 },
{ "MULTU1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x19,			IDX_MULTU1 },
{ "DIV1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x1a,			IDX_DIV1 },
{ "DIVU1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x1b,			IDX_DIVU1 },
{ "MADD",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x0,			IDX_MADD },
{ "MADD1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x20,			IDX_MADD1 },
{ "MADDU",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x1,			IDX_MADDU },
{ "MADDU1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x21,			IDX_MADDU1 },
{ "MFHI1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x10,			IDX_MFHI1 },
{ "MTHI1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x11,			IDX_MTHI1 },
{ "MFLO1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x12,			IDX_MFLO1 },
{ "MTLO1",		0x1c,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x13,			IDX_MTLO1 },

{ "MFSA",		0x0,		ANY_VALUE,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x28,			IDX_MFSA },
{ "MTSA",		0x0,		ANY_VALUE,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x29,			IDX_MTSA },
{ "MTSAB",		0x1,		ANY_VALUE,		0x18,			ANY_VALUE,		ANY_VALUE,		IDX_MTSAB },
{ "MTSAH",		0x1,		ANY_VALUE,		0x19,			ANY_VALUE,		ANY_VALUE,		IDX_MTSAH },

{ "PABSH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x5,			0x28,			IDX_PABSH },
{ "PABSW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1,			0x28,			IDX_PABSW },
{ "PADDB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x8,			0x8,			IDX_PADDB },
{ "PADDH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x4,			0x8,			IDX_PADDH },
{ "PADDSB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x18,			0x8,			IDX_PADDSB },
{ "PADDSH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x14,			0x8,			IDX_PADDSH },
{ "PADDSW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x10,			0x8,			IDX_PADDSW },
{ "PADDUB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x18,			0x28,			IDX_PADDUB },
{ "PADDUH", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x14,			0x28,			IDX_PADDUH },
{ "PADDUW", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x10,			0x28,			IDX_PADDUW },
{ "PADDW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x0,			0x8,			IDX_PADDW },
{ "PADSBH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x4,			0x28,			IDX_PADSBH },
{ "PAND",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x12,			0x9,			IDX_PAND },
{ "PCEQB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xa,			0x28,			IDX_PCEQB },
{ "PCEQH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x6,			0x28,			IDX_PCEQH },
{ "PCEQW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x2,			0x28,			IDX_PCEQW },
{ "PCGTB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xa,			0x8,			IDX_PCGTB },
{ "PCGTH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x6,			0x8,			IDX_PCGTH },
{ "PCGTW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x2,			0x8,			IDX_PCGTW },
{ "PCPYH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1b,			0x29,			IDX_PCPYH },
{ "PCPYLD",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xe,			0x9,			IDX_PCPYLD },
{ "PCPYUD",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xe,			0x29,			IDX_PCPYUD },
{ "PDIVBW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1d,			0x9,			IDX_PDIVBW },
{ "PDIVUW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xd,			0x29,			IDX_PDIVUW },
{ "PDIVW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xd,			0x9,			IDX_PDIVW },
{ "PEXCH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1a,			0x29,			IDX_PEXCH },
{ "PEXCW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1e,			0x29,			IDX_PEXCW },
{ "PEXEH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1a,			0x9,			IDX_PEXEH },
{ "PEXEW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1e,			0x9,			IDX_PEXEW },
{ "PEXT5",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1e,			0x8,			IDX_PEXT5 },
{ "PEXTLB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1a,			0x8,			IDX_PEXTLB },
{ "PEXTLH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x16,			0x8,			IDX_PEXTLH },
{ "PEXTLW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x12,			0x8,			IDX_PEXTLW },
{ "PEXTUB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1a,			0x28,			IDX_PEXTUB },
{ "PEXTUH", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x16,			0x28,			IDX_PEXTUH },
{ "PEXTUW", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x12,			0x28,			IDX_PEXTUW },
{ "PHMADH", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x11,			0x9,			IDX_PHMADH },
{ "PHMSBH", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0x15,			0x9,			IDX_PHMSBH },
{ "PINTEH", 	0x1c,		ANY_VALUE,		ANY_VALUE,		0xa,			0x29,			IDX_PINTEH },
{ "PINTH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xa,			0x9,			IDX_PINTH },
{ "PLZCW",		0x1c,		ANY_VALUE,		0x0,			0x0,			0x4,			IDX_PLZCW },
{ "PMADDH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x10,			0x9,			IDX_PMADDH },
{ "PMADDUW",	0x1c,		ANY_VALUE,		ANY_VALUE,		0x0,			0x29,			IDX_PMADDUW },
{ "PMADDW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x0,			0x9,			IDX_PMADDW },
{ "PMAXH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x7,			0x8,			IDX_PMAXH },
{ "PMAXW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x3,			0x8,			IDX_PMAXW },
{ "PMFHI",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x8,			0x9,			IDX_PMFHI },
{ "PMFHL_LH",	0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x3,			0x30,			IDX_PMFHL_LH },
{ "PMFHL_LW",	0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x0,			0x30,			IDX_PMFHL_LW },
{ "PMFHL_SH",	0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x4,			0x30,			IDX_PMFHL_SH },
{ "PMFHL_SLW",	0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x2,			0x30,			IDX_PMFHL_SLW },
{ "PMFHL_UW",	0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x1,			0x30,			IDX_PMFHL_UW },
{ "PMFLO",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x9,			0x9,			IDX_PMFLO },
{ "PMINH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x7,			0x28,			IDX_PMINH },
{ "PMINW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x3,			0x28,			IDX_PMINW },
{ "PMSUBH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x14,			0x9,			IDX_PMSUBH },
{ "PMSUBW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x4,			0x9,			IDX_PMSUBW },
{ "PMTHI",		0x1c,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x8,			0x29,			IDX_PMTHI },
{ "PMTHL_LW",	0x1c,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x0,			0x31,			IDX_PMTHL_LW },
{ "PMTLO",		0x1c,		ANY_VALUE,		ANY_VALUE /*0x0*/,			0x9,			0x29,			IDX_PMTLO },
{ "PMULTH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1c,			0x9,			IDX_PMULTH },
{ "PMULTUW",	0x1c,		ANY_VALUE,		ANY_VALUE,		0xc,			0x29,			IDX_PMULTUW },
{ "PMULTW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0xc,	 		0x9,			IDX_PMULTW },
{ "PNOR",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x13,			0x29,			IDX_PNOR },
{ "POR",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x12,			0x29,			IDX_POR },
{ "PPAC5",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1f,			0x8,			IDX_PPAC5 },
{ "PPACB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1b,			0x8,			IDX_PPACB },
{ "PPACH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x17,			0x8,			IDX_PPACH },
{ "PPACW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x13,			0x8,			IDX_PPACW },
{ "PREVH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1b,			0x9,			IDX_PREVH },
{ "PROT3W",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		0x1f,			0x9,			IDX_PROT3W },
{ "PSLLH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x34,			IDX_PSLLH },
{ "PSLLVW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x2,			0x9,			IDX_PSLLVW },
{ "PSLLW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3c,			IDX_PSLLW },
{ "PSRAH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x37,			IDX_PSRAH },
{ "PSRAVW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x3,			0x29,			IDX_PSRAVW },
{ "PSRAW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3f,			IDX_PSRAW },
{ "PSRLH",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x36,			IDX_PSRLH },
{ "PSRLVW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x3,			0x9,			IDX_PSRLVW },
{ "PSRLW",		0x1c,		ANY_VALUE /*0x0*/,			ANY_VALUE,		ANY_VALUE,		0x3e,			IDX_PSRLW },
{ "PSUBB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x9,			0x8,			IDX_PSUBB },
{ "PSUBH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x5,			0x8,			IDX_PSUBH },
{ "PSUBSB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x19,			0x9,			IDX_PSUBSB },
{ "PSUBSH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x15,			0x8,			IDX_PSUBSH },
{ "PSUBSW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x11,			0x8,			IDX_PSUBSW },
{ "PSUBUB",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x19,			0x28,			IDX_PSUBUB },
{ "PSUBUH",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x15,			0x28,			IDX_PSUBUH },
{ "PSUBUW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x11,			0x28,			IDX_PSUBUW },
{ "PSUBW",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1,			0x8,			IDX_PSUBW },
{ "PXOR",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x13,			0x9,			IDX_PXOR },
{ "QFSRV",		0x1c,		ANY_VALUE,		ANY_VALUE,		0x1b,			0x28,			IDX_QFSRV },

{ "SYNC",		0x0,		ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			ANY_VALUE,		0xf,			IDX_SYNC },

{ "CACHE",		0x2f,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_CACHE },

{ "BC0F",		0x10,		0x8,			0x0,			ANY_VALUE,		ANY_VALUE,		IDX_BC0F },
{ "BC0FL",		0x10,		0x8,			0x2,			ANY_VALUE,		ANY_VALUE,		IDX_BC0FL },
{ "BC0T",		0x10,		0x8,			0x1,			ANY_VALUE,		ANY_VALUE,		IDX_BC0T },
{ "BC0TL",		0x10,		0x8,			0x3,			ANY_VALUE,		ANY_VALUE,		IDX_BC0TL },
{ "DI",			0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x39,			IDX_DI },
{ "EI",			0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x38,			IDX_EI },
{ "ERET",		0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x18,			IDX_ERET },

//{ "MF0",		0x10,		0x0,			ANY_VALUE,		0x0,			0x0,			IDX_CFC0 },
//{ "MT0",		0x10,		0x4,			ANY_VALUE,		0x0,			0x0,			IDX_CTC0 },

{ "TLBP",		0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x8,			IDX_TLBP },
{ "TLBR",		0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x1,			IDX_TLBR },
{ "TLBWI",		0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x2,			IDX_TLBWI },
{ "TLBWR",		0x10,		0x10,			ANY_VALUE /*0x0*/,			ANY_VALUE /*0x0*/,			0x6,			IDX_TLBWR },

{ "BC1F",		0x11,		0x8,			0x0,			ANY_VALUE,		ANY_VALUE,		IDX_BC1F },
{ "BC1FL",		0x11,		0x8,			0x2,			ANY_VALUE,		ANY_VALUE,		IDX_BC1FL },
{ "BC1T",		0x11,		0x8,			0x1,			ANY_VALUE,		ANY_VALUE,		IDX_BC1T },
{ "BC1TL",		0x11,		0x8,			0x3,			ANY_VALUE,		ANY_VALUE,		IDX_BC1TL },

{ "BC2F",		0x12,		0x8,			0x0,			ANY_VALUE,		ANY_VALUE,		IDX_BC2F },
{ "BC2FL",		0x12,		0x8,			0x2,			ANY_VALUE,		ANY_VALUE,		IDX_BC2FL },
{ "BC2T",		0x12,		0x8,			0x1,			ANY_VALUE,		ANY_VALUE,		IDX_BC2T },
{ "BC2TL",		0x12,		0x8,			0x3,			ANY_VALUE,		ANY_VALUE,		IDX_BC2TL },

{ "MFC1",		0x11,		0x0,			ANY_VALUE,		0x0,			0x0,			IDX_MFC1 },
{ "CFC1",		0x11,		0x2,			ANY_VALUE,		0x0,			0x0,			IDX_CFC1 },
{ "MTC1",		0x11,		0x4,			ANY_VALUE,		0x0,			0x0,			IDX_MTC1 },
{ "CTC1",		0x11,		0x6,			ANY_VALUE,		0x0,			0x0,			IDX_CTC1 },

{ "LWC1",		0x31,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LWC1 },
{ "SWC1",		0x39,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SWC1 },

{ "ABS.S",		0x11,		0x10,			0x0,			ANY_VALUE,		0x5,			IDX_ABS_S },
{ "ADD.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x0,			IDX_ADD_S },
{ "ADDA.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x18,			IDX_ADDA_S },
{ "C.EQ.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x32,			IDX_C_EQ_S },
{ "C.F.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x30,			IDX_C_F_S },
{ "C.LE.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x36,			IDX_C_LE_S },
{ "C.LT.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x34,			IDX_C_LT_S },
{ "CVT.S.W",	0x11,		0x14,			0x0,			ANY_VALUE,		0x20,			IDX_CVT_S_W },
{ "CVT.W.S",	0x11,		0x10,			0x0,			ANY_VALUE,		0x24,			IDX_CVT_W_S },
{ "DIV.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x3,			IDX_DIV_S },
{ "MADD.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x1c,			IDX_MADD_S },
{ "MADDA.S",	0x11,		0x10,			ANY_VALUE,		0x0,			0x1e,			IDX_MADDA_S },
{ "MAX.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x28,			IDX_MAX_S },
{ "MIN.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x29,			IDX_MIN_S },
{ "MOV.S",		0x11,		0x10,			0x0,			ANY_VALUE,		0x6,			IDX_MOV_S },
{ "MSUB.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x1d,			IDX_MSUB_S },
{ "MSUBA.S",	0x11,		0x10,			ANY_VALUE,		0x0,			0x1f,			IDX_MSUBA_S },
{ "MUL.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x2,			IDX_MUL_S },
{ "MULA.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x1a,			IDX_MULA_S },
{ "NEG.S",		0x11,		0x10,			0x0,			ANY_VALUE,		0x7,			IDX_NEG_S },
{ "RSQRT.S",	0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x16,			IDX_RSQRT_S },
{ "SQRT.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x4,			IDX_SQRT_S },
{ "SUB.S",		0x11,		0x10,			ANY_VALUE,		ANY_VALUE,		0x1,			IDX_SUB_S },
{ "SUBA.S",		0x11,		0x10,			ANY_VALUE,		0x0,			0x19,			IDX_SUBA_S },

{ "QMFC2.NI",	0x12,		0x1,			ANY_VALUE,		0x0,			0x0,			IDX_QMFC2_NI },
{ "QMFC2.I",	0x12,		0x1,			ANY_VALUE,		0x0,			0x1,			IDX_QMFC2_I },
{ "QMTC2.NI",	0x12,		0x5,			ANY_VALUE,		0x0,			0x0,			IDX_QMTC2_NI },
{ "QMTC2.I",	0x12,		0x5,			ANY_VALUE,		0x0,			0x1,			IDX_QMTC2_I },
{ "LQC2",		0x36,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_LQC2 },
{ "SQC2",		0x3e,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_SQC2 },


{ "VABS",		0x12,		ANY_VALUE,		ANY_VALUE,		0x7,			0x3d,			IDX_VABS },

{ "VADD",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x28,			IDX_VADD },
{ "VADDi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x22,			IDX_VADDi },
{ "VADDq",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x20,			IDX_VADDq },
{ "VADDX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x00,			IDX_VADDBCX },
{ "VADDY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x01,			IDX_VADDBCY },
{ "VADDZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x02,			IDX_VADDBCZ },
{ "VADDW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x03,			IDX_VADDBCW },

{ "VADDA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0a,			0x3c,			IDX_VADDA },
{ "VADDAi",		0x12,		ANY_VALUE,		ANY_VALUE,		0x08,			0x3e,			IDX_VADDAi },
{ "VADDAq",		0x12,		ANY_VALUE,		ANY_VALUE,		0x08,			0x3c,			IDX_VADDAq },
{ "VADDAX",		0x12,		ANY_VALUE,		ANY_VALUE,		0x00,			0x3c,			IDX_VADDABCX },
{ "VADDAY",		0x12,		ANY_VALUE,		ANY_VALUE,		0x00,			0x3d,			IDX_VADDABCY },
{ "VADDAZ",		0x12,		ANY_VALUE,		ANY_VALUE,		0x00,			0x3e,			IDX_VADDABCZ },
{ "VADDAW",		0x12,		ANY_VALUE,		ANY_VALUE,		0x00,			0x3f,			IDX_VADDABCW },

{ "VCALLMS",	0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x38,			IDX_VCALLMS },
{ "VCALLMSR",	0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x39,			IDX_VCALLMSR },
{ "VCLIP",		0x12,		ANY_VALUE,		ANY_VALUE,		0x07,			0x3f,			IDX_VCLIP },
{ "VDIV",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0e,			0x3c,			IDX_VDIV },

{ "VFTOI0",		0x12,		ANY_VALUE,		ANY_VALUE,		0x05,			0x3c,			IDX_VFTOI0 },
{ "VFTOI4",		0x12,		ANY_VALUE,		ANY_VALUE,		0x05,			0x3d,			IDX_VFTOI4 },
{ "VFTOI12",	0x12,		ANY_VALUE,		ANY_VALUE,		0x05,			0x3e,			IDX_VFTOI12 },
{ "VFTOI15",	0x12,		ANY_VALUE,		ANY_VALUE,		0x05,			0x3f,			IDX_VFTOI15 },

{ "VIADD",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x30,			IDX_VIADD },
{ "VIADDI",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x32,			IDX_VIADDI },
{ "VIAND",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x34,			IDX_VIAND },
{ "VILWR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0f,			0x3e,			IDX_VILWR },
{ "VIOR",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x35,			IDX_VIOR },
{ "VISUB",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x31,			IDX_VISUB },
{ "VISWR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0f,			0x3f,			IDX_VISWR },

{ "VITOF0",		0x12,		ANY_VALUE,		ANY_VALUE,		0x04,			0x3c,			IDX_VITOF0 },
{ "VITOF4",		0x12,		ANY_VALUE,		ANY_VALUE,		0x04,			0x3d,			IDX_VITOF4 },
{ "VITOF12",	0x12,		ANY_VALUE,		ANY_VALUE,		0x04,			0x3e,			IDX_VITOF12 },
{ "VITOF15",	0x12,		ANY_VALUE,		ANY_VALUE,		0x04,			0x3f,			IDX_VITOF15 },

{ "VLQD",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0d,			0x3e,			IDX_VLQD },
{ "VLQI",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0d,			0x3c,			IDX_VLQI },

{ "VMADD",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x29,			IDX_VMADD },
{ "VMADDi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x23,			IDX_VMADDi },
{ "VMADDq",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x21,			IDX_VMADDq },
{ "VMADDX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x08,			IDX_VMADDBCX },
{ "VMADDY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x09,			IDX_VMADDBCY },
{ "VMADDZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0a,			IDX_VMADDBCZ },
{ "VMADDW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0b,			IDX_VMADDBCW },

{ "VMADDA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0a,			0x3d,			IDX_VMADDA },
{ "VMADDAi",	0x12,		ANY_VALUE,		ANY_VALUE,		0x08,			0x3f,			IDX_VMADDAi },
{ "VMADDAq",	0x12,		ANY_VALUE,		ANY_VALUE,		0x08,			0x3d,			IDX_VMADDAq },
{ "VMADDAX",	0x12,		ANY_VALUE,		ANY_VALUE,		0x02,			0x3c,			IDX_VMADDABCX },
{ "VMADDAY",	0x12,		ANY_VALUE,		ANY_VALUE,		0x02,			0x3d,			IDX_VMADDABCY },
{ "VMADDAZ",	0x12,		ANY_VALUE,		ANY_VALUE,		0x02,			0x3e,			IDX_VMADDABCZ },
{ "VMADDAW",	0x12,		ANY_VALUE,		ANY_VALUE,		0x02,			0x3f,			IDX_VMADDABCW },

{ "VMAX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2b,			IDX_VMAX },
{ "VMAXi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1d,			IDX_VMAXi },
{ "VMAXX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x10,			IDX_VMAXBCX },
{ "VMAXY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x11,			IDX_VMAXBCY },
{ "VMAXZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x12,			IDX_VMAXBCZ },
{ "VMAXW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x13,			IDX_VMAXBCW },

{ "VMFIR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0f,			0x3d,			IDX_VMFIR },

{ "VMINI",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2f,			IDX_VMINI },
{ "VMINIi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1f,			IDX_VMINIi },
{ "VMINIX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x14,			IDX_VMINIBCX },
{ "VMINIY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x15,			IDX_VMINIBCY },
{ "VMINIZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x16,			IDX_VMINIBCZ },
{ "VMINIW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x17,			IDX_VMINIBCW },

{ "VMOVE",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0c,			0x3c,			IDX_VMOVE },
{ "VMR32",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0c,			0x3d,			IDX_VMR32 },

{ "VMSUB",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2d,			IDX_VMSUB },
{ "VMSUBi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x27,			IDX_VMSUBi },
{ "VMSUBq",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x25,			IDX_VMSUBq },
{ "VMSUBX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0c,			IDX_VMSUBBCX },
{ "VMSUBY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0d,			IDX_VMSUBBCY },
{ "VMSUBZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0e,			IDX_VMSUBBCZ },
{ "VMSUBW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x0f,			IDX_VMSUBBCW },

{ "VMSUBA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0b,			0x3d,			IDX_VMSUBA },
{ "VMSUBAi",	0x12,		ANY_VALUE,		ANY_VALUE,		0x09,			0x3f,			IDX_VMSUBAi },
{ "VMSUBAq",	0x12,		ANY_VALUE,		ANY_VALUE,		0x09,			0x3d,			IDX_VMSUBAq },
{ "VMSUBAX",	0x12,		ANY_VALUE,		ANY_VALUE,		0x03,			0x3c,			IDX_VMSUBABCX },
{ "VMSUBAY",	0x12,		ANY_VALUE,		ANY_VALUE,		0x03,			0x3d,			IDX_VMSUBABCY },
{ "VMSUBAZ",	0x12,		ANY_VALUE,		ANY_VALUE,		0x03,			0x3e,			IDX_VMSUBABCZ },
{ "VMSUBAW",	0x12,		ANY_VALUE,		ANY_VALUE,		0x03,			0x3f,			IDX_VMSUBABCW },

{ "VMTIR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0f,			0x3c,			IDX_VMTIR },

{ "VMUL",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2a,			IDX_VMUL },
{ "VMULi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1e,			IDX_VMULi },
{ "VMULq",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1c,			IDX_VMULq },
{ "VMULX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x18,			IDX_VMULBCX },
{ "VMULY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x19,			IDX_VMULBCY },
{ "VMULZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1a,			IDX_VMULBCZ },
{ "VMULW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x1b,			IDX_VMULBCW },

{ "VMULA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0a,			0x3e,			IDX_VMULA },
{ "VMULAi",		0x12,		ANY_VALUE,		ANY_VALUE,		0x07,			0x3e,			IDX_VMULAi },
{ "VMULAq",		0x12,		ANY_VALUE,		ANY_VALUE,		0x07,			0x3c,			IDX_VMULAq },
{ "VMULAX",		0x12,		ANY_VALUE,		ANY_VALUE,		0x06,			0x3c,			IDX_VMULABCX },
{ "VMULAY",		0x12,		ANY_VALUE,		ANY_VALUE,		0x06,			0x3d,			IDX_VMULABCY },
{ "VMULAZ",		0x12,		ANY_VALUE,		ANY_VALUE,		0x06,			0x3e,			IDX_VMULABCZ },
{ "VMULAW",		0x12,		ANY_VALUE,		ANY_VALUE,		0x06,			0x3f,			IDX_VMULABCW },

{ "VNOP",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0b,			0x3f,			IDX_VNOP },
{ "VOPMULA",	0x12,		ANY_VALUE,		ANY_VALUE,		0x0b,			0x3e,			IDX_VOPMULA },
{ "VOPMSUB",	0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2e,			IDX_VOPMSUB },
{ "VRGET",		0x12,		ANY_VALUE,		ANY_VALUE,		0x10,			0x3d,			IDX_VRGET },
{ "VRINIT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x10,			0x3e,			IDX_VRINIT },
{ "VRNEXT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x10,			0x3c,			IDX_VRNEXT },
{ "VRSQRT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0e,			0x3e,			IDX_VRSQRT },
{ "VRXOR",		0x12,		ANY_VALUE,		ANY_VALUE,		0x10,			0x3f,			IDX_VRXOR },
{ "VSQD",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0d,			0x3f,			IDX_VSQD },
{ "VSQI",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0d,			0x3d,			IDX_VSQI },
{ "VSQRT",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0e,			0x3d,			IDX_VSQRT },

{ "VSUB",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x2c,			IDX_VSUB },
{ "VSUBi",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x26,			IDX_VSUBi },
{ "VSUBq",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x24,			IDX_VSUBq },
{ "VSUBX",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x04,			IDX_VSUBBCX },
{ "VSUBY",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x05,			IDX_VSUBBCY },
{ "VSUBZ",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x06,			IDX_VSUBBCZ },
{ "VSUBW",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		0x07,			IDX_VSUBBCW },

{ "VSUBA",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0b,			0x3c,			IDX_VSUBA },
{ "VSUBAi",		0x12,		ANY_VALUE,		ANY_VALUE,		0x09,			0x3e,			IDX_VSUBAi },
{ "VSUBAq",		0x12,		ANY_VALUE,		ANY_VALUE,		0x09,			0x3c,			IDX_VSUBAq },
{ "VSUBAX",		0x12,		ANY_VALUE,		ANY_VALUE,		0x01,			0x3c,			IDX_VSUBABCX },
{ "VSUBAY",		0x12,		ANY_VALUE,		ANY_VALUE,		0x01,			0x3d,			IDX_VSUBABCY },
{ "VSUBAZ",		0x12,		ANY_VALUE,		ANY_VALUE,		0x01,			0x3e,			IDX_VSUBABCZ },
{ "VSUBAW",		0x12,		ANY_VALUE,		ANY_VALUE,		0x01,			0x3f,			IDX_VSUBABCW },

{ "VWAITQ",		0x12,		ANY_VALUE,		ANY_VALUE,		0x0e,			0x3f,			IDX_VWAITQ },

{ "COP2",		0x12,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_COP2 },

//{ "INVALID",	ANY_VALUE,	ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		ANY_VALUE,		IDX_Invalid }

};


// returns -1 on error
int Lookup::FindByName ( string NameOfInstruction )
{
	bool found;
	string Line_NoComment;
	string sInst;
	int StartPos, EndPos;
	
	// get rid of any comments
	//cout << "hasInstruction: removing comment from line. Line=" << NameOfInstruction.c_str() << "\n";
	//Line_NoComment = removeLabel( removeComment ( NameOfInstruction ) );
	
	// remove whitespace
	//cout << "hasInstruction: trimming line. Line_NoComment=" << NameOfInstruction.c_str() << "\n";
	Line_NoComment = Trim ( NameOfInstruction );
	
	// convert to upper case
	//cout << "hasInstruction: converting line to uppercase. Line_NoComment=" << Line_NoComment << "\n";
	Line_NoComment = UCase ( Line_NoComment );
	
	// get the instruction
	//cout << "hasInstruction: Getting the instruction.\n";
	//cout << "hasInstruction: Getting StartPos. Line_NoComment=" << Line_NoComment << "\n";
	StartPos = Line_NoComment.find_first_not_of ( " \r\n\t" );
	
	//cout << "StartPos=" << StartPos << "\n";
	
	if ( StartPos != string::npos )
	{
		//cout << "hasInstruction: Getting EndPos.\n";
		EndPos = Line_NoComment.find_first_of ( " \r\n\t", StartPos );
		
		if ( EndPos == string::npos ) EndPos = Line_NoComment.length();
		
		//cout << "hasInstruction: Getting just the instruction. EndPos=" << EndPos << "\n";
		sInst = Line_NoComment.substr( StartPos, EndPos - StartPos );
		
#ifdef INLINE_DEBUG
		debug << "hasInstruction: Instruction=" << sInst.c_str () << "\n";
#endif
		
		//cout << "hasInstruction: Instruction=" << sInst.c_str () << "\n";
		//cout << "\nnum instructions = " << dec << ( sizeof( Entries ) / sizeof( Entries [ 0 ] ) );
		
		// see if we find the instruction in the list
		for ( int i = 0; i < ( sizeof( Entries ) / sizeof( Entries [ 0 ] ) ); i++ )
		{
			if ( !sInst.compare( Entries [ i ].Name ) )
			{
				//cout << "\nfound instruction: " << sInst.c_str() << "\n";
				return Entries [ i ].InstructionIndex;
			}
		}
	}
	
	//cout << "\ndid not find instruction: " << NameOfInstruction;
	return -1;
}



//Debug::Log Lookup::debug;

#ifdef ENABLE_R5900_LOOKUP_TABLE

void Lookup::Start ()
{
	u32 Opcode, Rs, Rt, Shft, Funct, Index, ElementsInExecute, ElementsInBranchLoad1;
	Instruction::Format i;
	
	u32 ulCounter, ulRemainder;

	cout << "Running constructor for R5900::Lookup class.\n";
	
	if ( c_bObjectInitialized ) return;

#ifdef INLINE_DEBUG_ENABLE	
	debug.Create ( "R5900_Lookup_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "Running constructor for R5900::Lookup class.\r\n";
#endif


	//LookupTable = new unsigned short[c_iLookupTable_Size];
	LookupTable = (unsigned short*)malloc(c_iLookupTable_Size);

	
	ElementsInExecute = (sizeof(Entries) / sizeof(Entries[0]));
	
	// clear table first
	cout << "\nSize of R5900 lookup table in bytes=" << dec << sizeof( LookupTable );
	for ( Index = 0; Index < (c_iLookupTable_Size >> 3 ); Index++ ) ((u64*)LookupTable) [ Index ] = 0;
	
	for ( Index = ElementsInExecute - 1; Index < ElementsInExecute; Index-- )
	{
		ulRemainder = 0;
		
		Opcode = Entries [ Index ].Opcode;
		Rs = Entries [ Index ].Rs;
		Rt = Entries [ Index ].Rt;
		Shft = Entries [ Index ].Shift;
		Funct = Entries [ Index ].Funct;
		
		i.Opcode = Opcode;
		i.Rs = Rs;
		i.Rt = Rt;
		i.Shift = Shft;
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
			
			if ( Shft == 0xff )
			{
				// 5 bits
				i.Shift = ulRemainder & 0x1f;
				ulRemainder >>= 5;
			}
			
			if ( Funct == 0xff )
			{
				// 6 bits
				i.Funct = ulRemainder & 0x3f;
				ulRemainder >>= 6;
			}
			
			LookupTable [ ( ( i.Value >> 16 ) | ( i.Value << 16 ) ) & c_iLookupTable_Mask ] = Entries [ Index ].InstructionIndex;

			//if ( )
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


u16 Lookup::FindByInstruction(u32 instruction)
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

		case 0x0a:
			return IDX_MOVZ;
			break;
		case 0x0b:
			return IDX_MOVN;
			break;

		case 0x0c:
			return IDX_SYSCALL;
			break;

		case 0x0d:
			return IDX_BREAK;
			break;

		case 0x0f:
			return IDX_SYNC;
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

		case 0x14:
			return IDX_DSLLV;
			break;
		case 0x16:
			return IDX_DSRLV;
			break;
		case 0x17:
			return IDX_DSRAV;
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

		case 0x28:
			return IDX_MFSA;
			break;
		case 0x29:
			return IDX_MTSA;
			break;

		case 0x2a:
			return IDX_SLT;
			break;

		case 0x2b:
			return IDX_SLTU;
			break;

		case 0x2c:
			return IDX_DADD;
			break;
		case 0x2d:
			return IDX_DADDU;
			break;
		case 0x2e:
			return IDX_DSUB;
			break;
		case 0x2f:
			return IDX_DSUBU;
			break;

		case 0x30:
			return IDX_TGE;
			break;
		case 0x31:
			return IDX_TGEU;
			break;
		case 0x32:
			return IDX_TLT;
			break;
		case 0x33:
			return IDX_TLTU;
			break;
		case 0x34:
			return IDX_TEQ;
			break;
		case 0x36:
			return IDX_TNE;
			break;

		case 0x38:
			return IDX_DSLL;
			break;
		case 0x3a:
			return IDX_DSRL;
			break;
		case 0x3b:
			return IDX_DSRA;
			break;
		case 0x3c:
			return IDX_DSLL32;
			break;
		case 0x3e:
			return IDX_DSRL32;
			break;
		case 0x3f:
			return IDX_DSRA32;
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
		case 0x02:
			return IDX_BLTZL;
			break;
		case 0x03:
			return IDX_BGEZL;
			break;

		case 0x08:
			return IDX_TGEI;
			break;
		case 0x09:
			return IDX_TGEIU;
			break;
		case 0x0a:
			return IDX_TLTI;
			break;
		case 0x0b:
			return IDX_TLTIU;
			break;
		case 0x0c:
			return IDX_TEQI;
			break;
		case 0x0e:
			return IDX_TNEI;
			break;

		case 0x10:
			return IDX_BLTZAL;
			break;
		case 0x11:
			return IDX_BGEZAL;
			break;
		case 0x12:
			return IDX_BLTZALL;
			break;
		case 0x13:
			return IDX_BGEZALL;
			break;

		case 0x18:
			return IDX_MTSAB;
			break;
		case 0x19:
			return IDX_MTSAH;
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

		case 0x08:
			switch (i.Rt)
			{
			case 0x00:
				return IDX_BC0F;
				break;
			case 0x01:
				return IDX_BC0T;
				break;
			case 0x02:
				return IDX_BC0FL;
				break;
			case 0x03:
				return IDX_BC0TL;
				break;
			}
			break;

		case 0x10:
			switch (i.Funct)
			{
			case 0x01:
				return IDX_TLBR;
				break;
			case 0x02:
				return IDX_TLBWI;
				break;
			case 0x06:
				return IDX_TLBWR;
				break;
			case 0x08:
				return IDX_TLBP;
				break;
			case 0x18:
				return IDX_ERET;
				break;
			case 0x38:
				return IDX_EI;
				break;
			case 0x39:
				return IDX_DI;
				break;
			}
			break;
		}
		break;

	case 0x11:
		switch (i.Rs)
		{
		case 0x00:
			//if (!(i.Shift | i.Funct)) return IDX_MFC1;
			return IDX_MFC1;
			break;
		case 0x02:
			//if (!(i.Shift | i.Funct)) return IDX_CFC1;
			return IDX_CFC1;
			break;
		case 0x04:
			//if (!(i.Shift | i.Funct)) return IDX_MTC1;
			return IDX_MTC1;
			break;
		case 0x06:
			//if (!(i.Shift | i.Funct)) return IDX_CTC1;
			return IDX_CTC1;
			break;
		case 0x08:
			switch (i.Rt)
			{
			case 0x00:
				return IDX_BC1F;
				break;
			case 0x01:
				return IDX_BC1T;
				break;
			case 0x02:
				return IDX_BC1FL;
				break;
			case 0x03:
				return IDX_BC1TL;
				break;
			}
			break;
		case 0x10:
			switch (i.Funct)
			{
			case 0x00:
				return IDX_ADD_S;
				break;
			case 0x01:
				return IDX_SUB_S;
				break;
			case 0x02:
				return IDX_MUL_S;
				break;
			case 0x03:
				return IDX_DIV_S;
				break;
			case 0x04:
				return IDX_SQRT_S;
				break;
			case 0x05:
				//if(!(i.Rt)) return IDX_ABS_S;
				return IDX_ABS_S;
				break;
			case 0x06:
				//if (!(i.Rt)) return IDX_MOV_S;
				return IDX_MOV_S;
				break;
			case 0x07:
				//if (!(i.Rt)) return IDX_NEG_S;
				return IDX_NEG_S;
				break;
			case 0x16:
				return IDX_RSQRT_S;
				break;
			case 0x18:
				//if (!(i.Shift)) return IDX_ADDA_S;
				return IDX_ADDA_S;
				break;
			case 0x19:
				//if (!(i.Shift)) return IDX_SUBA_S;
				return IDX_SUBA_S;
				break;
			case 0x1a:
				//if (!(i.Shift)) return IDX_MULA_S;
				return IDX_MULA_S;
				break;
			case 0x1c:
				return IDX_MADD_S;
				break;
			case 0x1d:
				return IDX_MSUB_S;
				break;
			case 0x1e:
				//if (!(i.Shift)) return IDX_MADDA_S;
				return IDX_MADDA_S;
				break;
			case 0x1f:
				//if (!(i.Shift)) return IDX_MSUBA_S;
				return IDX_MSUBA_S;
				break;
			case 0x24:
				//if (!(i.Rt)) return IDX_CVT_W_S;
				return IDX_CVT_W_S;
				break;
			case 0x28:
				return IDX_MAX_S;
				break;
			case 0x29:
				return IDX_MIN_S;
				break;
			case 0x30:
				//if (!(i.Shift)) return IDX_C_F_S;
				return IDX_C_F_S;
				break;
			case 0x32:
				//if (!(i.Shift)) return IDX_C_EQ_S;
				return IDX_C_EQ_S;
				break;
			case 0x34:
				//if (!(i.Shift)) return IDX_C_LT_S;
				return IDX_C_LT_S;
				break;
			case 0x36:
				//if (!(i.Shift)) return IDX_C_LE_S;
				return IDX_C_LE_S;
				break;
			}
			break;

		case 0x14:
			switch (i.Funct)
			{
			case 0x20:
				//if (!(i.Rt)) return IDX_CVT_S_W;
				return IDX_CVT_S_W;
				break;
			}
			break;
		}
		break;

	case 0x12:
		switch (i.Rs)
		{
		case 0x01:
			//if ((!i.Shift) && (!i.Funct)) return IDX_QMFC2_NI;
			//if ((!i.Shift) && (i.Funct == 1)) return IDX_QMFC2_I;
			if (i.Funct & 1) return IDX_QMFC2_I; else return IDX_QMFC2_NI;
			break;
		case 0x02:
			//if ((!i.Funct)) return IDX_CFC2_NI;
			//if ((i.Funct == 1)) return IDX_CFC2_I;
			if (i.Funct & 1) return IDX_CFC2_I; else return IDX_CFC2_NI;
			break;
		case 0x05:
			//if ((!i.Shift) && (!i.Funct)) return IDX_QMTC2_NI;
			//if ((!i.Shift) && (i.Funct == 1)) return IDX_QMTC2_I;
			if (i.Funct & 1) return IDX_QMTC2_I; else return IDX_QMTC2_NI;
			break;
		case 0x06:
			//if ((!i.Funct)) return IDX_CTC2_NI;
			//if ((i.Funct == 1)) return IDX_CTC2_I;
			if (i.Funct & 1) return IDX_CTC2_I; else return IDX_CTC2_NI;
			break;
		case 0x08:
			switch (i.Rt)
			{
			case 0x00:
				return IDX_BC2F;
				break;
			case 0x01:
				return IDX_BC2T;
				break;
			case 0x02:
				return IDX_BC2FL;
				break;
			case 0x03:
				return IDX_BC2TL;
				break;
			}
			break;
		}
		if ((i.Rs & 0x10) == 0x10)
		{
			switch (i.Funct)
			{
			case 0x00:
				return IDX_VADDBCX;
				break;
			case 0x01:
				return IDX_VADDBCY;
				break;
			case 0x02:
				return IDX_VADDBCZ;
				break;
			case 0x03:
				return IDX_VADDBCW;
				break;

			case 0x04:
				return IDX_VSUBBCX;
				break;
			case 0x05:
				return IDX_VSUBBCY;
				break;
			case 0x06:
				return IDX_VSUBBCZ;
				break;
			case 0x07:
				return IDX_VSUBBCW;
				break;

			case 0x08:
				return IDX_VMADDBCX;
				break;
			case 0x09:
				return IDX_VMADDBCY;
				break;
			case 0x0a:
				return IDX_VMADDBCZ;
				break;
			case 0x0b:
				return IDX_VMADDBCW;
				break;

			case 0x0c:
				return IDX_VMSUBBCX;
				break;
			case 0x0d:
				return IDX_VMSUBBCY;
				break;
			case 0x0e:
				return IDX_VMSUBBCZ;
				break;
			case 0x0f:
				return IDX_VMSUBBCW;
				break;

			case 0x10:
				return IDX_VMAXBCX;
				break;
			case 0x11:
				return IDX_VMAXBCY;
				break;
			case 0x12:
				return IDX_VMAXBCZ;
				break;
			case 0x13:
				return IDX_VMAXBCW;
				break;

			case 0x14:
				return IDX_VMINIBCX;
				break;
			case 0x15:
				return IDX_VMINIBCY;
				break;
			case 0x16:
				return IDX_VMINIBCZ;
				break;
			case 0x17:
				return IDX_VMINIBCW;
				break;

			case 0x18:
				return IDX_VMULBCX;
				break;
			case 0x19:
				return IDX_VMULBCY;
				break;
			case 0x1a:
				return IDX_VMULBCZ;
				break;
			case 0x1b:
				return IDX_VMULBCW;
				break;

			case 0x1c:
				return IDX_VMULq;
				break;
			case 0x1d:
				return IDX_VMAXi;
				break;
			case 0x1e:
				return IDX_VMULi;
				break;
			case 0x1f:
				return IDX_VMINIi;
				break;

			case 0x20:
				return IDX_VADDq;
				break;
			case 0x21:
				return IDX_VMADDq;
				break;
			case 0x22:
				return IDX_VADDi;
				break;
			case 0x23:
				return IDX_VMADDi;
				break;

			case 0x24:
				return IDX_VSUBq;
				break;
			case 0x25:
				return IDX_VMSUBq;
				break;
			case 0x26:
				return IDX_VSUBi;
				break;
			case 0x27:
				return IDX_VMSUBi;
				break;

			case 0x28:
				return IDX_VADD;
				break;
			case 0x29:
				return IDX_VMADD;
				break;
			case 0x2a:
				return IDX_VMUL;
				break;
			case 0x2b:
				return IDX_VMAX;
				break;

			case 0x2c:
				return IDX_VSUB;
				break;
			case 0x2d:
				return IDX_VMSUB;
				break;
			case 0x2e:
				return IDX_VOPMSUB;
				break;
			case 0x2f:
				return IDX_VMINI;
				break;

			case 0x30:
				return IDX_VIADD;
				break;
			case 0x31:
				return IDX_VISUB;
				break;
			case 0x32:
				return IDX_VIADDI;
				break;
			case 0x33:
				break;

			case 0x34:
				return IDX_VIAND;
				break;
			case 0x35:
				return IDX_VIOR;
				break;
			case 0x36:
				break;
			case 0x37:
				break;

			case 0x38:
				return IDX_VCALLMS;
				break;
			case 0x39:
				return IDX_VCALLMSR;
				break;
			case 0x3a:
				break;
			case 0x3b:
				break;

			case 0x3c:
				switch (i.Shift)
				{
				case 0x00:
					return IDX_VADDABCX;
					break;
				case 0x01:
					return IDX_VSUBABCX;
					break;
				case 0x02:
					return IDX_VMADDABCX;
					break;
				case 0x03:
					return IDX_VMSUBABCX;
					break;
				case 0x04:
					return IDX_VITOF0;
					break;
				case 0x05:
					return IDX_VFTOI0;
					break;
				case 0x06:
					return IDX_VMULABCX;
					break;
				case 0x07:
					return IDX_VMULAq;
					break;
				case 0x08:
					return IDX_VADDAq;
					break;
				case 0x09:
					return IDX_VSUBAq;
					break;
				case 0x0a:
					return IDX_VADDA;
					break;
				case 0x0b:
					return IDX_VSUBA;
					break;
				case 0x0c:
					return IDX_VMOVE;
					break;
				case 0x0d:
					return IDX_VLQI;
					break;
				case 0x0e:
					return IDX_VDIV;
					break;
				case 0x0f:
					return IDX_VMTIR;
					break;
				case 0x10:
					return IDX_VRNEXT;
					break;
				}
				break;
			case 0x3d:
				switch (i.Shift)
				{
				case 0x00:
					return IDX_VADDABCY;
					break;
				case 0x01:
					return IDX_VSUBABCY;
					break;
				case 0x02:
					return IDX_VMADDABCY;
					break;
				case 0x03:
					return IDX_VMSUBABCY;
					break;
				case 0x04:
					return IDX_VITOF4;
					break;
				case 0x05:
					return IDX_VFTOI4;
					break;
				case 0x06:
					return IDX_VMULABCY;
					break;
				case 0x07:
					return IDX_VABS;
					break;
				case 0x08:
					return IDX_VMADDAq;
					break;
				case 0x09:
					return IDX_VMSUBAq;
					break;
				case 0x0a:
					return IDX_VMADDA;
					break;
				case 0x0b:
					return IDX_VMSUBA;
					break;
				case 0x0c:
					return IDX_VMR32;
					break;
				case 0x0d:
					return IDX_VSQI;
					break;
				case 0x0e:
					return IDX_VSQRT;
					break;
				case 0x0f:
					return IDX_VMFIR;
					break;
				case 0x10:
					return IDX_VRGET;
					break;
				}
				break;
			case 0x3e:
				switch (i.Shift)
				{
				case 0x00:
					return IDX_VADDABCZ;
					break;
				case 0x01:
					return IDX_VSUBABCZ;
					break;
				case 0x02:
					return IDX_VMADDABCZ;
					break;
				case 0x03:
					return IDX_VMSUBABCZ;
					break;
				case 0x04:
					return IDX_VITOF12;
					break;
				case 0x05:
					return IDX_VFTOI12;
					break;
				case 0x06:
					return IDX_VMULABCZ;
					break;
				case 0x07:
					return IDX_VMULAi;
					break;
				case 0x08:
					return IDX_VADDAi;
					break;
				case 0x09:
					return IDX_VSUBAi;
					break;
				case 0x0a:
					return IDX_VMULA;
					break;
				case 0x0b:
					return IDX_VOPMULA;
					break;
				case 0x0d:
					return IDX_VLQD;
					break;
				case 0x0e:
					return IDX_VRSQRT;
					break;
				case 0x0f:
					return IDX_VILWR;
					break;
				case 0x10:
					return IDX_VRINIT;
					break;
				}
				break;
			case 0x3f:
				switch (i.Shift)
				{
				case 0x00:
					return IDX_VADDABCW;
					break;
				case 0x01:
					return IDX_VSUBABCW;
					break;
				case 0x02:
					return IDX_VMADDABCW;
					break;
				case 0x03:
					return IDX_VMSUBABCW;
					break;
				case 0x04:
					return IDX_VITOF15;
					break;
				case 0x05:
					return IDX_VFTOI15;
					break;
				case 0x06:
					return IDX_VMULABCW;
					break;
				case 0x07:
					return IDX_VCLIP;
					break;
				case 0x08:
					return IDX_VMADDAi;
					break;
				case 0x09:
					return IDX_VMSUBAi;
					break;
				case 0x0b:
					return IDX_VNOP;
					break;
				case 0x0d:
					return IDX_VSQD;
					break;
				case 0x0e:
					return IDX_VWAITQ;
					break;
				case 0x0f:
					return IDX_VISWR;
					break;
				case 0x10:
					return IDX_VRXOR;
					break;
				}
				break;
			}
		}
		break;

	case 0x14:
		return IDX_BEQL;
		break;
	case 0x15:
		return IDX_BNEL;
		break;
	case 0x16:
		return IDX_BLEZL;
		break;
	case 0x17:
		return IDX_BGTZL;
		break;

	case 0x18:
		return IDX_DADDI;
		break;
	case 0x19:
		return IDX_DADDIU;
		break;
	case 0x1a:
		return IDX_LDL;
		break;
	case 0x1b:
		return IDX_LDR;
		break;

	case 0x1c:
		switch (i.Funct)
		{
		case 0x00:
			return IDX_MADD;
			break;
		case 0x01:
			return IDX_MADDU;
			break;

		case 0x04:
			return IDX_PLZCW;
			break;

		case 0x08:
			switch (i.Shift)
			{
			case 0x00:
				return IDX_PADDW;
				break;
			case 0x01:
				return IDX_PSUBW;
				break;
			case 0x02:
				return IDX_PCGTW;
				break;
			case 0x03:
				return IDX_PMAXW;
				break;
			case 0x04:
				return IDX_PADDH;
				break;
			case 0x05:
				return IDX_PSUBH;
				break;
			case 0x06:
				return IDX_PCGTH;
				break;
			case 0x07:
				return IDX_PMAXH;
				break;
			case 0x08:
				return IDX_PADDB;
				break;
			case 0x09:
				return IDX_PSUBB;
				break;
			case 0x0a:
				return IDX_PCGTB;
				break;
			case 0x10:
				return IDX_PADDSW;
				break;
			case 0x11:
				return IDX_PSUBSW;
				break;
			case 0x12:
				return IDX_PEXTLW;
				break;
			case 0x13:
				return IDX_PPACW;
				break;
			case 0x14:
				return IDX_PADDSH;
				break;
			case 0x15:
				return IDX_PSUBSH;
				break;
			case 0x16:
				return IDX_PEXTLH;
				break;
			case 0x17:
				return IDX_PPACH;
				break;
			case 0x18:
				return IDX_PADDSB;
				break;
			case 0x19:
				return IDX_PSUBSB;
				break;
			case 0x1a:
				return IDX_PEXTLB;
				break;
			case 0x1b:
				return IDX_PPACB;
				break;

			case 0x1e:
				return IDX_PEXT5;
				break;
			case 0x1f:
				return IDX_PPAC5;
				break;
			}
			break;
		case 0x09:
			switch (i.Shift)
			{
			case 0x00:
				return IDX_PMADDW;
				break;

			case 0x02:
				return IDX_PSLLVW;
				break;
			case 0x03:
				return IDX_PSRLVW;
				break;
			case 0x04:
				return IDX_PMSUBW;
				break;

			case 0x08:
				return IDX_PMFHI;
				break;
			case 0x09:
				return IDX_PMFLO;
				break;
			case 0x0a:
				return IDX_PINTH;
				break;

			case 0x0c:
				return IDX_PMULTW;
				break;
			case 0x0d:
				return IDX_PDIVW;
				break;
			case 0x0e:
				return IDX_PCPYLD;
				break;

			case 0x10:
				return IDX_PMADDH;
				break;
			case 0x11:
				return IDX_PHMADH;
				break;
			case 0x12:
				return IDX_PAND;
				break;
			case 0x13:
				return IDX_PXOR;
				break;
			case 0x14:
				return IDX_PMSUBH;
				break;
			case 0x15:
				return IDX_PHMSBH;
				break;

			case 0x1a:
				return IDX_PEXEH;
				break;
			case 0x1b:
				return IDX_PREVH;
				break;
			case 0x1c:
				return IDX_PMULTH;
				break;
			case 0x1d:
				return IDX_PDIVBW;
				break;
			case 0x1e:
				return IDX_PEXEW;
				break;
			case 0x1f:
				return IDX_PROT3W;
				break;
			}
			break;

		case 0x10:
			return IDX_MFHI1;
			break;
		case 0x11:
			return IDX_MTHI1;
			break;
		case 0x12:
			return IDX_MFLO1;
			break;
		case 0x13:
			return IDX_MTLO1;
			break;

		case 0x18:
			return IDX_MULT1;
			break;
		case 0x19:
			return IDX_MULTU1;
			break;
		case 0x1a:
			return IDX_DIV1;
			break;
		case 0x1b:
			return IDX_DIVU1;
			break;

		case 0x20:
			return IDX_MADD1;
			break;
		case 0x21:
			return IDX_MADDU1;
			break;

		case 0x28:
			switch (i.Shift)
			{
			case 0x01:
				return IDX_PABSW;
				break;
			case 0x02:
				return IDX_PCEQW;
				break;
			case 0x03:
				return IDX_PMINW;
				break;
			case 0x04:
				return IDX_PADSBH;
				break;
			case 0x05:
				return IDX_PABSH;
				break;
			case 0x06:
				return IDX_PCEQH;
				break;
			case 0x07:
				return IDX_PMINH;
				break;

			case 0x0a:
				return IDX_PCEQB;
				break;

			case 0x10:
				return IDX_PADDUW;
				break;
			case 0x11:
				return IDX_PSUBUW;
				break;
			case 0x12:
				return IDX_PEXTUW;
				break;

			case 0x14:
				return IDX_PADDUH;
				break;
			case 0x15:
				return IDX_PSUBUH;
				break;
			case 0x16:
				return IDX_PEXTUH;
				break;

			case 0x18:
				return IDX_PADDUB;
				break;
			case 0x19:
				return IDX_PSUBUB;
				break;
			case 0x1a:
				return IDX_PEXTUB;
				break;
			case 0x1b:
				return IDX_QFSRV;
				break;
			}
			break;
		case 0x29:
			switch (i.Shift)
			{
			case 0x00:
				return IDX_PMADDUW;
				break;

			case 0x03:
				return IDX_PSRAVW;
				break;

			case 0x08:
				return IDX_PMTHI;
				break;
			case 0x09:
				return IDX_PMTLO;
				break;
			case 0x0a:
				return IDX_PINTEH;
				break;

			case 0x0c:
				return IDX_PMULTUW;
				break;
			case 0x0d:
				return IDX_PDIVUW;
				break;
			case 0x0e:
				return IDX_PCPYUD;
				break;

			case 0x12:
				return IDX_POR;
				break;
			case 0x13:
				return IDX_PNOR;
				break;

			case 0x1a:
				return IDX_PEXCH;
				break;
			case 0x1b:
				return IDX_PCPYH;
				break;

			case 0x1e:
				return IDX_PEXCW;
				break;
			}
			break;
		case 0x30:
			switch (i.Shift)
			{
			case 0x00:
				return IDX_PMFHL_LW;
				break;
			case 0x01:
				return IDX_PMFHL_UW;
				break;
			case 0x02:
				return IDX_PMFHL_SLW;
				break;
			case 0x03:
				return IDX_PMFHL_LH;
				break;
			case 0x04:
				return IDX_PMFHL_SH;
				break;
			}
			break;
		case 0x31:
			if (!i.Shift)
			{
				return IDX_PMTHL_LW;
			}
			break;
		case 0x34:
			return IDX_PSLLH;
			break;
		case 0x36:
			return IDX_PSRLH;
			break;
		case 0x37:
			return IDX_PSRAH;
			break;
		case 0x3c:
			return IDX_PSLLW;
			break;
		case 0x3e:
			return IDX_PSRLW;
			break;
		case 0x3f:
			return IDX_PSRAW;
			break;
		}
		break;

	case 0x1e:
		return IDX_LQ;
		break;
	case 0x1f:
		return IDX_SQ;
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
	case 0x27:
		return IDX_LWU;
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
	case 0x2c:
		return IDX_SDL;
		break;
	case 0x2d:
		return IDX_SDR;
		break;
	case 0x2e:
		return IDX_SWR;
		break;
	case 0x2f:
		return IDX_CACHE;
		break;

	case 0x31:
		return IDX_LWC1;
		break;

	case 0x33:
		return IDX_PREF;
		break;

	case 0x36:
		return IDX_LQC2;
		break;
	case 0x37:
		return IDX_LD;
		break;

	case 0x39:
		return IDX_SWC1;
		break;

	case 0x3e:
		return IDX_SQC2;
		break;
	case 0x3f:
		return IDX_SD;
		break;
	}

	return IDX_INVALID;
}

