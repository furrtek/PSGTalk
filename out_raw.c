#include "main.h"

int out_raw(unsigned char ** out_buffer, unsigned int const * frequencies, unsigned int const * volumes,
	const unsigned long frame_count, const unsigned int n) {
	
	unsigned char * out_buffer_value;
	unsigned long idx;
	unsigned long data_idx = 0;
	unsigned int c, frequency;
	unsigned long f;
	
	if (n < 256) {
		file_length = ((frame_count * psg_channels * 2) + 1);			// Frequency(byte)/volume pairs
		*out_buffer = malloc(file_length * sizeof(unsigned char));
		if (*out_buffer == NULL)
			return 0;
		
		out_buffer_value = *out_buffer;
		
		// For each frame
		for (f = 0; f < frame_count - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				out_buffer_value[data_idx++] = frequencies[idx];
				out_buffer_value[data_idx++] = volumes[idx];
			}
		}
	} else {
		file_length = ((frame_count * psg_channels * 3) + 1);			// Frequency(word)/volume pairs
		*out_buffer = malloc(file_length * sizeof(unsigned char));
		if (*out_buffer == NULL)
			return 0;
		
		out_buffer_value = *out_buffer;
		
		// For each frame
		for (f = 0; f < frame_count - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				frequency = frequencies[idx];
				out_buffer_value[data_idx++] = frequency >> 8;
				out_buffer_value[data_idx++] = frequency & 0xFF;
				out_buffer_value[data_idx++] = volumes[idx];
			}
		}
	}
	
	return 1;
}
