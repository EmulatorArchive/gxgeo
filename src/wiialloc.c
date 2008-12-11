 /*  wii-malloc allocate memory in MEM2
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
  #include <string.h>

  #include "wiialloc.h"
  
  #define MEM2_LO    0x90080000      
  #define MEM2_HI    0x93080000	     /* IOS Heap starts here */
  #define MEM2_SIZE  MEM2_HI - MEM2_LO
  
  typedef struct MEM2_item {
	size_t 				size;
	void   				*offset;
	struct MEM2_item* 	next;
  } MEM2_item;
  
  static MEM2_item *memory = NULL;
  
  int MEM2_item_compare(MEM2_item* first, MEM2_item* second)
  {
	/* if the first pointer is higher than the second return 1 else 0 */
	return (first->offset > second->offset);
  }
  
  void MEM2_item_swap(MEM2_item** first, MEM2_item** second)
  {
	/* store the first pointer */
	MEM2_item *tmp = *first;
	/* set the first pointer to the second pointer */
	*first = *second;
	/* set the second pointer to the first pointer (stored in tmp) */
	*second = tmp;
  }
  
  void MEM2_item_sort(MEM2_item *list)
  {
	if (list) {
		MEM2_item *item;
		int swapped;
		do {
			swapped = 0;
			/* loop all the currently allocated memory items */
			for (item = memory; item->next; item = item->next) {
				/* if the current item is greater than the next item */
				if (MEM2_item_compare(item, item->next)) {
					/* swap them */
					MEM2_item_swap(&item, &item->next);
					swapped = 1;
				}
			}
		} while (swapped);
	}
  }
  
  void *calloc2(size_t nelem, size_t elsize)
  {
	void* temp = malloc2(nelem*elsize);
	memset(temp, 0, nelem*elsize);
	return temp;
  }
  
  void *malloc2(size_t size)
  {
	if (size == 0 || size > MEM2_SIZE) return NULL;
	
	MEM2_item *mem_item;
	unsigned int to_insert;
	
	/* if the memory list is NULL, ie no memory has been allocated then it can go at MEM2_LO */
	to_insert = 0;
	/* printf("\t\tmemory: %p\n\t\tsize wanted:%x\n",memory,size); */
	if (memory) {
		/* loop all the currently allocated memory items */
		for (mem_item = memory; mem_item; mem_item = mem_item->next) {
			/* printf("\t\t0x%x allocated at 0x%x\t\n",mem_item->size, (unsigned int)mem_item->offset);*/
			if ((unsigned int)mem_item->offset - to_insert > size) break;
			to_insert = (unsigned int)mem_item->offset + mem_item->size;
		}
	}
	if (MEM2_SIZE - to_insert >= size) {
		/* insert a new item at the end of the list */
		for (mem_item = memory; mem_item && mem_item->next; mem_item = mem_item->next);
		if (!mem_item) { 
			memory = (MEM2_item*) malloc(sizeof(MEM2_item));
			memory->size = size;
			memory->offset = (void*)to_insert;
			memory->next = NULL;
		} else {
			mem_item->next = (MEM2_item*) malloc(sizeof(MEM2_item));
			mem_item->next->size = size;
			mem_item->next->offset = (void*)to_insert;
			mem_item->next->next = NULL;
		}
		//printf("\t\t%p\n",(void*)(to_insert+MEM2_LO));
		return (void*)(to_insert+MEM2_LO);
	}
	else return NULL;
  }
  
  void free2(void *ptr)
  {
	/* if ptr is not NULL */
	if (ptr) {
		unsigned int offset = (unsigned int)ptr - MEM2_LO;
		MEM2_item *previous = NULL;
		MEM2_item *mem_item = NULL;
		/* loop all the memory items in the list */
		for (mem_item = memory; mem_item; mem_item = mem_item->next) {
			/* if the input offset matches the stored one */
			if ((unsigned int)mem_item->offset == offset) {
				/* remove it from the list */
				(previous?previous:memory)->next = mem_item->next;
				/* remove it from memory */
				if (mem_item==memory) { memory = NULL; }
				free(mem_item);
				
				break;
			}
			/* if it's not then go on to the next one and store the current item so it can be updated */
			previous = mem_item;
		}
	}
  }
