/*
 * Realmode Emulator
 * - Opcode Prefixes
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
#include "ops_alu.h"

#define STRING_HEAD(__check_zf, __use_di, __use_si, __before, __after)	do{\
	uint32_t	srcOfs, destOfs; \
	uint32_t	mask; \
	DEBUG_S(__before); \
	if( State->Decoder.bOverrideAddress ) { \
		mask = 0xFFFFFFFF; \
		srcOfs = State->SI.D;	destOfs = State->SI.D; \
		if(__use_di)	DEBUG_S(" ES:[EDI]"); \
		if(__use_si)	DEBUG_S(" DS:[ESI]"); \
	} else { \
		mask = 0xFFFF; \
		srcOfs = State->SI.W;	destOfs = State->SI.W; \
		if(__use_di)	DEBUG_S(" ES:[DI]"); \
		if(__use_si)	DEBUG_S(" DS:[SI]"); \
	} \
	DEBUG_S(__after); \
	if( !__check_zf && State->Decoder.RepeatType == REP ) { \
		DEBUG_S(" (max 0x%x times)", State->CX.W); \
		if( State->CX.W == 0 ) { \
			State->Decoder.RepeatType = 0; \
			return 0; \
		} \
	} \
	do { \
		if( __check_zf && State->Decoder.RepeatType == REPNZ && (State->Flags & FLAG_ZF) ) \
			break; \
		if( __check_zf && State->Decoder.RepeatType == REP && !(State->Flags & FLAG_ZF) ) \
			break; \
		if( !__check_zf && State->Decoder.RepeatType == REP && State->CX.W ) \
			break; \
		-- State->CX.W; \
		do { \
			while(0)
#define STRING_FOOTER() \
		} while(0);\
		if(State->Flags & FLAG_DF) { \
			srcOfs --; destOfs --; \
		} else { \
			srcOfs ++; destOfs ++; \
		} \
		srcOfs &= mask; destOfs &= mask; \
	} while(State->Decoder.RepeatType); \
	State->Decoder.RepeatType = 0; \
	} while(0)

// === CODE ===
DEF_OPCODE_FCN(MOV, SB)
{
	 int	ret;
	STRING_HEAD(0, 1, 1, "", "");	// REP, using DI and SI
	// ---
	uint8_t	tmp;
	ret = RME_Int_Read8(State, State->DS, srcOfs, &tmp);
	if(ret)	return ret;
	ret = RME_Int_Write8(State, State->ES, destOfs, tmp);
	if(ret)	return ret;
	// ---
	STRING_FOOTER();
	
	return 0;
}

DEF_OPCODE_FCN(MOV, SW)
{
	 int	ret;
	STRING_HEAD(0, 1, 1, "", "");	// REP, using DI and SI
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	tmp;
		ret = RME_Int_Read32(State, State->DS, srcOfs, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write32(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
	}
	else
	{
		uint16_t	tmp;
		ret = RME_Int_Read16(State, State->DS, srcOfs, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write16(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
	}
	// ---
	STRING_FOOTER();
	
	return 0;
}

DEF_OPCODE_FCN(STO, SB)
{
	 int	ret;
	STRING_HEAD(0, 1, 0, "", " AL");
	
	ret = RME_Int_Write8(State, State->ES, destOfs, State->AX.B.L);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(STO, SW)
{
	 int	ret;
	STRING_HEAD(0, 1, 0, "", " AX");
	
	if( State->Decoder.bOverrideOperand )
		ret = RME_Int_Write32(State, State->ES, destOfs, State->AX.D);
	else
		ret = RME_Int_Write16(State, State->ES, destOfs, State->AX.W);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(LOD, SB)	// REP
{
	 int	ret;
	STRING_HEAD(0, 0, 1, " AL", "");
	
	ret = RME_Int_Read8(State, State->ES, destOfs, &State->AX.B.L);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(LOD, SW)
{
	 int	ret;
	STRING_HEAD(0, 0, 1, " AX", "");
	
	if( State->Decoder.bOverrideOperand )
		ret = RME_Int_Read32(State, State->ES, destOfs, &State->AX.D);
	else
		ret = RME_Int_Read16(State, State->ES, destOfs, &State->AX.W);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}

DEF_OPCODE_FCN(CMP, SB)
{
	 int	ret;
	STRING_HEAD(1, 1, 1, "", "");
	
	uint8_t	tmp1, tmp2;
	uint8_t	*dest=&tmp1, *src=&tmp2;
	const int	width = 8;
	
	ret = RME_Int_Read8(State, State->DS, srcOfs, &tmp1);
	if(ret)	return ret;
	ret = RME_Int_Read8(State, State->ES, destOfs, &tmp2);
	if(ret)	return ret;
	
	{ALU_OPCODE_CMP_CODE}
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(CMP, SW)
{
	 int	ret;
	 
	STRING_HEAD(1, 1, 1, "", "");
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	tmp1, tmp2;
		uint32_t	*dest=&tmp1, *src=&tmp2;
		const int	width = 32;
		ret = RME_Int_Read32(State, State->DS, srcOfs, &tmp1);
		if(ret)	return ret;
		ret = RME_Int_Read32(State, State->ES, destOfs, &tmp2);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
	}
	else
	{
		uint16_t	tmp1, tmp2;
		uint16_t	*dest=&tmp1, *src=&tmp2;
		const int	width = 16;
		ret = RME_Int_Read16(State, State->DS, srcOfs, &tmp1);
		if(ret)	return ret;
		ret = RME_Int_Read16(State, State->ES, destOfs, &tmp2);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
	}
	
	STRING_FOOTER();
	return 0;
}

// String Compare with A
DEF_OPCODE_FCN(SCA, SB)
{
	 int	ret;
	STRING_HEAD(1, 1, 0, "", "");
	
	uint8_t	tmp;
	uint8_t	*dest=&tmp, *src=&State->AX.B.L;
	const int	width = 8;
	ret = RME_Int_Read8(State, State->ES, destOfs, &tmp);
	if(ret)	return ret;
	
	{ALU_OPCODE_CMP_CODE}
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(SCA, SW)
{
	 int	ret;
	 
	STRING_HEAD(1, 1, 0, "", "");
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	tmp;
		uint32_t	*dest=&tmp, *src=&State->AX.D;
		const int	width = 32;
		ret = RME_Int_Read32(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
	}
	else
	{
		uint16_t	tmp;
		uint16_t	*dest=&tmp, *src=&State->AX.W;
		const int	width = 16;
		ret = RME_Int_Read16(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
	}
	
	STRING_FOOTER();
	return 0;
}
