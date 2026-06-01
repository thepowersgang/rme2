/*
 * Realmode Emulator Plugin
 * - By John Hodge (thePowersGang)
 *
 * This code is published under the FreeBSD licence
 * (See the file COPYING for details)
 *
 * ---
 * Core Emulator Include
 */
#ifndef _RME_INTERNAL_H_
#define _RME_INTERNAL_H_

#ifndef __GNUC__
# define __attribute__(...)
#endif

#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <rme_config.h>

/**
 */
enum gpRegs
{
	AL, CL, DL, BL,
	AH, CH, DH, BH
};
enum gpRegsX
{
	AX, CX, DX, BX,
	SP, BP, SI, DI
};

enum sRegs
{
	SREG_ES,
	SREG_CS,
	SREG_SS,
	SREG_DS,
	SREG_FS,	// added 386
	SREG_GS,
};

#define TRY(ret, fcn)	do { ret = fcn; if(ret) return ret; } while(0)

#define OPCODE_RI(name, code)	name##_RI_AL = code|AL,	name##_RI_BL = code|BL,\
	name##_RI_CL = code|CL,	name##_RI_DL = code|DL,\
	name##_RI_AH = code|AH,	name##_RI_BH = code|BH,\
	name##_RI_CH = code|CH,	name##_RI_DH = code|DH,\
	name##_RI_AX = code|AL|8,	name##_RI_BX = code|BL|8,\
	name##_RI_CX = code|CL|8,	name##_RI_DX = code|DL|8,\
	name##_RI_SP = code|AH|8,	name##_RI_BP = code|CH|8,\
	name##_RI_SI = code|DH|8,	name##_RI_DI = code|BH|8

enum opcodes {
	// cspell:disable
	ADD_MR = 0x00,	ADD_MRX = 0x01,
	ADD_RM = 0x02,	ADD_RMX = 0x03,
	ADD_AI = 0x04,	ADD_AIX = 0x05,

	OR_MR = 0x08,	OR_MRX = 0x09,
	OR_RM = 0x0A,	OR_RMX = 0x0B,
	OR_AI = 0x0C,	OR_AIX = 0x0D,

	AND_MR = 0x20,	AND_MRX = 0x21,
	AND_RM = 0x22,	AND_RMX = 0x23,
	AND_AI = 0x24,	AND_AIX = 0x25,

	SUB_MR = 0x28,	SUB_MRX = 0x29,
	SUB_RM = 0x2A,	SUB_RMX = 0x2B,
	SUB_AI = 0x2C,	SUB_AIX = 0x2D,

	XOR_MR = 0x30,	XOR_MRX = 0x31,
	XOR_RM = 0x32,	XOR_RMX = 0x33,
	XOR_AI = 0x34,	XOR_AIX = 0x35,

	CMP_MR = 0x38,	CMP_MRX = 0x39,
	CMP_RM = 0x3A,	CMP_RMX = 0x3B,
	CMP_AI = 0x3C,	CMP_AIX = 0x3D,

	INC_A = 0x40|AL,	INC_B = 0x40|BL,
	INC_C = 0x40|CL,	INC_D = 0x40|DL,
	INC_Sp = 0x40|AH,	INC_Bp = 0x40|CH,
	INC_Si = 0x40|DH,	INC_Di = 0x40|BH,

	DEC_A = 0x48|AL,	DEC_B = 0x48|BL,
	DEC_C = 0x48|CL,	DEC_D = 0x48|DL,
	DEC_Sp = 0x48|AH,	DEC_Bp = 0x48|CH,
	DEC_Si = 0x48|DH,	DEC_Di = 0x48|BH,

	INT3 = 0xCC,	INT_I = 0xCD,
	IRET = 0xCF,

//	DIV_R = 0xFA,	DIV_RX = 0xFB,
//	DIV_M = 0xFA,	DIV_MX = 0xFB,


