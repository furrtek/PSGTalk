#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_MODES 5

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

int parseargs(int argc, char * argv[]) {
	char opt;
	int c;
	
	if (argc < 2) {
		printusage();
		return 1;
	}
	
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
		puts("VGM mode only works with 1 update per frame (-u 1), forced to 1.\n");
		rate = 1;
	}
	
	return 0;
}
