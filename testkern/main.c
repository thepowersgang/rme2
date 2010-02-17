/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 * 
 * - Kernel Core
 */
#include "common.h"
#include "../src/rme.h"

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
	emu->AX = (0x00<<8) | 0x04;	// Set Mode 0x04
	i = RME_CallInt(emu, 0x10);
	#endif
	
	#if 1
	emu->AX = (0x0B<<8) | 0x00;	// Set Border Colour
	emu->BX = (0x00<<0) | 0x02;	// Colour 1
	i = RME_CallInt(emu, 0x10);
	#endif
	
	#if 0
	emu->AX = (0x0F<<8) | 0;
	i = RME_CallInt(emu, 0x10);
	#endif
	
	#if 0
	emu->AX = (0x02 << 8) | 1;
	emu->CX = 1;	// Cylinder 0, Sector 1
	emu->DX = 0x0;	// Head 0, FDD 1
	emu->ES = 0x100;	emu->BX = 0x0;
	i = RME_CallInt(emu, 0x13);
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
