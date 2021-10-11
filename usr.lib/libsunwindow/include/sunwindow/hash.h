/*	@(#)hash.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file contains the routine definitions for all of the routines exported
 * by the hash table package.
 */

#ifndef	hash_defined
#define	hash_defined

/*
 * Do not typedef Hash if this header file is being used by hash.c.
 */
#ifndef Hash_defined
#define	Hash_defined
struct _hash {
	int count;		/* Current number of entires */
	long key_empty;		/* Value to use for an empty key */
	char (*key_equal)();	/* Routine to test equality of two keys */
	int (*key_hash)();	/* Function to hash a key */
	int (*key_insert)();	/* Function to insert a key */
	long *keys;		/* Array of keys */
	int limit;		/* Maximum entries before rehashing */
	long value_empty;	/* Value to use for an empty value */
	int (*value_insert)();	/* Routine to insert a value */
	long *values;		/* Values array */
	int slots;		/* Number of slots in hash table */
};
typedef struct _hash *Hash;
#endif

/*
 * hash_create(Size, Key_Empty, Key_Equal, Key_Hash, Key_Insert, Value_Empty,
 * Value_Insert, 7) will create and return a hash table using the parameters.
 * Due to the large  number of arguments, the last argument must be the number
 * 7 so that a quick check can be made to make sure that they are all there.
 * All of the arguments except the last one can be NULL'ed out.
 */
Hash hash_create();

/*
 * hash_find(Hash, Key, &Value)=>{True,False} will lookup Key in Hash.  If Key
 * is found in Hash, True will be returned and the associated value will be
 * stored into Value, provided Value is non-Null.  Otherwise, False will be
 * returned.
 */
Bool hash_find();

/*
 * hash_get(Hash, Key)=>Value will lookup the value for Key in Hash.  If Key
 * is not in Hash, a fatal error occurs.
 */
long hash_get();

/*
 * hash_insert(Hash, Key, Value)=>{True,False} will insert Value into Hash
 * under Key.  If Key is already in Hash, False will be returned and the
 * previous value will not be changed.  Otherwise, True will be returned.
 */
Bool hash_insert();

/*
 * hash_lookup(Hash, Key)=>value will lookup Key in Hash.  If Key is not
 * in Hash, Empty_Value will be returned.
 */
long hash_lookup();

/*
 * hash_replace(Hash, Key, Value)=>{True,False} will insert Value into Hash
 * under Key.  If Key was already in Hash, True will be returned and the
 * previous value will be replaced.  Otherwise, False will be returned and
 * the Value will be inserted under Key.
 */
Bool hash_replace();

/*
 * hash_show(Hash) will show the contents of Hash on the console.  This
 * routine is used for testing and debugging purposes only.
 */
void hash_show();

/*
 * hash_size(Hash) returns the number of entries in Hash.
 */
int hash_size();

#endif
#define	SMALLEST_BLK	sizeof(struct dblk) 	/* Size of smallest block */


/*
 * Description of a data block.  
 * A data block consists of a length word, possibly followed by
 * a filler word for alignment, followed by the user's data.
 * To back up from the user's data to the length word, use
 * (address of data) - ALIGNSIZ;
 */

#ifdef sparc
#define ALIGNSIZ	sizeof(double)
struct	dblk	{
	long	size;			/* Size of the block */
	long	filler;			/* filler, for double alignment */
	char	data[ALIGNSIZ];		/* Addr returned to the caller */
};
#endif

#ifdef mc68000
#define ALIGNSIZ	sizeof(long)
struct	dblk	{
	long	size;			/* Size of the block */
	char	data[ALIGNSIZ];		/* Addr returned to the caller */
};
#endif

#define	roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
