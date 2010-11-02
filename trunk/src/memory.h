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

#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "video.h"
#include "ym2610/2610intf.h"
#include "state.h"
#include "driver.h"

#define READ_WORD(a)          (*(Uint16 *)(a))
#define WRITE_WORD(a,d)       (*(Uint16 *)(a) = (d))
#define READ_BYTE(a)          (*(Uint8 *)(a))
#define WRITE_BYTE(a,d)       (*(Uint8 *)(a) = (d))
#define SWAP_BYTE_ADDRESS(a)  ((Uintptr)(a)^1)
#define SWAP16(y) SDL_Swap16(y)
#define SWAP32(y) SDL_Swap32(y)
		    
/* Programs are stored as BIGENDIAN */
#  ifdef WORDS_BIGENDIAN
#    define WRITE_WORD_ROM WRITE_WORD
#    define READ_WORD_ROM READ_WORD
#    define WRITE_BYTE_ROM WRITE_BYTE
#    define READ_BYTE_ROM READ_BYTE
#  else /* WORDS_BIGENDIAN */
#    define WRITE_WORD_ROM(a,d) (WRITE_WORD(a,SWAP16(d)))
#    define READ_WORD_ROM(a) (SWAP16(READ_WORD(a)))
#    define WRITE_BYTE_ROM WRITE_BYTE
#    define READ_BYTE_ROM READ_BYTE
#  endif


#define CHECK_ALLOC(a) {if (!a) {printf("\t\tOut of Memory (%s:%i)\n",__FILE__,__LINE__);sleep(10);exit(1);}}


typedef struct neo_mem {
    Uint8 *cpu;
    Uint32 cpu_size;
    Uint8 *ram;
    Uint8 *bios;
    Uint32 bios_size;
    Uint8 *ng_lo;
    Uint8 *sm1;
    Uint32 sm1_size;
    Uint8 *sfix_board;
    Uint8 *sfix_game;
    Uint32 sfix_size;
    Uint8 *sound1;
    Uint32 sound1_size;
    Uint8 *sound2;
    Uint32 sound2_size;
    Uint8 *gfx;
    Uint32 gfx_size;
    Uint32 nb_of_tiles;
    //tile **tile_in_cache;
    Uint8 video[0x20000];
    Uint8 *pal1, *pal2;
    Uint8 *pal_pc1, *pal_pc2;
    Uint8 sram[0x10000];
    Uint32 *pen_usage;
    Uint8 fix_board_usage[4096];
    Uint8 *fix_game_usage;
    Uint8 z80_ram[0x800];
    Uint8 game_vector[0x80];
    /* internal representation of key */
    Uint8 intern_p1, intern_p2, intern_coin, intern_start;
    /* crypted rom bankswitch system */
    Uint32 bksw_handler;
    Uint8 bksw_unscramble[6];
    Uint8 bksw_offset[64];
    Uint8 kof2003_bksw[0x2000];
	Uint8 memcard[0x800];
	Uint8 mapped;
} neo_mem;

neo_mem memory;

/* video related */
extern int irq2enable, irq2start, irq2repeat, irq2control;
extern int lastirq2line;
extern int irq2repeat_limit;
int palno, vptr, high_tile, vhigh_tile, vvhigh_tile;
Sint16 modulo;
Uint8 *current_pal;
Uint8 *current_pal_dirty;
Uint32 *current_pc_pal;
Uint8 *current_fix;
Uint8 *fix_usage;

/* memory card */
extern Uint8 neo_memcard[0x1000];;

/* sram */
Uint8 sram_lock;
//Uint32 sram_protection_hack;
int sram_protection_hack;

/* Sound control */
Uint8 sound_code;
Uint8 pending_command;
Uint8 result_code;


/* 68k cpu Banking control */
extern Uint32 bankaddress;		/* current bank */
Uint8 current_cpu_bank;
Uint16 z80_bank[4];

/* misc utility func */
void update_all_pal(void);
void dump_hardware_reg(void);

