 /*  gngeo sound manager / mmap simulator ;)
  *  Copyright (C) 2008 Matt Jeffery
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
  
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include "wiialloc.h"
	#include "sound_man.h"
	#include "memory.h"
  
  typedef struct {
    unsigned char *start;
    size_t size;
    unsigned char *mapped;  
  } sound_mem;
  
  unsigned char *load_sound(unsigned char *dst, int offset, size_t size)
  {
    if (dst) {
      
      
    }
    return dst;
  }
  
 