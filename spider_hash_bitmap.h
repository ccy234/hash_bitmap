#include "bitmap.h"
#include "ohtbl.h"

#ifndef SPIDER_HASH_BITMAP_H
#define SPIDER_HASH_BITMAP_H 1

#define BITMAP_SIZE 65536
#define SPIDER_IP_HTBL_SIZE 9997

typedef struct {
    char *name;
    uint32_t hash;
    uint16_t spider_id;
    uint16_t ipmask_16;
    BITMAP *ip_map;
}l7_spider_t;


int init_spider_ip(ohtbl_t *spider_htbl);
int add_spider_ip(uint16_t spider_id, char *spider_name, uint32_t ip, int mask, ohtbl_t *spider_htbl);
int del_spider_ip(uint16_t spider_id, uint32_t ip, ohtbl_t *spider_htbl);
int check_spider_ip(uint16_t spider_id, uint32_t ip, ohtbl_t *spider_htbl);
void destory_spider_ip(ohtbl_t *spider_htbl);


#endif
