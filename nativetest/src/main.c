/*
 * Realmode Emulator - Native Tester
 */
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <rme.h>
#include <SDL/SDL.h>

//#define COL_WHITE	0x80808080
#define COL_BLACK	0x00000000
#define COL_RED 	0x00FF0000
#define COL_GREEN	0x0000FF00
#define COL_BLUE	0x000000FF
#define COL_WHITE	0x00FFFFFF

// === IMPORTS ===
t_farptr	LoadDosExe(tRME_State *state, const char *file, t_farptr *stackptr);

// === PROTOTYPES ===
void	HandleEvent(SDL_Event *Event);
 int	HLECall10(tRME_State *State, int IntNum);
 int	HLECall12(tRME_State *State, int IntNum);
 int	HLECall(tRME_State *State, int IntNum);
void	PutChar(uint8_t ch, uint32_t BGC, uint32_t FGC);
void	PutString(const char *String, uint32_t BGC, uint32_t FGC);

// === GLOBALS ===
SDL_Surface	*gScreen;
char	*gasFDDs[4] = {"fdd.img", NULL, NULL, NULL};
FILE	*gaFDDs[4];
const char	*gsBinaryFile;
const char	*gsDosExe;
const char	*gsMemoryDumpFile;
 int	gbDisableGUI = 0;
uint8_t	gaMemory[0x110000];
// - GUI Key Queue
const int	cKeyBufferSize = 16;
 int	gKeyBufferPos = 0;
struct {
	uint8_t	Scancode;
	uint8_t	ASCII;
} gKeyBuffer[16];

