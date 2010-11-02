/*  gxgeo a neogeo emulator
 *  Copyright (C) 2008 Matt Jeffey
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

#pragma once
#ifndef __BITMAP_H__
#define __BITMAP_H__

  #include "video.h"

  #ifdef WORDS_BIGENDIAN
    #define SWAP_16(A) ((((unsigned short)(A) & 0xff00) >> 8) | \
                      (((unsigned short)(A) & 0x00ff) << 8))
    #define SWAP_32(A) ((((unsigned int)(A) & 0xff000000) >> 24) | \
                      (((unsigned int)(A) & 0x00ff0000) >> 8)  | \
                      (((unsigned int)(A) & 0x0000ff00) << 8)  | \
                      (((unsigned int)(A) & 0x000000ff) << 24))
  #else
    #define SWAP_16(A) (A)
    #define SWAP_32(A) (A)
  #endif
  
  #define RED_MASK    0xF800 // 1111 1000 0000 0000
  #define GREEN_MASK  0x07E0 // 0000 0111 1110 0000
  #define BLUE_MASK   0x001F // 0000 0000 0001 1111
  
  #define BF_TYPE 0x4D42             /* "MB" */
  #define BI_RGB       0             /* No compression - straight BGR data */
  #define BI_RLE8      1             /* 8-bit run-length compression */
  #define BI_RLE4      2             /* 4-bit run-length compression */
  #define BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

  typedef struct {
     //unsigned short type;                 /* Magic identifier            */
     unsigned int size;                       /* File size in bytes          */
     unsigned short reserved1, reserved2;
     unsigned int offset;                     /* Offset to image data, bytes */
  } HEADER;

  typedef struct {
     unsigned int size;               /* Header size in bytes      */
     int width,height;                /* Width and height of image */
     unsigned short planes;       /* Number of colour planes   */
     unsigned short bits;         /* Bits per pixel            */
     unsigned int compression;        /* Compression type          */
     unsigned int imagesize;          /* Image size in bytes       */
     int xresolution,yresolution;     /* Pixels per meter          */
     unsigned int ncolours;           /* Number of colours         */
     unsigned int importantcolours;   /* Important colours         */
  } INFOHEADER;
  
  int SaveBuffer(const char *filename, t_bitmap *data); 
  int SaveFullBuffer(const char *filename, t_bitmap *data);
#endif
