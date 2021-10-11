/*	@(#)hash.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef	_NSE_HASH_H
#define	_NSE_HASH_H

/*
 * This file contains the routine definitions for all of the routines exported
 * by the hash table package.
 */

#include <nse/types.h>

typedef struct _bucket *Bucket;	/* Hash bucket */
struct _bucket {
	int	index;		/* Hash index = key_hash(key) */
	int	key;		/* Key */
	Bucket	next;		/* Next bucket in chain */
	int	value;		/* Bucket value */
};

typedef struct _hash *Hash;
struct _hash {
	Bucket	*buckets;		/* Bucket array */
	int	count;			/* Current number of entries */
	long	key_empty;		/* Value to use for an empty key */
	bool_t	(*key_equal)();		/* Key equality test routine */
	int	(*key_hash)();		/* Function to hash a key */
	int	(*key_insert)();	/* Function to insert a key */
	int	limit;			/* Maximum entries before rehashing */
	int	mask;			/* Hash index mask */
	long	value_empty;		/* Value to use for an empty value */
	int	(*value_insert)();	/* Routine to insert a value */
	int	slots;			/* Number of slots in hash table */
};

/*
 * hash_access(hash, key, value, create, replace, value_destroy, key_pointer,
 * value_pointer) =>{True, False} is used to access the a key-value pair in
 * Hash.  There are two cases that need to be considered.  The first case is
 * if Key is already in Hash.  In this case, if Create is True a Key and Value
 * will be inserted into Hash; otherwise key and value will not be inserted.
 * No matter what, False will be returned in this first case.  The second case
 * is if Key and Value are already in  Hash.  In this case, if Replace is True,
 * the value in hash will be replaced with Value.  If Value_Destroy is
 * non-NULL, the previous value will be destroyed by calling
 * Value_Destroy(Previous_Value).  No matter what, True will be returned in
 * this second case.  If Key_Pointer is non-NULL, the key in Hash will be
 * stored into *Key_Pointer.  If there is no Key in Hash, Key_Empty (from
 * hash_create) will be stored into *Key_Pointer.  Similarly, if Value_Pointer
 * is non-NULL, the value in Hash will be stored into *Value_Pointer.  If there
 * is no value in Hash, Value_Empty (from hash_create) will be stored into
 * *Value_Pointer.
 */
bool_t nse_hash_access();

/*
 * hash_create(Size, Key_Empty, Key_Equal, Key_Hash, Key_Insert, Value_Empty,
 * Value_Insert, 7) will create and return a hash table using the parameters.
 * Due to the large  number of arguments, the last argument must be the number
 * 7 so that a quick check can be made to make sure that they are all there.
 * All of the arguments except the last one can be NULL'ed out.
 */
Hash nse_hash_create();

/*
 * hash_destroy(hash, key_destroy, value_destroy) will deallocate the storage
 * associated with Hash.  Each key-value pair in Hash will be destroyed by
 * calling Key_Destroy(Key) and Value_Destroy(Value).  If Key_Destroy is NULL,
 * it will not be called.  Likewise, if Value_Destroy is NULL, it will not
 * be called.
 */
void nse_hash_destroy();

/*
 * hash_full_insert(hash, key, value, key_pointer, value_pointer)
 * =>{True, False} will insert Key-Value into Hash.  If Key is already in
 * Hash, the key and value already in Hash will not be affected and True will
 * be returned.  If Key is not already in Hash, Key and Value are inserted
 * into Hash and False is returned.  If Key_Pointer is non-NULL, *Key_Pointer
 * will be assigned the key stored in Hash.  If Value_pointer is non-NULL,
 * *Value_Pointer will be assigend the value stored in Hash.
 */
bool_t nse_hash_full_insert();