/* cpu 68k interface */
int cpu_68k_getcycle(void);
void cpu_68k_init(void);
void cpu_68k_reset(void);
int cpu_68k_run(Uint32 nb_cycle);
void cpu_68k_interrupt(int a);
void cpu_68k_bankswitch(Uint32 address);
void cpu_68k_disassemble(int pc, int nb_instr);
void cpu_68k_dumpreg(void);
int cpu_68k_run_step(void);
Uint32 cpu_68k_getpc(void);
void cpu_68k_fill_state(M68K_STATE *st);
void cpu_68k_set_state(M68K_STATE *st);
int cpu_68k_debuger(void (*execstep)(void),void (*dump)(void));


/* cpu z80 interface */
void cpu_z80_run(int nbcycle);
void cpu_z80_nmi(void);
void cpu_z80_raise_irq(int l);
void cpu_z80_lower_irq(void);
void cpu_z80_init(void);
void cpu_z80_switchbank(Uint8 bank, Uint16 PortNo);
Uint8 z80_port_read(Uint16 PortNo);
void z80_port_write(Uint16 PortNb, Uint8 Value);
void cpu_z80_set_state(Z80_STATE *st);
void cpu_z80_fill_state(Z80_STATE *st);

/* memory handler prototype */
void neogeo_sound_irq(int irq);

#define LONG_FETCH(fetchname) Uint32 fetchname ## _long(Uint32 addr) { \
      return (fetchname ## _word(addr) << 16) |	fetchname ## _word(addr+2); \
}

#define LONG_STORE(storename) void storename ## _long(Uint32 addr, Uint32 data) { \
      storename ## _word(addr,data>>16); \
      storename ## _word(addr+2,data & 0xffff); \
}

/* 68k fetching function */
Uint8 mem68k_fetch_ram_byte(Uint32 addr);
Uint16 mem68k_fetch_ram_word(Uint32 addr);
Uint32 mem68k_fetch_ram_long(Uint32 addr);
Uint8 mem68k_fetch_invalid_byte(Uint32 addr);
Uint16 mem68k_fetch_invalid_word(Uint32 addr);
Uint32 mem68k_fetch_invalid_long(Uint32 addr);
Uint8 mem68k_fetch_bk_normal_byte(Uint32 addr);
Uint16 mem68k_fetch_bk_normal_word(Uint32 addr);
Uint32 mem68k_fetch_bk_normal_long(Uint32 addr);
Uint8 mem68k_fetch_cpu_byte(Uint32 addr);
Uint16 mem68k_fetch_cpu_word(Uint32 addr);
Uint32 mem68k_fetch_cpu_long(Uint32 addr);
Uint8 mem68k_fetch_bios_byte(Uint32 addr);
Uint16 mem68k_fetch_bios_word(Uint32 addr);
Uint32 mem68k_fetch_bios_long(Uint32 addr);
Uint8 mem68k_fetch_sram_byte(Uint32 addr);
Uint16 mem68k_fetch_sram_word(Uint32 addr);
Uint32 mem68k_fetch_sram_long(Uint32 addr);
Uint8 mem68k_fetch_pal_byte(Uint32 addr);
Uint16 mem68k_fetch_pal_word(Uint32 addr);
Uint32 mem68k_fetch_pal_long(Uint32 addr);
Uint8 mem68k_fetch_video_byte(Uint32 addr);
Uint16 mem68k_fetch_video_word(Uint32 addr);
Uint32 mem68k_fetch_video_long(Uint32 addr);
Uint8 mem68k_fetch_ctl1_byte(Uint32 addr);
Uint16 mem68k_fetch_ctl1_word(Uint32 addr);
Uint32 mem68k_fetch_ctl1_long(Uint32 addr);
Uint8 mem68k_fetch_ctl2_byte(Uint32 addr);
Uint16 mem68k_fetch_ctl2_word(Uint32 addr);
Uint32 mem68k_fetch_ctl2_long(Uint32 addr);
Uint8 mem68k_fetch_ctl3_byte(Uint32 addr);
Uint16 mem68k_fetch_ctl3_word(Uint32 addr);
Uint32 mem68k_fetch_ctl3_long(Uint32 addr);
Uint8 mem68k_fetch_coin_byte(Uint32 addr);
Uint16 mem68k_fetch_coin_word(Uint32 addr);
Uint32 mem68k_fetch_coin_long(Uint32 addr);
Uint8 mem68k_fetch_memcrd_byte(Uint32 addr);
Uint16 mem68k_fetch_memcrd_word(Uint32 addr);
Uint32 mem68k_fetch_memcrd_long(Uint32 addr);
Uint8 mem68k_fetch_bk_kof2003_byte(Uint32 addr);
Uint16 mem68k_fetch_bk_kof2003_word(Uint32 addr);
Uint32 mem68k_fetch_bk_kof2003_long(Uint32 addr);

