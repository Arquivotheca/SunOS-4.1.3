/*	@(#)hashfn.h 1.1 92/07/30 SMI	*/
/*
 *    hashfn.h -- external declarations
 */

#ifndef hashfn_h
#define hashfn_h

#include <sys/types.h>

typedef struct he_ HashEntry;

struct he_ {
    HashEntry    *he_next;
    HashEntry    *he_prev;
    caddr_t        he_key;
    caddr_t        he_payload;
};

typedef struct ht_ HashTable;

struct ht_ {
    int            ht_size;
    
    /* hash func: int f(caddr_t) */
    int            (*ht_hash_fn)();

    /* compare func: int f(caddr_t, caddr_t) returns 0 for equal */
    int            (*ht_cmp_fn)();
    HashEntry    **ht_table;
};

#ifndef hashfn_c

extern HashTable *hashfn_new_table();
extern void hashfn_dispose_table();

extern caddr_t /* payload pointer */ hashfn_lookup();
extern caddr_t /* payload pointer */ hashfn_install();
extern caddr_t /* payload pointer */ hashfn_delete();

extern caddr_t /* key pointer */ hashfn_first_key();
extern caddr_t /* key pointer */ hashfn_next_key();

#endif hashfn_c
#endif hashfn_h
