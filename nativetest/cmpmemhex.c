#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

int main(int argc, char *argv[])
{
	if( argc != 3 ) {
		fprintf(stderr, "Usage: %s <template> <have>\n", argv[0]);
		fprintf(stderr, "Compares a template hex header with a binary memory dump\n");
		return 1;
	}

	FILE	*hexfile = fopen(argv[1], "rb");
	if( !hexfile ) {
		perror("Hex File");
		fprintf(stderr, "Can't open template file '%s'\n", argv[1]);
		return 1;
	}
	FILE	*binfile = fopen(argv[2], "rb");
	if( !binfile ) {
		perror("Binary File");
		fprintf(stderr, "Can't open file to check '%s'\n", argv[1]);
		return 1;
	}
	
	fseek(binfile, 0, SEEK_END);
	int bytes = ftell(binfile);
	fseek(binfile, 0, SEEK_SET);
	
	int	failed = 0;
	size_t	n_bytes = 0;
	char	line[512];
	while( fgets(line, sizeof(line)-1, hexfile) )
	{
		size_t	line_len = strlen(line);
		if( line[0] != ';' || line[1] == ' ' )
			break;

		int ofs = 1;
		assert(isxdigit(line[ofs++]));
		assert(isxdigit(line[ofs++]));
		if( line[ofs] == 'h' )
			ofs ++;
		assert(line[ofs++] == ':');

		bool	exp_mask[16];
		uint8_t	exp_bytes[16];
		uint8_t	have_bytes[16];

		// Expect ';\x\x: (\x\x|XX )*'
		for( int i = 0; i < 16; i ++ )
		{
			while( isspace(line[ofs]) )
				ofs ++;
			if( line[ofs] == '\0' ) {
				exp_mask[i] = false;
			}
			else if( line[ofs] == 'X' ) {
				// ignore
				exp_mask[i] = false;
				ofs += 2;
			}
			else {
				exp_mask[i] = true;
				exp_bytes[i] = strtol(line+ofs, NULL, 16);
				ofs += 2;
			}
		}

		bool	local_failed = false;
		for( int i = 0; i < 16; i ++ )
		{
			have_bytes[i] = fgetc(binfile);
			local_failed |= (exp_mask[i] && have_bytes[i] != exp_bytes[i]);
		}
		
		if( local_failed )
		{
			fprintf(stderr, "%02x:", n_bytes);
			for( int i = 0; i < 16; i ++ )
			{
				if( i == 8 )	fprintf(stderr, " ");
				fprintf(stderr, " %02x", have_bytes[i]);
			}
			fprintf(stderr, " !=");
			for( int i = 0; i < 16; i ++ )
			{
				if( i == 8 )	fprintf(stderr, " ");
				if( exp_mask[i] )
					fprintf(stderr, " %02x", exp_bytes[i]);
				else
					fprintf(stderr, " XX");
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "   ");
			for( int i = 0; i < 16; i ++ )
			{
				if( i == 8 )	fprintf(stderr, " ");
				if( exp_mask[i] && have_bytes[i] != exp_bytes[i] )
					fprintf(stderr, " ^ ");
				else
					fprintf(stderr, "   ");
			}
			for( int i = 0; i < 16; i ++ )
			{
				if( i == 8 )	fprintf(stderr, " ");
				if( exp_mask[i] && have_bytes[i] != exp_bytes[i] )
					fprintf(stderr, " %02x", have_bytes[i]);
				else
					fprintf(stderr, "   ");
			}
			fprintf(stderr, "\n");
			failed = 1;
		}
		n_bytes += 16;
	}

	return failed;
}

// vim: ts=4 noexpandtab sw=4
