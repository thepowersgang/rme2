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
#include "rme.h"

#define DEBUG	1

// === CONSTANTS ===
#define RME_MAGIC_IP	0xFFFF
#define RME_MAGIC_CS	0xFFFF

#define FLAG_DEFAULT	0x2
#define FLAG_CF	0x001
#define FLAG_PF	0x004
#define FLAG_AF	0x010
#define FLAG_ZF	0x040
#define FLAG_SF	0x080
#define FLAG_TF	0x100
#define FLAG_IF	0x200
#define FLAG_DF	0x400
#define FLAG_OF	0x800

#define STACK(x)	(*(uint16_t*)RME_Int_GetPtr(State, State->SS, (x)))
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
#define PAIRITY32(v)	(PAIRITY8(v) ^ PAIRITY8((v)>>8) ^ PAIRITY8((v)>>16) ^ PAIRITY8((v)>>24))
#define SET_PF(v,w) do{\
	if(w==8)State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;\
	else	State->Flags |= (PAIRITY8(v) ^ PAIRITY8(v>>8)) ? FLAG_PF : 0;\
	}while(0)
// --- Operations
#define RME_Int_DoAdd(State, to, from, width)	do{\
	(to) += (from);\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	State->Flags |= ((to) == 0) ? FLAG_ZF : 0;\
	State->Flags |= ((to) >> ((width)-1)) ? FLAG_SF : 0;\
	State->Flags |= ((to) < (from)) ? FLAG_OF|FLAG_CF : 0;\
	SET_PF((to),(width));\
	}while(0)

// === PROTOTYPES ===
 int	RME_CallInt(tRME_State *State, int Num);
void	RME_DumpRegs(tRME_State *State);
 int	RME_Call(tRME_State *State);
static int	RME_Int_DoOpcode(tRME_State *State);

static inline void	*RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static inline uint8_t	RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static inline uint16_t	RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static int	RME_Int_GenToFromB(tRME_State *State, uint8_t **to, uint8_t **from);
static int	RME_Int_GenToFromW(tRME_State *State, uint16_t **to, uint16_t **from);
static uint16_t	*Seg(tRME_State *State, int code);
static void	DoCondJMP(tRME_State *State, uint8_t type, uint16_t offset);

// === GLOBALS ===
static const char *casReg8Names[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
static const char *casReg16Names[] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};

//=== CODE ===
/**
 * \brief Run Realmode interrupt
 */
int RME_CallInt(tRME_State *State, int Num)
{
	struct {
		uint16_t	offset;
		uint16_t	segment;
	}	*IVT;
	
	DEBUG_S("RM_Int: Calling Int 0x%x\n", Num);
	
	if(Num < 0 || Num > 0xFF) {
		DEBUG_S("WARNING: %i is not a valid interrupt number", Num);
		return -1;
	}
	
	IVT = (void*)State->Memory[0];
	State->CS = IVT[Num].segment;
	State->IP = IVT[Num].offset;
	
	//RME_DumpRegs(State);
	return RME_Call(State);
}

/**
 * \brief Dump Realmode Registers
 */
void RME_DumpRegs(tRME_State *State)
{
	DEBUG_S("\n");
	DEBUG_S("AX = 0x%x\tBX = 0x%x\tCX = 0x%x\tDX = 0x%x\n",
		State->AX, State->BX,
		State->CX, State->DX
		);
	DEBUG_S("SP = 0x%x\tBP = 0x%x\tSI = 0x%x\tDI = 0x%x\n",
		State->SP, State->BP,
		State->SI, State->DI);
	DEBUG_S("SS = 0x%x\tCS = 0x%x\tDS = 0x%x\tES = 0x%x\n",
		State->SS, State->CS,
		State->DS, State->ES);
	DEBUG_S("IP = 0x%x\tFLAGS = 0x%x\n",
		State->IP, State->Flags);
}

/**
 * Call a realmode function (a jump to a magic location is used as the return)
 */
int RME_Call(tRME_State *State)
{
	for(;;)
	{
		if(State->IP == RME_MAGIC_IP && State->CS == RME_MAGIC_CS)
			return 0;
		if(RME_Int_DoOpcode(State) != 0)
			return -1;
	}
}

/**
 * \brief Processes a single instruction
 */
