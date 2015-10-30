#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int loadwav(const char *filename, unsigned char **result) { 
	int size = 0;
	short wavechs;
	short wavebits;
	
	FILE *f = fopen(filename, "rb");
	if (f == NULL) { 
		*result = NULL;
		return 0;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f) - 44;
	fseek(f, 22, SEEK_SET);
	fread(&wavechs, sizeof(short), 1, f);
	if (wavechs != 1) return 0;
	fseek(f, 24, SEEK_SET);
	fread(&samplerate, sizeof(long), 1, f);
	fseek(f, 34, SEEK_SET);
	fread(&wavebits, sizeof(short), 1, f);
	if (wavebits != 8) return 0;
	fseek(f, 44, SEEK_SET);
	*result = (char *)malloc(size);
	if (size != fread(*result, sizeof(unsigned char), size, f)) { 
		free(*result);
		return 0;
	} 
	fclose(f);
	return size;
}
