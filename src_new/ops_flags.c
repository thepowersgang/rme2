/*
 * Realmode Emulator
 * - Flag manipulation
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"

// === CODE ===
DEF_OPCODE_FCN(Flag, CLC)
{
	State->Flags &= ~FLAG_CF;
	return 0;
}
DEF_OPCODE_FCN(Flag, STC)
{
	State->Flags |= FLAG_CF;
	return 0;
}
DEF_OPCODE_FCN(Flag, CLI)
{
	State->Flags &= ~FLAG_IF;
	return 0;
}
DEF_OPCODE_FCN(Flag, STI)
{
	State->Flags |= FLAG_IF;
	return 0;
}
DEF_OPCODE_FCN(Flag, CLD)
{
	State->Flags &= ~FLAG_DF;
	return 0;
}
DEF_OPCODE_FCN(Flag, STD)
{
	State->Flags |= FLAG_DF;
	return 0;
}
