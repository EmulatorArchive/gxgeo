/***************************************************************************

  streams.c

  Handle general purpose audio streams

***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include "emu.h"
#include <math.h>
#include "sound.h"


#define MIXER_MAX_CHANNELS 16
#define BUFFER_LEN 16384

#define CPU_FPS 60

extern int frame;

Uint16 left_buffer[BUFFER_LEN];
Uint16 right_buffer[BUFFER_LEN];
Uint16 play_buffer[BUFFER_LEN] __attribute__ ((__aligned__ (32)));


int SamplePan[] = { 0, 255, 128, 0, 255 };


static int stream_joined_channels[MIXER_MAX_CHANNELS];

static int stream_vol[MIXER_MAX_CHANNELS];
static int cur_channel;

struct {
    Sint16 *buffer;
    int param;
    void (*callback) (int param, Sint16 ** buffer, int length);
} stream[MIXER_MAX_CHANNELS];


void mixer_set_volume(int channel,int volume) {
    stream_vol[channel]=volume;
}

int streams_sh_start(void)
{
  int i;
  cur_channel=0;
  for (i = 0; i < MIXER_MAX_CHANNELS; i++) {
    stream_joined_channels[i] = 1;
    stream[i].buffer = 0;
  }
  return 0;
}


void streams_sh_stop(void)
{
  int i;
  
  for (i = 0; i < MIXER_MAX_CHANNELS; i++) {
    free(stream[i].buffer);
    stream[i].buffer = 0;
  }
}

#define ADJUST_VOLUME(s, v)	(s = (s*v)/128)
void MixAudio16(Uint8 *dst, Uint8 *src, Uint32 len, int volume)
{
#ifdef WORDS_BIGENDIAN
	Sint16 src1, src2;
	int dst_sample;
	const int max_audioval = ((1<<(16-1))-1);
	const int min_audioval = -(1<<(16-1));

	len /= 2;
	while ( len-- ) {
		src1 = ((src[0])<<8|src[1]);
		ADJUST_VOLUME(src1, volume);
		src2 = ((dst[0])<<8|dst[1]);
		src += 2;
		dst_sample = src1+src2;
		if ( dst_sample > max_audioval ) {
			dst_sample = max_audioval;
		} else if ( dst_sample < min_audioval ) {
			dst_sample = min_audioval;
		}
		dst[1] = dst_sample&0xFF;
		dst_sample >>= 8;
		dst[0] = dst_sample&0xFF;
		dst += 2;
	}
#else
	Sint16 src1, src2;
	int dst_sample;
	const int max_audioval = ((1<<(16-1))-1);
	const int min_audioval = -(1<<(16-1));

	len /= 2;
	while ( len-- ) {
		src1 = ((src[1])<<8|src[0]);
		ADJUST_VOLUME(src1, volume);
		src2 = ((dst[1])<<8|dst[0]);
		src += 2;
		dst_sample = src1+src2;
		if ( dst_sample > max_audioval ) {
			dst_sample = max_audioval;
		} else if ( dst_sample < min_audioval ) {
			dst_sample = min_audioval;
		}
		dst[0] = dst_sample&0xFF;
		dst_sample >>= 8;
		dst[1] = dst_sample&0xFF;
		dst += 2;
	}
#endif
}

void streamupdate(int len)
{
    static int current_pos;
    int channel, i;
    Uint16 *bl, *br;
    Uint16 *pl;


/*
    if (len+current_pos >= (desired->samples<<2))
	len = (desired->samples<<2) - current_pos;
*/
    /* update all the output buffers */
    memset(left_buffer, 0, len);
    memset(right_buffer, 0, len);

    for (channel = 0; channel < MIXER_MAX_CHANNELS; channel += stream_joined_channels[channel]) {
		if (stream[channel].buffer) {
			int buflen;
			
		    buflen = len >> 2;	// BUFFER_LEN; // newpos - stream_buffer_pos[channel];

		    if (stream_joined_channels[channel] > 1) {
				Sint16 *buf[MIXER_MAX_CHANNELS];

				// on update le YM2610
				if (buflen > 0) {
					for (i = 0; i < stream_joined_channels[channel]; i++)
						buf[i] = stream[channel + i].buffer;	// + stream[channel+i].buffer_pos;
				    (*stream[channel].callback) (stream[channel].param, buf, buflen);
				}

			}
		}
    }

    bl = left_buffer;
    br = right_buffer;

    for (channel = 0; channel < MIXER_MAX_CHANNELS; channel += stream_joined_channels[channel]) {
      if (stream[channel].buffer) {
        for (i = 0; i < stream_joined_channels[channel]; i++) {
          if (SamplePan[channel + i] <= 128)
            MixAudio16((Uint16 *) bl, (Uint16 *) stream[channel + i].buffer, len >> 1, 128);
          if (SamplePan[channel + i] >= 128)
            MixAudio16((Uint16 *) br, (Uint16 *) stream[channel + i].buffer, len >> 1, 128);
        }
      }
    }

    //SDL_LockAudio();
	//pl = play_buffer + (current_pos>>1);
    pl = (Uint16 *) play_buffer;
    for (i = 0; i < len >> 1; ++i) {
      *pl++ = right_buffer[i];
      *pl++ = left_buffer[i];
    }
    //SDL_UnlockAudio();

/*
    current_pos+=len;
    if (current_pos >= (desired->samples<<2))
	current_pos=0;
*/
}

int stream_init_multi(int channels, int param, void (*callback) (int param, Sint16 ** buffer, int length))
{
  int i;
  
  stream_joined_channels[cur_channel] = channels;
  
  for (i = 0; i < channels; i++) {  
    if ((stream[cur_channel + i].buffer = malloc(sizeof(Sint16) * BUFFER_LEN)) == 0) return -1;
  }
  
  stream[cur_channel].param = param;
  stream[cur_channel].callback = callback;
  cur_channel += channels;
  return cur_channel - channels;
}
