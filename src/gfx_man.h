 /*  gngeo gfx manager / mmap simulator ;)
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
	#pragma once
  
	#include <stdlib.h>
	#include <stdio.h>
	
	int init_gfx_manager(char *file_name, size_t gfx_size);
	void close_gfx_manager();
	int read_pen_usage(void* pusage);
	unsigned char *get_tile(int tileno);
	unsigned char read_byte_file(int offset);
