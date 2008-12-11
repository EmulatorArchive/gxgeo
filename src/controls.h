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

#ifndef __CONTROLS_H__
#define __CONTROLS_H__ 1
#include <wiiuse/wpad.h>
#include "types.h"
#include "emu.h"

enum {
  WIIMOTE = 0,
  NUNCHUK,
  CLASSIC,
  GCPAD,
  CONTROL_MAX
};

enum {
    BUTTON_A = 0,
    BUTTON_B,
    BUTTON_C,
    BUTTON_D,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_START,
    BUTTON_COIN,
    BUTTON_MAX
};

enum {
    WI1         = 0x00000001,
    WI2         = 0x00000002,
    WIA         = 0x00000004,
    WIB         = 0x00000008,
    WIUP        = 0x00000010,
    WIDOWN      = 0x00000020,
    WILEFT      = 0x00000040,
    WIRIGHT     = 0x00000080,
    WIMINUS     = 0x00000100,
    WIPLUS      = 0x00000200,
    WIHOME      = 0x00000400
};

enum {
    NUSUP       = 0x00000800,
    NUSDOWN     = 0x00001000,
    NUSLEFT     = 0x00002000,
    NUSRIGHT    = 0x00004000,
    NUC         = 0x00008000,
    NUZ         = 0x00010000,
    NUEUP       = 0x00000810, /* NUNCHUK EITHER DPAD UP OR STICK UP */
    NUEDOWN     = 0x00001020, 
    NUELEFT     = 0x00002040,
    NUERIGHT    = 0x00004080
};

enum {
    CLA         = 0x00000001,
    CLB         = 0x00000002,
    CLX         = 0x00000004,
    CLY         = 0x00000008,
    CLMINUS     = 0x00000010,
    CLPLUS      = 0x00000020,
    CLHOME      = 0x00000040,
    CLDUP       = 0x00000080,
    CLDDOWN     = 0x00000100,
    CLDLEFT     = 0x00000200,
    CLDRIGHT    = 0x00000400,
    CLLUP       = 0x00000800,
    CLLDOWN     = 0x00001000,
    CLLLEFT     = 0x00002000,
    CLLRIGHT    = 0x00004000,
    CLRUP       = 0x00008000,
    CLRDOWN     = 0x00010000,
    CLRLEFT     = 0x00020000,
    CLRRIGHT    = 0x00040000,
    CLLTRIG     = 0x00080000,
    CLRTRIG     = 0x00100000,
    CLLSHOULDER = 0x00200000,
    CLRSHOULDER = 0x00400000,
    CLEUP       = 0x00008880,  /* CLASSIC CONTROLLER EITHER DPAD UP OR LEFT/RIGHT STICK UP */
    CLEDOWN     = 0x00011100,
    CLELEFT     = 0x00022200,
    CLERIGHT    = 0x00044400,
};

enum {
    GCA         = 0x00000001,
    GCB         = 0x00000002,
    GCX         = 0x00000004,
    GCY         = 0x00000008,
    GCSTART     = 0x00000010,
    GCDUP       = 0x00000020,
    GCDDOWN     = 0x00000040,
    GCDLEFT     = 0x00000080,
    GCDRIGHT    = 0x00000100,
    GCSUP       = 0x00000200,
    GCSDOWN     = 0x00000400,
    GCSLEFT     = 0x00000800,
    GCSRIGHT    = 0x00001000,
    GCCUP       = 0x00002000,
    GCCDOWN     = 0x00004000,
    GCCLEFT     = 0x00008000,
    GCCRIGHT    = 0x00010000,
    GCLTRIG     = 0x00020000,
    GCRTRIG     = 0x00040000,
    GCZ         = 0x00080000,
    GCEUP       = 0x00002220, /* GAME CUBE CONTROLLER EITHER DPAD UP OR LEFT STICK UP */
    GCEDOWN     = 0x00004440,
    GCELEFT     = 0x00008880,
    GCERIGHT    = 0x00011100,
};

Uint8 joy_button[2][BUTTON_MAX];
int *joy_wiimote;
int *joy_nunchuk;
int *joy_classic;
int *joy_gcpad;


int read_input(Uint8 joy[2][BUTTON_MAX], int held);
#endif
