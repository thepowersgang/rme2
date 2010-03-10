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

#include <common.h>

#include "rme.h"

// Settings
#define DEBUG	1	// Enable debug?
#define	printf	printf	// Formatted print function
#define	outb(state,port,val)	outb(port,val)	// Write 1 byte to an IO Port
#define	outw(state,port,val)	outw(port,val)	// Write 2 bytes to an IO Port
#define	inb(state,port)	inb(port)	// Read 1 byte from an IO Port
#define	inw(state,port)	inw(port)	// Read 2 bytes from an IO Port

// === CONSTANTS ===
#define FLAG_DEFAULT	0x2
/**
 * \brief FLAGS Register Values
 * \{
 */
#define FLAG_CF	0x001	//!< Carry Flag
#define FLAG_PF	0x004	//!< Pairity Flag
#define FLAG_AF	0x010	//!< ?
#define FLAG_ZF	0x040	//!< Zero Flag
#define FLAG_SF	0x080	//!< Sign Flag
#define FLAG_TF	0x100	//!< ?
#define FLAG_IF	0x200	//!< Interrupt Flag
#define FLAG_DF	0x400	//!< Direction Flag
#define FLAG_OF	0x800	//!< Overflow Flag
/**
 * \}
 */

/**
 * \name Exception Handlers
 * \{
 */
#define	RME_Int_Expt_DivideError(State)	(RME_ERR_DIVERR)
/**
 * \}
 */

// --- Stack Primiatives ---
#define PUSH(v)	RME_Int_Write16(State,State->SS,State->SP-=2,(v))
#define POP(dst)	do{\
	uint16_t v;\
	RME_Int_Read16(State,State->SS,State->SP,&v);State->SP+=2;\
	(dst)=v;\
	}while(0)

// --- Debug Macro ---
#if DEBUG
# define DEBUG_S(...)	printf(__VA_ARGS__)
#else
# define DEBUG_S(...)
#endif

// === MACRO VOODOO ===
// --- Case Helpers
#define CASE4(b)	case(b):case((b)+1):case((b)+2):case((b)+3)
#define CASE8(b)	CASE4(b):CASE4((b)+4)
#define CASE16(b)	CASE4(b):CASE4((b)+4):CASE4((b)+8):CASE4((b)+12)
#define CASE4K(b,k)	case(b):case((b)+1*(k)):case((b)+2*(k)):case((b)+3*(k))
#define CASE8K(b,k)	CASE4K(b,k):CASE4K((b)+4*(k),k)
#define CASE16K(b,k)	CASE4K(b,k):CASE4K((b)+4*(k),k):CASE4K((b)+8*(k),k):CASE4K((b)+12*(k),k)
// --- Operation helpers
#define PAIRITY8(v)	((((v)>>7)&1)^(((v)>>6)&1)^(((v)>>5)&1)^(((v)>>4)&1)^(((v)>>3)&1)^(((v)>>2)&1)^(((v)>>1)&1)^((v)&1))
#define SET_PF(State,v,w) do{\
	if(w==8)State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;\
	else	State->Flags |= (PAIRITY8(v) ^ PAIRITY8(v>>8)) ? FLAG_PF : 0;\
	}while(0)
#define SET_COMM_FLAGS(State,v,w) do{\
	State->Flags |= ((v) == 0) ? FLAG_ZF : 0;\
	State->Flags |= ((v) >> ((w)-1)) ? FLAG_SF : 0;\
	if(w==8)State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;\
	else	State->Flags |= (PAIRITY8(v) ^ PAIRITY8(v>>8)) ? FLAG_PF : 0;\
	}while(0)

// --- Operations
/**
 * \brief 0 - Add
 */
#define RME_Int_DoAdd(State, to, from, width)	do{\
	(to) += (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) < (from)) ? FLAG_OF|FLAG_CF : 0;\
	}while(0)
/**
 * \brief 1 - Bitwise OR
 */
#define RME_Int_DoOr(State, to, from, width)	do{\
	(to) |= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	}while(0)
/**
 * \brief 2 - Add with Carry
 */
#define RME_Int_DoAdc(State, to, from, width)	do{\
	(to) += (from) + ((State->Flags&FLAG_CF)?1:0);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) < (from)) ? FLAG_OF|FLAG_CF : 0;\
	}while(0)
/**
 * \brief 3 - Subtract with borrow
 */
#define RME_Int_DoSbb(State, to, from, width)	do{\
	(to) -= (from) + ((State->Flags&FLAG_CF)?1:0);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) > (from)) ? FLAG_OF|FLAG_CF : 0;\
	}while(0)
// 4: Bitwise AND
#define RME_Int_DoAnd(State, to, from, width)	do{\
	(to) &= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	}while(0)
// 5: Subtract
#define RME_Int_DoSub(State, to, from, width)	do{\
	(to) -= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	State->Flags |= ((to) > (from)) ? FLAG_OF|FLAG_CF : 0;\
	}while(0)
// 6: Bitwise XOR
#define RME_Int_DoXor(State, to, from, width)	do{\
	(to) ^= (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,(to),(width));\
	}while(0)
// 7: Compare (Set flags according to SUB)
#define RME_Int_DoCmp(State, to, from, width)	do{\
	int v = (to)-(from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,v,(width));\
	State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;\
	}while(0)

// Logical TEST (Set flags according to AND)
#define RME_Int_DoTest(State, to, from, width)	do{\
	int v = (to) & (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	SET_COMM_FLAGS(State,v,(width));\
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

/**
 * \brief Delegates an Arithmatic Operation to the required helper
 */
#define RME_Int_DoArithOp(num, State, to, from, width)	do{\
	switch( (num) ) {\
	case 0:	RME_Int_DoAdd(State, (to), (from), (width));	break;\
	case 1:	RME_Int_DoOr (State, (to), (from), (width));	break;\
	case 2:	RME_Int_DoAdc(State, (to), (from), (width));	break;\
	case 3:	RME_Int_DoSbb(State, (to), (from), (width));	break;\
	case 4:	RME_Int_DoAnd(State, (to), (from), (width));	break;\
	case 5:	RME_Int_DoSub(State, (to), (from), (width));	break;\
	case 6:	RME_Int_DoXor(State, (to), (from), (width));	break;\
	case 7:	RME_Int_DoCmp(State, (to), (from), (width));	break;\
	default: DEBUG_S(" - Undef DoArithOP %i\n", (num));	return RME_ERR_UNDEFOPCODE;\
	}}while(0)

// --- Memory Helpers
/**
 * \brief Read an unsigned byte from the instruction stream
 * Reads 1 byte as an unsigned integer from CS:IP and increases IP by 1.
 */
#define READ_INSTR8(dst)	do{int r;uint8_t v;\
	r=RME_Int_Read8(State,State->CS,State->IP+State->Decoder.IPOffset,&v);\
	if(r)	return r;\
	State->Decoder.IPOffset++;\
	(dst) = v;\
	}while(0)
