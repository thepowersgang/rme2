/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 *
 * - Kernel Core
 */
#include "common.h"
#include "../src/rme.h"

typedef struct {
	uint16_t	Offset;
	uint16_t	Segment;
}	t_farptr;

// === CODE ===
int main()
{
	tRME_State	*emu;
	void	*lowCache;
	void	*zeroptr = (void*)0;
	 int	i;

	Heap_Init();

	printf("Kernel up and running\n");

	lowCache = malloc( RME_BLOCK_SIZE );
	memcpy(lowCache, zeroptr, RME_BLOCK_SIZE+1);

	emu = RME_CreateState();

	emu->Memory[0] = lowCache;	// The RME has NULL checks
	for( i = RME_BLOCK_SIZE; i < 0x100000; i += RME_BLOCK_SIZE )
		emu->Memory[i/RME_BLOCK_SIZE] = (void*)i;


	#if 0
	emu->AX = (0x00<<8) | 0x11;	// Set Mode 0x11
	i = RME_CallInt(emu, 0x10);
	#endif

	// VESA
	#if 1
	{
		struct VesaControllerInfo {
			char	Signature[4];	// == "VBE2"
			uint16_t	Version;	// == 0x0300 for Vesa 3.0
			t_farptr	OemString;	// isa vbeFarPtr
			uint8_t	Capabilities[4];
			t_farptr	Videomodes;	// isa vbeParPtr
			uint16_t	TotalMemory;// as # of 64KB blocks
		}	*info = (void*)0x10000;
		memcpy(info->Signature, "VBE2", 4);
		emu->AX = 0x4F00;
		emu->ES = 0x1000;
		emu->DI = 0;
		i = RME_CallInt(emu, 0x10);
	}
	#endif

	#if 0
	emu->AX = (0x0B<<8) | 0x00;	// Set Border Colour
	emu->BX = (0x00<<0) | 0x02;	// Colour 1
	i = RME_CallInt(emu, 0x10);
	#endif

	#if 0
	emu->AX = (0x0F<<8) | 0;	// Function 0xF?
	i = RME_CallInt(emu, 0x10);
	#endif

	// Read Sector
	#if 0
	emu->AX = 0x0201;	// Function 2, 1 sector
	emu->CX = 1;	// Cylinder 0, Sector 1
	emu->DX = 0x10;	// Head 0, HDD 1
	emu->ES = 0x1000;	emu->BX = 0x0;
	i = RME_CallInt(emu, 0x13);
	printf("\n%02x %02x",
		*(uint8_t*)(0x10000+510),
		*(uint8_t*)(0x10000+511)
		);
	#endif

	//i = RME_CallInt(emu, 0x11);	// Equipment Test

	switch( i )
	{
	case RME_ERR_OK:
		printf("\n--- Emulator exited successfully!\n");
		printf("emu->AX = 0x%04x\n", emu->AX);
		break;
	case RME_ERR_INVAL:
		printf("\n--- ERROR: Invalid parameters\n");
		break;
	case RME_ERR_BADMEM:
		printf("\n--- ERROR: Emulator accessed bad memory\n");
		break;
	case RME_ERR_UNDEFOPCODE:
		printf("\n--- ERROR: Emulator hit an undefined opcode\n");
		break;
	case RME_ERR_DIVERR:
		printf("\n--- ERROR: Division Fault\n");
		break;
	default:
		printf("\n--- ERROR: Unknown error %i\n", i);
		break;
	}

	for(;;)
		__asm__ __volatile__ ("hlt");
}
