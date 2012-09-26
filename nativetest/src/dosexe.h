/*
 * 
 */
#ifndef _DOSEXE_H_
#define _DOSEXE_H_

/*
 * FORMAT:
 *  Header
 *  n Relocation Entries
 *  Code/Data
 */

typedef struct sExeHeader
{
	uint16_t	signature; /* == 0x5a4D */
	uint16_t	bytes_in_last_block;
	uint16_t	blocks_in_file;
	uint16_t	num_relocs;
	uint16_t	header_paragraphs;
	uint16_t	min_extra_paragraphs;
	uint16_t	max_extra_paragraphs;
	uint16_t	ss;
	uint16_t	sp;
	uint16_t	checksum;
	uint16_t	ip;
	uint16_t	cs;
	uint16_t	reloc_table_offset;
	uint16_t	overlay_number;
}	tExeHeader;

typedef struct sExeReloc {
	uint16_t	offset;
	uint16_t	segment;
}	tExeReloc;

#endif