/**
 * \brief Read a signed byte from the instruction stream
 * Reads 1 byte as an signed integer from CS:IP and increases IP by 1.
 */
#define READ_INSTR8S(dst)	do{int r;int8_t v;\
	r=RME_Int_Read8(State,State->CS,State->IP+State->Decoder.IPOffset,(uint8_t*)&v);\
	if(r)	return r;\
	State->Decoder.IPOffset++;\
	(dst) = v;\
	}while(0)
/**
 * \brief Read a word from the instruction stream
 * Reads 2 bytes as an unsigned integer from CS:IP and increases IP by 2.
 */
#define READ_INSTR16(dst)	do{int r;uint16_t v;\
	r=RME_Int_Read16(State,State->CS,State->IP+State->Decoder.IPOffset,&v);\
	if(r)	return r;\
	State->Decoder.IPOffset+=2;\
	(dst) = v;\
	}while(0)

// === PROTOTYPES ===
tRME_State	*RME_CreateState(void);
void	RME_DumpRegs(tRME_State *State);
 int	RME_CallInt(tRME_State *State, int Num);
 int	RME_Call(tRME_State *State);
static int	RME_Int_DoOpcode(tRME_State *State);

static inline int	RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint16_t Ofs, void **Ptr);
static inline int	RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t *Dst);
static inline int	RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t *Dst);
static inline int	RME_Int_Write8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t Val);
static inline int	RME_Int_Write16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t Val);
static int	RME_Int_ParseModRMB(tRME_State *State, uint8_t **to, uint8_t **from) __attribute__((warn_unused_result));
static int	RME_Int_ParseModRMW(tRME_State *State, uint16_t **to, uint16_t **from) __attribute__((warn_unused_result));
static uint16_t	*Seg(tRME_State *State, int code);
static int	RME_Int_DoCondJMP(tRME_State *State, uint8_t type, uint16_t offset, const char *name);

// === GLOBALS ===
static const char *casReg8Names[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
static const char *casReg16Names[] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
static const char *casArithOps[] = {"ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP"};
static const char *casLogicOps[] = {"L0-", "L1-", "L2-", "L3-", "SHL", "SHR", "L6-", "L7-"};

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
	DEBUG_S("AX %04x  CX %04x  DX %04x  BX %04x\n",
		State->AX, State->BX, State->CX, State->DX);
	DEBUG_S("SP %04x  BP %04x  SI %04x  DI %04x\n",
		State->SP, State->BP, State->SI, State->DI);
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
		DEBUG_S("WARNING: %i is not a valid interrupt number", Num);
		return RME_ERR_INVAL;
	}

	ret = RME_Int_Read16(State, 0, Num*4, &State->IP);
	if(ret)	return ret;
	RME_Int_Read16(State, 0, Num*4+2, &State->CS);

	PUSH(State->Flags);
	PUSH(RME_MAGIC_CS);
	PUSH(RME_MAGIC_IP);

	return RME_Call(State);
}

