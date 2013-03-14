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

#define DESTINATION_SEG	0x8000

// === PROTOTYPES ===
t_farptr	LoadDosExe(tRME_State *state, const char *file);

// === CODE ===
t_farptr LoadDosExe(tRME_State *state, const char *file)
{
	tExeHeader	hdr;
	t_farptr	ret = {0,0};
	FILE	*fp;
	 int	dataStart, dataSize;
	 int	relocStart, i;
	void	*data;
	
	fp = fopen(file, "rb");
	if(!fp) {
		printf("File '%s' does not exist\n", file);
		return ret;
	}
	fread(&hdr, sizeof(tExeHeader), 1, fp);
	
	// Sanity check signature
	if(hdr.signature != 0x5A4D) {
		printf("DOS EXE header is invalid (%04x)\n", hdr.signature);
		return ret;
	}
	
	dataStart = hdr.header_paragraphs*16;
	dataSize = hdr.blocks_in_file*512 - dataSize;
	relocStart = hdr.reloc_table_offset;
	printf("hdr.cs = %x, hdr.ip = %x\n", hdr.cs, hdr.ip);
	
	fseek(fp, dataStart, SEEK_SET);
	data = malloc(dataSize);
	fread(data, dataSize, 1, fp);
	
	// Relocate
	fseek(fp, relocStart, SEEK_SET);
	for( i = 0; i < hdr.num_relocs; i++ )
	{
		tExeReloc	reloc;
		fread(&reloc, sizeof(tExeReloc), 1, fp);
		*(uint16_t*)(data + reloc.segment*16 + reloc.offset) += DESTINATION_SEG;
	}

	 int	base = (DESTINATION_SEG*16) / RME_BLOCK_SIZE;
	if( (DESTINATION_SEG*16) % RME_BLOCK_SIZE != 0 ) {
		memcpy(&state->Memory[base], data, MIN(RME_BLOCK_SIZE, dataSize));
		i ++;
	}
	for( i = 1; i < (DESTINATION_SEG*16+dataSize)/RME_BLOCK_SIZE; i ++ )
	{
		memcpy(&state->Memory[base+i], data+i*RME_BLOCK_SIZE, dataSize-i*RME_BLOCK_SIZE);
	}

	free(data);
	
	fclose(fp);

	return ret;
}
