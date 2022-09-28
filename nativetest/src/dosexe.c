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

#define DESTINATION_SEG	0x100

inline unsigned int MIN(unsigned int a, unsigned int b) {return (a < b)?a:b;}

// === CODE ===
t_farptr LoadDosExe(tRME_State *state, const char *file, t_farptr *stackptr)
{
	tExeHeader	hdr;
	t_farptr	ret = {0,0};
	FILE	*fp;
	 int	dataStart, dataSize;
	 int	relocStart;
	void	*data;
	
	fp = fopen(file, "rb");
	if(!fp) {
		printf("File '%s' does not exist\n", file);
		return ret;
	}
	if( fread(&hdr, sizeof(tExeHeader), 1, fp) != 1 ) {
		perror("LoadDosExe - fread header");
		exit(-1);
	}
	
	// Sanity check signature
	if(hdr.signature != 0x5A4D) {
		printf("DOS EXE header is invalid (%04x)\n", hdr.signature);
		return ret;
	}
	
	printf("LoadDosExe: hdr.cs = %x, hdr.ip = %x\n", hdr.cs, hdr.ip);
	dataStart = hdr.header_paragraphs*16;
	dataSize = hdr.blocks_in_file * 512 - (512 - hdr.bytes_in_last_block) % 512 - dataStart;
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

	 int	base = (DESTINATION_SEG*16) / RME_BLOCK_SIZE;
	uint8_t	*readdata = data;
	int i = 0;
	if( (DESTINATION_SEG*16) % RME_BLOCK_SIZE != 0 ) {
		off_t	ofs = (DESTINATION_SEG*16) % RME_BLOCK_SIZE;
		size_t	copysize = MIN(RME_BLOCK_SIZE - ofs, dataSize);
		printf("- Partial copy 0x%x+0x%x 0x%x", base*RME_BLOCK_SIZE, ofs, copysize);
		memcpy(state->Memory[base] + ofs, readdata, copysize);
		readdata += copysize;
		dataSize -= copysize;
		i ++;
	}
	for( ; i < (dataSize + RME_BLOCK_SIZE-1)/RME_BLOCK_SIZE; i ++ )
	{
		size_t	copysize = MIN(RME_BLOCK_SIZE, dataSize-i*RME_BLOCK_SIZE);
		printf("- Full copy 0x%x : 0x%x\n", (base+i)*RME_BLOCK_SIZE, copysize);
		memcpy(state->Memory[base+i], readdata, copysize);
		readdata += copysize;
	}

	free(data);
	fclose(fp);

	ret.Segment = DESTINATION_SEG + hdr.cs;
	ret.Offset = hdr.ip;
	stackptr->Segment = DESTINATION_SEG + hdr.ss;
	stackptr->Offset = hdr.sp;

	return ret;
_error:
	fclose(fp);
	free(data);
	return ret;
}
