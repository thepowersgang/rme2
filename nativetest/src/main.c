/*
 * Realmode Emulator - Native Tester
 */
#ifndef ENABLE_GUI
# define ENABLE_GUI	0
#endif
#include "common.h"
//#include "dev_vga.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <rme.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>	// off_t

#define VIDEO_COLS	80
#define VIDEO_ROWS	25

// === PROTOTYPES ===
void	ParseArgs(int argc, char* argv[]);
void	PrintUsage(const char* argv0, bool show_full_help);
void	PrintDebug(struct sRME_State *State, const char* Fmt, va_list args);
void	PrintError(struct sRME_State *State, const char* Fmt, va_list args);
 int	HLECall3(struct sRME_State *State, int IntNum);
 int	IoCall_In(tRME_State* State, uint16_t Port, size_t Size, void* Dst);
 int	IoCall_Out(tRME_State* State, uint16_t Port, size_t Size, uint32_t Val);

// === GLOBALS ===
char	*gasFDDs[4] = {"fdd.img", NULL, NULL, NULL};
const char	*gsBinaryFile;
const char	*gsDosExe;
const char	*gsLogFile;
const char	*gsMemoryDumpFile;
const char	*gsCPUType = "80286";
 int	gbDisableGUI = 0;

FILE	*gaFDDs[4];
FILE*	gLogFile;
/// @brief CONFIG - Enable difference calculation in memory 
bool	gbDiff_Memory;
/// @brief Current working copy of the memory
uint8_t	gaMemory[0x110000];
/// @brief Saved copy of memory state, used to detect changes 
uint8_t	gaMemory_prev[0x110000];
/// @brief Saved copy of register state
struct PrevRegisters {
	uint32_t	gprs[8];

	uint16_t	ss;	//!< Stack Segment
	uint16_t	ds;	//!< Data Segment
	uint16_t	es;	//!< Extra Segment
	uint16_t	fs;	//!< Extra Segment 2
	uint16_t	gs;	//!< Extra Segment 3

	uint16_t	cs;	//!< Code Segment
	uint16_t	ip;	//!< Instruction Pointer
	uint16_t	flags;	//!< State Flags
} gPrevRegisters;
int gDebugLevel = DEBUG;
// - GUI Key Queue
const int	cKeyBufferSize = 16;
 int	gKeyBufferPos = 0;
struct sKeyBufEnt gKeyBuffer[16];

