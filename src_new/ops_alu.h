/*
 * Realmode Emulator
 * - ALU Operations Header
 * 
 */
#ifndef _RME_OPS_ALU_H_
#define _RME_OPS_ALU_H_

// 0: Add
// NOTE: 'A' Flag?
#define ALU_OPCODE_ADD_CODE	\
	*dest += *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	State->Flags |= (*dest < *src) ? FLAG_OF|FLAG_CF : 0; \
	State->Flags |= ((*dest&7) < (*src&7)) ? FLAG_AF : 0;
// 1: Bitwise OR
#define ALU_OPCODE_OR_CODE	\
	*dest |= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 2: Add with carry
#define ALU_OPCODE_ADC_CODE	\
	*dest += *src + ((State->Flags & FLAG_CF) ? 1 : 0); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	State->Flags |= (*dest < *src) ? FLAG_OF|FLAG_CF : 0; \
	State->Flags |= ((*dest&7) < (*src&7)) ? FLAG_AF : 0;
// 3: Subtract with Borrow
#define ALU_OPCODE_SBB_CODE	\
	int v = *dest - *src + ((State->Flags & FLAG_CF) ? 1 : 0); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	if( *dest < *src || *src == (((1ULL<<(width-1))-1)|(1ULL<<(width-1))) )	State->Flags |= FLAG_CF; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1ULL<<(width-1)) )	State->Flags |= FLAG_OF; \
	*dest = v;
// 4: Bitwise AND
#define ALU_OPCODE_AND_CODE	\
	*dest &= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 5: Subtract
#define ALU_OPCODE_SUB_CODE	\
	int v = *dest - *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	State->Flags |= (*dest < (*src) ? FLAG_CF : 0; \
	State->Flags |= ((*dest&7) < (*src&7)) ? FLAG_AF : 0; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1ULL<<(width-1)) )	State->Flags |= FLAG_OF; \
	*dest = v;
// 6: Bitwise XOR
#define ALU_OPCODE_XOR_CODE	\
	*dest ^= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 7: Compare
// NOTE: The variable `hack` is just used as dummy space (and, yes it goes
//       out of scope, but nothing else should come back in, so it doesn't
//       matter)
#define ALU_OPCODE_CMP_CODE	\
	int v = *dest - *src; \
	uint32_t hack = 0;\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	State->Flags |= (*dest < (*src) ? FLAG_CF : 0; \
	State->Flags |= ((*dest&7) < (*src&7)) ? FLAG_AF : 0; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1ULL<<(width-1)) )	State->Flags |= FLAG_OF; \
	dest = (void*)&hack; \
	*dest = v;

// x: Test
// NOTE: The variable `hack` is just used as dummy space
#define ALU_OPCODE_TEST_CODE	\
	uint32_t hack = 0; \
	dest = (void*)&hack; \
	*dest &= *src; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// x: NOT
#define ALU_OPCODE_NOT_CODE	\
	*dest = ~*dest;
// x: NEG
// - TODO: OF/AF?
#define ALU_OPCODE_NEG_CODE	\
	if( *dest == 0 )	State->Flags &= ~FLAG_CF; \
	else { \
		State->Flags |= FLAG_CF; \
		*dest = -*dest; \
	}

// x: Increment
#define ALU_OPCODE_INC_CODE	\
	(*dest) ++; \
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF); \
	if(*dest == 0)	State->Flags |= FLAG_OF;
// x: Decrement
#define ALU_OPCODE_DEC_CODE	\
	(*dest) --; \
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF); \
	if(*dest + 1 == 0)	State->Flags |= FLAG_OF;

// 0: Rotate Left
#define ALU_OPCODE_ROL_CODE	\
	 int	amt = (*src & 31) % width; \
	*dest = (*dest << amt) | (*dest >> (width-amt)); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	State->Flags |= (*dest >> (width-1)) ? FLAG_CF : 0;
// 1: Rotate Right
#define ALU_OPCODE_ROR_CODE	\
	 int	amt = (*src & 31) % width; \
	*dest = (*dest >> amt) | (*dest << (width-amt)); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	State->Flags |= (*dest >> (width-1)) ? FLAG_CF : 0;
	
// 2: Rotate Carry Left
#define ALU_OPCODE_RCL_CODE	\
	if(*src > 0) { \
	 int	amt = (*src & 31) % width+1; \
	uint64_t	carry = (State->Flags & FLAG_CF) ? 1 : 0; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	carry <<= amt; \
	if( (*dest >> (width-amt)) & 1 )	State->Flags |= FLAG_CF; \
	*dest = (*dest << amt) | (*dest >> (width-amt-1)) | carry; \
	}
// 3: Rotate Carry Right
#define ALU_OPCODE_RCR_CODE	\
	if(*src > 0) { \
	 int	amt = (*src & 31) % width+1; \
	uint64_t	carry = (State->Flags & FLAG_CF) ? 1 : 0; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	carry <<= (width-amt); \
	if( (*dest >> (amt-1)) & 1 )	State->Flags |= FLAG_CF; \
	*dest = (*dest >> amt) | (*dest << (width-amt-1)) | carry; \
	}
// 4: Shift Logical Left
#define ALU_OPCODE_SHL_CODE	\
	 int	amt = *src & 31; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if(amt > 0 && amt <= width) \
		State->Flags |= ((*dest >> (width-amt)) & 1) ? FLAG_CF : 0; \
	if(amt > 0 && amt < width) \
		*dest <<= amt;
