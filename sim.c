#include "main.h"

int square(int a) {
	a &= 0xFF;
	if (a > 0x7F)
		return 1;
	else
		return -1;
}

int gensim(const unsigned int frame_size, const unsigned long frame_count, const char channels,
	unsigned int const * frequencies, unsigned int const * volumes) {
	
	unsigned long size;
	unsigned long f, s;
	unsigned int c;
	unsigned char * wave_out;
	unsigned long data_idx = 0;
	unsigned long idx;
	int mix;
	unsigned char b;
	float adjust;
	
	size = (frame_count * frame_size) + 1;
		return 1;
	wave_out = malloc(size * sizeof(unsigned char));
	if (wave_out == NULL)
		return 0;

	// For each frame
	for (f = 0; f < frame_count - 1; f++) {
		// Generate samples with dirty square wave algorithm
		for (s = 0; s < frame_size; s++) {
			adjust = s / frame_size / 2;
			mix = 0;
			// Certainly wrong
			for (c = 0; c < channels; c++) {
				idx = (f * channels) + c;
            	mix += 64 * (square(0xFF * frequencies[idx] * adjust) * attenuation_lut[(int)volumes[idx]]);
			}
            b = (unsigned char)mix;
            wave_out[data_idx++] = b;
		}
	}
	
	FILE * fsim = fopen("psgtalk.raw", "wb");
	fwrite(wave_out, size, 1, fsim);
	fclose(fsim);
	
	free(wave_out);
	
	return 1;
}