/**
 * Call a realmode function (a jump to a magic location is used as the return)
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
	uint16_t	pt2, pt1;	// Spare Words, used for values read from memory
	uint16_t	seg;
	uint16_t	*toW, *fromW;
	 uint8_t	*toB, *fromB;
	 uint8_t	repType = 0;
	uint16_t	repStart = 0;
	 uint8_t	opcode, byte2;
	 int	ret;

	uint16_t	startIP, startCS;

	startIP = State->IP;
	startCS = State->CS;

	State->Decoder.OverrideSegment = -1;
	State->Decoder.IPOffset = 0;
	State->InstrNum ++;

	DEBUG_S("(%8i) [0x%x] %04x:%04x ", State->InstrNum, State->CS*16+State->IP, State->CS, State->IP);

decode:
	READ_INSTR8( opcode );
	switch( opcode )
	{

	// Prefixes
	case OVR_CS:
		DEBUG_S("<CS> ");
		State->Decoder.OverrideSegment = SREG_CS;
		goto decode;
	case OVR_SS:
		DEBUG_S("<SS> ");
		State->Decoder.OverrideSegment = SREG_SS;
		goto decode;
	case OVR_DS:
		DEBUG_S("<DS> ");
		State->Decoder.OverrideSegment = SREG_DS;
		goto decode;
	case OVR_ES:
		DEBUG_S("<ES> ");
		State->Decoder.OverrideSegment = SREG_ES;
		goto decode;

	case 0x66:	//Operand Size Override
		DEBUG_S("PREFIX: OPERAND OVERRIDE");
		break;
	case 0x67:	//Memory Size Override
		DEBUG_S("PREFIX: ADDR OVERRIDE");
		break;

	// Repeat Prefix
	case REP:	DEBUG_S("REP ");
		repType = REP;
		repStart = State->IP;
		goto decode;
	case REPNZ:	DEBUG_S("REPNZ ");
		repType = REPNZ;
		repStart = State->IP;
		goto decode;

	// <op> MR
	CASE8K(0x00, 0x8):
		DEBUG_S("%s (MR)", casArithOps[opcode >> 3]);
		ret = RME_Int_ParseModRMB(State, &fromB, &toB);
		if(ret)	return ret;
		RME_Int_DoArithOp( opcode >> 3, State, *toB, *fromB, 8 );
		break;
	// <op> MRX
	CASE8K(0x01, 0x8):
		DEBUG_S("%s (MRX)", casArithOps[opcode >> 3]);
		ret = RME_Int_ParseModRMW(State, &fromW, &toW);
		if(ret)	return ret;
		RME_Int_DoArithOp( opcode >> 3, State, *toW, *fromW, 16 );
		break;

	// <op> RM
	CASE8K(0x02, 0x8):
		DEBUG_S("%s (RM)", casArithOps[opcode >> 3]);
		ret = RME_Int_ParseModRMB(State, &toB, &fromB);
		if(ret)	return ret;
		RME_Int_DoArithOp( opcode >> 3, State, *toB, *fromB, 8 );
		break;
	// <op> RMX
	CASE8K(0x03, 0x8):
		DEBUG_S("%s (RM)", casArithOps[opcode >> 3]);
		ret = RME_Int_ParseModRMW(State, &toW, &fromW);
		if(ret)	return ret;
		RME_Int_DoArithOp( opcode >> 3, State, *toW, *fromW, 16 );
		break;

	// <op> AI
	CASE8K(0x04, 8):
		READ_INSTR8( pt2 );
		DEBUG_S("%s (AI) AL 0x%02x", casArithOps[opcode >> 3], pt2);
		RME_Int_DoArithOp( opcode >> 3, State, *(uint8_t*)&State->AX, pt2, 8 );
		break;

	// <op> AIX
	CASE8K(0x05, 8):
		READ_INSTR16( pt2 );
		DEBUG_S("%s (AIX) AX 0x%04x", casArithOps[opcode >> 3], pt2);
		RME_Int_DoArithOp( opcode >> 3, State, State->AX, pt2, 16 );
		break;

	// <op> RI
	case 0x80:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RI)", casArithOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMB(State, NULL, &toB);
		if(ret)	return ret;
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	// <op> RIX
	case 0x81:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RIX)", casArithOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
		if(ret)	return ret;
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);	//Print Immediate Valiue
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	// 0x82 MAY be a valid instruction, with the same effect as 0x80 (<op> RI)
	// <op> RI8X
	case 0x83:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RI8X)", casArithOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
		if(ret)	return ret;
		READ_INSTR8S( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toW, pt2, 16 );
		break;

	// ==== Logic Functions (Shifts) ===
	#define RME_Int_DoLogicOp( num, State, to, from, width )	do{\
		switch( (num) ) {\
		case 4:	RME_Int_DoShl(State, (to), (from), (width));	break;\
		case 5:	RME_Int_DoShr(State, (to), (from), (width));	break;\
		default: DEBUG_S(" - DoLogicOp Undef %i\n", (num)); return RME_ERR_UNDEFOPCODE;\
		}}while(0)
	// <op> RI8
	case 0xC0:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RI8)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMB(State, NULL, &toB);
		if(ret)	return ret;
		READ_INSTR8( pt2 );
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	// <op> RI8X
	case 0xC1:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RI8X)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);
		if(ret)	return ret;
		READ_INSTR8( pt2 );
		DEBUG_S("0x%04x", pt2);
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, pt2, 8 );
		break;
	// <op> R1
	case 0xD0:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (R1)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMB(State, NULL, &toB);
		if(ret)	return ret;
		DEBUG_S(" 1");
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, 1, 8 );
		break;
	// <op> R1X
	case 0xD1:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (R1X)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);
		if(ret)	return ret;
		DEBUG_S(" 1");
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, 1, 8 );
		break;
	// <op> RCl
	case 0xD2:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RCl)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMB(State, NULL, &toB);
		if(ret)	return ret;
		DEBUG_S(" CL");
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, State->CX&0xFF, 8 );
		break;
	// <op> RClX
	case 0xD3:
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		DEBUG_S("%s (RClX)", casLogicOps[(byte2 >> 3) & 7]);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);
		if(ret)	return ret;
		DEBUG_S(" CL");
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, State->CX&0xFF, 8 );
		break;

	// <op> RI
	case 0xF6:	//Register Immidate
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		switch( (byte2>>3) & 7 )
		{
		case 0:	// TEST r/m8, Imm8
			DEBUG_S("TEST (RI)");
			ret = RME_Int_ParseModRMB(State, NULL, &toB);
			if(ret)	return ret;
			READ_INSTR8(pt2);
			DEBUG_S(" 0x%02x", pt2);
			RME_Int_DoTest(State, *toB, pt2, 8);
			break;
		case 1:	// Undef
			DEBUG_S("0xF6 /1 Undefined\n");
			return RME_ERR_UNDEFOPCODE;
		//case 2:	break;	// NOT r/m8
		//case 3:	break;	// NEG r/m8
		//case 4:	break;	// MUL AX = AL * r/m8
		//case 5:	break;	// IMUL AX = AL * r/m8
		case 6:	// DIV AX, r/m8 (unsigned)
			DEBUG_S("DIV (RI) AX");
			ret = RME_Int_ParseModRMB(State, NULL, &fromB);
			if(ret)	return ret;
			if(*fromB == 0)	return RME_Int_Expt_DivideError(State);
			pt2 = State->AX / *fromW;
			if(pt2 > 0xFF)	return RME_Int_Expt_DivideError(State);
			State->DX = State->AX - pt2 * (*fromW);
			State->AX = pt2;
			break;
		//case 7:	break;	// IDIV AX, r/m8 (signed)
		default:
			DEBUG_S("0xF6 /%x unknown\n", (byte2>>3) & 7);
			return RME_ERR_UNDEFOPCODE;
		}
		break;
	// <op> RIX
	case 0xF7:	//Register Immidate Extended
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		switch( (byte2>>3) & 7 ) {
		case 0:	// TEST r/m16, Imm16
			DEBUG_S("TEST (RIX)");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);
			if(ret)	return ret;
			READ_INSTR16(pt2);
			DEBUG_S(" 0x%04x", pt2);
			RME_Int_DoTest(State, *toW, pt2, 16);
			break;
		case 1:
			DEBUG_S("0xF7 /1 Undefined\n");
			return RME_ERR_UNDEFOPCODE;
		//case 2:	break;	// NOT r/m16
		//case 3:	break;	// NEG r/m16
		//case 4:	break;	// MUL AX, r/m16
		case 5:	// IMUL AX, r/m16
			{
			uint32_t	dword;
			DEBUG_S("IMUL (RIX) AX");
			ret = RME_Int_ParseModRMW(State, NULL, &fromW);
			if(ret)	return ret;
			dword = (int16_t)State->AX * (int16_t)*fromW;
			if((int32_t)dword == (int16_t)State->AX)
				State->Flags |= FLAG_CF|FLAG_OF;
			else
				State->Flags |= FLAG_CF|FLAG_OF;
			}
			break;
		case 6:	// DIV DX:AX, r/m16
			{
			uint32_t	dword, dword2;
			DEBUG_S("DIV (RIX) DX:AX");
			ret = RME_Int_ParseModRMW(State, NULL, &fromW);
			if(ret)	return ret;
			if( *fromW == 0 )	return RME_Int_Expt_DivideError(State);
			dword = (State->DX<<16)|State->AX;
			dword2 = dword / *fromW;
			if(dword2 > 0xFFFF)	return RME_Int_Expt_DivideError(State);
			State->AX = dword2;
			State->DX = dword - dword2 * (*fromW);
			}
			break;
		//case 7:	break;	// IDIV DX:AX, r/m16
		default:
			DEBUG_S("0xF7 /%x unknown\n", (byte2>>3) & 7);
			return RME_ERR_UNDEFOPCODE;
		}
		break;

	case 0xFE:	//Register
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		switch( (byte2>>3) & 7 ) {
		case 0:
			DEBUG_S("INC (R)");
			ret = RME_Int_ParseModRMB(State, NULL, &toB);	//Get Register Value
			if(ret)	return ret;
			(*toB) ++;
			break;
		case 1:
			DEBUG_S("DEC (R)");
			ret = RME_Int_ParseModRMB(State, NULL, &toB);	//Get Register Value
			if(ret)	return ret;
			(*toB) --;
			break;
		default:
			DEBUG_S("0xFE /%x unknown", (byte2>>3) & 7);
			return RME_ERR_UNDEFOPCODE;
		}
		break;

	case 0xFF:	//Register Extended
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		switch( (byte2>>3) & 7 ) {
		case 0:
			DEBUG_S("INC (RX)");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
			if(ret)	return ret;
			(*toW) ++;
			break;
		case 1:
			DEBUG_S("DEC (RX)");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
			if(ret)	return ret;
			(*toW) --;
			break;
		case 2:
			DEBUG_S("CALL (RX) NEAR");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
			if(ret)	return ret;
			PUSH( State->IP + State->Decoder.IPOffset );
			DEBUG_S(" (0x%04x)", *toW);
			State->IP = *toW;
			goto ret;
		case 3:
			DEBUG_S("CALL (MX) FAR --NI--\n");
			return RME_ERR_UNDEFOPCODE;
		case 4:
			DEBUG_S("JMP (RX) NEAR");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
			if(ret)	return ret;
			DEBUG_S(" (0x%04x)", *toW);
			State->IP = *toW;
			goto ret;
		case 5:
			DEBUG_S("JMP (MX) FAR --NI--\n");
			return RME_ERR_UNDEFOPCODE;
		case 6:
			DEBUG_S("PUSH (RX)");
			ret = RME_Int_ParseModRMW(State, NULL, &toW);	//Get Register Value
			if(ret)	return ret;
			PUSH( *toW );
			break;
		case 7:
			DEBUG_S("0xFF /7 - Undefined\n");
			return RME_ERR_UNDEFOPCODE;
		}
		break;

	//TEST Family
	case TEST_RM:	DEBUG_S("TEST (RR)");	//Test Register
		ret = RME_Int_ParseModRMB(State, &toB, &fromB);
		if(ret)	return ret;
		RME_Int_DoTest(State, *toB, *fromB, 8);
		break;
	case TEST_RMX:	DEBUG_S("TEST (RRX)");	//Test Register Extended
		ret = RME_Int_ParseModRMW(State, &toW, &fromW);
		if(ret)	return ret;
		RME_Int_DoTest(State, *toW, *fromW, 16);
		break;
	case TEST_AI:	DEBUG_S("TEST (AI)");
		READ_INSTR8( pt2 );
		RME_Int_DoTest(State, State->AX&0xFF, pt2, 8);
		break;
	case TEST_AIX:	DEBUG_S("TEST (AIX)");
		READ_INSTR16( pt2 );
		RME_Int_DoTest(State, State->AX, pt2, 16);
		break;

	// Flag Control
	case CLC:	DEBUG_S("CLC");	State->Flags &= ~FLAG_CF;	break;
	case STC:	DEBUG_S("STC");	State->Flags |= FLAG_CF;	break;
	case CLI:	DEBUG_S("CLI");	State->Flags &= ~FLAG_IF;	break;
	case STI:	DEBUG_S("STI");	State->Flags |= FLAG_IF;	break;
	case CLD:	DEBUG_S("CLD");	State->Flags &= ~FLAG_DF;	break;
	case STD:	DEBUG_S("STD");	State->Flags |= FLAG_DF;	break;

	// DEC Register
	case DEC_A:		DEBUG_S("DEC AX");	State->AX --;	break;
	case DEC_B:		DEBUG_S("DEC BX");	State->BX --;	break;
	case DEC_C:		DEBUG_S("DEC CX");	State->CX --;	break;
	case DEC_D:		DEBUG_S("DEC DX");	State->DX --;	break;
	case DEC_Sp:	DEBUG_S("DEC SP");	State->SP --;	break;
	case DEC_Bp:	DEBUG_S("DEC BP");	State->BP --;	break;
	case DEC_Si:	DEBUG_S("DEC SI");	State->SI --;	break;
	case DEC_Di:	DEBUG_S("DEC DI");	State->DI --;	break;
	// INC Register
	case INC_A:		DEBUG_S("INC AX");	State->AX ++;	break;
	case INC_B:		DEBUG_S("INC BX");	State->BX ++;	break;
	case INC_C:		DEBUG_S("INC CX");	State->CX ++;	break;
	case INC_D:		DEBUG_S("INC DX");	State->DX ++;	break;
	case INC_Sp:	DEBUG_S("INC SP");	State->SP ++;	break;
	case INC_Bp:	DEBUG_S("INC BP");	State->BP ++;	break;
	case INC_Si:	DEBUG_S("INC SI");	State->SI ++;	break;
	case INC_Di:	DEBUG_S("INC DI");	State->DI ++;	break;

	// IN <port>, A
	case IN_AI:	// Imm8, AL
		READ_INSTR8( pt2 );
		DEBUG_S("IN (AI) 0x%02x AL", pt2);
		*(uint8_t*)(&State->AX) = inb( State, pt2 );
		break;
	case IN_AIX:	// Imm8, AX
		READ_INSTR8( pt2 );
		DEBUG_S("IN (AIX) 0x%02x AX", pt2);
		State->AX = inw( State, pt2 );
		break;
	case IN_ADx:	// DX, AL
		DEBUG_S("IN (ADx) DX AL");
		*(uint8_t*)(&State->AX) = inb(State, State->DX);
		break;
	case IN_ADxX:	// DX, AX
		DEBUG_S("IN (ADxX) DX AX");
		State->AX = inw(State, State->DX);
		break;
	// OUT <port>, A
	case OUT_IA:	// Imm8, AL
		READ_INSTR8( pt2 );
		DEBUG_S("OUT (IA) 0x%02x AL", pt2);
		outb( State, pt2, State->AX&0xFF );
		break;
	case OUT_IAX:	// Imm8, AX
		READ_INSTR8( pt2 );
		DEBUG_S("OUT (IAX) 0x%02x AX", pt2);
		outw( State, pt2, State->AX );
		break;
	case OUT_DxA:	// DX, AL
		DEBUG_S("OUT (DxA) DX AL");
		outb( State, State->DX, State->AX&0xFF );
		break;
	case OUT_DxAX:	// DX, AX
		DEBUG_S("OUT (DxAX) DX AX");
		outw( State, State->DX, State->AX );
		break;

	//INT Family
	case INT3:
		DEBUG_S("INT 3");
		RME_Int_Read16(State, 0, 3*4, &pt1);	// Offset
		RME_Int_Read16(State, 0, 3*4+2, &pt2);	// Segment
		PUSH( State->Flags );
		PUSH( State->CS );
		PUSH( State->IP );
		State->IP = pt1;
		State->CS = pt2;
		goto ret;
		break;
	case INT_I:
		READ_INSTR8( byte2 );
		DEBUG_S("INT 0x%02x", byte2);
		RME_Int_Read16(State, 0, byte2*4, &pt1);	// Offset
		RME_Int_Read16(State, 0, byte2*4+2, &pt2);	// Segment
		PUSH( State->Flags );
		PUSH( State->CS );
		PUSH( State->IP );
		State->IP = pt1;
		State->CS = pt2;
		goto ret;
	case IRET:
		DEBUG_S("IRET");
		POP( State->IP );
		POP( State->CS );
		POP( State->Flags );
		goto ret;

	//MOV Family
	case MOV_MoA:	// Store AL at Memory Offset
		DEBUG_S("MOV (MoA)");
		seg = (State->Decoder.OverrideSegment==-1) ? SREG_DS : State->Decoder.OverrideSegment;
		seg = *Seg(State, seg);
		READ_INSTR16( pt2 );
		DEBUG_S(":0x%04x AX", pt2);
		RME_Int_Write8(State, seg, pt2, State->AX & 0xFF);
		break;
	case MOV_MoAX:	//Store AX at Memory Offset
		DEBUG_S("MOV (MoAX)");
		seg = (State->Decoder.OverrideSegment==-1) ? SREG_DS : State->Decoder.OverrideSegment;
		seg = *Seg(State, seg);
		READ_INSTR16( pt2 );
		DEBUG_S(":0x%04x AX", pt2);
		RME_Int_Write16(State, seg, pt2, State->AX);
	case MOV_AMo:	//Memory Offset to AL
		DEBUG_S("MOV (AMo) AL");
		seg = (State->Decoder.OverrideSegment==-1) ? SREG_DS : State->Decoder.OverrideSegment;
		seg = *Seg(State, seg);
		READ_INSTR16( pt2 );
		DEBUG_S(":0x%04x", pt2);
		RME_Int_Read8(State, seg, pt2, (uint8_t*)&State->AX);
		break;
	case MOV_AMoX:	//Memory Offset to AX
		DEBUG_S("MOV (AMoX) AX");
		seg = (State->Decoder.OverrideSegment==-1) ? SREG_DS : State->Decoder.OverrideSegment;
		seg = *Seg(State, seg);
		READ_INSTR16( pt2 );
		DEBUG_S(":0x%04x", pt2);
		RME_Int_Read16(State, State->DS, pt2, &State->AX);
		break;
	case MOV_MI:
		DEBUG_S("MOV (RI)");
		ret = RME_Int_ParseModRMB(State, &toB, NULL);
		if(ret)	return ret;
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		*toB = pt2;
		break;
	case MOV_MIX:
		DEBUG_S("MOV (RIX)");
		ret = RME_Int_ParseModRMW(State, &toW, NULL);
		if(ret)	return ret;
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		*toW = pt2;
		break;
	case MOV_RM:
		DEBUG_S("MOV (RM)");
		ret = RME_Int_ParseModRMB(State, &toB, &fromB);
		if(ret)	return ret;
		*toB = *fromB;
		break;
	case MOV_RMX:
		DEBUG_S("MOV (RMX)");
		ret = RME_Int_ParseModRMW(State, &toW, &fromW);
		if(ret)	return ret;
		*toW = *fromW;
		break;
	case MOV_MR:	DEBUG_S("MOV (RM) REV");
		ret = RME_Int_ParseModRMB(State, &fromB, &toB);
		if(ret)	return ret;
		*toB = *fromB;
		break;
	case MOV_MRX:	DEBUG_S("MOV (RMX) REV");
		ret = RME_Int_ParseModRMW(State, &fromW, &toW);
		if(ret)	return ret;
		*toW = *fromW;
		break;

	case MOV_RI_AL:	case MOV_RI_CL:
	case MOV_RI_DL:	case MOV_RI_BL:
	case MOV_RI_AH:	case MOV_RI_CH:
	case MOV_RI_DH:	case MOV_RI_BH:
		READ_INSTR8( pt2 );
		DEBUG_S("MOV (RI) %s 0x%02x", casReg8Names[opcode&7], pt2);
		if(opcode&4)
			State->GPRs[opcode&3] = (State->GPRs[opcode&3]&0xFF) | (pt2<<8);
		else
			State->GPRs[opcode&3] = (State->GPRs[opcode&3]&0xFF00) | pt2;
		break;

	case MOV_RI_AX:	case MOV_RI_CX:
	case MOV_RI_DX:	case MOV_RI_BX:
	case MOV_RI_SP:	case MOV_RI_BP:
	case MOV_RI_SI:	case MOV_RI_DI:
		READ_INSTR16( pt2 );
		DEBUG_S("MOV (RIX) %s 0x%04x", casReg16Names[opcode&7], pt2);
		State->GPRs[opcode&7] = pt2;
		break;

	case MOV_RS:
		DEBUG_S("MOV (RS)");
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		fromW = Seg(State, (byte2>>3)&7);
		ret = RME_Int_ParseModRMW(State, NULL, &toW);
		if(ret)	return ret;
		*toW = *fromW;
		break;

	case MOV_SR:
		DEBUG_S("MOV (SR)");
		READ_INSTR8( byte2 );	State->Decoder.IPOffset --;
		toW = Seg(State, (byte2>>3)&7);
		ret = RME_Int_ParseModRMW(State, NULL, &fromW);
		if(ret)	return ret;
		*toW = *fromW;
		break;


	// JMP Family
	case JMP_S:	// Short Jump
		READ_INSTR8S( pt2 );
		DEBUG_S("JMP (S) .+0x%04x", pt2);
		State->IP += pt2;
		break;
	case JMP_N:	// Near Jump
		READ_INSTR16( pt2 );
		DEBUG_S("JMP (N) .+0x%04x", pt2 );
		State->IP += pt2;
		break;
	case JMP_F:	// Far Jump
		READ_INSTR16( pt1 );
		READ_INSTR16( pt2 );
		DEBUG_S("JMP FAR %04x:%04x", pt2, pt1);
		State->CS = pt2;	State->IP = pt1;
		goto ret;

	//XCHG Family
	case XCHG_AA:	//NOP 0x90
		DEBUG_S("NOP");
		break;
	case XCHG_AB:
		DEBUG_S("XCHG AX BX");
		pt2 = State->AX;	State->AX = State->BX;	State->BX = pt2;
		break;
	case XCHG_AC:
		DEBUG_S("XCHG AX CX");
		pt2 = State->AX;	State->AX = State->CX;	State->CX = pt2;
		break;
	case XCHG_AD:
		DEBUG_S("XCHG AX DX");
		pt2 = State->AX;	State->AX = State->DX;	State->DX = pt2;
		break;
	case XCHG_ASp:
		DEBUG_S("XCHG AX SP");
		pt2 = State->AX;	State->AX = State->SP;	State->SP = pt2;
		break;
	case XCHG_ABp:
		DEBUG_S("XCHG AX BP");
		pt2 = State->AX;	State->AX = State->BP;	State->BP = pt2;
		break;
	case XCHG_ASi:
		DEBUG_S("XCHG AX SI");
		pt2 = State->AX;	State->AX = State->SI;	State->SI = pt2;
		break;
	case XCHG_ADi:
		DEBUG_S("XCHG AX DI");
		pt2 = State->AX;	State->AX = State->DI;	State->DI = pt2;
		break;
	case XCHG_RM:
		DEBUG_S("XCHG (RM)");
		ret = RME_Int_ParseModRMW(State, &toW, &fromW);
		if(ret)	return ret;
		pt2 = *toW;		*toW = *fromW;	*fromW = pt2;
		break;

	//PUSH Family
	case PUSHF:
		DEBUG_S("PUSHF");
		PUSH(State->Flags);
		break;
	case PUSHA:
		DEBUG_S("PUSHA");
		pt2 = State->SP;
		PUSH(State->AX);	PUSH(State->CX);
		PUSH(State->DX);	PUSH(State->BX);
		PUSH(pt2);	PUSH(State->BP);
		PUSH(State->SI);	PUSH(State->DI);
		break;
	case PUSH_AX:	DEBUG_S("PUSH AX");	PUSH(State->AX);	break;
	case PUSH_BX:	DEBUG_S("PUSH BX");	PUSH(State->BX);	break;
	case PUSH_CX:	DEBUG_S("PUSH CX");	PUSH(State->CX);	break;
	case PUSH_DX:	DEBUG_S("PUSH DX");	PUSH(State->DX);	break;
	case PUSH_SP:	DEBUG_S("PUSH Sp");	pt2 = State->SP; PUSH(pt2);	break;
	case PUSH_BP:	DEBUG_S("PUSH BP");	PUSH(State->BP);	break;
	case PUSH_SI:	DEBUG_S("PUSH SI");	PUSH(State->SI);	break;
	case PUSH_DI:	DEBUG_S("PUSH DI");	PUSH(State->DI);	break;
	case PUSH_ES:	DEBUG_S("PUSH ES");	PUSH(State->ES);	break;
	case PUSH_CS:	DEBUG_S("PUSH CS");	PUSH(State->CS);	break;
	case PUSH_SS:	DEBUG_S("PUSH SS");	PUSH(State->SS);	break;
	case PUSH_DS:	DEBUG_S("PUSH DS");	PUSH(State->DS);	break;
	case PUSH_I8:
		READ_INSTR8( pt2 );
		DEBUG_S("PUSH (I8) 0x%02x", pt2);
		PUSH(pt2);
		break;
	case PUSH_I:
		READ_INSTR16( pt2 );
		DEBUG_S("PUSH (I) 0x%04x", pt2);
		PUSH(pt2);
		break;

	//POP Family
	case POPF:
		DEBUG_S("POPF");
		POP(State->Flags);
		break;
	case POPA:
		DEBUG_S("POPA");
		POP(State->DI);	POP(State->SI);
		POP(State->BP);	State->SP += 2;
		POP(State->BX);	POP(State->DX);
		POP(State->CX);	POP(State->AX);
		break;
	case POP_AX:	DEBUG_S("POP AX");	POP(State->AX);	break;
	case POP_BX:	DEBUG_S("POP BX");	POP(State->BX);	break;
	case POP_CX:	DEBUG_S("POP CX");	POP(State->CX);	break;
	case POP_DX:	DEBUG_S("POP DX");	POP(State->DX);	break;
	case POP_SP:	DEBUG_S("POP SP");	State->SP += 2;	break;	// POP SP does practically nothing
	case POP_BP:	DEBUG_S("POP BP");	POP(State->BP);	break;
	case POP_SI:	DEBUG_S("POP SI");	POP(State->SI);	break;
	case POP_DI:	DEBUG_S("POP DI");	POP(State->DI);	break;
	case POP_ES:	DEBUG_S("POP ES");	POP(State->ES);	break;
	case POP_SS:	DEBUG_S("POP SS");	POP(State->SS);	break;
	case POP_DS:	DEBUG_S("POP DS");	POP(State->DS);	break;

	// CALL Family
	case CALL_N:
		READ_INSTR16( pt2 );
		DEBUG_S("CALL (N) .+0x%04x", pt2);
		State->IP += State->Decoder.IPOffset;
		PUSH(State->IP);
		State->IP += pt2;
		goto ret;
	case CALL_F:
		READ_INSTR16( pt1 );
		READ_INSTR16( pt2 );
		DEBUG_S("CALL (F) %04x:%04x", pt2, pt1);
		PUSH(State->CS);
		PUSH(State->IP + State->Decoder.IPOffset);
		State->CS = pt2;
		State->IP = pt1;
		goto ret;
	case RET_N:
		DEBUG_S("RET (N)");
		POP(State->IP);
		goto ret;
	case RET_F:
		DEBUG_S("RET (F)");
		POP(State->IP);
		POP(State->CS);
		goto ret;

	// -- String Operations --
	case STOSB:
		DEBUG_S("STOSB ES:[DI] AL");
		if( repType == REP )	DEBUG_S(" (0x%x times)", State->CX);
		do {
			ret = RME_Int_Write8(State, State->ES, State->DI, State->AX&0xFF);
			if(ret)	return ret;
			State->DI ++;
		} while(repType == REP && State->CX && State->CX--);
		repType = 0;
		break;
	case STOSW:
		DEBUG_S("STOSW ES:[DI] AX");
		if( repType == REP )	DEBUG_S(" (0x%x times)", State->CX);
		do {
			ret = RME_Int_Write16(State, State->ES, State->DI, State->AX);
			if(ret)	return ret;
			State->DI += 2;
		} while(repType == REP && State->CX && (State->CX-=2)+2);
		repType = 0;
		break;

	// Misc
	case LEA:
		DEBUG_S("LEA");
		// LEA needs to parse the address itself, because it needs the
		// emulated address, not the true address.
		// Hence, it cannot use RME_Int_ParseModRMB/W
		READ_INSTR8(byte2);
		switch(byte2 >> 6)
		{
		case 0:	// No Offset
			pt1 = 0;
			break;
		case 1:	// 8-bit signed
			READ_INSTR8S(pt1);
			break;
		case 2:	// 16-bit
			READ_INSTR16(pt1);
			break;
		case 3:
			return RME_ERR_UNDEFOPCODE;
		}

		switch(byte2 & 7)
		{
		case 0:
			DEBUG_S(" DS:[BX+SI+0x%x]", pt1);
			pt1 += State->BX + State->SI;
			break;
		case 1:
			DEBUG_S(" DS:[BX+DI+0x%x]", pt1);
			pt1 += State->BX + State->DI;
			break;
		case 2:
			DEBUG_S(" SS:[BP+SI+0x%x]", pt1);
			pt1 += State->BP + State->SI;
			break;
		case 3:
			DEBUG_S(" SS:[BP+DI+0x%x]", pt1);
			pt1 += State->BP + State->DI;
			break;
		case 4:
			DEBUG_S(" DS:[SI+0x%x]", pt1);
			pt1 += State->SI;
			break;
		case 5:
			DEBUG_S(" DS:[DI+0x%x]", pt1);
			pt1 += State->DI;
			break;
		case 6:
			if( (byte2 >> 6) == 0 ) {
				READ_INSTR16(pt1);
				DEBUG_S(" DS:[0x%x]", pt1);
			} else {
				DEBUG_S(" SS:[BP+0x%x]", pt1);
				pt1 += State->BP;
			}
			break;
		case 7:
			DEBUG_S(" DS:[BX+0x%x]", pt1);
			pt1 += State->BX;
			break;
		}
		State->GPRs[ (byte2>>3)&7 ] = pt1;
		break;

	// -- Loops --
	case LOOP:	DEBUG_S("LOOP ");
		READ_INSTR8S( pt2 );
		if(State->CX != 0)
			State->IP += pt2;
		break;
	case LOOPNZ:	DEBUG_S("LOOPNZ ");
		READ_INSTR8S( pt2 );
		if(State->CX != 0 && !(State->Flags & FLAG_ZF))
			State->IP += pt2;
		break;
	case LOOPZ:	DEBUG_S("LOOPZ ");
		READ_INSTR8S( pt2 );
		if(State->CX != 0 && State->Flags & FLAG_ZF)
			State->IP += pt2;
		break;

	// Short Jumps
	CASE16(0x70):
		READ_INSTR8S( pt2 );
		ret = RME_Int_DoCondJMP(State, opcode & 0xF, pt2, "(S)");
		if(ret)	return ret;
		break;


	// -- Two Byte Opcodes --
	case 0x0F:
		READ_INSTR8( byte2 );

		switch(byte2)
		{
		//--- Near Jump --- (1000cccc)
		CASE16(0x80):
			READ_INSTR16( pt2 );
			ret = RME_Int_DoCondJMP(State, byte2&0xF, pt2, "(N)");
			if(ret)	return ret;
			break;

		default:
			DEBUG_S("0x0F 0x%02x unknown\n", byte2);
			return RME_ERR_UNDEFOPCODE;
		}
		break;

	default:
		DEBUG_S("Unkown Opcode 0x%02x", opcode);
		return RME_ERR_UNDEFOPCODE;
	}

	if(repType)
	{
		DEBUG_S("Prefix 0x%02x used with wrong opcode 0x%02x", repType, opcode);
		return RME_ERR_UNDEFOPCODE;
	}

	State->IP += State->Decoder.IPOffset;

ret:
	#if DEBUG
	{
		uint16_t	i = startIP;
		uint8_t	byte;
		 int	j = State->Decoder.IPOffset;

		DEBUG_S("\t;");
		while(i < 0x10000 && j--) {
			RME_Int_Read8(State, startCS, i, &byte);
			DEBUG_S(" %02x", byte);
			i ++;
		}
		if(j > 0)
		{
			while(j--) {
				RME_Int_Read8(State, startCS, i, &byte);
				DEBUG_S(" %02x", byte);
				i ++;
			}
		}
	}
	#endif
	DEBUG_S("\n");
	return 0;
}

static inline int RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint16_t Ofs, void **Ptr)
{
	uint32_t	addr = Seg * 16 + Ofs;
	#if RME_DO_NULL_CHECK
	# if RME_ALLOW_ZERO_TO_BE_NULL
	if(addr/RME_BLOCK_SIZE && State->Memory[addr/RME_BLOCK_SIZE] == NULL)
	# else
	if(State->Memory[addr/RME_BLOCK_SIZE] == NULL)
	# endif
		return RME_ERR_BADMEM;
	#endif
	*Ptr = &State->Memory[addr/RME_BLOCK_SIZE][addr%RME_BLOCK_SIZE];
	return 0;
}

static inline int RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t *Dst)
{
	uint8_t	*ptr;
	 int	ret;
	ret = RME_Int_GetPtr(State, Seg, Ofs, (void**)&ptr);
	if(ret)	return ret;
	*Dst = *ptr;
	return 0;
}

static inline int RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t *Dst)
{
	uint16_t	*ptr;
	 int	ret;
	ret = RME_Int_GetPtr(State, Seg, Ofs, (void**)&ptr);
	if(ret)	return ret;
	*Dst = *ptr;
	return 0;
}

static inline int RME_Int_Write8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t Val)
{
	uint8_t	*ptr;
	 int	ret;
	ret = RME_Int_GetPtr(State, Seg, Ofs, (void**)&ptr);
	if(ret)	return ret;
	*ptr = Val;
	return 0;
}

static inline int RME_Int_Write16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t Val)
{
	uint16_t	*ptr;
	 int	ret;
	ret = RME_Int_GetPtr(State, Seg, Ofs, (void**)&ptr);
	if(ret)	return ret;
	*ptr = Val;
	return 0;
}

/**
 * \brief Returns a pointer to the specified byte register
 * \param State	Emulator State
 * \param num	Register Number
 */
