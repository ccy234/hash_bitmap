#ifndef BITMAP_H
#define BITMAP_H 1

typedef unsigned int uint32_t;

typedef struct {
    #define SHIFT 5
    #define MASK 0x1F

    int size;
    uint32_t *map;
}BITMAP;

BITMAP *create_bitmap(int size);
int set_bitmap(int num, BITMAP *map);
int clr_bitmap(int num, BITMAP *map);
int check_bitmap(int num, BITMAP *map);
int destory_bitmap(BITMAP *map);

#endif /*BITMAP_T*/
