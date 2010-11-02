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

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <fat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include "fileio.h"
#include "unzip.h"
#include "driver.h"
#include "conf.h"
#include "pbar.h"
#include "memory.h"
#include "neocrypt.h"
#include "wiialloc.h"
#include "gfx_man.h"

extern int neogeo_fix_bank_type;

static LIST *driver_list=NULL;

static bool goto_next_driver(FILE * f)
{
    char buf[512];
    char game[12], type[10];
    long pos;

    while (!feof(f)) {
	pos=ftell(f);
	my_fgets(buf, 510, f);
	if (sscanf(buf, "game %12s %10s", game, type) >= 2) {
	    fseek(f,pos,SEEK_SET);
	    return true;
	}
    }
    return false;
}

static int get_sec_by_name(char *section) {
    static char *sec[SEC_MAX]={"CPU","SFIX","SM1","SOUND1","SOUND2","GFX"};
    int i;
    for(i=0;i<SEC_MAX;i++) {
	if (strcmp(sec[i],section)==0)
	    return i;
    }
    /*printf("Unknow section %s\n",section);*/
    return -1;
}

static int get_romtype_by_name(char *type) {
    static char *rt[ROMTYPE_MAX]={"MGD2","MVS","MVS_CMC42","MVS_CMC50"};
    int i;
    for(i=0;i<ROMTYPE_MAX;i++) {
		if (strcmp(rt[i],type)==0)
			return i;
    }
    printf("Unknown rom type %s\n",type);
    return -1;
}

static void add_driver_section(Uint32 s, SECTION *sec,FILE *f) {
    char buf[512], a[64], b[64];
    Uint32 size, offset;
    sec->size=s;
    sec->item=NULL;
    //  printf("SECTION size=%x\n",sec->size);
    while (1) {
        SECTION_ITEM *item;
        my_fgets(buf, 511, f);
        if (strcmp(buf, "END") == 0)
            break;
        sscanf(buf, "%s %x %x %s", a, &offset, &size, b);
        item=calloc(1, sizeof(SECTION_ITEM)); CHECK_ALLOC(item);

        item->filename=strdup(a);
        item->begin=offset;
        item->size=size;
        item->type=(strcmp(b,"ALTERNATE")==0?LD_ALTERNATE:LD_NORM);
        sec->item=list_append(sec->item,item);
	//    printf("%s %x %x %d\n",item->filename,item->begin,item->size,item->type);
    }
}

/* add driver define in f */
static void add_driver(FILE *f,char *fullname)
{
	DRIVER *dr;
    char buf[512], a[64], b[64];
    Uint32 s, size, offset;
    int sec;
    char game[12], type[10];
    char *t;

    dr = calloc(1, sizeof(DRIVER)); CHECK_ALLOC(dr);

    /* TODO: driver creation */
    my_fgets(buf, 510, f);
    sscanf(buf, "game %12s %10s", game, type);
    dr->name=strdup(game);
    dr->rom_type=get_romtype_by_name(type);
    dr->special_bios=0;
    dr->banksw_type=BANKSW_NORMAL;
    dr->sfix_bank=0;

#if 0
    t = strchr(buf, '"');
    if (t) {
	    char *e;
	    t+=1;
	    e=strchr(t, '"');
	    if (e) {
		    e[0]=0;
		    dr->longname=strdup(t);
	    } else
		    dr->longname=NULL;
    }
    else dr->longname=NULL;
#endif
    if (fullname)
	    dr->longname=strdup(fullname);
    else
	    dr->longname=NULL;

    //printf("Add %8s | %s \n",dr->name,dr->longname);

	while (1) {
		my_fgets(buf, 511, f);
		if (strcmp(buf, "END") == 0)
		    break;
		sscanf(buf, "%s %x", a, &s);
		sec=get_sec_by_name(a);
		if (sec==-1) {
		    int b=0;
		    if (strcmp(a,"BIOS")==0) { add_driver_section(s,&(dr->bios), f); dr->special_bios=1;}
		    else if (strcmp(a,"XOR")==0) { dr->xor=s; }
		    else if (strcmp(a,"SFIXBANK")==0) { dr->sfix_bank=s; }
		    else if (strcmp(a,"BANKSWITCH")==0) {
			dr->banksw_type=s;
			if(s==BANKSW_SCRAMBLE) {
			    /* not implemented yet */
			    my_fgets(buf,511,f);
			    sscanf(buf,"%x",&dr->banksw_addr);
			    my_fgets(buf, 511, f);
			    sscanf(buf, "%d %d %d %d %d %d",
				   (int *) &dr->banksw_unscramble[0],
				   (int *) &dr->banksw_unscramble[1],
				   (int *) &dr->banksw_unscramble[2],
				   (int *) &dr->banksw_unscramble[3],
				   (int *) &dr->banksw_unscramble[4],
				   (int *) &dr->banksw_unscramble[5]);
			    while (1) {
				my_fgets(buf, 511, f);
				if (strcmp(buf, "END") == 0)
				    break;
				sscanf(buf,"%x", &dr->banksw_off[b]);
				b++;
			    }
			}
		    } else {
			printf("Unknown section %s\n",a);
			return;
		    }
		} else add_driver_section(s,&(dr->section[sec]),f);
    }


    //driver_list=list_prepend(driver_list,dr);
    driver_list=list_append(driver_list,dr);
}

