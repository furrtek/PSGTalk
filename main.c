// PSGTalk 0.21
// Generates PSG (SN76489) update streams from speech wave files
// to mimic voice with square tones
// Furrtek 2015

// TODO: Overlap
// TODO: Improve quality (A LOT) on real hardware
//       Compare SMS/Coleco/NGP output with simulation
// TODO: Detect and handle different wave samplerates
// TODO: Harmonics discrimination adaptation/parameter
// TODO: Handle frenquencies too low
// TODO: Handle more channels
// TODO: Register streams for each system
// TODO: Log/linear ?
// TODO: Brute force spectrum matching ?
// TODO: Noise channel ?
// TODO: Compression ?
// TODO: Psychoacoustic model ?

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

int parseargs(int argc, char * argv[]);
int loadwav(const char *filename, unsigned char **result);
int gensim(int ws, int framei, char channels, int freqs[], int vols[]);

char * modestr[5] = {"raw", "ntsc", "pal", "vgm", "ngp"};
modetype mode = MODE_NTSC;

char lintolog(double in) {
	double result;
	
	/*in = in - 16;
	if (in < 0) in = 0;
	result = log10(in)*16;
    if (result > 15) result = 15;
    if (result < 0) result = 0;
    return result;*/
    
    result = in * 4;
    if (result > 15) result = 15;
    if (result < 0) result = 0;
    return result;
}

