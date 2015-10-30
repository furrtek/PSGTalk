#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int loadwav(const char *filename, unsigned char **result) { 
	int size = 0;
	
	FILE *f = fopen(filename, "rb");
	if (f == NULL) { 
		*result = NULL;
		return 0;
	} 
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size);
	if (size != fread(*result, sizeof(unsigned char), size, f)) { 
		free(*result);
		return 0;
	} 
	fclose(f);
	return size;
}
