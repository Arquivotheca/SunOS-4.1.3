#ifndef lint
static char sccsid[] = "@(#)hash.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file contains a bunch of routines that implement a general purpose
 * hash table.
 */

#include <stdio.h>		/* Standard I/O definitions */
#include <nse/hash.h>		/* Include hash table exported routines */

#define	EMPTY	((Bucket)NULL)	/* Empty slot */
#define	HASH_MAGIC 0xadff1582	/* Magic number for hash read and write */
#define	REHASH	((Bucket)1)	/* Slot that needs to be rehashed */
#define	SPECIAL	2		/* All special slot values less than this */

#define True TRUE
#define False FALSE
typedef bool_t (*Bool_routine)();
typedef int (*Int_routine)();
typedef void (*Void_routine)();


/*
 * Imported routines:
 */
extern char	*malloc();
extern char	*valloc();

/*
 * Internally used routines:
 */
static Bucket	bucket_allocate();
static void	bucket_deallocate();
static int	*data_allocate();
static void	data_deallocate();
static void	free_memory();
extern bool_t	nse_hash_access();
static void	nse_hash_bucket_deallocate();
static Bucket	nse_hash_bucket_first();
extern int	nse_hash_check();
extern Hash	nse_hash_create();
extern void	nse_hash_destroy();
extern bool_t	nse_hash_full_insert();
extern bool_t	nse_hash_full_lookup();
extern bool_t	nse_hash_full_replace();
extern int	nse_hash_get();
extern int	nse_hash_histogram();
extern void	nse_hash_histogram_display();
static void	nse_hash_initialize();
extern bool_t	nse_hash_insert();
static bool_t	nse_hash_int_equal();
static int	nse_hash_int_hash();
static int	nse_hash_int_insert();
extern void	nse_hash_read();
extern int	nse_hash_lookup();
static void	nse_hash_rehash2();
extern bool_t	nse_hash_replace();
static void	nse_hash_resize();
extern int	nse_hash_scan();
extern void	nse_hash_show();
extern int	nse_hash_size();
extern void	nse_hash_write();
static char	*memory_allocate();
static void	memory_deallocate();
static void	read_magic();
static int	read_word();
static void	write_magic();
static void	write_word();

/* Bucket allocator: */
static	Bucket	free_buckets;		/* Free bucket list */

/*
 * bucket_allocate() will allocate and return a new bucket symbol
 */
static Bucket
bucket_allocate()
{
	Bucket		bucket;		/* Temporary bucket */
	static Bucket	buckets;	/* Current set of bucket objects */
	static int	buckets_per_page; /* Number of buckets per page */
	static int	page_size = 0;	/* System page size */
	static int	remaining;	/* Number of remaining buckets */
	static int	valloc_size;	/* Amount of memory to valloc */

	if (page_size == 0){
		page_size = getpagesize();
		buckets_per_page = page_size / sizeof(*buckets);
		valloc_size = buckets_per_page * sizeof(*buckets);
		remaining = 0;
	}
	if (free_buckets != NULL){
		bucket = free_buckets;
		free_buckets = *(Bucket *)bucket;
		return bucket;
	}
	if (remaining == 0){
		buckets = (Bucket)valloc((unsigned)valloc_size);
		bzero((char *)buckets, valloc_size);
		remaining = buckets_per_page;
	}
	remaining--;
	return buckets++;
}

/*
 * bucket_deallocate(bucket) will deallocate a bucket.
 */
static void
bucket_deallocate(bucket)
	Bucket	bucket;		/* Bucket to deallocate */
{
	*(Bucket *)bucket = free_buckets;
	free_buckets = bucket;
}

/*
 * data_allocate(size, value) will get Size words of data and initialize them
 * to Value.
 */
static int *
data_allocate(size, value)
	register int	size;		/* Number of words to allocate */
	register int	value;		/* Value to initialize to */
{
	register int	*data;		/* Data array */
	register int	*pointer;	/* Pointer into data */

	pointer =
	    (int *)memory_allocate(size * sizeof(int));
	data = pointer;
	while (size-- > 0)
		*pointer++ = value;
	return data;
}

/*
 * data_deallocate(data, size) will deallocate Size words of Data.
 */
static void
data_deallocate(data, size)
	int		*data;		/* Data to free */
	int		size;		/* Number of words to free */
{
	memory_deallocate((char *)data, size * sizeof(int));
}

