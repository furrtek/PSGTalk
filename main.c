// PSGTalk 0.2
// Generates PSG (SN76489) update streams from speech wave files
// to mimic voice with square tones
// Furrtek 2015

// TODO: Improve quality (A LOT) on real hardware
//       Compare SMS/Coleco/NGP output with simulation
// TODO: Normalize FFT output according to size
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
#include <unistd.h>

int loadwav(const char *filename, unsigned char **result) 
{ 
	int size = 0;
	
	FILE *f = fopen(filename, "rb");
	if (f == NULL) { 
		*result = NULL;
		return 0;
	} 
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size);
	if (size != fread(*result, sizeof(unsigned char), size, f)) { 
		free(*result);
		return 0;
	} 
	fclose(f);
	return size;
}

double sq(double a) {
	if (sin(a) > 0)
		return 1;
	else
		return -1;	
}

void printusage(void) {
	puts("Usage:\n");
	puts("psgtalk [options] speech.wav\n");
	puts("Wave file needs to be 44100Hz 8bit mono");
	puts("-r [16~512]: Frequency resolution, default: 64");
	puts("               Large value increases quality but also computation time");
	puts("-u [1~64]:   PSG updates per frame, default: 4");
	puts("               Large value increases quality but also data size");
	puts("               Values above 1 will require raster interrupts for playback");
	puts("-c [1~3]:    Number of PSG channels to use, default: 3");
	puts("               3 is best, 2 is average, 1 is unintelligible");
	puts("-m [mode]:   Output mode, default: ntsc");
	puts("               raw: sequential frequency values for each channel");
	puts("               bytes for r < 256, words for r >= 256");
	puts("               ntsc: NTSC (223722Hz) words");
	puts("               pal: PAL (221681Hz) words");
	puts("               ngp: NeoGeo Pocket (192000Hz) words");
	puts("-s:          Generate psgtalk.raw 44100Hz 8bit mono simulation file");
}

int main(int argc, char *argv[]) {
	unsigned char * content;
	int size, wsize;
	int c, ws, m, n, k, t, f = 0;
	double kmax, akmax, akmin, bkmax, bkmin;
	double sa, sb, sc, powmax;
	unsigned char b;
	double *pow;
	unsigned char *wavout;
	unsigned char *datout;
	double angle, inr, sumr, sumi;
	int psgfreq;
	int freqs[3][4096];			// TODO: malloc
	int vols[3][4096];
	int framei = 0, frames;
	int s, mix;
	int fres, rate, channels, sim, fps;
	char opt;
	char modearg[5];			// Beware !
	char outfilepath[256];
	int ext, granu;
	int datword;
	int volume;
	
	typedef enum {MODE_RAW = 0, MODE_NTSC, MODE_PAL, MODE_NGP} modetype;
	
	char * modestr[4] = {"raw", "ntsc", "pal", "ngp"};
	
	puts("PSGTalk 0.2 - furrtek 2015\n");
	
	if (argc < 2) {
		printusage();
		return 1;
	}
	
	fres = 64;
	rate = 4;
	channels = 3;
	modetype mode = MODE_NTSC;
	sim = 0;
	
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
				if (sscanf(optarg, "%s", &modearg) != 1) {
					printusage();
					return 1;
				}
				for (c=0; c<4; c++) {
					if (!strcmp(modearg, modestr[c])) {
						mode = c;
						break;
					}
				}
				if (c == 4) {
					puts("Invalid mode.\n");
					return 1;
				}
				break;
			case 's':
				sim = 1;
		}
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
	printf("Mode: %s\n\n", modestr[mode]);

	ws = 1 * 44100 / (fps * rate);	// Sample block size
	m = 1;							// Discrimination between power peaks
    
	pow = malloc((fres-1) * sizeof(double));
	
	frames = (wsize / ws) +1;
		
	do {
		// TODO: Window !
		
		// DFTize sample block
	    n = ws - 1;
    	for (k=0; k<fres-1; k++) {	// For each output element
  	      	sumr = 0;
  	      	sumi = 0;
        	for (t=0; t<n; t++) {	// For each input element
            	angle = (3.14 * 2 * t * k / n);
            	inr = (double)content[t + f] / 256;
            	sumr += (inr * cos(angle));
            	sumi += (-inr * sin(angle));
			}
        	pow[k] = sqrt((sumi * sumi) + (sumr * sumr));
    	}

		// Find highest power and its associated frequency
    	powmax = 0;
    	for (k=1; k<fres; k++) {
			if (pow[k] > powmax) {
				powmax = pow[k];
				kmax = k;
			}
		}
    	freqs[0][framei] = kmax;
    	vols[0][framei] = powmax;
    
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
	    	freqs[1][framei] = kmax;
	    	vols[1][framei] = powmax;
	    	
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
		    	freqs[2][framei] = kmax;
		    	vols[2][framei] = powmax;
			}
		}
        
    	framei++;
    	f += ws;
    
    	printf("\rComputing frame %u/%u.", framei, frames);

	} while (f < wsize);
	
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
					datout[(f*channels*2)+(c*2)] = freqs[c][f];
					datout[(f*channels*2)+(c*2)+1] = vols[c][f];
				}
			}
		} else {
			size = ((framei * channels * 3) + 1);			// Frequency(word)/volume pairs
			datout = malloc(size * sizeof(unsigned char));
			// For each frame
			for (f=0; f<framei-1; f++) {
				// For each channel
				for (c=0; c<channels; c++) {
					datout[(f*channels*3)+(c*3)] = freqs[c][f] / 256;
					datout[(f*channels*3)+(c*3)+1] = freqs[c][f] & 255;
					datout[(f*channels*3)+(c*3)+2] = vols[c][f];
				}
			}
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
				datword = psgfreq / (freqs[c][f] * granu);
				if (datword > 1023) datword = 1023;		// TODO: Handle low freqs by cutting volume ?
				volume = vols[c][f];
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
	printf("Bitrate: %ubps\n", rate*fps*2*8);
	
	// Generate simulation file if needed
	if (sim) {
		size = ((framei * ws) + 1);
		wavout = malloc(size * sizeof(unsigned char));
	
		// For each frame
		for (f=0; f<framei-1; f++) {
			// Generate samples with dirty square wave algorithm
			for (s=0; s<ws; s++) {
	            sa = 128 + (sq(2 * 3.14 * freqs[0][f] * s / ws) * vols[0][f] * 2);
	            mix = sa;
	            if (channels > 1) {
					sb = 128 + (sq(2 * 3.14 * freqs[1][f] * s / ws) * vols[1][f] * 4);
	            	mix += sb;
				}
				if (channels > 2) {
					sc = 128 + (sq(2 * 3.14 * freqs[2][f] * s / ws) * vols[2][f] * 4);
					mix += sc;
				}
	            b = (unsigned char)(mix / channels);
	            wavout[s + (f * ws)] = b;
			}
		}
		
		FILE *fsim = fopen("psgtalk.raw", "wb");
		fwrite(wavout, size, 1, fsim);
		fclose(fsim);
		
		free(wavout);
		puts("\nSimulation file written.\n");
	}
	
	free(content);
	free(pow);
	
	return 0;
}
