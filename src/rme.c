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
static inline void	*RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static inline uint8_t	RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static inline uint16_t	RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs);
static int	RME_Int_GenToFromB(tRME_State *State, uint8_t **to, uint8_t **from);
static int	RME_Int_GenToFromW(tRME_State *State, uint16_t **to, uint16_t **from);
static uint16_t	*Seg(tRME_State *State, int code);
static void	DoCondJMP(tRME_State *State, uint8_t type, uint16_t offset);

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
	RME_Call(State);
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
	uint16_t	pt2;
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
		RME_Int_DoALUOp( opcode >> 3, State, *(uint8_t*)&State->AX, pt2, 8 );
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
		default:	putHex((byte2 >> 3) & 7);	break;
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
		default:	putHex((byte2 >> 3) & 7);	break;
		}
		RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
		pt2 = RME_Int_Read16(State, State->CS, State->IP);
		State->IP += 2;
		DEBUG_S(" 0x%04x", pt2);	//Print Immediate Valiue
		RME_Int_DoArithOp( (byte2 >> 3) & 7, State, *toB, pt2, 8 );
		break;
	#if 0
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
			*toB ++;
			break;
		case 1:
			DEBUG_S("DEC (R)");
			RME_Int_GenToFromB(State, NULL, &toB);	//Get Register Value
			*toB --;
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
			*toW ++;
			break;
		case 1:
			DEBUG_S("DEC (RX)");
			RME_Int_GenToFromW(State, NULL, &toW);	//Get Register Value
			*toW --;
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
		State->IP += genToFromB(r, (uint8_t*)(code+State->IP), &toB, &fromB);
		pt2 = *toB & *fromB;
		State->Flags &= ~(FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
		if(pt2 == 0)	State->Flags |= FLAG_ZF;
		if(pt2 & 0x80)	State->Flags |= FLAG_SF;
		while( pt2 >>= 1 )
			if(pt2&1)	State->Flags ^= FLAG_PF;	//Set Parity
		break;
	case TEST_RMX:	DEBUG_S("TEST (RRX)");	//Test Register Extended
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &toW, &fromW);
		pt2 = *toW & *fromW;
		State->Flags &= ~(FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
		if(pt2 == 0)	State->Flags |= FLAG_ZF;
		if(pt2 & 0x8000)	State->Flags |= FLAG_SF;
		while( pt2 >>= 1 )
			if(pt2&1)	State->Flags ^= FLAG_PF;	//Set Parity
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
	case DEC_Sp:	DEBUG_S("DEC SP");	r->sp --;	break;
	case DEC_Bp:	DEBUG_S("DEC BP");	r->bp --;	break;
	case DEC_Si:	DEBUG_S("DEC SI");	r->si --;	break;
	case DEC_Di:	DEBUG_S("DEC DI");	r->di --;	break;
	//INC Family
	case INC_A:		DEBUG_S("INC AX");	State->AX ++;	break;
	case INC_B:		DEBUG_S("INC BX");	State->BX ++;	break;
	case INC_C:		DEBUG_S("INC CX");	State->CX ++;	break;
	case INC_D:		DEBUG_S("INC DX");	State->DX ++;	break;
	case INC_Sp:	DEBUG_S("INC SP");	r->sp ++;	break;
	case INC_Bp:	DEBUG_S("INC BP");	r->bp ++;	break;
	case INC_Si:	DEBUG_S("INC SI");	r->si ++;	break;
	case INC_Di:	DEBUG_S("INC DI");	r->di ++;	break;
	
	//IN Family
	case IN_AI:
		DEBUG_S("IN (AI) 0x"); DEBUG_H(*(uint8_t*)(code+State->IP)); DEBUG_S(" AL");
		State->AX &= 0xFF00;
		State->AX |= inportb( *(uint8_t*)(code+State->IP) );
		State->IP += 1;
		break;
	case IN_AIX:
		DEBUG_S("IN (AIX) 0x"); DEBUG_H(*(uint16_t*)(code+State->IP)); DEBUG_S(" AX");
		State->AX = inportw( *(uint16_t*)(code+State->IP) );
		State->IP += 2;
		break;
	case IN_ADx:
		DEBUG_S("IN (ADx) DX AL");
		State->AX &= 0xFF00;
		State->AX |= inportb(r->dx);
		break;
	case IN_ADxX:
		DEBUG_S("IN (ADxX) DX AX");
		State->AX = inportw(r->dx);
		break;
	//OUT Family
	case OUT_IA:
		DEBUG_S("OUT (IA) 0x"); DEBUG_H(*(uint8_t*)(code+State->IP)); DEBUG_S(" AL");
		outportb( *(uint8_t*)(code+State->IP), State->AX&0xFF );
		State->IP += 1;
		break;
	case OUT_IAX:
		DEBUG_S("OUT (IAX) 0x"); DEBUG_H(*(uint16_t*)(code+State->IP)); DEBUG_S(" AX");
		outportw( *(uint16_t*)(code+State->IP), State->AX );
		State->IP += 2;
		break;
	case OUT_DxA:
		DEBUG_S("OUT (DxA) DX AL");
		outportb( r->dx, State->AX&0xFF );
		break;
	case OUT_DxAX:
		DEBUG_S("OUT (DxAX) DX AX");
		outportb( r->dx, State->AX );
		break;
	
	//INT Family
	case INT3:
		DEBUG_S("INT 3");
		//debugRealMode(r);
		break;
	case INT_I:
		DEBUG_S("INT 0x");	DEBUG_H(code[State->IP]);
		State->IP++;
		break;
	case IRET:
		DEBUG_S("IRET\n");
		return;
		break;
		
	//MOV Family
	case MOV_MoA:	DEBUG_S("MOV (MoA)");	//Memory Offset from AL
		pt2 = *(uint16_t*)(code+State->IP);
		DEBUG_S(" DS:0x");	DEBUG_H(pt2);	DEBUG_S(" AL");
		*(uint8_t*)(r->ds*16+pt2) = State->AX&0xFF;
		State->IP += 2;
		break;
	case MOV_MoAX:	//Memory Offset from AX
		DEBUG_S("MOV (MoAX)");
		pt2 = *(uint16_t*)(code+State->IP);
		DEBUG_S(" DS:0x");	DEBUG_H(pt2);	DEBUG_S(" AX");
		*(uint16_t*)(r->ds*16+pt2) = State->AX;
		State->IP += 2;
	case MOV_AMo:	//Memory Offset to AL
		DEBUG_S("MOV (AMo)");
		pt2 = *(uint16_t*)(code+State->IP);
		DEBUG_S("AX DS:0x");	DEBUG_H(pt2);
		State->AX &= 0xFF00;
		State->AX |= *(uint8_t*)(r->ds*16+pt2);
		State->IP += 2;
		break;
	case MOV_AMoX:	//Memory Offset to AX
		DEBUG_S("MOV (AMoX)");
		pt2 = *(uint16_t*)(code+State->IP);
		DEBUG_S(" AX DS:0x");	DEBUG_H(pt2);
		State->AX = *(uint16_t*)(r->ds*16+pt2);
		State->IP += 2;
		break;
	case MOV_MI:
		DEBUG_S("MOV (MI)");
		State->IP += genToFromB(r, (uint8_t*)(code+State->IP), &toB, NULL);
		*toB = code[State->IP];
		State->IP ++;
		break;
	case MOV_MIX:
		DEBUG_S("MOV (MIX)");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &toW, NULL);
		*toW = *(uint16_t*)(code+State->IP);
		State->IP += 2;
		break;
	case MOV_RM:
		DEBUG_S("MOV (RR)");
		State->IP += genToFromB(r, (uint8_t*)(code+State->IP), &toB, &fromB);
		*toB = *fromB;
		break;
	case MOV_RMX:
		DEBUG_S("MOV (RRX)");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &toW, &fromW);
		*toW = *fromW;
		break;
	case MOV_MR:	DEBUG_S("MOV (MR)REV");
		RME_Int_GenToFromB(State, &toB, &fromB);
		*toB = *fromB;
		break;
	case MOV_MRX:	DEBUG_S("MOV (MRX)REV");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &fromW, &toW);
		*toW = *fromW;
		break;
	case MOV_RI_AL:	DEBUG_S("MOV (RI) AL 0x");	DEBUG_H(code[State->IP]);
		State->AX &= 0xFF00;	State->AX |= code[State->IP];
		State->IP ++;
		break;
	case MOV_RI_BL:	DEBUG_S("MOV (RI) BL 0x");	DEBUG_H(code[State->IP]);
		State->BX &= 0xFF00;	State->BX |= code[State->IP];
		State->IP ++;
		break;
	case MOV_RI_CL:	DEBUG_S("MOV (RI) CL 0x");	DEBUG_H(code[State->IP]);
		State->CX &= 0xFF00;	State->CX |= code[State->IP];
		State->IP ++;
		break;
	case MOV_RI_DL:	DEBUG_S("MOV (RI) DL 0x");	DEBUG_H(code[State->IP]);
		r->dx &= 0xFF00;	r->dx |= code[State->IP];
		State->IP ++;
		break;
	case MOV_RI_AH:	DEBUG_S("MOV (RI) AH 0x");	DEBUG_H(code[State->IP]);
		State->AX &= 0xFF;	State->AX |= code[State->IP]<<8;
		State->IP ++;
		break;
	case MOV_RI_BH:	DEBUG_S("MOV (RI) BH 0x");	DEBUG_H(code[State->IP]);
		State->BX &= 0xFF;	State->BX |= code[State->IP]<<8;
		State->IP ++;
		break;
	case MOV_RI_CH:	DEBUG_S("MOV (RI) CH 0x");	DEBUG_H(code[State->IP]);
		State->CX &= 0xFF;	State->CX |= code[State->IP]<<8;
		State->IP ++;
		break;
	case MOV_RI_DH:	DEBUG_S("MOV (RI) DH 0x");	DEBUG_H(code[State->IP]);
		r->dx &= 0xFF;	r->dx |= code[State->IP]<<8;
		State->IP ++;
		break;
	case MOV_RI_AX:	DEBUG_S("MOV (RI) AX 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		State->AX = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_BX:	DEBUG_S("MOV (RI) BX 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		State->BX = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_CX:	DEBUG_S("MOV (RI) CX 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		State->CX = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_DX:	DEBUG_S("MOV (RI) DX 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		r->dx = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_SP:	DEBUG_S("MOV (RI) SP 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		r->sp = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_BP:	DEBUG_S("MOV (RI) BP 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		r->bp = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_SI:	DEBUG_S("MOV (RI) SI 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		r->si = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	case MOV_RI_DI:	DEBUG_S("MOV (RI) DI 0x");	DEBUG_H(*(uint16_t*)(code+State->IP));
		r->di = *(uint16_t*)(code+State->IP);		State->IP += 2;
		break;
	
	case MOV_RS:
		DEBUG_S("MOV (RS)");
		fromW = Seg(State, (code[State->IP]>>3)&7);
		RME_Int_GenToFromW(State, NULL, &toW);
		*toW = *fromW;
		break;
		
	case MOV_SR:
		DEBUG_S("MOV (SR)");
		toW = Seg(State, (code[State->IP]>>3)&7);
		RME_Int_GenToFromW(State, NULL, &fromW);
		*toW = *fromW;
		break;
		
		
	//JMP Family
	case JMP_S:	//Short Jump
		DEBUG_S("JMP (S) .+0x");DEBUG_H(code[State->IP]);
		State->IP += *(uint8_t*)(code+State->IP) + 1;
		break;
	case JMP_N:	//Near Jump
		DEBUG_S("JMP (S) .+0x");DEBUG_H( *(uint16_t*)(code+State->IP) );
		State->IP += *(uint16_t*)(code+State->IP) + 2;
		break;
	case JMP_F:	//Near Jump
		DEBUG_S("JMP FAR ");DEBUG_H( *(uint16_t*)(code+State->IP+2) );
		DEBUG_C(':');DEBUG_H( *(uint16_t*)(code+State->IP) );
		r->cs = *(uint16_t*)(code+State->IP+2);
		State->IP = *(uint16_t*)(code+State->IP);
		code = (uint8_t*)(r->cs<<4);
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
		pt2 = State->AX;	State->AX = r->dx;	r->dx = pt2;
		break;
	case XCHG_ASp:
		DEBUG_S("XCHG AX SP");
		pt2 = State->AX;	State->AX = r->sp;	r->sp = pt2;
		break;
	case XCHG_ABp:
		DEBUG_S("XCHG AX BP");
		pt2 = State->AX;	State->AX = r->bp;	r->bp = pt2;
		break;
	case XCHG_ASi:
		DEBUG_S("XCHG AX SI");
		pt2 = State->AX;	State->AX = r->si;	r->si = pt2;
		break;
	case XCHG_ADi:
		DEBUG_S("XCHG AX DI");
		pt2 = State->AX;	State->AX = r->di;	r->di = pt2;
		break;
	case XCHG_RM:
		DEBUG_S("XCHG (RR)");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &toW, &fromW);
		pt2 = *toW;		*toW = *fromW;	*fromW = pt2;
		break;
		
	//PUSH Family
	case PUSHF:
		DEBUG_S("PUSHF");
		STACK(r->sp-=2) = State->Flags;
		break;
	case PUSHA:
		DEBUG_S("PUSHA");
		STACK(r->sp-=2) = State->AX;	STACK(r->sp-=2) = State->BX;
		STACK(r->sp-=2) = State->CX;	STACK(r->sp-=2) = r->dx;
		STACK(r->sp-=2) = r->sp;	STACK(r->sp-=2) = r->bp;
		STACK(r->sp-=2) = r->si;	STACK(r->sp-=2) = r->di;
		//STACK(r->sp-=2) = r->ss;	STACK(r->sp-=2) = r->cs;
		//STACK(r->sp-=2) = r->ds;	STACK(r->sp-=2) = r->es;
		break;
	case PUSH_AX:	DEBUG_S("PUSH AX");	STACK(r->sp-=2) = State->AX;	break;
	case PUSH_BX:	DEBUG_S("PUSH BX");	STACK(r->sp-=2) = State->BX;	break;
	case PUSH_CX:	DEBUG_S("PUSH CX");	STACK(r->sp-=2) = State->CX;	break;
	case PUSH_DX:	DEBUG_S("PUSH DX");	STACK(r->sp-=2) = r->dx;	break;
	case PUSH_SP:	DEBUG_S("PUSH Sp");	STACK(r->sp) = r->sp;	r->sp-=2;	break;
	case PUSH_BP:	DEBUG_S("PUSH BP");	STACK(r->sp-=2) = r->bp;	break;
	case PUSH_SI:	DEBUG_S("PUSH SI");	STACK(r->sp-=2) = r->si;	break;
	case PUSH_DI:	DEBUG_S("PUSH DI");	STACK(r->sp-=2) = r->di;	break;
	case PUSH_ES:	DEBUG_S("PUSH ES");	STACK(r->sp-=2) = r->es;	break;
	case PUSH_CS:	DEBUG_S("PUSH CS");	STACK(r->sp-=2) = r->cs;	break;
	case PUSH_SS:	DEBUG_S("PUSH SS");	STACK(r->sp-=2) = r->ss;	break;
	case PUSH_DS:	DEBUG_S("PUSH DS");	STACK(r->sp-=2) = r->ds;	break;
	case PUSH_I8:
		DEBUG_S("PUSH (I8) 0x"); DEBUG_H(code[State->IP]);
		STACK(r->sp-=2) = code[State->IP];
		State->IP++;
		break;
	case PUSH_I:
		DEBUG_S("PUSH (I) "); DEBUG_H( *(uint16_t*)(code+State->IP) );
		STACK(r->sp-=2) = *(uint16_t*)(code+State->IP);
		State->IP+=2;
		break;		

	//POP Family
	case POPF:
		DEBUG_S("POPF");
		State->Flags = STACK(r->sp);	r->sp += 2;
		break;
	case POPA:
		DEBUG_S("POPA");
		//r->es = STACK(r->sp); r->sp+=2;	r->ds = STACK(r->sp); r->sp+=2;
		//r->cs = STACK(r->sp); r->sp+=2;	r->ss = STACK(r->sp); r->sp+=2;
		r->di = STACK(r->sp); r->sp+=2;	r->si = STACK(r->sp); r->sp+=2;
		r->bp = STACK(r->sp); r->sp+=2;	r->sp = STACK(r->sp); r->sp+=2;
		r->dx = STACK(r->sp); r->sp+=2;	State->CX = STACK(r->sp); r->sp+=2;
		State->BX = STACK(r->sp); r->sp+=2;	State->AX = STACK(r->sp); r->sp+=2;
		break;
	case POP_AX:	DEBUG_S("POP AX");	State->AX = STACK(r->sp); r->sp+=2;	break;
	case POP_BX:	DEBUG_S("POP BX");	State->BX = STACK(r->sp); r->sp+=2;	break;
	case POP_CX:	DEBUG_S("POP CX");	State->CX = STACK(r->sp); r->sp+=2;	break;
	case POP_DX:	DEBUG_S("POP DX");	r->dx = STACK(r->sp); r->sp+=2;	break;
	case POP_SP:	DEBUG_S("POP SP");	State->AX = STACK(r->sp+=2);	break;
	case POP_BP:	DEBUG_S("POP BP");	r->bp = STACK(r->sp); r->sp+=2;	break;
	case POP_SI:	DEBUG_S("POP SI");	r->si = STACK(r->sp); r->sp+=2;	break;
	case POP_DI:	DEBUG_S("POP DI");	r->di = STACK(r->sp); r->sp+=2;	break;
	case POP_ES:	DEBUG_S("POP ES");	r->es = STACK(r->sp); r->sp+=2;	break;
//		case POP_CS:	DEBUG_S("POP CS");	r->cs = STACK(r->sp); r->sp+=2;	break;
	case POP_SS:	DEBUG_S("POP SS");
		r->ss = STACK(r->sp);	r->sp+=2;
		break;
	case POP_DS:	DEBUG_S("POP DS");	r->ds = STACK(r->sp); r->sp+=2;	break;
	
	//CALL Family
	/*case CALL_MF:	//MF, MN, R
		DEBUG_S("CALL (MF) ");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), NULL, &toW);
		STACK(r->sp-=2) = State->IP;
		State->IP += *toW;
		break;*/
	case CALL_N:
		DEBUG_S("CALL (N) .+0x"); DEBUG_H(*(uint16_t*)(code+State->IP));
		State->IP+=2;
		STACK(r->sp-=2) = State->IP;
		State->IP += *(uint16_t*)(code+State->IP-2);
		break;
	case CALL_F:
		DEBUG_S("CALL (F) "); DEBUG_H(*(uint16_t*)(code+State->IP+2));
		DEBUG_C(':');	DEBUG_H(*(uint16_t*)(code+State->IP));
		State->IP+=4;
		STACK(r->sp-=2) = r->cs;
		STACK(r->sp-=2) = State->IP;
		r->cs = *(uint16_t*)(code+State->IP-2);
		State->IP = *(uint16_t*)(code+State->IP-4);
		code = (uint8_t*)(r->cs<<4);
		break;
	case RET_N:
		DEBUG_S("RET (N)");
		State->IP = STACK(r->sp); r->sp+=2;
		break;
	case RET_F:
		DEBUG_S("RET (F)");
		State->IP = STACK(r->sp); r->sp+=2;
		r->cs = STACK(r->sp); r->sp+=2;
		code = (uint8_t*)(r->cs<<4);
		break;
	
	//STOS Functions
	case STOSB:
		*(uint8_t*)(r->es*16+r->di) = State->AX & 0xFF;
		r->di ++;
		break;
	case STOSW:
		*(uint16_t*)(r->es*16+r->di) = State->AX;
		r->di += 2;
		break;
	
	#if 0
	//Misc
	case LEA:
		DEBUG_S("LEA");
		State->IP += genToFromW(r, (uint8_t*)(code+State->IP), &toW, &fromW);
		if((uint32_t) fromW > 0x100000) {
			DEBUG_S("\nERROR: Memory Address Expected\n");
			return;
		}
		*toW = (Uint32) fromW & 0xFFFF;
		break;
	#endif
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
	
	default:
		//Short Jump
		if( (code[State->IP-1] & 0xF0) == 0x70)
		{
			DEBUG_S("J");
			pt2 = code[State->IP];
			switch(code[State->IP-1] & 0xF) {
			
			case 0:	//Overflow
				DEBUG_C('O');
				if(State->Flags & FLAG_OF)	State->IP += code[State->IP];
				break;
			case 1:	//No Overflow
				DEBUG_S("NO");
				if(State->Flags ^ FLAG_OF)	State->IP += code[State->IP];
				break;
			case 2:	//Carry
				DEBUG_C('C');
				if(State->Flags & FLAG_CF)	State->IP += code[State->IP];
				break;
			case 3:	//No Carry
				DEBUG_S("NC");
				if(State->Flags ^ FLAG_CF)	State->IP += code[State->IP];
				break;	
			case 4:	//Equal
				DEBUG_S("Z");
				if(State->Flags & FLAG_ZF)	State->IP += code[State->IP];
				break;
			case 5:	//Not Equal
				DEBUG_S("NZ");
				if(!(State->Flags & FLAG_ZF))	State->IP += code[State->IP];
				break;
			case 6:	//Below or Equal
				DEBUG_S("BE");
				if(State->Flags & FLAG_CF || State->Flags & FLAG_ZF)	State->IP += code[State->IP];
				break;
			case 7:	//Above
				DEBUG_S("A");
				if(State->Flags ^ FLAG_CF && State->Flags ^ FLAG_ZF)	State->IP += code[State->IP];
				break;
			
			}
			DEBUG_S(" (S) .+0x");
			DEBUG_H(pt2);
			
			State->IP++;
			break;
		}
		puts("Unkown Opcode IP = 0x");	putHex(code[State->IP-1]);
		__asm__ __volatile__ ("sti");
		for(;;)
			__asm__ __volatile__ ("hlt");
		return;
		break;
	}
	DEBUG_C('\n');
	
	
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
		if(State->Flags ^ FLAG_ZF && State->CX--)
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
		if(State->Flags ^ FLAG_ZF)
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
		DEBUG_F(" DS:[BX+SI+0x%x]", disp);
		addr = (State->DS << 4) + State->BX + State->SI + disp;
		break;
	case 1:
		DEBUG_F(" DS:[BX+DI+0x%x]", disp);
		addr = (State->DS << 4) + State->BX + State->DI + disp;
		break;
	case 2:
		DEBUG_F(" SS:[BP+SI+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + State->SI + disp;
		break;
	case 3:
		DEBUG_F(" SS:[BP+DI+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + State->DI + disp;
		break;
	case 4:
		DEBUG_F(" DS:[SI+0x%x]", disp);
		addr = (State->DS << 4) + State->SI + disp;
		break;
	case 5:
		DEBUG_F(" DS:[DI+0x%x]", disp);
		addr = (State->DS << 4) + State->DI + disp;
		break;
	case 6:
		DEBUG_F(" SS:[BP+0x%x]", disp);
		addr = (State->SS << 4) + State->BP + disp;
		break;
	case 7:
		DEBUG_F(" DS:[BX+0x%x]", disp);
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
	case 0:	DEBUG_C('O');	//Overflow
		if(State->Flags & FLAG_OF)	State->IP += offset;
		break;
	case 1:	DEBUG_S("NO");	//No Overflow
		if(!(State->Flags & FLAG_OF))	State->IP += offset;
		break;
	case 2:	DEBUG_C('C');	//Carry
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
	DEBUG_S(" NEAR .+0x");	DEBUG_H(offset);
}
