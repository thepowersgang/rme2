/*
 * Realmode Emulator - Native Tester
 * 
 * - DOS Executable Loader
 */
#include "common.h"
#include <rme.h>
#include "dosexe.h"
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
