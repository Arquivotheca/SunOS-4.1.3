/*      @(#)a.out.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03       */

#ifndef _sun3_a_out_h
#define _sun3_a_out_h

#include <sys/exec.h>

/*
 * memory management parameters
 */

#define PAGSIZ		0x02000
#define SEGSIZ		0x20000
#define	OLD_PAGSIZ	0x00800	/*  Page   size under Release 2.0 */
#define	OLD_SEGSIZ	0x08000	/* Segment size under Release 2.0 */

/*
 * returns 1 if an object file type is invalid, i.e., if the other macros
 * defined below will not yield the correct offsets.  Note that a file may
 * have N_BADMAG(x) = 0 and may be fully linked, but still may not be
 * executable.
 */

#define	N_BADMAG(x) \
	((x).a_magic!=OMAGIC && (x).a_magic!=NMAGIC && (x).a_magic!=ZMAGIC)

/*
 * relocation parameters. These are architecture-dependent
 * and can be deduced from the machine type.  They are used
 * to calculate offsets of segments within the object file;
 * See N_TXTOFF(x), etc. below.
 */

#define	N_PAGSIZ(x) \
	((x).a_machtype == M_OLDSUN2? OLD_PAGSIZ : PAGSIZ)
#define	N_SEGSIZ(x) \
	((x).a_machtype == M_OLDSUN2? OLD_SEGSIZ : SEGSIZ)

/*
 * offsets of various sections of an object file.
 */

#define	N_TXTOFF(x) \
	/* text segment */ \
	( (x).a_machtype == M_OLDSUN2 \
	? ((x).a_magic==ZMAGIC ? N_PAGSIZ(x) : sizeof (struct exec)) \
	: ((x).a_magic==ZMAGIC ? 0 : sizeof (struct exec)) )

#define	N_DATOFF(x)   /* data segment */	\
	(N_TXTOFF(x) + (x).a_text)

#define	N_TRELOFF(x)  /* text reloc'n */	\
	(N_DATOFF(x) + (x).a_data)

#define	N_DRELOFF(x) /* data relocation*/	\
	(N_TRELOFF(x) + (x).a_trsize)

#define	N_SYMOFF(x) \
	/* symbol table */ \
	(N_TXTOFF(x)+(x).a_text+(x).a_data+(x).a_trsize+(x).a_drsize)

#define	N_STROFF(x) \
	/* string table */ \
	(N_SYMOFF(x) + (x).a_syms)

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */

#define	_N_BASEADDR(x) \
	(((x).a_magic == ZMAGIC) && ((x).a_entry < N_PAGSIZ(x)) ? \
	    0 : N_PAGSIZ(x))

#define N_TXTADDR(x) \
	((x).a_machtype == M_OLDSUN2? N_SEGSIZ(x) : _N_BASEADDR(x))

#define N_DATADDR(x) \
	(((x).a_magic==OMAGIC)? (N_TXTADDR(x)+(x).a_text) \
	: (N_SEGSIZ(x)+((N_TXTADDR(x)+(x).a_text-1) & ~(N_SEGSIZ(x)-1))))

#define N_BSSADDR(x)  (N_DATADDR(x)+(x).a_data)

/*
 * Format of a relocation datum.
 */

/*
 * Sparc relocation types
 */
 
enum reloc_type
{
	RELOC_8,	RELOC_16,	RELOC_32,	/* simplest relocs    */
	RELOC_DISP8,	RELOC_DISP16,	RELOC_DISP32,	/* Disp's (pc-rel)    */
	RELOC_WDISP30,	RELOC_WDISP22,			/* SR word disp's     */
	RELOC_HI22,	RELOC_22,			/* SR 22-bit relocs   */
	RELOC_13,	RELOC_LO10,			/* SR 13&10-bit relocs*/
	RELOC_SFA_BASE,	RELOC_SFA_OFF13,		/* SR S.F.A. relocs   */
	RELOC_BASE10,	RELOC_BASE13,	RELOC_BASE22,	/* base_relative pic */
	RELOC_PC10,	RELOC_PC22,			/* special pc-rel pic*/
	RELOC_JMP_TBL,					/* jmp_tbl_rel in pic */
	RELOC_SEGOFF16,					/* ShLib offset-in-seg*/
	RELOC_GLOB_DAT, RELOC_JMP_SLOT, RELOC_RELATIVE, /* rtld relocs        */
};

/*
 * Format of a relocation datum.
 */
 
struct reloc_info_68k {
	long	r_address;	/* address which is relocated */
unsigned int	r_symbolnum:24,	/* local symbol ordinal */
		r_pcrel:1, 	/* was relocated pc relative already */
		r_length:2,	/* 0=byte, 1=word, 2=long */
		r_extern:1,	/* does not include value of sym referenced */
		r_baserel:1,	/* linkage table relative */
		r_jmptable:1,	/* pc-relative to jump table */
		r_relative:1,	/* relative relocation */
		:1;		
};

#define relocation_info reloc_info_68k	/* for backward compatibility */



/*
 * Format of a symbol table entry
 */
struct	nlist {
	union {
		char	*n_name;	/* for use when in-core */
		long	n_strx;		/* index into file string table */
	} n_un;
	unsigned char	n_type;		/* type flag (N_TEXT,..)  */
	char	n_other;		/* unused */
	short	n_desc;			/* see <stab.h> */
	unsigned long	n_value;	/* value of symbol (or sdb offset) */
};

/*
 * Simple values for n_type.
 */
#define	N_UNDF	0x0		/* undefined */
#define	N_ABS	0x2		/* absolute */
#define	N_TEXT	0x4		/* text */
#define	N_DATA	0x6		/* data */
#define	N_BSS	0x8		/* bss */
#define	N_COMM	0x12		/* common (internal to ld) */
#define	N_FN	0x1e		/* file name symbol */

#define	N_EXT	01		/* external bit, or'ed in */
#define	N_TYPE	0x1e		/* mask for all the type bits */

/*
 * Dbx entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */
#define	N_STAB	0xe0		/* if any of these bits set, a dbx symbol */

/*
 * Format for namelist values.
 */
#define	N_FORMAT	"%08x"

/*
 * secondary sections.
 * this stuff follows the string table.
 * not even its presence or absence is noted in the
 * exec header (?). the secondary header gives
 * the number of sections. following it is an
 * array of "extra_nsects" int's which give the
 * sizeof of the individual sections. the presence of   
 * even the header is optional.  
 */
			  
#define EXTRA_MAGIC     1040            /* taxing concept  */
#define EXTRA_IDENT     0               /* ident's in 0th extra section */

struct extra_sections {
	   	int     extra_magic;            /* should be EXTRA_MAGIC */
	   	int     extra_nsects;           /* number of extra sections */
};

#endif /*!_sun3_a_out_h*/