static inline uint8_t *RegB(tRME_State *State, int num)
{
	switch(num)
	{
	case 0:	DEBUG_S(" AL");	return (uint8_t*)&State->AX;
	case 1:	DEBUG_S(" CL");	return (uint8_t*)&State->CX;
	case 2:	DEBUG_S(" DL");	return (uint8_t*)&State->DX;
	case 3:	DEBUG_S(" BL");	return (uint8_t*)&State->BX;
	case 4:	DEBUG_S(" AH");	return ((uint8_t*)&State->AX)+1;
	case 5:	DEBUG_S(" CH");	return ((uint8_t*)&State->CX)+1;
	case 6:	DEBUG_S(" DH");	return ((uint8_t*)&State->DX)+1;
	case 7:	DEBUG_S(" BH");	return ((uint8_t*)&State->BX)+1;
	}
	return 0;
}

/**
 * \brief Returns a pointer to the specified word register
 * \param State	Emulator State
 * \param num	Register Number
 */
static inline uint16_t *RegW(tRME_State *State, int num)
{
	switch(num)
	{
	case 0:	DEBUG_S(" AX");	return &State->AX;
	case 1:	DEBUG_S(" CX");	return &State->CX;
	case 2:	DEBUG_S(" DX");	return &State->DX;
	case 3:	DEBUG_S(" BX");	return &State->BX;
	case 4:	DEBUG_S(" SP");	return &State->SP;
	case 5:	DEBUG_S(" BP");	return &State->BP;
	case 6:	DEBUG_S(" SI");	return &State->SI;
	case 7:	DEBUG_S(" DI");	return &State->DI;
	}
	return 0;
}

