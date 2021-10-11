/* @(#)rtld.h 1.1 92/07/30 SMI */

/*
 * Global definitions for run-time link editor.
 */

/*
 * General macros.
 */
#define PROUND(x) (((x) + PAGESIZE - 1) & ~(PAGESIZE - 1))
					/* round to page size */
#define	MAIN_BASE	PAGSIZ		/* place where "main" is mapped */
#define SROUND(x) ((x + SEGSIZ - 1) &~ (SEGSIZ - 1))
					/* round to segment size */
#define	SIZE(x) (SROUND((x).a_text) + (x).a_data + (x).a_bss)
					/* address space "claim" */
#define max(a,b) ((a) < (b) ? (b) : (a))

/*
 * Code collapsing macros.
 */
#define	LM2LP(lmp)	((struct ld_private *)lmp->lm_lpd)

/*
 * Define our own versions of ctype macros to avoid hauling in the
 * ctype_ array.
 */
#define isdigit(c)	(((c) >= '0') && ((c) <= '9'))
#define	isspace(c)	(((c) == ' ') || (((c) >= '\t') && ((c) <= '\015')))

/*
 * State descriptor for object accessed via "dlopen".
 */
#define	DL_MAGIC 0x580331		/* unlikely quantity */
#define	DL_CIGAM 0x830504		/* matching unlikely quantity */

struct dl_object {
	long	dl_magic;		/* DL_MAGIC */
	struct	dl_object *dl_next;	/* next dlopen object */
	struct	dl_object *dl_dep;	/* next dependent object */
	struct	link_map *dl_lmp;	/* point back to the link map */
	long	dl_refcnt;		/* reference count */
	long	dl_cigam;		/* DL_CIGAM */
};

/*
 * Loader link_map private data.  Generally a cache for relocated items
 * fetched from the object's link_dynamic structure.
 *
 * N.B. lp_lobase is a hack to overcome a problem in version #2 __DYNAMIC
 * programs.  For main programs, ld-generated data structures in the text
 * segment are relocated against "0" rather than the actual text base
 * address.  
 */
struct	ld_private {
	struct	jbind *lp_plt;		/* procedure linkage table */
	struct	relocation_info *lp_rp;	/* relocation table */
	struct	fshash *lp_hash;	/* hash table */
	struct	nlist *lp_symtab;	/* symbol table */
	char	*lp_symstr;		/* symbol strings */
	caddr_t	lp_textbase;		/* base address for text addressing */
	struct	nlist *(*lp_interp)();	/* link map interpreter */
	long	lp_refcnt;		/* reference count of link map */
	struct 	dl_object *lp_dlp;	/* pointer to a dlopen object */
	struct	dl_object *lp_dlh;	/* pointer to head of a dl chain */
	caddr_t	lp_symbol_base;		/* base address for symbols */
};

/*
 * macro off which to base text addressing -- use this to access
 * link_object lists and addresses and other things which are sensitive
 * to the "main program" bug.
 */
#define	TEXTBASE(lmp)	(LM2LP(lmp)->lp_textbase)

/*
 * General global declarations
 */
extern int stdout;			/* file descriptor for results */
extern int stderr;			/* file descriptor for errors */
extern int errno;			/* system call error value */

extern char **environ;			/* environment strings */

extern void getreuid();			/* system call jacket for r/euid */
extern void getregid();			/* ditto for r/egid */
extern char *rtmalloc();		/* heap allocation */
extern char *getenv();			/* access to environment variables */
extern caddr_t mmap();			/* memory mapping system call */
extern int secure_objects();		/* indicates setu/gid or not */
extern void panic();			/* fatal error handling */

/*
 * Error code (strings).
 */
extern	char DLE_none[];		/* no error */
extern	char DLE_mode_error[];		/* dlopen mode */
extern	char DLE_bad_handle[];		/* invalid dl object handle */
extern	char DLE_can_not_open[];	/* unable to map */
extern	char DLE_undefined[];		/* undefined symbol reference */
extern	char DLE_conflict[];		/* conflicting use of dependent .so */
