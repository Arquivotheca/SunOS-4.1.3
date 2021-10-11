/*	@(#)hashfn.c 1.1 92/07/30 SMI	*/
/*
 *    hashfn.c
 */

#define hashfn_c

#include <sunwindow/hashfn.h>
#include <sunwindow/sv_malloc.h>

/*
 *    hashfn_new_table(size, hash_fn, cmp_fn) -- create a new hash table object
 *        of bucket table size size, with int hashing function hash_fn, and
 *        int comparison function cmp_fn which must return 0 on match; pointer
 *        to the new initialized table is returned
 */
HashTable *
hashfn_new_table(size, hash_fn, cmp_fn)
    int size;
    int            (*hash_fn)();
    int            (*cmp_fn)();
{
    HashTable *h;
    int i;

    h = (HashTable *)sv_malloc(sizeof(HashTable));
    h->ht_size = size;
    h->ht_hash_fn = hash_fn;
    h->ht_cmp_fn = cmp_fn;
    h->ht_table = (HashEntry **)sv_malloc(size * sizeof(HashEntry *));
    for (i = 0; i < size; i++)
        h->ht_table[i] = (HashEntry *)0;

    return h;
}

/*
 *    hashfn_dispose_table(h) -- destroy hash table h, freeing all its storage;
 *        the de-referenced key/payload structures are assumed not to contain
 *        pointers to dynamically allocated storage
 */
void
hashfn_dispose_table(h)
    HashTable    *h;
{
    int i;
    HashEntry *e, *f;
    
    for (i = 0; i < h->ht_size; i++)
        for (e = h->ht_table[i]; e ; e = f) {
            f = e->he_next;
            free(e->he_key);
            free(e->he_payload);
            free(e);
        }
            
    free(h->ht_table);
    free(h);
}

/* hide hash entry data type with local storage */

static int hashval;
static HashEntry *entry;

/*
 *    hashfn_lookup(h, key) -- lookup key in hash table; return payload pointer
 *        or null if not found
 */
caddr_t /* payload pointer */
hashfn_lookup(h, key)
    HashTable    *h;
    caddr_t        key;
{
    hashval = (*h->ht_hash_fn)(key) % h->ht_size; /* trust no one */
    
    for (entry = h->ht_table[hashval]; entry ; entry = entry->he_next)
        if (!(*h->ht_cmp_fn)(key, entry->he_key))
            return entry->he_payload;

    return (caddr_t)0;
}

/*
 *    hashfn_install(h, key, payload) -- install a key/payload pair into the
 *        table; note that the pointers themselves are installed so they
 *        should represent unpinned storage, or at least storage that nothing
 *        will happen to during hash table inquires; on deletion an attempt
 *        will be made to delete the key storage, and the payload pointer
 *        will be passed back
 */
caddr_t /* payload pointer */
hashfn_install(h, key, payload)
    HashTable    *h;
    caddr_t        key;
    caddr_t        payload;
{
    HashEntry    *e;

    if (hashfn_lookup(h, key)) {
        entry->he_payload = payload;
    } else {
        e = (HashEntry *)sv_malloc(sizeof(HashEntry));

        e->he_next = h->ht_table[hashval];    /* connect to next */
        if (e->he_next)
            e->he_next->he_prev = e;        /* connect next's prev to us */
        e->he_prev = (HashEntry *)0;        /* note we have no prev */
        h->ht_table[hashval] = e;            /* connect hash bucket to us */
    
        e->he_key = key;
        e->he_payload = payload;
    }
    
    return payload;
}

/*
 *    hashfn_delete(h, key) -- attempt to delete key from hash table; return
 *        payload pointer if deletion successful, otherwise return null
 *        indicating that the key did not exist
 */
caddr_t /* payload pointer */
hashfn_delete(h, key)
    HashTable    *h;
    caddr_t        key;
{
    caddr_t        p;
    HashEntry    *e, *f;

    if ((p = hashfn_lookup(h, key))) {
        if ((f = entry->he_prev))
            f->he_next = entry->he_next;    /* if prev connect his next */

        if ((f = entry->he_next))
            f->he_prev = entry->he_prev;    /* if next connect his prev */
            
        free(entry->he_key);     /* note key pointer is freed */
        free(entry);

        return p; /* return the payload pointer */
    } else {
        return (caddr_t)0; /* nothing deleted */
    }
}    

/* range traverse local storage */

static int bucket;
static HashEntry *tr_entry;

/*
 *    hashfn_first_key(h, payload) -- return first key in hash table; set payload
 *        pointer to key's payload; return null if empty hash table; traverse
 *        through the entries is by increasing hash bucket index and then
 *        reverse order of insertion into each bucket
 */
caddr_t /* key pointer */
hashfn_first_key(h, payload)
    HashTable    *h;
    caddr_t        *payload;
{
    
    for (bucket = 0; bucket < h->ht_size; bucket++)
        if ((tr_entry = h->ht_table[bucket])) {
            *payload = tr_entry->he_payload;
            return tr_entry->he_key;
        }

    return (caddr_t)0;
}

/*
 *    hashfn_next_key(h, payload) -- return next key in hash table; set payload
 *        pointer to key's payload; return null if at end of hash table
 */
caddr_t /* key pointer */
hashfn_next_key(h, payload)
    HashTable    *h;
    caddr_t        *payload;
{
    if ((tr_entry = tr_entry->he_next)) {
        *payload = tr_entry->he_payload;
        return tr_entry->he_key;
    } else {
        for (++bucket; bucket < h->ht_size; bucket++)
            if ((tr_entry = h->ht_table[bucket])) {
                *payload = tr_entry->he_payload;
                return tr_entry->he_key;
            }

        return (caddr_t)0;
    }
}