/**
 * \brief Returns a pointer to the specified segment register
 * \param State	Emulator State
 * \param code	Segment register Number (0 to 3)
 */
static uint16_t *Seg(tRME_State *State, int code)
{
	switch(code) {
	case 0:	DEBUG_S(" ES");	return &State->ES;
	case 1:	DEBUG_S(" CS");	return &State->CS;
	case 2:	DEBUG_S(" SS");	return &State->SS;
	case 3:	DEBUG_S(" DS");	return &State->DS;
	default:
		DEBUG_S("ERROR - Invalid value passed to Seg(). (%i is not a segment)", code);
	}
	return NULL;
}

/**
 * \brief Performs a memory addressing function
 * \param State	Emulator State
 * \param mmm	Function ID (mmm field from ModR/M byte)
 * \param disp	Displacement
 * \param ptr	Destination for final pointer
 */
static int DoFunc(tRME_State *State, int mmm, int16_t disp, void *ptr)
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

	switch(mmm)
	{
	case -1:	// R/M == 6 when Mod == 0
		READ_INSTR16( disp );
		DEBUG_S(":[0x%x]", disp);
		addr = disp;
		break;

	case 0:
		DEBUG_S(":[BX+SI+0x%x]", disp);
		addr = State->BX + State->SI + disp;
		break;
	case 1:
		DEBUG_S(":[BX+DI+0x%x]", disp);
		addr = State->BX + State->DI + disp;
		break;
	case 2:
		DEBUG_S(":[BP+SI+0x%x]", disp);
		addr = State->BP + State->SI + disp;
		break;
	case 3:
		DEBUG_S(":[BP+DI+0x%x]", disp);
		addr = State->BP + State->DI + disp;
		break;
	case 4:
		DEBUG_S(":[SI+0x%x]", disp);
		addr = State->SI + disp;
		break;
	case 5:
		DEBUG_S(":[DI+0x%x]", disp);
		addr = State->DI + disp;
		break;
	case 6:
		DEBUG_S(":[BP+0x%x]", disp);
		addr = State->BP + disp;
		break;
	case 7:
		DEBUG_S(":[BX+0x%x]", disp);
		addr = State->BX + disp;
		break;
	}
	return RME_Int_GetPtr(State, seg, addr, ptr);
}

