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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "emu.h"
#include "memory.h"
#include "frame_skip.h"
#include "pd4990a/pd4990a.h"
#include "messages.h"
#include "debug.h"
#include "sound.h"
#include "controls.h"
#include "timer.h"
#include "streams.h"
#include "ym2610/2610intf.h"
#include "sound.h"
#include "screen.h"
#include "neocrypt.h"
#include "conf.h"
#include "driver.h"
#include <wiiuse/wpad.h>

int frame;
int nb_interlace = 256;
int current_line;
int arcade;

extern int irq2enable, irq2start, irq2repeat, irq2control, irq2taken;
extern int lastirq2line;
extern int irq2repeat_limit;
extern Uint32 irq2pos_value;
extern Uint8 *joy_touse[4];


void setup_misc_patch(char *name)
{
    sram_protection_hack = -1;
    if (!strcmp(name,"fatfury3") ||
  !strcmp(name,"samsho3") ||
  !strcmp(name,"samsho3a") ||
  !strcmp(name,"samsho4") ||
  !strcmp(name,"aof3") ||
  !strcmp(name,"rbff1") ||
  !strcmp(name,"rbffspec") ||
  !strcmp(name,"kof95") ||
  !strcmp(name,"kof96") ||
  !strcmp(name,"kof96h") ||
  !strcmp(name,"kof97") ||
  !strcmp(name,"kof97a") ||
  !strcmp(name,"kof97pls") ||
  !strcmp(name,"kof98") ||
  !strcmp(name,"kof98k") ||
  !strcmp(name,"kof98n") ||
  !strcmp(name,"kof99") ||
  !strcmp(name,"kof99a") ||
  !strcmp(name,"kof99e") ||
  !strcmp(name,"kof99n") ||
  !strcmp(name,"kof99p") ||
  !strcmp(name,"kof2000") ||
  !strcmp(name,"kof2000n") ||
  !strcmp(name,"kizuna") ||
  !strcmp(name,"lastblad") ||
  !strcmp(name,"lastblda") ||
  !strcmp(name,"lastbld2") ||
  !strcmp(name,"rbff2") ||
  !strcmp(name,"rbff2a") ||
  !strcmp(name,"mslug2") ||
  !strcmp(name,"mslug3") ||
  !strcmp(name,"garou") ||
  !strcmp(name,"garouo") ||
  !strcmp(name,"garoup"))
      sram_protection_hack = 0x100;



    if (!strcmp(name, "pulstar"))
    sram_protection_hack = 0x35a;


    if (!strcmp(name, "ssideki")) {
    WRITE_WORD_ROM(&memory.cpu[0x2240], 0x4e71);
    }


    if (!strcmp(name, "fatfury3")) {
    WRITE_WORD_ROM(memory.cpu, 0x0010);
    }

    
    /* Many mgd2 dump have a strange initial PC, so as some MVS */
    if ((!strcmp(name, "aodk")) ||
  (!strcmp(name, "bjourney")) ||
  (!strcmp(name, "maglord")) ||
  (!strcmp(name, "mosyougi")) ||
  (!strcmp(name, "twinspri")) ||
  (!strcmp(name, "whp")) || 
  (conf.rom_type == MGD2) ||
  (CF_BOOL(cf_get_item_by_name("forcepc"))) ) {
  Uint8 *RAM = memory.cpu;
  WRITE_WORD_ROM(&RAM[4], 0x00c0);
  WRITE_WORD_ROM(&RAM[6], 0x0402);
    }

    if (!strcmp(name, "mslugx")) {
  /* patch out protection checks */
  int i;
  Uint8 *RAM = memory.cpu;
  for (i = 0; i < memory.cpu_size; i += 2) {
      if ((READ_WORD_ROM(&RAM[i + 0]) == 0x0243) && 
    (READ_WORD_ROM(&RAM[i + 2]) == 0x0001) &&  /* andi.w  #$1, D3 */
    (READ_WORD_ROM(&RAM[i + 4]) == 0x6600)) {  /* bne xxxx */

    WRITE_WORD_ROM(&RAM[i + 4], 0x4e71);
    WRITE_WORD_ROM(&RAM[i + 6], 0x4e71);
      }
  }

  WRITE_WORD_ROM(&RAM[0x3bdc], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3bde], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3be0], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3c0c], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3c0e], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3c10], 0x4e71);

  WRITE_WORD_ROM(&RAM[0x3c36], 0x4e71);
  WRITE_WORD_ROM(&RAM[0x3c38], 0x4e71);
    }


}

