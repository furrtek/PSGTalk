# PSGTalk
Command line tool for generating register streams for the SN76489 (PSG) from speech wave files.

Used to mimic voice in PSG-based systems such as the Sega Master System, SNK NeoGeo Pocket, ColecoVision...

Playback example source included.

# Usage

Input needs to be a 44100Hz 8bit mono wave file.

-r (16 to 512): Frequency resolution, FFT size. Default: 64.

-u (1 to 64):   PSG updates per frame. Default: 4.

-c (1 to 3):    Number of channels used. Default: 3.

-m (mode):      raw, ntsc, pal, vgm or ngp

-s:             Generate simulation file

Execute without parameters for details.
