/*
 * Realmode Emulator
 * - MOV Family of operations (includes XCHG)
 * 
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"

#define XCHG(a,b)	do{uint32_t t=(a);(a)=(b);(b)=(t);}while(0)

// === CODE ===
// r/m = r
DEF_OPCODE_FCN(MOV, MR)
{
	 int	ret;
	struct ValueRef dest, src;
	TRY(ret, RME_Int_ParseModRMRev(State, &src, &dest));
	
	uint8_t v;
	TRY(ret, RME_Int_ReadV8(State, &src, &v));
	TRY(ret, RME_Int_WriteV8(State, &dest, v));
	
	return 0;
}
DEF_OPCODE_FCN(MOV, MRX)
{
	 int	ret;
	struct ValueRefX dest, src;
	TRY(ret, RME_Int_ParseModRMXRev(State, &src, &dest));
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t v;
		TRY(ret, RME_Int_ReadV32(State, &src, &v));
		TRY(ret, RME_Int_WriteV32(State, &dest, v));
	}
	else
	{
		uint16_t v;
		TRY(ret, RME_Int_ReadV16(State, &src, &v));
		TRY(ret, RME_Int_WriteV16(State, &dest, v));
	}
	
	return 0;
}

// r = r/m
DEF_OPCODE_FCN(MOV, RM)
{
	 int	ret;
	struct ValueRef dest, src;
	TRY(ret, RME_Int_ParseModRM(State, &dest, &src));
	
	uint8_t v;
	TRY(ret, RME_Int_ReadV8(State, &src, &v));
	TRY(ret, RME_Int_WriteV8(State, &dest, v));
	
	return 0;
}
DEF_OPCODE_FCN(MOV, RMX)
{
	 int	ret;
	struct ValueRefX dest, src;
	TRY(ret, RME_Int_ParseModRMX(State, &dest, &src));
	
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t v;
		TRY(ret, RME_Int_ReadV32(State, &src, &v));
		TRY(ret, RME_Int_WriteV32(State, &dest, v));
	}
	else
	{
		uint16_t v;
		TRY(ret, RME_Int_ReadV16(State, &src, &v));
		TRY(ret, RME_Int_WriteV16(State, &dest, v));
	}
	
	return 0;
}
// r/m = I
DEF_OPCODE_FCN(MOV, MI )
{
	 int	ret;
	struct ValueRef	dest;
	TRY(ret, RME_Int_ParseModRMRev(State, NULL, &dest));
	uint8_t	val;
	READ_INSTR8( val );
	RME_Int_DebugPrint(State, " 0x%02x", val);

	TRY(ret, RME_Int_WriteV8(State, &dest, val));
	
	return 0;
}
DEF_OPCODE_FCN(MOV, MIX)
{
	 int	ret;
	struct ValueRefX	dest;
	TRY(ret, RME_Int_ParseModRMXRev(State, NULL, &dest));
	
	if(State->Decoder.bOverrideOperand)
	{
		uint32_t	val;
		READ_INSTR32( val );
		RME_Int_DebugPrint(State, " 0x%08x", val);
		TRY(ret, RME_Int_WriteV32(State, &dest, val));
	}
	else
	{
		uint16_t	val;
		READ_INSTR16( val );
		RME_Int_DebugPrint(State, " 0x%04x", val);
		TRY(ret, RME_Int_WriteV16(State, &dest, val));
	}
	return 0;
}

// A := [imm16/32]
DEF_OPCODE_FCN(MOV, AMo)
{
	 int	ret;
	uint16_t	seg;
	uint32_t	ofs;
	
	RME_Int_DebugPrint(State, " AL");
	
	seg = *GET_SEGMENT(State, SREG_DS);
	if( State->Decoder.bOverrideAddress ) {
		READ_INSTR32( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	else {
		READ_INSTR16( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	
	ret = RME_Int_Read8(State, seg, ofs, &State->AX.B.L);
	if(ret)	return ret;
	
	return 0;
}
DEF_OPCODE_FCN(MOV, AMoX)
{
	 int	ret;
	uint32_t	ofs;
	
	RME_Int_DebugPrint(State, " %s", (State->Decoder.bOverrideOperand?"EAX":"AX"));
	
	uint16_t seg = *GET_SEGMENT(State, SREG_DS);
	if( State->Decoder.bOverrideAddress ) {
		READ_INSTR32( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	else {
		READ_INSTR16( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	
	if( State->Decoder.bOverrideOperand )
		ret = RME_Int_Read32(State, seg, ofs, &State->AX.D);
	else
		ret = RME_Int_Read16(State, seg, ofs, &State->AX.W);
		
	if(ret)	return ret;
	
	return 0;
}

// [imm16/32] := A
DEF_OPCODE_FCN(MOV, MoA)
{
	 int	ret;
	uint16_t	seg;
	uint32_t	ofs;
	
	seg = *GET_SEGMENT(State, SREG_DS);
	if( State->Decoder.bOverrideAddress ) {
		READ_INSTR32( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	else {
		READ_INSTR16( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	RME_Int_DebugPrint(State, " AL");
	ret = RME_Int_Write8(State, seg, ofs, State->AX.B.L);
	if(ret)	return ret;
	
	return 0;
}
DEF_OPCODE_FCN(MOV, MoAX)
{
	 int	ret;
	uint16_t	seg;
	uint32_t	ofs;
	
	seg = *GET_SEGMENT(State, SREG_DS);
	if( State->Decoder.bOverrideAddress ) {
		READ_INSTR32( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	else {
		READ_INSTR16( ofs );
		RME_Int_DebugPrint(State, ":[0x%x]", ofs);
	}
	
	RME_Int_DebugPrint(State, " %s", (State->Decoder.bOverrideOperand?"EAX":"AX"));
	
	if( State->Decoder.bOverrideOperand )
		ret = RME_Int_Write32(State, seg, ofs, State->AX.D);
	else
		ret = RME_Int_Write16(State, seg, ofs, State->AX.W);
		
	if(ret)	return ret;
	
	return 0;
}

// r/m = sr
DEF_OPCODE_FCN(MOV, RS)
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
		return RME_ERR_UNDEFOPCODE;
	
	struct ModRM mod_rm;
	TRY(ret, RME_Int_GetModRM(State, &mod_rm));
	
	struct ValueRefX	dest;
	TRY(ret, RME_Int_DecodeModMX(State, &mod_rm, &dest));
	
	uint16_t v = *Seg(State, mod_rm.rrr);
	TRY(ret, RME_Int_WriteV16(State, &dest, v));
	
	return 0;
}
// sr = r
DEF_OPCODE_FCN(MOV, SR )
{
	 int	ret;
	
	if( State->Decoder.bOverrideOperand )
		return RME_ERR_UNDEFOPCODE;

	struct ModRM mod_rm;
	TRY(ret, RME_Int_GetModRM(State, &mod_rm));

	// NOTE: `Seg` prints, so call first
	uint16_t* dest = Seg(State, mod_rm.rrr);
	struct ValueRefX	src;
	TRY(ret, RME_Int_DecodeModMX(State, &mod_rm, &src));

	TRY(ret, RME_Int_ReadV16(State, &src, dest));
	
	return 0;
}
// r = I
DEF_OPCODE_FCN(MOV, RegB)
{
	uint8_t	val, *dest;
	READ_INSTR8(val);
	dest = RegB(State, Param);
	RME_Int_DebugPrint(State, " 0x%02x", val);
	*dest = val;
	return 0;
}
DEF_OPCODE_FCN(MOV, Reg)
{
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	val, *dest;
		READ_INSTR32(val);
		dest = (void*)RegW(State, Param);
		RME_Int_DebugPrint(State, " 0x%08x", val);
		*dest = val;
	}
	else
	{
		uint16_t	val, *dest;
		READ_INSTR16(val);
		dest = RegW(State, Param);
		RME_Int_DebugPrint(State, " 0x%04x", val);
		*dest = val;
	}
	return 0;
}

// Move and Zero Extend
DEF_OPCODE_FCN(MOV,Z)
{
	 int	ret;
	struct ValueRef	src;
	struct ModRM	modrm;
	
	TRY(ret, RME_Int_GetModRM(State, &modrm));
	
	void* dest = RegW(State, modrm.rrr);
	TRY(ret, RME_Int_DecodeModM(State, &modrm, &src));
	uint8_t	v;
	TRY(ret, RME_Int_ReadV8(State, &src, &v));
	
	if( State->Decoder.bOverrideOperand ) {
		*(uint32_t*)dest = v;
	}
	else {
		*(uint16_t*)dest = v;
	}
	
	return 0;
}
DEF_OPCODE_FCN(MOV,ZX)
{
	 int	ret;
	
	struct ValueRefX	src;
	struct ModRM	modrm;
	
	TRY(ret, RME_Int_GetModRM(State, &modrm));
	
	void* dest = RegW(State, modrm.rrr);
	TRY(ret, RME_Int_DecodeModMX(State, &modrm, &src));
	uint16_t	v;
	TRY(ret, RME_Int_ReadV16(State, &src, &v));
	
	if( State->Decoder.bOverrideOperand ) {
		*(uint32_t*)dest = v;
	}
	else {
		*(uint16_t*)dest = v;
	}
	
	return 0;
}

// Exchange Family
DEF_OPCODE_FCN(XCHG, RM)
{
	 int	ret;
	struct ValueRef dst, src;
	TRY(ret, RME_Int_ParseModRM(State, &dst, &src));
	uint8_t dst_v, src_v;
	TRY(ret, RME_Int_ReadV8(State, &dst, &dst_v));
	TRY(ret, RME_Int_ReadV8(State, &src, &src_v));
	TRY(ret, RME_Int_WriteV8(State, &dst, src_v));
	TRY(ret, RME_Int_WriteV8(State, &src, dst_v));
	return 0;
}
DEF_OPCODE_FCN(XCHG, RMX)
{
	 int	ret;
	struct ValueRefX dst, src;
	TRY(ret, RME_Int_ParseModRMX(State, &dst, &src));
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t dst_v, src_v;
		TRY(ret, RME_Int_ReadV32(State, &dst, &dst_v));
		TRY(ret, RME_Int_ReadV32(State, &src, &src_v));
		TRY(ret, RME_Int_WriteV32(State, &dst, src_v));
		TRY(ret, RME_Int_WriteV32(State, &src, dst_v));
	}
	else
	{
		uint16_t dst_v, src_v;
		TRY(ret, RME_Int_ReadV16(State, &dst, &dst_v));
		TRY(ret, RME_Int_ReadV16(State, &src, &src_v));
		TRY(ret, RME_Int_WriteV16(State, &dst, src_v));
		TRY(ret, RME_Int_WriteV16(State, &src, dst_v));
	}
	return 0;
}
DEF_OPCODE_FCN(XCHG, Reg)	// A with Reg
{
	union {
		uint16_t	*W;
		uint32_t	*D;
	}	src;
	if( Param == 0 ) {
		RME_Int_DebugPrint(State, " - NOP");
		return 0;
	}
	
	RME_Int_DebugPrint(State, " %s", (State->Decoder.bOverrideOperand?"EAX":"AX"));
	
	src.W = RegW(State, Param);
	if(State->Decoder.bOverrideOperand)
		XCHG(State->AX.D, *src.D);
	else
		XCHG(State->AX.W, *src.W);
	return 0;
}
