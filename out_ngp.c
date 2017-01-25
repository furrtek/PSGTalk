#include "main.h"

int out_ngp(unsigned char * out_buffer, channels_t const * frequencies, channels_t const * volumes,
	const unsigned long frame_count) {
	
	unsigned int c;
	unsigned long idx;
	unsigned long f, file_length;
	unsigned long psg_freq = 96000 * 2;
	unsigned int volume;
	unsigned int data_word;
	unsigned long data_idx = 0;
	
	file_length = ((frame_count * psg_channels * 2) + 1);
	out_buffer = malloc(file_length * sizeof(unsigned char));
	if (out_buffer == NULL)
		return 0;
	
	// For each frame
	for (f = 0; f < frame_count - 1; f++) {
		// For each channel
		for (c = 0; c < psg_channels; c++) {
			idx = (f * psg_channels) + c;
		
			data_word = psg_freq / ((float)frequencies[idx].ch[c] * freq_step);
			if (data_word > 1023) data_word = 1023;
			
			volume = volumes[idx].ch[c];
			if (volume > 15) volume = 15;
			data_word |= (volume << 10);
			
			out_buffer[data_idx++] = data_word >> 8;
			out_buffer[data_idx++] = data_word & 0xFF;
		}
	}
	
	return 1;
}