void neogeo_init(void)
{
    modulo = 1;
    sram_lock=0;
    sound_code=0;
    pending_command=0;
    result_code=0;
    if (memory.cpu_size>0x100000)
    cpu_68k_bankswitch(0x100000);
    else
    cpu_68k_bankswitch(0);
#if 0 /* NOSTATE */
    neogeo_init_save_state();
#endif
}

/* called after the rom is opened */
void init_neo(char *rom_name)
{
    cpu_68k_init();
    neogeo_init();
    pd4990a_init();
    setup_misc_patch(rom_name);
    if (conf.sound) {
      cpu_z80_init();
      ogc_init_audio();
      streams_sh_start();
      YM2610_sh_start();
      conf.snd_st_reg_create=1;
    }
    cpu_68k_reset();
    arcade = (conf.system != SYS_HOME);
   
}
#include "bitmap.h"
#include "video.h"
#include <stdio.h>
extern t_bitmap buffer;
static void take_screenshot(void) {
  /* Generate a BMP of the current buffer */
  char sbuffer[4096];
  sprintf(sbuffer, "fat:/screenie_%i.bmp",(int)time(NULL));
  SaveBuffer(sbuffer, &buffer);
}

static int fc;
static int last_line;
static int skip_this_frame = 0;

static inline int neo_interrupt(void)
{
/*
    static int fc;
    int skip_this_frame;
*/

    pd4990a_addretrace();
    // printf("neogeo_frame_counter_speed %d\n",neogeo_frame_counter_speed);
    if (!(irq2control & 0x8)) {
    if (fc >= neogeo_frame_counter_speed) {
        fc = 0;
        neogeo_frame_counter++;
    }
    fc++;
    }

    skip_this_frame = skip_next_frame;
    skip_next_frame = frame_skip(0);

    if (!skip_this_frame) {
    draw_screen();
    }
    return 1;
}



static inline void update_screen(void) {
    
    if (irq2control & 0x40)
    irq2start = (irq2pos_value + 3) / 0x180;  /* ridhero gives 0x17d */
    else
    irq2start = 1000;
  current_line = 0;

  if (!skip_this_frame) {
    if (last_line < 21) 
    { /* there was no IRQ2 while the beam was in the visible area 
        -> no need for scanline rendering */
      draw_screen();
      //draw_screen_scanline(last_line-21, 262, 1);
    } else {
      draw_screen_scanline(last_line-21, 262, 1);
    }
  }

  last_line=0;

  pd4990a_addretrace();
  if (fc >= neogeo_frame_counter_speed) {
    fc = 0;
    neogeo_frame_counter++;
  }
  fc++;
    
    skip_this_frame = skip_next_frame;
    skip_next_frame = frame_skip(0);
}

static inline int update_scanline(void) {
    /* int i; */
    irq2taken = 0;

    if (irq2control & 0x10) {  
    if (current_line  == irq2start ) {
        if (irq2control & 0x80)
        irq2start += (irq2pos_value + 3) / 0x180;
        irq2taken = 1;
    }
    }


    if (irq2taken) {
    if (!skip_this_frame) {
        draw_screen_scanline(last_line-21, current_line-20, 0);
    }
    last_line = current_line;
    }
    current_line++;
    return irq2taken;
}