/**
 * \brief Parses the ModR/M byte as a 8-bit value
 * \param State	Emulator State
 * \param to	R field destination (ignored if NULL)
 * \param from	M field destination (ignored if NULL)
 */
int RME_Int_ParseModRMB(tRME_State *State, uint8_t **to, uint8_t **from)
{
	uint8_t	d;
	uint16_t	ofs;
	 int	ret;

	READ_INSTR8(d);
	switch(d >> 6)
	{
	case 0:	//No Offset
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) {
			if((d & 7) == 6)
				ret = DoFunc( State, -1, 0, from );
			else
				ret = DoFunc( State, d & 7, 0, from );
			if(ret)	return ret;
		}
		return 0;
	case 1:	//8 Bit
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) {
			READ_INSTR8S( ofs );
			ret = DoFunc( State, d & 7, ofs, from);
			if(ret)	return ret;
		}
		return 0;
	case 2:	//16 Bit
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) {
			READ_INSTR16( ofs );
			ret = DoFunc( State, d & 7, ofs, from );
			if(ret)	return ret;
		}
		return 0;
	case 3:	//Regs Only
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) *from = RegB( State, d & 7 );
		return 0;
	}
	return 0;
}

/**
 * \brief Parses the ModR/M byte as a 16-bit value
 * \param State	Emulator State
 * \param to	R field destination (ignored if NULL)
 * \param from	M field destination (ignored if NULL)
 */