/*
 * nse_hash_access(hash, key, value, create, replace, value_destroy, 
 * key_pointer, * value_pointer) =>{True, False} is used to access the a 
 * key-value pair in
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
bool_t
nse_hash_access(hash, key, value, create, replace, value_destroy,
    key_pointer, value_pointer)
	register Hash	hash;		/* Hash table */
	int		key;		/* Key */
	int		value;		/* Value */
	bool_t		create;		/* True => Create new entry */
	bool_t		replace;	/* True => Replace value */
	Void_routine	value_destroy;	/* Value destroy routine */
	int		*key_pointer;	/* Place to store key */
	int		*value_pointer;	/* Place to store value */
{
	register Bucket	bucket;		/* Bucket to use */
	Bucket		*buckets;	/* Pointer into bucket array */
	register int	index;		/* Hash index */
	register Bool_routine key_equal;/* Key equality test routine */
	register int	mask;		/* Hash index mask */
	register bool_t return_value;	/* Return value */

	/* Fetch the starting bucket. */
	index = hash->key_hash(key);
	mask = hash->mask;
	buckets = &hash->buckets[index & mask];
	bucket = *buckets;

	/* Rehash, if necessary. 
	if (bucket == REHASH)
		bucket = nse_hash_rehash(hash, index); */

	/* Scan the hash table looking for Key. */
	key_equal = hash->key_equal;
	while (bucket != NULL){
		if ((bucket->index == index) && key_equal(bucket->key, key))
			break;
		bucket = bucket->next;
	}

	if (bucket == NULL){
		/* Create a new entry in Hash. */
		if (create){
			bucket = bucket_allocate();
			bucket->index = index;
			bucket->key = hash->key_insert(key);
			bucket->value = hash->value_insert(value);
			bucket->next = *buckets;
			*buckets = bucket;
			if (hash->count++ > hash->limit)
				nse_hash_resize(hash);
		}
		return_value = False;
	} else {
		/* Replace the value in Hash. */
		if (replace){
			if (value_destroy != NULL)
				value_destroy(bucket->value);
			bucket->value = hash->value_insert(value);
		}
		return_value = True;
	}

	/* Return everything. */
	if (key_pointer != NULL)
		*key_pointer =
		    (bucket == NULL) ? hash->key_empty : bucket->key;
	if (value_pointer != NULL)
		*value_pointer =
		    (bucket == NULL) ? hash->value_empty : bucket->value;
	return return_value;
}

#ifdef UNUSED
/*
 * nse_hash_bucket_deallocate(hash, bucket) will deallocate the Bucket for Hash.
 */
/* ARGSUSED */
static void
nse_hash_bucket_deallocate(hash, bucket)
	Hash		hash;		/* Hash table */
	Bucket		bucket;		/* Bucket to deallocate */
{
}

/*
 * nse_hash_bucket_first(hash, index) will fetch the first bucket from Hash
 * correspondint to Index.  This routine performs any rehashing that needs
 * to be done.
 */
static Bucket
nse_hash_bucket_first(hash, index)
	register Hash	hash;		/* Hash table */
	register int	index;		/* Index into hash slots */
{
	register Bucket *buckets;	/* Bucket array */

	index &= hash->mask;
	buckets = hash->buckets;
	return buckets[index];
}

/*
 * nse_hash_bucket_insert(hash, key, create) will attempt to find Key in Hash.
 * If Key is in Hash, the associated bucket will be returned.  If Key is
 * not in Hash and Create is True, a new bucket will be allocated and inserted
 * into Hash.  Otherwise, NULL will be returned.
 */
static Bucket *
nse_hash_bucket_find(hash, key, create)
	register Hash	hash;		/* Hash table */
	register int	key;		/* Key to lookup */
	bool_t		create;		/* True => create new bucket */
{
	register Bucket	bucket;		/* Current bucket */
	register int	index;		/* Hash index */
	register Bool_routine key_equal;/* Key equality routine */

	/* Go searching for the bucket */
/*	index = hash->key_hash(key);
	key_equal = hash->key_equal;
	bucket = nse_hash_bucket_first(hash, index);
	while ((unsigned)bucket > SPECIAL){
		if ((index == bucket->index) && key_equal(key, bucket->key))
			return bucket;
		bucket = bucket->next;
	}
	if (create){
		bucket = nse_hash_bucket_allocate(hash);
		bucket->index = index;
		bucket->key = hash->key_empty;
		bucket->value = hash->value_empty;
		index &= hash->mask;
		bucket->next = hash->buckets[index];
		hash->buckets[index] = bucket;
	}
	return bucket;
*/
}

/*
 * nse_hash_bucket_lookup(hash, key) will return the bucket assocated with Key
 * from Hash.  If Key is not in Hash, NULL will be returned.
 */
