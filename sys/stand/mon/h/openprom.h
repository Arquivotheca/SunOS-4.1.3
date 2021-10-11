/*
 * @(#)openprom.h 1.1 92/07/30 SMI
 * Copyright (c) 1989, 1990 Sun Microsystems, Inc.
 *
 */


/*
 * NOTICE: This should only be included by a very limitted number
 * of PROM library functions that interface directly to the PROM.
 * All others should use the library interface functions to avoid
 * dealing with the various differences in the PROM interface versions.
 *
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
 *	set	_romp, %o1	! any register other than %o0 will probably do
 *	st	%o0, [%o1]	! save it away
 *	.......			! set up your stack, etc......
 */

#ifndef _mon_openprom_h
#define	_mon_openprom_h

#include <mon/obpdefs.h>

extern struct sunromvec *romp;		/* as per the above discussion */

/*
 * Autoconfig operations
 */
struct config_ops {
	dnode_t	(*devr_next)();		/* next(nodeid); */
	dnode_t	(*devr_child)();	/* child(nodeid); */
	int	(*devr_getproplen)();	/* getproplen(nodeid, name); */
	int	(*devr_getprop)();	/* getprop(nodeid, name, buf); */
	int	(*devr_setprop)();	/* setprop(nodeid, name, val, size); */
	caddr_t	(*devr_nextprop)();	/* nextprop(nodeid, latpropname); */
};

/*
 * Other than the first four fields of the sunromvec,
 * the fields in the V3 romvec are not aligned since
 * they don't have compatability with the old fields.
 *
 * KEY:
 *		op_XXX:		Interfaces common to *all* versions.
 *		v_XXX:		V0 Interface (included in V2 for compatability)
 *		op2_XXX:	V2 and later only
 *		op3_XXX:	V3 and later only
 *
 *	It's a fatal error (results are completely undefined and range from
 *	unkown behaviour to deadlock) to call a non-existant interface.
 *	(I.e.: A V0 interface from a V3 PROM or an MP-only interface on a
 *	UP.)  The structures line up for programmers convenience only!
 */

struct sunromvec {
	u_int		op_magic;	  /* magic mushroom */
	u_int		op_romvec_version; /* Version number of "romvec" */
	u_int		op_plugin_version; /* Plugin Architecture version */
	u_int		op_mon_id;	  /* version # of monitor firmware */

	struct memlist	**v_physmemory;	/* total physical memory list */
	struct memlist	**v_virtmemory;	/* taken virtual memory list */
	struct memlist	**v_availmemory; /* available physical memory */
	struct config_ops *op_config_ops; /* dev_info configuration access */

	/*
	 * storage device access facilities
	 */
	char		**v_bootcommand;  /* expanded with PROM defaults */
	u_int		(*v_open)();	/* v_open(char *name); */
	u_int		(*v_close)();	/* v_close(ihandle_t fileid); */

	/*
	 * block-oriented device access
	 */
	u_int		(*v_read_blocks)();
	u_int		(*v_write_blocks)();

	/*
	 * network device access
	 */
	u_int		(*v_xmit_packet)();
	u_int		(*v_poll_packet)();

	/*
	 * byte-oriented device access
	 */
	u_int		(*v_read_bytes)();
	u_int		(*v_write_bytes)();

	/*
	 * 'File' access - i.e.,  Tapes for byte devices.
	 * TFTP for network devices
	 */
	u_int 		(*v_seek)();

	/*
	 * single character I/O
	 */
	u_char		*v_insource;	/* Current source of input */
	u_char		*v_outsink;	/* Currrent output sink */
	u_char		(*v_getchar)();	/* Get a character from input */
	void		(*v_putchar)();	/* Put a character to output sink. */
	int		(*v_mayget)();	/* Maybe get a character, or "-1". */
	int		(*v_mayput)();	/* Maybe put a character, or "-1". */

	/*
	 * Frame buffer
	 */
	void		(*v_fwritestr)(); /* write a string to framebuffer */

	/*
	 * Miscellaneous Goodies
	 */
	void		(*op_boot)();	/* reboot machine */
	int		(*v_printf)();	/* handles fmt string plus 5 args */
	void		(*op_enter)();	/* Entry for keyboard abort. */
	int 		*op_milliseconds; /* Counts in milliseconds. */
	void		(*op_exit)();	/* Exit from user program. */

