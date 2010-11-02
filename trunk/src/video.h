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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"

#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )
#define SWAP_4(x) ( ((x) << 24) | \
		 (((x) << 8) & 0x00ff0000) | \
		 (((x) >> 8) & 0x0000ff00) | \
		 ((x) >> 24) )
#define FIX_SHORT(x) (*(unsigned short *)&(x) = SWAP_2(*(unsigned short *)&(x)))
#define FIX_INT(x)   (*(unsigned int *)&(x)   = SWAP_4(*(unsigned int *)&(x)))
#define FIX_FLOAT(x) FIX_INT(x)

typedef struct
{
	int x;			/* X offset of viewport within bitmap */
	int y;			/* Y offset of viewport within bitmap */
	int w;			/* Width of viewport */
	int h;			/* Height of viewport */
	int ow;			/* Previous width of viewport */
	int oh;			/* Previous height of viewport */
	int changed;	/* 1= Viewport width or height have changed */
} rect;

typedef struct
{
	Uint8 		*data;			/* Bitmap data */
	int 		width;			/* Bitmap width (32+512+32) */
	int 		height;			/* Bitmap height (256) */
	int 		depth;			/* Color depth (8 bits) */
	int 		pitch;			/* Width of bitmap in bytes */
	int 		granularity;	/* Size of each pixel in bytes */
	rect		viewport; 	
	int 		remap;			/* 1= Translate pixel data */
} t_bitmap;

t_bitmap buffer;

#define RASTER_LINES 261

unsigned int neogeo_frame_counter;
extern unsigned int neogeo_frame_counter_speed;

void init_video(void);
void debug_draw_tile(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp);
void draw_screen_scanline(int start_line, int end_line, int refresh);
void draw_screen(void);
// void show_cache(void);
void convert_all_char(unsigned char *Ptr, int size, unsigned char *usage_ptr);
void convert_mgd2_tiles(unsigned char *buf, int len);
void convert_tile(int tileno);


#endif