static Bucket
nse_hash_bucket_lookup(hash, key)
	register Hash	hash;		/* Hash table */
	register int	key;		/* Key to lookup */
{
	register Bucket	bucket;		/* Current bucket */
	register int	index;		/* Hash index */
	register Bool_routine key_equal;/* Key equality routine */

	index = hash->key_hash(key);
	bucket = nse_hash_bucket_first(hash, index);
	key_equal = hash->key_equal;
	while ((unsigned)bucket > SPECIAL){
		if ((index == bucket->index) && key_equal(key, bucket->key))
			return bucket;
		bucket = bucket->next;
	}
	return NULL;
}
#endif

/*
 * nse_hash_check(hash) will check hash for consistency.
 */
int
nse_hash_check(hash)
	Hash		hash;		/* Hash table to check */
{	
	register Bucket	bucket;		/* Current bucket */
	register Bucket *buckets;	/* Bucket array */
	register int	index;		/* Loop index */
	register int	slots;		/* Slots in bucket array */

	buckets = hash->buckets;
	slots = hash->slots;
	for (index = 0; index < slots; index++)
		for (bucket = buckets[index];
		    (unsigned)bucket > SPECIAL; bucket = bucket->next)
			if ((unsigned)bucket >= 0x1000000){
				fprintf(stderr, "bkt:%x index:%d\n",
							bucket, index);
				fflush(stderr);
				abort();
			}
}

/*
 * nse_hash_create(Size, Key_Empty, Key_Equal, Key_Hash, Key_Insert, 
 * Value_Empty, Value_Insert, 7) will create and return a hash table 
 * using the parameters.
 * Due to the large  number of arguments, the last argument must be the number
 * 7 so that a quick check can be made to make sure that they are all there.
 * All of the arguments except the last one can be NULL'ed out.
 */
Hash
nse_hash_create(size, key_empty, key_equal, key_hash, key_insert,
				value_empty, value_insert, check)
	register int	size;		/* Number of initial slots */
	int		key_empty;	/* Empty key value */
	Bool_routine	key_equal;	/* Key equality routine */
	Int_routine	key_hash;	/* Key hash function */
	Int_routine	key_insert;	/* Key insertion routine */
	int		value_empty;	/* Empty value */
	Int_routine	value_insert;	/* Value insertion routine */
	int		check;		/* Argument count check */
{
	register Hash	hash;		/* New hash table */
	register int	slots;		/* Number of slots */

	if (check != 7){
		fprintf(stderr, "HASH_CREATE: Wrong number of arguments\n");
		exit(1);
	}

	/* Number of slots must be a power of two. */
	slots = 1;
	while (slots < size)
		slots <<= 1;

	hash = (Hash)memory_allocate(sizeof *hash);
	hash->count = 0;	
	hash->key_empty = key_empty;
	hash->key_equal = (key_equal != NULL) ? key_equal : nse_hash_int_equal;
	hash->key_hash = (key_hash != NULL) ? key_hash : nse_hash_int_hash;
	hash->key_insert =
		(key_insert != NULL) ? key_insert : nse_hash_int_insert;
	hash->value_empty = value_empty;
	hash->value_insert =
		(value_insert != NULL) ? value_insert : nse_hash_int_insert;
	nse_hash_initialize(hash, slots);
	return hash;
}

/*
 * nse_hash_destroy(hash, key_destroy, value_destroy) will deallocate the 
 * storage
 * associated with Hash.  Each key-value pair in Hash will be destroyed by
 * calling Key_Destroy(Key) and Value_Destroy(Value).  If Key_Destroy is NULL,
 * it will not be called.  Likewise, if Value_Destroy is NULL, it will not
 * be called.
 */
void
nse_hash_destroy(hash, key_destroy, value_destroy)
	register Hash		hash;		/* Hash table to destroy */
	register Void_routine	key_destroy;	/* Key destroy routine */
	register Void_routine	value_destroy;	/* Value destroy routine */
{
	register Bucket	bucket;			/* Current bucket */
	register Bucket	*bucket_pointer;	/* Pointer into bucket array */
	register int	slots;			/* Number of slots */

	bucket_pointer = hash->buckets;
	for (slots = hash->slots; slots > 0; slots--){
		bucket = *bucket_pointer++;
		while ((unsigned)bucket > SPECIAL){
			if (key_destroy != NULL)
				key_destroy(bucket->key);
			if (value_destroy != NULL)
				value_destroy(bucket->value);
			bucket_deallocate(bucket);
			bucket = bucket->next;
		}
	}
	data_deallocate((int *)hash->buckets, hash->slots);
	memory_deallocate((char *)hash, sizeof *hash);
}

