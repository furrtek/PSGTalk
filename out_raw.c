#include "main.h"

int out_raw(unsigned char * out_buffer, unsigned int const * frequencies, unsigned int const * volumes, const unsigned int n) {
	unsigned long file_length, framei = 0;
	unsigned long idx;
	unsigned long data_idx = 0;
	
	int c, f;
	
	if (n < 256) {
		file_length = ((framei * psg_channels * 2) + 1);			// Frequency(byte)/volume pairs
		out_buffer = malloc(file_length * sizeof(unsigned char));
		if (out_buffer == NULL)
			return 0;
		
		// For each frame
		for (f = 0; f < framei - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				out_buffer[data_idx++] = frequencies[idx];
				out_buffer[data_idx++] = volumes[idx];
			}
		}
	} else {
		file_length = ((framei * psg_channels * 3) + 1);			// Frequency(word)/volume pairs
		out_buffer = malloc(file_length * sizeof(unsigned char));
		if (out_buffer == NULL)
			return 0;
		
		// For each frame
		for (f = 0; f < framei - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				out_buffer[data_idx++] = frequencies[idx] >> 8;
				out_buffer[data_idx++] = frequencies[idx] & 0xFF;
				out_buffer[data_idx++] = volumes[idx];
			}
		}
	}
	
	return 1;
}
