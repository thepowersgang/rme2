/*
 */
#ifndef _RME_OPCODE_TABLE_
#define _RME_OPCODE_TABLE_

#include "opcode_prototypes.h"

#define REP_2(val...)	val,val
#define REP_4(val...)	val,val,val,val
#define REP_8(val...)	val,val,val,val,val,val,val,val

#define UNDEF_OP	{"Undef",NULL,0}
#define DEF_OP_X(name,type,arg)	{#name" ("#type")",RME_Op_##name##_##type,arg}
#define DEF_OP(name,type)	DEF_OP_X(name, type, 0)

#define DEF_ALU_OP(name)	DEF_OP(name,MR), DEF_OP(name,MRX),\
	DEF_OP(name,RM), DEF_OP(name,RMX),\
	DEF_OP(name,AI), DEF_OP(name,AIX)

#define DEF_REG_OP(name)	DEF_OP_X(name,Reg,AX), DEF_OP_X(name,Reg,CX),\
	DEF_OP_X(name,Reg,DX), DEF_OP_X(name,Reg,BX),\
	DEF_OP_X(name,Reg,SP), DEF_OP_X(name,Reg,BP),\
	DEF_OP_X(name,Reg,SI), DEF_OP_X(name,Reg,DI)
#define DEF_REGB_OP(name)	DEF_OP_X(name,RegB,AX), DEF_OP_X(name,RegB,CX),\
	DEF_OP_X(name,RegB,DX), DEF_OP_X(name,RegB,BX),\
	DEF_OP_X(name,RegB,SP), DEF_OP_X(name,RegB,BP),\
	DEF_OP_X(name,RegB,SI), DEF_OP_X(name,RegB,DI)

typedef struct sOperation
{
	const char	*Name;
	tOpcodeFcn Function;
	 int	Arg;
} tOperation;

