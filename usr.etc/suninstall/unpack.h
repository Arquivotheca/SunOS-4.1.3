/*
*	unpack.h	"@(#)unpack.h 1.1 92/07/30 SMI"
*/

struct default_desc {
	char *def_name;	  /* name of this directory */
	char *def_desc;	  /* description of system it works with */
    	struct default_desc *def_next; /* next entry */
	unsigned long  def_size;          /* size of this descriptor */
};

struct namelist {		
    char *name;
    char *loadpt;
    long  size;
    struct namelist *next;
};


#define KEYLEN 80
#define DATALEN (256-KEYLEN)

struct desc_datum {
	/*
	 * fixed lengths OK?
	 */
	char dat_key[KEYLEN];
	char dat_data[DATALEN];
};

#define DEFAULTS_DIR "/usr/etc/install/default"
#define DEFAULT_DESCRIPTION_FILE ".desc"
#define INSTALLATION_PROG "installation"
#define CONFIGURATION_PROG "config_host"

#define SMALLER(a,b) ((a)>(b)?(b):(a))
#define GREATER(a,b) ((a)>(b)?(a):(b))

#define YES_NO		1
#define ONE_TWO  	2

#define BASENAME(name,p) if ((p=rindex(name,'/'))==NULL) p=name; else p++;

#define USAGE \
    "usage: %s [ -n ] [ -f ] [ -q ] [ -d ] [ -s var=value ... ] [ template ]\n"

/*
 * these make a bit field describing what we know and what we'd like to know
 */
#define NEED_HOSTNAME   0x01
#define NEED_TIMEZONE   0x02
#define NEED_NETWORK    0x04
#define NEED_IP0        0x08
#define NEED_YP_TYPE    0x10
#define NEED_DOMAINNAME 0x20
#define NEED_TEMPLATE   0x40
#define	NEED_PRESERVE	0x80

#define GOT_PRESERVE_Y	0x400
#define GOT_PRESERVE_N	0x800

#define GOT_YPTYPE_C	0x1000
#define GOT_YPTYPE_N	0x2000

#define GOT_NETWORK_Y   0x4000
#define GOT_NETWORK_N   0x8000

#define NEED_ALL_MASK	0x00ff /* this must change if more fields are added */

/*
 * these are possible OK combinations to not use curses junk at all
 */
#define OK_1 GOT_NETWORK_N
#define OK_2 GOT_NETWORK_N|NEED_IP0
#define OK_3 GOT_NETWORK_N|NEED_IP0|NEED_YP_TYPE
#define OK_4 GOT_NETWORK_N|NEED_IP0|NEED_YP_TYPE|NEED_DOMAINNAME

#define OK_5 GOT_NETWORK_N|GOT_YPTYPE_C
#define OK_6 GOT_NETWORK_N|GOT_YPTYPE_C|NEED_IP0
#define OK_7 GOT_NETWORK_N|GOT_YPTYPE_C|NEED_IP0|NEED_YP_TYPE
#define OK_8 GOT_NETWORK_N|GOT_YPTYPE_C|NEED_IP0|NEED_YP_TYPE|NEED_DOMAINNAME

#define OK_9 GOT_NETWORK_N|GOT_YPTYPE_N|NEED_IP0|NEED_YP_TYPE|NEED_DOMAINNAME
#define OK_10 GOT_NETWORK_N|GOT_YPTYPE_N|NEED_IP0|NEED_YP_TYPE
#define OK_11 GOT_NETWORK_N|GOT_YPTYPE_N|NEED_IP0
#define OK_12 GOT_NETWORK_N|GOT_YPTYPE_N

#define OK_13 GOT_NETWORK_Y|GOT_YPTYPE_N|NEED_DOMAINNAME
#define OK_14 GOT_NETWORK_Y|GOT_YPTYPE_C

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
#define TAB			'\t'		/* forward */

/*
 *	Help Character
 */
#define	QUESTION		'?'		/* help screen */



#define A_NBLOCKS    dk.dka_map[0].dkl_nblk
#define A_START_CYL  dk.dka_map[0].dkl_cylno
#define B_NBLOCKS    dk.dka_map[1].dkl_nblk
#define B_START_CYL  dk.dka_map[1].dkl_cylno
#define C_NBLOCKS    dk.dka_map[2].dkl_nblk
#define C_START_CYL  dk.dka_map[2].dkl_cylno
#define D_NBLOCKS    dk.dka_map[3].dkl_nblk
#define D_START_CYL  dk.dka_map[3].dkl_cylno
#define E_NBLOCKS    dk.dka_map[4].dkl_nblk
#define E_START_CYL  dk.dka_map[4].dkl_cylno
#define F_NBLOCKS    dk.dka_map[5].dkl_nblk
#define F_START_CYL  dk.dka_map[5].dkl_cylno
#define G_NBLOCKS    dk.dka_map[6].dkl_nblk
#define G_START_CYL  dk.dka_map[6].dkl_cylno
#define H_NBLOCKS    dk.dka_map[7].dkl_nblk
#define H_START_CYL  dk.dka_map[7].dkl_cylno

#define BLOCK_SIZE  512
#define DEVICENAME  16
#define CATEGORY_FILE "/etc/install/category.standalone"
#define RE_PREINSTALLED_FILE "/etc/install/category.repreinstalled"
#define LABEL_SCRIPT "/etc/install/label_script"
#define DISK_FILE "/tmp/disk_file"
#define TAPE_DRIVE_TYPE "local"
#define PRESERVE_ALL "all"
#define REDISPLAY	1
#define NOREDISPLAY 	0
#define CONFIRM_B	2

#define NOMENU 0
#define JUSTHELP 1
#define TIME_PAGE 2
#define STIME_PAGE 3
#define NET_PAGE 4
#define IP_PAGE 5
#define NIS_PAGE 6
#define DOMAIN_PAGE 7
#define CONFIRM_PAGE 8
#define USERFN_PAGE 9
#define USERUN_PAGE 10
#define USERUID_PAGE 11
