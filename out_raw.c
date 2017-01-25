#include "main.h"

int out_raw(unsigned char * out_buffer, channels_t const * frequencies, channels_t const * volumes,
	const unsigned long frame_count, const unsigned int n) {
	
	unsigned long file_length;
	unsigned long idx;
	unsigned long data_idx = 0;
	unsigned int frequency;
	
	int c, f;
	
	if (n < 256) {
		file_length = ((frame_count * psg_channels * 2) + 1);			// Frequency(byte)/volume pairs
		out_buffer = malloc(file_length * sizeof(unsigned char));
		if (out_buffer == NULL)
			return 0;
		
		// For each frame
		for (f = 0; f < frame_count - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				out_buffer[data_idx++] = frequencies[idx].ch[c];
				out_buffer[data_idx++] = volumes[idx].ch[c];
			}
		}
	} else {
		file_length = ((frame_count * psg_channels * 3) + 1);			// Frequency(word)/volume pairs
		out_buffer = malloc(file_length * sizeof(unsigned char));
		if (out_buffer == NULL)
			return 0;
		
		// For each frame
		for (f = 0; f < frame_count - 1; f++) {
			// For each channel
			for (c = 0; c < psg_channels; c++) {
				idx = (f * psg_channels) + c;
				frequency = frequencies[idx].ch[c];
				out_buffer[data_idx++] = frequency >> 8;
				out_buffer[data_idx++] = frequency & 0xFF;
				out_buffer[data_idx++] = volumes[idx].ch[c];
			}
		}
	}
	
	return 1;
}
