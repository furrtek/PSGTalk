// PSGTalk 0.21
// Generates PSG (SN76489) update streams from speech wave files
// to mimic voice with square tones
// Furrtek 2015

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define MAX_MODES 5

int loadwav(const char *filename, unsigned char **result);
int gensim(int ws, int framei, char channels, int freqs[3*4096], int vols[3*4096]);

void printusage(void) {
	puts("Usage:\n");
	puts("psgtalk [options] speech.wav\n");
	puts("Wave file needs to be 44100Hz 8bit mono");
	puts("-r [16~512]: Frequency resolution, default: 64");
	puts("               Large value increases quality but also computation time");
	puts("-u [1~64]:   PSG updates per frame, default: 2");
	puts("               Large value increases quality but also data size");
	puts("               Values above 1 will require raster interrupts for playback");
	puts("-c [1~3]:    Number of PSG channels to use, default: 3");
	puts("               3 is best, 2 is average, 1 is unintelligible");
	puts("-m [mode]:   Output mode, default: ntsc");
	puts("               raw: sequential frequency values for each channel");
	puts("               bytes for r < 256, words for r >= 256");
	puts("               ntsc: NTSC (223722Hz) words");
	puts("               pal: PAL (221681Hz) words");
	puts("               vgm: Master System NTSC VGM (no header)");
	puts("               ngp: NeoGeo Pocket (192000Hz) words");
	puts("-s:          Generate psgtalk.raw 44100Hz 8bit mono simulation file");
}