// === CODE ===
int main(int argc, char *argv[])
{
	tRME_State	*emu;
	void	*data;
	FILE	*fp;
	 int	ret, i;
	 int	len, tmp;
	 int	nFDDs = 0;

	// TODO: Better parameter interpretation
	for( i = 1; i < argc; i ++ )
	{
		if( argv[i][0] == '-' && argv[i][1] == '-' )
		{
			if( strcmp(argv[i], "--nogui") == 0 ) {
				gbDisableGUI = 1;
			}
			else {
			}
		}
		else if( argv[i][0] == '-' )
		{
			switch( argv[i][1] )
			{
			case 'b':
				gsBinaryFile = argv[++i];
				break;
			case 'O':
				gsMemoryDumpFile = argv[++i];
				break;
			case 'd':
				gsDosExe = argv[++i];
				break;
			}
		}
		else
		{
			if(nFDDs < 4)
				gasFDDs[nFDDs++] = argv[i];
		}
	}

	signal(SIGINT, exit);

	if( !gbDisableGUI ) {
		gScreen = SDL_SetVideoMode(80*8, 25*16, 32, SDL_HWSURFACE);
		PutString("RME NativeTest\r\n", COL_BLACK, COL_WHITE);
	}

	// Open FDD image
	printf("Loading '%s'\n", gasFDDs[0]);
	gaFDDs[0] = fopen(gasFDDs[0], "rb");
	if( !gaFDDs[0] ) {
		perror("Unable to open file");
		return 1;
	}

	// Fill with a #UD
	memset(gaMemory, 0xF1, sizeof(gaMemory));	// 0xF1 = ICEBP/INT 1/#UD
	//memset(gaMemory, 0xCC, sizeof(gaMemory));	// 0xCC = INT 3

	// Create BIOS Structures
	// - BIOS Entrypoint (All BIOS calls are HLE, so trap them)
	gaMemory[0xF0000+i*2] = 0x67;	// XCHG (RMX)
	gaMemory[0xF0001+i*2] = 0311;	// r BX BX
	// - Interrupt Vector Table
	for( i = 0; i < 0x100; i ++ )
	{
		gaMemory[i*4+0] = i;
		gaMemory[i*4+1] = 0x00;
		gaMemory[i*4+2] = RME_HLE_CS&0xFF;
		gaMemory[i*4+3] = RME_HLE_CS>>8;
	}
	// - Disk Paramter Block
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
	
		DiskInfo = (void*)&gaMemory[0xF1000];
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
	emu->HLECallbacks[0x03] = HLECall;	// 0x03 - Debug
	emu->HLECallbacks[0x10] = HLECall10;	// 0x10 - VGA BIOS
	emu->HLECallbacks[0x12] = HLECall12;	// 0x12 - Get Memory Size
	emu->HLECallbacks[0x13] = HLECall;	// 0x13 - Disk IO
	emu->HLECallbacks[0x16] = HLECall;	// 0x16 - Keyboard Input
	emu->HLECallbacks[0x18] = HLECall;	// 0x18 - Diskless Boot Hook
	emu->HLECallbacks[0x19] = HLECall;	// 0x19 - System Bootstrap Loader
	for( i = 0; i < 0x110000; i += RME_BLOCK_SIZE )
		emu->Memory[i/RME_BLOCK_SIZE] = &gaMemory[i];

	if( gsDosExe )
	{
		printf("Loading DOS Exe \"%s\"\n", gsDosExe);
		t_farptr stack;
		t_farptr ep = LoadDosExe(emu, gsDosExe, &stack);
		if( ep.Segment == 0 && ep.Offset == 0 ) {
			// Load error
			return -1;
		}
		
		emu->CS = ep.Segment;
		emu->IP = ep.Offset;
		emu->SS = stack.Segment;
		emu->SP.W = stack.Offset;
	}
	else if( gsBinaryFile )
	{
		FILE	*fp = fopen(gsBinaryFile, "rb");
		 int	len;
		
		memset(gaMemory, 0, 0x400);
		printf("Booting '%s' at 0xF0000\n", gsBinaryFile);
		
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		
		if(len > 0x10000)
		{
			fprintf(stderr, "Binary file '%s' is too large, not loading\n", gsBinaryFile);
		}
		else
		{
			fread( &gaMemory[0xF0000], len, 1, fp );
		}
		fclose(fp);

		emu->CS = 0xF000;
		emu->IP = 0xFFF0;
	}
	else
	{
		printf("Booting Disk #0\n");
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
	}
	
	
//	emu->SS = 0xA000;
	emu->SP.W = 0xFFFE;
	*(uint16_t*)&gaMemory[0xA0000-2] = 0xFFFF;

	// Main emulation loop
	while( (ret = RME_RunOne(emu)) == RME_ERR_OK )
	{
		SDL_Event	ev;
		while( SDL_PollEvent(&ev) )
		{
			HandleEvent(&ev);
		}
	}

	// Write out memory
	if( gsMemoryDumpFile )
	{
		FILE *fp = fopen(gsMemoryDumpFile, "wb");
		fwrite(gaMemory, 1024*1024, 1, fp);
		fclose(fp);
		printf("\n--- Memory written to '%s'", gsMemoryDumpFile);
	}

	switch( ret )
	{
	case RME_ERR_OK:
		printf("\n--- Emulator exited successfully!\n");
		printf("emu->AX = 0x%04x\n", emu->AX.W);
		break;
	case RME_ERR_INVAL:
		printf("\n--- ERROR: Invalid parameters\n");
		return 1;
	case RME_ERR_BADMEM:
		printf("\n--- ERROR: Emulator accessed bad memory\n");
		return 1;
	case RME_ERR_UNDEFOPCODE:
		printf("\n--- ERROR: Emulator hit an undefined opcode\n");
		return 1;
	case RME_ERR_DIVERR:
		printf("\n--- ERROR: Division Fault\n");
		return 1;
	case RME_ERR_HALT:
		if( gbDisableGUI ) {
			printf("\n");
			return 0;
		}
		else
		{
			SDL_Event	e;
			SDL_WM_SetCaption("RME - CPU Halted, press any key to quit", "RME - Halted");
			while( SDL_WaitEvent(&e) )
			{
				if(e.type == SDL_QUIT)
					exit(0);
				else if(e.type == SDL_KEYDOWN)
					exit(0);
			}
			printf("\n--- STOP: CPU Halted\n");
		}
		break;
	default:
		printf("\n--- ERROR: Unknown error %i\n", ret);
		return 1;
	}

	return 0;
}

void HandleEvent(SDL_Event *Event)
{
	switch(Event->type)
	{
	case SDL_QUIT:
		fprintf(stderr, "Window closed, quitting\n");
		exit(0);
	case SDL_KEYDOWN:
		if( gKeyBufferPos == cKeyBufferSize )
		{
			// BEEP!
		}
		else
		{
			gKeyBuffer[gKeyBufferPos].Scancode = Event->key.keysym.sym;
//			gKeyBuffer[gKeyBufferPos].ASCII = Event->key.keysym.unicode;
			gKeyBuffer[gKeyBufferPos].ASCII = Event->key.keysym.sym;
			printf("%i: %x %x\n",
				gKeyBufferPos, gKeyBuffer[gKeyBufferPos].Scancode, gKeyBuffer[gKeyBufferPos].ASCII);
			gKeyBufferPos ++;
		}
		break;
	}
}

#include "font.h"
/**
 * \brief Screen output
 */
