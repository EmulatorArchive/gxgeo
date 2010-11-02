#ifndef _TRANSPACK_H_
#define _TRANSPACK_H_

enum {
    TILE_UNCONVERTED=0,
    TILE_NORMAL,
    TILE_INVISIBLE,
    TILE_TRANSPARENT25,
    TILE_TRANSPARENT50,
    TILE_FULL            /* Special cas for optimisation: tile doesn't contain
                            any transparent pixels */
};

typedef struct TRANS_PACK {
    Uint32 begin, end;
    Uint8 type;
    struct TRANS_PACK *next;
} TRANS_PACK;
TRANS_PACK *tile_trans;

TRANS_PACK* trans_pack_find(Uint32 tile);
void trans_pack_open(char *filename);

#endif