/*
 * nse_hash_full_insert(hash, key, value, key_pointer, value_pointer)
 * =>{True, False} will insert Key-Value into Hash.  If Key is already in
 * Hash, the key and value already in Hash will not be affected and True will
 * be returned.  If Key is not already in Hash, Key and Value are inserted
 * into Hash and False is returned.  If Key_Pointer is non-NULL, *Key_Pointer
 * will be assigned the key stored in Hash.  If Value_pointer is non-NULL,
 * *Value_Pointer will be assigend the value stored in Hash.
 */
/* VARARGS1 */
bool_t
nse_hash_full_insert(hash, key, value, key_pointer, value_pointer)
	register Hash	hash;		/* Hash table */
	int		key;		/* Key to lookup */
	int		value;		/* Value to store */
	int		*key_pointer;	/* Place to store key */
	int		*value_pointer;	/* Place to store value */
{
	return nse_hash_access(hash, key, value, True, False, 
	    (Void_routine)NULL, key_pointer, value_pointer);
}

/*
 * nse_hash_full_lookup(hash, key, key_pointer, value_pointer)=>{True, False} 
 * will find the key-value pair associated with Key in Hash.  If Key is in 
 * Hash, the
 * key stored with Hash will be returned in *Key_Pointer, the value stored with
 * Hash will be returned in *Value_Pointer, and True will be returned.  If Key
 * is not in Hash, the empty key will be stored into *Key_Pointer, the empty
 * value will be stored into *Value_Pointer.  If Key_Pointer is NULL, no key
 * will be stored into it.  If Value_Pointer is NULL, no value will be stored
 * into it.
 */
/* VARARGS1 */
bool_t
nse_hash_full_lookup(hash, key, key_pointer, value_pointer)
	register Hash	hash;		/* Hash table */
	int		key;		/* Key to lookup */
	int		*key_pointer;	/* Place to store key */
	int		*value_pointer;	/* Place to store value */
{
	return nse_hash_access(hash, key, 0, False, False, (Void_routine)NULL,
	    key_pointer, value_pointer);
}

/*
 * nse_hash_full_replace(hash, key, value, key_pointer, value_pointer)
 * =>{True, False} will insert Key-Value into Hash.  If Key is already in
 * Hash, the key and value will just be replaced and True will be returned.
 *  If Key is not already in Hash, Key and Value are inserted into Hash and
 * False is returned.  If Key_Pointer is non-NULL, *Key_Pointer will be
 * assigned the key stored in Hash.  If Value_pointer is non-NULL,
 * *Value_Pointer will be assigend the value stored in Hash.
 */
/* VARARGS1 */
bool_t
nse_hash_full_replace(hash, key, value, key_pointer, value_pointer)
	register Hash	hash;		/* Hash table */
	int		key;		/* Key to lookup */
	int		value;		/* Value to store */
	int		*key_pointer;	/* Place to store key */
	int		*value_pointer;	/* Place to store value */
{
	return nse_hash_access(hash, key, value, True, True, (Void_routine)NULL,
	    key_pointer, value_pointer);
}

/*
 * nse_hash_get(Hash, Key)=>Value will lookup the value for Key in Hash.  If Key
 * is not in Hash, a fatal error occurs.
 */
/* VARARGS1 */
int
nse_hash_get(hash, key)
	register Hash	hash;	/* Hash table */
	int		key;	/* Key to lookup */
{
	int		value;	/* Return value */

	if (nse_hash_access(hash,
	    key, 0, False, False, (Void_routine)NULL, (int *)NULL, &value))
		return value;
	else {
		fprintf(stderr,
			"HASH_GET:Could not find key (0x%x) in table (0x%x)\n",
			key, hash);
		exit(1);
		/* NOTREACHED */
	}
}

#ifdef UNUSED
/*
 * nse_hash_histogram(hash, histogram, size) will scan through Hash generating
 * a histogram of the bucket length and storing the result into the Size
 * words starting at Histogram.  The maximum desired histogram value will
 * be returned.
 */
int
nse_hash_histogram(hash, histogram, size)
	Hash		hash;		/* Hash table */
	register int	*histogram;	/* Place to store histogram */
	register int	size;		/* Maximum size of histogram */
{
	register Bucket	bucket;		/* Current bucket */
	register Bucket *buckets;	/* Bucket array */
	register int	count;		/* Number of buckets */
	register int	maximum;	/* Maximum histogram value */
	register int	slots;		/* Slots in bucket array */

	bzero((char *)histogram, size * sizeof(int));
	buckets = hash->buckets;
	maximum = 0;
	for (slots = hash->slots; slots > 0; slots--){
		count = 0;
		for (bucket = *buckets++;
		    (unsigned)bucket > SPECIAL; bucket = bucket->next)
			count++;
		if (count < size)
			histogram[count]++;
		if (count > maximum)
			maximum = count;
	}
	return maximum;
}

