/*	@(#)struct.h 1.1 92/07/30 Copyright Sun Micro"		*/

/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

/***** NOTE: "sundiag.h" should be included before this file *****/
/***** NOTE: "probe_sundiag.h should be included before this file *****/

struct test_info
{
  int	group;	/* test group(index to the group info. array) */
  int	id;	/* test id(which type of test(device) */
  int	unit;	/* the unit # of the device(for mutiple devices, otherwise -1)*/
		/* the data space for the 2nd(and above) devices need to be
		   allocated dynamically at runtime(by malloc()) */
  int	type;	/* test type(0=default, 1=non_default, 2=intervention,
		   12=enabled intervention test) */
  int	popup;	/* whether option popup is needed, TRUE if yes */
  int	test_no;	/* # of tests for this device */
  int	which_test;	/* ordinal number of the test for the device */
  int	dev_enable;	/* device enable/disable flag */
  int	enable;		/* test enable/disable flag */
  int	pass;		/* # of passes */
  int	error;		/* # of errors */
  char	*priority;  /* the priority offset to be fed to nice() */
  int	pid;	    /* the process's id of this tests(0 if not running) */
  char	*label;     /* English label */
  char	*devname;   /* generic device file name */
  char	*testname;  /* test name used to invoke the test */
  char	*tail;	    /* command line options(if not NULL) */
  char	*environ;   /* environmental variables to be passed to the test */
  char	*env;	    /* temporary space for parsing the environmental variable */
		    /* also used to store command tail of user-defined tests */
  char	*special;   /* device-specific information to be displayed(e.g. size
				   of memory, disk space) */
  struct u_tag *conf;		/* device information from probing routine */
  Panel_item	select;		/* handle of the selection item */
  Panel_item	option;		/* handle of the option item */
  Panel_item	msg;		/* handle of the status message */	
  caddr_t	data;		/* test-related device specification */
};

struct group_info
{
  char	*c_label;/* group label in control window */
  char	*s_label;/* group label in status window */
  int	max_no;	 /* max. # of tests can be run at the same time in a group */
  int	tests_no;/* # of tests that are running concurrently */
  int	enable;	 /* group enable/disable flag */
  int	first;	 /* index(of the first test in the group) to test info. array */
  int	last;	 /* the index of the last forked test in this group(-1, if none
		    are running) */
  Panel_item	select;		/* handle of the group selection box */
  Panel_item	msg;		/* handle of the status message */
};

struct	loopback
{
  char	from[22];  /* source of the loopback(21 character long at most) */
  char	to[22]     /* destination of the loopback(21 character long at most) */
};

/***** Misc. defines *****/
#define	ENABLE		1
#define	DISABLE		0
	
/***** Group id defines *****/
#define	MEMGROUP	0
#define	DISKGROUP	1
#define CPUGROUP	2
#define	DEVGROUP	3
#define	TAPEGROUP	4
#define	IPCGROUP	5
#define USRGROUP	6

/***** Test id defines *****/
/* --- MEMGROUP --- */
#define	PMEM		0
#define	VMEM		PMEM+1

/* --- DISKGROUP --- */
#define	SCSIDISK1	VMEM+1		/* read only */
#define	SCSIDISK2	SCSIDISK1+1	/* read/write */
#define XYDISK1		SCSIDISK2+1	/* read only */
#define XYDISK2		XYDISK1+1	/* read/write */
#define	XDDISK1		XYDISK2+1	/* read only */
#define	XDDISK2		XDDISK1+1	/* read/write */
#define IPIDISK1	XDDISK2+1	/* read only */
#define IPIDISK2	IPIDISK1+1	/* read/write */
#define IDDISK1		IPIDISK2+1	/* read only */
#define IDDISK2		IDDISK1+1	/* read/write */
#define	SFDISK1		IDDISK2+1	/* raw write/read */
#define	SFDISK2		SFDISK1+1	/* file system read/write */
#define	OBFDISK1	SFDISK2+1	/* raw write/read */
#define	OBFDISK2	OBFDISK1+1	/* file system read/write */
#define	CDROM		OBFDISK2+1