	MOV_AMo = 0xA0,	MOV_AMoX = 0xA1,
	MOV_MoA = 0xA2,	MOV_MoAX = 0xA3,
	OPCODE_RI(MOV, 0xB0),
	MOV_MI = 0xC6,	MOV_MIX = 0xC7,
	MOV_MR = 0x88,	MOV_MRX = 0x89,
	MOV_RM = 0x8A,	MOV_RMX = 0x8B,
	MOV_RS = 0x8C,	MOV_SR = 0x8E,
//	MOV_MS = 0x8C,	MOV_SM = 0x8E,

//	MUL_R = 0xF6,	MUL_RX = 0xF7,
//	MUL_M = 0xF6,	MUL_MX = 0xF7,

	NOP = 0x90,
	XCHG_AA = 0x90,	XCHG_AB = 0x90|BL,
	XCHG_AC = 0x90|CL,	XCHG_AD = 0x90|DL,
	XCHG_ASp = 0x90|AH,	XCHG_ABp = 0x90|CH,
	XCHG_ASi = 0x90|DH,	XCHG_ADi = 0x90|BH,
	XCHG_RM = 0x86,	XCHG_RMX = 0x87,

	NOT_R = 0xF6,	NOT_RX = 0xF7,
	NOT_M = 0xF6,	NOT_MX = 0xF7,


	IN_AI = 0xE4,	IN_AIX = 0xE5,
	OUT_IA = 0xE6,	OUT_IAX = 0xE7,
	IN_ADx = 0xEC,	IN_ADxX = 0xED,
	OUT_DxA = 0xEE,	OUT_DxAX = 0xEF,

	PUSH_AX = 0x50|AL,	PUSH_BX = 0x50|BL,
	PUSH_CX = 0x50|CL,	PUSH_DX = 0x50|DL,
	PUSH_SP = 0x50|AH,	PUSH_BP = 0x50|CH,
	PUSH_SI = 0x50|DH,	PUSH_DI = 0x50|BH,
	// PUSH_MX = 0xFF,	// - TODO: Check (maybe 0x87)
	PUSH_ES = 0x06|(SREG_ES<<3),	PUSH_CS = 0x06|(SREG_CS<<3),
	PUSH_SS = 0x06|(SREG_SS<<3),	PUSH_DS = 0x06|(SREG_DS<<3),
	PUSH_I8 = 0x6A,	PUSH_I = 0x68,
	PUSHA = 0x60,	PUSHF = 0x9C,

	POP_AX = 0x58|AL,	POP_BX = 0x58|BL,
	POP_CX = 0x58|CL,	POP_DX = 0x58|DL,
	POP_SP = 0x58|AH,	POP_BP = 0x58|CH,
	POP_SI = 0x58|DH,	POP_DI = 0x58|BH,
	POP_ES = 7|(SREG_ES<<3),
	POP_SS = 7|(SREG_SS<<3),	POP_DS = 7|(SREG_DS<<3),
	POP_MX = 0x8F,
	POPA = 0x61,	POPF = 0x9D,

	RET_N = 0xC3,	RET_iN = 0xC2,
	RET_F = 0xCB,	RET_iF = 0xCA,

	CALL_MF = 0xFF,	CALL_MN = 0xFF,
	CALL_N = 0xE8,	CALL_F = 0x9A,
	CALL_R = 0xFF,

	JMP_MF = 0xFF,	JMP_N = 0xE9,
	JMP_S = 0xEB,	JMP_F = 0xEA,

	LES = 0xC4,
	LDS = 0xC5,
	LEA = 0x8D,
	
	CBW = 0x98,

	CLC = 0xF8,	STC = 0xF9,
	CLI = 0xFA,	STI = 0xFB,
	CLD = 0xFC,	STD = 0xFD,

	TEST_RM = 0x84,	TEST_RMX = 0x85,
	TEST_AI = 0xA8,	TEST_AIX = 0xA9,

