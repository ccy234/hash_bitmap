/*
 * Copyright (c) 2014, Jusse Wang <wanyco@gmail.com>.
 * All rights reserved.
 *
 * @date: 2014-11-07 
 * @file ohtbl.c
 * @brief open-addressed hash tables implementation file
 *
 */

/* ohtbl.c - open-addressed hash tables                                   */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by   */
/* the Free Software Foundation; either version 2, or (at your option)    */
/* any later version.                                                     */
/*                                                                        */
/* This program is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/* GNU General Public License for more details.                           */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, write to the Free Software            */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA              */
/* 02111-1307, USA.                                                       */

/* code is based on "Mastering Algorithms with C", Kyle Loudon */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <linux/types.h>
#include <assert.h>

#include "ohtbl.h"

#define l7_log_err printf
/** 
 * Reserve a memory location
 * when a element be removed, the hash table element's data pointer will be set to the vacated's address
 */
static char vacated;

/**
 * @function: obthl_init
 * @bried: open-addressed hash table initialization
 * @param: htbl: the open-addressed hash table pointer, you must allocate it well before.
 *         size: the hash table size
 *         h1:   calculate hash value function pointer 1
 *         h2:   calculate hash value function pointer 2
 *         match:match  hash table element function pointer
 *         alloc:       allocate hash table memory function pointer
 *         destroy:     free/destroy hash table memory function pointer
 *         tolerance:   the maximum number of hash table element tolerance
 * @return: 1 if everything ok, otherwise return 0.
 */
uint16_t g_proc_index = 0;
int ohtbl_init(ohtbl_t *htbl, unsigned int size, unsigned int (*h1)(const void *elem),
                unsigned int (*h2)(const void *elem), int (*match)(const void *ohtbl_elem, const void *elem),
                void *(*alloc)(size_t), void (*destroy)(void *data), unsigned int tolerance)
{
    unsigned int i;
    unsigned int hash_size_byte, active_size_byte;

    rwlock_init(&htbl->lock);

    hash_size_byte = (size + 1) * sizeof(struct ohtbl_item);
    htbl->table = (struct ohtbl_item *)alloc(hash_size_byte);
    if (htbl->table == NULL) {
        return 0;
    }
    memset(htbl->table, 0, hash_size_byte);


    active_size_byte = (size + 1) * sizeof(unsigned int);
    htbl->active = (unsigned int *)alloc(active_size_byte);
    if (htbl->active == NULL) {
        destroy(htbl->table);
        htbl->table = NULL;
        return 0;
    }
    memset(htbl->active, 0, active_size_byte);


    /* Initialize each position */
    htbl->size = size;
    for (i = 0; i < htbl->size; i++) {
        htbl->table[i].data = NULL;
    }


    /* Set the vacated member to the sentinel memory address reserved for this */
    htbl->vacated = &vacated;

    /* Encapsulate the functions */
    htbl->h1 = h1;
    htbl->h2 = h2;
    htbl->match = match;
    htbl->destroy = destroy;

    /* Initialize the number of elements in the table */
    htbl->count = htbl->conflict = 0;

    /* Initialize tolerance */
    htbl->tolerance = tolerance >= size ? (size * 0.8) : tolerance;

    return 1;

}

/**
 * @function: ohtbl_reset
 * @brief: reset all the hash table element to zero, you have to manage the element pointer (e.g. reuse/free)
 * @param: htbl: the open-addressed hash table which to reset.
 * @return: void
 */
void ohtbl_reset(ohtbl_t *htbl)
{
    int lock_ret;
    unsigned int hash_size_byte, active_size_byte;

    if (htbl == NULL) {
        return;
    }


    lock_ret = rwlock_wrlock_ms(&htbl->lock, 5000);

    if (lock_ret == 0) {
        l7_log_err("hash table wrlock() error, write: %d --- flag: %"PRIu64", read: %d --- flag: %"PRIu64"", htbl->lock.write, htbl->lock.wrflag, htbl->lock.read, htbl->lock.rdflag);
    }
    
    hash_size_byte = (htbl->size + 1) * sizeof(struct ohtbl_item);
    memset(htbl->table, 0, hash_size_byte);

    active_size_byte = (htbl->size + 1) * sizeof(uint32_t);
    memset(htbl->active, 0, active_size_byte);

    htbl->count = htbl->conflict = 0;
    
    rwlock_init(&htbl->lock);
    rwlock_wrunlock(&htbl->lock);

}