	/*
	 * Note:  Different semantics for V0 versus other op_vector_cmd:
	 */
	void		(**op_vector_cmd)(); /* Handler for the vector */
	void		(*op_interpret)();  /* interpret forth string */

	struct bootparam **v_bootparam;	/* boot parameters and `old' style */
					/* device access */

	u_int		(*v_mac_address)(); /* Copyout ether address */
				/* v_mac_address(int fd, caddr_t buf); */

	/*
	 * new V2 openprom stuff
	 */
	char	**op2_bootpath;		/* Full path name of boot device */
	char	**op2_bootargs;		/* Boot command line after dev spec */

	ihandle_t	*op2_stdin;		/* Console input device */
	ihandle_t	*op2_stdout;		/* Console output device */

	phandle_t	(*op2_phandle)(); /* Convert ihandle to phandle */
						/* op2_phandle(ihandle_t); */

	caddr_t	(*op2_alloc)();		/* Allocate physical memory */
				/* op2_alloc(caddr_t virthint, u_int size); */

	void	(*op2_free)();		/* Deallocate physical memory */
				/* op2_free(caddr_t virt, u_int size); */

	caddr_t	(*op2_map)();		/* Create device mapping */
				/* (*op2_map)(caddr_t virthint, u_int space, */
					/* u_int phys, u_int size); */

	void	(*op2_unmap)();		/* Destroy device mapping */
				/* *op2_unmap(caddr_t virt, u_int size); */

	ihandle_t  (*op2_open)();	/* ihandle_t op2_open(char *name); */
	u_int	(*op2_close)();		/* u_int op2_close(ihandle_t fileid); */
	int	(*op2_read)();
			/* op2_read(ihandle_t fileid, caddr_t buf, u_int len) */

	int	(*op2_write)();
			/* op2_write(ihandle_t f_id, caddr_t buf, u_int len) */

	int	(*op2_seek)();
			/* op2_seek(ihandle_t f_id, u_int offsh, u_int offsl) */


	void	(*op2_chain)();
				/* op2_chain)(caddr_t virt, u_int size,  */
					/* caddr_t entry, caddr_t argaddr,  */
					/* u_int arglen); */

	void	(*op2_release)();  /* op2_release(caddr_t virt, u_int size); */

	/*
	 * End V2 stuff
	 */

	int	*v_reserved[15];

	/*
	 * Sun4c specific romvec routines (From sys/sun4c/machine/romvec.h)
	 * Common to all PROM versions.
	 */

	void	(*op_setcxsegmap)();	/* Set segment in any context. */

	/*
	 * V3 MP only functions: It's a fatal error to call these from a UP.
	 */


	int (*op3_startcpu)();
				/* op3_startcpu(dnode_t moduleid, */
					/* dev_reg_t contextable, */
					/* int whichcontext,  */
					/* addr_t pc); */

	int (*op3_stopcpu)();	/* op3_stopcpu(dnode_t); */

	int (*op3_idlecpu)();	/* op3_idlecpu(dnode_t); */
	int (*op3_resumecpu)();	/* op3_resumecpu(dnode_t); */
};

/*
 * XXX - This is not, strictly speaking, correct: we should never
 * depend upon "magic addresses". However, the MONSTART/MONEND
 * addresses are wired into too many standalones, and they are
 * not likely to change in any event.
 */
#if	defined(sun4c) || defined(sun4m)
#define	MONSTART	(0xffd00000)
#define	MONEND		(0xfff00000)
#endif




/*
 * Common Interfaces in all OBP versions:
 */

/*
 * boot(char *bootcommand)
 */
#define	OBP_BOOT		(*romp->op_boot)

/*
 * Abort/enter to the monitor (may be resumed)
 */
#define	OBP_ENTER_MON		(*romp->op_enter)

/*
 * Abort/exit to monitor (may not be resumed)
 */
#define	OBP_EXIT_TO_MON		(*romp->op_exit)

/*
 * PROM Interpreter: (char *forth_string)
 */
#define	OBP_INTERPRET		(*romp->op_interpret)

/*
 * Callback handler:  (Set it to your handler)
 *
 * WARNING: The handler code is different in V0 vs. Later interfaces.
 */
#define	OBP_CB_HANDLER	(*romp->op_vector_cmd)

/*
 * Unreferenced time in milliseconds
 */
#define	OBP_MILLISECONDS	(*romp->op_milliseconds)

/*
 * Configuration ops: (all OBP versions)
 */