/*
 * hash_full_lookup(hash, key, key_pointer, value_pointer)=>{True, False} will
 * find the key-value pair associated with Key in Hash.  If Key is in Hash, the
 * key stored with Hash will be returned in *Key_Pointer, the value stored with
 * Hash will be returned in *Value_Pointer, and True will be returned.  If Key
 * is not in Hash, the empty key will be stored into *Key_Pointer, the empty
 * value will be stored into *Value_Pointer.  If Key_Pointer is NULL, no key
 * will be stored into it.  If Value_Pointer is NULL, no value will be stored
 * into it.
 */
bool_t nse_hash_full_lookup();

/*
 * hash_full_replace(hash, key, value, key_pointer, value_pointer)
 * =>{True, False} will insert Key-Value into Hash.  If Key is already in
 * Hash, the key and value will just be replaced and True will be returned.
 *  If Key is not already in Hash, Key and Value are inserted into Hash and
 * False is returned.  If Key_Pointer is non-NULL, *Key_Pointer will be
 * assigned the key stored in Hash.  If Value_pointer is non-NULL,
 * *Value_Pointer will be assigend the value stored in Hash.
 */
bool_t nse_hash_full_replace();

/*
 * hash_get(Hash, Key)=>Value will lookup the value for Key in Hash.  If Key
 * is not in Hash, a fatal error occurs.
 */
int nse_hash_get();

/*
 * hash_histogram(hash, histogram, size) will scan through Hash generating
 * a histogram of the bucket length and storing the result into the Size
 * words starting at Histogram.  The maximum desired histogram value will
 * be returned.
 */
int nse_hash_histogram();

/*
 * hash_histogram_display(hash, out_file) will print a histogram of Hash to
 * Out_File.
 */
void nse_hash_histogram_display();

/*
 * hash_insert(hash, key, value)=>{True,False} will attempt to find Key in
 * Hash.  If Key is already in Hash, True will be returned without affecting
 * Key's associated value.  Otherwise, Key and Value will be inserted into
 * Hash and False will be returned.
 */
bool_t nse_hash_insert();

/*
 * hash_lookup(Hash, Key)=>value will lookup Key in Hash.  If Key is not
 * in Hash, Empty_Value will be returned.
 */
int nse_hash_lookup();

/*
 * hash_read(hash, in_file, key_read, key_handle, value_read, value_handle)
 * will read a hash table from In_File into Hash.  Hash must be an empty
 * that was created in the same what that it was originally written out.
 * Key_Read(In_File, Key_Handle)=>Key to read a key from In_File and
 * Value_Read(In_File, Value_Handle, Key)=>Value to read a value from In_File.
 * The Key that was just read in is passed as the third argument to Value_Read.
 * If Key_Read or Value_Read are NULL, integer read routines will be used.
 */
void nse_hash_read();

/*
 * hash_replace(hash, key, value)=>{True,False} will attempt to find Key in
 * Hash.  If Key is not in Hash, Key and Value will be inserted into Hash
 * and False will be returned.  Otherwise, Value will replace the Key value
 * already in Hash, and True will be returned.
 */
bool_t nse_hash_replace();

/*
 * hash_scan(Hash, Routine, Data) will scan the entire contents of Hash
 * calling Routine(Key, Value, Data)=>int for each key-value pair in Hash.
 * The sum of the values returned by Routine will be returned.
 */
int nse_hash_scan();

/*
 * hash_show(Hash) will show the contents of Hash on the console.  This
 * routine is used for testing and debugging purposes only.
 */
void nse_hash_show();

/*
 * hash_size(Hash) returns the number of entries in Hash.
 */
int nse_hash_size();

/*
 * hash_write(hash, out_file, key_write, key_handle, value_write, value_handle)
 * will write out Hash to Out_File by calling
 * Key_Write(Out_File, Key, Key_Handle) to write Key to Out_File
 * Value_Write(Out_File, Value, Value_Handle) to write Valut to Out_File.
 * If Key_Write or Value_Write are NULL, integer write routines will be used.
 */
void nse_hash_write();

#endif

