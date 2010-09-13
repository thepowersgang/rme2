/*
 * Realmode Emulator - Native Tester
 */
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <rme.h>
#include "dosexe.h"
#include <SDL/SDL.h>

#define PACKED	__attribute__((packed))

typedef struct {
	uint16_t	Offset;
	uint16_t	Segment;
} PACKED	t_farptr;

//#define COL_WHITE	0x80808080
#define COL_WHITE	0x00FFFFFF

// === IMPORTS ===
extern void	LoadDosExe(const char *file);

// === PROTOTYPES ===
 int	HLECall10(tRME_State *State, int IntNum);
 int	HLECall(tRME_State *State, int IntNum);
void	PutChar(uint8_t ch, uint32_t FGC);

// === GLOBALS ===
SDL_Surface	*gScreen;
char	*gasFDDs[4] = {"fdd.img", NULL, NULL, NULL};
FILE	*gaFDDs[4];
uint8_t	gaMemory[0x110000];

// === CODE ===
int main(int argc, char *argv[])
{
	tRME_State	*emu;
	void	*data;
	FILE	*fp;
	 int	ret, i;
	 int	len, tmp;

	// TODO: Better parameter interpretation
	for( i = 1; i < argc; i ++ ) {
		gasFDDs[0] = argv[i];
	}

	signal(SIGINT, exit);
	
	gScreen = SDL_SetVideoMode(80*8, 25*16, 32, SDL_HWSURFACE);
	PutChar('R', COL_WHITE);
	PutChar('M', COL_WHITE);
	PutChar('E', COL_WHITE);
	PutChar('\n', COL_WHITE);

	// Open FDD image
	printf("Loading '%s'\n", gasFDDs[0]);
	gaFDDs[0] = fopen(gasFDDs[0], "rb");
	if( !gaFDDs[0] ) {
		perror("Unable to open file");
		return 1;
	}

	// Create BIOS Structures
	{
		struct {
			uint16_t	Length;
			uint16_t	Flags;
			uint32_t	NumCyl;
			uint32_t	NumHeads;
			uint32_t	NumSector;
			uint64_t	TotalSectors;
			uint16_t	BytesPerSector;
		}	__attribute__((packed))	*DiskInfo;
	
		DiskInfo = (void*)&gaMemory[0xF0000];
		DiskInfo[0].Length = 0x1A;
		DiskInfo[0].Flags = 1;	// CHS Info is valid
		DiskInfo[1].NumCyl = 80;
		DiskInfo[1].NumHeads = 2;
		DiskInfo[1].NumSector = 18;
		DiskInfo[0].TotalSectors = 1440*2;
		DiskInfo[0].BytesPerSector = 512;
	}

	// Create and initialise RME State
	emu = RME_CreateState();

	emu->HLECallbacks[0x10] = HLECall10;
	emu->HLECallbacks[0x13] = HLECall;
	emu->HLECallbacks[0x16] = HLECall;
	for( i = 0; i < 0x110000; i += RME_BLOCK_SIZE )
		emu->Memory[i/RME_BLOCK_SIZE] = &gaMemory[i];

	// Read boot sector
	if( fread( &gaMemory[0x7C00], 512, 1, gaFDDs[0] ) != 1 )
	{
		fprintf(stderr, "Disk image < 512 bytes\n");
		return 0;
	}
	if( *(uint16_t*)&gaMemory[0x7DFE] != 0xAA55 )
	{
		fprintf(stderr, "Invalid boot signature on boot sector\n");
		return 0;
	}

	emu->CS = 0x07C0;
	emu->IP = 0x0000;
	emu->DX.W = 0x0000;	// Disk

	ret = RME_Call(emu);

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

	return 0;
}

#include "font.h"
/**
 * \brief Screen output
 */