static void print_driver(void *data) {
    DRIVER *dr=data;
    int i;
    printf("\t\tgame %8s | %s \n",dr->name,dr->longname);
    for (i=0;i<SEC_MAX;i++) {
		LIST *l;
		printf("\t\tSection %d\n",i);
		
		for(l=dr->section[i].item;l;l=l->next) {
		    SECTION_ITEM *item=l->data;
		    printf("\t\t%8s %x %x\n",item->filename,item->begin,item->size);
		}
    }
}
/* Using sys/dir.h */
bool dr_load_driver_dir(char *dirname) {
	DIR_ITER *dir;
	struct stat st;
	int dirc = 0,
		dirca = 0;
	char *filename = alloca(MAXPATHLEN+1);
	char *path = alloca(MAXPATHLEN+1);
	if (!(dir=diropen (dirname))) {
		printf("Couldn't open dir %s\n",dirname);
		return false; 
	}
	while (dirnext(dir, filename, &st) == 0)
		if (!(st.st_mode & S_IFDIR))
			dirc++;
	dirreset(dir);
	create_progress_bar("Load Drivers");
	while (dirnext(dir, filename, &st) == 0) {
		// Is regular file
		if (!(st.st_mode & S_IFDIR)) {
			sprintf(path,"%s/%s",dirname,filename);
			dr_load_driver(path);
			update_progress_bar(dirca++,dirc);
		}
	}
	terminate_progress_bar();
	dirclose(dir);
	return true;
}


/* load the specified file, and create the driver struct */
bool dr_load_driver(char *filename) {
    FILE *f;
    LIST *i;
    char buf[512],*fullname;

    f=fopen(filename,"r");
    if (!f) {
		printf("Couldn't find %s\n",filename);
		return false;
    }
    /* fill game information */
    my_fgets(buf, 510, f);
    
    if (strncmp(buf,"longname ",9)==0) {
	    fullname=buf+9; /* hummm, caca... */
	    //chomp(fullname);
    } else {
	    fullname=NULL;
    }

    while(goto_next_driver(f)==true) {
		  add_driver(f,fullname);
    }

    //list_foreach(driver_list,print_driver);

    fclose(f);
    return true;
}

void dr_list_all(void);
void dr_list_available(void);

static bool file_is_zip(char *name) {
    unzFile *gz;
    gz=unzOpen(name);
    if (gz!=NULL) {
		  unzClose(gz);
		  return true;
    }
    return false;
}

static void free_ziplist_item(void *data) {
    free(data);
}

/* check if the driver dr correspond to the zip file pointed by gz 
   (zip_list contain the zip file content)
*/
static bool check_driver_for_zip(DRIVER *dr,unzFile *gz,LIST *zip_list) {
    int i;
    LIST *l,*zl;
    for (i=0;i<SEC_MAX;i++) {
    	//printf("Check section %d\n",i);
    	for(l=dr->section[i].item; l != NULL; l=l->next) {
    	    SECTION_ITEM *item=l->data;
			if (item->filename)
    	    if (strcmp(item->filename,"-")!=0) {
    			for(zl=zip_list;zl;zl=zl->next) {
    			    //printf("Check filename %s %s\n",(char*)zl->data,item->filename);
    			    if (strcasecmp(item->filename,(char*)zl->data)==0) {
    					//printf("filename %s=%s\n",(char*)zl->data,item->filename);
    					break;
    			    }
    			}
    			//printf("Zl %s = %p\n",item->filename,zl);
    			if (zl==NULL)
    			    return false;
    	    }
    	   
    	}
    }
    return true;
}

