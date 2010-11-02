/*  gxgeo a neogeo emulator
 *  Copyright (C) 2008 Matt Jeffery
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
 
#include <wiiuse/wpad.h>
#include "controls.h"
#include "types.h"

int player_channel[2];

static int joystick_direction[2][4];

unsigned int *joy_touse[4];
    
void init_input(void)
{
  /* Init Gamepads */
  PAD_Init();
  WPAD_Init();
  WPAD_SetDataFormat(0, WPAD_FMT_BTNS);
  joystick_direction[0][0] = 0;
  joystick_direction[0][1] = 0;
  joystick_direction[0][2] = 0;
  joystick_direction[0][3] = 0;
  joystick_direction[1][0] = 0;
  joystick_direction[1][1] = 0;
  joystick_direction[1][2] = 0;
  joystick_direction[1][3] = 0;
  player_channel[0] = 0;
  player_channel[1] = 0;
}

int read_mask_input_wiimote(unsigned int channel, int *data)
{
  *data = 0; 
  
  WPAD_ReadPending(channel, NULL);
  unsigned int        type;
  struct expansion_t  exp_data;
  int                 wiimote_connection_status = WPAD_Probe(channel, &type);
  
  exp_data.type = -1;
  
  if(wiimote_connection_status == WPAD_ERR_NONE) {
    WPADData *wd = WPAD_Data(channel);
    WPAD_Expansion(channel, &exp_data);       /* get the expansion info for the current wiimote */
    int pressed = wd->btns_h;

    if (exp_data.type == WPAD_EXP_CLASSIC) { /* Classic Controller */
      int pressedc = wd->exp.classic.btns;
      double lx_pos = 2*((double)(exp_data.classic.ljs.pos.x - exp_data.classic.ljs.min.x) / (double)(exp_data.classic.ljs.max.x - exp_data.classic.ljs.min.x)) - 1.0;
      double rx_pos = 2*((double)(exp_data.classic.rjs.pos.x - exp_data.classic.rjs.min.x) / (double)(exp_data.classic.rjs.max.x - exp_data.classic.rjs.min.x)) - 1.0;
      double ly_pos = 2*((double)(exp_data.classic.ljs.pos.y - exp_data.classic.ljs.min.y) / (double)(exp_data.classic.ljs.max.y - exp_data.classic.rjs.min.y)) - 1.0;
      double ry_pos = 2*((double)(exp_data.classic.rjs.pos.y - exp_data.classic.rjs.min.y) / (double)(exp_data.classic.rjs.max.y - exp_data.classic.rjs.min.y)) - 1.0;
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_A?CLA:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_B?CLB:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_X?CLX:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_Y?CLY:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_MINUS?CLMINUS:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_PLUS?CLPLUS:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_HOME?CLHOME:0 );
      *data       |= ( (ly_pos >=  0.5)?CLLUP:0 );
      *data       |= ( (ly_pos <= -0.5)?CLLDOWN:0 );
      *data       |= ( (lx_pos <= -0.5)?CLLLEFT:0 );
      *data       |= ( (lx_pos >=  0.5)?CLLRIGHT:0 );
      *data       |= ( (ry_pos >=  0.5)?CLRUP:0 );
      *data       |= ( (ry_pos <= -0.5)?CLRDOWN:0 );
      *data       |= ( (rx_pos <= -0.5)?CLRLEFT:0 );
      *data       |= ( (rx_pos >=  0.5)?CLRRIGHT:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_UP?CLDUP:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_DOWN?CLDDOWN:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_LEFT?CLDLEFT:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_RIGHT?CLDRIGHT:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_ZR?CLRSHOULDER:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_ZL?CLLSHOULDER:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_FULL_R?CLRTRIG:0 );
      *data       |= ( pressedc & CLASSIC_CTRL_BUTTON_FULL_L?CLLTRIG:0 );
      return CLASSIC;
    } else {
      *data       |= ( pressed & WPAD_BUTTON_1?WI1:0 );
      *data       |= ( pressed & WPAD_BUTTON_2?WI2:0 );
      *data       |= ( pressed & WPAD_BUTTON_A?WIA:0 );
      *data       |= ( pressed & WPAD_BUTTON_B?WIB:0 );
      *data       |= ( pressed & WPAD_BUTTON_MINUS?WIMINUS:0 );
      *data       |= ( pressed & WPAD_BUTTON_PLUS?WIPLUS:0 );
      *data       |= ( pressed & WPAD_BUTTON_HOME?WIHOME:0 );
      *data       |= ( pressed & WPAD_BUTTON_RIGHT?WIRIGHT:0 );
      *data       |= ( pressed & WPAD_BUTTON_LEFT?WILEFT:0 );
      *data       |= ( pressed & WPAD_BUTTON_UP?WIUP:0 );
      *data       |= ( pressed & WPAD_BUTTON_DOWN?WIDOWN:0 );
      if (exp_data.type == WPAD_EXP_NUNCHUK) {  /* If there is a nunchuck do this */
        int pressedn = wd->exp.nunchuk.btns_held;
        double x_pos = 2*((double)(exp_data.nunchuk.js.pos.x - exp_data.nunchuk.js.min.x) / (double)(exp_data.nunchuk.js.max.x - exp_data.nunchuk.js.min.x)) - 1.0;
        double y_pos = 2*((double)(exp_data.nunchuk.js.pos.y - exp_data.nunchuk.js.min.y) / (double)(exp_data.nunchuk.js.max.y - exp_data.nunchuk.js.min.y)) - 1.0;
        *data       |= ( pressedn & NUNCHUK_BUTTON_Z?NUZ:0 );
        *data       |= ( pressedn & NUNCHUK_BUTTON_C?NUC:0 );
        *data       |= ( (y_pos <= -0.5)?NUSDOWN:0 );
        *data       |= ( (y_pos >= 0.5)?NUSUP:0 );
        *data       |= ( (x_pos <= -0.5)?NUSLEFT:0 );
        *data       |= ( (x_pos >= 0.5)?NUSRIGHT:0 );
        return NUNCHUK;
      }
      return WIIMOTE;
    }
  }
  
  return -1;
}

