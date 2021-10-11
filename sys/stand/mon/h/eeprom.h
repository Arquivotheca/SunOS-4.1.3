/*
 * eeprom.h
 *
 * @(#)eeprom.h 1.1 92/07/30 SMI  
 * Copyright (c) 1986, 1987 Sun Microsystems, Inc.
 */

/*
 * This structure defines the contents of the EEPROM for all Sun
 * Workstations with the SunMon monitor. (Sun Workstations with
 * the Open Boot monitor, e.g. SPARCstation-1, use a different
 * layout that is opaque to application programs. All access
 * to Open Boot-type EEPROMs is through monitor resources; see
 * eeprom(8) for more details.)
 *
 * The EEPROM is divided into a diagnostic section, a reserved section,
 * a ROM section, and a software section (defined in detail
 * elsewhere).
 */

#ifndef _mon_eeprom_h
#define _mon_eeprom_h

#include "password.h"
#include "idprom.h"

/* Guard for some minor differences between kbd.h and keyboard.h */

#ifndef _sundev_kbd_h
#include "keyboard.h"
#endif

struct ee_keymap {
      unsigned char   keymap[128];    /* PROM/EEPROM is still in 7-bit land */
};

#define MAX_SLOTS	12
#define CONFIG_SIZE	16	/* bytes of config data for board slot */