char *get_zip_name(char *name) {
    char *zip;
    char *path = CF_STR(cf_get_item_by_name("rompath"));
    if (file_is_zip(name)) {
	    zip=malloc(strlen(name)+1); CHECK_ALLOC(zip)
        strcpy(zip,name);
    } else {
        int len = strlen(path) + strlen(name) + 6;
        zip = malloc(len); CHECK_ALLOC(zip)
        sprintf(zip,"%s/%s.zip",path,name);
    }
    return zip;
}

/* return the correct driver for the zip file zip*/

DRIVER *get_driver_for_zip(char *zip) {
    unzFile *gz;
    int i;
    char zfilename[256];
    char *t;
    LIST *zip_list=NULL,*l,*zl;
    int res;

    /* first, we check if it a zip */
    gz = unzOpen(zip);
    if (gz==NULL) {
		return NULL;
    }
    //printf("Get driver for %s\n",zip);
    /* now, we create a list containing the content of the zip */
    i=0;
    unzGoToFirstFile(gz);
    do {
		unzGetCurrentFileInfo(gz,NULL,zfilename,256,NULL,0,NULL,0);
		//printf("List zip %s\n",zfilename);
		t=strrchr(zfilename,'.');
		if (! ( (strncasecmp(zfilename,"n",1)==0 && strlen(zfilename)<=12 )|| 
			(t && (strcasecmp(t,".rom")==0 || strcasecmp(t,".bin")==0) ) )
		    )
		    
		    i++;
		if (i>10) {
		    printf("More than 10 file are not rom....\n");
		    /* more than 10 files are not rom.... must not be a valid romset 
		       10 files should be enough */
		    list_erase_all(zip_list,free_ziplist_item);
		    return NULL;
		}
		zip_list=list_prepend(zip_list,strdup(zfilename));
    } while (unzGoToNextFile(gz)!=UNZ_END_OF_LIST_OF_FILE);
    
	//printf("Checking the lists of drivers:\n");
    /* now we check every driver to see if it match the zip content */
    for (l=driver_list;l;l=l->next) {
		DRIVER *dr=l->data;
		if (check_driver_for_zip(dr,gz,zip_list)==true) {
			unzClose(gz);
			list_erase_all(zip_list,free_ziplist_item);
			//printf("\t%s...bingo!\n",dr->name);
			return dr;
		}
    }
    list_erase_all(zip_list,free_ziplist_item);
    unzClose(gz);
    /* not match found */
    return NULL;
}

DRIVER *dr_get_by_name(char *name) {
    char *zip=get_zip_name(name);
    DRIVER *dr=get_driver_for_zip(zip);
    free(zip);
    return dr;
}

static int zfread(unzFile * f, void *buffer, int length)
{
    Uint8 *buf = (Uint8*)buffer;
    Uint8 *tempbuf;
    Uint32 totread, r, i;
    int totlength=length;
    totread = 0;
    tempbuf=alloca(4097);

    while (length) {
		r = length;
		if (r > 4096)
		    r = 4096;

		r = unzReadCurrentFile(f, tempbuf, r);
		if (r == 0) {
		    terminate_progress_bar();
		    return totread;
		}
		memcpy(buf, tempbuf, r);

		buf += r;
		totread += r;
		length -= r;
		update_progress_bar(totread,totlength);
    }

    terminate_progress_bar();
    return totread;
}

static int zfread_alternate(unzFile * f, void *buffer, int length, int inc)
{
    Uint8 *buf = buffer;
    Uint8 tempbuf[4096];
    Uint32 totread, r, i;
    int totlength=length;
    totread = 0;
    

    while (length) {

	r = length;
	if (r > 4096)
	    r = 4096;

	r = unzReadCurrentFile(f, tempbuf, r);
	if (r == 0) {
	    terminate_progress_bar();
	    return totread;
	}
	for (i = 0; i < r; i++) {
	    *buf = tempbuf[i];
	    buf += inc;
	}
	totread += r;
	length -= r;
	update_progress_bar(totread,totlength);
    }

    terminate_progress_bar();
    return totread;
}

  bool dr_load_section(unzFile *gz, SECTION s, Uint8 *current_buf) {
    LIST *l;
    for(l=s.item;l;l=l->next) {
      SECTION_ITEM *item=l->data;

      if (strcmp(item->filename,"-")!=0)
      {
        if ((unzLocateFile(gz, item->filename, 2) == UNZ_END_OF_LIST_OF_FILE) || (unzOpenCurrentFile(gz) != UNZ_OK))
        {
          unzClose(gz);
          return false;
        }
      }
      create_progress_bar(item->filename);
      if (item->type==LD_ALTERNATE) zfread_alternate(gz, current_buf + item->begin, item->size, 2);
      else zfread(gz, current_buf + item->begin, item->size);
    }
    return true;
  }