int RME_Int_DoOpcode(tRME_State *State)
{
	uint16_t	pt2, pt1;	// Spare Words, used for values read from memory
	uint16_t	*toW, *fromW;
	 uint8_t	*toB, *fromB;
	 uint8_t	repType = 0;
	uint16_t	repStart = 0;
	 uint8_t	opcode, byte2;
	
	State->Decoder.OverrideSegment = -1;
	
functionTop:

	DEBUG_S("[0x%x] %x:%x ", State->CS*16+State->IP, State->CS, State->IP);
	
	opcode = RME_Int_Read8(State, State->CS, State->IP++);
	DEBUG_S("(0x%x) ", opcode);
	switch( opcode )
	{
	
	// Loops/Repeats
	case REP:	case REPNZ:
	case LOOP:
	case LOOPZ:	case LOOPNZ:
		repType = opcode;
		repStart = State->IP;
		break;
	
	

	#define RME_Int_DoArithOp(num, State, to, from, width)	do{\
		switch( (num) ) {\
		case 0:	RME_Int_DoAdd(State, (to), (from), (width));	break;\
		case 1:	RME_Int_DoOr (State, (to), (from), (width));	break;\
		case 4:	RME_Int_DoAnd(State, (to), (from), (width));	break;\
		case 5:	RME_Int_DoSub(State, (to), (from), (width));	break;\
		case 6:	RME_Int_DoXor(State, (to), (from), (width));	break;\
		case 7:	RME_Int_DoCmp(State, (to), (from), (width));	break;\
		}}while(0)
	
	// <op> MR
	CASE8K(0x00, 0x8):
		RME_Int_GenToFromB(State, &fromB, &toB);
		RME_Int_DoArithOp( opcode >> 3, State, *toB, *fromB, 8 );
		break;
	// <op> MRX
	CASE8K(0x01, 0x8):
		RME_Int_GenToFromW(State, &fromW, &toW);
		RME_Int_DoArithOp( opcode >> 3, State, *toW, *fromW, 16 );
		break;
	
	// <op> RM
	CASE8K(0x02, 0x8):
		RME_Int_GenToFromB(State, &toB, &fromB);
		RME_Int_DoArithOp( opcode >> 3, State, *toB, *fromB, 8 );
		break;
	// <op> RMX
	CASE8K(0x03, 0x8):
		RME_Int_GenToFromW(State, &toW, &fromW);
		RME_Int_DoArithOp( opcode >> 3, State, *toW, *fromW, 16 );
		break;
	
	// <op> AI
	CASE8K(0x04, 8):
		pt2 = RME_Int_Read8(State, State->CS, State->IP);
		State->IP ++;
		RME_Int_DoArithOp( opcode >> 3, State, *(uint8_t*)&State->AX, pt2, 8 );
		break;
	
	// <op> AIX
	CASE8K(0x05, 8):
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		RME_Int_DoArithOp( opcode >> 3, State, State->AX, pt2, 16 );
		break;
	
	// <op> RI
	case 0x80:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2 >> 3) & 7 ) {	// rrr
		case 0:	DEBUG_S("ADD (RI)");	break;
		case 4:	DEBUG_S("AND (RI)");	break;
		case 7: DEBUG_S("CMP (RI)");	break;
		default:	DEBUG_S("%x (RI)", (byte2 >> 3) & 7);	break;
		}
		RME_Int_GenToFromB(State, NULL, &toB);
		
		pt2 = RME_Int_Read8(State, State->CS, State->IP);
		State->IP ++;
		DEBUG_S(" 0x%02x", pt2);
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	// <op> RIX
	case 0x81:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2 >> 3) & 7 ) {	// rrr
		case 0:	DEBUG_S("ADD (RIX)");	break;
		case 4:	DEBUG_S("AND (RIX)");	break;
		case 7: DEBUG_S("CMP (RIX)");	break;
		default:	DEBUG_S("%x (RIX)", (byte2 >> 3) & 7);	break;
		}
		RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S(" 0x%04x", pt2);	//Print Immediate Valiue
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	#if 0
	// TODO - Check if this is valid (and valid for RM)
	case 0x82:	//ADD_RI8
		byte2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		switch( (byte2 >> 3) & 7 ) {	// rrr
		case 0:	DEBUG_S("ADD (RI8)");	break;
		case 5:	DEBUG_S("SUB (RI8)");	break;
		case 7:	DEBUG_S("CMP (RI8)");	break;
		default:	DEBUG_S("UNIMPLEMENTED 0x82");	break;
		}
		RME_Int_GenToFromB(State, NULL, &toB);	//Get Register Value
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S(" 0x%04x", pt2);
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	#endif
	case 0x83:	//ADD_RI8X
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2 >> 3) & 7 ) {	// rrr
		case 0:	DEBUG_S("ADD (RI8X)");	break;
		case 5:	DEBUG_S("SUB (RI8X)");	break;
		case 7:	DEBUG_S("CMP (RI8X)");	break;
		default:	DEBUG_S("UNIMPLEMENTED 0x83");	break;
		}
		RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
		pt2 = (int16_t)RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S(" 0x%04x", pt2);
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toW, pt2, 16 );
		break;
		
	// ==== Logic Functions (Shifts) ===
	#define RME_Int_DoLogicOp( num, State, to, from, width )	do{\
		switch( (num) ) {\
		case 4:	RME_Int_DoShl(State, (to), (from), (width));	break;\
		case 5:	RME_Int_DoShr(State, (to), (from), (width));	break;\
		}}while(0)
	// <op> RI8
	case 0xC0:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromB(State, NULL, &toB);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	// <op> RI8X
	case 0xC1:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromW(State, NULL, &toW);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, pt2, 8 );
		break;
	// <op> R1
	case 0xD0:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromB(State, NULL, &toB);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, 1, 8 );
		break;
	// <op> R1X
	case 0xD1:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromW(State, NULL, &toW);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, 1, 8 );
		break;
	// <op> RCl
	case 0xD2:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromB(State, NULL, &toB);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toB, State->CX&0xFF, 8 );
		break;
	// <op> RClX
	case 0xD3:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		RME_Int_GenToFromW(State, NULL, &toW);
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		RME_Int_DoLogicOp( (byte2 >> 3) & 7, State, *toW, State->CX&0xFF, 8 );
		break;
	
	
	//==== COMPOUND 1st Bytes ====
	case 0x0F:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		State->IP ++;
		
		switch(byte2)
		{
		//--- PUSH Segment --- (100sss000)
		case 0x80:	/*10 000 001 */	case 0x88:	/*10 001 001 */
		case 0x90:	/*10 010 001 */	case 0x98:	/*10 011 001 */
			fromW = Seg(State, (byte2>>3)&3);
			STACK(State->SP-=2) = *fromW;
			break;
		
		//--- POP Segment --- (100sss001)
		case 0x81:	/*10 000 001 */	case 0x89:	/*10 001 001 */
		case 0x91:	/*10 010 001 */	case 0x99:	/*10 011 001 */
			toW = Seg(State, (byte2>>3)&3);
			*toW = STACK(State->SP);	State->SP += 2;
			break;
			
		//--- Near Jump --- (1000cccc)
		//TODO: Check why collision at 80,81,88,89 exists
		//HACK: Ingnoring Overflow and No Overflow
		/*case 0x80:	case 0x81:*/	case 0x82:	case 0x83:
		case 0x84:	case 0x85:	case 0x86:	case 0x87:
		/*case 0x88:	case 0x89:*/	case 0x8A:	case 0x8B:
		case 0x8C:	case 0x8D:	case 0x8E:	case 0x8F:
			DoCondJMP(State, pt2&0xF, RME_Int_Read16(State, State->CS, State->IP));
			State->IP += 2;
			break;
		
		}
		break;
	
	// <op> RI
	case 0xF6:	//Register Immidate
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch((byte2&0x38)>>3) {
		case 0:
			DEBUG_S("TEST (RI)");
			RME_Int_GenToFromB(State, NULL, &toB);
			pt2 = RME_Int_Read8(State, State->CS, State->IP);
			State->IP ++;
			DEBUG_S(" 0x%02x", pt2);
			RME_Int_DoTest(State, *toB, pt2, 8);
			break;
		case 1:	return RME_ERR_UNDEFOPCODE;	// ???
		case 2:	break;	// NOT r/m8
		case 3:	break;	// NEG r/m8
		case 4:	break;	// MUL AX = AL * r/m8
		case 5:	break;	// IMUL AX = AL * r/m8
		case 6:	break;	// DIV AX, r/m8 (unsigned)
		case 7:	break;	// IDIV AX, r/m8 (signed)
		}
		break;
	// <op> RIX
	case 0xF7:	//Register Immidate Extended
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2>>3) & 7 ) {
		case 0:
			DEBUG_S("TEST (RIX)");
			RME_Int_GenToFromW(State, NULL, &toW);
			pt2 = RME_Int_Read16(State, State->CS, State->IP);
			State->IP ++;
			DEBUG_S(" 0x%04x", pt2);
			RME_Int_DoTest(State, *toW, pt2, 16);
			break;
		
		case 2:	break;	// NOT r/m16
		case 3:	break;	// NEG r/m16
		case 4:	break;	// MUL AX, r/m16
		case 5:	break;	// IMUL AX, r/m16
		case 6:	break;	// DIV DX:AX, r/m16
		case 7:	break;	// IDIV DX:AX, r/m16
		}
		break;
		
	case 0xFE:	//Register
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2>>3) & 7 ) {
		case 0:
			DEBUG_S("INC (R)");
			RME_Int_GenToFromB(State, NULL, &toB);	//Get Register Value
			(*toB) ++;
			break;
		case 1:
			DEBUG_S("DEC (R)");
			RME_Int_GenToFromB(State, NULL, &toB);	//Get Register Value
			(*toB) --;
			break;
		default:
			return RME_ERR_UNDEFOPCODE;
		}
		break;
	
	case 0xFF:	//Register Extended
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		switch( (byte2>>3) & 7 ) {
		case 0:
			DEBUG_S("INC (RX)");
			RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
			(*toW) ++;
			break;
		case 1:
			DEBUG_S("DEC (RX)");
			RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
			(*toW) --;
			break;
		case 2:
			DEBUG_S("CALL (RX) NEAR");
			RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
			STACK(State->SP-=2) = State->IP;
			State->IP += *toW;
			break;
		case 3:
			DEBUG_S("CALL (MX) FAR --NI--");
			break;
		case 4:
			DEBUG_S("JMP (RX) NEAR --NI--");
			break;
		case 5:
			DEBUG_S("JMP (MX) FAR --NI--");
			break;
		case 6:
			DEBUG_S("PUSH (RX)");
			RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
			STACK(State->SP-=2) = *toW;
			break;
		case 7:
			return RME_ERR_UNDEFOPCODE;
		}
		break;
		
	//TEST Family
	case TEST_RM:	DEBUG_S("TEST (RR)");	//Test Register
		RME_Int_GenToFromB(State, &toB, &fromB);
		RME_Int_DoTest(State, *toB, *fromB, 8);
		break;
	case TEST_RMX:	DEBUG_S("TEST (RRX)");	//Test Register Extended
		RME_Int_GenToFromW(State, &toW, &fromW);
		RME_Int_DoTest(State, *toW, *fromW, 16);
		break;
	
	//CLI - Ignored
	case CLI:
		DEBUG_S("CLI");
		break;
	
	//DEC Family
	case DEC_A:		DEBUG_S("DEC AX");	State->AX --;	break;
	case DEC_B:		DEBUG_S("DEC BX");	State->BX --;	break;
	case DEC_C:		DEBUG_S("DEC CX");	State->CX --;	break;
	case DEC_D:		DEBUG_S("DEC DX");	State->DX --;	break;
	case DEC_Sp:	DEBUG_S("DEC SP");	State->SP --;	break;
	case DEC_Bp:	DEBUG_S("DEC BP");	State->BP --;	break;
	case DEC_Si:	DEBUG_S("DEC SI");	State->SI --;	break;
	case DEC_Di:	DEBUG_S("DEC DI");	State->DI --;	break;
	//INC Family
	case INC_A:		DEBUG_S("INC AX");	State->AX ++;	break;
	case INC_B:		DEBUG_S("INC BX");	State->BX ++;	break;
	case INC_C:		DEBUG_S("INC CX");	State->CX ++;	break;
	case INC_D:		DEBUG_S("INC DX");	State->DX ++;	break;
	case INC_Sp:	DEBUG_S("INC SP");	State->SP ++;	break;
	case INC_Bp:	DEBUG_S("INC BP");	State->BP ++;	break;
	case INC_Si:	DEBUG_S("INC SI");	State->SI ++;	break;
	case INC_Di:	DEBUG_S("INC DI");	State->DI ++;	break;
	
	//IN Family
	case IN_AI:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("IN (AI) 0x%02x AL", pt2);
		State->AX &= 0xFF00;
		State->AX |= inportb( pt2 );
		break;
	case IN_AIX:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("IN (AI) 0x%02x AX", pt2);
		State->AX = inportw( pt2 );
		break;
	case IN_ADx:
		DEBUG_S("IN (ADx) DX AL");
		State->AX &= 0xFF00;
		State->AX |= inportb(State->DX);
		break;
	case IN_ADxX:
		DEBUG_S("IN (ADxX) DX AX");
		State->AX = inportw(State->DX);
		break;
	//OUT Family
	case OUT_IA:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("OUT (IA) 0x%02x AL", pt2);
		outportb( pt2, State->AX&0xFF );
		State->IP += 1;
		break;
	case OUT_IAX:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("OUT (IAX) 0x%02x AX", pt2);
		outportw( pt2, State->AX );
		State->IP += 2;
		break;
	case OUT_DxA:
		DEBUG_S("OUT (DxA) DX AL");
		outportb( State->DX, State->AX&0xFF );
		break;
	case OUT_DxAX:
		DEBUG_S("OUT (DxAX) DX AX");
		outportb( State->DX, State->AX );
		break;
	
	//INT Family
	case INT3:
		DEBUG_S("INT 3");
		//debugRealMode(r);
		break;
	case INT_I:
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		State->IP ++;
		DEBUG_S("INT 0x%02x", byte2);
		// Todo: Nested Interrupts
		DEBUG_S("TODO: Interrupts");
		break;
	case IRET:
		DEBUG_S("IRET");
		// Todo: Return from nested interrupts
		break;
		
	//MOV Family
	case MOV_MoA:	// Store AL at Memory Offset
		DEBUG_S("MOV (MoA)");
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S(" DS:0x%04x AX", pt2);
		*(uint8_t*)RME_Int_GetPtr(State, State->DS, pt2) = State->AX & 0xFF;
		break;
	case MOV_MoAX:	//Store AX at Memory Offset
		DEBUG_S("MOV (MoAX)");
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S(" DS:0x%04x AX", pt2);
		*(uint16_t*)RME_Int_GetPtr(State, State->DS, pt2) = State->AX;
	case MOV_AMo:	//Memory Offset to AL
		DEBUG_S("MOV (AMo)");
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S("AX DS:0x%04x", pt2);
		State->AX &= 0xFF00;
		State->AX |= *(uint8_t*)RME_Int_GetPtr(State, State->DS, pt2);
		break;
	case MOV_AMoX:	//Memory Offset to AX
		DEBUG_S("MOV (AMoX)");
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S(" AX DS:0x%04x", pt2);
		State->AX = *(uint16_t*)RME_Int_GetPtr(State, State->DS, pt2);
		break;
	case MOV_MI:
		DEBUG_S("MOV (MI)");
		RME_Int_GenToFromB(State, &toB, NULL);
		*toB = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		break;
	case MOV_MIX:
		DEBUG_S("MOV (MIX)");
		RME_Int_GenToFromW(State, &toW, NULL);
		*toW = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		break;
	case MOV_RM:
		DEBUG_S("MOV (RR)");
		RME_Int_GenToFromB(State, &toB, &fromB);
		*toB = *fromB;
		break;
	case MOV_RMX:
		DEBUG_S("MOV (RRX)");
		RME_Int_GenToFromW(State, &toW, &fromW);
		*toW = *fromW;
		break;
	case MOV_MR:	DEBUG_S("MOV (MR)REV");
		RME_Int_GenToFromB(State, &fromB, &toB);
		*toB = *fromB;
		break;
	case MOV_MRX:	DEBUG_S("MOV (MRX)REV");
		RME_Int_GenToFromW(State, &fromW, &toW);
		*toW = *fromW;
		break;
	
	case MOV_RI_AL:	case MOV_RI_CL:
	case MOV_RI_DL:	case MOV_RI_BL:
	case MOV_RI_AH:	case MOV_RI_CH:
	case MOV_RI_DH:	case MOV_RI_BH:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("MOV (RI) %s 0x%02x", casReg8Names[opcode&7], pt2);
		if(opcode&4)
			State->GPRs[opcode&3] = (State->GPRs[opcode&3]&0xFF00) | pt2;
		else
			State->GPRs[opcode&3] = (State->GPRs[opcode&3]&0xFF) | (pt2<<8);
		break;
	
	case MOV_RI_AX:	case MOV_RI_CX:
	case MOV_RI_DX:	case MOV_RI_BX:
	case MOV_RI_SP:	case MOV_RI_BP:
	case MOV_RI_SI:	case MOV_RI_DI:
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("MOV (RIX) %s 0x%04x", casReg16Names[opcode&7], pt2);
		State->GPRs[opcode&7] = pt2;
		break;
	
	case MOV_RS:
		DEBUG_S("MOV (RS)");
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		fromW = Seg(State, (byte2>>3)&7);
		RME_Int_GenToFromW(State, NULL, &toW);
		*toW = *fromW;
		break;
		
	case MOV_SR:
		DEBUG_S("MOV (SR)");
		byte2 = RME_Int_Read8(State, State->CS, State->IP);
		toW = Seg(State, (byte2>>3)&7);
		RME_Int_GenToFromW(State, NULL, &fromW);
		*toW = *fromW;
		break;
		
		
	//JMP Family
	case JMP_S:	// Short Jump
		pt2 = (int8_t)RME_Int_Read8(State, State->CS, State->IP);
		State->IP ++;
		DEBUG_S("JMP (S) .+0x%04x", pt2);
		State->IP += pt2;
		break;
	case JMP_N:	// Near Jump
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("JMP (N) .+0x%04x", pt2 );
		State->IP += pt2;
		break;
	case JMP_F:	// Far Jump
		pt1 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("JMP FAR %04x:%04x", pt2, pt1);
		State->CS = pt2;	State->IP = pt1;
		break;
	
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
		RME_Int_GenToFromW(State, &toW, &fromW);
		pt2 = *toW;		*toW = *fromW;	*fromW = pt2;
		break;
		
	//PUSH Family
	case PUSHF:
		DEBUG_S("PUSHF");
		STACK(State->SP-=2) = State->Flags;
		break;
	case PUSHA:
		DEBUG_S("PUSHA");
		STACK(State->SP-=2) = State->AX;	STACK(State->SP-=2) = State->BX;
		STACK(State->SP-=2) = State->CX;	STACK(State->SP-=2) = State->DX;
		STACK(State->SP-=2) = State->SP;	STACK(State->SP-=2) = State->BP;
		STACK(State->SP-=2) = State->SI;	STACK(State->SP-=2) = State->DI;
		//STACK(State->SP-=2) = r->ss;	STACK(State->SP-=2) = r->cs;
		//STACK(State->SP-=2) = r->ds;	STACK(State->SP-=2) = r->es;
		break;
	case PUSH_AX:	DEBUG_S("PUSH AX");	STACK(State->SP-=2) = State->AX;	break;
	case PUSH_BX:	DEBUG_S("PUSH BX");	STACK(State->SP-=2) = State->BX;	break;
	case PUSH_CX:	DEBUG_S("PUSH CX");	STACK(State->SP-=2) = State->CX;	break;
	case PUSH_DX:	DEBUG_S("PUSH DX");	STACK(State->SP-=2) = State->DX;	break;
	case PUSH_SP:	DEBUG_S("PUSH Sp");	STACK(State->SP) = State->SP;	State->SP-=2;	break;
	case PUSH_BP:	DEBUG_S("PUSH BP");	STACK(State->SP-=2) = State->BP;	break;
	case PUSH_SI:	DEBUG_S("PUSH SI");	STACK(State->SP-=2) = State->SI;	break;
	case PUSH_DI:	DEBUG_S("PUSH DI");	STACK(State->SP-=2) = State->DI;	break;
	case PUSH_ES:	DEBUG_S("PUSH ES");	STACK(State->SP-=2) = State->ES;	break;
	case PUSH_CS:	DEBUG_S("PUSH CS");	STACK(State->SP-=2) = State->CS;	break;
	case PUSH_SS:	DEBUG_S("PUSH SS");	STACK(State->SP-=2) = State->SS;	break;
	case PUSH_DS:	DEBUG_S("PUSH DS");	STACK(State->SP-=2) = State->DS;	break;
	case PUSH_I8:
		pt2 = RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		DEBUG_S("PUSH (I8) 0x%02x", pt2);
		STACK(State->SP-=2) = pt2;
		break;
	case PUSH_I:
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("PUSH (I) 0x%04x", pt2);
		STACK(State->SP-=2) = pt2;
		break;		

	//POP Family
	case POPF:
		DEBUG_S("POPF");
		State->Flags = STACK(State->SP);	State->SP += 2;
		break;
	case POPA:
		DEBUG_S("POPA");
		//r->es = STACK(State->SP); State->SP+=2;	r->ds = STACK(State->SP); State->SP+=2;
		//r->cs = STACK(State->SP); State->SP+=2;	r->ss = STACK(State->SP); State->SP+=2;
		State->DI = STACK(State->SP); State->SP+=2;	State->SI = STACK(State->SP); State->SP+=2;
		State->BP = STACK(State->SP); State->SP+=2;	State->SP = STACK(State->SP); State->SP+=2;
		State->DX = STACK(State->SP); State->SP+=2;	State->CX = STACK(State->SP); State->SP+=2;
		State->BX = STACK(State->SP); State->SP+=2;	State->AX = STACK(State->SP); State->SP+=2;
		break;
	case POP_AX:	DEBUG_S("POP AX");	State->AX = STACK(State->SP); State->SP+=2;	break;
	case POP_BX:	DEBUG_S("POP BX");	State->BX = STACK(State->SP); State->SP+=2;	break;
	case POP_CX:	DEBUG_S("POP CX");	State->CX = STACK(State->SP); State->SP+=2;	break;
	case POP_DX:	DEBUG_S("POP DX");	State->DX = STACK(State->SP); State->SP+=2;	break;
	case POP_SP:	DEBUG_S("POP SP");	State->SP += 2;	break;	// POP SP does practically nothing
	case POP_BP:	DEBUG_S("POP BP");	State->BP = STACK(State->SP); State->SP+=2;	break;
	case POP_SI:	DEBUG_S("POP SI");	State->SI = STACK(State->SP); State->SP+=2;	break;
	case POP_DI:	DEBUG_S("POP DI");	State->DI = STACK(State->SP); State->SP+=2;	break;