// 5: Shift Logical Right
#define ALU_OPCODE_SHR_CODE	\
	 int	amt = *src & 31; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	if(amt > 0 && amt <= width) \
		State->Flags |= ((*dest >> (amt-1)) & 1) ? FLAG_CF : 0; \
	if(amt > 0 && amt < width) \
		*dest >>= amt;
// 6: Shift Arithmetic Left
#define ALU_OPCODE_SAL_CODE	ALU_OPCODE_SHL_CODE
// 7: Shift Arithmetic Right (applies sign extension)
#define ALU_OPCODE_SAR_CODE	\
	 int	amt = *src & 31; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	if(amt > 0 && amt <= width) \
		State->Flags |= ((*dest >> (amt-1)) & 1) ? FLAG_CF : 0; \
	if(amt > 0 && amt < width) { \
		*dest >>= amt; \
		if((*dest >> (width-amt)) & 1) \
			*dest |= 0xFFFFFFFF >> width; \
	}

// Misc 4: MUL
// CF,OF set if upper bits set; SF, ZF, AF and PF are undefined
#define ALU_OPCODE_MUL_CODE \
	uint64_t	result;\
	switch(width) { \
	case 8: \
		result = (uint16_t)State->AX.B.L * *src; \
		State->AX.W = result; \
		break; \
	case 16: \
		result = (uint32_t)State->AX.W * (*src); \
		State->DX.W = result >> 16; \
		State->AX.W = result & 0xFFFF; \
		break; \
	case 32: \
		result = (uint64_t)State->AX.D * (*src); \
		State->DX.D = result >> 32; \
		State->AX.D = result & 0xFFFFFFFF; \
		break; \
	} \
	if(result >> width) \
		State->Flags |= FLAG_CF|FLAG_OF; \
	else \
		State->Flags &= ~(FLAG_CF|FLAG_OF);
// Misc 5: IMUL
// CF,OF set if upper bits set; SF, ZF, AF and PF are undefined
#define ALU_OPCODE_IMUL_CODE \
	int64_t	result;\
	switch(width) { \
	case 8: \
		result = (int16_t)State->AX.B.L * (*(int8_t*)src); \
		State->AX.W = result; \
		break; \
	case 16: \
		result = (int32_t)State->AX.W * (*(int16_t*)src); \
		State->DX.W = result >> 16; \
		State->AX.W = result & 0xFFFF; \
		break; \
	case 32: \
		result = (uint64_t)State->AX.D * (*(int32_t*)src); \
		State->DX.D = result >> 32; \
		State->AX.D = result & 0xFFFFFFFF; \
		break; \
	} \
	if(result >> width) \
		State->Flags |= FLAG_CF|FLAG_OF; \
	else \
		State->Flags &= ~(FLAG_CF|FLAG_OF);

// Misc 6: DIV
// NOTE: DIV is a real special case, as it has substantially different
//       behavior between different sizes (due to DX:AX)
#define ALU_OPCODE_DIV_CODE if( *src == 0 )	return RME_ERR_DIVERR; \
	switch(width) { \
	case 8: { \
		uint16_t	result; \
		result = State->AX.W / *src; \
		if(result > 0xFF)	return RME_ERR_DIVERR; \
		State->AX.B.H = State->AX.W - result * (*src); \
		State->AX.B.L = result; \
		} break; \
	case 16: { \
		uint32_t	numerator, result; \
		numerator = ((uint32_t)State->DX.W << 16) | State->AX.W; \
		result = numerator / *src; \
		if(result > 0xFFFF)	return RME_ERR_DIVERR; \
		State->AX.W = result; \
		State->DX.W = numerator - result * (*src); \
		} break; \
	case 32: { \
		uint64_t	numerator, result; \
		numerator = ((uint64_t)State->DX.D << 32) | State->AX.D; \
		result = numerator / *src; \
		if(result > 0xFFFFFFFF)	return RME_ERR_DIVERR; \
		State->AX.D = result; \
		State->DX.D = numerator - result * (*src); \
		} break; \
	}
	
// Misc 7: IDIV
// NOTE: DIV is a real special case, as it has substantially different
//       behavior between different sizes (due to DX:AX)
// TODO: Test
#define ALU_OPCODE_IDIV_CODE if( *src == 0 )	return RME_ERR_DIVERR; \
	switch(width) { \
	case 8: { \
		int16_t	result; \
		result = (int16_t)State->AX.W / *(int8_t*)src; \
		if(result & 0xFF00)	return RME_ERR_DIVERR; \
		State->AX.B.H = (int16_t)State->AX.W % *(int8_t*)src; \
		State->AX.B.L = result; \
		} break; \
	case 16: { \
		int32_t	numerator, result; \
		numerator = (int32_t)( ((uint32_t)State->DX.W << 16) | State->AX.W ); \
		result = numerator / *(int16_t*)src; \
		if(result > 0x7FFF || result < -0x8000)	return RME_ERR_DIVERR; \
		State->AX.W = result; \
		State->DX.W = numerator % (*(int16_t*)src); \
		} break; \
	case 32: { \
		int64_t	numerator, result; \
		numerator = (int64_t)( ((uint64_t)State->DX.D << 32) | State->AX.D ); \
		result = numerator / *(int32_t*)src; \
		if(result > 0x7FFFFFFF || result < -0x80000000)	return RME_ERR_DIVERR; \
		State->AX.D = result; \
		State->DX.D = numerator % (*(int32_t*)src); \
		} break; \
	}

#endif
