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

/**
 * \brief Internals of LDS and LES 
 * \note Does ModR/M parsing for it self to avoid issues with alignment
 */
static inline int _LDS_LES_internal(tRME_State *State, uint16_t *SegRegPtr)
{
	 int	ret;
	uint16_t	seg, *src, *dest;
	uint32_t	addr;
	uint8_t	modrm_byte;
	
	READ_INSTR8(modrm_byte);
	
	if( (modrm_byte >> 6) == 3 )
		return RME_ERR_UNDEFOPCODE;	// Source cannot be a register
	
	ret = RME_Int_GetMMM(State, modrm_byte >> 6, modrm_byte & 7, &seg, &addr);
	if(ret)	return ret;
	
	// Get segment
	ret = RME_Int_GetPtr(State, seg, addr, (void**)&src);
	if(ret)	return ret;
	*SegRegPtr = *src;
	// Get address of the destination
	addr += 2;
	ret = RME_Int_GetPtr(State, seg, addr, (void**)&src);
	if(ret)	return ret;
	
	dest = RegW(State, (modrm_byte >> 3) & 7);
	
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
	uint8_t	modrm_byte;
	 int	modrm_mod, modrm_rrr, modrm_mmm;
	
	READ_INSTR8(modrm_byte);
	
	modrm_mod = modrm_byte >> 6;
	modrm_rrr = (modrm_byte >> 3) & 7;
	modrm_mmm = modrm_byte & 7;
	
	if( modrm_mod == 3 )
		return RME_ERR_UNDEFOPCODE;	// Source cannot be a register
	
	dest = RegW(State, modrm_rrr);
	
	ret = RME_Int_GetMMM(State, modrm_mod, modrm_mmm, &seg, &addr);
	if(ret)	return ret;
	
	if( State->Decoder.bOverrideOperand )
		*(uint32_t*)dest = addr;
	else
		*dest = addr;
	return 0;
}
