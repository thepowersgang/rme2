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
	
	
	//emu->AX = 0x0A00 | 0x41;
	//emu->BX = 0x0A00 | 0x0F;
	//emu->CX = 1;
	//switch( RME_CallInt(emu, 0x10) )
	switch( RME_CallInt(emu, 0x12) )	// Get mem size
	{
	case RME_ERR_OK:	printf("\n--- Emulator exited successfully!\n");	break;
	case RME_ERR_BADMEM:
		printf("\n--- ERROR: Emulator accessed bad memory\n");
		break;
	case RME_ERR_UNDEFOPCODE:
		printf("\n--- ERROR: Emulator hit an undefined opcode\n");
		break;
	}
	printf("emu->AX = 0x%04x\n", emu->AX);
	
	for(;;)
		__asm__ __volatile__ ("hlt");
}
