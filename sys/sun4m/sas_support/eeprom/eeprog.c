#ident "@(#)eeprog.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*
 * eeprog.c - generate a fake sun4m NVRAM image
 *
 */
#include	<fcntl.h>
#include	<stdio.h>
#include 	<time.h>
#include	<mon/openprom.h>
#include	<mon/idprom.h>
#include	<machine/clock.h>
	
#define	VEEPROM		0xFEFFF000
#define	IDPOS		0x1FD8
#define	CLKPOS		0x1FF8


/************************* idprom **********************************/

struct idprom idp = {
	IDFORM_1,	/* format identifier */
	/* The following fields are valid only in format IDFORM_1. */
	MACHINE,	/* machine type */
	1,2,3,4,5,6, 	/* ethernet address */
	0x20000000,	/* date of manufacture */
	0x000001,	/* serial number - 24bits */
	0,		/* xor checksum */
	0,0,0,0,0,0,	/* undefined - 16 bytes */
};

/*************************** clock *********************************/

struct mostek48T02  clock = {
	0,/* 	u_char	clk_ctrl;	/* ctrl register */
	0,/* 	u_char	clk_sec;	/* counter - seconds 0-59 */
	0,/* 	u_char	clk_min;	/* counter - minutes 0-59 */
	0,/* 	u_char	clk_hour;	/* counter - hours 0-23 */
	0, /* 	u_char	clk_weekday;	/* counter - weekday 1-7 */
	0,/* 	u_char	clk_day;	/* counter - day 1-31 */
	0,/* 	u_char	clk_month;	/* counter - month 1-12 */
	0 /* 	u_char	clk_year;	/* counter - year 0-99 */
};

/************************** memory *********************************/

struct memlist  phy_mem = {
	0,   /* struct memlist	*next;		/* link to next list element */
	0,   /* u_int		address;	/* starting address of memory segment */
	0x400000,   /* u_int		size;		/* size of same */
};
struct memlist  vir_mem = {
	0,   /* struct memlist	*next;		/* link to next list element */
	0xFFD00000,   /* u_int		address;	/* starting address of memory segment */
	0x200000,   /* u_int		size;		/* size of same */
};
struct memlist  avail_mem = {
	0,   /* struct memlist	*next;		/* link to next list element */
	0,   /* u_int		address;	/* starting address of memory segment */
	0x379000,   /* u_int		size;		/* size of same */
};


/********************* eeprom structure ******************************/

struct sunromvec  ee = {
	0,  /* u_int		v_magic;	  /* magic mushroom */
	0,  /* u_int      		v_romvec_version; /* Version number of "romvec" */
	0,  /* u_int		v_plugin_version; /* Plugin Architecture version */
	0,  /* u_int		v_mon_id;	  /* version # of monitor firmware */
	/* struct memlist	**v_physmemory;	  /* total physical memory list */
	(struct memlist   **) (sizeof(struct sunromvec) + VEEPROM),
	/* struct memlist	**v_virtmemory;	  /* taken virtual memory list */
	(struct memlist   **) (sizeof(struct sunromvec) + VEEPROM + sizeof(struct memlist)),
	/* struct memlist	**v_availmemory;  /* available physical memory */
	(struct memlist   **) (sizeof(struct sunromvec) + VEEPROM + (2*sizeof(struct memlist))),
	0,  /* struct config_ops *v_config_ops;	  /* dev_info configuration access */
	/*
	 * storage device access facilities
	 */
	0,  /* char		**v_bootcommand;  /* expanded with PROM defaults */
	0,  /* u_int		(*v_open)(/* char *name * /); */
		0,  /* u_int		(*v_close)(/* u_int fileid * /); 
		       /*
			* block-oriented device access
			*/
	0,  /* u_int		(*v_read_blocks)(); */
	0,  /* u_int		(*v_write_blocks)(); */
	/*
	 * network device access
	 */
	0,  /* u_int		(*v_xmit_packet)(); */
	0,  /* u_int		(*v_poll_packet)(); */
	/*
	 * byte-oriented device access
	 */
	0,  /* u_int		(*v_read_bytes)(); */
	0,  /* u_int		(*v_write_bytes)(); */
	
