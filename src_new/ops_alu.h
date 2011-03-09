/*
 * Realmode Emulator
 * - ALU Operations Header
 * 
 */
#ifndef _RME_OPS_ALU_H_
#define _RME_OPS_ALU_H_
 
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
	DEBUG_S(#__name" (AI) AL 0x%02x", *dest); \
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
		DEBUG_S(#__name" (AIX) EAX 0x%08x", *dest); \
		READ_INSTR32( *src ); \
		__code \
		SET_COMM_FLAGS(State,*dest,width); \
	} else { \
		uint16_t *dest=&State->AX.W, *src=(void*)&srcData; \
		 int	width=16;\
		DEBUG_S(#__name" (AIX) AX 0x%04x", *dest); \
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

// 0: Add
// NOTE: 'A' Flag?
#define ALU_OPCODE_ADD_CODE	\
	*dest += *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	State->Flags |= (*dest < *src) ? FLAG_OF|FLAG_CF : 0;
// 1: Bitwise OR
#define ALU_OPCODE_OR_CODE	\
	*dest |= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 2: Add with carry
#define ALU_OPCODE_ADC_CODE	\
	*dest += *src + ((State->Flags & FLAG_CF) ? 1 : 0); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if( *dest < *src )	State->Flags |= FLAG_OF|FLAG_CF;
// 3: Subtract with Borrow
#define ALU_OPCODE_SBB_CODE	\
	int v = *dest - *src + ((State->Flags & FLAG_CF) ? 1 : 0); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if( *dest < *src || *src == (((1<<(width-1))-1)|(1<<(width-1))) )	State->Flags |= FLAG_CF; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1<<(width-1)) )	State->Flags |= FLAG_OF; \
	*dest = v;
// 4: Bitwise AND
#define ALU_OPCODE_AND_CODE	\
	*dest &= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 5: Subtract
#define ALU_OPCODE_SUB_CODE	\
	int v = *dest - *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if( *dest < *src )	State->Flags |= FLAG_CF; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1<<(width-1)) )	State->Flags |= FLAG_OF; \
	*dest = v;
// 6: Bitwise XOR
#define ALU_OPCODE_XOR_CODE	\
	*dest ^= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 7: Compare
// NOTE: The variable `hack` is just used as dummy space
#define ALU_OPCODE_CMP_CODE	\
	int v = *dest - *src; \
	uint32_t hack = 0;\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if( *dest < *src )	State->Flags |= FLAG_CF; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1<<(width-1)) )	State->Flags |= FLAG_OF; \
	dest = (void*)&hack; \
	*dest = v;
// x: Test
// NOTE: The variable `hack` is just used as dummy space
#define ALU_OPCODE_TEST_CODE	\
	uint32_t hack = 0; \
	dest = (void*)&hack; \
	*dest &= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);

#endif