//	case POP_CS:	DEBUG_S("POP CS");	State->CS = STACK(State->SP); State->SP+=2;	break;
	case POP_ES:	DEBUG_S("POP ES");	State->ES = STACK(State->SP); State->SP+=2;	break;
	case POP_SS:	DEBUG_S("POP SS");	State->SS = STACK(State->SP); State->SP+=2;	break;
	case POP_DS:	DEBUG_S("POP DS");	State->DS = STACK(State->SP); State->SP+=2;	break;
	
	//CALL Family
	/*case CALL_MF:	//MF, MN, R
		DEBUG_S("CALL (MF) ");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), NULL, &toW);
		STACK(State->SP-=2) = State->IP;
		State->IP += *toW;
		break;*/
	case CALL_N:
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("CALL (N) .+0x%04x", pt2);
		STACK(State->SP-=2) = State->IP;
		State->IP += pt2;
		break;
	case CALL_F:
		pt1 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		pt2 = RME_Int_Read16(State, State->CS, State->IP);	State->IP += 2;
		DEBUG_S("CALL (F) %04x:%04x", pt2, pt1);
		STACK(State->SP-=2) = State->CS;
		STACK(State->SP-=2) = State->IP;
		State->CS = pt2;
		State->IP = pt1;
		break;
	case RET_N:
		DEBUG_S("RET (N)");
		State->IP = STACK(State->SP); State->SP+=2;
		break;
	case RET_F:
		DEBUG_S("RET (F)");
		State->IP = STACK(State->SP); State->SP += 2;
		State->CS = STACK(State->SP); State->SP += 2;
		break;
	
	//STOS Functions
	case STOSB:
		*(uint8_t*)RME_Int_GetPtr(State, State->ES, State->DI) = State->AX & 0xFF;
		State->DI ++;
		break;
	case STOSW:
		*(uint16_t*)RME_Int_GetPtr(State, State->ES, State->DI) = State->AX;
		State->DI += 2;
		break;
	
	// Misc
	case LEA:
		DEBUG_S("LEA");
		// HACK - LEA needs to parse the address itself, because it
		// needs the emulated address, not the true address;
		byte2 = RME_Int_Read8(State, State->CS, State->IP++);
		switch(byte2 >> 6)
		{
		case 0:	// No Offset
			pt1 = 0;
			break;
		case 1:
			pt1 = (int8_t)RME_Int_Read8(State, State->CS, State->IP);
			State->IP ++;
			break;
		case 2:
			pt1 = RME_Int_Read8(State, State->CS, State->IP);
			State->IP += 2;
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
			DEBUG_S(" SS:[BP+0x%x]", pt1);
			pt1 += State->BP;
			break;
		case 7:
			DEBUG_S(" DS:[BX+0x%x]", pt1);
			pt1 += State->BX;
			break;
		}
		break;
	
	// Flag Manipulation
	case CLD:
		DEBUG_S("CLD");
		State->Flags &= ~FLAG_DF;
		break;
	
	//Prefixes
	case 0x66:	//Operand Size Override
		DEBUG_S("PREFIX: OPRADND OVERRIDE");
		break;
	case 0x67:	//Memory Size Override
		DEBUG_S("PREFIX: ADDR OVERRIDE");
		break;
	
	// Short Jumps
	CASE16(0x70):
		DEBUG_S("J");
		pt2 = (int8_t)RME_Int_Read8(State, State->CS, State->IP);	State->IP ++;
		switch(opcode & 0xF)
		{
		case 0:	//Overflow
			DEBUG_S("O");
			if( State->Flags & FLAG_OF )	State->IP += pt2;
			break;
		case 1:	//No Overflow
			DEBUG_S("NO");
			if(!(State->Flags & FLAG_OF))	State->IP += pt2;
			break;
		case 2:	//Carry
			DEBUG_S("C");
			if( State->Flags & FLAG_CF )	State->IP += pt2;
			break;
		case 3:	//No Carry
			DEBUG_S("NC");
			if(!(State->Flags & FLAG_CF))	State->IP += pt2;
			break;	
		case 4:	//Equal
			DEBUG_S("Z");
			if( State->Flags & FLAG_ZF )	State->IP += pt2;
			break;
		case 5:	//Not Equal
			DEBUG_S("NZ");
			if(!(State->Flags & FLAG_ZF))	State->IP += pt2;
			break;
		case 6:	//Below or Equal
			DEBUG_S("BE");
			if( State->Flags & FLAG_CF || State->Flags & FLAG_ZF )	State->IP += pt2;
			break;
		case 7:	//Above
			DEBUG_S("A");
			if(!(State->Flags & FLAG_CF || State->Flags & FLAG_ZF))	State->IP += pt2;
			break;
		
		}
		DEBUG_S(" (S) .+0x%04x", pt2);
		break;
	
	default:
		DEBUG_S("Unkown Opcode IP = 0x%02x", opcode);
		return RME_ERR_UNDEFOPCODE;
	}
	DEBUG_S("\n");
	
	
	switch(repType)
	{
	case REP:
		if(State->Flags & FLAG_ZF && State->CX--)
		{
			State->IP = repStart;
			goto functionTop;
		}
		break;
	case REPNZ:
		if( !(State->Flags & FLAG_ZF) && State->CX--)
		{
			State->IP = repStart;
			goto functionTop;
		}
		break;
	case LOOP:
		if(State->CX != 0)
		{
			State->IP = repStart;
			goto functionTop;
		}
		break;
	case LOOPZ:
		if(State->Flags & FLAG_ZF)
		{
			State->IP = repStart;
			goto functionTop;
		}
		break;
	case LOOPNZ:
		if( !(State->Flags & FLAG_ZF) )
		{
			State->IP = repStart;
			goto functionTop;
		}
		break;
	}
	
	return 0;
}

