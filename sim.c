#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

int square(int a) {
	a &= 0xFF;
	if (a > 0x7F)
		return 1;
	else
		return -1;	
}

int gensim(int ws, int framei, char channels, int freqs[3*4096], int vols[3*4096]) {
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
            sa = 128 + (square(0xFF * freqs[f*3] * s / ws) * (vols[f*3] & 0xF0));
            mix = sa;
            if (channels > 1) {
				sb = 128 + (square(0xFF * freqs[(f*3)+1] * s / ws) * (vols[(f*3)+1] & 0xF0));
            	mix += sb;
			}
			if (channels > 2) {
				sc = 128 + (square(0xFF * freqs[(f*3)+2] * s / ws) * (vols[(f*3)+2] & 0xF0));
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
	return 1;
}
