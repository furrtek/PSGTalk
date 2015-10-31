#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

extern double vol_lut[16];

int square(int a) {
	a &= 0xFF;
	if (a > 0x7F)
		return 1;
	else
		return -1;	
}

int gensim(int ws, int framei, char channels, int freqs[], int vols[]) {
	int size;
	int f, s, mix;
	unsigned char *wavout;
	int sa, sb, sc;
	unsigned char b;
	
	size = ((framei * ws) + 1);
	wavout = malloc(size * sizeof(unsigned char));

	// For each frame
	for (f=0; f<framei-1; f++) {
		// Generate samples with dirty square wave algorithm
		for (s=0; s<ws; s++) {
            sa = 128 + (square(0xFF * freqs[f*channels]/2 * s / ws) * vol_lut[vols[f*channels]]);
            mix = sa;
            if (channels > 1) {
				sb = 128 + (square(0xFF * freqs[(f*channels)+1]/2 * s / ws) * vol_lut[vols[(f*channels)+1]]);
            	mix += sb;
			}
			if (channels > 2) {
				sc = 128 + (square(0xFF * freqs[(f*channels)+2]/2 * s / ws) * vol_lut[vols[(f*channels)+2]]);
				mix += sc;
			}
            b = (unsigned char)mix;
            wavout[s + (f * ws)] = b;
		}
	}
	
	FILE *fsim = fopen("psgtalk.raw", "wb");
	fwrite(wavout, size, 1, fsim);
	fclose(fsim);
	
	free(wavout);
	return 1;
}