static inline void	*RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint16_t Ofs)
{
	uint32_t	addr = Seg * 16 + Ofs;
	return &State->Memory[addr>>16][addr&0xFFFF];
}

static inline uint8_t RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs)
{
	uint32_t	addr = Seg * 16 + Ofs;
	return State->Memory[addr>>16][addr&0xFFFF];
}

static inline uint16_t RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs)
{
	uint32_t	addr = Seg * 16 + Ofs;
	return *(uint16_t*)&State->Memory[addr>>16][addr&0xFFFF];
}

static inline uint8_t *RegB(tRME_State *State, int num)
{
	switch(num)
	{
	case 0:	DEBUG_S(" AL");	return (uint8_t*)&State->AX;
	case 1:	DEBUG_S(" CL");	return (uint8_t*)&State->CX;
	case 2:	DEBUG_S(" DL");	return (uint8_t*)&State->DX;
	case 3:	DEBUG_S(" BL");	return (uint8_t*)&State->BX;
	case 4:	DEBUG_S(" AH");	return (uint8_t*)(&State->AX)+1;
	case 5:	DEBUG_S(" CH");	return (uint8_t*)(&State->CX)+1;
	case 6:	DEBUG_S(" DH");	return (uint8_t*)(&State->DX)+1;
	case 7:	DEBUG_S(" BH");	return (uint8_t*)(&State->BX)+1;
	}
	return 0;
}

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