	/*
	 * 'File' access - i.e.,  Tapes for byte devices.  TFTP for network devices
	 */
	0,  /* u_int 		(*v_seek)(); */
	/*
	 * single character I/O
	 */
	0,  /* u_char		*v_insource;       /* Current source of input */
	0,  /* u_char		*v_outsink;        /* Currrent output sink */
	0,  /* u_char		(*v_getchar)();    /* Get a character from input */ 
	0,  /* void	(*v_putchar)();    /* Put a character to output sink.    */
	0,  /* int	(*v_mayget)();     /* Maybe get a character, or "-1".    */
	0,  /* int	(*v_mayput)();     /* Maybe put a character, or "-1".    */
	/* 
	 * Frame buffer
	 */
	0,  /* void		(*v_fwritestr)();  /* write a string to framebuffer */
	/*
	 * Miscellaneous Goodies
	 */
	0,  /* void		(*v_boot_me)();	   /* reboot machine */
	0,  /* int		(*v_printf)();	   /* handles format string plus 5 args */
	0,  /* void		(*v_abortent)();   /* Entry for keyboard abort. */
	0,  /* int 		*v_nmiclock;	   /* Counts in milliseconds. */
	0,  /* void		(*v_exit_to_mon)();/* Exit from user program. */
	0,  /* void		(**v_vector_cmd)();/* Handler for the vector */
	0,  /* void		(*v_interpret)();  /* interpret forth string */
	
	0,  /* struct bootparam	**v_bootparam;	/* boot parameters and `old' style device access */
	
	0,  /* u_int		(*v_mac_address)(/* int fd; caddr_t buf * /); */
		/* Copyout ether address */
		0,  /* int			*v_reserved[31]; */
		/*
		 * Beginning of machine-dependent portion
		 */
		0,  /* int     **v_lomemptaddr;    /* address of low memory ptes      */
	0,  /* int     **v_monptaddr;      /* address of debug/mon ptes       */
	0,  /* int     **v_dvmaptaddr;     /* address of dvma ptes            */
	0,  /* int     **v_monptphysaddr;  /* Physical Addr of the KADB PTE's */
	0,  /* int     **v_shadowpteaddr;  /* addr of shadow cp of DVMA pte's */
};

main(argc, argv)
char *argv[];
int argc;
{
	int	rtn, eefile, data;
	char	*eedata, *phydata, *virdata, *availdata;
	char	zero = 0;
	int	i, eepos = 0;
	u_char    *cp;
	u_char   val = 0;
	struct tm *tm;
	long	now;

	eedata = (char *) &ee;
	phydata = (char *) &phy_mem;
	virdata = (char *) &vir_mem;
	availdata = (char *) &avail_mem;

	if (argc != 2) {
		printf("Usage: %s eeprom.state\n", argv[0]);
		exit(2);
	}
	if ((eefile = open(argv[1],O_RDWR|O_CREAT, 0666)) == -1) {
		perror("open data file failed");
		exit(1);
	}

	write(eefile, eedata, (sizeof(struct sunromvec)));
	eepos += sizeof(struct sunromvec);
	write(eefile, phydata, (sizeof(struct memlist)));
	eepos += sizeof(struct memlist);
	write(eefile, virdata, (sizeof(struct memlist)));
	eepos += sizeof(struct memlist);
	write(eefile, availdata, (sizeof(struct memlist)));
	eepos += sizeof(struct memlist);
	eedata = &zero;
	for (; eepos < IDPOS; eepos++) write(eefile, eedata, 1);

	/* calculate checksum - see sun4m/autoconf.c getid() */
	cp = (u_char *)&idp;
	for (i = 0; i < 15; i++)
		val ^= *cp++;
	idp.id_xsum = val;

	eedata = (char *) &idp;
	
	write(eefile, eedata, (sizeof(struct idprom)));
	eepos += sizeof(struct idprom);
/* MARK -- DID YOU REALLY WANT TO WRITE THE IDPROM OUT TWICE??? -- LIMES
	write(eefile, eedata, (sizeof(struct idprom)));
	eepos += sizeof(struct idprom);
*/
	now = time(0);
	tm = gmtime(&now);
	/*
	 * clock chip keeps data in BCD
	 */
#define	btobcd(x)	((((x) / 10) << 4) + ((x) % 10))
	clock.clk_ctrl = 0x10;
	clock.clk_sec = btobcd(tm->tm_sec);
	clock.clk_min = btobcd(tm->tm_min);
	clock.clk_hour = btobcd(tm->tm_hour);
	/* chip's weekdays are 1..7, 7=sunday
	 * unix's weekdays are 0..6, 0=sunday */
	clock.clk_weekday = btobcd(((tm->tm_wday + 6) % 7) + 1);
	/* chip's days are 0x01..0x31
	 * unix's days are 1..31 */
	clock.clk_day = btobcd(tm->tm_mday);
	/* chip's months are 0x01..0x12
	 * unix's months are 0..11 */
	clock.clk_month = btobcd(tm->tm_mon + 1);
	/* chip's year base is 1968
	 * unix's year base is 1900 */
	clock.clk_year = btobcd(tm->tm_year - 68);

	eedata = (char *) &clock;
	write(eefile, eedata, (sizeof(struct mostek48T02)));
	exit(0);
}