void update_p1_key(void) {
  memory.intern_p1 = 0xFF;
  if (joy_button[0][BUTTON_UP])       memory.intern_p1 &= 0xFE;  // UP
  if (joy_button[0][BUTTON_DOWN])     memory.intern_p1 &= 0xFD;  // DOWN
  if (joy_button[0][BUTTON_LEFT])     memory.intern_p1 &= 0xFB;  // LEFT
  if (joy_button[0][BUTTON_RIGHT])    memory.intern_p1 &= 0xF7;  // RIGHT
  
  if (joy_button[0][BUTTON_A])        memory.intern_p1 &= 0xEF;  // A
  if (joy_button[0][BUTTON_B])        memory.intern_p1 &= 0xDF;  // B
  if (joy_button[0][BUTTON_C])        memory.intern_p1 &= 0xBF;  // C
  if (joy_button[0][BUTTON_D])        memory.intern_p1 &= 0x7F;  // D
  
}

void update_p2_key(void) {
  memory.intern_p2=0xFF;  
  if (joy_button[1][BUTTON_UP])       memory.intern_p2 &= 0xFE;  // UP
  if (joy_button[1][BUTTON_DOWN])     memory.intern_p2 &= 0xFD;  // DOWN
  if (joy_button[1][BUTTON_LEFT])     memory.intern_p2 &= 0xFB;  // LEFT
  if (joy_button[1][BUTTON_RIGHT])    memory.intern_p2 &= 0xF7;  // RIGHT
  
  if (joy_button[1][BUTTON_A])        memory.intern_p2 &= 0xEF;  // A
  if (joy_button[1][BUTTON_B])        memory.intern_p2 &= 0xDF;  // B
  if (joy_button[1][BUTTON_C])        memory.intern_p2 &= 0xBF;  // C
  if (joy_button[1][BUTTON_D])        memory.intern_p2 &= 0x7F;  // D
}

void update_start(void) {
  memory.intern_start = 0x8F;
  if (joy_button[0][BUTTON_START]) memory.intern_start &= 0xFE;
  if (joy_button[1][BUTTON_START]) memory.intern_start &= 0xFB;
  if (!arcade) { /* Select */
    if (joy_button[0][BUTTON_COIN]) memory.intern_start &= 0xFD;
    if (joy_button[1][BUTTON_COIN]) memory.intern_start &= 0xF7;
  }
}
void update_coin(void) {
  memory.intern_coin = 0x7;
  if (joy_button[0][BUTTON_COIN]) memory.intern_coin &= 0x6;
  if (joy_button[1][BUTTON_COIN]) memory.intern_coin &= 0x5;

}

static Uint16 pending_save_state=0,pending_load_state=0;
static int slow_motion = 0;

static inline void state_handling(save,load) {
#if 0 /* NOSTATE */
    if (save) {
    if (conf.sound) SDL_LockAudio();
    save_state(conf.game,save-1);
    if (conf.sound) SDL_UnlockAudio();
    reset_frame_skip();
    }
    if (load) {
    if (conf.sound) SDL_LockAudio();
    load_state(conf.game,load-1);
    if (conf.sound) SDL_UnlockAudio();
    reset_frame_skip();
    }
    pending_load_state=pending_save_state=0;
#endif
}

int evctr = 0;
 
void countevs(int chan, const WPADData *data) {
  evctr++;
}

