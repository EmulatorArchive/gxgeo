/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */  
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

#include "streams.h"
#include "emu.h"
#include "memory.h"
#include "conf.h"

//#define MIXER_MAX_CHANNELS 16
//#define CPU_FPS 60
#define BUFFER_LEN 6400
//extern int throttle;
//static int audio_sample_rate;
extern Uint16 play_buffer[BUFFER_LEN];
//extern unsigned char play_buffer[BUFFER_LEN] ;


#define STREAM_SIZE 3200

void ogc_update_stream()
{
  if (conf.sound) {
    streamupdate(STREAM_SIZE);
    AUDIO_StopDMA ();
    AUDIO_InitDMA((u32) play_buffer, STREAM_SIZE);
    DCFlushRange(play_buffer, STREAM_SIZE);
    AUDIO_StartDMA();
  }
}

int ogc_init_audio(void)
{
  if (conf.sound) {
  	AUDIO_Init (NULL);
  	AUDIO_SetDSPSampleRate (AI_SAMPLERATE_48KHZ);
  	//AUDIO_RegisterDMACallback (ogc_update_stream);
  	memset(play_buffer, 0, BUFFER_LEN);
    return 1;
  } return 1;
}

void ogc_close_audio(void) {
  if (conf.sound) {
    AUDIO_StopDMA ();
  }
}
