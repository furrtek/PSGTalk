#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

int main(int argc, char *argv[]) {
	unsigned char *content; 
	int size;
	int ws, aw, m, n, k, t, f = 0;
	double kmax, okmax, pkmax;
	double sa, sb, sc, powmax;
	unsigned char b;
	double *pow;
	unsigned char *wavout;
	double angle, inr, sumr, sumi;
	double freqs[3][4096];
	double vols[3][4096];
	int framei = 0;
	int windw;
	
	if (argc != 2) {
		puts("Specify a 44100Hz 8bit mono wave file.");
		return 1;
	}
	size = loadwav(argv[1], &content);
	if (!size) 
	{ 
		puts("Can not load file");
		return 1;
	}

	ws = 1 * 44100 / 120;	// Time resolution
    aw = 32;				// Frequency resolution
    m = 1;					// Discrimination
    
	pow = malloc((aw-1)*sizeof(double));
		
	do {

	    n = ws - 1;
    	for (k=0;k<aw-1;k++) {		// For each output element
  	      	sumr = 0;
  	      	sumi = 0;
        	for (t=0;t<n;t++) {		// For each input element
            	angle = (3.14 * 2 * t * k / n);
            	inr = (double)content[t + f] / 256;
            	sumr += (inr * cos(angle));
            	sumi += (-inr * sin(angle));
			}
        	pow[k] = sqrt((sumi * sumi) + (sumr * sumr));
    	}

    	powmax = 0;
    	for (k=1;k<aw;k++) {
			if (pow[k] > powmax) {
				powmax = pow[k];
				kmax = k;
			}
		}
    	freqs[0][framei] = kmax;
    	vols[0][framei] = (powmax / 16) * 16;
    	okmax = kmax;
    
   	 	powmax = 0;
    	for (k=1;k<aw;k++) {
        	if ((k > okmax + m) || (k < okmax - m)) {
            	if (pow[k] > powmax) {
                	powmax = pow[k];
                	kmax = k;
				}
			}
		}
    	freqs[1][framei] = kmax;
    	vols[1][framei] = (powmax / 16) * 16;
    	pkmax = kmax;
    
    	powmax = 0;
    	for (k=1;k<aw;k++) {
        	if ((k > okmax + m) || (k < okmax - m)) {
				if ((k > pkmax + m) || (k < pkmax - m)) {
            		if (pow[k] > powmax) {
                		powmax = pow[k];
                		kmax = k;
					}
				}
			}
		}
    	freqs[2][framei] = kmax;
    	vols[2][framei] = (powmax / 16) * 16;
        
    	framei++;
    	f += ws;
    
    	printf("\rComputing frame %u.",framei);

	} while (f < size);
	
	puts(" Done.\n");
	
	size = ((framei * ws)+1);
	
	wavout = malloc(size*sizeof(unsigned char));

	for (f=0;f<framei-1;f++) {
		for (windw=0;windw<ws;windw++) {
            sa = 128 + (sq(2 * 3.14 * freqs[0][f] * windw / ws) * vols[0][f] * 2);
            sb = 128 + (sq(2 * 3.14 * freqs[1][f] * windw / ws) * vols[1][f] * 2);
            sc = 128 + (sq(2 * 3.14 * freqs[2][f] * windw / ws) * vols[2][f] * 2);
            b = (unsigned char)((sa + sb + sc) / 3);
            wavout[windw + (f * ws)] = b;
		}
	}
	
	FILE *fo = fopen("out.raw", "wb");
	fwrite(wavout,size,1,fo);
	fclose(fo);
	
	free(wavout);
	free(content);
	free(pow);
	
	return 0;
}
