#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"

//#define SHIFT 5
//#define MASK 0x1F

BITMAP *create_bitmap(int size)
{
    BITMAP *pmap;
    
    pmap = (BITMAP *)calloc(1, sizeof(BITMAP));
    pmap->size = (size >> SHIFT) + 1;
    pmap->map = (uint32_t *)calloc(pmap->size, sizeof(uint32_t));
    return pmap;
}
int set_bitmap(int num, BITMAP *bitmap)
{
    int index_loc, bit_loc;
    uint32_t *map = bitmap->map;

    index_loc = num >> SHIFT; //num / 32
    bit_loc = num & MASK; // num % 32

    map[index_loc] |= 1 << bit_loc;
    return 1;
}

// return 0 false, 1 true
int clr_bitmap(int num, BITMAP *bitmap)
{
    int index_loc, bit_loc;
    uint32_t *map = bitmap->map;

    index_loc = num >> SHIFT;
    bit_loc = num & MASK;
    map[index_loc] &= ~(1 << bit_loc);
    return 1;
}

// return 0 false, 1 true
int check_bitmap(int num, BITMAP *bitmap)
{
    int i, flag;
    uint32_t *map = bitmap->map;

    i = 1 << (num & MASK);

    flag = map[num >> SHIFT] & i;

    return flag;
}

// return 0 false, 1 true
int destory_bitmap(BITMAP *bitmap)
{
    if (bitmap == NULL) {
        return 1;
    }
    if (bitmap->map != NULL) {
        free(bitmap->map); 
    }
    free(bitmap);
    return 1;
}
#if 0
int main(void)
{
    int i, num, space;
    BITMAP *bitmap;

    bitmap = create_bitmap(30); 
    printf("please select type> 1: set bit 2: clear bit 3: check bit\n");
    while (scanf("%d", &num) != EOF) {
        switch (num) {
        case 1:
            printf("input num:");
            scanf("%d", &num);
            set_bitmap(num, bitmap);
            printf("set %d sucess!\n", num);
            break;
        case 2:
            printf("input num:");
            scanf("%d", &num);
            clr_bitmap(num, bitmap);
            printf("clr %d sucess!\n", num);
            break;
        case 3:
            printf("input num:");
            scanf("%d", &num);
            if (check_bitmap(num, bitmap)) {
                printf("sucess!\n");
            } else {
                printf("fail!\n");
            }
            break;
        default:
            break;
        }
        printf("please select type> 1: set bit 2: clear bit 3: check bit\n");
    }
    destory_bitmap(bitmap);
    return 0;
}
#endif
