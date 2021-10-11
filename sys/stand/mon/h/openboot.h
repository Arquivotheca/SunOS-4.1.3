/*
 *	id: @(#)openboot.h 1.1 92/07/30
 *	purpose: 
 *	copyright: Copyright 1990 Sun Microsystems, Inc.  All Rights Reserved
 */
/*
 * This file defines the interface between the Open Boot Prom Monitor
 * and the programs that may request its services.
 * This interface, defined as a vector of pointers to useful things,
 * has been traditionally referred to as `the romvec'.
 *
 * The address of the romvec is passed as the first argument to the standalone
 * program, obviating the need for the address to be at a known fixed location.
 * Typically, the stand-alone's srt0.s file (which contains the _start entry)
 * would take care of all of `this'.
 * In SPARC assembler, `this' would be:
 *
 *	.data
 *	.global _romp
 * _romp:
 *	.word 0
 *	.text
 * _start:
 *	sethi	%hi(_romp),%g1		! %g1 could be any but %o1
 *	st	%o0,[%g1+%lo(_romp)]	! save it away
 *	.......			! set up your stack, etc......
 */

#ifndef _openboot_h
#define	_openboot_h

extern struct openboot *romp; /* as per the above discussion */

#define OBP_BOOTPATH		(*romp->op_bootpath)
#define OBP_BOOTARGS		(*romp->op_bootargs)

#define OBP_STDIN		(*romp->op_stdin)
#define OBP_STDINPATH		(*romp->op_stdin_path)

#define OBP_STDOUT		(*romp->op_stdout)
#define OBP_STDOUTPATH		(*romp->op_stdout_path)

#define OBP_NEXT		(*romp->op_config_ops->devr_next)
#define OBP_CHILD		(*romp->op_config_ops->devr_child)
#define OBP_GETPROPLEN		(*romp->op_config_ops->devr_getproplen)
#define OBP_GETPROP		(*romp->op_config_ops->devr_getprop)
#define OBP_SETPROP		(*romp->op_config_ops->devr_setprop)
#define OBP_NEXTPROP		(*romp->op_config_ops->devr_nextprop)

#define OBP_BOOT		(*romp->op_boot)
#define OBP_ENTER		(*romp->op_enter)
#define OBP_EXIT		(*romp->op_exit)

#define OBP_INTERPRET		(*romp->op_interpret)

#define OBP_PHANDLE		(*romp->op_phandle)

#define OBP_ALLOC		(*romp->op_alloc)
#define OBP_FREE		(*romp->op_free)
#define OBP_MAP			(*romp->op_map)
#define OBP_UNMAP		(*romp->op_unmap)

#define OBP_OPEN		(*romp->op_open)
#define OBP_CLOSE		(*romp->op_close)
#define OBP_READ		(*romp->op_read)
#define OBP_WRITE		(*romp->op_write)
#define OBP_SEEK		(*romp->op_seek)

#define OBP_MILISECS		(*romp->op_milisecs)

#define OBP_CALLBACK		(*romp->op_callback)
#define OBP_CHAIN		(*romp->op_chain)
#define OBP_RELEASE		(*romp->op_release)

#define OBP_STARTCPU		(*romp->op_startcpu)
#define OBP_STOPCPU		(*romp->op_stopcpu)
#define OBP_IDLECPU		(*romp->op_idlecpu)
#define OBP_RESUMECPU		(*romp->op_resumecpu)

/*
 *  OBP Module mailbox messages
 *
 *	00..7F	: power-on self test
 *
 *	80..8F	: active in boot prom ( at the "ok" prompt )
 *
 *	90..EF  : idle in boot prom
 *
 *	F0	: active in application
 *
 *	F1..FA	: reserved for future use
 *
 *	FB	: One processor exited to the PROM via op_exit(),
 *		  call to (*romp->op_stopcpu)() requested.
 *
 *	FC	: One processor entered the PROM via op_enter(),
 *		  call to (*romp->op_idlecpu)() requested.
 *
 *	FD	: One processor hit a BREAKPOINT,
 *		  call to (*romp->op_idlecpu)() requested.
 *
 *	FE	: One processor got a WATCHDOG RESET
 *		  call to (*romp->op_stopcpu)() requested.
 *
 *	FF	: This processor not available.
 *
 */

#define OBP_MAGIC		0x10010407
#define OBP_ROMVEC_VERSION	3
#define OBP_PLUGIN_VERSION	2

