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
#ifndef _RME_H_
#define _RME_H_

enum eRME_Errors
{
	RME_ERR_OK,
	RME_ERR_UNDEFOPCODE
};

/**
 * \brief Emulator state structure
 */
typedef struct
{
	uint16_t	AX, CX, DX, BX;
	uint16_t	SP, BP, SI, DI;
	
	uint16_t	SS, DS;
	uint16_t	ES;
	
	uint16_t	CS, IP;
	
	uint16_t	Flags;
	
	uint8_t	*Memory[16];	// 1Mib in 16 64 KiB blocks
	
	// --- Decoder State ---
	struct {
		 int	OverrideSegment;
	}	Decoder;
}	tRME_State;


/**
 * \brief Creates a blank RME instance
 */
tRME_State	*RME_CreateState();

/**
 * \brief Calls an interrupt
 * \param State	State returned from ::RME_CreateState
 * \param Number	Interrupt number
 */
 int	RME_CallInt(tRME_State *State, int Number);

/*
 * Definitions specific to the internals of the emulator
 */
#ifdef _RME_C_

/**
 */
enum gpRegs
{
	AL, CL, DL, BL,
	AH, CH, DH, BH
};

enum sRegs
{
	ES = 0,	CS = 1<<3,
	SS = 2<<3, DS = 3<<3,
	FS = 4<<3, GS = 5<<3
};

#define OPCODE_RI(name, code)	name##_RI_AL = code|AL,	name##_RI_BL = code|BL,\
	name##_RI_CL = code|CL,	name##_RI_DL = code|DL,\
	name##_RI_AH = code|AH,	name##_RI_BH = code|BH,\
	name##_RI_CH = code|CH,	name##_RI_DH = code|DH,\
	name##_RI_AX = code|AL|8,	name##_RI_BX = code|BL|8,\
	name##_RI_CX = code|CL|8,	name##_RI_DX = code|DL|8,\
	name##_RI_SP = code|AH|8,	name##_RI_BP = code|CH|8,\
	name##_RI_SI = code|DH|8,	name##_RI_DI = code|BH|8

enum opcodes {
	ADD_MR = 0x00,	ADD_MRX = 0x01,
	ADD_RM = 0x02,	ADD_RMX = 0x03,
	ADD_AI = 0x04,	ADD_AIX = 0x05,
	
	SUB_RM = 0x2A,	SUB_RMX = 0x2B,
	SUB_MR = 0x28,	SUB_MRX = 0x29,
	SUB_AI = 0x2C,	SUB_AIX = 0x2D,
	
	AND_RM = 0x22,	AND_RMX = 0x23,
	AND_MR = 0x20,	AND_MRX = 0x21,
	AND_AI = 0x24,	AND_AIX = 0x25,
	
	OR_RM = 0x0A,	OR_RMX = 0x0B,
	OR_MR = 0x08,	OR_MRX = 0x09,
	OR_AI = 0x0C,	OR_AIX = 0x0D,
	
	CLI = 0xFA,	//b11111010
	
	CMP_RM = 0x3A,	CMP_RMX = 0x3B,
	CMP_MR = 0x38,	CMP_MRX = 0x39,
	CMP_AI = 0x3C,	CMP_AIX = 0x3D,
	
	DEC_A = 0x48|AL,	DEC_B = 0x48|BL,
	DEC_C = 0x48|CL,	DEC_D = 0x48|DL,
	DEC_Sp = 0x48|AH,	DEC_Bp = 0x48|CH,
	DEC_Si = 0x48|DH,	DEC_Di = 0x48|BH,
	
	INC_A = 0x40|AL,	INC_B = 0x40|BL,
	INC_C = 0x40|CL,	INC_D = 0x40|DL,
	INC_Sp = 0x40|AH,	INC_Bp = 0x40|CH,
	INC_Si = 0x40|DH,	INC_Di = 0x40|BH,
	
	DIV_R = 0xFA,	DIV_RX = 0xFB,
	DIV_M = 0xFA,	DIV_MX = 0xFB,
	
