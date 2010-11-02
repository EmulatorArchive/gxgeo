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
  
  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include "wiialloc.h"
  #include "gfx_man.h"
  #include "pbar.h"
  #include "memory.h"
  
    
  #define TILE_SIZE 0x80
  #define TILE_NUM  0x40000
  
  #define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )
  #define SWAP_4(x) ( ((x) << 24) | \
           (((x) << 8) & 0x00ff0000) | \
           (((x) >> 8) & 0x0000ff00) | \
           ((x) >> 24) )
  #define FIX_SHORT(x) (*(unsigned short *)&(x) = SWAP_2(*(unsigned short *)&(x)))
  #define FIX_INT(x)   (*(unsigned int *)&(x)   = SWAP_4(*(unsigned int *)&(x)))
  #define FIX_FLOAT(x) FIX_INT(x)
  
  static int              tile_count      = 0;
  static int              gfx_size        = 0;
  static unsigned int     next_to_use     = 0;
  static FILE             *gfx_dump_file  = NULL;
  static unsigned char    *tile_data      = NULL;
  
  /* maps memory location of tile number */
  static unsigned int     tile_info[TILE_NUM];
  
  /* maps tile number to memory location */
  static unsigned int     *gfx_manager  = NULL;
  
  int init_gfx_manager(char *file_name, size_t size)
  {
    int ti = 0;
    /* if a file is already open, close it */
    if (gfx_dump_file) fclose(gfx_dump_file);
    
    /* open the requested gfx dump */
    gfx_dump_file = fopen(file_name, "rb");
    
    /* if the file cannot be opened return 0*/
    if (!gfx_dump_file) return 0;
    
    /* each tile is 128 bytes */
    gfx_size   = size;
    tile_count = (gfx_size >> 7);
  
    /* allocate the gfx manager */
    if (gfx_manager) free(gfx_manager);
    if (tile_data) free2(tile_data);
    
    /* the gfx_manager stores the location in tile_data
       of all the tiles, or 0 if it is not loaded       */
    gfx_manager = (unsigned int*) calloc( sizeof(unsigned int), tile_count );
    if (!gfx_manager) return 0;
    int i;
    for( i = 0; i < TILE_NUM && i < tile_count; i++ ) {
      tile_info[i] = 0;
      gfx_manager[i] = 0;
    }
    /* tile_data stores all the tiles that can be stored */
    tile_data = (unsigned char*) malloc2( TILE_SIZE * TILE_NUM );
    //CHECK_ALLOC(tile_data);
    if (!tile_data) return 0;
    /* rewind ( gfx_dump_file ); */
    /*
    create_progress_bar("Load GFX Dump");
    for (ti = 0; ti < TILE_NUM; ti++) {
      if (fread (&tile_data[i*TILE_SIZE], TILE_SIZE, 1, gfx_dump_file) != 1) {
        terminate_progress_bar();
        return 0;
      }
      update_progress_bar(ti, TILE_NUM);
    }
    terminate_progress_bar();
    */
    return 1;
  }
  
  unsigned char *get_tile(int tileno)
  {
    if (!gfx_dump_file || !gfx_manager) {
      printf("\t\tCannot access tiles\n");
      sleep(5);
      exit(1);
    }

    /* if the tile is currently in memory return it */
    if (gfx_manager[tileno] != 0) return &tile_data[gfx_manager[tileno] * TILE_SIZE];
    else { /* page fault ;) */
      /* calculate the tile offset */
      int file_offset = tileno << 7;
      next_to_use++;
      next_to_use = next_to_use % TILE_NUM;
      
      /* the tile in position next_to_use has been removed from memory */
      gfx_manager[tile_info[next_to_use]] = 0;
      
      /* tile number tileno is in position next_to_use */
      gfx_manager[tileno] = next_to_use;
      
      /* the tile in postion next_to_use is tile number tileno*/
      tile_info[next_to_use] = tileno;
      
      fseek( gfx_dump_file, file_offset, SEEK_SET );
      /* read the data in to the space provided */
      ///if (fread (&tile_data[next_to_use * TILE_SIZE], TILE_SIZE, 1, gfx_dump_file) != 1) return 0;
      int ti;
      
      for (ti = 0; ti < TILE_SIZE >> 2; ti++) {
        if (fread (&tile_data[next_to_use+(ti<<2)], sizeof(unsigned int), 1, gfx_dump_file) != 1) return 0;
        unsigned int t = tile_data[next_to_use+(ti<<2)];
        tile_data[next_to_use+(ti<<2)] = FIX_INT(t);
      }
      
      //convert_tile(tileno);
      return &tile_data[next_to_use * TILE_SIZE];
    }
  }
  
  void close_gfx_manager()
  {
    /* close the file */
    if (gfx_dump_file) fclose(gfx_dump_file);
    /* free all the memory allocated for the tiles */
    free(gfx_manager);
    free2(tile_data);
  }
  
  int read_pen_usage(void* pusage)
  {
    if (!gfx_dump_file)  return 0;
    /* seek to gfx_size and read tile_count*sizeof(unsigned int) bytes int to pusage */ 
    fseek( gfx_dump_file, gfx_size, SEEK_SET );
    int ti = 0;
    create_progress_bar("Load Pen Usage");
    for (ti = 0; ti < tile_count; ti++) {
      if (fread (&(((unsigned int*)pusage)[ti]), sizeof(unsigned int), 1, gfx_dump_file) != 1) {
        terminate_progress_bar();
        return 0;
      }
      unsigned int t = ((unsigned int*)pusage)[ti];
      ((unsigned int*)pusage)[ti] = FIX_INT(t);
      update_progress_bar(ti, tile_count);
    }
    terminate_progress_bar();
    return 1;
  }
  
  unsigned char read_byte_file(int offset)
  {
    if (!gfx_dump_file) return 0;
    fseek( gfx_dump_file, offset, SEEK_SET );
    return fgetc (gfx_dump_file);
  }