static uint16_t *Seg(tRME_State *State, int code)
{
	switch(code) {
	case 0:	DEBUG_S(" ES");	return &State->ES;
	case 1:	DEBUG_S(" CS");	return &State->CS;
	case 2:	DEBUG_S(" SS");	return &State->SS;
	case 3:	DEBUG_S(" DS");	return &State->DS;
	}
	return 0;
}

static uint16_t *DoFunc(tRME_State *State, int mmm, int16_t disp)
{
	uint32_t	addr;
	switch(mmm)
	{
	case 0:
		DEBUG_S(" DS:[BX+SI+0x%x]", disp);
		addr = (State->DS << 4) + State->BX + State->SI + disp;
		break;
	case 1:
		DEBUG_S(" DS:[BX+DI+0x%x]", disp);
		addr = (State->DS << 4) + State->BX + State->DI + disp;
		break;
	case 2:
		DEBUG_S(" SS:[BP+SI+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + State->SI + disp;
		break;
	case 3:
		DEBUG_S(" SS:[BP+DI+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + State->DI + disp;
		break;
	case 4:
		DEBUG_S(" DS:[SI+0x%x]", disp);
		addr = (State->DS << 4) + State->SI + disp;
		break;
	case 5:
		DEBUG_S(" DS:[DI+0x%x]", disp);
		addr = (State->DS << 4) + State->DI + disp;
		break;
	case 6:
		DEBUG_S(" SS:[BP+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + disp;
		break;
	case 7:
		DEBUG_S(" DS:[BX+0x%x]", disp);
		addr = (State->DS << 4) + State->BX + disp;
		break;
	}
	return (uint16_t*)&State->Memory[addr>>16][addr&0xFFFF];
}

int RME_Int_GenToFromB(tRME_State *State, uint8_t **to, uint8_t **from)
{
	 int	d = RME_Int_Read8(State, State->CS, State->IP);
	State->IP ++;
	switch((d >> 6)&3)
	{
	case 0:	//No Offset
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) *from = (uint8_t *) DoFunc( State, d & 7, 0 );
		return 1;
	case 1:	//8 Bit
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) {
			*from = (uint8_t *) DoFunc( State, d & 7, (int8_t)RME_Int_Read8(State, State->CS, State->IP) );
			State->IP ++;
		}
		return 2;
	case 2:	//16 Bit
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) {
			*from = (uint8_t *) DoFunc( State, d & 7, RME_Int_Read16(State, State->CS, State->IP) );
			State->IP += 2;
		}
		return 3;
	case 3:	//Regs Only
		if(to) *to = RegB( State, (d>>3) & 7 );
		if(from) *from = RegB( State, d & 7 );
		return 1;
	}
	return 0;
}