	MOVSB = 0xA4,	MOVSW = 0xA5,
	CMPSB = 0xA6,	CMPSW = 0xA7,
	STOSB = 0xAA,	STOSW = 0xAB,
	LODSB = 0xAC,	LODSW = 0xAD,
	SCASB = 0xAE,	SCASW = 0xAF,
	INSB  = 0x6C,	INSW = 0x6D,
	OUTSB = 0x6E,	OUTSW = 0x6F,

	// --- Unimplementeds
	FPU_ARITH	= 0xDC,

	// --- Overrides
	OVR_ES = 0x26,
	OVR_CS = 0x2E,
	OVR_SS = 0x36,
	OVR_DS = 0x3E,

	REPNZ = 0xF2,	REP = 0xF3,
	LOOPNZ = 0xE0,	LOOPZ = 0xE1,
	LOOP = 0xE2
	// cspell:enable
};

// --- Debug Macro ---
__attribute__((format(printf, 2, 3)))
extern void RME_Int_ErrorPrint(tRME_State* State, const char* fmt, ...);

/// @brief Print into the debug buffer
/// @param State Emulator state
/// @param fmt printf-style format string
/// @param  Arguments
__attribute__((format(printf, 2, 3)))
extern void RME_Int_DebugPrint(tRME_State* State, const char* fmt, ...);

// --- Operation helpers
//#define PARITY8(v)	((((v)>>7)&1)^(((v)>>6)&1)^(((v)>>5)&1)^(((v)>>4)&1)^(((v)>>3)&1)^(((v)>>2)&1)^(((v)>>1)&1)^((v)&1))
#define PARITY8(v)	(( ((v)>>7) ^ ((v)>>6) ^ ((v)>>5) ^ ((v)>>4) ^ ((v)>>3) ^ ((v)>>2) ^ ((v)>>1) ^ (v) )&1)
#define SET_COMM_FLAGS(State,v) do{\
	State->Flags &= ~(FLAG_ZF|FLAG_SF|FLAG_PF);\
	State->Flags |= ((v) == 0) ? FLAG_ZF : 0;\
	State->Flags |= ((v) >> (sizeof(v)*8-1)) ? FLAG_SF : 0;\
	State->Flags |= PARITY8(v) == 0 ? FLAG_PF : 0;\
	}while(0)

// --- Memory Helpers
/**
 * \brief Read an unsigned byte from the instruction stream
 * Reads 1 byte as an unsigned integer from CS:IP and increases IP by 1.
 */
#define READ_INSTR8(dst)	do{int r;uint8_t __v;\
	r=RME_Int_Read8(State,State->CS,State->IP+State->Decoder.IPOffset,&__v);\
	if(r)	return r;\
	State->Decoder.IPOffset++;\
	(dst) = __v;\
	}while(0)
/**
 * \brief Read a signed byte from the instruction stream
 * Reads 1 byte as an signed integer from CS:IP and increases IP by 1.
 */
#define READ_INSTR8S(dst)	do{int r;int8_t __v;\
	r=RME_Int_Read8(State,State->CS,State->IP+State->Decoder.IPOffset,(uint8_t*)&__v);\
	if(r)	return r;\
	State->Decoder.IPOffset++;\
	(dst) = __v;\
	}while(0)
/**
 * \brief Read a word from the instruction stream
 * Reads 2 bytes as an unsigned integer from CS:IP and increases IP by 2.
 */
#define READ_INSTR16(dst)	do{int r;uint16_t __v;\
	r=RME_Int_Read16(State,State->CS,State->IP+State->Decoder.IPOffset,&__v);\
	if(r)	return r;\
	State->Decoder.IPOffset+=2;\
	(dst) = __v;\
	}while(0)
/**
 * \brief Read a word from the instruction stream
 * Reads 4 bytes as an unsigned integer from CS:IP and increases IP by 4.
 */
#define READ_INSTR32(dst)	do{int r;uint32_t __v;\
	r=RME_Int_Read32(State,State->CS,State->IP+State->Decoder.IPOffset,&__v);\
	if(r)	return r;\
	State->Decoder.IPOffset+=4;\
	(dst) = __v;\
	}while(0)
