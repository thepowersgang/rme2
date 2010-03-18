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
	 int	ret;

	Heap_Init();

	printf("Kernel up and running\n");

	lowCache = malloc( RME_BLOCK_SIZE );
	memcpy(lowCache, zeroptr, RME_BLOCK_SIZE);

	emu = RME_CreateState();

	emu->Memory[0] = lowCache;	// The RME has NULL checks
	for( i = RME_BLOCK_SIZE; i < 0x100000; i += RME_BLOCK_SIZE )
		emu->Memory[i/RME_BLOCK_SIZE] = (void*)i;


	#if 1
	emu->AX.W = (0x00<<8) | 0x11;	// Set Mode 0x11
	ret = RME_CallInt(emu, 0x10);
	#endif

	// VESA
	#if 0
	{
		struct VesaControllerInfo {
			char	Signature[4];	// == "VBE2"
			uint16_t	Version;	// == 0x0300 for Vesa 3.0
			t_farptr	OemString;	// isa vbeFarPtr
			uint8_t	Capabilities[4];
			t_farptr	Videomodes;	// isa vbeParPtr
			uint16_t	TotalMemory;// as # of 64KB blocks
		}	*info = (void*)0x10000;
		
		struct {
			uint16_t	attributes;
			 uint8_t	winA, winB;
			uint16_t	granularity;
			uint16_t	winsize;
			uint16_t	segmentA, segmentB;
			t_farptr	realFctPtr;
			uint16_t	pitch; // bytes per scanline

			uint16_t	Xres, Yres;
			uint8_t	Wchar, Ychar, planes, bpp, banks;
			uint8_t	memory_model, bank_size, image_pages;
			uint8_t	reserved0;

			uint8_t	red_mask, red_position;
			uint8_t	green_mask, green_position;
			uint8_t	blue_mask, blue_position;
			uint8_t	rsv_mask, rsv_position;
			uint8_t	directcolor_attributes;

			uint32_t	physbase;  // your LFB address ;)
			uint32_t	reserved1;
			uint16_t	reserved2;
		}	*modeinfo = (void*)0x9000;
		
		uint16_t	*modes;
		
		memcpy(info->Signature, "VBE2", 4);
		emu->AX.W = 0x4F00;
		emu->ES = 0x1000;
		emu->DI.W = 0;
		ret = RME_CallInt(emu, 0x10);
		printf("emu->AX = 0x%04x\n", emu->AX.W);
		printf("info->Videomodes = {Segment:0x%04x,Offset:0x%04x}\n",
			info->Videomodes.Segment, info->Videomodes.Offset);
		modes = (void*)( (info->Videomodes.Segment*16) + info->Videomodes.Offset );
		for(i = 1; modes[i] != 0xFFFF; i++ )
		{
			emu->AX.W = 0x4F01;
			emu->CX.W = modes[i];
			emu->ES = 0x0900;
			emu->DI.W = 0x0000;
			RME_CallInt(emu, 0x10);
			printf("modes[%i] = 0x%04x\n", i, modes[i]);
			printf("modeinfo = {\n");
			printf("  .attributes = 0x%04x\n", modeinfo->attributes);
			printf("  .pitch = 0x%04x\n", modeinfo->pitch);
			printf("  .Xres = %i\n", modeinfo->Xres);
			printf("  .Yres = %i\n", modeinfo->Yres);
			printf("  .bpp = %i\n", modeinfo->bpp);
			printf("  .physbase = 0x%08x\n", modeinfo->physbase);
			printf("}\n");
			
			/*
			printf("  .width = %i\n", modes[i].width);
			printf("  .height = %i\n", modes[i].height);
			printf("  .pitch = 0x%04x\n", modes[i].pitch);
			printf("  .bpp = %i\n", modes[i].bpp);
			printf("  .flags = 0x%04x\n", modes[i].flags);
			printf("  .fbSize = 0x%04x\n", modes[i].fbSize);
			printf("  .framebuffer = 0x%08x\n", modes[i].framebuffer);
			*/
			//break;
		}
		
		emu->AX.W = 0x4F02;
		emu->BX.W = 0x0115;	// Qemu 800x600x24
		RME_CallInt(emu, 0x10);
	}
	#endif

	#if 0
	emu->AX.W = (0x0B<<8) | 0x00;	// Set Border Colour
	emu->BX.W = (0x00<<0) | 0x02;	// Colour 1
	ret = RME_CallInt(emu, 0x10);
	#endif

	#if 0
	emu->AX.W = (0x0F<<8) | 0;	// Function 0xF?
	ret = RME_CallInt(emu, 0x10);
	#endif

	// Read Sector
	#if 0
	emu->AX.W = 0x0201;	// Function 2, 1 sector
	emu->CX.W = 1;	// Cylinder 0, Sector 1
	emu->DX.W = 0x10;	// Head 0, HDD 1
	emu->ES = 0x1000;
	emu->BX.W = 0x0;
	ret = RME_CallInt(emu, 0x13);
	printf("\n%02x %02x",
		*(uint8_t*)(0x10000+510),
		*(uint8_t*)(0x10000+511)
		);
	#endif

	//ret = RME_CallInt(emu, 0x11);	// Equipment Test

	switch( ret )
	{
	case RME_ERR_OK:
		printf("\n--- Emulator exited successfully!\n");
		printf("emu->AX = 0x%04x\n", emu->AX.W);
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
		printf("\n--- ERROR: Unknown error %i\n", ret);
		break;
	}

	for(;;)
		__asm__ __volatile__ ("hlt");
}
