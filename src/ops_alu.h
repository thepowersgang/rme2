/*
 * Realmode Emulator
 * - ALU Operations Header
 * 
 */
#ifndef _RME_OPS_ALU_H_
#define _RME_OPS_ALU_H_

#define _ALU_SMASK	(1ULL << (sizeof(*dest)*8-1))
#define _ALU_NSMASK	((1ULL << (sizeof(*dest)*8-1))-1)

#define _ALU_ADD_SETFLAGS(res,a,b)	\
	State->Flags |= ((a)&_ALU_SMASK) == ((b)&_ALU_SMASK) && ((res)&_ALU_SMASK) != ((a)&_ALU_SMASK) ? FLAG_OF : 0; \
	if( (a) ) \
		State->Flags |= ((res) <= (b)) ? FLAG_CF : 0; \
	else \
		State->Flags |= ((res) < (b)) ? FLAG_CF : 0; \
	if( (a)&15 ) \
		State->Flags |= ((res)&15) <= ((b)&15) ? FLAG_AF : 0;\
	else \
		State->Flags |= ((res)&15) < ((b)&15) ? FLAG_AF : 0;

/// Set flags based on a subtraction:
/// - Carry: Set if there was an unsigned overflow (unsigned RHS is smaller than LHS)
/// - Aux Carry: Set if there was a carry from the low nibble
/// - Overflow: Set if the sign bit flips
#define __SUB_FLAGS(res,a,b)	\
	State->Flags |= ((a) < r) ? FLAG_CF : 0; \
	State->Flags |= (((a)&15) < (r&15)-c) ? FLAG_AF : 0; \
	typeof(r) _sub_tmp = ( (((a) ^ r) & ((a) ^ v)) & (1ULL<<((sizeof(v)*8)-1)) ); \
	if( _sub_tmp )	State->Flags |= FLAG_OF;
// 0: Add
#define ALU_OPCODE_ADD_CODE	\
	v = val_l + val_r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	_ALU_ADD_SETFLAGS(v, val_l, val_r)
// 1: Bitwise OR
#define ALU_OPCODE_OR_CODE	\
	v = val_l | val_r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 2: Add with carry
#define ALU_OPCODE_ADC_CODE	\
	v = val_l + val_r + ((State->Flags & FLAG_CF) ? 1 : 0); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	_ALU_ADD_SETFLAGS(v, val_l, val_r) \
	if(val_l && v == val_r ) State->Flags |= FLAG_CF;
// 3: Subtract with Borrow
#define ALU_OPCODE_SBB_CODE	\
	int c = (State->Flags & FLAG_CF) ? 1 : 0; \
	uint64_t r = (val_r + c); \
	v = val_l - r; \
	State->Flags &= ~(FLAG_OF|FLAG_CF|FLAG_AF); \
	__SUB_FLAGS(v, val_l, val_r)
// 4: Bitwise AND
#define ALU_OPCODE_AND_CODE	\
	v = val_l & val_r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 5: Subtract
#define ALU_OPCODE_SUB_CODE	\
	const int c = 0; \
	typeof(val_r) r = val_r; \
	v = val_l - r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	__SUB_FLAGS(v, val_l, val_r)
// 6: Bitwise XOR
#define ALU_OPCODE_XOR_CODE	\
	v = val_l ^ val_r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
// 7: Compare
// NOTE: The variable `hack` is just used as dummy space (and, yes it goes
//       out of scope, but nothing else should come back in, so it doesn't
//       matter)
#define ALU_OPCODE_CMP_CODE	\
	const int c = 0; \
	typeof(val_r) r = val_r; \
	v = val_l - r; \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF|FLAG_AF); \
	__SUB_FLAGS(v, val_l, val_r) \
	dest = NULL;

// x: Test
#define ALU_OPCODE_TEST_CODE	\
	v = (val_l) & (val_r); \
	State->Flags &= ~(FLAG_OF|FLAG_CF); \
	dest = NULL;
// x: NOT
#define ALU_OPCODE_NOT_CODE	\
	v = ~src;
// x: NEG
// - TODO: OF/AF?
#define ALU_OPCODE_NEG_CODE	\
	State->Flags &= ~(FLAG_CF|FLAG_AF|FLAG_OF); \
	if( src == 0 ) { \
		v = 0;\
	} \
	else { \
		State->Flags |= FLAG_CF; \
		State->Flags |= (src == 1u << (sizeof(src)*8-1)) ? FLAG_OF : 0;\
		State->Flags |= ((src&7) != 0) ? FLAG_AF : 0; \
		v = ~src + 1; \
	} \
	SET_COMM_FLAGS(State, v);

// x: Increment
#define ALU_OPCODE_INC_CODE(v)	\
	(v) ++; \
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF|FLAG_AF); \
	if((v) == (1UL << (sizeof(v)*8-1)))	State->Flags |= FLAG_OF; \
	if(((v)&15) == 0)	State->Flags |= FLAG_AF; \
	SET_COMM_FLAGS(State, (v));
