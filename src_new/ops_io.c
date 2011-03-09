/*
 * Realmode Emulator
 * - IO Operations
 * 
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"

#define	outB(state,port,val)	(outb(port,val),0)	// Write 1 byte to an IO Port
#define	outW(state,port,val)	(outw(port,val),0)	// Write 2 bytes to an IO Port
#define	outD(state,port,val)	(outl(port,val),0)	// Write 4 bytes to an IO Port
#define	inB(state,port,dst)	(*(dst)=inb((port)),0)	// Read 1 byte from an IO Port
#define	inW(state,port,dst)	(*(dst)=inw((port)),0)	// Read 2 bytes from an IO Port
#define	inD(state,port,dst)	(*(dst)=inl((port)),0)	// Read 4 bytes from an IO Port

// === CODE ===
DEF_OPCODE_FCN(IN, ADx)
{
	DEBUG_S("DX AL");
	return inB(State, State->DX.W, &State->AX.B.L);
}

DEF_OPCODE_FCN(IN, ADxX)
{
	if( State->Decoder.bOverrideAddress )
	{
		DEBUG_S("DX EAX");
		return inD(State, State->DX.W, &State->AX.D);
	}
	else
	{
		DEBUG_S("DX AX");
		return inW(State, State->DX.W, &State->AX.W);
	}
}

DEF_OPCODE_FCN(IN, AI)
{
	uint8_t	port;
	READ_INSTR8(port);
	DEBUG_S("0x%02 AL", port);
	return inB(State, port, &State->AX.B.L);
}

DEF_OPCODE_FCN(IN, AIX)
{
	uint8_t	port;
	READ_INSTR8(port);
	DEBUG_S("0x%02 ", port);
	if( State->Decoder.bOverrideAddress )
	{
		DEBUG_S("EAX");
		return inD(State, port, &State->AX.D);
	}
	else
	{
		DEBUG_S("AX");
		return inW(State, port, &State->AX.W);
	}
}

DEF_OPCODE_FCN(IN, SB);
DEF_OPCODE_FCN(IN, SW);

DEF_OPCODE_FCN(OUT, DxA)
{
	DEBUG_S("AL DX");
	return outB(State, State->DX.W, &State->AX.B.L);
}
DEF_OPCODE_FCN(OUT, DxAX)
{
	if( State->Decoder.bOverrideAddress )
	{
		DEBUG_S("EAX DX");
		return outD(State, State->DX.W, &State->AX.D);
	}
	else
	{
		DEBUG_S("AX DX");
		return outW(State, State->DX.W, &State->AX.W);
	}
}

DEF_OPCODE_FCN(OUT, AI)
{
	uint8_t	port;
	READ_INSTR8(port);
	DEBUG_S("AL 0x%02", port);
	return outB(State, port, &State->AX.B.L);
}

DEF_OPCODE_FCN(OUT, AIX)
{
	uint8_t	port;
	READ_INSTR8(port);
	if( State->Decoder.bOverrideAddress )
	{
		DEBUG_S("EAX 0x%02", port);
		return outD(State, port, &State->AX.D);
	}
	else
	{
		DEBUG_S("AX 0x%02", port);
		return outW(State, port, &State->AX.W);
	}
}

DEF_OPCODE_FCN(OUT, SB );
DEF_OPCODE_FCN(OUT, SW);