/*
 * nse_hash_histogram_display(hash, out_file) will print a histogram of Hash to
 * Out_File.
 */
void
nse_hash_histogram_display(hash, out_file)
	Hash		hash;		/* Hash table */
	register FILE	*out_file;	/* Output file */
{
	register int	*histogram;	/* Histogram */
	register int	index;		/* Index into histogram */
	register int	size;		/* Histogram size */

	size = nse_hash_histogram(hash, (int *)NULL, 0) + 1;
	histogram = (int *)memory_allocate(size * sizeof(int));
	nse_hash_histogram(hash, histogram, size);
	fprintf(out_file, "Length\tCount\n");
	for (index = 0; index < size; index++)
		fprintf(out_file, "[%d]:\t%d\n", index, histogram[index]);
	memory_deallocate((char *)histogram, size * sizeof(int));
}
#endif

/*
 * nse_hash_insert(hash, key, value)=>{True,False} will attempt to find Key in
 * Hash.  If Key is already in Hash, True will be returned without affecting
 * Key's associated value.  Otherwise, Key and Value will be inserted into
 * Hash and False will be returned.
 */
/* VARARGS1 */
bool_t
nse_hash_insert(hash, key, value)
	register Hash	hash;	/* Hash table */
	int		key;	/* Key to insert under */
	int		value;	/* Value to insert */
{
	return nse_hash_access(hash, key, value, True, False, 
	    (Void_routine)NULL, (int *)NULL, (int *)NULL);
}

/*
 * nse_hash_initialize(Hash, Size) will initialize Hash to contain Size table
 * entries.
 */
static void
nse_hash_initialize(hash, size)
	register Hash	hash;		/* Hash table */
	register int	size;		/* Number of slots */
{
	hash->buckets = (Bucket *)data_allocate(size, (int)EMPTY);
	hash->slots = size;
	hash->limit = (size << 3) / 10;
	hash->mask = size - 1;
}

/*
 * nse_hash_int_equal(Int1, Int2) will return True if Int1 equals Int2.
 */
static bool_t
nse_hash_int_equal(int1, int2)
	int	int1;		/* First integer */
	int	int2;		/* Second integer */
{
	return (bool_t)(int1 == int2);
}

/*
 * nse_hash_int_hash(number) will return a hash on number.
 */
static int
nse_hash_int_hash(number)
	int	number;		/* Number to hash */
{
	return number;
}

/*
 * nse_hash_int_insert(number) will return a copy of number.
 */

static int
nse_hash_int_insert(number)
	int	number;		/* Number to insert */
{
	return number;
}

/*
 * nse_hash_lookup(Hash, Key)=>value will lookup Key in Hash.  If Key is not
 * in Hash, Empty_Value will be returned.
 */
/* VARARGS1 */
int
nse_hash_lookup(hash, key)
	register Hash	hash;	/* Hash table */
	int		key;	/* Key to lookup */
{
	int		value;	/* Value with Key */

	nse_hash_access(hash, key, 0, False, False, (Void_routine)NULL,
	    (int *)NULL, &value);
	return value;
}

#ifdef UNUSED
/*
 * nse_hash_read(hash, in_file, key_read, key_handle, value_read, value_handle)
 * will read a hash table from In_File into Hash.  Hash must be an empty
 * that was created in the same what that it was originally written out.
 * Key_Read(In_File, Key_Handle)=>Key to read a key from In_File and
 * Value_Read(In_File, Value_Handle, Key)=>Value to read a value from In_File.
 * The Key that was just read in is passed as the third argument to Value_Read.
 * If Key_Read or Value_Read are NULL, integer read routines will be used.
 */
