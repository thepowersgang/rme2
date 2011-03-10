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
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	if( *dest < *src )	State->Flags |= FLAG_CF; \
	if( ((*dest ^ *src) & (*dest ^ v)) & (1ULL<<(width-1)) )	State->Flags |= FLAG_OF; \
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

// 1: Rotate Right
#define ALU_OPCODE_ROR_CODE	\
	*dest = (*dest >> *src) | (*dest << (width-*src)); \
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF); \
	State->Flags |= (*dest >> (width-1)) ? FLAG_CF : 0;
// 4: Shift Left
#define ALU_OPCODE_SHL_CODE	\
	*dest <<= *src;\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	State->Flags |= (*dest >> (width-1)) ? FLAG_CF : 0;

#define ALU_OPCODE_SHR_CODE	\
	*dest >>= *src;\
	State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\
	State->Flags |= (*dest & 1) ? FLAG_CF : 0;

#endif
