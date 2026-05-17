/*
 * Realmode Emulator - Native Tester
 * 
 * - DOS Executable Loader (and DOS syscalls)
 */
#include "common.h"
#include <rme.h>
#include "hle_dos.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>	// off_t

// Segment for the PSP, code is loaded 256 bytes above
#define DESTINATION_SEG	0x0FF0
#define SEG_ENVIRON	0x0100

inline unsigned int MIN(unsigned int a, unsigned int b) {return (a < b)?a:b;}

struct ProgramSegmentPrefix
{
	// 00
	uint8_t	exit_op[2];
	uint16_t	first_free_seg;
	// 04
	uint8_t	_reserved4;
	uint8_t	os_entry_pt_deprecated[5];
	// 0A
	//uint16_t	program_bytes;
	struct FAR_PTR	int22_addr;
	struct FAR_PTR	int23_addr;
	struct FAR_PTR	int24_addr;
	// 0x10
	uint16_t	parent_psp;
	uint8_t	_jft[20];
	// 0x2C
	uint16_t	env_segment;
	char	_pad[0x80 - 0x2E];

	// 0x80
	uint8_t	command_tail_len;
	uint8_t command_tail[0x7F];
} __attribute__((packed));
static int assert_size_ProgramSegmentPrefix[sizeof(struct ProgramSegmentPrefix) == 0x100 ? 1 : -1];

// === CODE ===
int LoadDosExe(tRME_State *state, const char *file)
{
	tExeHeader	hdr;
	FILE	*fp;
	 int	relocStart;
	void	*data;
	
	fp = fopen(file, "rb");
	if(!fp) {
		printf("File '%s' does not exist\n", file);
		return RME_ERR_INVAL;
	}
	if( fread(&hdr, sizeof(tExeHeader), 1, fp) != 1 ) {
		perror("LoadDosExe - fread header");
		return RME_ERR_INVAL;
	}
	
	// Sanity check signature
	if(hdr.signature != 0x5A4D) {
		printf("DOS EXE header is invalid (%04x)\n", hdr.signature);
		return RME_ERR_INVAL;
	}
	
	printf("LoadDosExe: hdr.cs = %x, hdr.ip = %x\n", hdr.cs, hdr.ip);
	const int dataStart = hdr.header_paragraphs*16;
	const int dataSize = hdr.blocks_in_file * 512 - (512 - hdr.bytes_in_last_block) % 512 - dataStart;
	relocStart = hdr.reloc_table_offset;
	printf("LoadDosExe: dataStart = %x, dataSize = %x, relocStart = %x\n",
		dataStart, dataSize, relocStart);
	
	fseek(fp, dataStart, SEEK_SET);
	data = malloc(dataSize);
	if( !data ) {
		perror("LoadDosExe - malloc");
		exit(1);
	}
	{
		size_t read_count = fread(data, 1, dataSize, fp);
		if(read_count != dataSize) {
			perror("LoadDosExe - read data");
			fprintf(stderr, " - %zu/%i bytes read\n", read_count, dataSize);
			goto _error;
		}
	}
	
	// Relocate
	fseek(fp, relocStart, SEEK_SET);
	for( int i = 0; i < hdr.num_relocs; i++ )
	{
		tExeReloc	reloc;
		if( fread(&reloc, sizeof(tExeReloc), 1, fp) != 1 ) {
			perror("LoadDosExe - fread reloc");
			goto _error;
		}
		printf("- Reloc %x:%x (%x) += %x\n", reloc.segment, reloc.offset,
			*(uint16_t*)(data + reloc.segment*16 + reloc.offset),
			DESTINATION_SEG
			);
		*(uint16_t*)(data + reloc.segment*16 + reloc.offset) += DESTINATION_SEG;
	}

	uint32_t dest_addr = DESTINATION_SEG*16 + 0x100;
	 int block_idx = dest_addr / RME_BLOCK_SIZE;
	uint8_t	*readdata = data;
	int remain_data = dataSize;
	if( dest_addr % RME_BLOCK_SIZE != 0 ) {
		unsigned ofs = dest_addr % RME_BLOCK_SIZE;
		size_t	copysize = MIN(RME_BLOCK_SIZE - ofs, remain_data);
		printf("- Partial copy 0x%x+0x%x 0x%zx\n", block_idx*RME_BLOCK_SIZE, ofs, copysize);
		memcpy(state->Memory[block_idx] + ofs, readdata, copysize);
		readdata += copysize;
		remain_data -= copysize;
		block_idx ++;
	}
	while(remain_data > 0)
	{
		size_t	copysize = MIN(RME_BLOCK_SIZE, remain_data);
		printf("- Full copy 0x%05x : 0x%zx from 0x%lx\n", block_idx*RME_BLOCK_SIZE, copysize, (readdata - (uint8_t*)data) + dataStart);
		memcpy(state->Memory[block_idx], readdata, copysize);
		readdata += copysize;
		remain_data -= copysize;
		block_idx ++;
	}

	free(data);
	fclose(fp);

	state->AX.W = 0;	// Length of command-line (or zero)
	state->BX.W = dataSize >> 16;
	state->CX.W = dataSize & 0xFFFF;
	state->DX.W = 0;	// Zero
	state->CS = DESTINATION_SEG + 0x10 + hdr.cs;
	state->IP = hdr.ip;
	state->SS   = DESTINATION_SEG + 0x10 + hdr.ss;
	state->SP.W = hdr.sp;
	// DS/ES are set to the load address
	state->DS = DESTINATION_SEG;
	state->ES = DESTINATION_SEG;

	{
		struct sRME_MemRef	mem;
		RME_GetPtr(state, DESTINATION_SEG, 0, 256, &mem);
		assert(mem.len_1 == 256);
		struct ProgramSegmentPrefix* psp = mem.range_1;
		memset(psp, 0, sizeof(psp));
		psp->exit_op[0] = 0xCD; psp->exit_op[1] = 0x20;	// INT 0x20
		psp->first_free_seg = DESTINATION_SEG + 0x10 + hdr.blocks_in_file * 512 / 16 + hdr.min_extra_paragraphs;
		psp->env_segment = SEG_ENVIRON;
		//psp->program_bytes = 0;

		// Create environment
		RME_GetPtr(state, SEG_ENVIRON, 0, 256, &mem);
		assert(mem.len_1 == 256);
		memcpy(mem.range_1, "PATH=C:\0", 9);
	}

	return RME_ERR_OK;
_error:
	fclose(fp);
	free(data);
	return RME_ERR_BUG;
}

