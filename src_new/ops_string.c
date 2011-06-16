/*
 * Realmode Emulator
 * - Opcode Prefixes
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
#include "ops_alu.h"	// For ALU_OPCODE_CMP_CODE
#include "ops_io.h"	// for inB/inW/...

#define STRING_HEAD(__check_zf, __use_di, __use_si, __before, __after, __step)	do{\
	const int __using_di=__use_di,__using_si=__use_si; \
	const int __step_bytes = __step; \
	uint32_t	srcOfs, destOfs; \
	uint32_t	mask; \
	DEBUG_S("%s",__before); \
	if( State->Decoder.bOverrideAddress ) { \
		mask = 0xFFFFFFFF; \
		srcOfs = State->SI.D;	destOfs = State->DI.D; \
		if(__use_di)	DEBUG_S(" ES:[EDI]"); \
		if(__use_si)	DEBUG_S(" DS:[ESI]"); \
	} else { \
		mask = 0xFFFF; \
		srcOfs = State->SI.W;	destOfs = State->DI.W; \
		if(__use_di)	DEBUG_S(" ES:[DI]"); \
		if(__use_si)	DEBUG_S(" DS:[SI]"); \
	} \
	DEBUG_S("%s",__after); \
	if( !__check_zf && State->Decoder.RepeatType ) { \
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
		if( !__check_zf && State->Decoder.RepeatType && !State->CX.W ) \
			break; \
		-- State->CX.W; \
		do { \
			while(0)
#define STRING_FOOTER() \
		} while(0);\
		if(State->Flags & FLAG_DF) { \
			srcOfs -= __step_bytes; destOfs -= __step_bytes; \
		} else { \
			srcOfs += __step_bytes; destOfs += __step_bytes; \
		} \
		srcOfs &= mask; destOfs &= mask; \
	} while(State->Decoder.RepeatType); \
	if( State->Decoder.bOverrideAddress ) { \
		if(__using_di)	State->DI.D = destOfs; \
		if(__using_si)	State->SI.D = srcOfs; \
	} else { \
		if(__using_di)	State->DI.W = destOfs; \
		if(__using_si)	State->SI.W = srcOfs; \
	} \
	State->Decoder.RepeatType = 0; \
	} while(0)

// === CODE ===
DEF_OPCODE_FCN(MOV, SB)
{
	 int	ret;
	STRING_HEAD(0, 1, 1, "", "", 1);	// REP, using DI and SI
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
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	tmp;
		STRING_HEAD(0, 1, 1, "", "", 4);	// REP, using DI and SI
		ret = RME_Int_Read32(State, State->DS, srcOfs, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write32(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	else
	{
		uint16_t	tmp;
		STRING_HEAD(0, 1, 1, "", "", 2);	// REP, using DI and SI
		ret = RME_Int_Read16(State, State->DS, srcOfs, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write16(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	
	return 0;
}

DEF_OPCODE_FCN(STO, SB)
{
	 int	ret;
	STRING_HEAD(0, 1, 0, "", " AL", 1);
	
	ret = RME_Int_Write8(State, State->ES, destOfs, State->AX.B.L);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(STO, SW)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(0, 1, 0, "", " EAX", 4);
		ret = RME_Int_Write32(State, State->ES, destOfs, State->AX.D);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(0, 1, 0, "", " AX", 2);
		ret = RME_Int_Write16(State, State->ES, destOfs, State->AX.W);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	
	return 0;
}
DEF_OPCODE_FCN(LOD, SB)	// REP
{
	 int	ret;
	STRING_HEAD(0, 0, 1, " AL", "", 1);
	
	ret = RME_Int_Read8(State, State->DS, srcOfs, &State->AX.B.L);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(LOD, SW)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(0, 0, 1, " EAX", "", 4);
		ret = RME_Int_Read32(State, State->DS, srcOfs, &State->AX.D);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(0, 0, 1, " AX", "", 2);
		ret = RME_Int_Read16(State, State->DS, srcOfs, &State->AX.W);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	return 0;
}

DEF_OPCODE_FCN(CMP, SB)
{
	 int	ret;
	STRING_HEAD(1, 1, 1, "", "", 1);
	
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
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(1, 1, 1, "", "", 4);
		uint32_t	tmp1, tmp2;
		uint32_t	*dest=&tmp1, *src=&tmp2;
		const int	width = 32;
		
		ret = RME_Int_Read32(State, State->DS, srcOfs, &tmp1);
		if(ret)	return ret;
		ret = RME_Int_Read32(State, State->ES, destOfs, &tmp2);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
		SET_COMM_FLAGS(State, *dest, width);
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(1, 1, 1, "", "", 2);
		uint16_t	tmp1, tmp2;
		uint16_t	*dest=&tmp1, *src=&tmp2;
		const int	width = 16;
		
		ret = RME_Int_Read16(State, State->DS, srcOfs, &tmp1);
		if(ret)	return ret;
		ret = RME_Int_Read16(State, State->ES, destOfs, &tmp2);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
		SET_COMM_FLAGS(State, *dest, width);
		STRING_FOOTER();
	}
	
	return 0;
}

// String Compare with A
DEF_OPCODE_FCN(SCA, SB)
{
	 int	ret;
	STRING_HEAD(1, 1, 0, "", "", 1);
	
	uint8_t	tmp;
	uint8_t	*dest=&tmp, *src=&State->AX.B.L;
	const int	width = 8;
	ret = RME_Int_Read8(State, State->ES, destOfs, &tmp);
	if(ret)	return ret;
	
	{ALU_OPCODE_CMP_CODE}
	SET_COMM_FLAGS(State, *dest, width);
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(SCA, SW)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(1, 1, 0, "", " EAX", 4);
		uint32_t	tmp;
		uint32_t	*dest=&tmp, *src=&State->AX.D;
		const int	width = 32;
		
		ret = RME_Int_Read32(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
		SET_COMM_FLAGS(State, *dest, width);
		
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(1, 1, 0, "", " AX", 2);
		uint16_t	tmp;
		uint16_t	*dest=&tmp, *src=&State->AX.W;
		const int	width = 16;
		
		ret = RME_Int_Read16(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		
		{ALU_OPCODE_CMP_CODE}
		SET_COMM_FLAGS(State, *dest, width);
		
		STRING_FOOTER();
	}
	
	return 0;
}

DEF_OPCODE_FCN(IN, SB)
{
	 int	ret;
	STRING_HEAD(0, 1, 0, "", " DX", 1);
	
	uint8_t	tmp;
	
	ret = inB(State, State->DX.W, &tmp);
	if(ret)	return ret;
	
	ret = RME_Int_Write8(State, State->ES, destOfs, tmp);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(IN, SW)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(0, 1, 0, "", " DX", 4);
		uint32_t	tmp;
		ret = inD(State, State->DX.W, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write32(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(0, 1, 0, "", " DX", 2);
		uint16_t	tmp;
		ret = inW(State, State->DX.W, &tmp);
		if(ret)	return ret;
		ret = RME_Int_Write16(State, State->ES, destOfs, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	
	return 0;
}
DEF_OPCODE_FCN(OUT, SB)
{
	 int	ret;
	STRING_HEAD(0, 0, 1, " DX", "", 1);
	
	uint8_t	tmp;
	
	ret = RME_Int_Read8(State, State->DS, srcOfs, &tmp);
	if(ret)	return ret;
	
	ret = outB(State, State->DX.W, tmp);
	if(ret)	return ret;
	
	STRING_FOOTER();
	return 0;
}
DEF_OPCODE_FCN(OUT, SW)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		STRING_HEAD(0, 0, 1, " DX", "", 4);
		uint32_t	tmp;
		ret = RME_Int_Read32(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		ret = outD(State, State->DX.W, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	else
	{
		STRING_HEAD(0, 0, 1, " DX", "", 2);
		uint16_t	tmp;
		ret = RME_Int_Read16(State, State->ES, destOfs, &tmp);
		if(ret)	return ret;
		ret = outW(State, State->DX.W, tmp);
		if(ret)	return ret;
		STRING_FOOTER();
	}
	
	return 0;
}
