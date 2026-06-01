/*
 * Realmode Emulator
 * - ALU Operations
 * 
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
#include "ops_alu.h"
 
#define CREATE_ALU_OPCODE_FCN_RM(__name, __code...) DEF_OPCODE_FCN(__name,RM) {\
	 int	ret;\
	struct ValueRef	reg; \
	uint8_t	v, *dest=&v, val_l, val_r; \
	TRY(ret, RME_Int_ParseModRM_Rd8Both(State, &reg, &val_l, &val_r)); \
	__code \
	SET_COMM_FLAGS(State,v); \
	if(dest) TRY(ret, RME_Int_WriteV8(State, &reg, v)); \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_RMX(__name, __code...) DEF_OPCODE_FCN(__name,RMX) {\
	 int	ret; \
	struct ValueRefX	reg; \
	if(State->Decoder.bOverrideOperand) { \
		uint32_t v, *dest=&v, val_l, val_r; \
		TRY(ret, RME_Int_ParseModRMX_Rd32Both(State, &reg, &val_l, &val_r)); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) TRY(ret, RME_Int_WriteV32(State, &reg, v)); \
	} else { \
		uint16_t v, *dest=&v, val_l, val_r; \
		TRY(ret, RME_Int_ParseModRMX_Rd16Both(State, &reg, &val_l, &val_r)); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) TRY(ret, RME_Int_WriteV16(State, &reg, v)); \
	} \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_MR(__name, __code...) DEF_OPCODE_FCN(__name,MR) {\
	 int	ret;\
	struct ValueRef	mem; \
	uint8_t	v, *dest=&v, val_l, val_r=0; \
	TRY(ret, RME_Int_ParseModRMRev_Rd8Both(State, &mem, &val_l, &val_r)); \
	__code \
	SET_COMM_FLAGS(State,v); \
	if(dest) TRY(ret, RME_Int_WriteV8(State, &mem, v)); \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_MRX(__name, __code...) DEF_OPCODE_FCN(__name,MRX) {\
	 int	ret; \
	struct ValueRefX mem, *dest=&mem; \
	if(State->Decoder.bOverrideOperand) { \
		uint32_t v, val_l, val_r; \
		TRY(ret, RME_Int_ParseModRMXRev_Rd32Both(State, &mem, &val_l, &val_r)); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) TRY(ret, RME_Int_WriteV32(State, &mem, v)); \
	} else { \
		uint16_t v, val_l, val_r; \
		TRY(ret, RME_Int_ParseModRMXRev_Rd16Both(State, &mem, &val_l, &val_r)); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) TRY(ret, RME_Int_WriteV16(State, &mem, v)); \
	} \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_AI(__name, __code...) DEF_OPCODE_FCN(__name,AI) {\
	uint8_t v, *dest = &State->AX.B.L, val_l = *dest, val_r; \
	READ_INSTR8( val_r ); \
	RME_Int_DebugPrint(State, " AL 0x%02x", val_r); \
	__code \
	SET_COMM_FLAGS(State,v); \
	if(dest) *dest = v;\
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_AIX(__name, __code...) DEF_OPCODE_FCN(__name,AIX) {\
	if(State->Decoder.bOverrideOperand) { \
		uint32_t v; \
		uint32_t *dest=&State->AX.D, val_l = *dest, val_r; \
		READ_INSTR32( val_r ); \
		RME_Int_DebugPrint(State, " EAX 0x%08x", val_r); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) *dest=v;\
	} else { \
		uint16_t v; \
		uint16_t *dest=&State->AX.W, val_l = *dest, val_r; \
		READ_INSTR16( val_r ); \
		RME_Int_DebugPrint(State, " AX 0x%04x", val_r); \
		__code \
		SET_COMM_FLAGS(State,v); \
		if(dest) *dest=v;\
	} \
	return 0; \
}

#define CREATE_ALU_OPCODE_FCN(__name, __code...) \
	CREATE_ALU_OPCODE_FCN_RM(__name, __code) \
	CREATE_ALU_OPCODE_FCN_RMX(__name, __code) \
	CREATE_ALU_OPCODE_FCN_MR(__name, __code) \
	CREATE_ALU_OPCODE_FCN_MRX(__name, __code) \
	CREATE_ALU_OPCODE_FCN_AI(__name, __code) \
	CREATE_ALU_OPCODE_FCN_AIX(__name, __code)

// === CODE ===
CREATE_ALU_OPCODE_FCN(ADD, ALU_OPCODE_ADD_CODE)
CREATE_ALU_OPCODE_FCN(OR , ALU_OPCODE_OR_CODE)
CREATE_ALU_OPCODE_FCN(ADC, ALU_OPCODE_ADC_CODE)
CREATE_ALU_OPCODE_FCN(SBB, ALU_OPCODE_SBB_CODE)
CREATE_ALU_OPCODE_FCN(AND, ALU_OPCODE_AND_CODE)
CREATE_ALU_OPCODE_FCN(SUB, ALU_OPCODE_SUB_CODE)
CREATE_ALU_OPCODE_FCN(XOR, ALU_OPCODE_XOR_CODE)
CREATE_ALU_OPCODE_FCN(CMP, ALU_OPCODE_CMP_CODE)

CREATE_ALU_OPCODE_FCN_MR(TEST, ALU_OPCODE_TEST_CODE)
CREATE_ALU_OPCODE_FCN_MRX(TEST, ALU_OPCODE_TEST_CODE)
CREATE_ALU_OPCODE_FCN_AI(TEST, ALU_OPCODE_TEST_CODE)
CREATE_ALU_OPCODE_FCN_AIX(TEST, ALU_OPCODE_TEST_CODE)

#define ALU_SELECT_OPERATION() do { switch( op_num ) { \
	case 0: do { ALU_OPCODE_ADD_CODE } while(0);	break; \
	case 1: do { ALU_OPCODE_OR_CODE  } while(0);	break; \
	case 2: do { ALU_OPCODE_ADC_CODE } while(0);	break; \
	case 3: do { ALU_OPCODE_SBB_CODE } while(0);	break; \
	case 4: do { ALU_OPCODE_AND_CODE } while(0);	break; \
	case 5: do { ALU_OPCODE_SUB_CODE } while(0);	break; \
	case 6: do { ALU_OPCODE_XOR_CODE } while(0);	break; \
	case 7: do { ALU_OPCODE_CMP_CODE } while(0);	break; \
	default: RME_Int_ErrorPrint(State, " - ALU Undef %i\n", op_num); return RME_ERR_UNDEFOPCODE;\
	} } while(0)
#define SHIFT_SELECT_OPERATION()	do{ switch( op_num ) {\
	case 0: { ALU_OPCODE_ROL_CODE }	break; \
	case 1: { ALU_OPCODE_ROR_CODE }	break; \
	case 2:	{ ALU_OPCODE_RCL_CODE }	break; \
	case 3:	{ ALU_OPCODE_RCR_CODE }	break; \
	case 4:	{ ALU_OPCODE_SHL_CODE }	break; \
	case 5:	{ ALU_OPCODE_SHR_CODE }	break; \
	case 6:	{ ALU_OPCODE_SAL_CODE }	break; \
	case 7:	{ ALU_OPCODE_SAR_CODE }	break; \
	default: RME_Int_ErrorPrint(State, " - Shift Undef %i\n", op_num); return RME_ERR_UNDEFOPCODE;\
	} }while(0)

#define MAKE_IMM8(var_name)    uint8_t  var_name; READ_INSTR8( var_name); RME_Int_DebugPrint(State, " 0x%02x", var_name)
#define MAKE_IMM16(var_name)   uint16_t var_name; READ_INSTR16(var_name); RME_Int_DebugPrint(State, " 0x%04x", var_name)
#define MAKE_IMM32(var_name)   uint32_t var_name; READ_INSTR32(var_name); RME_Int_DebugPrint(State, " 0x%08x", var_name)
#define MAKE_IMM8S16(var_name) uint16_t var_name; READ_INSTR8S(var_name); RME_Int_DebugPrint(State, " 0x%04x", var_name)
#define MAKE_IMM8S32(var_name) uint32_t var_name; READ_INSTR8S(var_name); RME_Int_DebugPrint(State, " 0x%08x", var_name)

DEF_OPCODE_FCN(Arith, MI)
{
	 int	ret;
	 int	op_num = Param;
	struct ValueRef	mem, *dest = &mem;
	uint8_t	v, val_l;
	
	TRY(ret, RME_Int_ParseModRM_MRd8(State, &mem, &val_l));
	MAKE_IMM8(val_r);
	
	// Read data, perform operation, set common flags
	ALU_SELECT_OPERATION();
	SET_COMM_FLAGS(State, v);
	if(dest) TRY(ret, RME_Int_WriteV8(State, &mem, v));
	
	return 0;
}

DEF_OPCODE_FCN(Arith, MIX)
{
	 int	ret;
	const int op_num = Param;
	struct ValueRefX	mem, *dest = &mem;
	
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t v, val_l;
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &mem, &val_l));
		MAKE_IMM32(val_r);

		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, v);
		if(dest) TRY(ret, RME_Int_WriteV32(State, &mem, v));
	}
	else
	{
		uint16_t	v, val_l;
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &mem, &val_l));
		MAKE_IMM16(val_r);

		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, v);
		if(dest) TRY(ret, RME_Int_WriteV16(State, &mem, v));
	}
	
	return 0;
}

DEF_OPCODE_FCN(Arith, MI8X)
{
	 int	ret, op_num = Param;
	struct ValueRefX	mem, *dest = &mem;
	
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t v, val_l;
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &mem, &val_l));
		MAKE_IMM8S32(val_r);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, v);
		if(dest) TRY(ret, RME_Int_WriteV32(State, &mem, v));
	}
	else
	{
		uint16_t	v, val_l;
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &mem, &val_l));
		MAKE_IMM8S16(val_r);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, v);
		if(dest) TRY(ret, RME_Int_WriteV16(State, &mem, v));
	}
	
	return 0;
}

DEF_OPCODE_FCN(INC, Reg)
{
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		{ALU_OPCODE_INC_CODE(*dest)}
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		{ALU_OPCODE_INC_CODE(*dest)}
	}
	
	return 0;
}

DEF_OPCODE_FCN(DEC, Reg)
{
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		{ALU_OPCODE_DEC_CODE(*dest)}
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		{ALU_OPCODE_DEC_CODE(*dest)}
	}
	return 0;
}

DEF_OPCODE_FCN(ArithMisc, MI)	// 0xF6
{
	 int	ret, op_num = Param;
	struct ValueRef mem, *dest = &mem;
	uint8_t	v=0, src;

	TRY(ret, RME_Int_ParseModRM_MRd8(State, &mem, &src));
	
	switch(Param)
	{
	case 0:	{ uint8_t val_l = src; MAKE_IMM8(val_r); ALU_OPCODE_TEST_CODE; (void)dest; SET_COMM_FLAGS(State, v); } break;
	case 1:	RME_Int_DebugPrint(State, " Misc /1 UNDEF");	return RME_ERR_UNDEFOPCODE;
	case 2: { ALU_OPCODE_NOT_CODE; TRY(ret, RME_Int_WriteV8(State, &mem, v)); } break;
	case 3: { ALU_OPCODE_NEG_CODE; TRY(ret, RME_Int_WriteV8(State, &mem, v)); } break;
	case 4:	{
		uint32_t result = (uint16_t)State->AX.B.L * src;
		State->AX.W = result;
		SET_COMM_FLAGS(State, State->AX.B.L);
		_MUL_FLAGS(result, 16)
	} break;
	case 5:	{
		int32_t result = (int16_t)(int8_t)State->AX.B.L * ((int8_t)src);
		State->AX.W = result;
		SET_COMM_FLAGS(State, State->AX.B.L);
		_IMUL_FLAGS(result, 8)
	} break;
	case 6:	{
		if(src == 0)	return RME_ERR_DIVERR;
		uint16_t	res, rem;
		res = State->AX.W / src;
		rem = State->AX.W % src;
		if(res > 0xFF)	return RME_ERR_DIVERR;
		State->AX.B.H = rem;
		State->AX.B.L = res;
	} break;
	case 7:	{
		if(src == 0)	return RME_ERR_DIVERR;
		int16_t	quot, rem;
		int16_t num = (int16_t)State->AX.W, den = (int8_t)src;
		quot = num / den;
		rem = num - quot*den;
		if(quot < -0x80 || quot > 0x7F)	return RME_ERR_DIVERR;
		State->AX.B.H = rem;
		State->AX.B.L = quot;
	} break;
	default: RME_Int_ErrorPrint(State, " - Misc %i error\n", op_num); return RME_ERR_UNDEFOPCODE;
	}
	
	return 0;
}

DEF_OPCODE_FCN(ArithMisc, MIX)	// 0xF7
{
	 int	ret, op_num = Param;
	struct ValueRefX mem, *dest = &mem;
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	v, src;
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &mem, &src));
		
		switch( op_num ) { \
		case 0:	{ uint32_t val_l = src; MAKE_IMM32(val_r); ALU_OPCODE_TEST_CODE; SET_COMM_FLAGS(State, v); } break;
		case 1:	RME_Int_DebugPrint(State, " Misc /1 UNDEF");	return RME_ERR_UNDEFOPCODE;
		case 2: { ALU_OPCODE_NOT_CODE; TRY(ret, RME_Int_WriteV32(State, &mem, v)); } break;
		case 3: { ALU_OPCODE_NEG_CODE; TRY(ret, RME_Int_WriteV32(State, &mem, v)); } break;
		case 4:	{
			uint64_t result = (uint64_t)State->AX.D * (src);
			State->DX.D = result >> 32;
			State->AX.D = result & 0xFFFFFFFF;
			SET_COMM_FLAGS(State, State->AX.D);
			_MUL_FLAGS(result, 32);
		} break;
		case 5:	{
			int64_t result = (int64_t)(int32_t)State->AX.D * ((int32_t)src);
			State->DX.D = result >> 32;
			State->AX.D = result & 0xFFFFFFFF;
			SET_COMM_FLAGS(State, State->AX.D);
			_IMUL_FLAGS(result, 32)
		} break;
		case 6:	{ALU_OPCODE_DIV_CODE} break;
		case 7:	{ALU_OPCODE_IDIV_CODE} break;
		default: RME_Int_ErrorPrint(State, " - Misc %i error\n", op_num); return RME_ERR_UNDEFOPCODE;
		}
	}
	else
	{
		uint16_t	v=0, src;
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &mem, &src));

		switch( op_num ) { \
		case 0:	{ uint16_t val_l = src; MAKE_IMM16(val_r); ALU_OPCODE_TEST_CODE; (void)dest; SET_COMM_FLAGS(State, v); } break;
		case 1:	RME_Int_DebugPrint(State, " Misc /1 UNDEF");	return RME_ERR_UNDEFOPCODE;
		case 2: { ALU_OPCODE_NOT_CODE; TRY(ret, RME_Int_WriteV16(State, &mem, v)); } break;
		case 3: { ALU_OPCODE_NEG_CODE; TRY(ret, RME_Int_WriteV16(State, &mem, v)); } break;
		case 4:	{
			uint32_t result = (uint32_t)State->AX.W * (src);
			State->DX.W = result >> 16;
			State->AX.W = result & 0xFFFF;
			SET_COMM_FLAGS(State, State->AX.W);
			_MUL_FLAGS(result, 16)
		} break;
		case 5:	{
			int64_t result = (int32_t)(int16_t)State->AX.W * ((int16_t)src);
			State->DX.W = result >> 16;
			State->AX.W = result & 0xFFFF;
			SET_COMM_FLAGS(State, State->AX.W);
			_IMUL_FLAGS(result, 16)
		} break;
		case 6:	{ALU_OPCODE_DIV_CODE} break;
		case 7:	{ALU_OPCODE_IDIV_CODE} break;
		default: RME_Int_ErrorPrint(State, " - Misc %i error\n", op_num); return RME_ERR_UNDEFOPCODE;
		}
	}
	
	return 0;
}

// Multiply mmm by imm8s and store in rrr
DEF_OPCODE_FCN(IMUL,MI8X)	// 0x6B
{
	 int	ret;
	struct ValueRefX dest, src_r;

	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_ErrorPrint(State, "IMUL (MIX) OvrSize Unimpl");
		return RME_ERR_BUG;
	}

	uint16_t	src_v;
	TRY(ret, RME_Int_ParseModRMX(State, &dest, &src_r));
	TRY(ret, RME_Int_ReadV16(State, &src_r, &src_v));
	MAKE_IMM8S16(imm16);

	int32_t result = src_v * imm16;
	SET_COMM_FLAGS(State, result);
	_IMUL_FLAGS(result, 16);
	TRY(ret, RME_Int_WriteV16(State, &dest, (uint16_t)result));
	
	return 0;
}

DEF_OPCODE_FCN(IMUL,MIX)	// 0x69
{
	 int	ret;
	struct ValueRefX dest, src_r;

	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_ErrorPrint(State, "IMUL (MIX) OvrSize Unimpl");
		return RME_ERR_BUG;
	}

	uint16_t	src_v;
	TRY(ret, RME_Int_ParseModRMX(State, &dest, &src_r));
	TRY(ret, RME_Int_ReadV16(State, &src_r, &src_v));
	MAKE_IMM16(imm16);

	int32_t result = src_v * imm16;
	SET_COMM_FLAGS(State, result);
	_IMUL_FLAGS(result, 16);
	TRY(ret, RME_Int_WriteV16(State, &dest, (uint16_t)result));
	
	return 0;
}

DEF_OPCODE_FCN(IMUL,RMX)	// 0x0F 0xAF
{
	 int	ret;

	struct ValueRefX dest;

	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_ErrorPrint(State, "IMUL (MIX) OvrSize Unimpl");
		return RME_ERR_BUG;
	}

	uint16_t val_l, val_r;
	TRY(ret, RME_Int_ParseModRMX_Rd16Both(State, &dest, &val_l, &val_r));
	
	int32_t result = (int16_t)val_l * (int16_t)val_r;
	
	SET_COMM_FLAGS(State, result);
	_IMUL_FLAGS(result, 16);
	TRY(ret, RME_Int_WriteV16(State, &dest, (uint16_t)result));
	
	return 0;
}

// 0xC0 - Shift by Imm8
DEF_OPCODE_FCN(Shift, MI)
{
	 int	ret, op_num = Param;
	struct ValueRef	dest;
	uint8_t	v;
	
	TRY(ret, RME_Int_ParseModRM_MRd8(State, &dest, &v));
	MAKE_IMM8(src);
	
	SHIFT_SELECT_OPERATION();
	TRY(ret, RME_Int_WriteV8(State, &dest, v));
	
	return 0;
}

// 0xC1 - Shift Extended by Imm8
DEF_OPCODE_FCN(Shift, MI8X)
{
	 int	ret, op_num = Param;
	struct ValueRefX	dest;
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &dest, &v));
		MAKE_IMM8(src);

		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV32(State, &dest, v));
	}
	else
	{
		uint16_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &dest, &v));
		MAKE_IMM8(src);
		
		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV16(State, &dest, v));
	}
	
	return 0;
}

// 0xD0 - Shift with 1
DEF_OPCODE_FCN(Shift, M1)
{
	 int	ret, op_num = Param;
	struct ValueRef	dest;
	const uint8_t	src = 1;
	uint8_t v;
	
	TRY(ret, RME_Int_ParseModRM_MRd8(State, &dest, &v));
	RME_Int_DebugPrint(State, " 1");
	
	SHIFT_SELECT_OPERATION();
	TRY(ret, RME_Int_WriteV8(State, &dest, v));
	
	return 0;
}

// 0xD1 - Shift Extended with 1
DEF_OPCODE_FCN(Shift, M1X)
{
	 int	ret, op_num = Param;
	struct ValueRefX	dest;
	const uint8_t	src = 1;
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &dest, &v));
		RME_Int_DebugPrint(State, " 1");
		
		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV32(State, &dest, v));
	}
	else
	{
		uint16_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &dest, &v));
		RME_Int_DebugPrint(State, " 1");
		
		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV16(State, &dest, v));
	}
	
	return 0;
}

// 0xD2 - Shift with CL
DEF_OPCODE_FCN(Shift, MCl)
{
	 int	ret, op_num = Param;
	struct ValueRef	dest;
	const uint8_t	src = State->CX.B.L;
	uint8_t v;
	
	TRY(ret, RME_Int_ParseModRM_MRd8(State, &dest, &v));
	RME_Int_DebugPrint(State, " CL");
	
	SHIFT_SELECT_OPERATION();
	TRY(ret, RME_Int_WriteV8(State, &dest, v));
	
	return 0;
}

// 0xD3 - Shift Extended with CL
DEF_OPCODE_FCN(Shift, MClX)
{
	 int	ret, op_num = Param;
	struct ValueRefX	dest;
	uint8_t	src = State->CX.B.L;
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd32(State, &dest, &v));
		RME_Int_DebugPrint(State, " CL");
		
		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV32(State, &dest, v));
	}
	else
	{
		uint16_t	v;
		
		TRY(ret, RME_Int_ParseModRMX_MRd16(State, &dest, &v));
		RME_Int_DebugPrint(State, " CL");
		
		SHIFT_SELECT_OPERATION();
		TRY(ret, RME_Int_WriteV16(State, &dest, v));
	}
	
	return 0;
}

/// Helper for the double-precision shift operations
#define _EXT_OP_SD(srcPtr, destPtr, __code)	struct ValueRefX rrr, mmm;\
	TRY(ret, RME_Int_ParseModRMXRev(State, &rrr, &mmm));\
	if(State->Decoder.bOverrideOperand) { \
		const int	width = 32; \
		uint32_t	val_l, val_r; \
		TRY(ret, RME_Int_ReadV32(State, &mmm, &val_l));\
		TRY(ret, RME_Int_ReadV32(State, &rrr, &val_r));\
		{__code} \
	} else { \
		const int	width = 16; \
		uint16_t	val_l, val_r; \
		TRY(ret, RME_Int_ReadV16(State, &mmm, &val_l));\
		TRY(ret, RME_Int_ReadV16(State, &rrr, &val_r));\
		{__code} \
	}

// 0x0F 0xAC - Double Precision Shift Right by imm8
DEF_OPCODE_FCN(SHRD, I8)
{
	 int	ret;
	uint8_t	amt;

	_EXT_OP_SD(srcPtr, destPtr, {
		READ_INSTR8(amt);
		RME_Int_DebugPrint(State, " %i", amt);
		ALU_OPCODE_SHRD_CODE
	});
	return 0;
}

// 0x0F 0xAD - Double Precision Shift Right by CL
DEF_OPCODE_FCN(SHRD, Cl)
{
	 int	ret;
	uint8_t	amt;

	_EXT_OP_SD(srcPtr, destPtr, {
		amt = State->CX.B.L;
		RME_Int_DebugPrint(State, " CL");
		ALU_OPCODE_SHRD_CODE
	});
	return 0;
}

// 0x0F 0xA4 - Double Precision Shift Left by imm8
DEF_OPCODE_FCN(SHLD, I8)
{
	 int	ret;
	uint8_t	amt;

	_EXT_OP_SD(srcPtr, destPtr, {	
		READ_INSTR8(amt);
		RME_Int_DebugPrint(State, " %i", amt);
		ALU_OPCODE_SHLD_CODE
	});
	return 0;
}

// 0x0F 0xA5 - Double Precision Shift Left by CL
DEF_OPCODE_FCN(SHLD, Cl)
{
	 int	ret;
	uint8_t	amt;

	_EXT_OP_SD(srcPtr, destPtr, {
		amt = State->CX.B.L;
		RME_Int_DebugPrint(State, " CL");
		ALU_OPCODE_SHLD_CODE
	});
	return 0;
}