/**
 * \brief Get a segment with overrides
 */
#define GET_SEGMENT(State,_def)	(Seg(State, (State->Decoder.OverrideSegment==-1?(_def):State->Decoder.OverrideSegment)))

// -- Per Compiler macros
#define	WARN_UNUSED_RET	__attribute__((warn_unused_result))

// --- Functions ---
struct MemRef {
	void*	range1;
	void*	range2;
	size_t	len_1;
};
/// @param PtrFirst	Pointer to the first byte in the range
/// @param LenFirst Length of the first portion of the range
/// @param PtrLast	Pointer to the last byte in the range (might be in a different allocation to PtrFirst)
static inline WARN_UNUSED_RET int	RME_Int_GetPtr(tRME_State *State, uint16_t Seg, uint32_t Ofs, uint16_t Len, struct MemRef* Out)
{
	assert(Out);
	// PtrLast can be null
	uint32_t	addr = (int)Seg * 16 + Ofs;
	assert(1 <= Len && Len <= RME_BLOCK_SIZE);
	 int	block_s = addr/RME_BLOCK_SIZE;
	 int	block_e = (addr+Len-1)/RME_BLOCK_SIZE;
	#if RME_DO_NULL_CHECK
	# if RME_ALLOW_ZERO_TO_BE_NULL
	if(block_s && State->Memory[block_s] == NULL)	return RME_ERR_BADMEM;
	if(block_e && State->Memory[block_e] == NULL)	return RME_ERR_BADMEM;
	# else
	if(State->Memory[block_s] == NULL)	return RME_ERR_BADMEM;
	if(State->Memory[block_e] == NULL)	return RME_ERR_BADMEM;
	# endif
	#endif
	Out->range1 = (void*)( (uint8_t*)State->Memory[block_s] + (addr%RME_BLOCK_SIZE) );
	uint16_t space = RME_BLOCK_SIZE - addr % RME_BLOCK_SIZE;
	Out->len_1 = space < Len ? space : Len;
	uint16_t tail_len = Len - Out->len_1;
	Out->range2 = (void*)( (uint8_t*)State->Memory[block_e] + ((addr+Len-1)%RME_BLOCK_SIZE) + 1 - tail_len );
	State->MemoryTouched[block_s] = 1;
	State->MemoryTouched[block_e] = 1;
	return 0;
}
static inline WARN_UNUSED_RET int	RME_Int_ReadBytes(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t *Dst, uint16_t Len) {
	if( Len > 0 ) {
		struct MemRef	mr;
		int	ret = RME_Int_GetPtr(State, Seg, Ofs, Len, &mr);
		if(ret)	return ret;
		if( mr.len_1 < Len ) {
			memcpy(Dst, mr.range1, mr.len_1);
			memcpy(Dst+mr.len_1, mr.range2, Len-mr.len_1);
		}
		else {
			memcpy(Dst, mr.range1, Len);
		}
	}
	return 0;
}
static inline WARN_UNUSED_RET int	RME_Int_WriteBytes(tRME_State *State, uint16_t Seg, uint16_t Ofs, const uint8_t *Src, uint16_t Len) {
	if( Len > 0 ) {
		struct MemRef	mr;
		int	ret = RME_Int_GetPtr(State, Seg, Ofs, Len, &mr);
		if(ret)	return ret;
		if( mr.len_1 < Len ) {
			memcpy(mr.range1, Src, mr.len_1);
			memcpy(mr.range2, Src+mr.len_1, Len-mr.len_1);
		}
		else {
			memcpy(mr.range1, Src, Len);
		}
	}
	return 0;
}
static inline WARN_UNUSED_RET int	RME_Int_Read8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t *Dst) {
	return RME_Int_ReadBytes(State, Seg, Ofs, Dst, 1);
}
static inline WARN_UNUSED_RET int	RME_Int_Read16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t *Dst) {
	int ret = RME_Int_ReadBytes(State, Seg, Ofs, (uint8_t*)Dst, 2);
	if(ret != 0)	return ret;
	// TODO: Check endian
	return ret;
}
static inline WARN_UNUSED_RET int	RME_Int_Read32(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint32_t *Dst) {
	int ret = RME_Int_ReadBytes(State, Seg, Ofs, (uint8_t*)Dst, 4);
	if(ret != 0)	return ret;
	// TODO: Check endian
	return ret;
}
static inline WARN_UNUSED_RET int	RME_Int_Write8(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint8_t Val) {
	return RME_Int_WriteBytes(State, Seg, Ofs, &Val, 1);
}
static inline WARN_UNUSED_RET int	RME_Int_Write16(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint16_t Val) {
	// TODO: Endian
	return RME_Int_WriteBytes(State, Seg, Ofs, (const uint8_t*)&Val, 2);
}
static inline WARN_UNUSED_RET int	RME_Int_Write32(tRME_State *State, uint16_t Seg, uint16_t Ofs, uint32_t Val) {
	// TODO: Endian
	return RME_Int_WriteBytes(State, Seg, Ofs, (const uint8_t*)&Val, 4);
}