/**
 * @function: ohtbl_destroy
 * @brief: destroy the hash table
 * @param: htbl: the open-addressed hash table which to destroy.
 * @return: void
 */
void ohtbl_destroy(ohtbl_t *htbl)
{
    unsigned int i;

    if (htbl->destroy != NULL){
        /* Call a user-defined function to free dynamically allocated data */
        for (i = 0; i < htbl->size; i++) {
             if (htbl->table[i].data != NULL && htbl->table[i].data != htbl->vacated) {
                htbl->destroy(htbl->table[i].data);
             }
        }
    }
    
    /* Free the storage allocated for the actives table */
    htbl->destroy(htbl->active);

    /* Free the storage allocated for the hash table */
    htbl->destroy(htbl->table);
    //* No operations are allowed now, but clear the structure as a precaution */
    memset(htbl, 0, sizeof(ohtbl_t));
}

/**
 * @function: obthl_insert
 * @brief: insert elem into hash table
 * @param: htbl: the hash table which to insert
 *         elem: hash table element
 * @return: 1 if there is no conflict, return -1 when the hash table element number 
 *          reached the maximum tolerated value, otherwise return 0
 */
int ohtbl_insert(ohtbl_t *htbl, void *elem)
{
    int lock_ret;
    unsigned int key = 0;

    /* Do not exceed the tolerance in the table */
    if (htbl->count == htbl->tolerance) {
        return -1;
    }

    /* Do nothing if the data is already in the table */
    if (__ohtbl_lookup(htbl, elem, &key) != NULL) {
        return 0;
    }

    /* Insert the data into the table */
    htbl->table[key].data = elem;
    htbl->table[key].hash = key;
    
    /* Record pos in active[] */
    lock_ret = rwlock_wrlock_ms(&htbl->lock, 1000);
    if (lock_ret == 0) {
        l7_log_err("hash table wrlock() error, write:%d --- flag: %"PRIu64", read:%d --- flag: %"PRIu64"", 
                        htbl->lock.write, htbl->lock.wrflag, htbl->lock.read, htbl->lock.rdflag);
    }

    htbl->active[htbl->count] = key;
    htbl->count++;
    htbl->table[key].pos = htbl->count;
    
    if (lock_ret != 0) {
        rwlock_wrunlock(&htbl->lock);
    }
    

    if (key != htbl->h1(elem) % htbl->size) {
        htbl->conflict++;
    }

    return 1;
}

/**
 * @function: obtbl_remove
 * @brief: remove the elem from hash table
 * @param: htbl: the hash table
 *         elem: the element which to remove
 * @return: the element pointer which be removed, or NULL
 */
void *ohtbl_remove(ohtbl_t *htbl, const void *elem)
{
    int lock_ret;
    unsigned int pos = 0;
    unsigned int tmp_key = 0;
    unsigned int i = 0;
    unsigned int key = 0;
    unsigned int next_key = 0;
    unsigned int is_conflict = 0;
    void *ret = NULL;

    if (htbl == NULL || htbl->count == 0) {
        return NULL;
    }

    for (i = 0; i < htbl->size; i++){
        key = (htbl->h1(elem) + i * htbl->h2(elem)) % htbl->size;
        if (htbl->table[key].data == NULL) {
            /* Return that the elem was not found */
            return NULL;
        } else if (htbl->table[key].data == htbl->vacated) {
            /* Search beyond vacated positions */
            continue;
        } else if (htbl->match(htbl->table[key].data, elem)) {
            /* Pass back the elem from the table */
            ret = htbl->table[key].data;
            htbl->table[key].data = htbl->vacated;


            /* Remove pos in active[] */            
            lock_ret = rwlock_wrlock_ms(&htbl->lock, 1000);
            if (lock_ret == 0) {
                l7_log_err("hash table wrlock() error, write:%d --- flag: %"PRIu64", read:%d --- flag: %"PRIu64"", 
                                htbl->lock.write, htbl->lock.wrflag, htbl->lock.read, htbl->lock.rdflag);
            }

            htbl->count--;
            pos = htbl->table[key].pos;
            if (pos <= htbl->count) {
                tmp_key = htbl->active[htbl->count];
                htbl->active[pos - 1] = tmp_key;
                htbl->table[tmp_key].pos = pos;
            }
            
            if (lock_ret != 0) {
                rwlock_wrunlock(&htbl->lock);
            }


            if ((i + 1) < htbl->size) {
                next_key = (htbl->h1(elem) + (i + 1) * htbl->h2(elem)) % htbl->size;
                if (htbl->table[next_key].data != NULL) {
                    is_conflict = 1;
                }
            }
            if (is_conflict == 1 && htbl->conflict > 0) {
                htbl->conflict--;
            }

            return ret;
        }
        is_conflict = 1;
    }

    return ret;
}