int main(int argc, char *argv[]) {
	unsigned char * wavebuffer;		// mallocated stuff
	unsigned char * workbuffer;
	unsigned char * window;
	unsigned char * datout;
	double * pow;
	int * freqs;
	int * vols;
	
	unsigned char consolidate;
	int size, wsize;
	long i;
	int c, ws, m, n, k, t, f = 0;
	double kmax, akmax, akmin, bkmax, bkmin;
	double powmax;
	double angle;
	double inr;
	double sumr, sumi;
	int psgfreq;
	int workrate;
	int undersample;
	int framei = 0, frames;
	char outfilepath[256];
	char cos_table[256];
	char sin_table[256];
	int ext;
	int granu;
	int datword;
	int volume;
	double vol;
	
	puts("PSGTalk 0.2 - furrtek 2015\n");
	
	// Defaults
	//fres = 64;
	rate = 2;
	channels = 3;
	sim = 0;
	
	if (parseargs(argc, argv)) return 1;

	wsize = loadwav(argv[argc-1], &wavebuffer);
	if (!wsize) {
		puts("Can't load wave file.\n");
		return 1;
	}
	
	if (mode == MODE_PAL)
		fps = 50;
	else
		fps = 60;
	
	// Undersample to just above 8192
	undersample = (samplerate / 8192);
	workrate = samplerate / undersample;
	
	wsize /= undersample;
	
	workbuffer = malloc(wsize * sizeof(unsigned char));
	
	// Decimate wave data
	for (i=0; i<wsize; i++) {
		workbuffer[i] = wavebuffer[i*undersample];
	}

	ws = workrate / (fps * rate);		// Frame size
	m = 1;								// Discrimination between power peaks
	n = ws - 1;
	frames = (wsize / ws);				// Total number of frames
	granu = (workrate / n) / 2;
	
	printf("FRAMES: %u\n", frames);
	printf("Samplerate: %luHz -> %u\n", samplerate, workrate);
	printf("Resolution: %uHz\n", granu);
	printf("Update rate: %u/frame (%uHz @ %ufps)\n", rate, fps * rate, fps);
	printf("Channels: %u\n", channels);
	printf("Mode: %s\n", modestr[mode]);
	printf("Bitrate: %ubps\n\n", rate*fps*2*8);
	
	pow = malloc(n * sizeof(double));
	window = malloc(n * sizeof(unsigned char));
	freqs = malloc(frames * sizeof(int) * channels);
	vols = malloc(frames * sizeof(int) * channels);
	
	// Generate log volume LUT
	vol = 127 / 4;					// 4 channels
	for (c=0; c<15; c++) {
		vol_lut[15-c] = vol;
		vol /= 1.2589;				// 2dB
		//vol_lut[15-c] = (vol)/(c+1);
	}
	vol_lut[0] = 0;

	// Generate cos and sin tables for speed
	for (c=0; c<255; c++) {
		angle = (c * 3.14 * 2) / 256;
		cos_table[c] = (cos(angle) * 127);
		sin_table[c] = (sin(angle) * 127);
	}
	
	// Generate window multiply table
	for (c=0; c<n; c++) {
		window[c] = sin_table[(c*255)/(n*2)]*2;
	}
		
	do {
		// Window frame :)
		for (t=0; t<n; t++) {
			workbuffer[t + f] = 128 + (((long)workbuffer[t + f]-127) * window[t])/256;
		}
		
		// DFT on sample block
    	for (k=0; k<n; k++) {		// For each output element
  	      	sumr = 0;
  	      	sumi = 0;
        	for (t=0; t<n; t++) {		// For each input element
            	c = (0xFF * t * ((double)k/2) / n);
            	inr = workbuffer[t + f] - 127;
            	sumr += (inr * cos_table[c & 0xFF]) / 256;
            	sumi += (inr * sin_table[c & 0xFF]) / 256;
			}
            sumr /= n;
        	sumi /= n;
        
        	pow[k] = sqrt((sumi * sumi) + (sumr * sumr));
    		//if (framei == 1) printf("SAMP=%u POW=%f (%uHz)\n", workbuffer[k + f], pow[k], k*granu);
		}

		// Find highest power and its associated frequency
    	powmax = 0;
    	for (k=1; k<(n-1); k++) {
			if (pow[k] > powmax) {
				powmax = pow[k];
				kmax = k;
			}
		}
		//printf("(%fHz)\n", kmax/2*granu);
    	freqs[framei*channels] = kmax;
    	vols[framei*channels] = lintolog(powmax);
		consolidate = (powmax >= (20/4)) ? 1 : 0;
		
    	/*if (framei == 1)  {
    		system("pause");
    		return 0;
		}*/
    
    	if (channels > 1) {
			if (consolidate) {
				powmax -= (20/4);
	    		freqs[(framei*channels)+1] = freqs[framei*channels];
	    		vols[(framei*channels)+1] = lintolog(powmax);
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
		    	freqs[(framei*channels)+1] = kmax;
		    	vols[(framei*channels)+1] = lintolog(powmax);
			}
	    	
			if (channels > 2) {
				if (consolidate) {
					powmax -= (20/4);
    				freqs[(framei*channels)+2] = freqs[(framei*channels)+1];
    				vols[(framei*channels)+2] = lintolog(powmax);
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
			    	freqs[(framei*channels)+2] = kmax;
			    	vols[(framei*channels)+2] = lintolog(powmax);
				}
			}
		}
        
    	framei++;
    	f += ws;
    
    	printf("\rComputing frame %u/%u.", framei, frames);

	} while (f < (wsize-ws));
	
	puts(" Done.\n");
	
	// Generate output file
	strcpy(outfilepath, argv[argc-1]);
	ext = strlen(outfilepath) - 3;
	outfilepath[ext++] = 'b';
	outfilepath[ext++] = 'i';
	outfilepath[ext] = 'n';
	
	FILE *fo = fopen(outfilepath, "wb");
	
	if (mode == MODE_RAW) {
		if (n < 256) {
			size = ((framei * channels * 2) + 1);			// Frequency(byte)/volume pairs
			datout = malloc(size * sizeof(unsigned char));
			// For each frame
			for (f=0; f<framei-1; f++) {
				// For each channel
				for (c=0; c<channels; c++) {
					datout[(f*channels*2)+(c*2)] = freqs[(f*channels)+c];
					datout[(f*channels*2)+(c*2)+1] = vols[(f*channels)+c];
				}
			}
		} else {
			size = ((framei * channels * 3) + 1);			// Frequency(word)/volume pairs
			datout = malloc(size * sizeof(unsigned char));
			// For each frame
			for (f=0; f<framei-1; f++) {
				// For each channel
				for (c=0; c<channels; c++) {
					datout[(f*channels*3)+(c*3)] = freqs[(f*channels)+c] / 256;
					datout[(f*channels*3)+(c*3)+1] = freqs[(f*channels)+c] & 255;
					datout[(f*channels*3)+(c*3)+2] = vols[(f*channels)+c];
				}
			}
		}
	} else if (mode == MODE_VGM) {
		size = (framei + (framei * channels * 6) + 1);
		datout = malloc(size * sizeof(unsigned char));
		// For each frame
		for (f=0; f<framei-1; f++) {
			// For each channel
			for (c=0; c<channels; c++) {
				datword = 111861*2 / (freqs[(f*channels)+c] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[(f*channels)+c] / 8;
				if (volume > 15) volume = 15;
				
				datout[f+(f*channels*6)+(c*6)] = 0x50;		// VGM "PSG write"
				datout[f+(f*channels*6)+(c*6)+1] = 0x90 | (c<<5) | volume;
				datout[f+(f*channels*6)+(c*6)+2] = 0x50;	// VGM "PSG write"
				datout[f+(f*channels*6)+(c*6)+3] = 0x80 | (c<<5) | (datword & 0x0F);
				datout[f+(f*channels*6)+(c*6)+4] = 0x50;	// VGM "PSG write"
				datout[f+(f*channels*6)+(c*6)+5] = datword / 16;
			}
			datout[f+(f*channels*6)+(channels*6)] = 0x62;	// VGM "NTSC wait"
		}
	} else {
		if (mode == MODE_NTSC) psgfreq = 111861*2;
		if (mode == MODE_PAL) psgfreq = 110841*2;
		if (mode == MODE_NGP) psgfreq = 96000*2;
		size = ((framei * channels * 2) + 1);			// 00vvvvffffffffff
		datout = malloc(size * sizeof(unsigned char));
		// For each frame
		for (f=0; f<framei-1; f++) {
			// For each channel
			for (c=0; c<channels; c++) {
				datword = psgfreq / ((double)freqs[(f*channels)+c] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[(f*channels)+c];
				if (volume > 15) volume = 15;
				datword |= (volume << 10);
				datout[(f*channels*2)+(c*2)] = datword / 256;
				datout[(f*channels*2)+(c*2)+1] = datword & 255;
			}
		}
		puts(" Done.\n");
	}
	
	fwrite(datout, size, 1, fo);
	fclose(fo);

	free(datout);
	
	puts("Output file written.");
	printf("Size: %ukB\n", size / 1024);
	
	// Generate simulation file if needed
	if (sim) {
		if (gensim(ws, framei, channels, freqs, vols)) puts("\nSimulation file written.\n");
	}
	
    system("pause");
    return 0;
	
	free(freqs);
	free(vols);
	free(window);
	free(wavebuffer);
	free(workbuffer);
	free(pow);

	return 0;
}
