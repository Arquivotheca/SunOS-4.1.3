#ident "@(#)fakeromp.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/* fakeromp.c - romvec for FakePROM */

#include "fakeprom.h"

struct sunromvec romvec = {
	ROMVEC_MAGIC,	/*    u_int		v_magic;	  /* magic mushroom */
	ROMVEC_VERSION,	/*    u_int      		v_romvec_version; /* Version number of "romvec" */
	PLUGIN_VERSION,	/*    u_int		v_plugin_version; /* Plugin Architecture version */
	0,	/*    u_int		v_mon_id;	  /* version # of monitor firmware */
	&physmemp,	/*    struct memlist	**v_physmemory;	  /* total physical memory list */
	&virtmemp,	/*    struct memlist	**v_virtmemory;	  /* taken virtual memory list */
	&availmemp,	/*    struct memlist	**v_availmemory;  /* available physical memory */
	&config_ops,	/*    struct config_ops	*v_config_ops;	  /* dev_info configuration access */
		/*    /**/
		/*     * storage device access facilities*/
		/*     */
	&bootstrp,	/*    char		**v_bootcommand;  /* expanded with PROM defaults */
	open,	/*    u_int		(*v_open)(/* char *name );*/
	close,	/*    u_int		(*v_close)(/* u_int fileid ); */
		/*    /**/
		/*     * block-oriented device access*/
		/*     */
	readblks,	/*    u_int		(*v_read_blocks)();*/
	writeblks,	/*    u_int		(*v_write_blocks)();*/
		/*    /**/
		/*     * network device access*/
		/*     */
	NULL,	/*    u_int		(*v_xmit_packet)();*/
	NULL,	/*    u_int		(*v_poll_packet)();*/
		/*    /**/
		/*     * byte-oriented device access*/
		/*     */
	NULL,	/*    u_int		(*v_read_bytes)();*/
	NULL,	/*    u_int		(*v_write_bytes)();*/
		/**/
		/*    /**/
		/*     * 'File' access - i.e.,  Tapes for byte devices.  TFTP for network devices*/
		/*     */
	NULL,	/*    u_int 		(*v_seek)();*/
		/*    /**/
		/*     * single character I/O*/
		/*     */
	NULL,	/*    u_char		*v_insource;       /* Current source of input */
	NULL,	/*    u_char		*v_outsink;        /* Currrent output sink */
	NULL,	/*    u_char		(*v_getchar)();    /* Get a character from input */
	v_putchar,	/*    void		(*v_putchar)();    /* Put a character to output sink.    */
	v_mayget,/*    int			(*v_mayget)();     /* Maybe get a character, or "-1".    */
	NULL,	/*    int			(*v_mayput)();     /* Maybe put a character, or "-1".    */
		/*    /* */
		/*     * Frame buffer*/
		/*     */
	NULL,	/*    void		(*v_fwritestr)();  /* write a string to framebuffer */
		/*    /**/
		/*     * Miscellaneous Goodies*/
		/*     */
	do_boot,	/*    void		(*v_boot_me)();	   /* reboot machine */
	printf,	/*    int			(*v_printf)();	   /* handles format string plus 5 args */
	do_enter,	/*    void		(*v_abortent)();   /* Entry for keyboard abort. */
	NULL,	/*    int 		*v_nmiclock;	   /* Counts in milliseconds. */
	do_exit,	/*    void		(*v_exit_to_mon)();/* Exit from user program. */
        &vcmdptr,	/*    void		(**v_vector_cmd)();/* Handler for the vector */
	NULL,	/*    void		(*v_interpret)();  /* interpret forth string */
		/**/
		/*#ifdef SAIO_COMPAT*/
	&bootparamp,	/*    struct bootparam	**v_bootparam;	/* boot parameters and `old' style device access */
		/*#else */
			/*    int			v_fill0;*/
		/*#endif SAIO_COMPAT*/
		/**/
	NULL,	/*    u_int		(*v_mac_address)(/* int fd; caddr_t buf );*/
/*
 * new openprom stuff
 */
    NULL,	/* char        **op_bootpath;          /* Full path name of boot device */
    NULL,	/* char        **op_bootargs;          /* Boot command line after dev spec */

    NULL,	/* ihandle_t   *op_stdin;                      /* Console input device */
    NULL,	/* ihandle_t   *op_stdout;             /* Console output device */

    NULL,	/* phandle_t   (*op_phandle)();/* ihandle_t ihandle */
                                        /* Convert ihandle to phandle */

    opalloc,	/* caddr_t     (*op_alloc)();/* caddr_t virthint, u_int size */
                                        /* Allocate physical memory */

    NULL,	/* void        (*op_free)();/* caddr_t virt, u_int size */
                                        /* Deallocate physical memory */

    NULL,	/* caddr_t     (*op_map)();/* caddr_t virthint, u_int space, u_int phys,
                                    u_int size */
                                        /* Create device mapping */

    NULL,	/* void        (*op_unmap));();/* caddr_t virt, u_int size */
                                        /* Destroy device mapping */


    NULL,	/* ihandle_t   (*op_open) ();/* char *name */
    NULL,	/* u_int       (*op_close)();/* ihandle_t fileid */
    NULL,	/* int         (*op_read) ();/* ihandle_t fileid, caddr_t buf, u_int len */
    NULL,	/* int         (*op_write)();/* ihandle_t fileid, caddr_t buf, u_int len */
    NULL,	/* int         (*op_seek) ();/* ihandle_t fileid, u_int offsh, u_int offsl */


    NULL,	/* void        (*op_chain)();/* caddr_t virt, u_int size, caddr_t entry,
    /*                            caddr_t argaddr, u_int arglen */

    NULL,	/* void        (*op_release)();/* caddr_t virt, u_int size */

		/*    int			*v_reserved[15];*/
		/*    /**/
		/*     * Beginning of machine-dependent portion*/
		/*     */
};
