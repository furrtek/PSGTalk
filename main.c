// PSGTalk 0.3
// Generates PSG (SN76489) update streams from speech wave files
// to mimic voice with square tones
// Furrtek 2017

// TODO: Noise channel ?
// TODO: Log/linear is wrong
// TODO: "consolidate" sucks
// TODO: Improve quality on real hardware
//       Compare SMS/Coleco/NGP output with simulation
// TODO: Harmonics discrimination adaptation/parameter
// TODO: Handle frenquencies too low
// TODO: Handle more channels
// TODO: Register streams for each system
// TODO: Brute force spectrum matching ?
// TODO: Compression ?
// TODO: Psychoacoustic model ?

#include "main.h"

char * modestr[5] = {"raw", "ntsc", "pal", "vgm", "ngp"};

char lintolog(float in) {
	float result;
    
    // So wrong it hurts...
    
    result = in * 4;
    if (result > 15) result = 15;
    if (result < 0) result = 0;
    return result;
}

int main(int argc, char *argv[]) {
	// mallocated stuff
	float * wave_buffer;
	float * aa_buffer;
	float * work_buffer;
	float * frame_buffer;
	float * dft_bins;
	channels_t * frequencies;
	channels_t * volumes;
	unsigned char * out_buffer;
	
	unsigned char consolidate;
	unsigned long file_length, wave_size, work_size;
	unsigned int frame_size, frame_inc;
	unsigned int i, ofs;
	
	unsigned long c, m, k, t, f;
	float kmax;
	float max_power;
	float in_r, sum_r, sum_i;
	unsigned long psg_freq;
	float ratio;
	unsigned long frame_idx = 0, frames;
	char outfilepath[256];
	unsigned long idx;
	unsigned long data_idx;
	unsigned int data_word;
	unsigned int ext;
	unsigned int volume;
	
	puts("PSGTalk 0.3 - furrtek 2017\n");
	
	// Overlap = 0
	//   Data: ######################## (24)
	// Frames: AAAAAAAA
	//                 BBBBBBBB
	//                         CCCCCCCC
	//  Count: 3
	// Length: 8 = 24 / 3 * (0 + 1)
	// Offset: 8 = Length * (1 - 0)
	
	// Overlap = 0.5
	//   Data: ######################## (24)
	// Frames: AAAAAAAAAAAA
	//               BBBBBBBBBBBB
	//                     CCCCCCCCCCCC
	//  Count: 3
	// Length: 12 = 24 / 3 * (0.5 + 1)
	// Offset: Length * (1 - 0.5)
	
	// Defaults
	overlap = 0.5;
	updates_per_frame = 2;
	psg_channels = 3;
	sim = 0;
	mode = MODE_NTSC;
	
	if (parse_args(argc, argv))
		return 1;

	wave_size = load_wav(argv[argc - 1], wave_buffer);
	if (!wave_size) {
		puts("Can't load wave file.\n");
		return 1;
	}
	
	if (mode == MODE_PAL)
		fps = 50;
	else
		fps = 60;
	
	// Anti-alias filtering
	aa_buffer = (float *)malloc(wave_size);
	if (aa_buffer == NULL) {
		puts("Memory allocation failed\n");
		free(wave_buffer);
		return 0;
	}
	lowpass(wave_buffer, aa_buffer, 8192.0, samplerate_in, wave_size);
	
	// Decimate
	ratio = ((float)samplerate_in / 8192.0);
	work_size = wave_size / (int)ratio;
	work_buffer = malloc(work_size * sizeof(unsigned char));
	if (work_buffer == NULL) {
		puts("Memory allocation failed\n");
		free(wave_buffer);
		free(aa_buffer);
		return 0;
	}
	for (i = 0; i < work_size; i++)
		work_buffer[i] = aa_buffer[(int)(i * ratio)];

	update_rate = fps * updates_per_frame;
	frame_size = (8192.0 * (overlap + 1.0)) / update_rate;
	frame_inc = (float)frame_size * (1.0 - overlap);
	m = 1;									// Discrimination between power peaks
	//n = frame_size - 1;
	frames = (work_size / frame_size);		// Total number of frames
	freq_step = (8192 / freq_res) / 2;
	
	frame_buffer = malloc(frame_size * sizeof(float));
	dft_bins = malloc(freq_res * sizeof(float));
	frequencies = malloc(frames * sizeof(channels_t) * psg_channels);
	volumes = malloc(frames * sizeof(channels_t) * psg_channels);
	
	if ((window_lut == NULL) || (dft_bins == NULL) || (frame_buffer == NULL) || (frequencies == NULL) || (volumes == NULL)) {
		puts("Memory allocation failed\n");
		return 1;
	}
	
	if (!make_LUTs(frame_size)) {
		puts("Table generation failed\n");
		return 1;
	}
	
	// Show recap
	printf("Frames: %lu\n", frames);
	printf("Overlap: %u%%\n", (unsigned int)(overlap * 100.0));
	printf("Samplerate: %luHz -> %u\n", samplerate_in, 8192);
	printf("Resolution: %uHz\n", freq_step);
	printf("Update rate: %u/frame (%uHz @ %ufps)\n", updates_per_frame, update_rate, fps);
	printf("PSG channels: %u\n", psg_channels);
	printf("Mode: %s\n", modestr[mode]);
	
	f = 0;
	do {
		// Copy frame
		memcpy(frame_buffer, &work_buffer[f], frame_size * sizeof(float));
		
		// Apply window
		for (t = 0; t < frame_size; t++)
			frame_buffer[t] *= window_lut[t];
		
		// Do DFT
    	for (k = 0; k < freq_res; k++) {		// For each output element
  	      	sum_r = 0;
  	      	sum_i = 0;
        	for (t = 0; t < frame_size; t++) {			// For each input element
            	c = ((32768 * t * k / 2) / frame_size) & 0x7FFF;
            	in_r = frame_buffer[t];
            	sum_r += (in_r * cos_lut[c]);
            	sum_i += (in_r * sin_lut[c]);
			}
            sum_r /= frame_size;
        	sum_i /= frame_size;
        
        	dft_bins[k] = sqrt((sum_i * sum_i) + (sum_r * sum_r));
    		//if (framei == 1) printf("SAMP=%u POW=%f (%uHz)\n", workbuffer[k + f], pow[k], k*freq_step);
		}

		// Find highest peaks
		consolidate = 0;
		for (c = 0; c < psg_channels; c++) {
			if (consolidate) {
				max_power -= 5;
	    		frequencies[frame_idx].ch[c] = frequencies[frame_idx].ch[c - 1];
	    		volumes[frame_idx].ch[c] = lintolog(max_power);
			} else {
				// Find highest power and its associated frequency (skip DC bin)
		    	max_power = 0;
		    	for (k = 1; k < (frame_size - 1); k++) {
					if (dft_bins[k] > max_power) {
						max_power = dft_bins[k];
						kmax = k;
					}
				}
				// Clear surrounding bins if needed
				for (t = 0; t < m; t++) {
					ofs = kmax - t - 1;
					if (ofs) dft_bins[ofs] = 0;
					ofs = kmax + t + 1;
					if (ofs < freq_res) dft_bins[ofs] = 0;	
				}
				
				//printf("(%fHz)\n", kmax/2*freq_step);
		    	frequencies[frame_idx].ch[c] = kmax;
		    	volumes[frame_idx].ch[c] = lintolog(max_power);
			}
			
			consolidate = (max_power >= 5) ? 1 : 0;
		}
        
    	frame_idx++;
    	f += frame_inc;
    
    	printf("\rComputing frame %lu/%lu.", frame_idx, frames);

	} while (f < (work_size - frame_size));
	
	puts(" Done.\n");
	
	// Generate output file
	strcpy(outfilepath, argv[argc - 1]);
	ext = strlen(outfilepath) - 3;
	if (mode == MODE_VGM) {
		outfilepath[ext++] = 'v';
		outfilepath[ext++] = 'g';
		outfilepath[ext] = 'm';
	} else {
		outfilepath[ext++] = 'b';
		outfilepath[ext++] = 'i';
		outfilepath[ext] = 'n';
	}


	FILE * fo = fopen(outfilepath, "wb");
	
	// Todo: differenciate MODE (PSG MODE) and OUTPUT MODE !
	if (mode == MODE_RAW) {
		out_raw(out_buffer, frequencies, volumes, frame_idx, freq_res);
	} else if (mode == MODE_VGM) {
		out_vgm(out_buffer, frequencies, volumes, frame_idx);
	} else {
		if (mode == MODE_NTSC) psg_freq = 111861 * 2;
		if (mode == MODE_PAL) psg_freq = 110841 * 2;
		if (mode == MODE_NGP) psg_freq = 96000 * 2;
		
		file_length = ((frame_idx * psg_channels * 2) + 1);
		out_buffer = malloc(file_length * sizeof(unsigned char));
		/*if (out_buffer == NULL) {
			puts("Memory allocation failed\n");
			return 1;
		}*/
		
		// For each frame
		data_idx = 0;
		for (f = 0; f < frame_idx - 1; f++) {
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
	}
	
	fwrite(out_buffer, file_length, 1, fo);
	fclose(fo);

	free(out_buffer);
	
	puts("Output file written.");
	printf("Size: %lukB\n", file_length / 1024);
	
	// Generate simulation file if needed
	if (sim) {
		if (gensim(frame_size, frame_idx, psg_channels, frequencies, volumes)) puts("\nSimulation file written.\n");
	}
	
	free(frequencies);
	free(volumes);
	free(frame_buffer);
	free(wave_buffer);
	free(work_buffer);
	free(dft_bins);
	
    //system("pause");
    //return 0;

	return 0;
}
