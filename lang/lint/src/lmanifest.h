/*	@(#)lmanifest.h 1.1 92/07/30 SMI; from S5R2 1.4	*/

/*	the key:
	LDI	defined and initialized: storage set aside
	LIB	defined on a library
	LDC	defined as a common region on UNIX
	LDX	defined by an extern: if ! pflag, same as LDI
	LRV	function returns a value
	LUV	function used in a value context
	LUE	function used in effects context
	LUM	mentioned somewhere other than at the declaration
	LDS	defined static object (like LDI)
	*/
# define LDI 01
# define LIB 02
# define LDC 04
# define LDX 010
# define LRV 020
# define LUV 040
# define LUE 0100
# define LUM 0200
# define LDS 0400

# define LFN 01000  /* filename record */

typedef struct ty {
	TWORD aty;
	short extra;
	short extra1;
	} ATYPE;

#define X_NONAME 0x8000		/* for extra1, if structure has no name */

typedef struct line {
	short decflag;
	char *name;
	short nargs;
	short fline;
	ATYPE type;
	} LINE;

union rec {
	struct line l;
	struct {
		short decflag;
		char *fn;
		int mno;
		} f;
	};

#ifdef CXREF
extern FILE *outfp;
#endif

