#ifndef _H
#define _H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef enum {MODE_RAW = 0, MODE_NTSC, MODE_PAL, MODE_VGM, MODE_NGP} psgmode_t;
typedef enum {WAIT_N = 0, WAIT_NTSC, WAIT_PAL} wait_t;

float * cos_lut;
float * sin_lut;
float * window_lut;

psgmode_t mode;
unsigned long samplerate_in;
float attenuation_lut[16];
unsigned char modearg[5];		// Beware !
int fres, update_rate, psg_channels, sim, fps;
extern psgmode_t mode;
extern char * modestr[5];


int parseargs(int argc, char * argv[]);
int load_wav(const char * filename, float * result);
int gensim(int ws, int framei, char channels, int freqs[], int vols[]);
int make_LUTs(unsigned long window_size);
void lowpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length);
void highpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length);
int out_vgm(unsigned char * out_buffer, unsigned int const * frequencies, unsigned int const * volumes);
int out_raw(unsigned char * out_buffer, unsigned int const * frequencies, unsigned int const * volumes, const unsigned int n);

#endif