typedef unsigned int ihandle_t;
typedef unsigned int phandle_t;
typedef unsigned int dnode_t;

/*
 * Autoconfig operations
 */
struct config_ops {
	dnode_t (*devr_next) ( /* dnode_t nodeid */ );
	dnode_t (*devr_child) ( /* dnode_t nodeid */ );
	int     (*devr_getproplen) ( /* dnode_t nodeid; caddr_t name; */ );
	int	(*devr_getprop) ( /* dnode_t nodeid; caddr_t name; addr_t
			         value; */ );
	int     (*devr_setprop) ( /* dnode_t nodeid; caddr_t name; addr_t
			         value; int len; */ );
	caddr_t (*devr_nextprop) ( /* dnode_t nodeid; caddr_t previous; */ );
};

#if 0
/*
 * This structure represents one piece of bus space occupied by a given
 * device. It is used in an array for devices with multiple address windows.
 *
 * ### kernel files that need this get it from <sun/openprom.h>
 */
typedef struct dev_reg {
	int reg_bustype;		/* cookie for bus type it's on */
	addr_t reg_addr;		/* address of reg relative to bus */
	u_int reg_size;			/* size of this register set */
} dev_reg_t;
#endif

struct openboot {
	u_int op_magic;          /* magic mushroom */
	u_int op_romvec_version; /* Version number of "romvec" */
	u_int op_plugin_version; /* Plugin Architecture version */
	u_int op_mon_id;         /* version # of monitor firmware */

	char  **op_bootpath; /* Full path name of boot device */
	char  **op_bootargs; /* Boot command line after dev spec */

	ihandle_t *op_stdin;   /* Console input device */
	char  **op_stdin_path; /* Full path name of stdin device */

	ihandle_t *op_stdout;   /* Console output device */
	char  **op_stdout_path; /* Full path name of stdout device */

	struct config_ops *op_config_ops; /* dev_info configuration access */

	void  (*op_boot) ( /* caddr_t cmd */ ); /* reboot machine */
	void  (*op_enter) ( /* void */ );       /* Entry for keyboard abort. */
	void  (*op_exit) ( /* void */ );        /* Exit from user program. */

	u_int (*op_interpret) ( /* caddr_t cmd */ ); /* interpret forth string */

		/* Convert ihandle to phandle */
	phandle_t (*op_phandle) ( /* ihandle_t ihandle */ );

		/* Allocate physical memory */
	caddr_t (*op_alloc) ( /* caddr_t virthint, u_int size */ );
		/* Deallocate physical memory */
	void    (*op_free) ( /* caddr_t virt, u_int size */ );
		/* Create device mapping */
	caddr_t (*op_map) ( /* caddr_t virthint, u_int space, u_int phys, u_int size */ );	
		/* Destroy device mapping */
	void (*op_unmap) ( /* caddr_t virt, u_int size */ );	

	ihandle_t (*op_open) ( /* char *name */ );
	int       (*op_close) ( /* ihandle_t fileid */ );
	int       (*op_read) ( /* ihandle_t fileid, caddr_t buf, u_int len */ );
	int       (*op_write) ( /* ihandle_t fileid, caddr_t buf, u_int len */ );
	int       (*op_seek) ( /* ihandle_t fileid, u_int offsh, u_int offsl */ );

	int   *op_millisecs;  /* Counts in milliseconds. */

	void  (**op_callback) ( /* void */ ); /* Handler for the vector */
	void (*op_chain) ( /* caddr_t virt, u_int size, caddr_t
			    entry, caddr_t argaddr, u_int arglen */ );
	void (*op_release) ( /* caddr_t virt, u_int size */ );

	/*
	 * sun4c/sun4 only: It's undefined to call this from other
	 * than a sun4/sun4c.
	 */

	void (*setcxsegmap)();

	/*
	 * Beginning of Multiprocessor sections: It's an error to call
	 * these on a UP.
	 */

	int (*op_startcpu)( /* dnode_t moduleid; dev_reg_t contextable; 
				int whichcontext;  addr_t pc; */ );
	int (*op_stopcpu)( /* dnode_t moduleid */ );

	int (*op_idlecpu)( /* dnode_t moduleid */ );
	int (*op_resumecpu)( /* dnode_t moduleid; */ );


};
#endif /* _openboot_h */
