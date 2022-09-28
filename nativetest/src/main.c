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
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#define VIDEO_COLS	80
#define VIDEO_ROWS	25
//#define COL_WHITE	0x80808080
#define COL_BLACK	0x00000000
#define COL_GRAY	0x00444444
#define COL_RED 	0x00FF0000
#define COL_GREEN	0x0000FF00
#define COL_BLUE	0x000000FF
#define COL_LGRAY	0x00CCCCCC
#define COL_WHITE	0x00FFFFFF

// === IMPORTS ===
t_farptr	LoadDosExe(tRME_State *state, const char *file, t_farptr *stackptr);

// === PROTOTYPES ===
void	ParseArgs(int argc, char* argv[]);
void	PrintUsage(const char* argv0, bool show_full_help);
void	HandleEvent(SDL_Event *Event, tRME_State *EmuState);
Uint32	Video_RedrawTimerCb(Uint32 interval, void *unused);
 int	HLECall10(tRME_State *State, int IntNum);
 int	HLECall12(tRME_State *State, int IntNum);
 int	HLECall13(tRME_State *State, int IntNum);
 int	HLECall(tRME_State *State, int IntNum);
void	PutChar(uint8_t ch, uint8_t attr);
void	PutString(const char *String, uint8_t attr);
void	Video_Redraw(void);

// === GLOBALS ===
SDL_Surface	*gScreen;
char	*gasFDDs[4] = {"fdd.img", NULL, NULL, NULL};
FILE	*gaFDDs[4];
const char	*gsBinaryFile;
const char	*gsDosExe;
const char	*gsMemoryDumpFile;
const char	*gsCPUType = "80286";
 int	gbDisableGUI = 0;
uint8_t	gaMemory[0x110000];
// - GUI Key Queue
const int	cKeyBufferSize = 16;
 int	gKeyBufferPos = 0;
struct {
	uint8_t	Scancode;
	uint8_t	ASCII;
} gKeyBuffer[16];
// - Video output
 int	gbIsRedrawing;
 int	giCursorX, giCursorY;
uint8_t	gCurAttributes = 0x0F;

