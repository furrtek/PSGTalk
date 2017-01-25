#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_MODES 5

void printusage(void) {
	puts("Usage:\n");
	puts("psgtalk [options] speech.wav\n");
	puts("Wave file needs to be 8bit signed mono");
	puts("-r [16~512]: Frequency resolution, default: 64");
	puts("               A large value increases quality but also computation time");
	puts("-u [1~64]:   PSG updates per frame, default: 2");
	puts("               A large value increases quality but also data size");
	puts("               Values above 1 will require raster interrupts for playback");
	puts("-c [1~3]:    Number of PSG channels to use, default: 3");
	puts("               3 is best, 2 is average, 1 is unintelligible");
	puts("-p [0~50]:   Window overlap percentage");
	puts("               0 is no overlap, 50 is half (maximum)");
	puts("-m [mode]:   Output mode, default: ntsc");
	puts("               raw: sequential frequency values for each channel");
	puts("               bytes for r < 256, words for r >= 256");
	puts("               ntsc: NTSC (223722Hz) words");
	puts("               pal: PAL (221681Hz) words");
	puts("               vgm: Master System NTSC VGM (no header)");
	puts("               ngp: NeoGeo Pocket (192000Hz) words");
	puts("-s:          Generate psgtalk.raw 44100Hz 8bit mono simulation file");
}

int parse_args(int argc, char * argv[]) {
	unsigned int c;
	unsigned int overlap_percent;
	char opt;
	
	if (argc < 2) {
		printusage();
		return 1;
	}
	
	while ((opt = getopt(argc, argv, "r:u:c:m:s:p")) != -1) {
		switch(opt) {
			case 'r':
				if (sscanf(optarg, "%i", &freq_res) != 1) {
					printusage();
					return 1;
				}
				if ((freq_res < 16) || (freq_res > 512)) {
					puts("Invalid frequency resolution.\n");
					return 1;
				}
				break;
			case 'u':
				if (sscanf(optarg, "%i", &update_rate) != 1) {
					printusage();
					return 1;
				}
				if ((update_rate < 1) || (update_rate > 64)) {
					puts("Invalid update rate.\n");
					return 1;
				}
				break;
			case 'c':
				if (sscanf(optarg, "%i", &psg_channels) != 1) {
					printusage();
					return 1;
				}
				if ((psg_channels < 1) || (psg_channels > 3)) {
					puts("Invalid active channels number.\n");
					return 1;
				}
				break;
			case 'm':
				if (sscanf(optarg, "%s", modearg) != 1) {
					printusage();
					return 1;
				}
				for (c = 0; c < MAX_MODES; c++) {
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
				break;
			case 'p':
				if (sscanf(optarg, "%i", &overlap_percent) != 1) {
					printusage();
					return 1;
				}
				if (overlap_percent > 64) {
					puts("Invalid overlap percentage.\n");
					return 1;
				}
				overlap = (float)overlap_percent / 100.0;
				break;
		}
	}
	
	return 0;
}