const tOperation	caOperations[256] = {
	/* ADD RM(X), PUSH ES, POP ES */
	/* 0x00 */	DEF_ALU_OP(ADD), DEF_OP_X(PUSH,Seg,SREG_ES), DEF_OP_X(POP,Seg,SREG_ES),
	/* OR RM(X), PUSH CS, #UD */
	/* 0x08 */	DEF_ALU_OP(OR ), DEF_OP_X(PUSH,Seg,SREG_CS), DEF_OP_X(POP,Seg,SREG_CS),
	/* ADC RM(X), PUSH SS, #UD (POP SS) */
	/* 0x10 */	DEF_ALU_OP(ADC), DEF_OP_X(PUSH,Seg,SREG_SS), UNDEF_OP,
	/* SBB RM(X), PUSH DS, POP DS */
	/* 0x18 */	DEF_ALU_OP(SBB), DEF_OP_X(PUSH,Seg,SREG_DS), DEF_OP_X(POP,Seg,SREG_DS),
	/* AND RM(X), ES Override, #UD */
	/* 0x20 */	DEF_ALU_OP(AND), DEF_OP_X(Ovr,Seg,SREG_ES), UNDEF_OP,
	/* SUB RM(X), CS Override, #UD */
	/* 0x28 */	DEF_ALU_OP(SUB), DEF_OP_X(Ovr,Seg,SREG_CS), UNDEF_OP,
	/* XOR RM(X), SS Override, #UD */
	/* 0x30 */	DEF_ALU_OP(XOR), DEF_OP_X(Ovr,Seg,SREG_SS), UNDEF_OP,
	/* CMP RM(X), DS Override, #UD */
	/* 0x38 */	DEF_ALU_OP(CMP), DEF_OP_X(Ovr,Seg,SREG_DS), UNDEF_OP,
	/* INC R */
	/* 0x40 */	DEF_REG_OP(INC),
	/* DEC R */
	/* 0x48 */	DEF_REG_OP(DEC),
	/* PUSH R */
	/* 0x50 */	DEF_REG_OP(PUSH),
	/* PUSH R */
	/* 0x58 */	DEF_REG_OP(POP),
	/* PUSHA, POPA, 6x #UD */
	/* 0x60 */	DEF_OP(PUSH,A), DEF_OP(POP,A), UNDEF_OP, UNDEF_OP,
	/*  0x64*/	UNDEF_OP, UNDEF_OP, DEF_OP(Ovr, OpSize), DEF_OP(Ovr, AddrSize),
	/* 0x68 */	DEF_OP(PUSH,I), UNDEF_OP, DEF_OP(PUSH,I8), UNDEF_OP,
	/*  0x6C*/	DEF_OP(IN,SB), DEF_OP(IN,SW), DEF_OP(OUT,SB), DEF_OP(OUT,SW),
	// Short Conditional Jumps
	/* 0x70 */	DEF_OP(JO,S), DEF_OP(JNO,S), DEF_OP(JC ,S), DEF_OP(JNC,S),
	/*  0x74*/	DEF_OP(JZ,S), DEF_OP(JNZ,S), DEF_OP(JBE,S), DEF_OP(JA ,S),
	/* 0x78 */	DEF_OP(JS,S), DEF_OP(JNS,S), DEF_OP(JPE,S), DEF_OP(JPO,S),
	/*  0x7C*/	DEF_OP(JL,S), DEF_OP(JGE,S), DEF_OP(JLE,S), DEF_OP(JG ,S),
	/* 0x80 */	DEF_OP(Arith,RI), DEF_OP(Arith,RIX), UNDEF_OP, UNDEF_OP,
	/*  0x84*/	UNDEF_OP, UNDEF_OP, DEF_OP(XCHG,RM), DEF_OP(XCHG,RMX),
	/* 0x88 */	DEF_OP(MOV,MR), DEF_OP(MOV,MRX), DEF_OP(MOV,RM), DEF_OP(MOV,RMX),
	/*  0x8C*/	DEF_OP(MOV,RS), DEF_OP(LEA,z), DEF_OP(MOV,SR), DEF_OP(POP,MX),
	/* 0x90 */	DEF_REG_OP(XCHG),
	/* 0x98 */	DEF_OP(CBW,z), UNDEF_OP, DEF_OP(CALL,F), UNDEF_OP,
	/*  0x9C*/	DEF_OP(PUSH,F), DEF_OP(POP,F), UNDEF_OP, UNDEF_OP,
	/* 0xA0 */	DEF_OP(MOV,AMo), DEF_OP(MOV,AMoX), DEF_OP(MOV,MoA), DEF_OP(MOV,MoAX),
	/*  0xA4*/	DEF_OP(MOV,SB), DEF_OP(MOV,SW), DEF_OP(CMP,SB), DEF_OP(CMP,SW),
	/* 0xA8 */	DEF_OP(TEST,AI), DEF_OP(TEST,AIX), DEF_OP(STO,SB), DEF_OP(STO,SW),
	/*  0xAC*/	DEF_OP(LOD,SB), DEF_OP(LOD,SW), DEF_OP(SCA,SB), DEF_OP(SCA,SW),
	/* 0xB0 */	DEF_REGB_OP(MOV),
	/* 0xB8 */	DEF_REG_OP(MOV),
	/* 0xC0 */	UNDEF_OP, DEF_OP(Logic, MI8X), DEF_OP(RET,iN), DEF_OP(RET,N),
	/*  0xC4*/	DEF_OP(LES,z), DEF_OP(LDS,z), DEF_OP(MOV,MI), DEF_OP(MOV,MIX),
	/* 0xC8 */	UNDEF_OP, UNDEF_OP, DEF_OP(RET,iF), DEF_OP(RET,F),
	/*  0xCC*/	DEF_OP(INT,3), DEF_OP(INT,I), UNDEF_OP, DEF_OP(IRET,z),
	/* 0xD0 */	UNDEF_OP, UNDEF_OP, UNDEF_OP, UNDEF_OP,
	/*  0xD4*/	UNDEF_OP, UNDEF_OP, UNDEF_OP, UNDEF_OP,
	/* 0xD8 */	UNDEF_OP, UNDEF_OP, UNDEF_OP, UNDEF_OP,
	/*  0xDC*/	UNDEF_OP, UNDEF_OP, UNDEF_OP, UNDEF_OP,
	/* 0xE0 */	DEF_OP(LOOPNZ,S), DEF_OP(LOOPZ,S), DEF_OP(LOOP,S), UNDEF_OP,
	/*  0xE4*/	DEF_OP(IN,AI ), DEF_OP(IN,AIX ), DEF_OP(OUT,AI ), DEF_OP(OUT,AIX ),
	/* 0xE8 */	DEF_OP(CALL,N), DEF_OP(JMP,N), DEF_OP(JMP,F), DEF_OP(JMP,S),
	/*  0xEC*/	DEF_OP(IN,ADx), DEF_OP(IN,ADxX), DEF_OP(OUT,DxA), DEF_OP(OUT,DxAX),
	/* 0xF0 */	UNDEF_OP, UNDEF_OP, DEF_OP(Prefix, REP), DEF_OP(Prefix, REPNZ),
	/*  0xF4*/	UNDEF_OP, UNDEF_OP, DEF_OP(ArithMisc, MI), DEF_OP(ArithMisc, MIX),
	/* 0xF8 */	DEF_OP(Flag, CLC), DEF_OP(Flag, STC), DEF_OP(Flag, CLI), DEF_OP(Flag, STI),
	/*  0xFC*/	DEF_OP(Flag, CLD), DEF_OP(Flag, STD), UNDEF_OP, UNDEF_OP
	/*0x100 */
};

#endif