struct ModRM {
	unsigned char mod;	// 2 bits
	unsigned char rrr;	// 3 bits, a register or operator selector
	unsigned char mmm;	// 3 bits
};
/// @brief Decode a ModRM byte
/// @param State 
/// @param out 
/// @return  
static inline int RME_Int_GetModRM(tRME_State *State, struct ModRM *out)
{
	uint8_t	byte;
	READ_INSTR8(byte);
	out->mod = byte >> 6;
	out->rrr = (byte >> 3) & 7;
	out->mmm = (byte >> 0) & 7;
	return 0;
}
#define VALUE_REF_FLAG_REGISTER	(1 << 0)
#define VALUE_REF_FLAG_WRAP	(1 << 1)
struct ValueRef {
	/// @brief Flags on the value reference
	/// - 0: Register or memory bit
	/// - 1: Indicates that the address is within 16 bytes of the end of the segment. Either wrap (<286) or fault (286)
	/// - 2: Stack segment was involved in the memory access
	uint8_t	flags;
	/// @brief 21 bit address (or register number)
	uint8_t	addr[3];
};
struct ValueRefX {
	struct ValueRef	r;
};
/// @brief Load and parse the ModRM byte, decoding/printing `rrr` before `mmm`
extern WARN_UNUSED_RET int	RME_Int_ParseModRM(tRME_State *State, struct ValueRef *reg, struct ValueRef *mem);
extern WARN_UNUSED_RET int	RME_Int_ParseModRMX(tRME_State *State, struct ValueRefX *reg, struct ValueRefX* mem);
/// @brief Load and parse the ModRM byte, decoding/printing `mmm` before `rrr`
extern WARN_UNUSED_RET int	RME_Int_ParseModRMRev(tRME_State *State, struct ValueRef *reg, struct ValueRef *mem);
extern WARN_UNUSED_RET int	RME_Int_ParseModRMXRev(tRME_State *State, struct ValueRefX *reg, struct ValueRefX *mem);
/// @brief Decode (and print) the MMM part of a ModRM byte
/// @param State Emulator state structure
/// @param modrm Decomposes ModRM byte
/// @param mem Output memory value reference
/// @return Error code
extern WARN_UNUSED_RET int	RME_Int_DecodeModM (tRME_State *State, const struct ModRM* modrm, struct ValueRef *mem);
extern WARN_UNUSED_RET int	RME_Int_DecodeModMX(tRME_State *State, const struct ModRM* modrm, struct ValueRefX *mem);
/// @brief Decode the MMM part of a ModRM byte into a segment:offset address
extern WARN_UNUSED_RET int	RME_Int_GetMMM(tRME_State *State, const struct ModRM* modrm, uint16_t *Segment, uint32_t *Address);

