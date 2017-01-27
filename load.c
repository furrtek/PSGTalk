#include "main.h"

int load_wav(const char * filename, float ** buffer) { 
	unsigned long size, i;
	short channels;
	short bits;
	
	FILE * f = fopen(filename, "rb");
	if (f == NULL)
		return 0;
	
	fseek(f, 0, SEEK_END);
	size = ftell(f) - 44;
	
	fseek(f, 22, SEEK_SET);
	fread(&channels, sizeof(short), 1, f);
	if (channels != 1) return 0;
	
	fseek(f, 24, SEEK_SET);
	fread(&samplerate_in, sizeof(long), 1, f);
	
	fseek(f, 34, SEEK_SET);
	fread(&bits, sizeof(short), 1, f);
	if (bits != 8) return 0;
	
	fseek(f, 44, SEEK_SET);
	
	*buffer = calloc(size, sizeof(float));
	
	if (*buffer == NULL)
		return 0;
	
	// Load and normalize
	for (i = 0; i < size; i++)
		(*buffer)[i] = (float)fgetc(f) / 128.0;

	fclose(f);
	
	return size;
}