int read_mask_input_gc(unsigned int gcpad, int *data)
{
  *data = 0;
  PAD_ScanPads();
  u16 pressedgc = PAD_ButtonsHeld(gcpad);

  double x_pos = 2*((double)(PAD_StickX(gcpad) + 0x80) / (double)(0xFF)) - 1.0;
  double y_pos = 2*((double)(PAD_StickY(gcpad) + 0x80) / (double)(0xFF)) - 1.0;
  double cx_pos = 2*((double)(PAD_SubStickX(gcpad) + 0x80) / (double)(0xFF)) - 1.0;
  double cy_pos = 2*((double)(PAD_SubStickY(gcpad) + 0x80) / (double)(0xFF)) - 1.0;

  *data       |= ( pressedgc & PAD_BUTTON_A?GCA:0 );
  *data       |= ( pressedgc & PAD_BUTTON_B?GCB:0 );
  *data       |= ( pressedgc & PAD_BUTTON_X?GCX:0 );
  *data       |= ( pressedgc & PAD_BUTTON_Y?GCY:0 );
  *data       |= ( pressedgc & PAD_TRIGGER_Z?GCZ:0 );
  *data       |= ( pressedgc & PAD_TRIGGER_R?GCRTRIG:0 );
  *data       |= ( pressedgc & PAD_TRIGGER_L?GCLTRIG:0 );
  *data       |= ( pressedgc & PAD_BUTTON_MENU?GCSTART:0 );
  *data       |= ( (y_pos >= 0.5)?GCSUP:0 );
  *data       |= ( (y_pos <= -0.5)?GCSDOWN:0 );
  *data       |= ( (x_pos <= -0.5)?GCSLEFT:0 );
  *data       |= ( (x_pos >= 0.5)?GCSRIGHT:0 );
  *data       |= ( (cy_pos >= 0.5)?GCCUP:0 );
  *data       |= ( (cy_pos <= -0.5)?GCCDOWN:0 );
  *data       |= ( (cx_pos <= -0.5)?GCCLEFT:0 );
  *data       |= ( (cx_pos >= 0.5)?GCCRIGHT:0 );
  *data       |= ( pressedgc & PAD_BUTTON_DOWN?GCDUP:0 );
  *data       |= ( pressedgc & PAD_BUTTON_UP?GCDDOWN:0 );
  *data       |= ( pressedgc & PAD_BUTTON_LEFT?GCDLEFT:0 );
  *data       |= ( pressedgc & PAD_BUTTON_RIGHT?GCDRIGHT:0 );
  return GCPAD;
}

unsigned int last_keys[] = { 0, 0 };
unsigned char home_key = 0;

char *GEO_BUTTONS[] = { "A", "B", "C", "D", "UP", "DOWN", "LEFT", "RIGHT", "START", "COIN" };
    
int read_input(unsigned char joy[2][BUTTON_MAX], int held)
{
  int keys = 0;
  int i;
  int wii_channel = 0;
  int player;
  for (player = 0; player < 2; player++)
  {
    int touse = 0;  
    
    /* Read current state of keys on channel */
    if (player_channel[player] > 4 || player_channel[player] < 0) player_channel[player] = 0;
    if (player_channel[player] == 0) touse = read_mask_input_wiimote(wii_channel++, &keys);
    else touse = read_mask_input_gc(player_channel[player]-1, &keys);
    
    /* check to see if keys have changed, if required */
    if (held) {
      int keys_x = keys & (~last_keys[player]);
      last_keys[player] = keys;
      /* gamecube controller hax0r  */
      keys_x |= (keys & (GCLTRIG | GCRTRIG));
      keys = keys_x;
    }
      
    /* fixed exit condition */
    int to_exit = 0;
    if (touse == NUNCHUK || touse == WIIMOTE) if (keys & WIHOME) to_exit = 1;
    if (touse == CLASSIC) if (keys & CLHOME) to_exit = 1;
    if (touse == GCPAD) if ((keys & GCLTRIG) && (keys & GCRTRIG)) to_exit = 1;    
    if (touse == -1) break;
    
    int home_key_x = to_exit & (~home_key);
    home_key = to_exit;
    to_exit = home_key_x;
    if (to_exit) return 1;
      
    /* touse is -1 when a channel is not connected, touse is the index of the array of controls */
    for (i = 0; i < BUTTON_MAX; i++) joy[player][i] = ((keys & joy_touse[touse][i]) != 0?1:0);
  }
  
  return 0;
}
