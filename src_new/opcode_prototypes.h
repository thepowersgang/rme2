/*
 */
#ifndef _RME_OPCODE_PROTOTYPES_
#define _RME_OPCODE_PROTOTYPES_

#define DEF_OPCODE_FCN(name,type)	int RME_Op_##name##_##type(tRME_State *State, int Param)

#define DEF_ALU_OPCODE_PROTO(name)	DEF_OPCODE_FCN(name,MR);DEF_OPCODE_FCN(name,MRX);\
	DEF_OPCODE_FCN(name,RM);DEF_OPCODE_FCN(name,RMX);\
	DEF_OPCODE_FCN(name,AI);DEF_OPCODE_FCN(name,AIX);

// Composite Operations
DEF_OPCODE_FCN(Ext, 0F);	// Two Byte Opcodes
DEF_OPCODE_FCN(Unary, M);	// INC/DEC r/m8
DEF_OPCODE_FCN(Unary, MX);	// INC/DEC r/m16, CALL/JMP/PUSH r/m16

// ALU
DEF_ALU_OPCODE_PROTO(ADD);
DEF_ALU_OPCODE_PROTO(ADC);
DEF_ALU_OPCODE_PROTO(SBB);
DEF_ALU_OPCODE_PROTO(OR);
DEF_ALU_OPCODE_PROTO(AND);
DEF_ALU_OPCODE_PROTO(SUB);
DEF_ALU_OPCODE_PROTO(XOR);
DEF_ALU_OPCODE_PROTO(CMP);
DEF_OPCODE_FCN(Arith, MI);	// 0x80 - Register Immediate
DEF_OPCODE_FCN(Arith, MIX);	// 0x81 - Register Immediate Word
DEF_OPCODE_FCN(Arith, MI8X);	// 0x83 - Register Word Immediate 8
DEF_OPCODE_FCN(TEST, MR);
DEF_OPCODE_FCN(TEST, MRX);
DEF_OPCODE_FCN(TEST, AI);
DEF_OPCODE_FCN(TEST, AIX);

DEF_OPCODE_FCN(ArithMisc, MI);	// 0xF6
DEF_OPCODE_FCN(ArithMisc, MIX);	// 0xF7

DEF_OPCODE_FCN(Shift, MI);	// 0xC0
DEF_OPCODE_FCN(Shift, MI8X);	// 0xC1
DEF_OPCODE_FCN(Shift, M1);	// 0xD0
DEF_OPCODE_FCN(Shift, M1X);	// 0xD1
DEF_OPCODE_FCN(Shift, MCl);	// 0xD2
DEF_OPCODE_FCN(Shift, MClX);	// 0xD3

DEF_OPCODE_FCN(INC, Reg);
DEF_OPCODE_FCN(DEC, Reg);

// Push/Pop
DEF_OPCODE_FCN(PUSH, Seg); DEF_OPCODE_FCN(POP, Seg);
DEF_OPCODE_FCN(PUSH, Reg); DEF_OPCODE_FCN(POP, Reg);
DEF_OPCODE_FCN(PUSH, A); DEF_OPCODE_FCN(POP, A);
DEF_OPCODE_FCN(PUSH, F); DEF_OPCODE_FCN(POP, F);
DEF_OPCODE_FCN(PUSH, MX); DEF_OPCODE_FCN(POP, MX);
DEF_OPCODE_FCN(PUSH, I);
DEF_OPCODE_FCN(PUSH, I8);

// I/O Space Access
DEF_OPCODE_FCN(IN, ADx); DEF_OPCODE_FCN(IN, ADxX);
DEF_OPCODE_FCN(IN, AI ); DEF_OPCODE_FCN(IN, AIX );

DEF_OPCODE_FCN(OUT, DxA); DEF_OPCODE_FCN(OUT, DxAX);
DEF_OPCODE_FCN(OUT, AI ); DEF_OPCODE_FCN(OUT, AIX );

// MOV family
DEF_OPCODE_FCN(MOV, MR ); DEF_OPCODE_FCN(MOV, MRX );	// r/m = r
DEF_OPCODE_FCN(MOV, RM ); DEF_OPCODE_FCN(MOV, RMX );	// r = r/m
DEF_OPCODE_FCN(MOV, MI ); DEF_OPCODE_FCN(MOV, MIX );	// r/m = A
DEF_OPCODE_FCN(MOV, AMo); DEF_OPCODE_FCN(MOV, AMoX);	// A := [imm16/32]
DEF_OPCODE_FCN(MOV, MoA); DEF_OPCODE_FCN(MOV, MoAX);	// [imm16/32] := A
DEF_OPCODE_FCN(MOV, RS );	// r = sr
DEF_OPCODE_FCN(MOV, SR );	// sr = r
DEF_OPCODE_FCN(MOV, RegB); DEF_OPCODE_FCN(MOV, Reg);	// r = I
DEF_OPCODE_FCN(MOV, Z);   DEF_OPCODE_FCN(MOV, ZX);
// Exchange Family
DEF_OPCODE_FCN(XCHG, RM); DEF_OPCODE_FCN(XCHG, RMX);
DEF_OPCODE_FCN(XCHG, Reg);

