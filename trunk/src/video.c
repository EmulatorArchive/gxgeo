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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "conf.h"
#include "types.h"
#include "video.h"
#include "memory.h"
#include "emu.h"
#include "messages.h"
#include "screen.h"
#include "frame_skip.h"
#include "transpack.h"
#include "pbar.h"
#include "driver.h"
#include "gfx_man.h"

extern int neogeo_fix_bank_type;

Uint8 strip_usage[0x300];

char *ldda_y_skip;
char *dda_x_skip;
char ddaxskip[16][16] =
{
    { 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0 },
    { 0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0 },
    { 0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
    { 1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0 },
    { 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1 },
    { 1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
    { 1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
};
Uint32 ddaxskip_i[16] ={
	0x0080,	0x0880,	0x0888,	0x2888,	0x288a,	0x2a8a,	0x2aaa,	0xaaaa,
	0xaaea,	0xbaea,	0xbaeb,	0xbbeb,	0xbbef,	0xfbef,	0xfbff,	0xffff
};
Uint32 dda_x_skip_i;

static __inline__ Uint16 alpha_blend(Uint16 dest,Uint16 src,Uint8 a)
{
    static Uint8 dr,dg,db,sr,sg,sb;
  
    dr=((dest&0xF800)>>11)<<3;
    dg=((dest&0x7E0)>>5)<<2;
    db=((dest&0x1F))<<3;

    sr=((src&0xF800)>>11)<<3;
    sg=((src&0x7E0)>>5)<<2;
    sb=((src&0x1F))<<3;
  
    dr = (((sr-dr)*(a))>>8)+dr;
    dg = (((sg-dg)*(a))>>8)+dg;
    db = (((sb-db)*(a))>>8)+db;
  
    return ((dr>>3)<<11)|((dg>>2)<<5)|(db>>3);
}
#define BLEND16_50(a,b) ((((a)&0xf7de)>>1)+(((b)&0xf7de)>>1))
#define BLEND16_25(a,b) alpha_blend(a,b,63)

char dda_y_skip[17];
Uint32 dda_y_skip_i;
Uint32 full_y_skip_i=0xFFFE;
char full_y_skip[16]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
unsigned int neogeo_frame_counter_speed=8;
static Uint16 fix_addr[40][32];
static Uint8 fix_shift[40];

static void fix_value_init(void) {
    int x, y;
    for(x=0;x<40;x++) {
        for(y=0;y<32;y++) {
            fix_addr[x][y] = 0xea00 + (y << 1) + 64 * (x/6);
        }
        fix_shift[x] = (5-(x%6));
    }
}

#define fix_add(x, y) (0x1000 * (((READ_WORD(&memory.video[fix_addr[x][y-1]]) >> fix_shift[x] * 2) & 3) ^ 3))


void convert_tile(int tileno)
{
    unsigned char swap[128];
    unsigned int *gfxdata;
    int x,y;
    unsigned int pen,filed;
    TRANS_PACK *t;

	if (!memory.gfx) gfxdata = (unsigned int *)get_tile(tileno);
	else gfxdata = (unsigned int *)&memory.gfx[ tileno << 7 ];
  
    memory.pen_usage[tileno] = 0;
  
    memcpy(swap,gfxdata,128);

    filed=1;
    for (y = 0;y < 16;y++) {
        unsigned int dw;
    
        dw = 0;
        for (x = 0;x < 8;x++)
        {
            pen  = ((swap[64 + (y<<2) + 3] >> x) & 1) << 3;
            pen |= ((swap[64 + (y<<2) + 1] >> x) & 1) << 2;
            pen |= ((swap[64 + (y<<2) + 2] >> x) & 1) << 1;
            pen |=  (swap[64 + (y<<2)    ] >> x) & 1;
			if (!pen) filed=0;
            dw |= pen << ((7-x)<<2);
            memory.pen_usage[tileno]  |= (1 << pen);
        }

		/*if (memory.gfx)*/ *(gfxdata++) = dw;
     
        dw = 0;
        for (x = 0;x < 8;x++)
        {
            pen  = ((swap[(y<<2) + 3] >> x) & 1) << 3;
            pen |= ((swap[(y<<2) + 1] >> x) & 1) << 2;
            pen |= ((swap[(y<<2) + 2] >> x) & 1) << 1;
            pen |=  (swap[(y<<2)    ] >> x) & 1;
			if (!pen) filed=0;
            dw |= pen << ((7-x)<<2);
            memory.pen_usage[tileno]  |= (1 << pen);
        }

		/*if (memory.gfx)*/ *(gfxdata++) = dw;
    }
    /* TODO: CHECK if it is really faster...
      if (filed==1) {
      memory.pen_usage[tileno]=TILE_FULL;
      } else 
    */
    if ((memory.pen_usage[tileno] & ~1) == 0) {
        memory.pen_usage[tileno]=TILE_INVISIBLE;
    } else {
		t=trans_pack_find(tileno);
		if (t!=NULL) {
			if (t->type==1)
				memory.pen_usage[tileno]=TILE_TRANSPARENT25;
			else {
				if (t->type==2)
					memory.pen_usage[tileno]=TILE_TRANSPARENT50;
				else
					memory.pen_usage[tileno]=TILE_NORMAL;
			}
		} else {
			memory.pen_usage[tileno]=TILE_NORMAL;
		}
    }
  
}

void convert_all_char(Uint8 *Ptr, int size, Uint8 *usage_ptr)
{
    int		i,j;
    unsigned char	usage;
    
    Uint8 *Src;
    Uint8 *sav_src;

    Src=(Uint8*)malloc(size);
    if (!Src) {
		printf("Not enought memory!!\n");
		return;
    }
    sav_src=Src;
    memcpy(Src,Ptr,size);
#ifdef WORDS_BIGENDIAN
#define CONVERT_TILE 	*Ptr++ = *(Src+8);\
						usage |= *(Src+8);\
						*Ptr++ = *(Src);\
						usage |= *(Src);\
						*Ptr++ = *(Src+24);\
						usage |= *(Src+24);\
						*Ptr++ = *(Src+16);\
						usage |= *(Src+16);\
						Src++;
#else
#define CONVERT_TILE 	*Ptr++ = *(Src+16);\
						usage |= *(Src+16);\
						*Ptr++ = *(Src+24);\
						usage |= *(Src+24);\
						*Ptr++ = *(Src);\
						usage |= *(Src);\
						*Ptr++ = *(Src+8);\
						usage |= *(Src+8);\
						Src++;
#endif
    for(i=size;i>0;i-=32) {
        usage = 0;
        for (j=0;j<8;j++) {
            CONVERT_TILE
                }
        Src+=24;
        *usage_ptr++ = usage;
    }
    free(sav_src);
#undef CONVERT_TILE
}

/* For MGD-2 dumps */
static int mgd2_tile_pos=0;
void convert_mgd2_tiles(unsigned char *buf,int len)
{
    int i;
    unsigned char t;

    if (len==memory.gfx_size && mgd2_tile_pos==memory.gfx_size) {
		mgd2_tile_pos=0;
    }
    if (len == 2) {
		return;
    }

    if (len == 6)
    {
        unsigned char swp[6];

        memcpy(swp,buf,6);
        buf[0] = swp[0];
        buf[1] = swp[3];
        buf[2] = swp[1];
        buf[3] = swp[4];
        buf[4] = swp[2];
        buf[5] = swp[5];

        return;
    }

    if (len % 4) exit(1);	/* must not happen */

    len /= 2;

    for (i = 0;i < len/2;i++)
    {
        t = buf[len/2 + i];
        buf[len/2 + i] = buf[len + i];
        buf[len + i] = t;
    }
    if (len==2) {
	mgd2_tile_pos+=2;
	if ((mgd2_tile_pos&0x3f)==0)  update_progress_bar(mgd2_tile_pos,memory.gfx_size);
    }
    convert_mgd2_tiles(buf,len);
    convert_mgd2_tiles(buf + len,len);
}




/* Drawing function generation */
#define RENAME(name) name##_tile
#define PUTPIXEL(dst,src) dst=src
#include "video_template.h"

#define RENAME(name) name##_tile_50
#define PUTPIXEL(dst,src) dst=BLEND16_50(src,dst)
#include "video_template.h"

#define RENAME(name) name##_tile_25
#define PUTPIXEL(dst,src) dst=BLEND16_25(src,dst)
#include "video_template.h"

/* Drawing function (debug) */
#define DEBUG_VIDEO
#define RENAME(name) name##_debug
#define PUTPIXEL(dst,src) dst=src
#include "video_template.h"
#undef DEBUG_VIDEO
#define PUTPIXEL(dst,src) dst=src
static __inline__ void draw_tile_full(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp)
{
    unsigned int *gfxdata,myword;
    int y;
    unsigned char col;
    unsigned short *br;
    unsigned int *paldata=(unsigned int *)&current_pc_pal[16*color];
    char *l_y_skip;
    int l; // Line skipping counter
#ifdef DEBUG_VIDEO
    int buf_w=544-zx;
    int buf_w_yflip=544+zx;
#else
    int buf_w=(buffer.pitch>>1)-zx;
    int buf_w_yflip=(buffer.pitch>>1)+zx;
#endif
    tileno=tileno%memory.nb_of_tiles;
   
    //gfxdata = (unsigned int *)&memory.gfx[ tileno<<7];

	if (!memory.gfx) gfxdata = (unsigned int *)get_tile(tileno);
	else gfxdata = (unsigned int *)&memory.gfx[ tileno << 7 ];
	
    /* y zoom table */
    if(zy==16)
        l_y_skip=full_y_skip;
    else
        l_y_skip=dda_y_skip;

    if (zx==16) {
        if (xflip) { 
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    
                    br[0]=paldata[(myword>>0)&0xf];
                    br[1]=paldata[(myword>>4)&0xf];
                    br[2]=paldata[(myword>>8)&0xf];
                    br[3]=paldata[(myword>>12)&0xf];
                    br[4]=paldata[(myword>>16)&0xf];
                    br[5]=paldata[(myword>>20)&0xf];
                    br[6]=paldata[(myword>>24)&0xf];
                    br[7]=paldata[(myword>>28)&0xf];
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    br[8]=paldata[(myword>>0)&0xf];
                    br[9]=paldata[(myword>>4)&0xf];
                    br[10]=paldata[(myword>>8)&0xf];
                    br[11]=paldata[(myword>>12)&0xf];
                    br[12]=paldata[(myword>>16)&0xf];
                    br[13]=paldata[(myword>>20)&0xf];
                    br[14]=paldata[(myword>>24)&0xf];
                    br[15]=paldata[(myword>>28)&0xf];
#ifdef DEBUG_VIDEO
                    br-=544;
#else
                    br-=(buffer.pitch>>1);
#endif
                    l++;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
					br[0]=paldata[(myword>>0)&0xf];
                    br[1]=paldata[(myword>>4)&0xf];
                    br[2]=paldata[(myword>>8)&0xf];
                    br[3]=paldata[(myword>>12)&0xf];
                    br[4]=paldata[(myword>>16)&0xf];
                    br[5]=paldata[(myword>>20)&0xf];
                    br[6]=paldata[(myword>>24)&0xf];
                    br[7]=paldata[(myword>>28)&0xf];
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    br[8]=paldata[(myword>>0)&0xf];
                    br[9]=paldata[(myword>>4)&0xf];
                    br[10]=paldata[(myword>>8)&0xf];
                    br[11]=paldata[(myword>>12)&0xf];
                    br[12]=paldata[(myword>>16)&0xf];
                    br[13]=paldata[(myword>>20)&0xf];
                    br[14]=paldata[(myword>>24)&0xf];
                    br[15]=paldata[(myword>>28)&0xf];
#ifdef DEBUG_VIDEO
                    br+=544;
#else
                    br+=(buffer.pitch>>1);
#endif
                    l++;
		
                }
            }
        }else {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    br[0]=paldata[(myword>>28)&0xf];
                    br[1]=paldata[(myword>>24)&0xf];
                    br[2]=paldata[(myword>>20)&0xf];
                    br[3]=paldata[(myword>>16)&0xf];
                    br[4]=paldata[(myword>>12)&0xf];
                    br[5]=paldata[(myword>>8)&0xf];
                    br[6]=paldata[(myword>>4)&0xf];
                    br[7]=paldata[(myword>>0)&0xf];
	      
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    br[8]=paldata[(myword>>28)&0xf];
                    br[9]=paldata[(myword>>24)&0xf];
                    br[10]=paldata[(myword>>20)&0xf];
                    br[11]=paldata[(myword>>16)&0xf];
                    br[12]=paldata[(myword>>12)&0xf];
                    br[13]=paldata[(myword>>8)&0xf];
                    br[14]=paldata[(myword>>4)&0xf];
                    br[15]=paldata[(myword>>0)&0xf];
                    l++;
#ifdef DEBUG_VIDEO
                    br-=544;
#else
                    br-=(buffer.pitch>>1);
#endif
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                   br[0]=paldata[(myword>>28)&0xf];
                    br[1]=paldata[(myword>>24)&0xf];
                    br[2]=paldata[(myword>>20)&0xf];
                    br[3]=paldata[(myword>>16)&0xf];
                    br[4]=paldata[(myword>>12)&0xf];
                    br[5]=paldata[(myword>>8)&0xf];
                    br[6]=paldata[(myword>>4)&0xf];
                    br[7]=paldata[(myword>>0)&0xf];
	      
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    br[8]=paldata[(myword>>28)&0xf];
                    br[9]=paldata[(myword>>24)&0xf];
                    br[10]=paldata[(myword>>20)&0xf];
                    br[11]=paldata[(myword>>16)&0xf];
                    br[12]=paldata[(myword>>12)&0xf];
                    br[13]=paldata[(myword>>8)&0xf];
                    br[14]=paldata[(myword>>4)&0xf];
                    br[15]=paldata[(myword>>0)&0xf];

                    l++;
#ifdef DEBUG_VIDEO
                    br+=544;
#else
                    br+=(buffer.pitch>>1);
#endif
                }
            }
        }
    }else { // zx!=16
        if (xflip) {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 1]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 2]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 3]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 4]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 5]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 6]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 7]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}

                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    if (dda_x_skip[ 8]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[ 9]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[10]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[11]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[12]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[13]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[14]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    if (dda_x_skip[15]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;}
                    br-=buf_w_yflip;
                    l++;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {
                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    if (dda_x_skip[ 0]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 

                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    if (dda_x_skip[ 8]) {if  ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if  ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if  ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if  ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if  ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if  ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if  ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if  ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 

                    br+=buf_w;
                    l++;
                }
            }
        }else {
            l=0;
            if (yflip) {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+((zy-1)+sy)*544+sx;
#else
                br= (unsigned short *)bmp+((zy-1)+sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {

                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	      
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    if (dda_x_skip[ 8]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    l++; 
                    br-=buf_w_yflip;
                }
            } else {
#ifdef DEBUG_VIDEO
                br= (unsigned short *)bmp+(sy)*544+sx;
#else
                br= (unsigned short *)bmp+(sy)*(buffer.pitch>>1)+sx;
#endif
                for(y=0;y<zy;y++) {

                    gfxdata+=l_y_skip[l]<<1;
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[0]); else myword = gfxdata[0];
                    if (dda_x_skip[ 0]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 1]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 2]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 3]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 4]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 5]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 6]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 7]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
	      
                    if (!memory.gfx) myword = /*FIX_INT*/(gfxdata[1]); else myword = gfxdata[1];
                    if (dda_x_skip[ 8]) {if ((col=((myword>>28)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[ 9]) {if ((col=((myword>>24)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[10]) {if ((col=((myword>>20)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[11]) {if ((col=((myword>>16)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[12]) {if ((col=((myword>>12)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[13]) {if ((col=((myword>>8)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[14]) {if ((col=((myword>>4)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    if (dda_x_skip[15]) {if ((col=((myword>>0)&0xf))) PUTPIXEL(*br,paldata[col]);br++;} 
                    l++;
                    br+=buf_w;
                }
            }
        }
    }
}

void debug_draw_tile(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp) {
    draw_debug(tileno,sx,sy,zx,zy,color,xflip,yflip,bmp);
}

static __inline__ void draw_fix_char(unsigned char *buf,int start,int end)
{
    unsigned int *gfxdata,myword;
    int x,y,yy;
    unsigned char col;
    unsigned short *br;
    unsigned int *paldata;
    unsigned int byte1,byte2;
    int banked,garouoffsets[32];
    int ystart=1,yend=32;
    
    //banked = (current_fix == memory.sfix_game && memory.sfix_size > 0x1000) ? 1 : 0;
    banked = (current_fix == memory.sfix_game && neogeo_fix_bank_type && memory.sfix_size > 0x1000) ? 1 : 0;
    //if (banked && conf.rom_type==MVS_CMC42)
    if (banked && neogeo_fix_bank_type == 1) {
	    int garoubank = 0;
	    int k = 0;
	    y = 0;
	    
	    while (y < 32)
	    {
		    if (READ_WORD(&memory.video[0xea00+(k<<1)]) == 0x0200 && 
			(READ_WORD(&memory.video[0xeb00+(k<<1)]) & 0xff00) == 0xff00) {
			    
			    garoubank = READ_WORD(&memory.video[0xeb00+(k<<1)]) & 3;
			    garouoffsets[y++] = garoubank;
		    }
		    garouoffsets[y++] = garoubank;
		    k += 2;
	    }
    }
    if (start!=0 && end!=0) {
		ystart=start>>3;
		yend=(end>>3)+1;
		if (ystart<1) ystart=1;
		buffer.viewport.x	=	0;
		buffer.viewport.y	=	start+16;
		buffer.viewport.w	=	buffer.width;
		buffer.viewport.h	=	(end-start)+16;
    }

    for(y=ystart;y<yend;y++)
        for(x=0;x<40;x++)
        {
	    byte1 = (READ_WORD(&memory.video[0xE000 + ((y + (x<<5))<<1)]));
            byte2 = byte1 >> 12;
            byte1 = byte1 & 0xfff;

	    if (banked) {
			switch (neogeo_fix_bank_type)
			{
				case 1:
					/* Garou, MSlug 3 */
					byte1 += 0x1000 * (garouoffsets[(y-2)&31] ^ 3);
					break;
				case 2:
					byte1 += fix_add(x, y); 
					/* byte1 += 0x1000 * (((READ_WORD(&memory.video[(0xea00 >> 1) + ((y-1)&31) + 32 * (x/6)])
					   >> (5-(x%6))*2) & 3) ^ 3);*/
					break;
			}
	    }

	    if ((byte1>=(memory.sfix_size>>5)) || (fix_usage[byte1]==0x00)) continue;

            br=(unsigned short*)buf+((y<<3))*buffer.width+(x<<3)+16;

            paldata=(unsigned int *)&current_pc_pal[16*byte2];
            gfxdata = (unsigned int *)&current_fix[ byte1<<5];
	
            for(yy=0;yy<8;yy++)
            {
                myword = gfxdata[yy];
                col=(myword>>28)&0xf; if (col) br[7]=paldata[col];
                col=(myword>>24)&0xf; if (col) br[6]=paldata[col];
                col=(myword>>20)&0xf; if (col) br[5]=paldata[col];
                col=(myword>>16)&0xf; if (col) br[4]=paldata[col];
                col=(myword>>12)&0xf; if (col) br[3]=paldata[col];
                col=(myword>>8)&0xf; if (col) br[2]=paldata[col];
                col=(myword>>4)&0xf; if (col) br[1]=paldata[col];
                col=(myword>>0)&0xf; if (col) br[0]=paldata[col];
                br+=buffer.width;
			}
        }
    if (start!=0 && end!=0) {
		buffer.viewport.x = 0;
		buffer.viewport.y = 0;
		buffer.viewport.w = 0;
		buffer.viewport.h = 0;
	}
}

void fill_rect(t_bitmap *bitmap, rect *view, Uint32 color)
{
	rect v;
	int x,y;
	if (bitmap) {
		if (view == NULL) {
			v.w = bitmap->width;
			v.h = bitmap->height;
			v.x = 0;
			v.y = 0;
		}
		else  {
			v.w = view->w;
			v.h = view->h;
			v.x = view->x;
			v.y = view->y;
		}
		for (y = v.y; y < v.y+v.h; y++) /* for each row */
			for (x = v.x; x < v.x+v.w; x++) /* for each col */
				((Uint16*)bitmap->data)[(y*bitmap->width) + x] = (Uint16)color;
	}

}

void draw_screen(void)
{
    int sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1;
    unsigned int offs,i,count,y;
    unsigned int tileno,tileatr,t1,t2,t3;
    char fullmode=0;
    int ddax=0,dday=0,rzx=15,yskip=0;
    unsigned char *vidram=memory.video;
    //    int drawtrans=0;

    fill_rect(&buffer,NULL,current_pc_pal[4095]);
	
    /* Draw sprites */
    for (count=0;count<0x300;count+=2) {
        t3 = READ_WORD( &vidram[0x10000 + count] );
        t1 = READ_WORD( &vidram[0x10400 + count] );
        t2 = READ_WORD( &vidram[0x10800 + count] );

	
	/* If this bit is set this new column is placed next to last one */
        if (t1 & 0x40) {
            sx += rzx;            /* new x */
//            if ( sx >= 0x1F0 )    /* x>496 => x-=512 */ 
//                sx -= 0x200;

	    /* Get new zoom for this column */
            zx = (t3 >> 8) & 0x0f;
            sy = oy;                /* y pos = old y pos */
        } else {	/* nope it is a new block */
	    /* Sprite scaling */
            zx = (t3 >> 8) & 0x0f;    /* zomm x */
            rzy = t3 & 0xff;          /* zoom y */
            if (rzy==0) continue;
            sx = (t2 >> 7);           /* x pos */
	    /*	    
		    drawtrans=0;
		    if (t2&0x7f) {
		    printf("t2 0-6 set to %x for strip %x\n",t2&0x7f,count>>1);
		    drawtrans=1;
		    }*/
      
	    /* Number of tiles in this strip */
            my = t1 & 0x3f;

            if (my == 0x20) fullmode = 1;
            else if (my >= 0x21) fullmode = 2;     /* most games use 0x21, but */
	    /* Alpha Mission II uses 0x3f */
            else fullmode = 0;

            sy = 0x200 - (t1 >> 7); /* sprite bank position */

            if (sy > 0x110) sy -= 0x200;
      
            if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
            {
                while (sy < 0) sy += ((rzy + 1)<<1);
            }

            oy = sy; /* on se souvient de la position y */
	
	
            if (rzy < 0xff && my < 0x10 && my){
		//my = (my<<8)/(rzy+1);
                my = my*255/rzy;
                if (my > 0x10) my = 0x10;
            }
      
            if (my > 0x20) my=0x20;
      
            ddax=0;	/* =16; NS990110 neodrift fix */	/* setup x zoom */
        }
	
	/* No point doing anything if tile strip is 0 */
        if (my==0) continue;
    
	/* Process x zoom */
        if(zx!=15) {
            dda_x_skip=ddaxskip[zx];
            rzx=zx+1;
      
        }
        else rzx=16;
	
	if ( sx >= 0x1F0 ) sx -= 0x200;
        if(sx>=320) continue;
	//if (sx<-16) continue;

	/* Setup y zoom */
        if(rzy==255)
            yskip=16;
        else
            dday=0;	/* =256; NS990105 mslug fix */

        offs = count<<6;

	/* my holds the number of tiles in each vertical multisprite block */
        for (y=0; y < my ;y++) {
            tileno  = READ_WORD(&vidram[offs]);
            offs+=2;
            tileatr = READ_WORD(&vidram[offs]);
            offs+=2;

            if (memory.nb_of_tiles>0x10000 && tileatr&0x10) tileno+=0x10000;
            if (memory.nb_of_tiles>0x20000 && tileatr&0x20) tileno+=0x20000;
            if (memory.nb_of_tiles>0x40000 && tileatr&0x40) tileno+=0x40000;

	    /* animation automatique */
	    /*if (tileatr&0x80) printf("PLOP\n");*/
            if (tileatr&0x8) {
		tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
	    } else {
		if (tileatr&0x4) {
		    tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);
		}
	    }

	    if (tileno>memory.nb_of_tiles) {
		
		continue;
	    }
      

            if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
            {
	  
                if (sy >= 248) sy -= ((rzy + 1)<<1);
            }
            else if (fullmode == 1)
            {
	  
                if (y == 0x10) sy -= ((rzy + 1)<<1);
            }
            else if (sy > 0x110) sy -= 0x200;	// NS990105 mslug2 fix


     
            if(rzy!=255)
            {
		    yskip=0;dda_y_skip_i=0;
		    dda_y_skip[0]=0;
		    for(i=0;i<16;i++)
		    {
			    dda_y_skip[i+1]=0;
			    dday-=(rzy+1);
			    if(dday<=0)
			    {
				    dday+=256;
				    yskip++;
				    dda_y_skip[yskip]++;
			    }
			    else dda_y_skip[yskip]++;
		    
			    //if (dda_y_skip[i])
			    //	    dda_y_skip_i=dda_y_skip_i|(1<<i); 
		    }
		    //printf("%04x\n",dda_y_skip_i);

            }
	    
      
            if (sx >= -16 && sx+15 < 336 && sy>=0 && sy+15 <256) {
		    /*
		      if (memory.pen_usage[tileno]==TILE_UNCONVERTED) {
		      convert_tile(tileno);
		      }
		    */
			
			//printf("\t\t#%i == %i\n",tileno,memory.pen_usage[tileno]);
			//sleep(10);
			//exit(1);

		        switch (memory.pen_usage[tileno]) {
/*
				case TILE_FULL:
					draw_tile_full(tileno,sx+16,sy,rzx,yskip,tileatr>>8,
						       tileatr & 0x01,tileatr & 0x02,
						       (unsigned char*)buffer.data);
					break;
*/
				case TILE_NORMAL:
				    draw_tile(tileno,sx+16,sy,rzx,yskip,tileatr>>8,
					      tileatr & 0x01,tileatr & 0x02,
					      (unsigned char*)buffer.data);
				    break;
				case TILE_TRANSPARENT50:
				    draw_tile_50(tileno,sx+16,sy,rzx,yskip,tileatr>>8,
						 tileatr & 0x01,tileatr & 0x02,
						 (unsigned char*)buffer.data);
				    break;
				case TILE_TRANSPARENT25:
				    draw_tile_25(tileno,sx+16,sy,rzx,yskip,tileatr>>8,
						 tileatr & 0x01,tileatr & 0x02,
						 (unsigned char*)buffer.data);
				    break;
		        }
	    }
            sy +=yskip;
        }  /* for y */
    }  /* for count */

    draw_fix_char(buffer.data,0,0);
    //SDL_UnlockSurface(buffer);
    if (conf.do_message) {
        text_message(buffer.viewport.x,buffer.viewport.h+buffer.viewport.y-13,conf.message);
        conf.do_message--;
    }
    if (show_fps)
        text_message(buffer.viewport.x,buffer.viewport.y,fps_str);

    screen_update();
}

void draw_screen_scanline(int start_line, int end_line,int refresh)
{
    int sx =0,sy =0,my =0,zx = 1, zy = 1;
    int offs,count,y;
    int tileno,tileatr;
    int tctl1,tctl2,tctl3;
    unsigned char *vidram=memory.video;
    rect clear_rect;
    int yy;
    int otile,tile,yoffs;
    int zoom_line;
    int invert;
    Uint8 *zoomy_rom;

#ifdef DEBUG_VIDEO
  
    clear_rect.x = buffer.viewport.x;
    clear_rect.w = buffer.viewport.w;
    clear_rect.y = buffer.viewport.y;
    clear_rect.h = buffer.viewport.w;
    fill_rect(&buffer,NULL,current_pc_pal[4095]);
    fill_rect(&buffer,&clear_rect,0xFF00);
#else
    if (start_line>255) start_line=255;  
    if (end_line>255) end_line=255;  

    clear_rect.x = buffer.viewport.x;
	clear_rect.w = buffer.viewport.w;
    clear_rect.y = start_line;
    clear_rect.h = end_line-start_line+1;

    fill_rect(&buffer,&clear_rect,current_pc_pal[4095]);
#endif
    /* Draw sprites */
    for (count=0;count<0x300;count+=2) {


        tctl3 = READ_WORD( &vidram[0x10000 + count] );
        tctl1 = READ_WORD( &vidram[0x10400 + count] );
        tctl2 = READ_WORD( &vidram[0x10800 + count] );

	/* If this bit is set this new column is placed next to last one */
        if (tctl1 & 0x40) {
            sx += zx+1;            /* new x */
      

	    /* Get new zoom for this column */
            zx = (tctl3 >> 8) & 0x0f;
        } else {	/* nope it is a new block */
	    /* Sprite scaling */
            zx = (tctl3 >> 8) & 0x0f;    /* zomm x */
            zy = tctl3 & 0xff;           /* zoom y */

            sx = (tctl2 >> 7);           /* x pos 0 - 512  */
  

	    /* Number of tiles in this strip */
            my = tctl1 & 0x3f;

            sy=512-(tctl1 >> 7);  /* y pos 512 - 0 */
      
            if (my > 0x20) my=0x20;
        }
    
       
	/* No point doing anything if tile strip is 0 */
        if (my==0) continue;
        if (sx>=496) { /* after 496, we consider sx negative */
	    //printf("SX=%d\n",sx);
	    sx-=512;
	    //continue;
	}

	if (sx>320) {
	    continue;
	    //sx-=16;
	    //printf("SX=%d\n",sx);
	}

	if (sx<-16) continue;
	//sx&=0x1ff;

	/* Process x zoom */
        if(zx!=16) {
            dda_x_skip=ddaxskip[zx];
        } else zx=16;
    

    
        offs = count<<6;
        zoomy_rom = memory.ng_lo + (zy << 8);
   
#ifdef DEBUG_VIDEO
        yskip=16;
        tsy=sy;

        for (y=0; y < my ;y++) {
            tileno  = READ_WORD(&vidram[offs]);
            offs+=2;
            tileatr = READ_WORD(&vidram[offs]);
            offs+=2;

            if (memory.nb_of_tiles>0x10000 && tileatr&0x10) tileno+=0x10000;
            if (memory.nb_of_tiles>0x20000 && tileatr&0x20) tileno+=0x20000;
            if (memory.nb_of_tiles>0x40000 && tileatr&0x40) tileno+=0x40000;

	    /* animation automatique */
            if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
            else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);
      
            if(zy!=255)
            {
                yskip=0;
                dda_y_skip[0]=0;
                for(i=0;i<16;i++)
                {
                    dda_y_skip[i+1]=0;
                    dday-=zy+1;
                    if(dday<=0)
                    {
                        dday+=256;
                        yskip++;
                        dda_y_skip[yskip]++;
                    }
                    else dda_y_skip[yskip]++;
                }
            }
      

            if (tsy>262) tsy-=512;
            if (sx >= 0 && sx+15 < 544 && tsy>=0 && tsy+15 <544) {
		    /*
                if (memory.pen_usage[tileno]==TILE_UNCONVERTED)
                    convert_tile(tileno);
		    */
	
                draw_tile(tileno,sx+visible_area.x,tsy,zx+1,yskip,tileatr>>8,
                          tileatr & 0x01,tileatr & 0x02,
                          (unsigned char*)buffer.data);
      
            }
            tsy +=yskip;
        }  /* for y */
            
#else
	otile=-1;
        for (yy=start_line;yy<=end_line;yy++) {
            y=yy-sy; /* y: 0 -> my*16 */
      
            if (y<0) y+=512;

            if (y>=(my<<4)) continue;

            invert=0;

            zoom_line = y & 0xff;

            if (y & 0x100) {
                zoom_line ^= 0xff; /* zoom_line = 255 - zoom_line */
                invert = 1;
            }
	    
	    if (my == 0x20)	/* fix for joyjoy, trally... */
	    {
		if (zy)
		{
		    zoom_line %= (zy<<1);
		    if (zoom_line >= zy)
		    {
			zoom_line = (zy<<1)-1 - zoom_line;
			invert ^= 1;
		    }
		}
	    }

            yoffs = zoomy_rom[zoom_line] & 0x0f;
            tile = zoomy_rom[zoom_line] >> 4;

            if (invert) {
                tile ^= 0x1f; // tile=31 - tile;
                yoffs ^= 0x0f; // yoffs= 15 - yoffs;
            }

		tileno  = READ_WORD(&vidram[offs+(tile<<2)]);
		tileatr = READ_WORD(&vidram[offs+(tile<<2)+2]);

		if (memory.nb_of_tiles>0x10000 && tileatr&0x10) tileno+=0x10000;
		if (memory.nb_of_tiles>0x20000 && tileatr&0x20) tileno+=0x20000;
		if (memory.nb_of_tiles>0x40000 && tileatr&0x40) tileno+=0x40000;

		/* animation automatique */
		if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
		else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);
		if (tileatr & 0x02) yoffs ^= 0x0f;	/* flip y */

		//if (memory.pen_usage[tileno]==TILE_UNCONVERTED) convert_tile(tileno);


/*
	    if (memory.pen_usage[tileno]==TILE_INVISIBLE)
		continue;
*/

	    switch (memory.pen_usage[tileno]) {
		    case TILE_NORMAL:
			draw_scanline_tile(tileno,yoffs,sx+16  ,yy,zx,tileatr>>8,
					   tileatr & 0x01,(unsigned char*)buffer.data);
			break;
		    case TILE_TRANSPARENT50:
			draw_scanline_tile_50(tileno,yoffs,sx+16  ,yy,zx,tileatr>>8,
	                               tileatr & 0x01,(unsigned char*)buffer.data);
			break;
		    case TILE_TRANSPARENT25:
			draw_scanline_tile_25(tileno,yoffs,sx+16  ,yy,zx,tileatr>>8,
	                               tileatr & 0x01,(unsigned char*)buffer.data);
			break;
	    }
	  
            
        }
#endif /* DEBUG_VIDEO */
    }  /* for count */

/*
#ifndef DEBUG_VIDEO
    draw_fix_char(buffer.data,start_line,end_line);
#endif
*/
    if (refresh) {
#ifndef DEBUG_VIDEO
	draw_fix_char(buffer.data,0,0);
#endif
 
        if (conf.do_message) {
            text_message(buffer.viewport.x,buffer.viewport.h+buffer.viewport.y-13,conf.message);
            conf.do_message--;
        }
        if (show_fps)
            text_message(buffer.viewport.x,buffer.viewport.y,fps_str);
#ifdef DEBUG_VIDEO
        SDL_BlitSurface(buffer,NULL,screen,NULL);
        SDL_UpdateRect(screen,0,0,0,0);
#else
        screen_update();
#endif
    }
}

void init_video(void) {
	memset (&buffer, 0, sizeof (buffer));
	buffer.width = 352;
	buffer.height = 256; /* should be 256 i think :/ */
	buffer.depth = 16;
	buffer.granularity = 2;
	buffer.pitch = (buffer.width * buffer.granularity);
	buffer.data = calloc (buffer.height * buffer.width, buffer.granularity);
	buffer.viewport.changed = 1;
	//memset(buffer.data, 0, buffer.height * buffer.pitch);
    if (CF_BOOL(cf_get_item_by_name("screen320"))) {
		buffer.viewport.w = 320;
		buffer.viewport.h = 224;
		buffer.viewport.x = 16;
		buffer.viewport.y = 16;
    } else {
		buffer.viewport.w = 304;
		buffer.viewport.h = 224;
		buffer.viewport.x = 24;
		buffer.viewport.y = 16;
    }
	buffer.remap = 1;
	fix_value_init();
}
