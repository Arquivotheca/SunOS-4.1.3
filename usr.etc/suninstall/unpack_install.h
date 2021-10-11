/*      @(#)unpack_install.h 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sun/dkio.h>
#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <setjmp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define RELEASE			"4.1"
#define INSTALL_DIR             "/usr/etc/install/files/"
#define INSTALL_SCRIPT_DIR      "/usr/etc/install/script/"
#define INSTALL_TOOL_DIR        "/usr/etc/install/"
#define	LOGFILE			"/usr/etc/install/files/suninstall.log"
#define	HOSTS			"/etc/hosts"
#define	FSTAB			"/etc/fstab"
#define EXPORTS			"/etc/exports"
#define	BOOTPARAMS		"/etc/bootparams"
#define	ETHERS			"/etc/ethers"
#define SYS_INFO		"sys_info"
#define DEFAULT_HOST		"default_host"
#define DEFAULT_CLIENT		"default_client"
#define BINLIST			"/lib/install.bin"	/* relative to /usr */
#define MAXISIZE		1024
#define BUFSIZE			512
#define STRSIZE                 512
#define DOMAINSIZE		256
#define MINSIZE			128


#define BS_HALF			20	/* Block size for 1/2" tapes */
#define BS_QUARTER		126	/* Block size for 1/4" tapes */
#define BS_QUARTER              126     /* Block size for 1/4" tapes */
#define BS_NQUARTER             40      /* Block size for new 1/4" tapes */
#define BS_DISKETTE             18      /* Block size for 31/2" diskettes */
#define BS_DEFAULT              BS_NQUARTER/* Block size for unknown tapes */


/* 
 * Tape drive types
 */
#define	LOCAL			"local"
#define	REMOTE			"remote"

/*
 * Operation types
 */
#define INSTALL			"install"
#define UPGRADE			"upgrade"

/*
 * Terminal types
 */
#define TVI925                  9
#define WYSE_50                 10
#define SUN                     11
#define OTHER                   12

/* 
 * Display units 
 */ 
#define	MBYTES			13
#define KBYTES			14
#define BYTES			15
#define BLOCKS			16
#define CYLINDERS		17

/*
 * Disk label
 */
#define DEFAULT_LABEL		18
#define USE_EXISTING_LABEL	19
#define MOD_EXISTING_LABEL	20
#define LABELSAVED		21

/* 
 * YP types
 */
#define	MASTER			"master"
#define	SLAVE			"slave"
#define	CLIENT			"client"
#define	NONE			"none"

/*
 * Architecture types
 */
#define SUN2			"sun2"
#define	SUN3			"sun3"
#define	SUN3X			"sun3x"
#define	SUN4			"sun4"
#define SUN386			"sun386"

/*
 *	Miscellaneous defines
 */

#define MINIROOT		(strlen(mount_pt))
#define STR_EQ(a,b)		(strcmp((a),(b)) == 0)
#define	fullpath(x,y)	if ((index(x,'/')!= x) && (strcmp(x, NONE))){	\
				(void) fprintf(stderr, 	\
			           "%s: %s arg must begin with /\n", 	\
					progname, x);	\
				y++;	\
			}

/*
 *	Movement commands
 */

#define CONTROL_B		(1 + 'b' - 'a') /* backward */
#define CONTROL_C		(1 + 'c' - 'a') /* interrupt */
#define CONTROL_F		(1 + 'f' - 'a') /* forward */
#define CONTROL_N		(1 + 'n' - 'a') /* forward */
#define CONTROL_P		(1 + 'p' - 'a') /* backward */
#define CONTROL_U		(1 + 'u' - 'a') /* erase word */
#define CONTROL_\		(1 + '\' - 'a') /* repaint screen */
#define SPACE			' ' 		/* forward */
#define RETURN			'\n' 		/* forward */

#define TERMINAL        "                                               \n \
Select your terminal type:                                              \n \
        1) Televideo 925                                                \n \
        2) Wyse Model 50                                                \n \
        3) Sun Workstation                                              \n \
        4) Other                                                        \n \
>> "

#define TERMTYPE        "                                               \n \
Enter the terminal type ( the type must be in /etc/termcap ):           \n \
>> "

#define TIMEZONENAME    "                                               \n \
Enter the local time zone name (type ? if unknown):                     \n \
>> "

#define CHANGETIME      "                                               \n \
Is this the correct date/time [y/n]:                                    \n \
	%.20s%s%s \
>> "