struct	eeprom {
	struct	ee_diag {		/* diagnostic's area of EEPROM */
/* 0x000 */	u_int	eed_test;	/* diagnostic test write area */
/* 0x004 */	u_short	eed_wrcnt[3];	/* diag area write count (3 copies) */
		short	eed_nu1;	/* not used */

/* 0x00c */	u_char	eed_chksum[3];	/* diag area checksum (3 copies) */
		char	eed_nu2;	/* not used */
/* 0x010 */	time_t	eed_hwupdate;	/* date of last hardware update */

/* 0x014 */	char	eed_memsize;	/* MB's of memory in system */
/* 0x015 */	char	eed_memtest;	/* MB's of memory to test on powerup */

/* 0x016 */	char	eed_scrsize;	/* screen size, in pixels */
#define	EED_SCR_1152X900	0x00
#define	EED_SCR_1024X1024	0x12
#define EED_SCR_1600X1280       0x13    /* new for hi rez */
#define EED_SCR_1440X1440       0x14
#define EED_SCR_640X480         0x15
#define EED_SCR_1280X1024	0x16	/* for Jet */

/* 0x017 */	char	eed_dogaction;	/* action to take on watchdog reset */
#define	EED_DOG_MONITOR		0x00	/* return to monitor command level */
#define	EED_DOG_REBOOT		0x12	/* perform power on reset and reboot */

/* 0x018 */	char	eed_defboot;	/* default boot? */
#define	EED_DEFBOOT		0x00	/* do default boot */
#define	EED_NODEFBOOT		0x12	/* don't do default boot */

/* 0x019 */	char	eed_bootdev[2];	/* name of boot device (e.g. xy, ie) */

/* 0x01b */	u_char	eed_bootctrl;	/* controller number to boot from */
/* 0x01c */	u_char	eed_bootunit;	/* unit number to boot from */
/* 0x01d */	u_char	eed_bootpart;	/* partition number to boot from */

/* 0x01e */	char	eed_kbdtype;	/* non-Sun keyboard type - for OEM's */
#define	EED_KBD_SUN	0		/* one of the Sun keyboards */

/* 0x01f */	char	eed_console;	/* device to use for console */
#define	EED_CONS_BW	0x00		/* use b&w monitor for console */
#define	EED_CONS_TTYA	0x10		/* use tty A port for console */
#define	EED_CONS_TTYB	0x11		/* use tty B port for console */
#define	EED_CONS_COLOR	0x12		/* use color monitor for console */
#define EED_CONS_P4	0x20		/* use the P4 monitor for console */

/* 0x020 */	char	eed_showlogo;	/* display Sun logo? */
#define	EED_LOGO	0x00
#define	EED_NOLOGO	0x12

/* 0x021 */	char	eed_keyclick;	/* keyboard click? */
#define	EED_NOKEYCLICK	0x00
#define	EED_KEYCLICK	0x12

/* 0x022 */	char    eed_diagdev[2]; /* name of boot device (e.g. xy, ie) */
/* 0x024 */	u_char    eed_diagctrl;   /* controller number to boot from */
/* 0x025 */	u_char    eed_diagunit;   /* unit number to boot from */
/* 0x026 */	u_char    eed_diagpart;   /* partition number to boot from */
#define	EED_WOB	0x12                      /* Bring system up white on black*/
/* 0x027 */	u_char    eed_videobg	;     /* not used */
/* 0x028 */	char    eed_diagpath[40]; /* boot path of diagnostic */
#define EED_TERM_34x80	00
#define EED_TERM_48x120 0x12    /* for large scrn size 1600x1280 */
/* 0x050 */     char    eed_colsize;   /* number of columns */
/* 0x051 */     char    eed_rowsize;   /* number of rows */

/* 0x052 */	char	eed_nu5[6];	/* not used */

/* 0x058 */	struct	eed_tty_def {	/* tty port defaults */
			char	eet_sel_baud;	/* user specifies baud rate */
#define	EET_DEFBAUD	0x00
#define	EET_SELBAUD	0x12
			u_char	eet_hi_baud;	/* upper byte of baud rate */
			u_char	eet_lo_baud;	/* lower byte of baud rate */
			u_char	eet_rtsdtr;	/* flag for dtr and rts */
#define NO_RTSDTR	0x12
			char	eet_unused[4];
		} eed_ttya_def, eed_ttyb_def;
/* 0x068 */	char	eed_banner[80];	/* banner if not displaying Sun logo */
			/* last two chars must be \r\n (XXX - why not \0?) */

/* 0x0b8 */	u_short	eed_pattern;	/* test pattern - must contain 0xAA55 */
/* 0x0ba */   	short   eed_nu6;        /* not used */
/* 0x0bc */	struct	eed_conf {	/* system configuration, by slot */
		    union {
			struct	eec_gen {
				u_char	eec_type;	/* board type code */
				char	eec_size[CONFIG_SIZE-1];
			} eec_gen;

			char	conf_byte[CONFIG_SIZE];
			u_char	eec_type;	/* type of this board */
#define	EEC_TYPE_NONE	0			/* no board this slot */
#define	EEC_TYPE_CPU	0x01			/* cpu */
			struct	eec_cpu {
				u_char	eec_type;	/* type of this board */
				u_char	eec_cpu_memsize; /* MB's on cpu */
				int	eec_cpu_unused:6;
				int	eec_cpu_dcp:1;		/* dcp? */
				int	eec_cpu_68881:1;	/* 68881? */
				u_char	eec_cpu_cachesize; /* KB's in cache */
			} eec_cpu;
			struct	eec_cpu_alt {
				u_char	eec_type;	/* type of this board */
				u_char	memsize;	/* MB's on cpu */
				u_char	option;		/* option flags */
#define CPU_HAS_DCP	0x02
#define CPU_HAS_68881	0x01
				u_char	cachesize;	/* KB's in cache */
			} eec_cpu_alt;

#define	EEC_TYPE_MEM	0x02			/* memory board */
			struct	eec_mem {
				u_char	eec_type;	/* type of this board */
				u_char	eec_mem_size;	/* MB's on card */
			} eec_mem;

#define	EEC_TYPE_COLOR	0x03			/* color frame buffer */
			struct	eec_color {
				u_char	eec_type;	/* type of this board */
				char	eec_color_type;
#define	EEC_COLOR_TYPE_CG2	2	/* cg2 color board */
#define	EEC_COLOR_TYPE_CG3	3	/* cg3 color board */
#define	EEC_COLOR_TYPE_CG5	5	/* cg5 color board */
			} eec_color;

#define	EEC_TYPE_BW	0x04			/* b&w frame buffer */

#define	EEC_TYPE_FPA	0x05			/* floating point accelerator */

#define	EEC_TYPE_DISK	0x06			/* SMD disk controller */
			struct	eec_disk {
				u_char	eec_type;	/* type of this board */
				char	eec_disk_type;	/* controller type */
#define EEC_DISK_TYPE_X450	1
#define EEC_DISK_TYPE_X451	2
				char	eec_disk_ctlr;	/* controller number */
				char	eec_disk_disks;	/* number of disks */
				char	eec_disk_cap[4];	/* capacity */
#define EEC_DISK_NONE	0
#define EEC_DISK_130	1
#define EEC_DISK_280	2
#define EEC_DISK_380	3
#define EEC_DISK_575	4
#define EEC_DISK_900    5
			} eec_disk;

#define	EEC_TYPE_TAPE	0x07			/* 1/2" tape controller */
			struct eec_tape {
				u_char	eec_type;	/* type of this board */
				char	eec_tape_type;	/* controller type */
#define	EEC_TAPE_TYPE_XT	1	/* Xylogics 472 */
#define	EEC_TAPE_TYPE_MT	2	/* TapeMaster */
				char	eec_tape_ctlr;	/* controller number */
				char	eec_tape_drives;/* number of drives */
			} eec_tape;

#define	EEC_TYPE_ETHER	0x08			/* Ethernet controller */

#define	EEC_TYPE_TTY	0x09			/* terminal multiplexer */
			struct eec_tty {
				u_char	eec_type;	/* type of this board */
				char	eec_tty_lines;	/* number of lines */
#define MAX_TTY_LINES	16
				char	manufacturer;	/* code for maker */
#define EEC_TTY_UNKNOWN	0
#define EEC_TTY_SYSTECH	1
#define EEC_TTY_SUN	2
			} eec_tty;

#define	EEC_TYPE_GP	0x0a			/* graphics processor/buffer */
                        struct  eec_gp {
                                u_char  eec_type;       /* type of this board */
                                char    eec_gp_type;
#define EEC_GP_TYPE_GPPLUS	1       /* GP+ graphic processor board */
#define EEC_GP_TYPE_GP2		2       /* GP2 graphic processor board */
                        } eec_gp;

#define	EEC_TYPE_DCP	0x0b			/* DCP ??? XXX */

#define	EEC_TYPE_SCSI	0x0c			/* SCSI controller */
			struct	eec_scsi {
				u_char	eec_type;	/* type of this board */
				char	eec_scsi_type;	/* host adaptor type */
#define EEC_SCSI_SUN2	2
#define EEC_SCSI_SUN3	3
				char	eec_scsi_tapes;	/* number of tapes */
				char	eec_scsi_disks;	/* number of disks */
				char	eec_scsi_tape_type;
#define EEC_SCSI_SYSG	1
#define EEC_SCSI_MT02	2
				char	eec_scsi_disk_type;
#define EEC_SCSI_MD21	1
#define EEC_SCSI_ADAPT	2
				char	eec_scsi_driv_code[2];
#define EEC_SCSI_D71	1
#define EEC_SCSI_D141	2
#define EEC_SCSI_D327   3
			} eec_scsi;
#define EEC_TYPE_IPC	0x0d

#define EEC_TYPE_GB	0x0e

#define EEC_TYPE_SCSI375	0x0f

#define EEC_TYPE_MAPKIT	0x10
			struct	eec_mapkit {
				u_char	eec_type;	/* type of this board */
				char	eec_mapkit_hw	/* whether INI */
#define EEC_TYPE_MAPKIT_INI	1
			} eec_mapkit;

#define EEC_TYPE_CHANNEL	0x11
#define EEC_TYPE_ALM2		0x12


            struct  eec_generic_net {   /* Information array for */ 
                                        /* the gen. Network Controller */ 
#define EEC_TYPE_GENERIC_NET  0x13  
                u_char  eec_devtype;    /* Device type code */ 
                u_char  eec_mem_type;   /* Memory type */ 
                u_char  gn_nu1;         /* For alignment */ 
                u_char  gn_nu2;         /* For alignment */ 
#define ID_VME      0x0                 /* Data buffers are in*/  
                                        /* VME space(Shared)*/ 
#define ID_DVMA     0x1                 /* or DVMA buffer memory type */  
                char    *dev_reg_ptr;   /* Pointer to device */  
                                        /* Registers(Physical)*/

        } eec_generic_net; 


#define	EEC_TYPE_END	0xff			/* end of card cage */
		    } eec_un;
		} eed_conf[MAX_SLOTS+1];
#define EEPROM_TABLE  0x58                    /* 1 indicates alternate key table. */
/* 0x18C */     u_char  which_kbt;      /* which keytable? */
/* 
 * Note. The following contains the full keyboard id returned by 
 * the type4 transmit layout command. It is not necessarily anything to 
 * do with locale of operation.
*/
/* 0x18D */     u_char  ee_kb_type;     
/* 0x18E */     u_char  ee_kb_id;      /* keyboard id in case of EEPROM table */
#define EEPROM_LOGO 0x12
/* 0x18F */     u_char  otherlogo;      /* True if eeprom logo  needed */
/* 0x190 */     struct ee_keymap ee_keytab_lc[1];
/* 0x210 */     struct ee_keymap ee_keytab_uc[1];
/* 0x290 */     u_char    ee_logo[64][8];   /* A 64X64 bit space for custom/OEM                                              * designed logo icon */
/* 0x490 */     struct password_inf ee_password_inf; /* Reserved for SFD   */


/* 0x49c */	char	eed_resv[0x500-0x49c];	/* reserved */
	} ee_diag;