#define	OBP_DEVR_NEXT		(*romp->op_config_ops->devr_next)
#define	OBP_DEVR_CHILD		(*romp->op_config_ops->devr_child)
#define	OBP_DEVR_GETPROPLEN	(*romp->op_config_ops->devr_getproplen)
#define	OBP_DEVR_GETPROP	(*romp->op_config_ops->devr_getprop)
#define	OBP_DEVR_SETPROP	(*romp->op_config_ops->devr_setprop)
#define	OBP_DEVR_NEXTPROP	(*romp->op_config_ops->devr_nextprop)

/*
 * sun4/sun4c ONLY... For Sun MMU's  It's undefined to use these on other
 * architectures, SO DON'T DO IT!
 */
#define	OBP_SETCXSEGMAP		(*romp->op_setcxsegmap)



/*
 * V0/V2 Only Interfaces: (They're only in V2 for compatability.)
 * Where possible, should use the V2/V3 interfaces.  For V2, probably
 * the only required interface is the insource/outsink since there's
 * no V2 interface corresponding to root node properties stdin-path/stdout-path.
 */

#define	OBP_V0_PHYSMEMORY	(*romp->v_physmemory)
#define	OBP_V0_VIRTMEMORY	(*romp->v_virtmemory)
#define	OBP_V0_AVAILMEMORY	(*romp->v_availmemory)

#define	OBP_V0_BOOTCMD		(*romp->v_bootcmd)

#define	OBP_V0_OPEN		(*romp->v_open)
#define	OBP_V0_CLOSE		(*romp->v_close)

#define	OBP_V0_READ_BLOCKS	(*romp->v_read_blocks)
#define	OBP_V0_WRITE_BLOCKS	(*romp->v_write_blocks)

#define	OBP_V0_XMIT_PACKET	(*romp->v_xmit_packet)
#define	OBP_V0_POLL_PACKET	(*romp->v_poll_packet)

#define	OBP_V0_READ_BYTES	(*romp->v_read_bytes)
#define	OBP_V0_WRITE_BYTES	(*romp->v_write_bytes)

#define	OBP_V0_SEEK		(*romp->v_seek)

#define	OBP_V0_INSOURCE		(*romp->v_insource)
#define	OBP_V0_OUTSINK		(*romp->v_outsink)

#define	OBP_V0_GETCHAR		(*romp->v_getchar)
#define	OBP_V0_PUTCHAR		(*romp->v_putchar)

#define	OBP_V0_MAYGET		(*romp->v_mayget)
#define	OBP_V0_MAYPUT		(*romp->v_mayput)

#define	OBP_V0_FWRITESTR	(*romp->v_fwritestr)

#define	OBP_V0_PRINTF		(*romp->v_printf)

#define	OBP_V0_BOOTPARAM	(*romp->v_bootparam)

#define	OBP_V0_MAC_ADDRESS	(*romp->v_mac_address)


/*
 * V2 and V3 interfaces:
 */

#define	OBP_V2_BOOTPATH		(*romp->op2_bootpath)
#define	OBP_V2_BOOTARGS		(*romp->op2_bootargs)

#define	OBP_V2_STDIN		(*romp->op2_stdin)
#define	OBP_V2_STDOUT		(*romp->op2_stdout)

#define	OBP_V2_PHANDLE		(*romp->op2_phandle)

#define	OBP_V2_ALLOC		(*romp->op2_alloc)
#define	OBP_V2_FREE		(*romp->op2_free)

#define	OBP_V2_MAP		(*romp->op2_map)
#define	OBP_V2_UNMAP		(*romp->op2_unmap)

#define	OBP_V2_OPEN		(*romp->op2_open)
#define	OBP_V2_CLOSE		(*romp->op2_close)

#define	OBP_V2_READ		(*romp->op2_read)
#define	OBP_V2_WRITE		(*romp->op2_write)
#define	OBP_V2_SEEK		(*romp->op2_seek)

#define	OBP_V2_CHAIN		(*romp->op2_chain)
#define	OBP_V2_RELEASE		(*romp->op2_release)

/*
 * V3 Only MP Interfaces:
 */
#define	OBP_V3_STARTCPU		(*romp->op3_startcpu)
#define	OBP_V3_STOPCPU		(*romp->op3_stopcpu)
#define	OBP_V3_IDLECPU		(*romp->op3_idlecpu)
#define	OBP_V3_RESUMECPU	(*romp->op3_resumecpu)

#endif _mon_openprom_h
