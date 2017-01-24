#include "main.h"

int make_LUTs(unsigned long window_size) {
	unsigned long i;
	float angle;
	
	cos_lut = (float *)malloc(32768 * sizeof(float));
	sin_lut = (float *)malloc(32768 * sizeof(float));
	window_lut = (float *)malloc(window_size * sizeof(float));
	if ((cos_lut == NULL) || (sin_lut == NULL) || (window_lut == NULL))
		return 0;
	
	// Cos and sin LUTs
	for (i = 0; i < 32768; i++) {
		angle = (i * 2 * M_PI) / 32768.0;
		cos_lut[i] = cos(angle);
		sin_lut[i] = sin(angle);
	}
	
	// Cosine window LUT
	for (i = 0; i < window_size; i++)
		window_lut[i] = sin((i * M_PI) / (window_size - 1));
	
	// Attenuation LUT
	// An SN76489 step is 2dB (10^-0.1)
	for (i = 0; i < 15; i++)
		attenuation_lut[15 - i] = powf(10.0, -((float)i / 10.0));
	
	attenuation_lut[0] = 0;
	
	return 1;
}

void lowpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length) {
	unsigned long i;
	
    float RC = 1.0 / (cutoff * 2 * M_PI);
    float dt = 1.0 / sample_rate;
    float alpha = RC / (RC + dt);
    
    out_buffer[0] = in_buffer[0];
    for (i = 1; i < length; i++)
        out_buffer[i] = out_buffer[i - 1] + (alpha * (in_buffer[i] - out_buffer[i - 1]));
}

void highpass(float * in_buffer, float * out_buffer, float cutoff, float sample_rate, unsigned long length) {
	unsigned long i;
	
    float RC = 1.0 / (cutoff * 2 * M_PI);
    float dt = 1.0 / sample_rate;
    float alpha = RC / (RC + dt);
    
    out_buffer[0] = in_buffer[0];
    for (i = 1; i < length; i++)
        out_buffer[i] = alpha * (out_buffer[i - 1] + in_buffer[i] - in_buffer[i - 1]);
}
