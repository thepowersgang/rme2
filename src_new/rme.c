/*
 * Realmode Emulator Plugin
 * - By John Hodge (thePowersGang)
 *
 * This code is published under the FreeBSD licence
 * (See the file COPYING for details)
 *
 * ---
 * Core Emulator
 */
#define _RME_C_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

//#include <common.h>

typedef uint16_t	Uint16;
typedef int16_t	Sint16;
typedef int8_t	Sint8;

#include "rme.h"

// Settings
#define RME_DO_NULL_CHECK	1
#define	printf	printf	// Formatted print function

// -- Per Compiler macros
#define	WARN_UNUSED_RET	__attribute__((warn_unused_result))

// === CONSTANTS ===
#define FLAG_DEFAULT	0x2

/**
 * \name Exception Handlers
 * \{
 */
#define	RME_Int_Expt_DivideError(State)	(RME_ERR_DIVERR)
/**
 * \}
 */

// === MACRO VOODOO ===
#define XCHG(a,b)	do{uint32_t t=(a);(a)=(b);(b)=(t);}while(0)
// --- Case Helpers
#define CASE4(b)	case(b):case((b)+1):case((b)+2):case((b)+3)
#define CASE8(b)	CASE4(b):CASE4((b)+4)
#define CASE16(b)	CASE4(b):CASE4((b)+4):CASE4((b)+8):CASE4((b)+12)
#define CASE4K(b,k)	case(b):case((b)+1*(k)):case((b)+2*(k)):case((b)+3*(k))
#define CASE8K(b,k)	CASE4K(b,k):CASE4K((b)+4*(k),k)
#define CASE16K(b,k)	CASE4K(b,k):CASE4K((b)+4*(k),k):CASE4K((b)+8*(k),k):CASE4K((b)+12*(k),k)

// --- OPERATIONS ---
// Logical TEST (Set flags according to AND)
#define RME_Int_DoTest(State, to, from, width)	do{\
	int v = (to) & (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,v,(width));\
	}while(0)
#define RME_Int_DoRol(State, to, from, width)	do{\
	(to) = ((to) << (from)) | ((to) >> ((width)-(from)));\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) >> ((width)-1)) ? FLAG_CF : 0;\
	}while(0)
#define RME_Int_DoRor(State, to, from, width)	do{\
	(to) = ((to) >> (from)) | ((to) << ((width)-(from)));\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) >> ((width)-1)) ? FLAG_CF : 0;\
	}while(0)
#define RME_Int_DoShl(State, to, from, width)	do{\
	(to) <<= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) >> ((width)-1)) ? FLAG_CF : 0;\
	}while(0)
#define RME_Int_DoShr(State, to, from, width)	do{\
	(to) >>= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) & 1) ? FLAG_CF : 0;\
	}while(0)

// === TYPES ===
typedef int (*tOpcodeFcn)(tRME_State *State, int Param);

// === PROTOTYPES ===
tRME_State	*RME_CreateState(void);
void	RME_DumpRegs(tRME_State *State);
 int	RME_CallInt(tRME_State *State, int Num);
 int	RME_Call(tRME_State *State);
static int	RME_Int_DoOpcode(tRME_State *State);

#if 0
static inline WARN_UNUSED_RET int	RME_Int_DoArithOp8(int Num, tRME_State *State, uint8_t *Dest, uint8_t Src);
static inline WARN_UNUSED_RET int	RME_Int_DoArithOp16(int Num, tRME_State *State, uint16_t *Dest, uint16_t Src);
static inline WARN_UNUSED_RET int	RME_Int_DoArithOp32(int Num, tRME_State *State, uint32_t *Dest, uint32_t Src);
#endif

