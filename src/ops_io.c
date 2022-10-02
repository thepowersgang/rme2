/*
 * Realmode Emulator
 * - IO Operations
 * 
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
#include "ops_io.h"

// === CODE ===
DEF_OPCODE_FCN(IN, ADx)
{
	RME_Int_DebugPrint(State, " DX AL");
	return inB(State, State->DX.W, &State->AX.B.L);
}

DEF_OPCODE_FCN(IN, ADxX)
{
	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_DebugPrint(State, " DX EAX");
		return inD(State, State->DX.W, &State->AX.D);
	}
	else
	{
		RME_Int_DebugPrint(State, "DX AX");
		return inW(State, State->DX.W, &State->AX.W);
	}
}

DEF_OPCODE_FCN(IN, AI)
{
	uint8_t	port;
	READ_INSTR8(port);
	RME_Int_DebugPrint(State, " 0x%02x AL", port);
	return inB(State, port, &State->AX.B.L);
}

DEF_OPCODE_FCN(IN, AIX)
{
	uint8_t	port;
	READ_INSTR8(port);
	RME_Int_DebugPrint(State, " 0x%02x", port);
	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_DebugPrint(State, " EAX");
		return inD(State, port, &State->AX.D);
	}
	else
	{
		RME_Int_DebugPrint(State, " AX");
		return inW(State, port, &State->AX.W);
	}
}

DEF_OPCODE_FCN(OUT, DxA)
{
	RME_Int_DebugPrint(State, " AL DX");
	return outB(State, State->DX.W, State->AX.B.L);
}
DEF_OPCODE_FCN(OUT, DxAX)
{
	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_DebugPrint(State, " EAX DX");
		return outD(State, State->DX.W, State->AX.D);
	}
	else
	{
		RME_Int_DebugPrint(State, " AX DX");
		return outW(State, State->DX.W, State->AX.W);
	}
}

DEF_OPCODE_FCN(OUT, AI)
{
	uint8_t	port;
	READ_INSTR8(port);
	RME_Int_DebugPrint(State, " AL 0x%02x", port);
	return outB(State, port, State->AX.B.L);
}

DEF_OPCODE_FCN(OUT, AIX)
{
	uint8_t	port;
	READ_INSTR8(port);
	if( State->Decoder.bOverrideOperand )
	{
		RME_Int_DebugPrint(State, " EAX 0x%02x", port);
		return outD(State, port, State->AX.D);
	}
	else
	{
		RME_Int_DebugPrint(State, " AX 0x%02x", port);
		return outW(State, port, State->AX.W);
	}
}
