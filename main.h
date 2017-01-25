#ifndef _H
#define _H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef enum {MODE_RAW = 0, MODE_NTSC, MODE_PAL, MODE_VGM, MODE_NGP} psgmode_t;
typedef enum {WAIT_N = 0, WAIT_NTSC, WAIT_PAL} wait_t;
typedef struct channels {
	unsigned int ch[3];
} channels_t;

float * cos_lut;
float * sin_lut;
float * window_lut;

psgmode_t mode;
float overlap;
unsigned int freq_step;
unsigned long samplerate_in;
int freq_res;
float attenuation_lut[16];
unsigned char modearg[5];		// Beware !
unsigned int updates_per_frame, update_rate, psg_channels, fps;
int sim;
extern psgmode_t mode;
extern char * modestr[5];


int parse_args(int argc, char * argv[]);
int load_wav(const char * filename, float * result);
int gensim(int ws, int framei, char channels, channels_t const * frequencies, channels_t const * volumes);
int make_LUTs(unsigned long window_size);
void lowpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length);
void highpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length);
int out_vgm(unsigned char * out_buffer, channels_t const * frequencies, channels_t const * volumes,
	const unsigned long frame_count);
int out_raw(unsigned char * out_buffer, channels_t const * frequencies, channels_t const * volumes,
	const unsigned long frame_count, const unsigned int n);

#endif