	IN_AI = 0xE4,	IN_AIX = 0xE5,
	IN_ADx = 0xEC,	IN_ADxX = 0xED,
	
	INSB = 0x6C,
	INSW = 0x6D,
	INSD = 0x6D,
	
	INT3 = 0xCC,	INT_I = 0xCD,
	IRET = 0xCF,
	
	MOV_MoA = 0xA2,	MOV_MoAX = 0xA3,
	MOV_AMo = 0xA0,	MOV_AMoX = 0xA1,
	OPCODE_RI(MOV, 0xB0),
	MOV_MI = 0xC6,	MOV_MIX = 0xC7,
	MOV_RM = 0x8A,	MOV_RMX = 0x8B,
	MOV_MR = 0x88,	MOV_MRX = 0x89,
	MOV_RS = 0x8C,	MOV_SR = 0x8E,
	MOV_MS = 0x8C,	MOV_SM = 0x8E,
	
	MUL_R = 0xF6,	MUL_RX = 0xF7,
	MUL_M = 0xF6,	MUL_MX = 0xF7,
	
	NOP = 0x90,
	XCHG_AA = 0x90,	XCHG_AB = 0x90|BL,
	XCHG_AC = 0x90|CL,	XCHG_AD = 0x90|DL,
	XCHG_ASp = 0x90|AH,	XCHG_ABp = 0x90|CH,
	XCHG_ASi = 0x90|DH,	XCHG_ADi = 0x90|BH,
	XCHG_RM = 0x86,
	
	NOT_R = 0xF6,	NOT_RX = 0xF7,
	NOT_M = 0xF6,	NOT_MX = 0xF7,
	
	OUT_IA = 0xE6,	OUT_IAX = 0xE7,
	OUT_DxA = 0xEE,	OUT_DxAX = 0xEF,
	
	POP_AX = 0x58|AL,	POP_BX = 0x58|BL,
	POP_CX = 0x58|CL,	POP_DX = 0x58|DL,
	POP_SP = 0x58|AH,	POP_BP = 0x58|CH,
	POP_SI = 0x58|DH,	POP_DI = 0x58|BH,
	POP_ES = 7|ES,	POP_CS = 7|CS,
	POP_SS = 7|SS,	POP_DS = 7|DS,
	POP_MX = 0x8F,
	POPA = 0x61,	POPF = 0x9D,
	
	PUSH_AX = 0x50|AL,	PUSH_BX = 0x50|BL,
	PUSH_CX = 0x50|CL,	PUSH_DX = 0x50|DL,
	PUSH_SP = 0x50|AH,	PUSH_BP = 0x50|CH,
	PUSH_SI = 0x50|DH,	PUSH_DI = 0x50|BH,
	PUSH_MX = 0xFF,
	PUSH_ES = 6|ES,	PUSH_CS = 6|CS,
	PUSH_SS = 6|SS,	PUSH_DS = 6|DS,
	PUSH_I8 = 0x6A,	PUSH_I = 0x68,
	PUSHA = 0x60,	PUSHF = 0x9C,
	
	RET_N = 0xC3,	RET_iN = 0xC2,
	RET_F = 0xCB,	RET_iF = 0xCA,
	
	CALL_MF = 0xFF,	CALL_MN = 0xFF,
	CALL_N = 0xE8,	CALL_F = 0x9A,
	CALL_R = 0xFF,
	
	JMP_MF = 0xFF,	JMP_N = 0xE9,
	JMP_S = 0xEB,	JMP_F = 0xEA,
	
	LEA = 0x8D,
	
	CLD = 0xFC,
	
	XOR_RM = 0x32,	XOR_RMX = 0x33,
	XOR_MR = 0x30,	XOR_MRX = 0x31,
	XOR_AI = 0x34,	XOR_AIX = 0x35,
	
	TEST_RM = 0x84,	TEST_RMX = 0x85,
	
	STOSB = 0xAA, STOSW = 0xAB,
	
	REP = 0xF3,	REPNZ = 0xF2,
	LOOP = 0xE2,	LOOPZ = 0xE0,
	LOOPNZ = 0xE1
};

#endif

#endif
