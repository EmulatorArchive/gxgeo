/*  gngeo, a neogeo emulator
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <ogc/lwp_watchdog.h>
#include "types.h"
#include "frame_skip.h"
#include "messages.h"
#include "emu.h"
#include "conf.h"
#include <wiiuse/wpad.h>

#ifndef uclock_t
#define uclock_t Uint32
#endif

#define TICKS_PER_SEC 1000000UL
//#define CPU_FPS 60
static int CPU_FPS=60;
static uclock_t F; /* frequency */

#define MAX_FRAMESKIP 10

static char init_frame_skip = 1;
char skip_next_frame = 0;
static struct timeval init_tv = { 0, 0 };
uclock_t bench;
bool do_bench = false;

uclock_t get_ticks(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    /*
    if (init_tv.tv_sec == 0) {
      init_tv.tv_sec 	= tv.tv_sec;
      init_tv.tv_usec = tv.tv_usec;		
    }
    */
    printf("time: %ul\n",gettick());
    return 0;/*(uclock_t)((tv.tv_sec - init_tv.tv_sec) * TICKS_PER_SEC) + (tv.tv_usec - init_tv.tv_usec);*/
}

void reset_frame_skip(void)
{
    static Uint8 init=0;

    if (!init) {
	    autoframeskip		=	CF_BOOL(cf_get_item_by_name("autoframeskip"));
	    show_fps			=	CF_BOOL(cf_get_item_by_name("showfps"));
	    sleep_idle			=	CF_BOOL(cf_get_item_by_name("sleepidle"));
		do_bench			=	CF_BOOL(cf_get_item_by_name("bench"));
	    init=1;
    }
    init_tv.tv_usec = 0;
    init_tv.tv_sec = 0;
    skip_next_frame = 0;
    init_frame_skip = 1;
    if (conf.pal)
		CPU_FPS=50;
    //F = (uclock_t) ((double) TICKS_PER_SEC / CPU_FPS);
    F = (uclock_t) ((double) secs_to_ticks(1) / CPU_FPS);
}

int frame_skip(int init) 
{
    static int f2skip; /* frame to skip */
    static uclock_t sec = 0;
    static uclock_t current_time;
    static uclock_t target; /* when the frame should be drawn */
    static int nbFrame = 0;
    static int totalFrame = 0;
    static int skpFrm = 0;

    if (init_frame_skip) {
		init_frame_skip = 0;
		target 	= gettick();
		bench	= gettick();// (CF_BOOL(cf_get_item_by_name("bench"))?get_ticks():0);
		    
		nbFrame = 0;
		//f2skip=0;
		//skpFrm=0;
		sec = 0;
		return 0;
    }

    target += F;
    if (f2skip > 0) {
      f2skip--;
      skpFrm++;
      return 1;
    } else
      skpFrm = 0;
    

    current_time = gettick();

    if (1) { /* is frame rate controlling turned on */
      if (current_time < target && f2skip == 0) { /* do we need to wait some? */
        while (gettick() < target) { /* still need to wait? */
          if (sleep_idle) {
            usleep(10);	/* lets wait around a while, ok? */
          }
        }
      } else {
        f2skip = (current_time - target) / (double) F; /* how many frames do we need to skip to catch up? */
        if (f2skip > MAX_FRAMESKIP) {
          f2skip = MAX_FRAMESKIP;	/* too many */
          reset_frame_skip();
        }
      }
    }

    nbFrame++;
    if(do_bench && ++totalFrame>=1000) {
      printf("average fps=%f \n",((double)(totalFrame)/ticks_to_secs(gettick()-bench)));
      exit(0);
    }
    if (gettick() - sec >= secs_to_ticks(1)) {
		//printf("%d\n",nbFrame);
    if (show_fps)
      sprintf(fps_str, "%2d", nbFrame-1);
      nbFrame = 0;
      sec = gettick();
    }
    return 0;
}