void main_loop(void)
{
    int neo_emu_done = 0;
    int m68k_overclk=CF_VAL(cf_get_item_by_name("68kclock"));
    int z80_overclk=CF_VAL(cf_get_item_by_name("z80clock"));
    int nb_frames=0;

    Uint32 cpu_68k_timeslice = (m68k_overclk==0?200000:200000+(m68k_overclk*200000/100.0));
    Uint32 cpu_68k_timeslice_scanline = cpu_68k_timeslice/262.0;
    Uint32 cpu_z80_timeslice = (z80_overclk==0?73333:73333+(z80_overclk*73333/100.0));
    Uint32 tm_cycle=0;

    Uint32 cpu_z80_timeslice_interlace = cpu_z80_timeslice / (float) nb_interlace;
    char ksym_code[5];
    Uint16 scancode, i, a;
    char input_buf[20];
    Uint8 show_keysym=0;
    int invert_joy=CF_BOOL(cf_get_item_by_name("invertjoy"));
    int x, y;
    for (x = 0; x < 2; x++) {
      for (y = 0; y < BUTTON_MAX; y++) {
        joy_button[x][y] = 0;
      }
    }
  
    reset_frame_skip();
    my_timer();
  /*
    printf("\tCpuspeed: %d\n",cpu_68k_timeslice);
    printf("\t%s\n",&memory.cpu[0x100]);
    printf("\tNGH = %04x\n",READ_WORD(&memory.cpu[0x108]));
    printf("\tSSN = %04x\n",READ_WORD(&memory.cpu[0x114]));
  */
    int loop_count = 0; 
    while (!neo_emu_done) {    
      if (conf.test_switch == 1) conf.test_switch = 0;
        
      neo_emu_done = read_input(joy_button, 0);
      
      // update the internal representation of keyslot 
      update_p1_key();
      update_p2_key();
      update_start();
      update_coin();

      if (slow_motion) usleep(100);
      if (conf.sound) {
        /* run z80 */
        for (i = 0; i < nb_interlace; i++) {
          cpu_z80_run(cpu_z80_timeslice_interlace);
          my_timer();
        }
      }
      if (!conf.debug) {
        /* run m68k */
        if (conf.raster) {
          for (i = 0; i < 261; i++) {
            tm_cycle=cpu_68k_run(cpu_68k_timeslice_scanline-tm_cycle);
            if (update_scanline()) { cpu_68k_interrupt(2); }
          }
          tm_cycle=cpu_68k_run(cpu_68k_timeslice_scanline-tm_cycle);
          state_handling(pending_save_state,pending_load_state);
          
          update_screen();
          cpu_68k_interrupt(1);
        } else {
          tm_cycle=cpu_68k_run(cpu_68k_timeslice-tm_cycle);
          a = neo_interrupt();
          
          /* state handling (we save/load before interrupt) */
          state_handling(pending_save_state,pending_load_state);
          if (a) cpu_68k_interrupt(a);
        }
        ogc_update_stream();
      } else {
        /* we are in debug mode -> we are just here for event handling */
        neo_emu_done=1;
      }
    }
  ogc_close_audio();
}

void cpu_68k_dpg_step(void) {
    static Uint32 nb_cycle;
    static Uint32 line_cycle;
    Uint32 cpu_68k_timeslice = 200000;
    Uint32 cpu_68k_timeslice_scanline = 200000/(float)262;
    Uint32 cycle;
    if (nb_cycle==0) {
    main_loop(); /* update event etc. */
    }
    cycle=cpu_68k_run_step();
    add_bt(cpu_68k_getpc());
    line_cycle+=cycle;
    nb_cycle+=cycle;
    if (nb_cycle>=cpu_68k_timeslice) {
    nb_cycle=line_cycle=0;
    if (conf.raster) {
        update_screen();
    } else {
        neo_interrupt();
    }
    state_handling(pending_save_state,pending_load_state);
    cpu_68k_interrupt(1);
    } else {
    if (line_cycle>=cpu_68k_timeslice_scanline) {
        line_cycle=0;
        if (conf.raster) {
        if (update_scanline())
            cpu_68k_interrupt(2);
        }
    }
    }
}

void debug_loop(void) {
    int a;
    do {
    a = cpu_68k_debuger(cpu_68k_dpg_step,dump_hardware_reg);
    } while(a!=-1);
}