int RME_Int_ParseModRMW(tRME_State *State, uint16_t **to, uint16_t **from)
{
	uint8_t	d;
	uint16_t	ofs;
	 int	ret;

	READ_INSTR8(d);
	switch(d >> 6)
	{
	case 0:	//No Offset
		if(to)	*to = RegW( State, (d>>3) & 7 );
		if(from) {
			if( (d & 7) == 6 )
				ret = DoFunc( State, -1, 0, from );
			else
				ret = DoFunc( State, d & 7, 0, from );
			if(ret)	return ret;
		}
		return 0;
	case 1:	//8 Bit
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) {
			READ_INSTR8S( ofs );
			ret = DoFunc( State, d & 7, ofs, from );
			if(ret)	return ret;

		}
		return 0;
	case 2:	//16 Bit
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) {
			READ_INSTR16( ofs );
			ret = DoFunc( State, d & 7, ofs, from );
			if(ret)	return ret;
		}
		return 0;
	case 3:	//Regs Only
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) *from = RegW( State, d & 7 );
		return 0;
	}
	return 0;
}

/**
 * \brief Does a conditional jump
 * \param State	Current emulator state
 * \param type	Jump Type (0-15)
 * \param offset	Offset to add to IP if jump succeeds
 * \param name	Jump class name (Short or Near)
 */
