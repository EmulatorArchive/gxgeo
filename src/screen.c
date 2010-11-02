/*  gxgeo a neogeo emulator
 *  Copyright (C) 2008 Matt Jeffery
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
 
#include "gx_supp.h"
#include <gccore.h>

extern GXRModeObj *vmode;
extern u32 *xfb[2];
extern t_bitmap buffer;
static int GX_started = 0;

bool screen_init()
{
  VIDEO_Init();
  GX_InitVideo();
  console_init(xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);
  
  return true;
}

void screen_update() { 
  if (GX_started == 0) {
    /* over type casting maybe */
    GX_Start(buffer.viewport.w, buffer.viewport.h, (int)(((double)buffer.viewport.w/(double)buffer.viewport.h)*(double)vmode->viHeight)/2, vmode->viHeight/2); 
    GX_started = 1;
  }
  GX_Render2(&buffer);
}

void screen_close() { GX_started = 0; }
void screen_restart() { GX_InitVideo(); console_init(xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2); }
