/*
 * Realmode Emulator
 * - Stack (Push/Pop) Operations
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
// NOTE: Only does 16-bit pushes

// === CODE ===
DEF_OPCODE_FCN(PUSH, Seg)
{
	 int	ret;
	uint16_t	*ptr = Seg(State, Param);
	PUSH(*ptr);
	return 0;
}

DEF_OPCODE_FCN(POP, Seg)
{
	uint16_t	*ptr = Seg(State, Param);
	 int	ret;
	POP(*ptr);
	return 0;
}

DEF_OPCODE_FCN(PUSH, Reg)
{
	 int	ret;
	uint16_t	*ptr = RegW(State, Param);
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	PUSH(*ptr);
	return 0;
}

DEF_OPCODE_FCN(POP, Reg)
{
	 int	ret;
	uint16_t	*ptr = RegW(State, Param);
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	POP(*ptr);
	return 0;
}

DEF_OPCODE_FCN(PUSH, A)
{
	 int	ret;
	uint16_t	pt2;
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	pt2 = State->SP.W;
	PUSH(State->AX.W);	PUSH(State->CX.W);
	PUSH(State->DX.W);	PUSH(State->BX.W);
	PUSH(pt2);        	PUSH(State->BP.W);
	PUSH(State->SI.W);	PUSH(State->DI.W);
	return 0;
}

DEF_OPCODE_FCN(POP, A)
{
	 int	ret;
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	POP(State->DI.W);	POP(State->SI.W);
	POP(State->BP.W);	State->SP.W += 2;
	POP(State->BX.W);	POP(State->DX.W);
	POP(State->CX.W);	POP(State->AX.W);
	return 0;
}

DEF_OPCODE_FCN(PUSH, F)
{
	 int	ret;
	PUSH(State->Flags);
	return 0;
}

DEF_OPCODE_FCN(POP, F)
{
	 int	ret;
	uint16_t	tmp;
	const uint16_t	keep_mask = 0x7002;
	const uint16_t	set_mask  = 0x0FD5;
	POP(tmp);
	State->Flags &= keep_mask;
	tmp &= set_mask;
	State->Flags |= tmp | 2;	// Bit 1 must always be set
	return 0;
}

DEF_OPCODE_FCN(PUSH, MX)
{
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	return RME_ERR_UNDEFOPCODE;
}
DEF_OPCODE_FCN(POP, MX)
{
	 int	ret;
	uint16_t	*ptr;
	if( State->Decoder.bOverrideOperand )	return RME_ERR_UNDEFOPCODE;
	
	ret = RME_Int_ParseModRMX(State, NULL, &ptr, 0);
	if(ret)	return ret;
	POP(*ptr);
	return 0;
}

DEF_OPCODE_FCN(PUSH, I)
{
	 int	ret;
	uint16_t	val;
	READ_INSTR16( val );
	DEBUG_S(" 0x%04x", val);
	PUSH(val);
	return 0;
}

DEF_OPCODE_FCN(PUSH, I8)
{
	 int	ret;
	uint8_t	val;
	READ_INSTR16( val );
	DEBUG_S(" 0x%02x", val);
	PUSH(val);
	return 0;
}