#if 0
/**
 * @function: ohtbl_flush
 * @brief: flush current position for traversing
 * @param: htbl: the hash table
 * @return: void
 */
void ohtbl_flush(ohtbl_t *htbl)
{
    htbl->cur = htbl->head;
}

/**
 * @function: ohtbl_traverse
 * @brief: Traverse hash table
 * @param: htbl: the hash table
 * @return: the hash table element pointer, or NULL
 */
void *ohtbl_traverse(ohtbl_t *htbl)
{
    if (htbl->cur == htbl->tail) {
        return NULL;
    }

    htbl->cur = htbl->cur->next;

    return htbl->cur->data;
}
#endif

/**
 * @function: ohtbl_foreach
 * @brief: traverse the hash table, and calls the callback function on each element
 * @param: htbl: the hash table
 *         callback: the callback function pointer
 *         data: the argument of callback function
 * @return: void
 */
void ohtbl_foreach(ohtbl_t *htbl, void (*callback)(void *ohtbl_elem, void *data), void *data)
{
    int lock_ret;
    unsigned int pos, key;
    void *elem;

    if (htbl == NULL || callback == NULL) {
        return;
    }
    
    lock_ret = rwlock_rdlock_ms(&htbl->lock, 2000);
    if (lock_ret == 0) {
        l7_log_err("hash table rdlock() error, write:%d --- flag: %"PRIu64", read:%d --- flag: %"PRIu64"", 
                        htbl->lock.write, htbl->lock.wrflag, htbl->lock.read, htbl->lock.rdflag);

    }
    
    for(pos = 0; pos < htbl->count; pos++) {
        key = htbl->active[pos];
        elem = htbl->table[key].data;
    
        callback(elem, data);
    }
    
    if (lock_ret != 0) {
        rwlock_rdunlock(&htbl->lock);
    }
}


/**
 * @function: __ohtbl_lookup
 * @brief: lookup the elem whether in the hash table
 * @param: htbl: the hash table
 *         elem: the element which will to be lookup
 *         ekey: use to save the elem's hash table key
 * @return: the hash table element pointer, or NULL
 */
void *__ohtbl_lookup(ohtbl_t *htbl, const void *elem, unsigned int *ekey)
{
    unsigned int key = 0;
    unsigned int i = 0, n = 0;

    /* Use double hashing to hash the key */
    for (i = 0; i < htbl->size; i++) {
        key = (htbl->h1(elem) + i * htbl->h2(elem)) % htbl->size;
        if (htbl->table[key].data == NULL) {
            /* Return that the elem was not found */
            if (ekey != NULL && n == 0) {
                *ekey = key;
                n++;
            }
            return NULL;
        } else if (htbl->table[key].data == htbl->vacated) {
            if (ekey != NULL && n == 0) {
                *ekey = key;
                n++;
            }
            continue;
        } else if (htbl->match(htbl->table[key].data, elem)) {
            if (ekey != NULL) {
                *ekey = key;
            }
            /* Data was found */
            return htbl->table[key].data;
        }
    }

    /* Return that the elem was not found */
    return NULL;
}

