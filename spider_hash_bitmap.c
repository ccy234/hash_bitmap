#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "spider_hash_bitmap.h"

#define IP_SHIFT 16
#define BITMAP_SIZE 65536
#define SPIDER_IP_HTBL_SIZE 9997


unsigned int spider_ip_elem_h1(const void *key)
{
    return (((l7_spider_t*)key)->hash % SPIDER_IP_HTBL_SIZE);
}

unsigned int spider_ip_elem_h2(const void *key)
{
    return (1 + ((l7_spider_t *)key)->hash % (SPIDER_IP_HTBL_SIZE -2));
}

int spider_ip_elem_match(const void *key1, const void *key2) 
{
    l7_spider_t *p1 = (l7_spider_t *)key1;
    l7_spider_t *p2 = (l7_spider_t *)key2;

    if (p1 == NULL || p2 == NULL)
        return 0;
    return (p1->hash == p2->hash) && (p1->ipmask_16 == p2->ipmask_16) && (p1->spider_id == p2->spider_id);
}


uint32_t l7_shm_fnv_32a_str(char *str, uint32_t str_len, uint32_t hval)
{
    unsigned char *s = (unsigned char *)str;    /* unsigned string */
    uint32_t i = 0;

    if (s == NULL) {
        return 0;
    }

    /*
     * FNV-1a hash each octet in the buffer
     */
    while (i<str_len) {
        i++;

        /* xor the bottom with the current octet */
        hval ^= (uint32_t)*s++;

        /* #define NO_FNV_GCC_OPTIMIZATION */
        /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
        /*
         * 32 bit magic FNV-1a prime
         */
#define FNV_32_PRIME ((uint32_t)0x01000193)
        hval *= FNV_32_PRIME;
#else
        hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }

    /* return our new hash value */
    return hval;
}

l7_spider_t *find_ip_ret(uint16_t spider_id, uint32_t ip, ohtbl_t *htbl, unsigned int *ekey)
{
    l7_spider_t spider_elem, *ret=NULL;
    uint32_t hash = 0;
    char str_ip[16]={0};
    
    spider_elem.spider_id = spider_id;
    spider_elem.ipmask_16 = ip >> IP_SHIFT;
    snprintf(str_ip, sizeof(str_ip), "%u-%u", spider_elem.spider_id, spider_elem.ipmask_16); 
    hash = l7_shm_fnv_32a_str(str_ip, sizeof(str_ip), 0);
    spider_elem.hash = hash;
    ret = (l7_spider_t*)__ohtbl_lookup(htbl, &spider_elem, ekey);
    return ret;
}

//return 1 sucess, 0 false
int init_spider_ip(ohtbl_t *spider_htbl)
{
    return ohtbl_init(spider_htbl, SPIDER_IP_HTBL_SIZE,  spider_ip_elem_h1, spider_ip_elem_h2, spider_ip_elem_match, malloc, free, SPIDER_IP_HTBL_SIZE*0.8);
}

//return 1 sucess, 0 false
int add_spider_ip(uint16_t spider_id, char *spider_name, uint32_t ip, int mask, ohtbl_t *spider_htbl)
{
    int htbl_insert_ret;
    uint32_t key, hash, i;
    BITMAP *ipmap=NULL;
    char str_ip[16]={0};
    uint32_t start = (ip & 0xffff);
    uint32_t end = start + pow(2, 32-mask);
    l7_spider_t *new_spider_ip_elem, *ret;

    ret = find_ip_ret(spider_id, ip, spider_htbl, &key);
    if (ret == NULL) {
        // insert elem into hashtab
        new_spider_ip_elem = malloc(sizeof(l7_spider_t));
        new_spider_ip_elem->spider_id = spider_id;
        new_spider_ip_elem->name = malloc(strlen(spider_name) + 1);
        snprintf(new_spider_ip_elem->name, sizeof(new_spider_ip_elem->name), "%s", spider_name);//fix
        new_spider_ip_elem->ipmask_16 = ip >> IP_SHIFT; 

        snprintf(str_ip, sizeof(str_ip), "%u-%u", new_spider_ip_elem->spider_id, new_spider_ip_elem->ipmask_16);
        hash = l7_shm_fnv_32a_str(str_ip, sizeof(str_ip), 0);
        new_spider_ip_elem->hash = hash;

        ipmap = create_bitmap(BITMAP_SIZE);
        if (ipmap == NULL) {
            return 0;
        }
        new_spider_ip_elem->ip_map = ipmap;
        for (i=start; i<end; i++) {
            // set ip map
            set_bitmap(i, ipmap);
        }

        htbl_insert_ret = ohtbl_insert(spider_htbl, new_spider_ip_elem);
        if (htbl_insert_ret != 1) {
            //l7_log_err("insert %s spider into name_id hashtable fail", file_name);
            printf("error insert\n");
            return 0;
        }
        new_spider_ip_elem = NULL;
    } else {
        // have exist elem in hashtal, add ip directly
        ipmap = ret->ip_map;
        for (i=start; i<end; i++) {
            // set ip map
            set_bitmap(i, ipmap);
        }
    }
    return 1;
}

//return 1 sucess, 0 false
int del_spider_ip(uint16_t spider_id, uint32_t ip, ohtbl_t *g_spider_htbl)
{
    //l7_spider_t new_spider_ip_elem, *ret;
    //unsigned int *key;
    //int i;

    ////create key
    //ret = find_ip_ret(spider_id, ip, g_spider_htbl, key);
    //if (ret != NULL) {
    //    // have exist elem in hashtal, add ip directly
    //} else {
    //    // insert elem into hashtab
    //}

    // clr ip map

}

//return 1 inside, 0 outside
int check_spider_ip(uint16_t spider_id, uint32_t ip, ohtbl_t *g_spider_htbl)
{
    l7_spider_t  *ret;
    unsigned int key;

    ret = find_ip_ret(spider_id, ip, g_spider_htbl, &key);
    if (ret == NULL) {
        // there is no this elem in hashtab
        return 0;
    }
    // have exist elem in hashtal, check if the ip is in bitmap
    return check_bitmap((ip & 0xffff), ret->ip_map);
}

void callback_spider_ip_bitmap_free(void *ohtbl_elem, void *data)
{
    l7_spider_t *spider_ip = (l7_spider_t *)ohtbl_elem;

    if (spider_ip == NULL) 
        return ;
    if (spider_ip->name != NULL) {
        free(spider_ip->name);
        spider_ip->name = NULL;
    }
    if (spider_ip->ip_map != NULL) {
        destory_bitmap(spider_ip->ip_map);
        spider_ip->ip_map = NULL;
    }
}

void destory_spider_ip(ohtbl_t *g_spider_htbl)
{
    ohtbl_foreach(g_spider_htbl, callback_spider_ip_bitmap_free, NULL);
    ohtbl_destroy(g_spider_htbl);
}