static int get_ascii_z(tRME_State* State, uint16_t Seg, uint16_t Ofs, const char** out_ptr)
{
	int ret;
	struct sRME_MemRef mem;
	uint16_t cur_ofs = Ofs;
	do {
		ret = RME_GetPtr(State, Seg, cur_ofs, 1, &mem);
		if(ret) return ret;
		if( *(char*)mem.range_1 == 0 ) {
			// Found the NUL
			uint16_t len = 1 + (cur_ofs - Ofs);
			ret = RME_GetPtr(State, Seg, Ofs, len, &mem);
			if(ret) return ret;
			if( mem.len_1 != len ) {
				FatalErrorF(State, "TODO: Handle cross-region strings\n");
				exit(1);
			}
			else {
				*out_ptr = mem.range_1;
			}
			return 0;
		}
		cur_ofs += 1;
	} while(cur_ofs != 0);
	PrintDebugF(State, "Failed to find NUL in %04x:%04x\n", Seg, Ofs);
	return RME_ERR_INVAL;
}
static int free_ascii_z(tRME_State* State, const char* str_ptr)
{
	// If `get_ascii_z` had to allocate a buffer, free it here
	return 0;
}

int HLECall21(tRME_State* State, int IntNum)
{
	if( IntNum != 0x21 ) {
		// EXTENDED MEMORY SPECIFICATION (XMS) v2+ - INSTALLATION CHECK
		if( IntNum == 0x2F && State->AX.W == 0x4300 ) {
			// Pretend that it doesn't exist
			return 0;
		}
		FatalErrorF(State, "Unhandled DOS TSR driver call: INT 0x%02x\n", IntNum);
	}
	static FILE* sDosFileTable[256];
	int ret;
	switch(State->AX.B.H)
	{
	case 0x25:	// SET INTERRUPT VECTOR
		PrintDebugF(State, "DOS SET_INTERRUPT_VECTOR #%i = %04x:%04x\n", State->AX.B.L, State->ES, State->BX.W);
		*((uint16_t*)State->Memory[0] + State->AX.B.L * 2 + 1) = State->ES;
		*((uint16_t*)State->Memory[0] + State->AX.B.L * 2) = State->BX.W;
		break;
	case 0x30:	// GET DOS VERSION
		PrintDebugF(State, "DOS GET_DOS_VERSION BL=%i\n", State->BX.B.L);
		// Pretend to be 5.0
		State->AX.B.H = 5;
		State->AX.B.L = 0;
		State->BX.B.H = 0;
		break;
	case 0x35:	// GET INTERRUPT VECTOR
		PrintDebugF(State, "DOS GET_INTERRUPT_VECTOR #%i\n", State->AX.B.L);
		State->ES = *((uint16_t*)State->Memory[0] + State->AX.B.L * 2 + 1);
		State->BX.W = *((uint16_t*)State->Memory[0] + State->AX.B.L * 2);
		break;
	case 0x3D: {	// OPEN - OPEN EXISTING FILE
		const char* path;
		int ret = get_ascii_z(State, State->DS, State->DX.W, &path);
		if(ret) return ret;
		PrintDebugF(State, "DOS OPEN EXISTING FILE: '%s' w/ mode=0x%x\n", path, State->AX.B.L);
		if( State->AX.B.L != 0 ) {
			FatalErrorF(State, "TODO: Handle non-read open modes");
		}
		// Skip first three, they're special
		for(int i = 3; i < sizeof(sDosFileTable)/sizeof(sDosFileTable[0]); i++) {
			if(!sDosFileTable[i]) {
				sDosFileTable[i] = fopen(path, "rb");
				if(!sDosFileTable[i]) {
					State->Flags |= FLAG_CF;
					State->AX.W = 2;	// File not found
					return 0;
				}
				else {
					State->Flags &= ~FLAG_CF;
					State->AX.W = i;
					return 0;
				}
			}
		}
		State->Flags |= FLAG_CF;
		State->AX.W = 4;	// Too many files
		return 0;
		} break;
	case 0x3e:
		PrintDebugF(State, "DOS CLOSE FILE: %i\n", State->BX.W);
		if( State->BX.W < sizeof(sDosFileTable)/sizeof(sDosFileTable[0]) && sDosFileTable[State->BX.W] ) {
			fclose(sDosFileTable[State->BX.W]);
			sDosFileTable[State->BX.W] = NULL;
			State->Flags &= ~FLAG_CF;
			State->AX.W = 0;
		}
		else {
			State->Flags |= FLAG_CF;
			State->AX.W = 2;	// File not found
		}
		return 0;
	case 0x40: {	// WRITE TO FILE OR DEVICE
		uint16_t file_handle = State->BX.W;
		uint16_t len = State->CX.W;
		struct sRME_MemRef	ptrs;
		if(ret = RME_GetPtr(State, State->DS, State->DX.W, len, &ptrs))	return ret;
		PrintDebugF(State, "DOS WRITE to %i: \"", file_handle);
		for(size_t i = 0; i < len; i++) {
			uint8_t c = i < ptrs.len_1
				? ((uint8_t*)ptrs.range_1)[i]
				: ((uint8_t*)ptrs.range_2)[i - ptrs.len_1];
			if(0x20 <= c && c < 0x7F) {
				PrintDebugF(State, "%c", c);
			}
			else {
				PrintDebugF(State, "\\x%02x", c);
			}
		}
		PrintDebugF(State, "\"\n");
		exit(1);
		}
	case 0x4a:
		PrintDebugF(State, "DOS RESIZE MEMORY BLOCK: Seg %04x to %04x paragraphs\n", State->ES, State->BX.W);
		// 0x0FF0 is the loaded binary, indicate that it can grow to 0xA0000
		if(State->ES == DESTINATION_SEG) {
			State->AX.W = 0;
			State->BX.W = 0xA000 - 0x0FF0;
			State->Flags &= ~FLAG_CF;
			return 0;
		}
		exit(1);
	case 0x44:
		switch(State->AX.W)
		{
		case 0x4400:
			PrintDebugF(State, "DOS IOCTL - GET DEVICE INFORMATION: Handle %i\n", State->BX.W);
			if(State->BX.W == 0) {
				State->AX.W = 0;
				State->DX.W = 0x81;	// character device, stdin
				State->Flags &= ~FLAG_CF;
				return 0;
			}
			if(State->BX.W == 1) {
				State->AX.W = 0;
				State->DX.W = 0x82;	// character device, stdout
				State->Flags &= ~FLAG_CF;
				return 0;
			}
			if(State->BX.W < sizeof(sDosFileTable)/sizeof(sDosFileTable[0])) {
				if( sDosFileTable[State->BX.W] ) {
					State->AX.W = 0;
					State->DX.W = 0;
					State->Flags &= ~FLAG_CF;
					return 0;
				}
			}
			FatalErrorF(State, "TODO 0x21 AX=0x%04x w/ Handle %i\n", State->AX.W, State->BX.W);
			break;
		default:
			PrintDebugF(State, "HLE Call INT 0x21 AX=0x%04x unknown\n", State->AX.W);
			RME_DumpRegs(State);
			exit(1);
		}
	case 0x56: {
		const char* src;
		if(ret = get_ascii_z(State, State->DS, State->DX.W, &src)) return ret;
		const char* dst;
		if(ret = get_ascii_z(State, State->ES, State->DI.W, &dst)) return ret;
		PrintDebugF(State, "DOS RENAME FILE: '%s' -> '%s'\n", src, dst);
		exit(1);
		} break;
	default:
		PrintDebugF(State, "HLE Call INT 0x21 AH=0x%02x unknown\n", State->AX.B.H);
		RME_DumpRegs(State);
		exit(1);
	}
	return 0;
}