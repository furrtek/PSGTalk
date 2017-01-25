#include "main.h"

int out_vgm(unsigned char * out_buffer, channels_t const * frequencies, channels_t const * volumes,
	const unsigned long frame_count) {
	
	unsigned int c;
	unsigned long f, file_length;
	unsigned long idx;
	wait_t wait_mode;
	unsigned long wait_n;
	unsigned long psg_freq, psg_internal;
	unsigned long data_idx = 64;	// Skip header for now
	unsigned int volume;
	unsigned int data_word;
	
	unsigned long vgm_header[16] = {
		0x206D6756,		// "Vgm "
		0x00000000,		// EOF
		0x00000110,		// Version
		0x00000000,		// SN76489 clock
		0x00000000,
		0x00000000,		// GD3 offset
		0x00000000,		// Sample count
		0x00000000,
		0x00000000,
		0x00000000,		// Rate
		0x00100900,		// LFSR type
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000
	};
	
	if (mode == MODE_NTSC) {
		psg_freq = 3579545;
		wait_mode = WAIT_NTSC;
		wait_n = 735;
	} else if (mode == MODE_PAL) {
		psg_freq = 3546893;
		wait_mode = WAIT_PAL;
		wait_n = 882;
	}
	
	if (update_rate > 1) {
		wait_mode = WAIT_N;
		wait_n = psg_freq / update_rate;
		file_length = (frame_count + (frame_count * psg_channels * 7) + 1) + 64;		// Wrong ?
	} else
		file_length = (frame_count + (frame_count * psg_channels * 9) + 1) + 64;		// Wrong ?
	
	out_buffer = malloc(file_length * sizeof(unsigned char));
	if (out_buffer == NULL)
		return 0;
	
	psg_internal = psg_freq / 16;
	
	// For each frame
	for (f = 0; f < frame_count - 1; f++) {
		// For each channel
		for (c = 0; c < psg_channels; c++) {
			idx = (f * psg_channels) + c;
			
			// Todo: Make frequency LUT ?
			data_word = psg_internal / (frequencies[idx].ch[c] * freq_step);
			if (data_word > 1023) data_word = 1023;
			
			volume = volumes[idx].ch[c];
			if (volume > 15) volume = 15;
			volume = 15 - volume;
			
			out_buffer[data_idx++] = 0x50;		// VGM "PSG write"
			out_buffer[data_idx++] = 0x90 | (c << 5) | volume;
			out_buffer[data_idx++] = 0x50;		// VGM "PSG write"
			out_buffer[data_idx++] = 0x80 | (c << 5) | (data_word & 0x0F);
			out_buffer[data_idx++] = 0x50;		// VGM "PSG write"
			out_buffer[data_idx++] = data_word >> 4;
		}
		
		if (wait_mode == WAIT_NTSC)
			out_buffer[data_idx++] = 0x62;	// VGM "NTSC wait"
		else if (wait_mode == WAIT_NTSC)
			out_buffer[data_idx++] = 0x63;	// VGM "PAL wait"
		else if (wait_mode == WAIT_N) {
			out_buffer[data_idx++] = 0x61;	// VGM "Wait n samples"
			out_buffer[data_idx++] = wait_n & 0xFF;
			out_buffer[data_idx++] = wait_n >> 8;
		}
	}
	
	// Make header
	vgm_header[1] = file_length;
	vgm_header[3] = psg_freq;
	vgm_header[6] = (frame_count - 1) * wait_n;
	memcpy(out_buffer, vgm_header, 64);
	
	//free(out_buffer);
	
	return 1;
}