void
nse_hash_read(hash, in_file, key_read, key_handle, value_read, value_handle)
	register Hash	hash;		/* Hash table to use */
	register FILE	*in_file;	/* File to input from */
	Int_routine	key_read;	/* Key read routine */
	int		key_handle;	/* Key handle */
	Int_routine	value_read;	/* Value read routine */
	int		value_handle;	/* Value handle */
{
	register Bucket	bucket;		/* Current bucket */
	register Bucket	*buckets;	/* Buckets array pointer */
	register int	count;		/* Loop counter */
	register int	key;		/* Key */
	register int	length;		/* Bucket length */

	if (hash->count != 0){
		fprintf(stderr, "hash_read: reading into non-empty table\n");
		exit(1);
	}
	data_deallocate((int *)hash->buckets, hash->slots);

	read_magic(in_file, HASH_MAGIC);
	hash->count = read_word(in_file);
	hash->limit = read_word(in_file);
	hash->mask = read_word(in_file);
	hash->slots = read_word(in_file);
	hash->buckets = (Bucket *)data_allocate(hash->slots, (int)EMPTY);

	if (key_read == NULL)
		key_read = read_word;
	if (value_read == NULL)
		value_read = read_word;
	buckets = hash->buckets;
	count = hash->slots;
	while (count-- > 0){
		length = read_word(in_file);
		bucket = NULL;
		while (length-- > 0){
			if (bucket == NULL){
				bucket = bucket_allocate();
				*buckets = bucket;
			} else {
				bucket->next = bucket_allocate();
				bucket = bucket->next;
			}
			bucket->index = read_word(in_file);
			key = key_read(in_file, key_handle);
			bucket->key = key;
			bucket->value = value_read(in_file, value_handle, key);
		}
		buckets++;
	}
	read_magic(in_file, HASH_MAGIC);
}
#endif


#ifdef UNUSED
/*
 * nse_hash_rehash(hash, index) will rehash all of the hash table entires that
 * might conflict with Index in Hash.
 */
static Bucket
nse_hash_rehash(hash, index)
	register Hash	hash;		/* Hash table to use */
	register int	index;		/* Hash index to work on */
{
	register Bucket	bucket;		/* Current bucket */
	register int	count;		/* Number of buckets to check */
	register Bucket	*pointer;	/* Current bucket pointer */
	Bucket		rehash[32];	/* Number of buckets to rehash */

	pointer = rehash;
	count = nse_hash_rehash1(hash, index, pointer);
	while (count-- > 0){
		bucket = *pointer++;
		if ((unsigned)bucket > SPECIAL)
			nse_hash_rehash2(hash, bucket);
	}
	return hash->buckets[index & hash->mask];
}

/*
 * nse_hash_rehash1(hash, index, rehash) will scan through the bucket array in
 * Hash and store the buckets than need to be rehashed for Index into Rehash.
 * The number of items to be rehashed is returned.
 */
static int
nse_hash_rehash1(hash, index, rehash)
	Hash		hash;		/* Hash table to use */
	register int	index;		/* Hash index to rehash */
	register Bucket *rehash;	/* Place to store results */
{
	register int	count;		/* Number of buckets to check */
	register Bucket *buckets;	/* Bucket array */
	register Bucket	*pointer;	/* Current bucket pointer */
	register Bucket *last_pointer;	/* Last value of Pointer */
	register int	mask;		/* Current hash mask */

	buckets = hash->buckets;
	count = 0;
	mask = hash->mask;
	last_pointer = NULL;
	while (mask != 0){
		pointer = buckets + (index & mask);
		if (pointer != last_pointer){
			last_pointer = pointer;
			rehash[count++] = *pointer;
			*pointer = NULL;
		}
		mask >>= 1;
	}
	return count;
}
#endif

/*
 * nse_hash_rehash2(hash, bucket) will rehash all of the buckets pointed to by
 * Bucket in Hash.
 */
static void
nse_hash_rehash2(hash, bucket)
	Hash		hash;		/* Hash table to use */
	register Bucket	bucket;		/* Bucket list to rehash */
{
	register Bucket *bucket_pointer;/* Ponter into bucket array */
	register Bucket	*buckets;	/* Bucket array */
	register int	mask;		/* Hash index mask */
	register Bucket	next_bucket;	/* Next bucket to rehash */

	buckets = hash->buckets;
	mask = hash->mask;
	while (bucket != NULL){
		next_bucket = bucket->next;
		bucket_pointer = buckets + (bucket->index & mask);
		bucket->next = *bucket_pointer;
		*bucket_pointer = bucket;
		bucket = next_bucket;
	}	
}

/*
 * nse_hash_replace(hash, key, value)=>{True,False} will attempt to find Key in
 * Hash.  If Key is not in Hash, Key and Value will be inserted into Hash
 * and False will be returned.  Otherwise, Value will replace the Key value
 * already in Hash, and True will be returned.
 */
/* VARARGS1 */
bool_t
nse_hash_replace(hash, key, value)
	register Hash	hash;	/* Hash table */
	int		key;	/* Key to insert under */
	int		value;	/* Value to insert */
{
	return nse_hash_access(hash, key, value, True, True, (Void_routine)NULL,
	    (int *)NULL, (int *)NULL);
}

/*
 * nse_hash_resize(Hash) will increase the number of slots in the hash table.
 */