void PutChar(uint8_t ch, uint32_t FGC)
{
	Uint8	*font;
	Uint32	*buf;
	 int	x, y;
	static int curX=0, curY=0;
	SDL_Rect	rc = {0,0,1,1};
	
	switch( ch )
	{
	case '\n':
		curY ++;
		return ;
	case '\r':
		curX = 0;
		return ;
	}
	
	font = &VTermFont[ch*FONT_HEIGHT];
	
	rc.w = 1;
	rc.h = 1;
	rc.x = curX*FONT_WIDTH;
	rc.y = curY*FONT_HEIGHT;
	
	for(y = 0; y < FONT_HEIGHT; y ++, rc.y ++)
	{
		for(x = 0; x < FONT_WIDTH; x ++, rc.x++)
		{
			if(*font & (1 << (FONT_WIDTH-x-1)))
				SDL_FillRect(gScreen, &rc, FGC);
		}
		rc.x -= FONT_WIDTH;
		buf = (void*)( (intptr_t)buf + gScreen->pitch );
		font ++;
	}
	
	curX ++;
	if(curX == 80) {
		curX = 0;
		curY ++;
		if(curY == 25)
			curY = 0;
	}
	
	SDL_Flip(gScreen);
}

/**
 */
int GetDiskParams(int Disk, int *NCyl, int *NHead, int *SPT)
{
	switch(Disk)
	{
	case 0:
		*NCyl = 80;
		*NHead = 2;
		*SPT = 18;
		return 4;	// Type = 4 (3.5" 1.44MB)
	default:
		return 0;
	}
}

int ReadDiskLBA(int Disk, int LBAAddr, int Count, void *Data)
{
	printf("ReadDiskLBA: (Disk=%i, LBAAddr=0x%x, Count=%i, Data=%p)\n",
		Disk, LBAAddr, Count, Data);
	switch(Disk)
	{
	case 0:
		if( fseek(gaFDDs[0], LBAAddr * 512, SEEK_SET) )
			perror("fseek failed");
//		printf("ftell() = 0x%x\n", ftell(gaFDDs[0]));
		return fread(Data, 512, Count, gaFDDs[0]);
	default:
		return 0;
	}
}

int ReadDiskCHS(int Disk, int Cylinder, int Head, int Sector, int Count, void *DataPtr)
{
	 int	lbaAddr;
	 int	nCyl, nHead, spt;
	
	printf("ReadDiskCHS: (Disk=%i, Cylinder=%i, Head=%i, Sector=%i, Count %i, Data=%p)\n",
		Disk, Cylinder, Head, Sector, Count, DataPtr);
	
	if( GetDiskParams(Disk, &nCyl, &nHead, &spt) == 0 )	return -1;
	
	printf(" nCyl=%i, nHead=%i, spt=%i\n", nCyl, nHead, spt);
	
	if( Cylinder >= nCyl ) {
		printf(" Cylinder(%i) >= nCyl(%i)\n", Cylinder, nCyl);
		return -0x01;
	}
	if( Head >= nHead ) {
		printf(" Head(%i) >= nHead(%i)\n", Head, nHead);
		return -0x01;
	}
	if( Sector > spt ) {
		printf(" Sector(%i) >= spt(%i)\n", Sector, spt);
		return -0x01;
	}
	// Multi-track reads allowed (because they are easy)
	
	lbaAddr = Cylinder * nHead * spt + Head * spt + Sector - 1;
	
	return ReadDiskLBA(Disk, lbaAddr, Count, DataPtr);
}

/**
 * \brief Do a HLE call (VGA BIOS)
 */
int HLECall10(tRME_State *State, int IntNum)
{
	 int	ret;
	switch(State->AX.B.H)
	{
	case 0x0E:
		PutChar(State->AX.B.L, 0xFFFFFFFF);
		break;
	default:
		printf("HLE Call INT 0x10\n");
		RME_DumpRegs(State);
		exit(1);
	}
	return 0;	// Silently ignore VGA BIOS calls
}

/**
 * \brief Do a HLE call
 */