	struct	ee_resv {		/* reserved area of EEPROM */
/* 0x500 */	u_short	eev_wrcnt[3];	/* write count (3 copies) */
		short	eev_nu1;	/* not used */
/* 0x508 */	u_char	eev_chksum[3];	/* reserved area checksum (3 copies) */
		char	eev_nu2;	/* not used */
/* 0x50c */	char	eev_resv[0x600-0x50c];
	} ee_resv;

	struct	ee_rom {		/* ROM area of EEPROM */
/* 0x600 */	u_short	eer_wrcnt[3];	/* write count (3 copies) */
		short	eer_nu1;	/* not used */
/* 0x608 */	u_char	eer_chksum[3];	/* ROM area checksum (3 copies) */
		char	eer_nu2;	/* not used */
/* 0x60c */	char	eer_resv[0x700-0x60c];
	} ee_rom;

	/*
	 * The following area is reserved for software (i.e. UNIX).
	 * The actual contents of this area are defined elsewhere.
	 */
#ifndef EE_SOFT_DEFINED
	struct	ee_softresv {		/* software area of EEPROM */
/* 0x700 */	u_short	ees_wrcnt[3];	/* write count (3 copies) */
		short	ees_nu1;	/* not used */
/* 0x708 */	u_char	ees_chksum[3];	/* software area checksum (3 copies) */
		char	ees_nu2;	/* not used */
#ifdef sun3x
/* 0x70c */	char	ees_resv[0x7d8-0x70c];
#else
/* 0x70c */	char	ees_resv[0x800-0x70c];
#endif sun3x
	} ee_soft;
#else
	struct	ee_soft ee_soft;
#endif
#ifdef sun3x
	/*
	 * IDPROM information is stored here if we don't have a
	 * separate IDPROM. This currently applies to the sun3x/80.
	 */
/* 0x7d8 */
        struct idprom idprom;
        /*
         * This is the TOD for The Mostek MK48T02.
         */
/* 0x7f8 */
        char clk[8];
#endif sun3x
};

#endif /*!_mon_eeprom_h*/