// String Functions
DEF_OPCODE_FCN(MOV, SB); DEF_OPCODE_FCN(MOV, SW);
DEF_OPCODE_FCN(STO, SB); DEF_OPCODE_FCN(STO, SW);
DEF_OPCODE_FCN(LOD, SB); DEF_OPCODE_FCN(LOD, SW);
DEF_OPCODE_FCN(CMP, SB); DEF_OPCODE_FCN(CMP, SW);
DEF_OPCODE_FCN(SCA, SB); DEF_OPCODE_FCN(SCA, SW);	// String Compare with A
DEF_OPCODE_FCN(IN , SB); DEF_OPCODE_FCN(IN , SW);
DEF_OPCODE_FCN(OUT, SB); DEF_OPCODE_FCN(OUT, SW);

// Conditional Short Jumps
DEF_OPCODE_FCN(JO, S); DEF_OPCODE_FCN(JNO, S);
DEF_OPCODE_FCN(JC, S); DEF_OPCODE_FCN(JNC, S);
DEF_OPCODE_FCN(JZ, S); DEF_OPCODE_FCN(JNZ, S);
DEF_OPCODE_FCN(JBE,S); DEF_OPCODE_FCN(JA , S);
DEF_OPCODE_FCN(JS, S); DEF_OPCODE_FCN(JNS, S);
DEF_OPCODE_FCN(JPE,S); DEF_OPCODE_FCN(JPO, S);
DEF_OPCODE_FCN(JL, S); DEF_OPCODE_FCN(JGE, S);
DEF_OPCODE_FCN(JLE,S); DEF_OPCODE_FCN(JG , S);
// Looping
DEF_OPCODE_FCN(LOOPNZ, S);
DEF_OPCODE_FCN(LOOPZ, S);
DEF_OPCODE_FCN(LOOP, S);
// Unconditional Jumps
DEF_OPCODE_FCN(JMP, N);
DEF_OPCODE_FCN(JMP, F);
DEF_OPCODE_FCN(JMP, S);
//DEF_OPCODE_FCN(Jcc, N);	// 0F 8x (Actually a series)
DEF_OPCODE_FCN(JO, N); DEF_OPCODE_FCN(JNO, N);
DEF_OPCODE_FCN(JC, N); DEF_OPCODE_FCN(JNC, N);
DEF_OPCODE_FCN(JZ, N); DEF_OPCODE_FCN(JNZ, N);
DEF_OPCODE_FCN(JBE,N); DEF_OPCODE_FCN(JA , N);
DEF_OPCODE_FCN(JS, N); DEF_OPCODE_FCN(JNS, N);
DEF_OPCODE_FCN(JPE,N); DEF_OPCODE_FCN(JPO, N);
DEF_OPCODE_FCN(JL, N); DEF_OPCODE_FCN(JGE, N);
DEF_OPCODE_FCN(JLE,N); DEF_OPCODE_FCN(JG , N);

// Call and Ret
DEF_OPCODE_FCN(CALL, N);
DEF_OPCODE_FCN(CALL, F);
DEF_OPCODE_FCN(RET, N);
DEF_OPCODE_FCN(RET, F);
DEF_OPCODE_FCN(RET, iN);	// Return, and pop imm16 bytes from stack
DEF_OPCODE_FCN(RET, iF);	// Return, and pop imm16 bytes from stack
// Interrupts
DEF_OPCODE_FCN(INT, 3);	// INT 0x3 - Debug
DEF_OPCODE_FCN(INT, I);	// INT imm8
DEF_OPCODE_FCN(IRET, z);	// Interrupt Return

// Overrides
DEF_OPCODE_FCN(Ovr, Seg);
DEF_OPCODE_FCN(Ovr, OpSize);
DEF_OPCODE_FCN(Ovr, AddrSize);
DEF_OPCODE_FCN(Prefix, REP);
DEF_OPCODE_FCN(Prefix, REPNZ);

// Flag Manipulation
DEF_OPCODE_FCN(Flag, CLC); DEF_OPCODE_FCN(Flag, STC);
DEF_OPCODE_FCN(Flag, CLI); DEF_OPCODE_FCN(Flag, STI);
DEF_OPCODE_FCN(Flag, CLD); DEF_OPCODE_FCN(Flag, STD);
DEF_OPCODE_FCN(Flag, CMC);
DEF_OPCODE_FCN(Flag, SAHF);
DEF_OPCODE_FCN(Flag, LAHF);

// Misc
DEF_OPCODE_FCN(CBW, z);	// Convert signed Byte to Word
DEF_OPCODE_FCN(HLT, z);	// Halt Execution
DEF_OPCODE_FCN(AAA, z);	// ASCII adjust AL after Addition
DEF_OPCODE_FCN(AAS, z);	// ASCII adjust AL after Subtraction
DEF_OPCODE_FCN(DAA, z);	// Decimal adjust AL after Addition
DEF_OPCODE_FCN(DAS, z);	// Decimal adjust AL after Subtraction
DEF_OPCODE_FCN(CWD, z);	// Convert Word to Doubleword
DEF_OPCODE_FCN(LES, z);	// Load ES:r16/32 with m16:m16/32
DEF_OPCODE_FCN(LDS, z);	// Load DS:r16/32 with m16:m16/32
DEF_OPCODE_FCN(LEA, z);	// Load effective address into r16/32
DEF_OPCODE_FCN(XLAT, z);	// Table Look-up Translation
//DEF_OPCODE_FCN(FPU, ARITH);

#endif