int main(int argc, char *argv[]) {
	unsigned char * content;
	unsigned char * window;
	int size, wsize;
	int c, ws, m, n, k, t, f = 0;
	double kmax, akmax, akmin, bkmax, bkmin;
	double powmax = 0;
	double * pow;
	unsigned char *datout;
	double angle;
	double inr;
	double sumr, sumi;
	int psgfreq;
	int * freqs;
	int * vols;
	int framei, frames;
	int fres, rate, channels, sim, fps;
	char opt;
	unsigned char modearg[5];		// Beware !
	char outfilepath[256];
	char cos_table[256];
	char sin_table[256];
	int ext;
	int granu;
	int datword;
	int volume;
	
	typedef enum {MODE_RAW = 0, MODE_NTSC, MODE_PAL, MODE_VGM, MODE_NGP} modetype;
	
	char * modestr[5] = {"raw", "ntsc", "pal", "vgm", "ngp"};
	
	puts("PSGTalk 0.2 - furrtek 2015\n");
	
	if (argc < 2) {
		printusage();
		return 1;
	}
	
	// Defaults
	fres = 64;
	rate = 2;
	channels = 3;
	modetype mode = MODE_NTSC;
	sim = 0;
	framei = 0;
	
	while ((opt = getopt(argc, argv, "r:u:c:m:s")) != -1) {
		switch(opt) {
			case 'r':
				if (sscanf(optarg, "%i", &fres) != 1) {
					printusage();
					return 1;
				}
				if ((fres < 16) || (fres > 512)) {
					puts("Invalid frequency resolution.\n");
					return 1;
				}
				break;
			case 'u':
				if (sscanf(optarg, "%i", &rate) != 1) {
					printusage();
					return 1;
				}
				if ((rate < 1) || (rate > 64)) {
					puts("Invalid update rate.\n");
					return 1;
				}
				break;
			case 'c':
				if (sscanf(optarg, "%i", &channels) != 1) {
					printusage();
					return 1;
				}
				if ((channels < 1) || (channels > 3)) {
					puts("Invalid active channels number.\n");
					return 1;
				}
				break;
			case 'm':
				if (sscanf(optarg, "%s", modearg) != 1) {
					printusage();
					return 1;
				}
				for (c=0; c<MAX_MODES; c++) {
					if (!strcmp(modearg, modestr[c])) {
						mode = c;
						break;
					}
				}
				if (c == MAX_MODES) {
					puts("Invalid mode.\n");
					return 1;
				}
				break;
			case 's':
				sim = 1;
		}
	}
	
	if ((mode == MODE_VGM) && (rate > 1)) {
		puts("VGM mode only works with 1 update per frame (-u 1).\n");
		return 1;
	}

	wsize = loadwav(argv[argc-1], &content);
	if (!wsize) { 
		puts("Can't load wave file.\n");
		return 1;
	}
	
	if (mode == MODE_PAL)
		fps = 50;
	else
		fps = 60;
	
	granu = (44100 / 2 / fres);
	
	printf("Resolution: %u (%uHz)\n", fres, granu);
	printf("Update rate: %u/frame (%uHz @ %ufps)\n", rate, fps * rate, fps);
	printf("Channels: %u\n", channels);
	printf("Mode: %s\n", modestr[mode]);
	printf("Bitrate: %ubps\n\n", rate*fps*2*8);

	ws = 1 * 44100 / (fps * rate);	// Sample block size
	m = 1;							// Discrimination between power peaks
	n = ws - 1;
	frames = (wsize / ws);
	
	pow = malloc((fres-1) * sizeof(double));
	window = malloc(n * sizeof(unsigned char));
	
	freqs = malloc(frames * sizeof(int) * channels);
	vols = malloc(frames * sizeof(int) * channels);

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
			content[t + f] = 128 + (((long)content[t + f]-127) * window[t])/256;
		}
		
		// DFT on sample block
    	for (k=0; k<fres-1; k++) {	// For each output element
  	      	sumr = 0;
  	      	sumi = 0;
        	for (t=0; t<n; t++) {	// For each input element
            	c= (0xFF * t * k / n);
            	inr = content[t + f];
            	sumr += (inr * cos_table[c & 0xFF]) / 256;
            	sumi += (inr * sin_table[c & 0xFF]) / 256;
			}
			sumr /= 64;
			sumi /= 64;
        	pow[k] = sqrt((sumi * sumi) + (sumr * sumr)) * rate;
    	}

		// Find highest power and its associated frequency
    	powmax = 0;
    	for (k=1; k<fres; k++) {
			if (pow[k] > powmax) {
				powmax = pow[k];
				kmax = k;
			}
		}
    	freqs[framei*3] = kmax;
    	vols[framei*3] = powmax;
    
    	if (channels > 1) {
    		akmax = kmax + m;
    		akmin = kmax - m;
    		
    		// Find the second highest power and its associated frequency
	   	 	powmax = 0;
	    	for (k=1; k<fres; k++) {
	        	if ((k > akmax) || (k < akmin)) {
	            	if (pow[k] > powmax) {
	                	powmax = pow[k];
	                	kmax = k;
					}
				}
			}
	    	freqs[(framei*3)+1] = kmax;
	    	vols[(framei*3)+1] = powmax;
	    	
    		if (channels > 2) {
    			bkmax = kmax + m;
    			bkmin = kmax - m;
	    
	    		// Find the third highest power and its associated frequency
		    	powmax = 0;
		    	for (k=1; k<fres; k++) {
		        	if ((k > akmax) || (k < akmin)) {
						if ((k > bkmax) || (k < bkmin)) {
		            		if (pow[k] > powmax) {
		                		powmax = pow[k];
		                		kmax = k;
							}
						}
					}
				}
		    	freqs[(framei*3)+2] = kmax;
		    	vols[(framei*3)+2] = powmax;
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
		if (fres < 256) {
			size = ((framei * channels * 2) + 1);			// Frequency(byte)/volume pairs
			datout = malloc(size * sizeof(unsigned char));
			// For each frame
			for (f=0; f<framei-1; f++) {
				// For each channel
				for (c=0; c<channels; c++) {
					datout[(f*channels*2)+(c*2)] = freqs[(f*3)+c];
					datout[(f*channels*2)+(c*2)+1] = vols[(f*3)+c];
				}
			}
		} else {
			size = ((framei * channels * 3) + 1);			// Frequency(word)/volume pairs
			datout = malloc(size * sizeof(unsigned char));
			// For each frame
			for (f=0; f<framei-1; f++) {
				// For each channel
				for (c=0; c<channels; c++) {
					datout[(f*channels*3)+(c*3)] = freqs[(f*3)+c] / 256;
					datout[(f*channels*3)+(c*3)+1] = freqs[(f*3)+c] & 255;
					datout[(f*channels*3)+(c*3)+2] = vols[(f*3)+c];
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
				datword = 111861*2 / (freqs[(f*3)+c] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[(f*3)+c] / 8;
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
				datword = psgfreq / (freqs[(f*3)+c] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[(f*3)+c] / 8;
				if (volume > 15) volume = 15;
				datword |= (volume << 10);
				datout[(f*channels*2)+(c*2)] = datword / 256;
				datout[(f*channels*2)+(c*2)+1] = datword & 255;
			}
		}
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
	
	free(freqs);
	free(vols);
	free(window);
	free(content);
	free(pow);

	return 0;
}