void set_bankswitchers(BANKSW_TYPE bt) {
    switch(bt) {
      case BANKSW_NORMAL:
            mem68k_fetch_bksw_byte=mem68k_fetch_bk_normal_byte;
            mem68k_fetch_bksw_word=mem68k_fetch_bk_normal_word;
            mem68k_fetch_bksw_long=mem68k_fetch_bk_normal_long;
            mem68k_store_bksw_byte=mem68k_store_bk_normal_byte;
            mem68k_store_bksw_word=mem68k_store_bk_normal_word;
            mem68k_store_bksw_long=mem68k_store_bk_normal_long;
            break;
      case BANKSW_KOF2003:
            mem68k_fetch_bksw_byte=mem68k_fetch_bk_kof2003_byte;
            mem68k_fetch_bksw_word=mem68k_fetch_bk_kof2003_word;
            mem68k_fetch_bksw_long=mem68k_fetch_bk_kof2003_long;
            mem68k_store_bksw_byte=mem68k_store_bk_kof2003_byte;
            mem68k_store_bksw_word=mem68k_store_bk_kof2003_word;
            mem68k_store_bksw_long=mem68k_store_bk_kof2003_long;
            break;
    }
}

bool dr_load_game(DRIVER *dr,char *name) {
    unzFile *gz;
    int i;
    LIST *l;
    char *zip=get_zip_name(name);
    gz = unzOpen(zip);
    free(zip);
	
	memory.mapped = 0;
	
    if (gz==NULL) {
		return false;
    }
	
	/* free the drivers to make more space 
	LIST *drl = driver_list;
	while( drl ) {
		if ( ((DRIVER*)drl->data) != dr)
			free((DRIVER*)drl->data);
		drl = drl->next;
	}
	*/
	
    if (dr->special_bios) {
	    memory.bios=malloc(dr->bios.size); CHECK_ALLOC(memory.bios);
      memory.bios_size=dr->bios.size;
      if (!dr_load_section(gz,dr->bios,memory.bios)) return false;
    }
	
    for (i=0;i<SEC_MAX;i++) {
		int s=dr->section[i].size;
		Uint8 *current_buf=NULL;
		//if (dr->section[i].item==NULL) continue;
		if (s==0) continue;
		//      printf("%p %d \n",dr->section[i].item,i);
		switch (i) {
			case SEC_CPU:
				memory.cpu = malloc(s); 
				if(!memory.cpu) { return false; }
				current_buf = memory.cpu;
				memory.cpu_size = s;
			    break;
			case SEC_SFIX:
				memory.sfix_game = malloc(s);
				if(!memory.sfix_game) { return false; }
				memory.fix_game_usage = malloc(s >> 5);
				if(!memory.fix_game_usage) { return false; }
			    current_buf = memory.sfix_game;
			    memory.sfix_size = s;
			    break;
			case SEC_SM1:
				memory.sm1 = malloc(s);
				if(!memory.sm1) { return false; }
			    current_buf = memory.sm1;
			    memory.sm1_size = s;
			    break;
			case SEC_SOUND1:
		        if (conf.sound) {
					memory.sound1 = malloc2(s);
					if (!memory.sound1) { return false; }
					memory.sound1_size = s;
					current_buf = memory.sound1;
		        }
				break;
			case SEC_SOUND2:
				if (conf.sound) {
	  				memory.sound2 = malloc2(s);
					if (!memory.sound2) { return false; }
	  				memory.sound2_size = s;
	  				current_buf = memory.sound2;
				}
				break; 
			case SEC_GFX:
				memory.gfx = malloc2(s); 
				/* don't need to check if the memory was available yet */
				#ifndef GFX_MAN 
				if (!memory.gfx) { free_game_memory(); return false; }
				#endif
				memory.gfx_size = s;
				current_buf = memory.gfx;
				memory.pen_usage = malloc((s >> 7) * sizeof(int));
				if (!memory.pen_usage) { free_game_memory(); return false; }
				memset(memory.pen_usage, 0, (s >> 7) * sizeof(int));
				memory.nb_of_tiles = s >> 7;
				//sleep(5);
			    break;
			    /* TODO: Crypted rom */
			default:
			    break;
		}
		/* if the GFX data space could not be allocated then the GFX section won't be loaded from the romset */
		if (current_buf) {
			if (!dr_load_section(gz,dr->section[i],current_buf)) return false;
		}
    }

    unzClose(gz);
	
	/* If there was not space enough for the GFX ROMs */
	if (!memory.gfx) {
	#ifdef GFX_MAN 
		char dump_name[512];
	    //Uint32 *pusage;
		//printf("\n\t\tNot enough room for GFX ROMs, time for BLIZZO-POWER!\n");
	    sprintf(dump_name,"%s/%s.gfx",DATA_DIRECTORY"/roms",dr->name);
		/* load the dump, if it cannot be loaded the function returns false */
		if ( init_gfx_manager(dump_name, memory.gfx_size) ) {
      if (read_pen_usage(memory.pen_usage))
        memory.mapped = 1;
		}
		if (!memory.mapped) {
			printf("\n\t\tUnable to load ROMSET. Sucks huh?\n");
			return false;
		}
   #else
		printf("\n\t\tUnable to load ROMSET. Sucks huh?\n");
		return false;
   #endif
	}
	
    /* TODO: Use directly the driver value insteed of recopying them */
    conf.game=dr->name;
    conf.rom_type=dr->rom_type;
    conf.special_bios=dr->special_bios;
    conf.extra_xor=dr->xor;
    neogeo_fix_bank_type = dr->sfix_bank;;
    set_bankswitchers(dr->banksw_type);
    for(i=0;i<6;i++)
		memory.bksw_unscramble[i]=dr->banksw_unscramble[i];
    for(i=0;i<64;i++)
		memory.bksw_offset[i]=dr->banksw_off[i];
	
	/* mapped tiles are already converted/decrypted */
	if (!memory.mapped) {		
		if (conf.rom_type == MGD2) {
			create_progress_bar("Convert MGD2");
			convert_mgd2_tiles(memory.gfx, memory.gfx_size);
			convert_mgd2_tiles(memory.gfx, memory.gfx_size);
			terminate_progress_bar();
		}
		
		if (conf.rom_type == MVS_CMC42) {
			create_progress_bar("Decrypt GFX ");
			kof99_neogeo_gfx_decrypt(conf.extra_xor);
			terminate_progress_bar();
		}
		
		if (conf.rom_type == MVS_CMC50) {
			create_progress_bar("Decrypt GFX ");
			kof2000_neogeo_gfx_decrypt(conf.extra_xor);
			terminate_progress_bar();
		}		
    } else if ((conf.rom_type == MVS_CMC42) || (conf.rom_type == MVS_CMC50)) {
	    create_progress_bar("Decrypt SFIX");
	    neogeo_sfix_decrypt();
	    terminate_progress_bar();
	}
	
    convert_all_char(memory.sfix_game, memory.sfix_size, memory.fix_game_usage);
	
	/* mapped tiles are already converted */
	if (!memory.mapped) {
		create_progress_bar("Convert tile");
		for (i = 0; i < memory.nb_of_tiles; i++) {
			convert_tile(i);
			if (i%100==0) update_progress_bar(i,memory.nb_of_tiles);
		}
		terminate_progress_bar();
	}
	
    if (memory.sound2 == NULL) {
		memory.sound2 = memory.sound1;
		memory.sound2_size = memory.sound1_size;
    }

    //backup neogeo game vectors
    memcpy(memory.game_vector,memory.cpu,0x80);
    printf("                                                                             \r");

    init_video();

    return true;
}

static int cmp_driver(void *a,void *b) {
    DRIVER *da=a;
    DRIVER *db=b;
    return strcmp(da->name,db->name);
}

void dr_list_all(void) {
    LIST *l;
    LIST *t=NULL;
    for(l=driver_list;l;l=l->next) {
		t=list_insert_sort_unique(t,l->data,cmp_driver);
    }
    for(l=t;l;l=l->next) {
		DRIVER *dr=l->data;
		printf("%-12s : %s\n",dr->name,dr->longname);
    }
}

LIST *driver_get_all(void) {
    return driver_list;
}