void PutChar(uint8_t ch, uint32_t BGC, uint32_t FGC)
{
	Uint8	*font;
	Uint32	*buf;
	 int	x, y;
	static int curX=0, curY=0;
	SDL_Rect	rc = {0,0,1,1};
	
	if( gbDisableGUI )
		return ;

	switch( ch )
	{
	case '\n':
		curY ++;
		// TODO: Scroll
		if(curY == 25)
			curY = 0;
		return ;
	case '\r':
		curX = 0;
		return ;
	case 8:
		if( curX > 0 )
			curX --;
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
			else
				SDL_FillRect(gScreen, &rc, BGC);
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

void PutString(const char *String, uint32_t BGC, uint32_t FGC)
{
	while(*String)
		PutChar(*String++, BGC, FGC);
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
	 int	ret;
	printf("ReadDiskLBA: (Disk=%i, LBAAddr=0x%x, Count=%i, Data=%p)\n",
		Disk, LBAAddr, Count, Data);
	switch(Disk)
	{
	case 0:
		if( fseek(gaFDDs[0], LBAAddr * 512, SEEK_SET) ) {
			fprintf(stderr, "fseek(gaFDDs[0], 0x%x*512, SEEK_SET)\n", LBAAddr);
			fprintf(stdout, "fseek(gaFDDs[0], 0x%x*512, SEEK_SET)\n", LBAAddr);
			perror("fseek failed");	
			memset(Data, 0, 512*Count);
			return 0;
		}
//		printf("ftell() = 0x%x\n", ftell(gaFDDs[0]));
		ret = fread(Data, 512, Count, gaFDDs[0]);
//		printf("%02x %02x %02x %02x %02x %02x %02x %02x",
//			((uint8_t*)Data)[0], ((uint8_t*)Data)[1], ((uint8_t*)Data)[2], ((uint8_t*)Data)[3],
//			((uint8_t*)Data)[4], ((uint8_t*)Data)[5], ((uint8_t*)Data)[6], ((uint8_t*)Data)[7]
//			);
		return ret;
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
	printf(" lbaAddr = %x\n", lbaAddr);
	
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
	// VIDEO - SET VIDEO MODE
	case 0x00:
		printf("HLE Call INT 0x10 BH=0x00 Unimpl\n");
		exit(1);
	// VIDEO - TELETYPE OUTPUT
	case 0x0E:
		// TODO: Better Colours
		PutChar(State->AX.B.L, COL_BLACK, COL_WHITE);
		break;
	// VIDEO - GET CURRENT VIDEO MODE
	case 0x0F:
		State->AX.B.H = 80;	// Cols
		State->AX.B.L = 0x03;	// Mode Number
		State->BX.B.L = 0;	// Page
		break;
	default:
		printf("HLE Call INT 0x10 BH=%02x Unk\n", State->AX.B.H);
		RME_DumpRegs(State);
		exit(1);
	}
	return 0;	// Silently ignore VGA BIOS calls
}


/**
 * \brief Do a HLE call (Get memory size)
 */
int HLECall12(tRME_State *State, int IntNum)
{
	State->AX.W = 0xA0000/1024 - 1;
	return 0;
}
/**
 * \brief Do a HLE call
 */
int HLECall(tRME_State *State, int IntNum)
{
	 int	ret;
	switch( IntNum )
	{
	case 0x03:
		printf("\nDebug Exception, press any key to exit\n");
		{
			SDL_Event	e;
			SDL_WM_SetCaption("RME - Debug Exception, press any key to quit", "RME - Stopped");
			gKeyBufferPos = 0;
			while( SDL_WaitEvent(&e) )
			{
				HandleEvent(&e);
				if( gKeyBufferPos )
					exit(0);
			}
		}
		break;
		
	case 0x13:
		switch(State->AX.B.H)
		{
		case 0x00:	// Reset Disk Subsystem
			printf(" 0x00 - Reset disk subsystem\n");
			State->Flags &= ~FLAG_CF;
			State->AX.B.H = 0;
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
			if( (State->AX.B.L & 0x3F) == 0 ) {
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
				printf(" 0x13:0x02 Incomplete read: %i/%i\n", ret, State->AX.B.L);
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
				
				// Maximum values are wanted
				cyl --;
				heads --;
				// Sector numbers are 1 based, so `sec` doesn't need to be changed
				
				State->AX.W = 0x0000;	// AX - Zero for success
				State->BX.B.L = type;	// BL - Disk Type (1.44M Floppy)
				State->CX.B.L = sec | ((cyl>>8)<<6);	// CL - Max Sector Number
				State->CX.B.H = cyl&0xFF;	// CH - Cylinder Count (Bits 0-7)
				State->DX.B.L = 1;	// DL - Number of drives
				State->DX.B.H = heads;	// DH - Maximum Head Number
				// Disk Parameter block
				State->ES = 0xF100;	State->DI.W = 0x0000;
			}
			break;

		case 0x15:	// Get Disk Type
			printf("HLE 0x13:0x15 - Get Disk Type 0x%02x\n", State->DX.B.L);
			{
				 int	cyl, heads, sec;
				State->Flags &= ~FLAG_CF;
				switch(GetDiskParams(State->DX.B.L, &cyl, &heads, &sec))
				{
				case 4:
					State->AX.B.H = 2;
					State->CX.W = sec * heads * cyl;
					State->DX.W = 0;
					break;
				default:
					printf(" - Disk type unknown\n");
					State->AX.B.H = 0;
					State->Flags |= FLAG_CF;
					break;
				}
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
				
				
				//printf(" packet = %p{Size:%i,Count=%i,Buffer=%04x:%04x,"
				//	"LBAStart=0x%"PRIx64",BufferLong=0x%"PRIx64"}\n",
				//	packet,
				//	packet->Size, packet->Count,
				//	packet->Buffer.Segment, packet->Buffer.Offset,
				//	packet->LBAStart, packet->BufferLong);
				
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
			printf("HLE Call INT 0x13 AH=0x%02x unknown\n", State->AX.B.H);
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	
	// --- Keyboard Input ---
	case 0x16:
		switch(State->AX.B.H)
		{
		// KEYBOARD - GET KEYSTROKE
		case 0x00:
			while( gKeyBufferPos == 0 )
			{
				SDL_Event	e;
				SDL_WaitEvent(&e);
				HandleEvent(&e);
			}
			State->AX.B.H = gKeyBuffer[0].Scancode;
			State->AX.B.L = gKeyBuffer[0].ASCII;
			gKeyBufferPos --;
			memmove(gKeyBuffer, gKeyBuffer+1, gKeyBufferPos*sizeof(gKeyBuffer[0]));
			break;
		// KEYBOARD - CHECK FOR KEYSTROKE
		case 0x01:
			// TODO: Keyboard queue
			if( gKeyBufferPos > 0 )
			{
				// Ignore scancodes > 83? (Non 83/84 keycodes)
				State->AX.B.H = gKeyBuffer[0].Scancode;
				State->AX.B.L = gKeyBuffer[0].ASCII;
				State->Flags &= ~FLAG_ZF;
			}
			else
			{
				State->Flags |= FLAG_ZF;
			}
			break;
		// KEYBOARD - GET SHIFT FLAGS
		case 0x02:
			// 0: Right Shift
			// 1: Left Shift
			// 2: Ctrl key
			// 3: Alt key
			// 4: Scroll Lock
			// 5: Num Lock
			// 6: Caps Lock
			// 7: Insert Lock
			State->AX.B.L = 0;
			break;
		// KEYBOARD - CHECK FOR ENHANCED KEYSTROKE
		case 0x11:
			if( gKeyBufferPos > 0 )
			{
				// Ignore scancodes > 83? (Non 83/84 keycodes)
				State->AX.B.H = gKeyBuffer[0].Scancode;
				State->AX.B.L = gKeyBuffer[0].ASCII;
				State->Flags &= ~FLAG_ZF;
			}
			else
			{
				State->Flags |= FLAG_ZF;
			}
			break;
		default:
			printf("HLE Call INT 0x16: AH=0x%02x unk\n", State->AX.B.H);
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	
	// --- Diskless Boot Hook (Boot error) ---
	case 0x18:
	// --- System Bootstrap Loader (called by MSDOS to reboot) ---
	case 0x19:
		PutString("\r\n[BIOS] Boot Error. Press any key to terminate emulator", COL_BLACK, COL_RED);
		{
			SDL_Event	e;
			while( SDL_WaitEvent(&e) )
			{
				if(e.type == SDL_QUIT)
					exit(0);
				else if(e.type == SDL_KEYDOWN)
					exit(0);
			}
		}
		break;
	
	default:
		printf("HLE Call INT 0x%02x Unknown\n", IntNum);
		RME_DumpRegs(State);
	}
	return 0;	// Emulate
}


uint8_t inb(uint16_t port) { return 0; }
uint16_t inw(uint16_t port) { return 0; }
uint32_t inl(uint16_t port) { return 0; }
void outb(uint16_t port, uint8_t val) { return ; }
void outw(uint16_t port, uint16_t val) { return ; }
void outl(uint16_t port, uint32_t val) { return ; }