#define DATETIME_MMDDYY "                                               \n \
Enter the current date and local time (e.g. 03/09/88 12:20:30); the date\n \
may be in one of the following formats:    				\n \
        mm/dd/yy                                                        \n \
        mm/dd/yyyy                                                      \n \
        dd.mm.yyyy                                                      \n \
        dd-mm-yyyy                                                      \n \
        dd-mm-yy                                                        \n \
        month dd, yyyy                                                  \n \
        dd month yyyy                                                   \n \
and the time may be in one of the following formats:                    \n \
        hh am/pm                                                        \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm:ss am/pm                                                  \n \
        hh:mm:ss                                                        \n \
        hh.mm.ss am/pm                                                  \n \
        hh.mm.ss                                                        \n \
>> "

#define DATETIME_DDMMYY "                                               \n \
Enter the current date and local time (e.g. 09/03/88 12:20:30); the date\n \
may be in one of the following formats:				   	\n \
        dd/mm/yy                                                        \n \
        dd/mm/yyyy                                                      \n \
        dd.mm.yyyy                                                      \n \
        dd-mm-yyyy                                                      \n \
        dd-mm-yy                                                        \n \
        month dd, yyyy                                                  \n \
        dd month yyyy                                                   \n \
and the time may be in one of the following formats:                    \n \
        hh am/pm                                                        \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm:ss am/pm                                                  \n \
        hh:mm:ss                                                        \n \
        hh.mm.ss am/pm                                                  \n \
        hh.mm.ss                                                        \n \
>> "

#define DATETIMEOK      "                                               \n \
Is this the correct date/time [y/n]:                                    \n \
        %.20s%s%s \
>> "

struct system_info {
	char	hostname[STRSIZE];	/* hostname of interface 0 */
	char	hostname1[STRSIZE];	/* hostname of interface 1 */
	char	arch[MINSIZE];		 /* sun2, sun3, etc. */
	char	timezone[STRSIZE];	 /* timezone name */
	char	type[25];		 /* standalone, server or dataless */
	char	ip0[MINSIZE];		 /* internet address ethertype unit 0 */
	char    ip1[MINSIZE];            /* internet address ethertype unit 1 */
	char	ether_type0[10];	 /* ec0, ie0, or le0 */
	char    ether_type1[10];         /* ec1, ie1, or le1 */
	char	yp_type[25];		 /* master, slave, client or none */
	char	domainname[DOMAINSIZE];	 /* domainname */
	char	root[MINSIZE];		 /* root disk partition */
	char	user[MINSIZE];		 /* user disk partition */
	char	kvm[MINSIZE];		 /* where the kvm lives */
	char	server[STRSIZE];	 /* for dataless configuration */
	char 	serverip[MINSIZE];       /* for dataless configuration */
	char	op[25];			 /* install or upgrade */
        char    termtype[MINSIZE];       /* entry in /etc/termcap */
	char	reboot[10];		 /* reboot or noreboot */
	char    rewind[10];              /* rewind or norewind */
	char	dataless_kvm[MINSIZE];	 /* partial kvm path for dataless */
	char	dataless_exec[MINSIZE];	 /* partial exec path for dataless */
};

struct server_info {
	char 		hostname[STRSIZE];
	struct hostent	hostent;
        char		arch[25];
	char		proto_path[STRSIZE]; 	
	char		swap_path[STRSIZE];
	char		exec_path[STRSIZE];
	char   		kvm_path[STRSIZE];
	char   		home_path[STRSIZE];
	char   		mail_path[STRSIZE];
	char   		share_path[STRSIZE];
	char   		crash_path[STRSIZE];
};

struct client_info {
	char    hostname[STRSIZE];
        char	arch[25];		 /* sun2, sun3, etc. */
        char    ip[MINSIZE];             /* internet address */
        char    ether[MINSIZE];          /* ethernet address */
	char	swap_size[25];	 	 /* bytes, Kbytes, Mbytes or blocks */
	char	root_path[STRSIZE]; 	
	char	swap_path[STRSIZE];
	char	home_path[STRSIZE];
	char	exec_path[STRSIZE];
	char	kvm_path[STRSIZE];
	char	mail_path[STRSIZE];
	char	share_path[STRSIZE];
	char	crash_path[STRSIZE];
	char	yp_type[25];  	 	 /* master, slave, client or none */
        char    domainname[STRSIZE];     /* domainname */
};

struct loc {
        int x;
        int y;
        char str[MINSIZE];
        char field[25];
};
