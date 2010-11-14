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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/dir.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include <gccore.h>

#include "controls.h"
#include "streams.h"
#include "ym2610/2610intf.h"
#include "fileio.h"
#include "video.h"
#include "screen.h"
#include "emu.h"
#include "sound.h"
#include "messages.h"
#include "memory.h"
#include "debug.h"
#include "conf.h"
#include "transpack.h"
#include "driver.h"
#include "list.h"
#include "pbar.h"
#include "gx_supp.h"

extern GXRModeObj *vmode;
extern u32 *xfb[2];
static LIST *pdriver_list        = NULL;
static LIST *pdriver_path_list    = NULL;
extern Uint8 *joy_touse[4];

extern int player_channel[2];

void calculate_hotkey_bitmasks()
{
#if 0
    int *p;
    int i, j, mask;
    char *p1_key_list[] = { "p1hotkey0", "p1hotkey1", "p1hotkey2", "p1hotkey3" };
    char *p2_key_list[] = { "p2hotkey0", "p2hotkey1", "p2hotkey2", "p2hotkey3" };


    for ( i = 0; i < 4; i++ ) {
          p=CF_ARRAY(cf_get_item_by_name(p1_key_list[i]));
          for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
              conf.p1_hotkey[i] = mask;
      }

    for ( i = 0; i < 4; i++ ) {
          p=CF_ARRAY(cf_get_item_by_name(p2_key_list[i]));
          for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
              conf.p2_hotkey[i] = mask;
    }
#endif
}

void init_joystick(void) 
{

    /* Setup controls */
    joy_wiimote = CF_ARRAY(cf_get_item_by_name("wiimote_key"));
    joy_nunchuk = CF_ARRAY(cf_get_item_by_name("nunchuk_key"));
    joy_classic = CF_ARRAY(cf_get_item_by_name("classic_key"));
    joy_gcpad = CF_ARRAY(cf_get_item_by_name("gcpad_key"));
    joy_touse[0] = joy_wiimote;
    joy_touse[1] = joy_nunchuk;
    joy_touse[2] = joy_classic;
    joy_touse[3] = joy_gcpad;
    /* conf.nb_joy = 1; count wii motes */
    /*
    conf.p2_joy = CF_ARRAY(cf_get_item_by_name("p2joy"));
    conf.p1_joy = CF_ARRAY(cf_get_item_by_name("p1joy"));
    */
}

void init_rest(void)
{
    /* calculate_hotkey_bitmasks();    
    init_joystick(); */
    /* init key mapping */
    /*
    conf.p1_key=CF_ARRAY(cf_get_item_by_name("p1key"));
    conf.p2_key=CF_ARRAY(cf_get_item_by_name("p2key"));
    */
}

void load_driver_list(char *dirname)
{
    DIR_ITER *dir;
    struct stat st;
    char *filename = alloca(MAXPATHLEN+1);
    char *path = alloca(MAXPATHLEN+1);
    char *t;
    int dirc = 0,
    dirca = 0;
    DRIVER *dr;
    if (!(dir=diropen (dirname)))
    {
        printf("Couldn't open dir %s\n",dirname);
        return;
    }
    while (dirnext(dir, filename, &st) == 0)
    {
        if (!(st.st_mode & S_IFDIR))
        {
            dirc++;
        }
    }
    dirreset(dir);
    create_progress_bar("Load ROMSETs");
    while (dirnext(dir, filename, &st) == 0) {
        // Is regular file
        if (!(st.st_mode & S_IFDIR)) {
            t = strrchr(filename,'.');
            if (t && !strcasecmp(t,".zip")) {
                sprintf(path,"%s/%s",dirname,filename);
                //printf("Loading driver for: %s\n",path);
                dr = get_driver_for_zip(path);
                if (dr) { 
                    pdriver_list=list_append(pdriver_list,dr);
                    pdriver_path_list=list_append(pdriver_path_list,strdup(filename));                    
                }
                update_progress_bar(dirca++,dirc);
            }
        }
    }
    terminate_progress_bar();
    dirclose(dir);
}

DRIVER *list_rom_loop(char **romname)
{
    LIST* dr = NULL;
    LIST* drf = NULL;
    int row;
    int start = 0;
    unsigned int type;
    int i,j;
    while (1)
    {
        row = 4;
        printf("\x1b[2J");
        printf ("\x1b[%d;%dH", row, 2 );
        printf ("Press HOME to EXIT, 1 or 2 to load a ROMSET");
        row += 2;
        int count = 0;
        int first_row = row;
        if (start < 0) start = 0;
        //for(i=pdriver_list; i; i=i->next) {
        dr = pdriver_list;
        drf = pdriver_path_list;
        while( dr && drf ) {
            if (count++ >= start)
            {
                printf ("\x1b[%d;%dH", row, 2 );
                printf ("\t\t%s %s\n", (row==first_row?"+":" "), ((DRIVER*)dr->data)->longname);
                if (row++ > 20)
                {
                    break;
                }
            }
            dr = dr->next;
            drf = drf->next;
        }
        
        Uint8 jbuttons[2][BUTTON_MAX];
        /* Clear the jbuttons matrix */
        for (i=0; i<2; i++) for (j=0; j<BUTTON_MAX; j++) jbuttons[i][j] = 0;
        /* read the input keys */
        if (read_input(jbuttons, 1)) exit(0);

        int i;
        for (i = 0; i < BUTTON_MAX; i++)
        {
            jbuttons[0][i] |= jbuttons[1][i];
        }
        if (jbuttons[0][BUTTON_DOWN]) 
        {
            if (start++ >= count-1)
            {
                start = count-1;
            }
        }
        if (jbuttons[0][BUTTON_UP]) 
        {
            start--;
        }
        if (jbuttons[0][BUTTON_RIGHT])
        {
            start += 10;
            if (start >= count)
            {
                start = count-1;
            }
        }
        if (jbuttons[0][BUTTON_LEFT]) start -= 10;
        
        if ( (jbuttons[0][BUTTON_A] || jbuttons[0][BUTTON_B]) && count )
        { 
            if (start < 0) start = 0;
            printf("button a or button b: %d %d", count, start);
            sleep(5);
            printf("\x1b[2J");
            dr = pdriver_list;
            drf = pdriver_path_list;
            count = 0;
            while( dr->next && drf->next )
            {
                if (count == start)
                {
                    break;
                }
                dr = dr->next;
                drf = drf->next;
                count++;
            }
            char *tempstr = strdup((char*)drf->data);
            char *t = strrchr(tempstr,'.');
            *t = 0;
            *romname = tempstr;
            exit(0);
            return (DRIVER*)dr->data;
        }
        VIDEO_WaitVSync();
        if (vmode->viTVMode & VI_NON_INTERLACE)
        {
            VIDEO_WaitVSync();
        }
    }
    return NULL;
}

