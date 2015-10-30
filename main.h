#ifndef _H
#define _H

typedef enum {MODE_RAW = 0, MODE_NTSC, MODE_PAL, MODE_VGM, MODE_NGP} modetype;

long samplerate;
double vol_lut[16];
unsigned char modearg[5];		// Beware !
int fres, rate, channels, sim, fps;
extern modetype mode;
extern char * modestr[5];

#endif
