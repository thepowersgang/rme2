/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <rme.h>

struct Args {
	int debug_level;
	const char* binfile;
	const char* memory_dump_file;
	const char* cpu_type;
};

uint8_t	gaMemory[0x110000];
uint8_t	gaMemory_prev[0x110000];
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

void ParseArgs(struct Args* args, int argc, char* argv[]);
void PrintUsage(const char* argv0, bool show_full_help);

// === CODE ===
int main(int argc, char *argv[])
{
	tRME_State	*emu;
	struct Args	args = {0};

	// TODO: Better parameter interpretation
	ParseArgs(&args, argc, argv);

	signal(SIGINT, exit);
	
	// Initialise memory
	memset(gaMemory, 0xF1, sizeof(gaMemory));	// 0xF1 = ICEBP/INT 1/#UD
	//memset(gaMemory+0xB8000, 0x00, VIDEO_ROWS*VIDEO_COLS*2);

	// Create BIOS Structures
	// - BIOS Entrypoint (All BIOS calls are HLE, so trap them)
	gaMemory[0xF0000] = 0x67;	// XCHG (RMX)
	gaMemory[0xF0001] = 0311;	// r BX BX
	// - Interrupt Vector Table
	for( int i = 0; i < 0x100; i ++ )
	{
		gaMemory[i*4+0] = i;
		gaMemory[i*4+1] = 0x00;
		gaMemory[i*4+2] = RME_HLE_CS&0xFF;
		gaMemory[i*4+3] = RME_HLE_CS>>8;
	}

	// Create and initialise RME State
	emu = RME_CreateState();
	emu->DebugLevel = args.debug_level;
	//emu->HLECallbacks[0x03] = HLECall;	// 0x03 - Debug
	//emu->HLECallbacks[0x10] = HLECall10;	// 0x10 - VGA BIOS
	//emu->HLECallbacks[0x12] = HLECall12;	// 0x12 - Get Memory Size
	//emu->HLECallbacks[0x13] = HLECall13;	// 0x13 - Disk IO
	//emu->HLECallbacks[0x16] = HLECall;	// 0x16 - Keyboard Input
	//emu->HLECallbacks[0x18] = HLECall;	// 0x18 - Diskless Boot Hook
	//emu->HLECallbacks[0x19] = HLECall;	// 0x19 - System Bootstrap Loader
	for( int i = 0; i < 0x110000; i += RME_BLOCK_SIZE ) {
		emu->Memory[i/RME_BLOCK_SIZE] = &gaMemory[i];
	}

	// Raw binary blob, loaded as a BIOS image at the end of memory
	if( args.binfile )
	{
		FILE	*fp = fopen(args.binfile, "rb");
		off_t	len;

		memset(gaMemory, 0, 0x400);	// clear IVT		

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if( len & 15 ) {
			return -1;
		}		

		size_t	base = 0x100000 - len;
		printf("Booting '%s' at 0x%x\n", args.binfile, (unsigned int)base);
		
		if(len > 0x10000)
		{
			fprintf(stderr, "Binary file '%s' is too large, not loading\n", args.binfile);
		}
		else
		{
			ssize_t rv = fread( &gaMemory[base], 1, len, fp );
			if(rv != len) {
				fprintf(stderr, "Error reading binary '%s'. %zi != %zi\n%s\n",
					args.binfile, rv, len, strerror(errno));
				exit(2);
			}
		}
		fclose(fp);

		emu->CS = 0xF000;
		emu->IP = 0xFFF0;
	}

	// Determine emulated CPU type
	if( args.cpu_type )
	{
		if( strcmp(args.cpu_type, "i8086") == 0 ) {
			emu->CPUType = RME_CPU_8086;
		}
		else if( strcmp(args.cpu_type, "80286") == 0 ) {
			emu->CPUType = RME_CPU_80286;
		}
		else if( strcmp(args.cpu_type, "386") == 0 ) {
			emu->CPUType = RME_CPU_386;
		}
		else {
			fprintf(stderr, "Unknown CPU type '%s'\n", args.cpu_type);
			return 0;
		}
	}
	
//	emu->SS = 0xA000;
	emu->SP.W = 0xFFFE;
	*(uint16_t*)&gaMemory[0xA0000-2] = 0xFFFF;

	memcpy(gaMemory_prev, gaMemory, sizeof(gaMemory));

	// Main emulation loop
	int ret;
	while( (ret = RME_RunOne(emu)) == RME_ERR_OK )
	{
		// Check for a change to the state (memory or registers)
		if(emu->DebugLevel >= 1)
		{
			bool printed = false;
			#define PRINT(...) do { if(!printed) { printed = true; printf(">"); } printf(__VA_ARGS__); } while(0)
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
			if( memcmp(gaMemory_prev, gaMemory, sizeof(gaMemory)) != 0 ) {
					for(size_t i = 0; i < sizeof(gaMemory); i += 2) {
						const uint8_t* cur = gaMemory+i;
						uint8_t* prev = gaMemory_prev+i;
						if( memcmp(prev, cur, 2) != 0 ) {
							uint16_t pv = prev[0] | ((uint16_t)prev[1] << 8);
							uint16_t cv = cur[0] | ((uint16_t)cur[1] << 8);
							PRINT(" %05zx:%04x=>%04x", i, pv, cv);
							memcpy(prev, cur, 4);
						}
					}
			}
			if(printed) printf("\n");
			#undef PRINT
			#undef CHECK_REG
		}
	}

	// Write out memory
	if( args.memory_dump_file )
	{
		FILE *fp = fopen(args.memory_dump_file, "wb");
		fwrite(gaMemory, 1024*1024, 1, fp);
		fclose(fp);
		printf("\n--- Memory written to '%s'", args.memory_dump_file);
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
		printf("\n");
		return 0;
	default:
		printf("\n--- ERROR: Unknown error %i\n", ret);
		return 1;
	}

	return 0;
}

void ParseArgs(struct Args* args, int argc, char* argv[])
{
	bool all_free = false;
	for( int i = 1; i < argc; i ++ )
	{
		const char* arg = argv[i];
		if( arg[0] != '-' || all_free ) {
			if( !args->binfile ) {
				args->binfile = argv[i];
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
			case 'O':
				args->memory_dump_file = argv[++i];
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
			else if( strcmp(arg, "--cpu") == 0 ) {
				assert(i + 1 != argc);
				args->cpu_type = argv[++i];
			}
			else if( strcmp(arg, "--debug-level") == 0 ) {
				assert(i + 1 != argc);
				args->debug_level = strtol(argv[++i], NULL, 10);
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
	fprintf(stderr, "Usage: %s <bios>\n", argv0);
	if( !show_full_help ) {
		fprintf(stderr, "  Pass --help to see full help\n");
		return ;
	}
	fprintf(stderr, "\n"
		"<bios>       | Load the provided flat binary to the end of memory, and boot from 0xF000:FFF0\n"
		"\n"
		"--cpu <name> | Modify the emulated CPU variant (i8086, 80286, 386)\n"
		"-O <output>  | Dump memory contents after emulator stalls\n"
		"-h, --help   | Print this message\n"
		);
}