void return_to_wii_menu(void)
{
    /* if *0x80001800 is 0 then restart*/
    if (!*((u32*)0x80001800)) SYS_ResetSystem(SYS_RETURNTOMENU,0,0);
}

int main(int argc, char **argv)
{
    char *rom_name = NULL;
    int nopt;
    char *drconf,*gpath;
    DRIVER *dr = NULL;
    Uint8 gui_res, gngeo_quit=0;
    char *country;
    char *system;
    
      
    /* must be the first thing to do */
    fatInitDefault();    

    screen_init(); /* Setup GX and enable the console. */
    
    init_input();
   
    cf_init();     
    cf_open_file(NULL);
    cf_init_cmd_line();
    
    atexit(return_to_wii_menu);
    
    init_joystick();
    
    player_channel[0]= CF_VAL(cf_get_item_by_name("player1"));
    player_channel[1]= CF_VAL(cf_get_item_by_name("player2"));

    printf("\x1b[%d;%dH", 4, 0);
  
    /* print vmode struct */
    /*
        printf("%s\t: 0x%08X\n", "viTVMode", vmode->viTVMode);
        printf("%s\t: %d\n", "fbWidth", vmode->fbWidth);
        printf("%s\t: %d\n", "efbHeight", vmode->efbHeight);
        printf("%s\t: %d\n", "xfbHeight ", vmode->xfbHeight);
        printf("%s\t: %d\n", "viXOrigin ", vmode->viXOrigin);
        printf("%s\t: %d\n", "viYOrigin", vmode->viYOrigin);
        printf("%s\t: %d\n", "viWidth", vmode->viWidth);
        printf("%s\t: %d\n", "viHeight", vmode->viHeight);
    */
    
    bool passed = false;
    
    /* load all the drivers */
    dr_load_driver_dir(DATA_DIRECTORY"/romrc");
    if (argc > 1)
    {
        rom_name = argv[1];
        
        /* load driver for rom */
        dr = dr_get_by_name(rom_name);
        gpath = DATA_DIRECTORY"/conf/";
        drconf = alloca(strlen(gpath)+strlen(dr->name)+strlen(".cf")+1);
        sprintf(drconf, "%s%s.cf", gpath, dr->name);
        cf_open_file(drconf);
        passed = init_game(rom_name);
        if (!passed)
        {
            printf("\x1b[%d;%dH", 4, 0);            
            printf("Unable to load ROM, too large. Returning to ROM Load Menu...\n");
            sleep(2);
        }
    }
    
    /* if the driver did not load, display ROM list */
    bool rom_list_loaded = false;
    if (!passed)
    {
        load_driver_list(DATA_DIRECTORY"/roms");
        rom_list_loaded = true;
    }
    
    for (;;)
    {
        screen_restart();
        while(!passed)
        {
            if (!rom_list_loaded)
            {
                printf ("\x1b[%d;%dH", 4, 0);
                load_driver_list(DATA_DIRECTORY"/roms");
                rom_list_loaded = true;
            }
            /* draw the rom list */
            dr = list_rom_loop(&rom_name);
            /* clear screen */
            printf ("\x1b[%d;%dH", 4, 0);
            
            /* try and open a config file <romname>.cf in the conf folder */
            gpath = DATA_DIRECTORY"/conf/";
            drconf = alloca(strlen(gpath)+strlen(dr->name)+strlen(".cf")+1);
            sprintf(drconf, "%s%s.cf", gpath, dr->name);
            cf_open_file(drconf);
            passed = init_game(rom_name);
            printf("\x1b[2J");
            if (!passed)
            {
                printf("\x1b[%d;%dH", 4, 0);            
                printf("Unable to load ROM, too large. Returning to ROM Load Menu...\n");
                sleep(2);
            }
        }
        
        if (conf.debug)
        {
            conf.sound=0;
        }
        
        player_channel[0]= CF_VAL(cf_get_item_by_name("player1"));
        player_channel[1]= CF_VAL(cf_get_item_by_name("player2"));
    
    
        /* start emulation loop */
        if (conf.debug)
        {
            debug_loop();
        }
        else
        {
            main_loop();
        }
        save_nvram(conf.game);
        save_memcard(conf.game);
        passed = false;
    }

    return 0;
}