extern WARN_UNUSED_RET int RME_Int_ReadV8(tRME_State *State, struct ValueRef* Ref, uint8_t *Dst);
extern WARN_UNUSED_RET int RME_Int_ReadV16(tRME_State *State, struct ValueRefX* Ref, uint16_t *Dst);
extern WARN_UNUSED_RET int RME_Int_ReadV32(tRME_State *State, struct ValueRefX* Ref, uint32_t *Dst);
extern WARN_UNUSED_RET int RME_Int_WriteV8(tRME_State *State, struct ValueRef* Ref, uint8_t Val);
extern WARN_UNUSED_RET int RME_Int_WriteV16(tRME_State *State, struct ValueRefX* Ref, uint16_t Val);
extern WARN_UNUSED_RET int RME_Int_WriteV32(tRME_State *State, struct ValueRefX* Ref, uint32_t Val);

/// Parse the ModRM byte, reading u8 from both operands
static inline WARN_UNUSED_RET int RME_Int_ParseModRM_Rd8Both(tRME_State *State, struct ValueRef *rrr, uint8_t *reg, uint8_t *mem) {
	int ret;
	struct ValueRef src;
	TRY(ret, RME_Int_ParseModRM(State, rrr, &src));
	TRY(ret, RME_Int_ReadV8(State, rrr, reg));
	TRY(ret, RME_Int_ReadV8(State, &src, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_Rd16Both(tRME_State *State, struct ValueRefX *rrr, uint16_t* reg, uint16_t *mem) {
	int ret;
	struct ValueRefX src;
	TRY(ret, RME_Int_ParseModRMX(State, rrr, &src));
	TRY(ret, RME_Int_ReadV16(State, rrr, reg));
	TRY(ret, RME_Int_ReadV16(State, &src, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_Rd32Both(tRME_State *State, struct ValueRefX *rrr, uint32_t* reg, uint32_t *mem) {
	int ret;
	struct ValueRefX src;
	TRY(ret, RME_Int_ParseModRMX(State, rrr, &src));
	TRY(ret, RME_Int_ReadV32(State, rrr, reg));
	TRY(ret, RME_Int_ReadV32(State, &src, mem));
	return 0;
}

/// Parse the ModRM byte in reverse order (mmm, rrr) reading u8 from both
static inline WARN_UNUSED_RET int RME_Int_ParseModRMRev_Rd8Both(tRME_State *State, struct ValueRef *mmm, uint8_t *mem, uint8_t *reg) {
	int ret;
	struct ValueRef src;
	TRY(ret, RME_Int_ParseModRMRev(State, &src, mmm));
	TRY(ret, RME_Int_ReadV8(State, mmm, mem));
	TRY(ret, RME_Int_ReadV8(State, &src, reg));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMXRev_Rd16Both(tRME_State *State, struct ValueRefX *mmm, uint16_t *mem, uint16_t *reg) {
	int ret;
	struct ValueRefX src;
	TRY(ret, RME_Int_ParseModRMXRev(State, &src, mmm));
	TRY(ret, RME_Int_ReadV16(State, mmm, mem));
	TRY(ret, RME_Int_ReadV16(State, &src, reg));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMXRev_Rd32Both(tRME_State *State, struct ValueRefX *mmm, uint32_t *mem, uint32_t *reg) {
	int ret;
	struct ValueRefX src;
	TRY(ret, RME_Int_ParseModRMXRev(State, &src, mmm));
	TRY(ret, RME_Int_ReadV32(State, mmm, mem));
	TRY(ret, RME_Int_ReadV32(State, &src, reg));
	return 0;
}

/// Parse just the mmm operand, and read from it
static inline WARN_UNUSED_RET int RME_Int_ParseModRM_MRd8(tRME_State *State, struct ValueRef *mmm, uint8_t *mem)
{
	int ret;
	TRY(ret, RME_Int_ParseModRM(State, NULL, mmm));
	TRY(ret, RME_Int_ReadV8(State, mmm, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_MRd16(tRME_State *State, struct ValueRefX *mmm, uint16_t *mem)
{
	int ret;
	TRY(ret, RME_Int_ParseModRMX(State, NULL, mmm));
	TRY(ret, RME_Int_ReadV16(State, mmm, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_MRd32(tRME_State *State, struct ValueRefX *mmm, uint32_t *mem)
{
	int ret;
	TRY(ret, RME_Int_ParseModRMX(State, NULL, mmm));
	TRY(ret, RME_Int_ReadV32(State, mmm, mem));
	return 0;
}


/// Parse both operands (rrr then mmm), reading u8 from mmm and providing the rrr operand
static inline WARN_UNUSED_RET int RME_Int_ParseModRM_RRdM8(tRME_State *State, struct ValueRef *rrr, uint8_t *mem)
{
	int ret;
	struct ValueRef	mmm;
	TRY(ret, RME_Int_ParseModRM(State, rrr, &mmm));
	TRY(ret, RME_Int_ReadV8(State, &mmm, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_RRd16(tRME_State *State, struct ValueRefX *rrr, uint16_t *mem)
{
	int ret;
	struct ValueRefX mmm;
	TRY(ret, RME_Int_ParseModRMX(State, rrr, &mmm));
	TRY(ret, RME_Int_ReadV16(State, &mmm, mem));
	return 0;
}
static inline WARN_UNUSED_RET int RME_Int_ParseModRMX_RRd32(tRME_State *State, struct ValueRefX *rrr, uint32_t *mem)
{
	int ret;
	struct ValueRefX mmm;
	TRY(ret, RME_Int_ParseModRMX(State, rrr, &mmm));
	TRY(ret, RME_Int_ReadV32(State, &mmm, mem));
	return 0;
}

// --- Stack Primitives ---
// TODO: Possible support for non 16-bit stack segment
#define PUSH(v)	do{\
	State->SP.W -= 2; \
	ret=RME_Int_Write16(State,State->SS,State->SP.W,(v));\
	if(ret)return ret;\
	}while(0)
#define POP(dst)	do{\
	uint16_t _tmp_v;\
	ret=RME_Int_Read16(State,State->SS,State->SP.W,&_tmp_v);\
	if(ret)return ret;\
	State->SP.W+=2;\
	(dst)=_tmp_v;\
	}while(0)

// --- Register Selecting ---
static inline uint16_t	*Seg(tRME_State *State, int code)
{
	static const char regnames[][3] = {"ES","CS","SS","DS","FS","GS"};
	RME_Int_DebugPrint(State, " %s", regnames[code]);
	switch(code) {
	case SREG_ES:	return &State->ES;
	case SREG_CS:	return &State->CS;
	case SREG_SS:	return &State->SS;
	case SREG_DS:	return &State->DS;
	case SREG_FS:	return &State->FS;
	case SREG_GS:	return &State->GS;
	default:
		RME_Int_ErrorPrint(State, "ERROR - Invalid value passed to Seg(). (%i is not a segment)", code);
	}
	return NULL;
}

static inline uint8_t	*RegB(tRME_State *State, int num)
{
	if(num > 7 || num < 0)	return NULL;
	static const char regnamesB[][3] = {"AL","CL","DL","BL","AH","CH","DH","BH"};
	RME_Int_DebugPrint(State, " %s", regnamesB[num]);
	if( num >= 4 )
		return &State->GPRs[num-4].B.H;
	else
		return &State->GPRs[num].B.L;
}

static inline uint16_t	*RegW(tRME_State *State, int num)
{
	if(num > 7 || num < 0)	return NULL;
	static const char regnamesD[][4] = {"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI"};
	static const char regnamesW[][3] = {"AX","CX","DX","BX","SP","BP","SI","DI"};
	RME_Int_DebugPrint(State, " %s", State->Decoder.bOverrideOperand ? regnamesD[num] : regnamesW[num]);
	return &State->GPRs[num].W;
}

#endif