static int RME_Int_DoCondJMP(tRME_State *State, uint8_t type, uint16_t offset, const char *name)
{
	DEBUG_S("J");
	switch(type) {
	case 0x0:	DEBUG_S("O");	//Overflow
		if(State->Flags & FLAG_OF)	State->IP += offset;
		break;
	case 0x1:	DEBUG_S("NO");	//No Overflow
		if(!(State->Flags & FLAG_OF))	State->IP += offset;
		break;
	case 0x2:	DEBUG_S("C");	//Carry
		if(State->Flags & FLAG_CF)	State->IP += offset;
		break;
	case 0x3:	DEBUG_S("NC");	//No Carry
		if(!(State->Flags & FLAG_CF))	State->IP += offset;
		break;
	case 0x4:	DEBUG_S("Z");	//Equal
		if(State->Flags & FLAG_ZF)	State->IP += offset;
		break;
	case 0x5:	DEBUG_S("NZ");	//Not Equal
		if(!(State->Flags & FLAG_ZF))	State->IP += offset;
		break;
	case 0x6:	DEBUG_S("BE");	//Below or Equal
		if(State->Flags & FLAG_CF || State->Flags & FLAG_ZF)	State->IP += offset;
		break;
	case 0x7:	DEBUG_S("A");	//Above
		if( !(State->Flags & FLAG_CF) && !(State->Flags & FLAG_ZF))
			State->IP += offset;
		break;

	case 0xC:	DEBUG_S("L");	// Less
		if( !!(State->Flags & FLAG_SF) != !!(State->Flags & FLAG_OF) )
			State->IP += offset;
		break;
	case 0xD:	DEBUG_S("GE");	// Greater or Equal
		if( !!(State->Flags & FLAG_SF) == !!(State->Flags & FLAG_OF) )
			State->IP += offset;
		break;
	case 0xE:	DEBUG_S("LE");	// Less or Equal
		if( State->Flags & FLAG_ZF || !!(State->Flags & FLAG_SF) != !!(State->Flags & FLAG_OF) )
			State->IP += offset;
		break;

	default:
		DEBUG_S(" 0x%x", type);
		return RME_ERR_UNDEFOPCODE;
	}
	DEBUG_S(" %s .+0x%04x", name, offset);
	return 0;
}
