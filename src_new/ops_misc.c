/*
 * Realmode Emulator
 * - Misc operations
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"

// === CODE ===
DEF_OPCODE_FCN(CBW, z)	// Convert signed Byte to Word
{
	if( State->Decoder.bOverrideOperand )
		State->AX.D = (uint32_t)( (int8_t)State->AX.B.L );
	else
		State->AX.W = (uint16_t)( (int8_t)State->AX.B.L );
	return 0;
}

// 0xf4 - Halt execution
DEF_OPCODE_FCN(HLT, z)
{
	return RME_ERR_HALT;
}

// 0x37 - ASCII adjust AL after Addition
DEF_OPCODE_FCN(AAA, z)
{
	if( (State->AX.B.L & 0xF) > 9 || (State->Flags & FLAG_AF) )
	{
		State->AX.B.L += 6;
		State->AX.B.H += 1;
		State->Flags |= FLAG_AF|FLAG_CF;
		State->AX.B.L |= 0xF;
	}
	else
	{
		State->Flags &= ~(FLAG_AF|FLAG_CF);
		State->AX.B.L |= 0xF;
	}
	return 0;
}

// 0x3F - ASCII adjust AL after Subtraction
DEF_OPCODE_FCN(AAS, z)
{
	if( (State->AX.B.L & 0xF) > 9 || (State->Flags & FLAG_AF) )
	{
		State->AX.B.L -= 6;
		State->AX.B.H -= 1;
		State->Flags |= FLAG_AF|FLAG_CF;
		State->AX.B.L |= 0xF;
	}
	else
	{
		State->Flags &= ~(FLAG_AF|FLAG_CF);
		State->AX.B.L |= 0xF;
	}
	return 0;
}

// 0x27 - Decimal adjust AL after Addition
DEF_OPCODE_FCN(DAA, z)
{
	 int	bOldCF = !!(State->Flags & FLAG_CF);
	if( (State->AX.B.L & 0xF) > 9 || (State->Flags & FLAG_AF) )
	{
		if( State->AX.B.L >= 0x100-6 )
			State->Flags |= FLAG_CF;
		State->AX.B.L += 6;
		State->Flags |= FLAG_AF;
	}
	
	if( State->AX.B.L > 0x99 || bOldCF )
	{
		State->AX.B.L += 0x60;
		State->Flags |= FLAG_CF;
	}
	else
	{
		State->Flags &= ~FLAG_CF;
	}
	return 0;
}

// 0x2F - Decimal adjust AL after Subtraction
DEF_OPCODE_FCN(DAS, z)
{
	 int	bOldCF = !!(State->Flags & FLAG_CF);
	if( (State->AX.B.L & 0xF) > 9 || (State->Flags & FLAG_AF) )
	{
		if( State->AX.B.L < 6 )
			State->Flags |= FLAG_CF;
		State->AX.B.L -= 6;
		State->Flags |= FLAG_AF;
	}
	
	if( State->AX.B.L > 0x99 || bOldCF )
	{
		State->AX.B.L -= 0x60;
		State->Flags |= FLAG_CF;
	}
	return 0;
}

// 0x99 - Convert Word to Doubleword
DEF_OPCODE_FCN(CWD, z)
{
	if( State->Decoder.bOverrideOperand )
	{
		if( State->AX.D & 0x80000000 )
			State->DX.D = 0xFFFFFFFF;
		else
			State->DX.D = 0;
	}
	else
	{
		if( State->AX.W & 0x8000 )
			State->DX.W = 0xFFFF;
		else
			State->DX.W = 0;
	}
	return 0;
}

/**
 * \brief Internals of LDS and LES 
 * \note Does ModR/M parsing for it self to avoid issues with alignment
 */
static inline int _LDS_LES_internal(tRME_State *State, uint16_t *SegRegPtr)
{
	 int	ret;
	uint16_t	seg, *src, *dest;
	uint32_t	addr;
	 int	mod, rrr, mmm;
	
	RME_Int_GetModRM(State, &mod, &rrr, &mmm);
	
	if( mod == 3 )
		return RME_ERR_UNDEFOPCODE;	// Source cannot be a register
	
	ret = RME_Int_GetMMM(State, mod, mmm, &seg, &addr);
	if(ret)	return ret;
	dest = RegW(State, rrr);
	
	// Get segment
	ret = RME_Int_GetPtr(State, seg, addr, (void**)&src);
	if(ret)	return ret;
	*SegRegPtr = *src;
	
	// Get address of the destination
	addr += 2;
	ret = RME_Int_GetPtr(State, seg, addr, (void**)&src);
	if(ret)	return ret;
	
	if( State->Decoder.bOverrideOperand )
	{
		*(uint32_t*)dest = *(uint32_t*)src;
	}
	else
	{
		*dest = *src;
	}
	return 0;
}

DEF_OPCODE_FCN(LES, z)	// Load ES:r16/32 with m16:m16/32
{
	return _LDS_LES_internal(State, &State->ES);
}

DEF_OPCODE_FCN(LDS, z)	// Load DS:r16/32 with m16:m16/32
{
	return _LDS_LES_internal(State, &State->DS);
}

DEF_OPCODE_FCN(LEA, z)
{
	 int	ret;
	uint16_t	seg, *dest;
	uint32_t	addr;
	 int	mod, rrr, mmm;
	
	RME_Int_GetModRM(State, &mod, &rrr, &mmm);
	
	if( mod == 3 )
		return RME_ERR_UNDEFOPCODE;	// Source cannot be a register
	
	dest = RegW(State, rrr);
	
	ret = RME_Int_GetMMM(State, mod, mmm, &seg, &addr);
	if(ret)	return ret;
	
	if( State->Decoder.bOverrideOperand )
		*(uint32_t*)dest = addr;
	else
		*dest = addr;
	return 0;
}
