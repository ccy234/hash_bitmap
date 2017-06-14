#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "spider_hash_bitmap.h"

//typedef struct {
//    uint16_t spider_id;
//    uint16_t ipmask_16;
//    char *name;
//    BITMAP *ip_map;
//}l7_spider_t;
//
//
//int init_spider_ip(ohtal_t *spider_htbl);
//int add_spider_ip(int spider_id, char *spider_name, int ip, int mask, ohtal_t *spider_htbl);
//int del_spider_ip(int spider_id, int ip, int mask, ohtal_t *spider_htbl);
//int check_spider_ip(int spider_id, int ip, ohtal_t *spider_htbl);
//void destory_spider_ip(ohtal_t *spider_htbl);
int main()
{
    int mask=30, ret=0;
    uint32_t ip;
    struct in_addr s;
    int spider_id1=1, spider_id2=2;
    char *ipn="192.168.0.4";//30
    char *ip1="192.168.0.6";
    char *ip2="192.168.0.17";
    char *spider_name="ccy";

    ohtbl_t *g_spider_htbl= (ohtbl_t *)calloc(1, sizeof(ohtbl_t));

    ret = init_spider_ip(g_spider_htbl);
    if (!ret) {
        printf("init spider ip fail\n");
    }
    printf("init spider ip sucess\n");

//0
    if (inet_pton(AF_INET, ipn, (void *)&s) == -1) {
        printf("turn ip fail\n");
        return -1;
    }
    ip = ntohl(s.s_addr);
    //ip = s.s_addr;

    ret = add_spider_ip(spider_id1, spider_name, ip, mask, g_spider_htbl);
    if (!ret) {
        printf("add spider %d-%s fail\n", spider_id1, ipn);
        return -1;
    }
    printf("add spider %d-0x%x sucess\n", spider_id1, ip);


//1
    if (inet_pton(AF_INET, ip1, (void *)&s) == -1) {
        printf("turn ip fail\n");
        return -1;
    }
    ip = ntohl(s.s_addr);
    //ip = s.s_addr;
    printf("check spider %d-0x%x sucess\n", spider_id1, ip);
    ret = check_spider_ip(spider_id1, ip, g_spider_htbl);
    if (!ret) {
        printf("%d-%s not in the spider lib\n", spider_id1, ip1);
    } else {
        printf("%d-%s in the spider lib\n", spider_id1, ip1);
    }
    
//2
    if (inet_pton(AF_INET, ip2, (void *)&s) == -1) {
        printf("turn ip fail\n");
        return -1;
    }
    ip = ntohl(s.s_addr);
    //ip = s.s_addr;
    printf("check spider %d-0x%x sucess\n", spider_id1, ip);
    ret = check_spider_ip(spider_id1, ip, g_spider_htbl);
    if (!ret) {
        printf("%d-%s not in the spider lib\n", spider_id1, ip2);
    } else {
        printf("%d-%s in the spider lib\n", spider_id1, ip2);
    }
    
//3
    if (inet_pton(AF_INET, ip1, (void *)&s) == -1) {
        printf("turn ip fail\n");
        return -1;
    }
    ip = ntohl(s.s_addr);
    //ip = s.s_addr;
    printf("check spider %d-0x%x sucess\n", spider_id2, ip);
    ret = check_spider_ip(spider_id2, ip, g_spider_htbl);
    if (!ret) {
        printf("%d-%s not in the spider lib\n", spider_id2, ip1);
    } else {
        printf("%d-%s in the spider lib\n", spider_id2, ip1);
    }

    destory_spider_ip(g_spider_htbl);
    return 0;
}