int RME_Int_GenToFromW(tRME_State *State, uint16_t **to, uint16_t **from)
{
	 int	d = RME_Int_Read8(State, State->CS, State->IP);
	switch(d >> 6)
	{
	case 0:	//No Offset
		if(to)	*to = RegW( State, (d>>3) & 7 );
		if(from) *from = DoFunc( State, d & 7, 0 );
		return 1;
	case 1:	//8 Bit
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) {
			*from = (uint16_t *) DoFunc( State, d & 7, (int8_t)RME_Int_Read8(State, State->CS, State->IP) );
			State->IP ++;
		}
		return 2;
	case 2:	//16 Bit
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) {
			*from = (uint16_t *) DoFunc( State, d & 7, RME_Int_Read16(State, State->CS, State->IP) );
			State->IP += 2;
		}
		return 3;
	case 3:	//Regs Only
		if(to) *to = RegW( State, (d>>3) & 7 );
		if(from) *from = RegW( State, d & 7 );
		return 1;
	}
	return 0;
}

static void DoCondJMP(tRME_State *State, uint8_t type, uint16_t offset)
{
	DEBUG_S("J");
	switch(type) {
	case 0:	DEBUG_S("O");	//Overflow
		if(State->Flags & FLAG_OF)	State->IP += offset;
		break;
	case 1:	DEBUG_S("NO");	//No Overflow
		if(!(State->Flags & FLAG_OF))	State->IP += offset;
		break;
	case 2:	DEBUG_S("C");	//Carry
		if(State->Flags & FLAG_CF)	State->IP += offset;
		break;
	case 3:	DEBUG_S("NC");	//No Carry
		if(!(State->Flags & FLAG_CF))	State->IP += offset;
		break;	
	case 4:	DEBUG_S("Z");	//Equal
		if(State->Flags & FLAG_ZF)	State->IP += offset;
		break;
	case 5:	DEBUG_S("NZ");	//Not Equal
		if(!(State->Flags & FLAG_ZF))	State->IP += offset;
		break;
	case 6:	DEBUG_S("BE");	//Below or Equal
		if(State->Flags & FLAG_CF || State->Flags & FLAG_ZF)	State->IP += offset;
		break;
	case 7:	DEBUG_S("A");	//Above
		if( !(State->Flags & FLAG_CF) && !(State->Flags & FLAG_ZF))
			State->IP += offset;
		break;
	
	}
	DEBUG_S(" NEAR .+0x%04x", offset);
}