/* 68k storring function */
void mem68k_store_invalid_byte(Uint32 addr, Uint8 data);
void mem68k_store_invalid_word(Uint32 addr, Uint16 data);
void mem68k_store_invalid_long(Uint32 addr, Uint32 data);
void mem68k_store_ram_byte(Uint32 addr, Uint8 data);
void mem68k_store_ram_word(Uint32 addr, Uint16 data);
void mem68k_store_ram_long(Uint32 addr, Uint32 data);
void mem68k_store_bk_normal_byte(Uint32 addr, Uint8 data);
void mem68k_store_bk_normal_word(Uint32 addr, Uint16 data);
void mem68k_store_bk_normal_long(Uint32 addr, Uint32 data);
void mem68k_store_sram_byte(Uint32 addr, Uint8 data);
void mem68k_store_sram_word(Uint32 addr, Uint16 data);
void mem68k_store_sram_long(Uint32 addr, Uint32 data);
void mem68k_store_pal_byte(Uint32 addr, Uint8 data);
void mem68k_store_pal_word(Uint32 addr, Uint16 data);
void mem68k_store_pal_long(Uint32 addr, Uint32 data);
void mem68k_store_video_byte(Uint32 addr, Uint8 data);
void mem68k_store_video_word(Uint32 addr, Uint16 data);
void mem68k_store_video_long(Uint32 addr, Uint32 data);
void mem68k_store_pd4990_byte(Uint32 addr, Uint8 data);
void mem68k_store_pd4990_word(Uint32 addr, Uint16 data);
void mem68k_store_pd4990_long(Uint32 addr, Uint32 data);
void mem68k_store_z80_byte(Uint32 addr, Uint8 data);
void mem68k_store_z80_word(Uint32 addr, Uint16 data);
void mem68k_store_z80_long(Uint32 addr, Uint32 data);
void mem68k_store_setting_byte(Uint32 addr, Uint8 data);
void mem68k_store_setting_word(Uint32 addr, Uint16 data);
void mem68k_store_setting_long(Uint32 addr, Uint32 data);
void mem68k_store_memcrd_byte(Uint32 addr, Uint8 data);
void mem68k_store_memcrd_word(Uint32 addr, Uint16 data);
void mem68k_store_memcrd_long(Uint32 addr, Uint32 data);
void mem68k_store_bk_kof2003_byte(Uint32 addr, Uint8 data);
void mem68k_store_bk_kof2003_word(Uint32 addr, Uint16 data);
void mem68k_store_bk_kof2003_long(Uint32 addr, Uint32 data);

Uint8 (*mem68k_fetch_bksw_byte)(Uint32);
Uint16 (*mem68k_fetch_bksw_word)(Uint32);
Uint32 (*mem68k_fetch_bksw_long)(Uint32);
void (*mem68k_store_bksw_byte)(Uint32,Uint8);
void (*mem68k_store_bksw_word)(Uint32,Uint16);
void (*mem68k_store_bksw_long)(Uint32,Uint32);
#endif
