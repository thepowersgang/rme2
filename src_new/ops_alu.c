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
	const int	width=8; \
	uint8_t	*dest=0, *src=0; \
	DEBUG_S(#__name" (MRX)"); \
	ret = RME_Int_ParseModRM(State, &dest, &src, 0); \
	if(ret)	return ret; \
	__code \
	SET_COMM_FLAGS(State,*dest,width); \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_RMX(__name, __code...) DEF_OPCODE_FCN(__name,RMX) {\
	 int	ret; \
	void	*destPtr=0, *srcPtr=0; \
	DEBUG_S(#__name" (RMX)"); \
	ret = RME_Int_ParseModRMX(State, (void*)&destPtr, (void*)&srcPtr, 0); \
	if(ret)	return ret; \
	if(State->Decoder.bOverrideOperand) { \
		uint32_t *dest=destPtr, *src=srcPtr; \
		 int	width=32;\
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} else { \
		uint16_t *dest=destPtr, *src=srcPtr; \
		 int	width=16;\
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_MR(__name, __code...) DEF_OPCODE_FCN(__name,MR) {\
	 int	ret;\
	const int	width=8; \
	uint8_t	*dest=0, *src=0; \
	DEBUG_S(#__name" (MR)"); \
	ret = RME_Int_ParseModRM(State, &src, &dest, 1); \
	if(ret)	return ret; \
	__code \
	SET_COMM_FLAGS(State,*dest,width); \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_MRX(__name, __code...) DEF_OPCODE_FCN(__name,MRX) {\
	 int	ret; \
	void	*destPtr=0, *srcPtr=0; \
	DEBUG_S(#__name" (MRX)"); \
	ret = RME_Int_ParseModRMX(State, (void*)&srcPtr, (void*)&destPtr, 1); \
	if(ret)	return ret; \
	if(State->Decoder.bOverrideOperand) { \
		uint32_t *dest=destPtr, *src=srcPtr; \
		 int	width=32;\
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} else { \
		uint16_t *dest=destPtr, *src=srcPtr; \
		 int	width=16;\
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_AI(__name, __code...) DEF_OPCODE_FCN(__name,AI) {\
	const int	width=8; \
	uint8_t srcData; \
	uint8_t	*dest=&State->AX.B.L, *src=&srcData; \
	DEBUG_S(#__name" (AI) AL 0x%02x", *src); \
	READ_INSTR8( srcData ); \
	__code \
	SET_COMM_FLAGS(State,*dest,width); \
	return 0; \
}
#define CREATE_ALU_OPCODE_FCN_AIX(__name, __code...) DEF_OPCODE_FCN(__name,AIX) {\
	uint32_t srcData; \
	if(State->Decoder.bOverrideOperand) { \
		uint32_t *dest=&State->AX.D, *src=(void*)&srcData; \
		 int	width=32;\
		DEBUG_S(#__name" (AIX) EAX 0x%08x", *src); \
		READ_INSTR32( *src ); \
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} else { \
		uint16_t *dest=&State->AX.W, *src=(void*)&srcData; \
		 int	width=16;\
		DEBUG_S(#__name" (AIX) AX 0x%04x", *src); \
		READ_INSTR16( *src ); \
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
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
	} } while(0)
#define LOGIC_SELECT_OPERATION()	do{ switch( op_num ) {\
	case 1: do { ALU_OPCODE_ROR_CODE } while(0);	break; \
	case 4:	RME_Int_DoShl(State, (to), (from), (width));	break;\
	case 5:	RME_Int_DoShr(State, (to), (from), (width));	break;\
	default: ERROR_S(" - Logic Undef %i\n", op_num); return RME_ERR_UNDEFOPCODE;\
	} }while(0)

DEF_OPCODE_FCN(Arith, RI)
{
	 int	ret;
	const int width = 8;
	uint8_t	srcData;
	uint8_t *dest, *src = &srcData;
	 int	op_num;
	
	// Get mmm term from ModRM byte
	READ_INSTR8( srcData );	State->Decoder.IPOffset --;
	op_num = (srcData >> 3) & 7;
	
	ret = RME_Int_ParseModRM(State, NULL, &dest, 0);
	if(ret)	return ret;
	
	// Read data, perform operation, set common flags
	READ_INSTR8(srcData);
	ALU_SELECT_OPERATION();
	SET_COMM_FLAGS(State, *dest, width);
	
	return 0;
}

DEF_OPCODE_FCN(Arith, RIX)
{
	 int	ret;
	 int	op_num;
	uint8_t	byte;
	void	*destPtr;
	
	// Read mmm from ModRM
	READ_INSTR8( byte );	State->Decoder.IPOffset --;
	op_num = (byte >> 3) & 7;
	
	ret = RME_Int_ParseModRMX(State, NULL, (void*)&destPtr, 0);
	if(ret)	return ret;
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	srcData, *src = &srcData, *dest = destPtr;
		const int	width = 32;
		READ_INSTR32(srcData);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, *dest, width);
		State->Flags |= (*dest == 0xFFFFFFFF) ? FLAG_OF : 0;
	}
	else
	{
		uint16_t	srcData, *src = &srcData, *dest = destPtr;
		const int	width = 16;
		READ_INSTR16(srcData);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, *dest, width);
	}
	
	return 0;
}

DEF_OPCODE_FCN(INC, Reg)
{
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		(*dest) ++;
		SET_COMM_FLAGS(State, *dest, 32);
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		(*dest) ++;
		SET_COMM_FLAGS(State, *dest, 16);
	}
	
	if(State->Flags & FLAG_ZF)
		State->Flags |= FLAG_OF;
	return 0;
}

DEF_OPCODE_FCN(DEC, Reg)
{
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		(*dest) --;
		SET_COMM_FLAGS(State, *dest, 32);
		State->Flags |= (*dest == 0xFFFFFFFF) ? FLAG_OF : 0;
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		(*dest) --;
		SET_COMM_FLAGS(State, *dest, 16);
		State->Flags |= (*dest == 0xFFFF) ? FLAG_OF : 0;
	}
	return 0;
}

DEF_OPCODE_FCN(ArithMisc, MI)	// 0xF6
{
	 int	ret, rrr;
	const int	width = 8;
	uint8_t	val;
	uint8_t	*src, *dest;

	ret = RME_Int_GetModRM(State, NULL, &rrr, NULL);	State->Decoder.IPOffset --;
	if(ret)	return ret;
	
	switch(rrr)
	{
	case 0:	// TEST r/m8, Imm8
		DEBUG_S("TEST");		
		
		ret = RME_Int_ParseModRM(State, NULL, &src, 0);
		if(ret)	return ret;
		
		READ_INSTR8(val);
		dest = &val;
		
		DEBUG_S(" 0x%02x", val);
		
		{ALU_OPCODE_TEST_CODE}
		
		SET_COMM_FLAGS(State, *dest, width);
		break;
	default:
		#warning "TODO: Implement more in ArithMisc,MIX"
		ERROR_S("0xF6 /%i Unimplemented\n", rrr);
		return RME_ERR_UNDEFOPCODE;
	}
	
	return 0;
}

DEF_OPCODE_FCN(ArithMisc, MIX)	// 0xF7
{
	 int	ret, rrr;
	uint8_t	*arg;

	ret = RME_Int_GetModRM(State, NULL, &rrr, NULL);	State->Decoder.IPOffset --;
	if(ret)	return ret;
	
	ret = RME_Int_ParseModRM(State, NULL, &arg, 0);
	if(ret)	return ret;
	
	switch(rrr)
	{
	default:
		#warning "TODO: Implement more in ArithMisc,MIX"
		ERROR_S("0xF7 /%i Unimplemented\n", rrr);
		return RME_ERR_UNDEFOPCODE;
	}
	
	return 0;
}

DEF_OPCODE_FCN(Logic, MI8X)
{
	 int	ret, op_num;
	uint8_t	*arg;

	ret = RME_Int_GetModRM(State, NULL, &op_num, NULL);	State->Decoder.IPOffset --;
	if(ret)	return ret;
	
	ret = RME_Int_ParseModRM(State, NULL, &arg, 0);
	if(ret)	return ret;
	
	#warning "TODO: Implement more in Logic,MI8X"
	return RME_ERR_UNDEFOPCODE;
}
