#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
	if( argc != 3 ) {
		fprintf(stderr, "Usage: %s <template> <have>\n", argv[0]);
		fprintf(stderr, "Compares a template file with a binary memory dump\n");
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
	for( int i = 0; i < bytes; i ++ )
	{
		uint8_t have = fgetc(binfile);
		uint8_t	want = fgetc(hexfile);
		
		if( feof(hexfile) ) {
			printf("End of template at %x\n", i);
			break;
		}
		if( want != have ) {
			fprintf(stderr, "Mismatch at 0x%x (%i) - exp %2X got %2X\n", i, i, want, have);
			failed = 1;
		}
	}

	return failed;
}
