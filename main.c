// PSGTalk 0.3
// Generates PSG (SN76489) update streams from speech wave files
// to mimic voice with square tones
// Furrtek 2017

// TODO: Overlap
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
	float * pow;
	unsigned int * freqs;
	unsigned int * vols;
	
	unsigned char * out_buffer;
	
	unsigned char consolidate;
	unsigned long file_length, wave_size, work_size;
	unsigned int frame_size, n;
	unsigned int i;
	
	int c, m, k, t, f = 0;
	float kmax, akmax, akmin, bkmax, bkmin;
	float powmax;
	float in_r, sum_r, sum_i;
	unsigned long psg_freq;
	float ratio;
	int framei = 0, frames;
	char outfilepath[256];
	int ext;
	int granu;
	int datword;
	int volume;
	
	puts("PSGTalk 0.3 - furrtek 2017\n");
	
	// Defaults
	update_rate = 2;
	psg_channels = 3;
	sim = 0;
	mode = MODE_NTSC;
	
	if (parseargs(argc, argv))
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

	frame_size = 8192 / (fps * update_rate);		// Frame size
	m = 1;									// Discrimination between power peaks
	n = frame_size - 1;
	frames = (work_size / frame_size);		// Total number of frames
	granu = (8192 / n) / 2;
	
	frame_buffer = malloc(n * sizeof(float));
	pow = malloc(n * sizeof(float));
	freqs = malloc(frames * sizeof(int) * psg_channels);
	vols = malloc(frames * sizeof(int) * psg_channels);
	
	if ((window_lut == NULL) || (pow == NULL) || (frame_buffer == NULL) || (freqs == NULL) || (vols == NULL)) {
		puts("Memory allocation failed\n");
		return 1;
	}
	
	if (!make_LUTs(n)) {
		puts("Table generation failed\n");
		return 1;
	}
	
	// Show recap
	printf("Frames: %u\n", frames);
	printf("Samplerate: %luHz -> %u\n", samplerate_in, 8192);
	printf("Resolution: %uHz\n", granu);
	printf("Update rate: %u/frame (%uHz @ %ufps)\n", update_rate, fps * update_rate, fps);
	printf("PSG channels: %u\n", psg_channels);
	printf("Mode: %s\n", modestr[mode]);
	printf("Bitrate: %ubps\n\n", update_rate * fps * 2 * 8);
		
	do {
		// Copy frame
		memcpy(frame_buffer, &work_buffer[f], n * sizeof(float));
		
		// Apply window
		for (t = 0; t < n; t++)
			frame_buffer[t] *= window_lut[t];
		
		// Do DFT
    	for (k = 0; k < n; k++) {			// For each output element
  	      	sum_r = 0;
  	      	sum_i = 0;
        	for (t = 0; t < n; t++) {		// For each input element
            	c = (unsigned long)(32768 * t * ((float)k / 2) / n) & 0x7FFF;
            	in_r = frame_buffer[t];
            	sum_r += (in_r * cos_lut[c]);
            	sum_i += (in_r * sin_lut[c]);
			}
            sum_r /= n;
        	sum_i /= n;
        
        	pow[k] = sqrt((sum_i * sum_i) + (sum_r * sum_r));
    		//if (framei == 1) printf("SAMP=%u POW=%f (%uHz)\n", workbuffer[k + f], pow[k], k*granu);
		}

		// Find highest power and its associated frequency (skip DC)
    	powmax = 0;
    	for (k = 1; k < (n - 1); k++) {
			if (pow[k] > powmax) {
				powmax = pow[k];
				kmax = k;
			}
		}
		//printf("(%fHz)\n", kmax/2*granu);
    	freqs[framei * update_rate] = kmax;
    	vols[framei * update_rate] = lintolog(powmax);
		consolidate = (powmax >= (20/4)) ? 1 : 0;
    
    	if (psg_channels > 1) {
			if (consolidate) {
				powmax -= (20/4);
	    		freqs[(framei*psg_channels)+1] = freqs[framei*psg_channels];
	    		vols[(framei*psg_channels)+1] = lintolog(powmax);
	    		consolidate = (powmax >= (20/4)) ? 1 : 0;
			} else {
	    		akmax = kmax + m;
	    		akmin = kmax - m;
	    		
	    		// Find the second highest power and its associated frequency
		   	 	powmax = 0;
		    	for (k=1; k<(n-1); k++) {
		        	if ((k > akmax) || (k < akmin)) {
		            	if (pow[k] > powmax) {
		                	powmax = pow[k];
		                	kmax = k;
						}
					}
				}
		    	freqs[(framei*psg_channels)+1] = kmax;
		    	vols[(framei*psg_channels)+1] = lintolog(powmax);
			}
	    	
			if (psg_channels > 2) {
				if (consolidate) {
					powmax -= (20/4);
    				freqs[(framei*psg_channels)+2] = freqs[(framei*psg_channels)+1];
    				vols[(framei*psg_channels)+2] = lintolog(powmax);
				} else {
					bkmax = kmax + m;
					bkmin = kmax - m;
    
		    		// Find the third highest power and its associated frequency
			    	powmax = 0;
			    	for (k=1; k<(n-1); k++) {
			        	if ((k > akmax) || (k < akmin)) {
							if ((k > bkmax) || (k < bkmin)) {
			            		if (pow[k] > powmax) {
			                		powmax = pow[k];
			                		kmax = k;
								}
							}
						}
					}
			    	freqs[(framei*psg_channels)+2] = kmax;
			    	vols[(framei*psg_channels)+2] = lintolog(powmax);
				}
			}
		}
        
    	framei++;
    	f += frame_size;
    
    	printf("\rComputing frame %u/%u.", framei, frames);

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
		out_raw(out_buffer, freqs, vols, n);
	} else if (mode == MODE_VGM) {
		out_vgm(out_buffer, freqs, vols);
	} else {
		if (mode == MODE_NTSC) psg_freq = 111861*2;
		if (mode == MODE_PAL) psg_freq = 110841*2;
		if (mode == MODE_NGP) psg_freq = 96000*2;
		
		file_length = ((framei * psg_channels * 2) + 1);
		out_buffer = malloc(file_length * sizeof(unsigned char));
		/*if (out_buffer == NULL) {
			puts("Memory allocation failed\n");
			return 1;
		}*/
		
		// For each frame
		for (f=0; f<framei-1; f++) {
			// For each channel
			for (c=0; c<psg_channels; c++) {
				datword = psg_freq / ((float)freqs[(f*psg_channels)+c] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[(f*psg_channels)+c];
				if (volume > 15) volume = 15;
				datword |= (volume << 10);
				out_buffer[(f*psg_channels*2)+(c*2)] = datword / 256;
				out_buffer[(f*psg_channels*2)+(c*2)+1] = datword & 255;
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
		if (gensim(frame_size, framei, psg_channels, freqs, vols)) puts("\nSimulation file written.\n");
	}
	
	free(freqs);
	free(vols);
	free(frame_buffer);
	free(wave_buffer);
	free(work_buffer);
	free(pow);
	
    //system("pause");
    //return 0;

	return 0;
}