// === CODE ===
int main(int argc, char *argv[])
{
	tRME_Callbacks	callbacks = {0};
	tRME_State	*emu;
	void	*data;
	FILE	*fp;
	 int	ret, i;
	 int	len, tmp;

	// TODO: Better parameter interpretation
	ParseArgs(argc, argv);

	if( gsLogFile ) {
		gLogFile = fopen(gsLogFile, "w");
		if(!gLogFile) {
			perror("Opening log file");
			return 1;
		}
	}

	signal(SIGINT, exit);
	
	// Initialise memory
	memset(gaMemory, 0xF1, sizeof(gaMemory));	// 0xF1 = ICEBP/INT 1/#UD
	memset(gaMemory+0xB8000, 0x00, VIDEO_ROWS*VIDEO_COLS*2);

	#if ENABLE_GUI
	if( !gbDisableGUI ) {
		UiSdl_Init();
    	PutString("RME NativeTest\r\n", 0x0F);
	}
	#endif

	// Open FDD image
	for(int i = 0; i < 4; i ++)
	{
		if( gasFDDs[i] && gasFDDs[i][0] != '\0' ) {
			PrintDebugF(emu, "Opening FD%i '%s'\n", i, gasFDDs[i]);
			gaFDDs[i] = fopen(gasFDDs[i], "rb");
			if( !gaFDDs[i] ) {
				fprintf(stderr, "Failed to open FDD image #%i: '%s' - %s\n", i, gasFDDs[i], strerror(errno));
			}
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
	// - Disk Parameter Block
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

	callbacks.PrintDebug = PrintDebug;
	callbacks.PrintError = PrintError;
	callbacks.In = IoCall_In;
	callbacks.Out = IoCall_Out;
	// Exception handling
	callbacks.HLECallbacks[0x03] = HLECall3;	// 0x03 - Debug
	// BIOS Calls
	if(true)
	{
		callbacks.HLECallbacks[0x10] = HLECall10;	// 0x10 - VGA BIOS
		callbacks.HLECallbacks[0x11] = HLECall  ;	// 0x11 - BIOS Equipment List
		callbacks.HLECallbacks[0x12] = HLECall12;	// 0x12 - Get Memory Size
		callbacks.HLECallbacks[0x13] = HLECall13;	// 0x13 - Disk IO
		callbacks.HLECallbacks[0x16] = HLECall;	// 0x16 - Keyboard Input
		callbacks.HLECallbacks[0x18] = HLECall;	// 0x18 - Diskless Boot Hook
		callbacks.HLECallbacks[0x19] = HLECall;	// 0x19 - System Bootstrap Loader
		callbacks.HLECallbacks[0x1a] = HLECall;	// 0x1a - Time
	}
	if(true)
	{
		// DOS etc calls
		callbacks.HLECallbacks[0x21] = HLECall21;	// 0x21 - DOS System Calls
		callbacks.HLECallbacks[0x2F] = HLECall21;	// 0x2F - Various, but EMS/HIMEM.SYS is why this is here
	}

	// Create and initialise RME State
	emu = RME_CreateState(&callbacks, NULL);
	emu->DebugLevel = gDebugLevel;
	for( i = 0; i < 0x110000; i += RME_BLOCK_SIZE ) {
		emu->Memory[i/RME_BLOCK_SIZE] = &gaMemory[i];
	}

	// DOS .exe file
	if( gsDosExe )
	{
		PrintDebugF(emu, "Loading DOS Exe \"%s\"\n", gsDosExe);
		if( LoadDosExe(emu, gsDosExe) ) {
			return -1;
		}
		PrintDebugF(emu, "Entry %x:%x, Stack %x:%x\n", emu->CS, emu->IP, emu->SS, emu->SP.W);
	}
	// Raw binary blob, loaded as a BIOS image at the end of memory
	else if( gsBinaryFile )
	{
		FILE	*fp = fopen(gsBinaryFile, "rb");
		if(!fp) {
			perror("Opening BIOS ROM file");
			return 1;
		}
		off_t	len;

		memset(gaMemory, 0, 0x400);	// clear IVT	

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		// Assert that the file is an enum number of paragraphs (segment steps)
		if( len & 15 ) {
			fprintf(stderr, "BIOS file size is not an even multiple of 16 bytes (0x%lx)\n", len);
			return -1;
		}
		// Must be at least 16 bytes long, in order for the entrypoint to be populated
		if( len >= 16 ) {
			fprintf(stderr, "BIOS file size is not an even multiple of 16 bytes (0x%lx)\n", len);
			return -1;
		}
		// Max size of 64KiB to fit between 0xF_0000 and 0x10_0000
		if( len > 0x10000 ) {
			fprintf(stderr, "BIOS file '%s' is too large (0x%lx > 0x10000), not loading\n", gsBinaryFile, len);
			return -1;
		}

		size_t	base = 0x100000 - len;
		printf("Booting '%s' at 0x%x\n", gsBinaryFile, (unsigned int)base);
		size_t rv = fread( &gaMemory[base], 1, len, fp );
		if(rv != len) {
			fprintf(stderr, "Error reading binary '%s'. %zi != %zi\n%s\n",
				gsBinaryFile, rv, len, strerror(errno));
			return -1;
		}
		fclose(fp);

		emu->CS = 0xF000;
		emu->IP = 0xFFF0;
		emu->SS = 0xA000;
		emu->SP.W = 0xFFFE;
		// set the return address to -1
		*(uint16_t*)&gaMemory[0xA0000-2] = 0xFFFF;
	}
	else if( gaFDDs[0] )
	{
		printf("Booting FDD #0 with BIOS emulation\n");

		// Read boot sector
		const size_t load_addr = 0x7C00;
		if( fread( &gaMemory[load_addr], 512, 1, gaFDDs[0] ) != 1 )
		{
			fprintf(stderr, "Failed to read boot sector from disk image (%s)\n", strerror(errno));
			return 0;
		}
		uint16_t sig = *(uint16_t*)&gaMemory[load_addr+0x1FE];
		if( sig != 0xAA55 )
		{
			fprintf(stderr, "Invalid boot signature on boot sector: 0x%04x != exp 0x%04x\n", sig, 0xAA55);
			return 0;
		}

		emu->CS = load_addr >> 4;
		emu->IP = 0x0000;
		emu->DX.W = 0x0000;	// Disk number = 0
		// Initialise the stack, and set a return address to 0xFFFF
		emu->SS = 0xA000;
		emu->SP.W = 0xFFFE;
		*(uint16_t*)&gaMemory[0xA0000-2] = 0xFFFF;
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

	memcpy(gaMemory_prev, gaMemory, sizeof(gaMemory));

	// Main emulation loop
	while( (ret = RME_RunOne(emu)) == RME_ERR_OK )
	{
		// Check for a change to the state (memory or registers)
		if(emu->DebugLevel >= 1)
		{
			bool printed = false;
			#define PRINT(...) do { if(!printed) { printed = true; PrintDebugF(emu, ">"); } PrintDebugF(emu, __VA_ARGS__); } while(0)
			#define CHECK_REG(name, prev, cur)	do { if(prev != cur) { PRINT(" %s:%04x=>%04x", name, prev, cur); prev = cur; } } while(0)
			CHECK_REG("AX", gPrevRegisters.gprs[0], emu->AX.D);
			CHECK_REG("CX", gPrevRegisters.gprs[1], emu->CX.D);
			CHECK_REG("DX", gPrevRegisters.gprs[2], emu->DX.D);
			CHECK_REG("BX", gPrevRegisters.gprs[3], emu->BX.D);
			CHECK_REG("SP", gPrevRegisters.gprs[4], emu->SP.D);
			CHECK_REG("BP", gPrevRegisters.gprs[5], emu->BP.D);
			CHECK_REG("SI", gPrevRegisters.gprs[6], emu->SI.D);
			CHECK_REG("DI", gPrevRegisters.gprs[7], emu->DI.D);
			CHECK_REG("SS", gPrevRegisters.ss, emu->SS);
			CHECK_REG("DS", gPrevRegisters.ds, emu->DS);
			CHECK_REG("ES", gPrevRegisters.es, emu->ES);
			CHECK_REG("FS", gPrevRegisters.fs, emu->FS);
			CHECK_REG("GS", gPrevRegisters.gs, emu->GS);
			CHECK_REG("Flags", gPrevRegisters.flags, emu->Flags);
			if( gbDiff_Memory )
			{
				assert( (sizeof(gaMemory) % RME_BLOCK_SIZE) == 0 );
				for(size_t ofs = 0; ofs < sizeof(gaMemory); ofs += RME_BLOCK_SIZE)
				{
					if( emu->MemoryTouched[ofs / RME_BLOCK_SIZE] )
					{
						emu->MemoryTouched[ofs / RME_BLOCK_SIZE] = 0;
						for(size_t i = ofs; i < ofs + RME_BLOCK_SIZE; i += 2) {
							const uint8_t* cur = gaMemory+i;
							uint8_t* prev = gaMemory_prev+i;
							if( memcmp(prev, cur, 2) != 0 ) {
								uint16_t pv = prev[0] | ((uint16_t)prev[1] << 8);
								uint16_t cv = cur[0] | ((uint16_t)cur[1] << 8);
								PRINT(" %05zx:%04x=>%04x", i, pv, cv);
								memcpy(prev, cur, 2);
							}
						}
					}
				}
			}
			if(printed) PrintDebugF(emu, "\n");
			#undef PRINT
			#undef CHECK_REG
		}

		for(size_t ofs = 0; ofs < sizeof(gaMemory); ofs += RME_BLOCK_SIZE)
		{
			emu->MemoryTouched[ofs / RME_BLOCK_SIZE] = 0;
		}

#if ENABLE_GUI
		if( !gbDisableGUI ) {
			UiSdl_PollEvents(emu);
		}
#endif
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
		RME_DumpRegs(emu);
		return 1;
	case RME_ERR_DIVERR:
		printf("\n--- ERROR: Division Fault\n");
		RME_DumpRegs(emu);
		return 1;
	case RME_ERR_BUG:
		printf("\n--- ERROR: Emulator bug\n");
		RME_DumpRegs(emu);
		return 1;
	case RME_ERR_BREAKPOINT:
		printf("\n--- STOP: Breakpoint\n");
		RME_DumpRegs(emu);
	case RME_ERR_HALT:
		#if ENABLE_GUI
		if(! gbDisableGUI )
		{
			UiSdl_Halted();
		}
		printf("\n--- STOP: CPU Halted\n");
		#endif
		printf("\n");
		return 0;
		break;
	default:
		printf("\n--- ERROR: Unknown error %i\n", ret);
		RME_DumpRegs(emu);
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
			case 'l':
				gsLogFile = argv[++i];
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
			else if( strcmp(arg, "--no-gui") == 0 ) {
				gbDisableGUI = 1;
			}
			else if( strcmp(arg, "--cpu") == 0 ) {
				assert(i + 1 != argc);
				gsCPUType = argv[++i];
			}
			else if( strcmp(arg, "--log-file") == 0 ) {
				assert(i + 1 != argc);
				gsLogFile = argv[++i];
			}
			else if( strcmp(arg, "--debug-level") == 0 ) {
				assert(i + 1 != argc);
				gDebugLevel = strtol(argv[++i], NULL, 10);
			}
			else if( strcmp(arg, "--diff-memory") == 0 ) {
				gbDiff_Memory = true;
			}
			else {
				fprintf(stderr, "Unknown long option '%s'\n", arg);
				PrintUsage(argv[0], false);
				exit(1);
			}
		}
	}
	#if ENABLE_GUI
	#else
	if( !gbDisableGUI ) {
		fprintf(stderr, "NOTE: GUI disabled at compile-time\n");
	}
	#endif
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
		"--no-gui     | Do not display a GUI\n"
		"--cpu <name> | Modify the emulated CPU variant (i8086, 80286, 386)\n"
		"-O <output>  | Dump memory contents after emulator stalls\n"
		"-h, --help   | Print this message\n"
		);
}

#define SCANCODE_ENTER	1
#define SCANCODE_SHIFT	2
void Input_PushKeysFromChar(char ch)
{
	switch(ch)
	{
	case '\n':
		gKeyBuffer[gKeyBufferPos].Scancode = SCANCODE_ENTER;
		break;
	case 'A' ... 'Z':
		gKeyBuffer[gKeyBufferPos].Scancode = SCANCODE_SHIFT;
		gKeyBuffer[gKeyBufferPos].ASCII = 0;
		gKeyBufferPos++;
	case 'a' ... 'z':
		gKeyBuffer[gKeyBufferPos].Scancode = ch & ~0x20;
		break;
	default:
		exit(1);
	}
	gKeyBuffer[gKeyBufferPos].ASCII = ch;
	gKeyBufferPos ++;
}
void Input_PushKey(int scancode, int ch)
{
	if( gKeyBufferPos == cKeyBufferSize ) {
	}
	else {
		gKeyBuffer[gKeyBufferPos].Scancode = scancode;
		gKeyBuffer[gKeyBufferPos].ASCII = ch;
		printf("%i: %x %x\n",
			gKeyBufferPos, gKeyBuffer[gKeyBufferPos].Scancode, gKeyBuffer[gKeyBufferPos].ASCII);
		gKeyBufferPos ++;
	}
}


void FatalErrorF(struct sRME_State *State, const char* Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	vfprintf(stderr, Fmt, args);
	va_end(args);
	exit(1);
}
void PrintDebugF(struct sRME_State *State, const char* Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	PrintDebug(State, Fmt, args);
	va_end(args);
}
void PrintDebug(struct sRME_State *State, const char* Fmt, va_list args)
{
	if( gLogFile ) {
		vfprintf(gLogFile, Fmt, args);
	}
	else {
		vprintf(Fmt, args);
	}
}
void PrintError(struct sRME_State *State, const char* Fmt, va_list args)
{
	PrintDebug(State, Fmt, args);
}
int	HLECall3(struct sRME_State *State, int IntNum)
{
	printf("\nDebug Exception, press any key to exit\n");
	#if ENABLE_GUI
	if( !gbDisableGUI )
	{
		UiSdl_Halted("Debug Exception");
	}
	#endif
	exit(0);
}

int IoCall_In(tRME_State* State, uint16_t Port, size_t Size, void* Dst) {
	switch(Port)
	{
	//case 0x3ce:
	//case 0x3cf:
	//	return VGA_IoPort_In(State, Port, Size, Dst);
	default:
		FatalErrorF(State, "TODO: in%i 0x%x\n", (int)Size, Port);
	}
}
int IoCall_Out(tRME_State* State, uint16_t Port, size_t Size, uint32_t Val) {
	switch(Port)
	{
	//case 0x3ce:
	//case 0x3cf:
	//	return VGA_IoPort_Out(State, Port, Size, Val);
	default:
		FatalErrorF(State, "TODO: out%i 0x%x, 0x%0*x\n", (int)Size, Port, 2*(int)Size, Val);
	}
}