static void
nse_hash_resize(hash)
	register Hash	hash;		/* Hash table to resize. */
{
	register int	count;		/* Loop counter */
	Bucket		*new_buckets;	/* New bucket array */
	register Bucket	*new_pointer;	/* Pointer into new bucket array */
	register int	new_slots;	/* New number of slots */
	Bucket		*old_buckets;	/* Old bucket array */
	register Bucket	*old_pointer;	/* Pointer into old buckedt array */
	register int	old_slots;	/* Old number of slots */

	/* Set up the new bucket array. */
	old_slots = hash->slots;
	old_buckets = hash->buckets;
	new_slots = old_slots << 1;
	new_buckets = (Bucket *)data_allocate(new_slots, (int)EMPTY);
	hash->slots = new_slots;
	hash->buckets = new_buckets;
	hash->limit <<= 1;
	hash->mask = new_slots - 1;

	/* Transfer the buckets from old to new. */
	old_pointer = old_buckets;
	new_pointer = new_buckets;
	for (count = new_slots; count > 0; count--)
		*new_pointer++ = EMPTY;
	for (count = old_slots; count > 0; count--)
		nse_hash_rehash2(hash, *old_pointer++);

	/* Dealloacte the old bucket array. */
	data_deallocate((int *)old_buckets, old_slots);
}

/*
 * nse_hash_scan(hash, routine, data) will scan the entire contents of Hash
 * calling Routine(Key, Value, Data)=>int for each key-value pair in Hash.
 * The sum of the values returned by Routine will be returned.
 */
/* VARARGS1 */
int
nse_hash_scan(hash, routine, data)
	register Hash		hash;		/* Hash table */
	register Int_routine	routine;	/* Routine to scan with */
	register int		data;		/* Data value */
{
	register Bucket		bucket;		/* Current bucket */
	register Bucket 	*buckets;	/* Bucket array */
	register int		slots;		/* Slots in bucket array */
	register int		sum;		/* Sum from routine */

	buckets = hash->buckets;
	sum = 0;
	for (slots = hash->slots; slots > 0; slots--)
		for (bucket = *buckets++;
		    (unsigned)bucket > SPECIAL; bucket = bucket->next)
			sum += routine(bucket->key, bucket->value, data);
	return sum;
}

/*
 * nse_hash_show(hash) will show the contents of Hash on the console.  This
 * routine is used for testing and debugging purposes only.
 */
void
nse_hash_show(hash)
	register Hash	hash;		/* Hash table */
{
	register Bucket	bucket;		/* Current bucket */
	register Bucket *buckets;	/* Bucket array */
	register int	slot;		/* Current slot in bucket array */
	register int	slots;		/* Slots in bucket array */

	printf("Hash:0x%x Count:%d Limit:%d Slots:%d\n",
			hash, hash->count, hash->limit, hash->slots);
	printf("Key_Empty:%d Key_Equal:0x%x Key_Hash:0x%x Key_Insert:0x%x\n",
		hash->key_empty, hash->key_equal,
		hash->key_hash, hash->key_insert);
	printf("Value_Empty:%d Value_Insert:%d Buckets:0x%x\n",
		hash->value_empty, hash->value_insert, hash->buckets);

	buckets = hash->buckets;
	slots = hash->slots;
	for (slot = 0; slot < slots; slot++){
		printf("[%d]", slot);
		bucket = buckets[slot];
		if (bucket == EMPTY)
			printf("\tEmpty\n");
		else if (bucket == REHASH)
			printf("\tRehash\n");
		else {	while (bucket != NULL){
				printf("\tBucket:0x%x Index:0x%x",
					bucket, bucket->index);
				printf(" Key:0x%x Value:0x%x\n",
					bucket->key, bucket->value);
				bucket = bucket->next;
			}
		}
	}
}

/*
 * nse_hash_size(hash) returns the number of entries in Hash.
 */
int
nse_hash_size(hash)
	Hash hash;		/* Hash table to use */
{
	return hash->count;
}

#ifdef UNUSED
/*
 * nse_hash_write(hash, out_file, key_write, key_handle, value_write, value_handle)
 * will write out Hash to Out_File by calling
 * Key_Write(Out_File, Key, Key_Handle) to write Key to Out_File
 * Value_Write(Out_File, Value, Value_Handle) to write Valut to Out_File.
 * If Key_Write or Value_Write are NULL, integer write routines will be used.
 */