int HLECall(tRME_State *State, int IntNum)
{
	 int	ret;
	switch( IntNum )
	{	
	case 0x13:
		switch(State->AX.B.H)
		{
		case 0x00:	// Reset Disk Subsystem
			// Does anything need to be done here?
			break;
		
		case 0x02:	// Read Sector(s) into memory
			RME_DumpRegs(State);
			// AL - Number of sectors to read
			// CH - Cylinder Number Low Bits
			// CL - Sector Number (bits 0-5), Cylinder Number High (bits 6,7)
			// DH - Head Number
			// DL - Disk Number
			// ES:BX - Destination Buffer
			
			// Zero count?
			if( (State->CX.B.L & 0x3F) == 0 ) {
				printf(" 0x13:0x02 Zero sectors\n");
				State->Flags |= 1;
				break;
			}
			
			ret = ReadDiskCHS(
				State->DX.B.L,	// Disk
				State->CX.B.H | ((State->CX.B.L & 0xC0)<<2),	// Cylinder
				State->DX.B.H,	// Head
				State->CX.B.L & 0x3F,	// Sector
				State->AX.B.L,	// Count
				&gaMemory[ State->ES*16 + State->BX.W ]
				);
			// Error check
			if( ret < 0 ) {
				printf(" 0x13:0x02 Error: 0x%x\n", -ret);
				State->AX.B.H = -ret;
				State->Flags |= 1;
				break;
			}
			if( ret != State->AX.B.L ) {
				printf(" 0x13:0x02 Incimplete read: %i/%i\n", ret, State->AX.B.L);
				State->AX.B.L = ret;
				State->Flags |= 1;
				break;
			}
			printf("Read 0x%x bytes to %04x:%04x ([0] = 0x%02x)\n",
				ret*512, State->ES, State->BX.W,
				gaMemory[ State->ES*16 + State->BX.W ]);
			State->AX.B.H = 0;
			State->Flags &= ~1;
			break;
		
		case 0x08:	// Get Drive Parameters
			{
				State->Flags &= ~1;
				 int	cyl, heads, sec;
				 int	type;
				type = GetDiskParams(State->DX.B.L, &cyl, &heads, &sec);
				if( type == 0 ) {
					State->Flags |= 1;
					break;
				}
				
				cyl --;	// Make `cyl` the max cylinder number
				heads --;	// Same for `heads`
				// Sector numbers are 1 based, so `sec` doesn't need to be changed
				
				// TODO: Use GetDiskParams here
				State->AX.W = 0x0000;	// AX - Zero for success
				State->BX.B.L = type;	// BL - Disk Type (1.44M Floppy)
				State->CX.B.L = sec | ((cyl>>8)<<6);	// CL - Max Sector Number
				State->CX.B.H = cyl&0xFF;	// CH - Cylinder Count (Bits 0-7)
				State->DX.B.L = 1;	// DL - Number of drives
				State->DX.B.H = heads;	// DH - Maximum Head Number
				// Disk Parameter block
				State->ES = 0xF000;	State->DI.W = 0x0000;
			}
			break;
		
		// Extended Read
		case 0x42:
			{
				uint32_t	laddr;
				struct {
					uint8_t	Size;
					uint8_t	Rsvd;	// Zero
					uint16_t	Count;
					t_farptr	Buffer;
					uint64_t	LBAStart;
					uint64_t	BufferLong;
				}	PACKED	*packet = (void*)&gaMemory[State->DS*16+State->SI.W];
				
				laddr = packet->Buffer.Segment*16 + packet->Buffer.Offset;
				
				
				printf(" packet = {Size:%i,Count=%i,Buffer=%04x:%04x,LBAStart=0x%lx,BufferLong=0x%lx}\n",
					packet->Size, packet->Count,
					packet->Buffer.Segment, packet->Buffer.Offset,
					packet->LBAStart, packet->BufferLong);
				
				if(laddr + packet->Count*512 > 0x110000) {
					State->Flags |= 1;
					State->AX.B.H = 0xBB;
					printf("Read past end of memory! (0x%x)\n",
						laddr + packet->Count*512);
					break;
				}
				ReadDiskLBA(State->DX.B.L,
					packet->LBAStart, packet->Count,
					&gaMemory[laddr]);
				State->Flags &= ~1;
				State->AX.B.H = 0x00;
				
			}
			break;
		
		default:
			printf("HLE Call INT 0x13\n");
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	
	// --- Keyboard Input ---
	case 0x16:
		switch(State->AX.B.H)
		{
		case 0x00:
			{
				SDL_Event	e;
				while( SDL_WaitEvent(&e) )
				{
					if(e.type == SDL_QUIT)
						exit(0);
					else if(e.type == SDL_KEYDOWN) {
						State->AX.B.L = e.key.keysym.sym;
						State->AX.B.H = e.key.keysym.sym;
						break;
					}
				}
			}
			break;
		default:
			printf("HLE Call INT 0x16\n");
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	
	default:
		printf("HLE Call INT 0x%02x Unknown\n", IntNum);
		RME_DumpRegs(State);
	}
	return 0;	// Emulate
}