// x: Decrement
#define ALU_OPCODE_DEC_CODE(v)	\
	(v) --; \
	State->Flags &= ~(FLAG_OF|FLAG_AF); \
	if((v) + 1 == (1UL << (sizeof(v)*8-1)))	State->Flags |= FLAG_OF; \
	if(((v) & 15) + 1 == 16)	State->Flags |= FLAG_AF; \
	SET_COMM_FLAGS(State, (v));

// 0: Rotate Left
#define ALU_OPCODE_ROL_CODE	\
	 int	amt = (src & 31) % (sizeof(v)*8); \
	if(amt > 0) { \
		v = (v << amt) | (v >> ((sizeof(v)*8)-amt)); \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		State->Flags |= (v & 1) ? FLAG_CF : 0; \
		State->Flags |= (v >> ((sizeof(v)*8)-1)) ^ (v & 1) ? FLAG_OF : 0; \
	}
// 1: Rotate Right
#define ALU_OPCODE_ROR_CODE	\
	 int	amt = (src & 31) % (sizeof(v)*8); \
	if(amt > 0) { \
		v = (v >> amt) | (v << ((sizeof(v)*8)-amt)); \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		State->Flags |= (v >> ((sizeof(v)*8)-1)) ? FLAG_CF : 0; \
		State->Flags |= ((v >> ((sizeof(v)*8)-1)) ^ (v >> ((sizeof(v)*8)-2))) & 1 ? FLAG_OF : 0; \
	}
	
// 2: Rotate Carry Left
#define ALU_OPCODE_RCL_CODE	\
	 int	amt = (src & 31) % ((sizeof(v)*8)+1); \
	if( amt > 0 ) { \
		 int	cf_new, cf = (State->Flags & FLAG_CF) ? 1 : 0; \
		typeof(v) val = v; \
		while(amt--) { \
			cf_new = val >> ((sizeof(v)*8)-1); \
			val = (val << 1) | cf; \
			cf = cf_new; \
		} \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		State->Flags |= cf ? FLAG_CF : 0; \
		State->Flags |= (cf ^ (val >> ((sizeof(v)*8)-1))) ? FLAG_OF : 0; \
		v = val; \
	}
// 3: Rotate Carry Right
#define ALU_OPCODE_RCR_CODE	\
	 int	amt = (src & 31) % ((sizeof(v)*8)+1); \
	if( amt > 0 ) { \
		 int	cf_new, cf = (State->Flags & FLAG_CF) ? 1 : 0; \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		typeof(v) val = v; \
		State->Flags |= (((val >> ((sizeof(v)*8)-1)) & 1) ^ cf) ? FLAG_OF : 0; \
		while(amt--) { \
			cf_new = val & 1; \
			val = (val >> 1) | (cf << ((sizeof(v)*8)-1)); \
			cf = cf_new; \
		} \
		State->Flags |= cf ? FLAG_CF : 0; \
		v = val; \
	}
// 4: Shift Logical Left
#define ALU_OPCODE_SHL_CODE	\
	unsigned	amt = src & 31; \
	if( amt > 0 ) { \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		if(amt > (sizeof(v)*8)) \
			v = 0; \
		else { \
			State->Flags |= (v >> ((sizeof(v)*8)-amt)) & 1 ? FLAG_CF : 0; \
			State->Flags |= ((v >> ((sizeof(v)*8)-amt)) ^ (v >> ((sizeof(v)*8)-amt-1))) & 1 ? FLAG_OF : 0; \
			v <<= amt; \
		}\
		SET_COMM_FLAGS(State, v); \
	}
// 5: Shift Logical Right
#define ALU_OPCODE_SHR_CODE	\
	unsigned	amt = src & 31; \
	if(amt > 0) { \
		State->Flags &= ~(FLAG_OF|FLAG_CF);\
		if( amt > (sizeof(v)*8) ) { \
			v = 0; \
		} else { \
			State->Flags |= (v >> (amt-1)) & 1 ? FLAG_CF : 0; \
			v >>= amt; \
		} \
		int high = v >> ((sizeof(v)*8)-2); \
		State->Flags |= ((high & 1) ^ (high >> 1)) ? FLAG_OF : 0; \
		SET_COMM_FLAGS(State, v); \
	}
// 6: Shift Arithmetic Left
#define ALU_OPCODE_SAL_CODE	ALU_OPCODE_SHL_CODE
// 7: Shift Arithmetic Right (applies sign extension)
#define ALU_OPCODE_SAR_CODE	\
	unsigned amt = src & 31; \
	if( amt > 0 ) { \
		int sgn = v >> ((sizeof(v)*8)-1); \
		State->Flags &= ~(FLAG_OF|FLAG_CF); \
		if(amt >= (sizeof(v)*8)) { \
			v = sgn ? -1 : 0; \
			State->Flags |= sgn ? FLAG_CF : 0; \
		} else { \
			State->Flags |= ((v >> (amt-1)) & 1) ? FLAG_CF : 0; \
			v = v >> amt; \
			v |= sgn ? 0xFFFFFFFF << ((sizeof(v)*8) - amt) : 0; \
		} \
		int high = v >> ((sizeof(v)*8)-2); \
		State->Flags |= ((high & 1) ^ (high >> 1)) ? FLAG_OF : 0; \
		SET_COMM_FLAGS(State, v); \
	}