// === CODE ===
int main(int argc, char *argv[])
{
	tRME_State	*emu;
	void	*data;
	FILE	*fp;
	 int	ret, i;
	 int	len, tmp;

	// TODO: Better parameter interpretation
	ParseArgs(argc, argv);

	signal(SIGINT, exit);
	
	// Initialise memory
	memset(gaMemory, 0xF1, sizeof(gaMemory));	// 0xF1 = ICEBP/INT 1/#UD
	memset(gaMemory+0xB8000, 0x00, VIDEO_ROWS*VIDEO_COLS*2);

	if( !gbDisableGUI ) {
		SDL_Init(SDL_INIT_TIMER);
		gScreen = SDL_SetVideoMode(VIDEO_COLS*8, VIDEO_ROWS*16, 32, SDL_HWSURFACE);
		SDL_AddTimer(100, Video_RedrawTimerCb, NULL);
		PutString("RME NativeTest\r\n", 0x0F);
	}

	// Open FDD image
	if( gasFDDs[0] && gasFDDs[0][0] != '\0' )
	{
		printf("Loading '%s'\n", gasFDDs[0]);
		gaFDDs[0] = fopen(gasFDDs[0], "rb");
		if( !gaFDDs[0] ) {
			perror("Opening FDD image");
			//return 1;
		}
	}

	// Create BIOS Structures
	// - BIOS Entrypoint (All BIOS calls are HLE, so trap them)
	gaMemory[0xF0000] = 0x67;	// XCHG (RMX)
	gaMemory[0xF0001] = 0311;	// r BX BX
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
	emu->HLECallbacks[0x13] = HLECall13;	// 0x13 - Disk IO
	emu->HLECallbacks[0x16] = HLECall;	// 0x16 - Keyboard Input
	emu->HLECallbacks[0x18] = HLECall;	// 0x18 - Diskless Boot Hook
	emu->HLECallbacks[0x19] = HLECall;	// 0x19 - System Bootstrap Loader
	for( i = 0; i < 0x110000; i += RME_BLOCK_SIZE )
		emu->Memory[i/RME_BLOCK_SIZE] = &gaMemory[i];

	// DOS .exe file
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
		printf("Entry %x:%x, Stack %x:%x\n", ep.Segment, ep.Offset, stack.Segment, stack.Offset);
	}
	// Raw binary blob, loaded as a BIOS image at the end of memory
	else if( gsBinaryFile )
	{
		FILE	*fp = fopen(gsBinaryFile, "rb");
		off_t	len;

		memset(gaMemory, 0, 0x400);	// clear IVT		

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if( len & 15 ) {
			return -1;
		}		

		size_t	base = 0x100000 - len;
		printf("Booting '%s' at 0x%x\n", gsBinaryFile, (unsigned int)base);
		
		if(len > 0x10000)
		{
			fprintf(stderr, "Binary file '%s' is too large, not loading\n", gsBinaryFile);
		}
		else
		{
			size_t rv = fread( &gaMemory[base], 1, len, fp );
			if(rv != len) {
				fprintf(stderr, "Error reading binary '%s'. %zi != %zi\n%s\n",
					gsBinaryFile, rv, len, strerror(errno));
				exit(2);
			}
		}
		fclose(fp);

		emu->CS = 0xF000;
		emu->IP = 0xFFF0;
	}
	else if( gaFDDs[0] )
	{
		printf("Booting Disk #0\n");
		// Read boot sector
		if( gaFDDs[0] && fread( &gaMemory[0x7C00], 512, 1, gaFDDs[0] ) != 1 )
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
	else {
		fprintf(stderr, "Booting with no media!\n");
	}
	

	// Determine emulated CPU type
	if( strcmp(gsCPUType, "i8086") == 0 ) {
		emu->CPUType = RME_CPU_8086;
	}
	else if( strcmp(gsCPUType, "80286") == 0 ) {
		emu->CPUType = RME_CPU_80286;
	}
	else if( strcmp(gsCPUType, "386") == 0 ) {
		emu->CPUType = RME_CPU_386;
	}
	else {
		fprintf(stderr, "Unknown CPU type '%s'\n", gsCPUType);
		return 0;
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
			HandleEvent(&ev, emu);
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
	case RME_ERR_BREAKPOINT:
		printf("\n--- STOP: Breakpoint\n");
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

void ParseArgs(int argc, char* argv[])
{
	 int	nFDDs = 0;
	bool all_free = false;
	for( int i = 1; i < argc; i ++ )
	{
		const char* arg = argv[i];
		if( arg[0] != '-' || all_free ) {
			if(nFDDs < 4) {
				gasFDDs[nFDDs++] = argv[i];
			}
			else {
				fprintf(stderr, "To many FDD images provided\n");
				PrintUsage(argv[0], false);
				exit(1);
			}
		}
		else if( arg[1] == '\0') {
			fprintf(stderr, "'-' isn't a valid argument\n");
			exit(1);
		}
		// Short
		else if( arg[1] != '-' ) {
			switch( arg[1] )
			{
			case 'h':
				PrintUsage(arg, true);
				exit(0);
			case 'b':
				gsBinaryFile = argv[++i];
				break;
			case 'O':
				gsMemoryDumpFile = argv[++i];
				break;
			case 'd':
				gsDosExe = argv[++i];
				break;
			default:
				fprintf(stderr, "Unknown short option '-%c'\n", arg[1]);
				PrintUsage(argv[0], false);
				exit(1);
			}
		}
		// AllFree
		else if( arg[2] == '\0' ) {
			all_free = true;
		}
		// Long
		else
		{
			if( strcmp(arg, "--help") == 0 ) {
				PrintUsage(argv[0], true);
				exit(0);
			}
			else if( strcmp(arg, "--nogui") == 0 ) {
				gbDisableGUI = 1;
			}
			else if( strcmp(arg, "--cpu") == 0 ) {
				assert(i + 1 != argc);
				gsCPUType = argv[++i];
			}
			else {
				fprintf(stderr, "Unknown long option '%s'\n", arg);
				PrintUsage(argv[0], false);
				exit(1);
			}
		}
	}
}
void PrintUsage(const char* argv0, bool show_full_help)
{
	fprintf(stderr, "Usage: %s [-d <exename>|-b <bios>|<fdd0> ...]\n", argv0);
	if( !show_full_help ) {
		fprintf(stderr, "  Pass --help to see full help\n");
		return ;
	}
	fprintf(stderr, "\n"
		"-d <exename> | Load a DOS .exe file and run it\n"
		"-b <bios>    | Load the provided flat binary to the end of memory, and boot from 0xF000:FFF0\n"
		"<fddN>       | Open FDD images and boot from the first one (defaults to 'fdd.img' if none provided)\n"
		"\n"
		"--nogui      | Do not display a GUI\n"
		"--cpu <name> | Modify the emulated CPU variant (i8086, 80286, 386)\n"
		"-O <output>  | Dump memory contents after emulator stalls\n"
		"-h, --help   | Print this message\n"
		);
}

void HandleEvent(SDL_Event *Event, tRME_State *EmuState)
{
	switch(Event->type)
	{
	case SDL_QUIT:
		RME_DumpRegs(EmuState);
		fprintf(stderr, "Window closed, quitting\n");
		exit(0);
	case SDL_KEYDOWN:
		if( Event->key.keysym.sym == SDLK_BACKSPACE ) {
			RME_DumpRegs(EmuState);
		}
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
	case SDL_USEREVENT:
		Video_Redraw();
		gbIsRedrawing = 0;
		break;
	}
}

Uint32 Video_RedrawTimerCb(Uint32 interval, void *unused)
{
	if( !gbIsRedrawing )
	{
		gbIsRedrawing = 1;
		SDL_UserEvent	ue = {.type=SDL_USEREVENT,.code=0};
		SDL_Event	e = {.type=SDL_USEREVENT, .user = ue};
		SDL_PushEvent(&e);
	}
	return interval;
}

#include "font.h"
void DrawChar(int X, int Y, uint8_t ch, uint32_t BGC, uint32_t FGC)
{
	if( gbDisableGUI )
		return ;

	Uint8	*font;
	Uint32	*buf;

	SDL_Rect	rc = {0,0,1,1};
	
	font = &VTermFont[ch*FONT_HEIGHT];
	
	rc.w = 1; rc.h = 1;
	rc.x = X*FONT_WIDTH;
	rc.y = Y*FONT_HEIGHT;
	
	for(int y = 0; y < FONT_HEIGHT; y ++, rc.y ++)
	{
		for(int x = 0; x < FONT_WIDTH; x ++, rc.x++)
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
}

void Video_ScrollUp(int Page, uint8_t Attr, int nLines, int Top, int Left, int Bottom, int Right)
{
	// TODO
}

void Video_Redraw(void)
{
	uint32_t	colours[] = {
		0x000000, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFF0000, 0xFF00FF, 0x884400, 0xCCCCCC,
		0x444444, 0x4444FF, 0x44FF44, 0xFFFF44, 0xFF4444, 0xFF44FF, 0xFF8800, 0xFFFFFF,
	};
	// TODO: Other modes?
	uint8_t	*vidmem = &gaMemory[0xB8000];
	for( int row = 0; row < VIDEO_ROWS; row ++ )
	{
		for( int col = 0; col < VIDEO_COLS; col ++ )
		{
			uint8_t	ch = vidmem[(row*VIDEO_COLS+col)*2+0];
			uint8_t	at = vidmem[(row*VIDEO_COLS+col)*2+1];
			DrawChar(col, row, ch, colours[at>>4], colours[at&15]);
		}
	}
	SDL_Flip(gScreen);
//	printf("Video redraw complete\n");
}

/**
 * \brief Screen output
 */
void PutChar(uint8_t ch, uint8_t attrib)
{
	switch( ch )
	{
	case '\n':
		giCursorY ++;
		// TODO: Scroll
		if(giCursorY == 25)
			giCursorY = 0;
		return ;
	case '\r':
		giCursorX = 0;
		return ;
	case 8:
		if( giCursorX > 0 )
			giCursorX --;
		return ;
	}

	gaMemory[0xB8000 + (giCursorY*VIDEO_COLS+giCursorX)*2 + 0] = ch;
	gaMemory[0xB8000 + (giCursorY*VIDEO_COLS+giCursorX)*2 + 1] = attrib;	// TODO: Better attrib
	
	giCursorX ++;
	if(giCursorX == VIDEO_COLS) {
		giCursorX = 0;
		giCursorY ++;
		if(giCursorY == VIDEO_ROWS) {
			Video_ScrollUp(0, 1, attrib, 0, 0, VIDEO_ROWS, VIDEO_COLS);
			giCursorY --;
		}
	}
}

void PutString(const char *String, uint8_t attr)
{
	while(*String)
		PutChar(*String++, attr);
	Video_Redraw();
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
	//printf("ReadDiskLBA: (Disk=%i, LBAAddr=0x%x, Count=%i, Data=%p)\n",
	//	Disk, LBAAddr, Count, Data);
	switch(Disk)
	{
	case 0:
		if( fseek(gaFDDs[0], LBAAddr * 512, SEEK_SET) ) {
			fprintf(stdout, "fseek(gaFDDs[0], 0x%x*512, SEEK_SET)\n", LBAAddr);
			perror("FDD fseek failed");	
			memset(Data, 0, 512*Count);
			return 0;
		}
//		printf("ftell() = 0x%x\n", ftell(gaFDDs[0]));
		ret = fread(Data, 512, Count, gaFDDs[0]);
		if( ret != Count ) {
			perror("ReadDiskLBA  fread");
			printf(" %i/%i sectors read\n", ret, Count);
		}
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
	
	//printf("ReadDiskCHS: (Disk=%i, Cylinder=%i, Head=%i, Sector=%i, Count %i, Data=%p)\n",
	//	Disk, Cylinder, Head, Sector, Count, DataPtr);
	
	if( GetDiskParams(Disk, &nCyl, &nHead, &spt) == 0 ) {
		printf(" GetDiskParams(Disk=0x%02x) return 0\n", Disk);
		return -1;
	}
	
	//printf(" nCyl=%i, nHead=%i, spt=%i\n", nCyl, nHead, spt);
	
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
	//printf(" lbaAddr = %x\n", lbaAddr);
	
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
		if( State->AX.B.L == 3 ) {
			State->AX.B.L = 0x30;
			return 0;
		}
		printf("HLE Call INT 0x10/AH=0x00: VIDEO - SET VIDEO MODE AL=0x%x\n", State->AX.B.L);
		exit(1);
	// VIDEO - SET TEXT-MODE CURSOR SHAPE
	case 0x01:
		printf("INT10/AH=01 - TODO: Set cursor shape CH=%02x, CL=%02x", State->CX.B.H, State->CX.B.L);
		break;
	// VIDEO - SET CURSOR POSITION
	case 0x02:
		giCursorX = State->DX.B.L;
		giCursorY = State->DX.B.H;
		break;
	// VIDEO - SET ACTIVE DISPLAY PAGE
	case 0x05: {
		// giCurPage = State->AX.B.L;
		break; }
	// VIDEO - SCROLL UP WINDOW
	case 0x06: {
		 int	lines = State->AX.B.L;
		uint8_t	attr = (State->BX.B.H << 8);
		int tx = State->CX.B.L;
		int ty = State->CX.B.H;
		int bx = State->DX.B.L;
		int by = State->DX.B.H;
		
		if( lines == 0 ) {
			// AL=0: Clear screen
			for( int i = 0; i < VIDEO_ROWS*VIDEO_COLS; i ++ )
			{
				gaMemory[0xB8000+i*2+0] = 0;
				gaMemory[0xB8000+i*2+1] = attr;
			}
		}
		else {
			Video_ScrollUp(0, lines, attr, ty, tx, by, bx);
		}
		
		break; }
	// VIDEO - READ CHARACTER AND ATTRIBUTE AT CURSOR POSITION
	case 0x08:
		State->AX.B.L = gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+0];
		State->AX.B.H = gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+1];
		break;
	// VIDEO - WRITE CHARACTER AND ATTRIBUTE AT CURSOR POSITION
	case 0x09: {
		uint8_t	ch = State->AX.B.L;
		 int	page = State->BX.B.H;
		uint8_t	attr = State->BX.B.L;
		 int	count = State->CX.W;
		for( int i = 0; i < count; i ++ )
		{
			gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+0] = ch;
			gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+1] = attr;
			giCursorX ++;
			if( giCursorX == VIDEO_ROWS ) {
				giCursorX --;
				break;
			}
		}
		break; }
	// VIDEO - TELETYPE OUTPUT
	case 0x0E:
		// TODO: Better Colours
		PutChar(State->AX.B.L, gCurAttributes);
		//Video_Redraw();
		break;
	// VIDEO - GET CURRENT VIDEO MODE
	case 0x0F:
		State->AX.B.H = 80;	// Cols
		State->AX.B.L = 0x03;	// Mode Number
		State->BX.B.L = 0;	// Page
		break;
	// Extensions: 0x10XX
	case 0x10:
		switch(State->AX.W)
		{
		// VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
		case 0x1003:
			// TODO: Attributes
			gCurAttributes ^= 0x80;
			break;
		default:
			printf("HLE Call INT 0x10 AX=0x%04x Unk\n", State->AX.W);
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	// VIDEO - GET BLANKING ATTRIBUTE
	case 0x12:
		State->BX.B.H = 0;
		break;
	default:
		printf("HLE Call INT 0x10 AX=%04x Unk\n", State->AX.W);
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

/*
 * \brief HLE Calls "INT 0x13" Disk Subsystem
 */
int HLECall13(tRME_State *State, int IntNum)
{
	 int	ret;
	switch(State->AX.B.H)
	{
	// DISK - RESET DISK SYSTEM
	case 0x00:
		printf("HLE 0x13:0x00 - Reset disk subsystem\n");
		State->Flags &= ~FLAG_CF;
		State->AX.B.H = 0;
		// Does anything need to be done here?
		break;
	
	case 0x02: {	// Read Sector(s) into memory
		//RME_DumpRegs(State);
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
		
		 int	disk = State->DX.B.L;
		 int	cyl = State->CX.B.H | ((State->CX.B.L & 0xC0)<<2);
		 int	head = State->DX.B.H;
		 int	sect = State->CX.B.L & 0x3F;
		 int	count = State->AX.B.L;
		ret = ReadDiskCHS( disk, cyl, head, sect, count, &gaMemory[ State->ES*16 + State->BX.W ] );
		// Error check
		if( ret < 0 ) {
			printf(" 0x13:0x02 ReadDiskCHS Ret -0x%x\n", -ret);
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
		printf("HLE 0x13:0x02 - Read sectors (D%02x,%i,%i,%i)+%i to %x:%x\n",
			disk, cyl, head, sect, count, State->ES, State->BX.W 
			);
		State->AX.B.H = 0;
		State->Flags &= ~1;
		break; }
	
	case 0x08: {	// Get Drive Parameters
		printf("HLE 0x13:0x08 - Get Drive Parameters (D%02x)\n", State->DX.B.L);
		State->Flags &= ~(FLAG_CF);
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
		break; }

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
				HandleEvent(&e, State);
				if( gKeyBufferPos )
					exit(0);
			}
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
				HandleEvent(&e, State);
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
		PutString("\r\n[BIOS] Boot Error. Press any key to terminate emulator", 0x04);
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


uint8_t inb(uint16_t port) {
	printf("INB 0x%x\n", port);
	return 0;
}
uint16_t inw(uint16_t port) {
	printf("INW 0x%x\n", port);
	return 0;
}
uint32_t inl(uint16_t port) {
	printf("INL 0x%x\n", port);
	return 0;
}
void outb(uint16_t port, uint8_t val) {
	printf("OUTB 0x%x, 0x%x\n", port, val);
	return ;
}
void outw(uint16_t port, uint16_t val) {
	printf("OUTW 0x%x, 0x%x\n", port, val);
	return ;
}
void outl(uint16_t port, uint32_t val) {
	printf("OUTL 0x%x, 0x%x\n", port, val);
	return ;
}
