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

  #include <stdio.h>
  #include "bitmap.h"

  int SaveFullBuffer(const char *filename, t_bitmap *data)
  {
    FILE        *fp;          /* Open file pointer */
    int         bitsize;      /* Size of bitmap pixels */
    HEADER      header;       /* File header */
    INFOHEADER  infoheader;

    /* Try opening the file; use "wb" mode to write this *binary* file. */
    if ((fp = fopen(filename, "wb")) == NULL) return (-1);
        
    /* Figure out the bitmap size */
    bitsize = data->width * data->height * 3;

    /* Write the file header, bitmap information, and bitmap pixel data... */
    //header.type                   = SWAP_16(BF_TYPE);
    int size = sizeof(HEADER) + 2 + sizeof(INFOHEADER) + bitsize;
    header.size                   = SWAP_32(size);
    header.reserved1              = 0;
    header.reserved2              = 0;
    int offset = sizeof(HEADER) + 2 + sizeof(INFOHEADER);
    header.offset                 = SWAP_32(offset);
    
    infoheader.size               = SWAP_32(sizeof(INFOHEADER));
    infoheader.width              = SWAP_32(data->width);
    infoheader.height             = SWAP_32(data->height);
    infoheader.planes             = SWAP_16(1);
    infoheader.bits               = SWAP_16(24);
    infoheader.compression        = SWAP_32(BI_RGB);
    infoheader.imagesize          = SWAP_32(bitsize);
    infoheader.xresolution        = 0;
    infoheader.yresolution        = 0;
    infoheader.ncolours           = 0;
    infoheader.importantcolours   = 0;

    /*
    unsigned short *src1   = (unsigned short *)(  data->data + 
                                               ( (data->viewport.x) * data->granularity ) +
                                               ( (data->viewport.y + data->viewport.h-1) * data->pitch )
                                               );
    */
    unsigned short *src1;
    unsigned char  *buffer = (unsigned char *) malloc(bitsize);
    int w,h,i=0;
    for (h = data->height-1; h >= 0; h--) {
      src1   = (unsigned short *)( data->data + ( h * data->pitch ) );
      for (w = 0; w < data->width; w++) {
        unsigned short pixel = (*src1);
        unsigned char red_value   = ( pixel & RED_MASK) >> 11;
        unsigned char green_value = ( pixel & GREEN_MASK) >> 5;
        unsigned char blue_value  = ( pixel & BLUE_MASK);

        /* Expand to 8-bit values: */
        unsigned char red   = red_value << 3;
        unsigned char green = green_value << 2;
        unsigned char blue  = blue_value << 3;

        buffer[i++] = blue;
        buffer[i++] = green;
        buffer[i++] = red;      
        src1++;
      }

    }

    if (fwrite(&"BM", 1, 2, fp) < 2) {
      /* Couldn't write the file header - return... */
      fclose(fp);
      return (-1);
    }
    
    if (fwrite(&header, 1, sizeof(HEADER), fp) < sizeof(HEADER)) {
      /* Couldn't write the file header - return... */
      fclose(fp);
      return (-1);
    }

    if (fwrite(&infoheader, 1, sizeof(INFOHEADER), fp) < sizeof(INFOHEADER))
    {
      /* Couldn't write the bitmap header - return... */
      fclose(fp);
      return (-1);
    }

    if (fwrite(buffer, 1, bitsize, fp) < bitsize)
    {
      /* Couldn't write the bitmap - return... */
      fclose(fp);
      return (-1);
    }

    /* OK, everything went fine - return... */
    fclose(fp);
    free(buffer);
    return (0);
  }
  
  int SaveBuffer(const char *filename, t_bitmap *data)
  {
    FILE        *fp;          /* Open file pointer */
    int         bitsize;      /* Size of bitmap pixels */
    HEADER      header;       /* File header */
    INFOHEADER  infoheader;

    /* Try opening the file; use "wb" mode to write this *binary* file. */
    if ((fp = fopen(filename, "wb")) == NULL) return (-1);
        
    /* Figure out the bitmap size */
    bitsize = data->viewport.w * data->viewport.h * 3;

    /* Write the file header, bitmap information, and bitmap pixel data... */
    //header.type                   = SWAP_16(BF_TYPE);
    int size = sizeof(HEADER) + 2 + sizeof(INFOHEADER) + bitsize;
    header.size                   = SWAP_32(size);
    header.reserved1              = 0;
    header.reserved2              = 0;
    int offset = sizeof(HEADER) + 2 + sizeof(INFOHEADER);
    header.offset                 = SWAP_32(offset);
    
    infoheader.size               = SWAP_32(sizeof(INFOHEADER));
    infoheader.width              = SWAP_32(data->viewport.w);
    infoheader.height             = SWAP_32(data->viewport.h);
    infoheader.planes             = SWAP_16(1);
    infoheader.bits               = SWAP_16(24);
    infoheader.compression        = SWAP_32(BI_RGB);
    infoheader.imagesize          = SWAP_32(bitsize);
    infoheader.xresolution        = 0;
    infoheader.yresolution        = 0;
    infoheader.ncolours           = 0;
    infoheader.importantcolours   = 0;

    /*
    unsigned short *src1   = (unsigned short *)(  data->data + 
                                               ( (data->viewport.x) * data->granularity ) +
                                               ( (data->viewport.y + data->viewport.h-1) * data->pitch )
                                               );
    */
    unsigned short *src1;
    unsigned char  *buffer = (unsigned char *) malloc(bitsize);
    int w,h,i=0;
    for (h = data->viewport.h-1; h >= 0; h--) {
      src1   = (unsigned short *)(  data->data + 
                                 ( (data->viewport.x) * data->granularity ) +
                                 ( (data->viewport.y + h) * data->pitch )
                                 );
      for (w = 0; w < data->viewport.w; w++) {
        unsigned short pixel = (*src1);
        unsigned char red_value   = ( pixel & RED_MASK) >> 11;
        unsigned char green_value = ( pixel & GREEN_MASK) >> 5;
        unsigned char blue_value  = ( pixel & BLUE_MASK);

        /* Expand to 8-bit values: */
        unsigned char red   = red_value << 3;
        unsigned char green = green_value << 2;
        unsigned char blue  = blue_value << 3;

        buffer[i++] = blue;
        buffer[i++] = green;
        buffer[i++] = red;      
        src1++;
      }

    }

    if (fwrite(&"BM", 1, 2, fp) < 2) {
      /* Couldn't write the file header - return... */
      fclose(fp);
      return (-1);
    }
    
    if (fwrite(&header, 1, sizeof(HEADER), fp) < sizeof(HEADER)) {
      /* Couldn't write the file header - return... */
      fclose(fp);
      return (-1);
    }

    if (fwrite(&infoheader, 1, sizeof(INFOHEADER), fp) < sizeof(INFOHEADER))
    {
      /* Couldn't write the bitmap header - return... */
      fclose(fp);
      return (-1);
    }

    if (fwrite(buffer, 1, bitsize, fp) < bitsize)
    {
      /* Couldn't write the bitmap - return... */
      fclose(fp);
      return (-1);
    }

    /* OK, everything went fine - return... */
    fclose(fp);
    free(buffer);
    return (0);
  }