// === GLOBALS ===
#if DEBUG
static const char *casArithOps[] = {"ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP"};
static const char *casLogicOps[] = {"ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "L6-", "L7-"};
#endif

#include "opcode_table.h"

// === CODE ===
/**
 * \brief Creates a blank RME State
 */
tRME_State *RME_CreateState(void)
{
	tRME_State	*state = calloc(sizeof(tRME_State), 1);

	if(state == NULL)	return NULL;

	// Initial Stack
	state->Flags = FLAG_DEFAULT;

	// Stub CS/IP
	state->CS = 0xF000;
	state->IP = 0xFFF0;

	return state;
}

/**
 * \brief Dump Realmode Registers
 */
void RME_DumpRegs(tRME_State *State)
{
	DEBUG_S("\n");
	#if USE_SIZE_OVERRIDES == 1
	DEBUG_S("EAX %08x  ECX %08x  EDX %08x  EBX %08x\n",
		State->AX.D, State->CX.D, State->DX.D, State->BX.D);
	DEBUG_S("ESP %08x  EBP %08x  ESI %08x  EDI %08x\n",
		State->SP.D, State->BP.D, State->SI.D, State->DI.D);
	#else
	DEBUG_S("AX %04x  CX %04x  DX %04x  BX %04x\n",
		State->AX.W, State->CX.W, State->DX.W, State->BX.W);
	DEBUG_S("SP %04x  BP %04x  SI %04x  DI %04x\n",
		State->SP.W, State->BP.W, State->SI.W, State->DI.W);
	#endif
	DEBUG_S("SS %04x  DS %04x  ES %04x\n",
		State->SS, State->DS, State->ES);
	DEBUG_S("CS:IP = 0x%04x:%04x\n", State->CS, State->IP);
	DEBUG_S("Flags = %04x\n", State->Flags);
}

/**
 * \brief Run Realmode interrupt
 */
int RME_CallInt(tRME_State *State, int Num)
{
	 int	ret;
	DEBUG_S("RM_Int: Calling Int 0x%x\n", Num);

	if(Num < 0 || Num > 0xFF) {
		ERROR_S("WARNING: %i is not a valid interrupt number", Num);
		return RME_ERR_INVAL;
	}

	ret = RME_Int_Read16(State, 0, Num*4, &State->IP);
	if(ret)	return ret;
	ret = RME_Int_Read16(State, 0, Num*4+2, &State->CS);
	if(ret)	return ret;

	PUSH(State->Flags);
	PUSH(RME_MAGIC_CS);
	PUSH(RME_MAGIC_IP);

	return RME_Call(State);
}

/**
 * \brief Call a realmode function (a jump to a magic location is used as the return)
 */
int RME_Call(tRME_State *State)
{
	 int	ret;
	for(;;)
	{
		#if DEBUG >= 2
		RME_DumpRegs(State);
		#endif
		if(State->IP == RME_MAGIC_IP && State->CS == RME_MAGIC_CS)
			return 0;
		ret = RME_Int_DoOpcode(State);
		if(ret)	return ret;
	}
}

/**
 * \brief Processes a single instruction
 */
int RME_Int_DoOpcode(tRME_State *State)
{
	 uint8_t	opcode;	// Current opcode and second byte
	 int	ret;	// Return value from functions
	uint16_t	startIP, startCS;	// Initial CPU location

	startIP = State->IP;
	startCS = State->CS;

	State->Decoder.OverrideSegment = -1;
	State->Decoder.RepeatType = 0;
	State->Decoder.bOverrideOperand = 0;
	State->Decoder.bOverrideAddress = 0;
	State->Decoder.bDontChangeIP = 0;
	State->Decoder.IPOffset = 0;
	State->InstrNum ++;

	DEBUG_S("(%8i) [0x%x] %04x:%04x ", State->InstrNum, State->CS*16+State->IP, State->CS, State->IP);

	do
	{
		READ_INSTR8( opcode );
		if( caOperations[opcode].Function == NULL )
		{
			ERROR_S("Unkown Opcode 0x%02x", opcode);
			return RME_ERR_UNDEFOPCODE;
		}
		DEBUG_S("%s ", caOperations[opcode].Name);
		ret = caOperations[opcode].Function(State, caOperations[opcode].Arg);
	} while( ret == RME_ERR_CONTINUE );	// RME_ERR_CONTINUE is returned by prefixes
	

	// repType is cleared if it is used, so if it's not used, it's invalid
	if(State->Decoder.RepeatType)
	{
		DEBUG_S("Prefix 0x%02x used with wrong opcode 0x%02x", State->Decoder.RepeatType, opcode);
		return RME_ERR_UNDEFOPCODE;
	}

	if( !State->Decoder.bDontChangeIP )
		State->IP += State->Decoder.IPOffset;

	#if DEBUG
	{
		uint16_t	i = startIP;
		uint8_t	byte;
		 int	j = State->Decoder.IPOffset;

		DEBUG_S("\t;");
		while(i < 0x10000 && j--) {
			ret = RME_Int_Read8(State, startCS, i, &byte);
			DEBUG_S(" %02x", byte);
			i ++;
		}
		if(j > 0)
		{
			while(j--) {
				ret = RME_Int_Read8(State, startCS, i, &byte);
				DEBUG_S(" %02x", byte);
				i ++;
			}
		}
	}
	#endif
	DEBUG_S("\n");
	return 0;
}

/**
 * \brief Performs a memory addressing function
 * \param State	Emulator State
 * \param mmm	Function ID (mmm field from ModR/M byte)
 * \param disp	Displacement
 * \param ptr	Destination for final pointer
 */
static int DoFunc(tRME_State *State, int mmm, int16_t disp, uint16_t *Segment, uint32_t *Offset)
{
	uint32_t	addr;
	uint16_t	seg;

	switch(mmm){
	case 2:	case 3:	case 6:
		seg = SREG_SS;
		break;
	default:
		seg = SREG_DS;
		break;
	}

	if(State->Decoder.OverrideSegment != -1)
		seg = State->Decoder.OverrideSegment;

	seg = *Seg(State, seg);

	// Only keep the (mod=0) flag when R/M = 6
	if( (mmm & 7) != 6 )
		mmm &= 7;

	switch(mmm)
	{
	case 6|8:	// R/M == 6 when Mod == 0
		READ_INSTR16( disp );
		DEBUG_S(":[0x%x]", disp);
		addr = disp;
		break;

	case 0:
		DEBUG_S(":[BX+SI+0x%x]", disp);
		addr = State->BX.W + State->SI.W + disp;
		break;
	case 1:
		DEBUG_S(":[BX+DI+0x%x]", disp);
		addr = State->BX.W + State->DI.W + disp;
		break;
	case 2:
		DEBUG_S(":[BP+SI+0x%x]", disp);
		addr = State->BP.W + State->SI.W + disp;
		break;
	case 3:
		DEBUG_S(":[BP+DI+0x%x]", disp);
		addr = State->BP.W + State->DI.W + disp;
		break;
	case 4:
		DEBUG_S(":[SI+0x%x]", disp);
		addr = State->SI.W + disp;
		break;
	case 5:
		DEBUG_S(":[DI+0x%x]", disp);
		addr = State->DI.W + disp;
		break;
	case 6:
		DEBUG_S(":[BP+0x%x]", disp);
		addr = State->BP.W + disp;
		break;
	case 7:
		DEBUG_S(":[BX+0x%x]", disp);
		addr = State->BX.W + disp;
		break;
	default:
		ERROR_S("Unknown mmm value passed to DoFunc (%i)", mmm);
		return RME_ERR_BUG;
	}
	//return RME_Int_GetPtr(State, seg, addr, ptr);
	*Segment = seg;
	*Offset = addr;
	return 0;
}

/**
 * \brief Performs a memory addressing function (32-bit, with SIB if nessesary)
 * \param State	Emulator State
 * \param mmm	Function ID (mmm field from ModR/M byte)
 * \param disp	Displacement
 * \param ptr	Destination for final pointer
 */
static int DoFunc32(tRME_State *State, int mmm, int32_t disp, uint16_t *Segment, uint32_t *Offset)
{
	uint32_t	addr;
	uint16_t	seg;
	uint8_t	sib;

	switch(mmm){
	case 2:	case 3:	case 6:
		seg = SREG_SS;
		break;
	default:
		seg = SREG_DS;
		break;
	}

	if(State->Decoder.OverrideSegment != -1)
		seg = State->Decoder.OverrideSegment;

	seg = *Seg(State, seg);

	// Only keep the (mod=0) flag when R/M = 5
	if( (mmm & 7) != 5 )
		mmm &= 7;

	switch(mmm & 7)
	{
	case 0:
		DEBUG_S(":[EAX+0x%x]", disp);
		addr = State->AX.D + disp;
		break;
	case 1:
		DEBUG_S(":[ECX+0x%x]", disp);
		addr = State->CX.D + disp;
		break;
	case 2:
		DEBUG_S(":[EDX+0x%x]", disp);
		addr = State->DX.D + disp;
		break;
	case 3:
		DEBUG_S(":[EBX+0x%x]", disp);
		addr = State->BX.D + disp;
		break;
	case 4:	// SIB
		READ_INSTR8(sib);
		DEBUG_S(":[");
		// Index Reg
		switch( (sib >> 3) & 7 )
		{
		case 0:	DEBUG_S("EAX");	addr = State->AX.D;	break;
		case 1:	DEBUG_S("ECX");	addr = State->CX.D;	break;
		case 2:	DEBUG_S("EDX");	addr = State->DX.D;	break;
		case 3:	DEBUG_S("EBX");	addr = State->BX.D;	break;
		case 4:	DEBUG_S("EIZ");	addr = 0;	break;
		case 5:	DEBUG_S("EBP");	addr = State->BP.D;	break;
		case 6:	DEBUG_S("ESI");	addr = State->SI.D;	break;
		case 7:	DEBUG_S("EDI");	addr = State->DI.D;	break;
		}
		// Scale
		DEBUG_S("*%i", 1 << (sib >> 6));
		addr <<= (sib >> 6);
		// Base
		switch( sib & 7 )
		{
		case 0:	DEBUG_S("+EAX");	addr += State->AX.D;	break;
		case 1:	DEBUG_S("+ECX");	addr += State->CX.D;	break;
		case 2:	DEBUG_S("+EDX");	addr += State->DX.D;	break;
		case 3:	DEBUG_S("+EBX");	addr += State->BX.D;	break;
		case 4:	DEBUG_S("+ESP");	addr += State->SP.D;	break;
		case 5:	// SPECIAL CASE
			if( mmm & 8 ) {
				READ_INSTR32(disp);
			}
			else
			{
				DEBUG_S("+EBP");
				addr += State->BP.D;
			}
			break;
		case 6:	DEBUG_S("+ESI");	addr += State->SI.D;	break;
		case 7:	DEBUG_S("+EDI");	addr += State->DI.D;	break;
		}
		DEBUG_S("+0x%x]", disp);
		addr += disp;
		break;
	case 5:
		if( mmm & 8 )
		{
			// R/M == 5 when Mod == 0
			READ_INSTR32( addr );
			DEBUG_S(":[0x%x]", addr);
		}
		else
		{
			DEBUG_S(":[EBP+0x%x]", disp);
			addr = State->BP.D + disp;
		}
		break;
	case 6:
		DEBUG_S(":[ESI+0x%x]", disp);
		addr = State->SI.D + disp;
		break;
	case 7:
		DEBUG_S(":[EDI+0x%x]", disp);
		addr = State->DI.D + disp;
		break;
	default:
		ERROR_S("Unknown mmm value passed to DoFunc32 (%i)", mmm);
		return RME_ERR_BUG;
	}
	//return RME_Int_GetPtr(State, seg, addr, ptr);
	*Segment = seg;
	*Offset = addr;
	return 0;
}

inline int RME_Int_GetMMM(tRME_State *State, int mod, int mmm, uint16_t *Segment, uint32_t *Offset)
{
	uint16_t	ofs;
	 int	ret;
	switch(mod)
	{
	case 0:	// No Offset
		ret = DoFunc( State, mmm | 8, 0, Segment, Offset );
		if(ret)	return ret;
		break;
	case 1:	// 8 Bit
		READ_INSTR8S( ofs );
		ret = DoFunc( State, mmm, ofs, Segment, Offset);
		if(ret)	return ret;
		break;
	case 2:	// 16 Bit
		READ_INSTR16( ofs );
		ret = DoFunc( State, mmm, ofs, Segment, Offset );
		if(ret)	return ret;
		break;
	case 3:
		ERROR_S("Unknown mod value passed to RME_Int_GetMMM (%i)", mod);
		return RME_ERR_BUG;
	}
	return 0;
}

/**
 * \brief Parses the ModR/M byte as a 8-bit value
 * \param State	Emulator State
 * \param to	R field destination (ignored if NULL)
 * \param from	M field destination (ignored if NULL)
 */
int RME_Int_ParseModRM(tRME_State *State, uint8_t **to, uint8_t **from, int bReverse)
{
	 int	ret;
	 int	mod, rrr, mmm;

	RME_Int_GetModRM(State, &mod, &rrr, &mmm);
	
	#if DEBUG
	if(!bReverse) {
	#endif
		if(to) *to = RegB( State, rrr );
	#if DEBUG
	}
	#endif
	if(from)
	{
		if( mod == 3 )
			*from = RegB( State, mmm );
		else
		{
			uint16_t	segment;
			uint32_t	offset;
			ret = RME_Int_GetMMM( State, mod, mmm, &segment, &offset );
			if(ret)	return ret;
			ret = RME_Int_GetPtr(State, segment, offset, (void**)from);
			if(ret)	return ret;
		}
	}
	#if DEBUG
	if(bReverse) {
		if(to) *to = RegB( State, rrr );
	}
	#endif
	return 0;
}

/**
 * \brief Parses the ModR/M byte as a 16-bit value
 * \param State	Emulator State
 * \param to	R field destination (ignored if NULL)
 * \param from	M field destination (ignored if NULL)
 */
int RME_Int_ParseModRMX(tRME_State *State, uint16_t **to, uint16_t **from, int bReverse)
{
	 int	ret;
	 int	mod, rrr, mmm;

	RME_Int_GetModRM(State, &mod, &rrr, &mmm);
	
	#if DEBUG
	if(!bReverse) {
	#endif
		if(to) *to = RegW( State, rrr );
	#if DEBUG
	}
	#endif
	if(from)
	{
		if( mod == 3 )
			*from = RegW( State, mmm );
		else
		{
			uint16_t	segment;
			uint32_t	offset;
			ret = RME_Int_GetMMM( State, mod, mmm, &segment, &offset );
			if(ret)	return ret;
			ret = RME_Int_GetPtr(State, segment, offset, (void**)from);
			if(ret)	return ret;
		}
	}
	#if DEBUG
	if(bReverse) {
		if(to) *to = RegW( State, rrr );
	}
	#endif
	return 0;
}