void
nse_hash_write(hash, out_file, key_write, key_handle, value_write, value_handle)
	register Hash	hash;		/* Hash table to use */
	register FILE	*out_file;	/* File to output to */
	Void_routine	key_write;	/* Key write routine */
	int		key_handle;	/* Key handle */
	Void_routine	value_write;	/* Value write routine */
	int		value_handle;	/* Value handle */
{
	register Bucket	bucket;		/* Current bucket */
	register Bucket	*buckets;	/* Buckets array pointer */
	register int	count;		/* Loop counter */
	register int	length;		/* Bucket length */

	write_magic(out_file, HASH_MAGIC);
	write_word(out_file, hash->count);
	write_word(out_file, hash->limit);
	write_word(out_file, hash->mask);
	write_word(out_file, hash->slots);

	if (key_write == NULL)
		key_write = write_word;
	if (value_write == NULL)
		value_write = write_word;
	buckets = hash->buckets;
	count = hash->slots;
	while (count-- > 0){
		length = 0;
		for (bucket = *buckets; bucket != NULL; bucket = bucket->next)
			length++;
		write_word(out_file, length);
		for (bucket = *buckets++; bucket != NULL;
					 bucket = bucket->next){
			write_word(out_file, bucket->index);
			key_write(out_file, bucket->key, key_handle);
			value_write(out_file, bucket->value, value_handle);
		}
	}
	write_magic(out_file, HASH_MAGIC);
}
#endif

/*
 * memory_allocate(size) will allocate Size bytes of memory.  If insufficient
 * memory is available, a fatal error will occur.
 */
static char *
memory_allocate(size)
	int		size;		/* Number of bytes to allocate */
{
	register char	*data;		/* Newly allocated data */

	data = malloc((unsigned)size);
	if (data != NULL)
		return data;
	else {
		fprintf(stderr,
			"Could not allcoate %d bytes (Out of swap space?)\n",
			size);
		exit(1);
		/* NOTREACHED */
	}
}

/*
 * memory_deallocate(memory, size) will free the Size bytes of Memory.
 */
/* ARGSUSED */
static void
memory_deallocate(memory, size)
	char		*memory;	/* Data to free */
	int		size;		/* Number of bytes to free */
{
	free(memory);
}

#ifdef UNUSED
/*
 * read_magic(in_file, magic) will make sure that the next word from In_File
 * is Magic.
 */
static void
read_magic(in_file, magic)
	FILE		*in_file;	/* Input file */
	int		magic;		/* Magic number */
{
	int		temp;		/* Temporary */

	temp = read_word(in_file);
	if (temp != magic){
		fprintf(stderr, "0x%x found instead of 0x%x\n", temp, magic);
		exit(1);
	}
}

/*
 * read_word(in_file) will read in a word from In_File.
 */
static int
read_word(in_file)
	register FILE	*in_file;	/* Input file */
{
	return getw(in_file);
}

/*
 * write_magic(out_file, magic) will write Magic to Out_File.
 */
static void
write_magic(out_file, magic)
	FILE		*out_file;	/* Output file */
	int		magic;		/* Magic number */
{
	write_word(out_file, magic);
}

/*
 * write_word(out_file, word) will write Word to Out_file.
 */
static void
write_word(out_file, word)
	register FILE	*out_file;	/* Output file */
	int		word;		/* Word to output */
{
	putw(word, out_file);
}
#endif

/*
 * Generic routine for printing out stats about a hash table.
 * Gives total number of elements, the number of bytes occupied by
 * the keys and values, the number of bytes private to the data structure
 * for the keys and values (strings for instances), and the amount of 
 * hash overhead.
 */
void
nse_hash_print_stats(fp, hash, keysize, valsize, func, str)
	FILE		*fp;
	Hash		hash;
	int		keysize;
	int		valsize;
	Nse_intfunc	func;
	char		*str;
{
	int		n;
	int		data_bytes;
	int		private_bytes;
	int		hash_bytes;

	if (hash == NULL) {
		return;
	}
	n = hash->count;
	fprintf(fp, "\n");
	fprintf(fp, "%s\n", str);
	fprintf(fp, "\t%-20s %6d\n", "# elements in hash", n);
	fprintf(fp, "\t%-20s %6d\n", "hash table size", hash->slots);
	data_bytes = n * (keysize + valsize);
	fprintf(fp, "\t%-20s %6d\n", "bytes in hash data", data_bytes);
	if (func != NULL) {
		private_bytes = nse_hash_scan(hash, func, NULL);
	} else {
		private_bytes = 0;
	}
	fprintf(fp, "\t%-20s %6d\n", "data-specific bytes", private_bytes);
	hash_bytes = sizeof(struct _hash) +
		hash->slots * sizeof(Bucket) +
		n * sizeof(struct _bucket);
	fprintf(fp, "\t%-20s %6d\n", "hash overhead", hash_bytes);
	fprintf(fp, "\t%-20s %6d\n", "total bytes", 
	    data_bytes + private_bytes + hash_bytes);
}
