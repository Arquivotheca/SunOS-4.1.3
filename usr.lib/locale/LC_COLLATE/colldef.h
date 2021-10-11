/*
 * colldef.h
 */

/* static  char *sccsid = "@(#)colldef.h 1.1 92/07/30 SMI"; */

#include <stdio.h>
#ifdef SUNOS4.1
#include <locale.h>
#endif
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>

/*
 * misc defines
 */
#define PROG	"colldef"

#define IDSIZE 	128
#define BSIZE  	256
#define OFF	0
#define ON	1
#define DONT_CARE_P 1
#define DONT_CARE_S 0

#define OK	0
#define UNDEF	0
#define ERROR	-1

/*
 * interface to strcoll(), etc
 */
#ifndef SUNOS4.1
#define MAXSUBS 64
#endif
#define NUM_1_TO_MANY	MAXSUBS
#define NUM_2_TO_ONE 	256
#define MAXSTR		256
#define TABSIZE		256
#define TRUE		1
#define FALSE		0

/*
 * dump this table first
 */
struct colldef {
	unsigned char	primary_sort[256];
	unsigned char	secondary_sort[256];
};


/*
 * second
 */
struct _1_to_m {
	char one;
	char *many;
};

/*
 * third
 */
struct _2_to_1 {
	char one;
	char two;
	char mapped_primary;
	char mapped_secondary;
};

/*
 * charmap structure
 */
struct charmap {
	char *mapping;		/* character to be mapped */
	unsigned int map_val;	/* mapped value */
	struct charmap *next;
};

/* 
 * Type holder 
 */ 
struct type_var { 
	int type;       /* string or number */ 
	union { 
		int num_val; 
		char *str_val;   
	} type_val; 
}; 

/*
 * Macro defines
 */
#define ME_HEX(_P) (_P == 'A' || _P == 'B' || _P == 'C' || _P == 'D' || \
		    _P == 'E' || _P == 'F' || \
                    _P == 'a' || _P == 'b' || _P == 'c' || _P == 'd' || \
		    _P == 'e' || _P == 'f')
#define TYPE_VARmalloc() (struct type_var *) malloc(sizeof(struct type_var))
#define CHARMAPmalloc()	(struct charmap *)malloc(sizeof(struct charmap))

extern char *strsave();
extern struct charmap * lookup_map();