// Misc 4: MUL
// CF,OF set if upper bits set; SF, ZF, AF and PF are undefined
#define _MUL_FLAGS(result, width) \
	if(result >> width) \
		State->Flags |= FLAG_CF|FLAG_OF; \
	else \
		State->Flags &= ~(FLAG_CF|FLAG_OF);

// Misc 5: IMUL
// CF,OF set if upper bits set; SF, ZF, AF and PF are undefined
#define _IMUL_FLAGS(result,width)	\
	if(result < -(1ll << (width-1)) || result > ((1ll << (width-1))-1)) \
		State->Flags |= FLAG_CF|FLAG_OF; \
	else \
		State->Flags &= ~(FLAG_CF|FLAG_OF);

// Misc 6: DIV
// NOTE: DIV is a real special case, as it has substantially different
//       behavior between different sizes (due to DX:AX)
#define ALU_OPCODE_DIV_CODE if( src == 0 )	return RME_ERR_DIVERR; \
	switch(sizeof(src)*8) { \
	case 8: { \
		uint16_t	res, rem; \
		res = State->AX.W / src; \
		rem = State->AX.W % src; \
		if(res > 0xFF)	return RME_ERR_DIVERR; \
		State->AX.B.H = rem; \
		State->AX.B.L = res; \
		} break; \
	case 16: { \
		uint32_t	numerator, result, rem; \
		numerator = ((uint32_t)State->DX.W << 16) | State->AX.W; \
		result = numerator / src; \
		rem = numerator % src; \
		if(result > 0xFFFF)	return RME_ERR_DIVERR; \
		State->DX.W = rem; \
		State->AX.W = result; \
		} break; \
	case 32: { \
		uint64_t	numerator, result, rem; \
		numerator = ((uint64_t)State->DX.D << 32) | State->AX.D; \
		result = numerator / src; \
		rem = numerator % src; \
		if(result > 0xFFFFFFFF)	return RME_ERR_DIVERR; \
		State->DX.D = rem; \
		State->AX.D = result; \
		} break; \
	}
	
// Misc 7: IDIV
// NOTE: DIV is a real special case, as it has substantially different
//       behavior between different sizes (due to DX:AX)
// TODO: Test
#define ALU_OPCODE_IDIV_CODE if( src == 0 )	return RME_ERR_DIVERR; \
	switch(sizeof(src)*8) { \
	case 8: { \
		int16_t	quot, rem; \
		int16_t num = (int16_t)State->AX.W, den = (int8_t)src; \
		quot = num / den; \
		rem = num - quot*den; \
		if(quot < -0x80 || quot > 0x7F)	return RME_ERR_DIVERR; \
		State->AX.B.H = rem; \
		State->AX.B.L = quot; \
		} break; \
	case 16: { \
		int32_t	numerator, result; \
		numerator = (int32_t)( ((uint32_t)State->DX.W << 16) | State->AX.W ); \
		result = numerator / (int16_t)src; \
		if(result > 0x7FFF || result < -0x8000)	return RME_ERR_DIVERR; \
		State->DX.W = numerator % ((int16_t)src); \
		State->AX.W = result; \
		} break; \
	case 32: { \
		int64_t	numerator, result; \
		numerator = (int64_t)( ((uint64_t)State->DX.D << 32) | State->AX.D ); \
		result = numerator / (int32_t)src; \
		if(result > 0x7FFFFFFF || result < -0x80000000)	return RME_ERR_DIVERR; \
		State->DX.D = numerator % ((int32_t)src); \
		State->AX.D = result; \
		} break; \
	}

// Double Precision Shift Right
#define ALU_OPCODE_SHRD_CODE	\
	if( amt > 0 ) { \
		State->Flags &= ~(FLAG_CF|FLAG_OF|FLAG_AF); \
		State->Flags |= (val_l >> (amt-1)) & 1 ? FLAG_CF : 0; \
		val_l >>= amt; \
		val_l |= val_r << (width - amt); \
		if( amt == 1 ) \
			State->Flags |= ((val_l >> (width-1)) ^ (val_l >> (width-2))) & 1 ? FLAG_OF : 0; \
		SET_COMM_FLAGS(State, val_l); \
	}
// Double Precision Shift Left
#define ALU_OPCODE_SHLD_CODE	\
	if( amt > 0 ) { \
		State->Flags &= ~(FLAG_CF|FLAG_OF|FLAG_AF); \
		State->Flags |= (val_l >> (width-amt)) & 1 ? FLAG_CF : 0; \
		val_l <<= amt; \
		val_l |= val_r >> (width - amt); \
		if( amt == 1 ) \
			State->Flags |= ((val_l >> (width-1)) ^ !!(State->Flags & FLAG_CF)) & 1 ? FLAG_OF : 0; \
		SET_COMM_FLAGS(State, val_l); \
	}


#endif
