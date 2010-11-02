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
#include "messages.h"
#include "video.h"
#include "emu.h"
#include "timer.h"
#include "frame_skip.h"
#include "screen.h"
#include <stdarg.h>


static int font_w=8;
static int font_h=9;

/* TODO - Rewrite this. */ 

void text_message(int x, int y, char *string)
{
	// Draw message on frame
}

void draw_message(char *string)
{
    strcpy(conf.message, string);
    conf.do_message = 75;
}