/* --- CPUGROUP --- */
#define	MC68881		CDROM+1
#define	MC68882		MC68881+1
#define	FPUTEST		MC68882+1
#define FPU2		FPUTEST+1
#define	DES		FPU2+1
#define	ENET0		DES+1		/* on-board ethernet(ie0) */
#define ENET1		ENET0+1		/* on-board ethernet(le0) */
#define	TRNET		ENET1+1		/* S-Bus token ring */
#define SBUS_HSI        TRNET+1         /* S-Bus HSI*/
#define CPU_SP		SBUS_HSI+1	/* ttya & ttyb */
#define CPU_SP1		CPU_SP+1	/* ttyc & ttyd */
#define PP		CPU_SP1+1	/* on-board printer port */
#define	AUDIO		PP+1		/* audio port on Campus */
#define BW2		AUDIO+1		/* bwtwo0 (monochrome frame buffer */
#define COLOR3		BW2+1		/* cgthree0(on-board color) */
#define	COLOR4		COLOR3+1	/* cgfour0(on-board color) */
#define COLOR6		COLOR4+1	/* cgsix0(lego color) */
#define COLOR8		COLOR6+1	/* cgeight(ibis color) */
#define CG12		COLOR8+1	/* Egret (cg9 + gp2) */	
#define GT		CG12+1		/* Hawk2 Graphics Tower */	
#define MP4             GT+1            /* Galaxy sun4m (multi-processors) */
#define SPIF            MP4+1

#ifdef	sun386
#define	I387		SPIF+1		/* Sun386i's 387 */
#define	FPX		I387+1
#define	SUNVGA		FPX+1
#define	ENET2		SUNVGA+1	/* 2nd ethernet board(ie1) */
#else
#define	ENET2		SPIF+1		/* 2nd ethernet board(ie1) */
#endif

/* --- DEVGROUP (Others Group) --- */
					/* Interface's Network Coprocessor: */
#define OMNI_NET	ENET2+1		/* OMNI ethernet board (ne0) */
#define FDDI		OMNI_NET+1	/* fddi board */
#define TV1		FDDI+1		/* tvone0, Flamingo */
#define	COLOR2		TV1+1		/* cgtwo0 */
#define	COLOR5		COLOR2+1	/* cgfive(double frame buffer) */
#define COLOR9		COLOR5+1	/* cgnine(crane color) */
#define	FPA		COLOR9+1
#define FPA3X		FPA+1
#define	SKY		FPA3X+1
#define GP		SKY+1
#define GP2		GP+1
#define MTI		GP2+1		/* ALM */
#define	MCP		MTI+1		/* ALM2 */
#define	PRINTER		MCP+1
#define SCP2		PRINTER+1	/* SunLink(MCP) */
#define SCP		SCP2+1		/* SunLink(DCP) */
#define HSI		SCP+1		/* SunLink(HSI) */
#define SCSISP1		HSI+1
#define SCSISP2		SCSISP1+1
#define TAAC		SCSISP2+1
#define PRESTO		TAAC+1		/* prestoserve NFS accelerator */
#define VFC		PRESTO+1	/* Video Frame Capture */
#define ZEBRA1		VFC+1		/* SBus Bi-direction parallel port */
#define ZEBRA2		ZEBRA1+1	/* SBus Video Serial Port */ 

/* --- TAPEGROUP --- */
#define MAGTAPE1	ZEBRA2+1		/* mt */
#define MAGTAPE2	MAGTAPE1+1	/* xt */
#define SCSITAPE	MAGTAPE2+1

/* --- IPCGROUP --- */
#define IPC		SCSITAPE+1

/* --- USRGROUP --- */
#define	USER		IPC+1		/* user-defined tests */

#define	TEST_NO		USER+1		/* the total # of different tests */
