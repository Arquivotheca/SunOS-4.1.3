/* @(#)install.h 1.1 92/07/30 SMI */

/*
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _INSTALL_H_
#define	_INSTALL_H_

/*
 *	Name:		install.h
 *
 *	Description:	This file contains the global declarations necessary
 *		for the suninstall programs.
 */

/*
 *	Only #include files that are necessary for this file to compile.
 */
#include <sys/param.h>
#include <sun/dkio.h>


/*
 *	Global constants:
 */

/*
 *	Installation methods
 */
#define RE_PREINSTALLED         0
#define	EASY_INSTALL		1
#define	MANUAL_INSTALL		2
#define	EXIT_INSTALL		3


/*
 *	Answer types:
 */
#define	ANS_NO			0
#define	ANS_YES			1

/*
 *	Architecture types:
 *
 *	Note:	The architectures must be numbered from 1 to ARCH_MAX.
 *		Update lib/cv_arch.c if an architecture is added.
 */
#ifndef SunB1
#	define ARCH_SUN2	1
#	define ARCH_SUN3	2
#	define ARCH_SUN3X	3
#	define ARCH_SUN4	4
#	define ARCH_SUN386	5
#	define ARCH_SUN4C	6
#	define ARCH_SUN4M	7
#	define ARCH_MAX		7
#else
#	define ARCH_SUN3	1
#	define ARCH_SUN3X	2
#	define ARCH_SUN4	3
#	define ARCH_SUN386	4
#	define ARCH_SUN4C	5
#	define ARCH_SUN4M	6
#	define ARCH_MAX		6
#endif /* SunB1 */



/*
 *	Client choice codes:
 */
#define	CLNT_CREATE		1
#define	CLNT_DELETE		2
#define	CLNT_DISPLAY		3
#define	CLNT_EDIT		4

#define	DISKS_PER_COL		6		/* # disks per column - 1 */
#define	DISK_WIDTH		10		/* disk column width */

#define	ARCHS_PER_COL		2		/* # arches per column - 1 */
#define	ARCHS_WIDTH		20		/* disk column width */

/*
 *	Disk label:
 */
#define	DKL_DEFAULT		1
#define	DKL_EXISTING		2
#define	DKL_MODIFY		3
#define	DKL_SAVED		4

/*
 *	Display units :
 */
#define	DU_BLOCKS		1
#define	DU_BYTES		2
#define	DU_CYLINDERS		3
#define	DU_KBYTES		4
#define	DU_MBYTES		5

/*
 *	Maximum disk devices
 */
#define	MAX_SD_DISKS		20
#define	MAX_XY_DISKS		4
#define	MAX_XD_DISKS		16
#define	MAX_ID_DISKS		64


/*
 *	ifconfig() access check types
 */
#define	IFCONFIG_MOUNT		1
#define	IFCONFIG_RSH		2


/*
 *	Ethernet choices:
 *	(in ether_type)
 */
#define	ETHER_0			0
/* for ETHER_n use ETHER_0 + n */
#define	ETHER_1			1
#define	ETHER_NONE		-1
#define	MAX_ETHER_INTERFACES	16

/*
 *	Ethernet types:
 *
 *	Note:	The ethernet types must be numbered from 1 to ETHER_MAX.
 *		Update lib/cv_ether.c if a ethernet type is added.
 */
#define	ETHER_IE		1
#define	ETHER_LE		2
#define	ETHER_MAX		2

/*
 *	Installable media flags:
 */
#define	IFLAG_COMM		1
#define	IFLAG_DES		2
#define	IFLAG_OPT		3
#define	IFLAG_REQ		4

/*
 *	Media file kinds:
 */
#define	KIND_UNDEFINED		0
#define	KIND_CONTENTS		1
#define	KIND_AMORPHOUS		2
#define	KIND_STANDALONE		3
#define	KIND_EXECUTABLE		4
#define	KIND_INSTALLABLE	5
#define	KIND_INSTALL_TOOL	6

#ifdef SunB1
/*
 *	Label types for SunB1.  LAB_SYS_LOW must be the lowest value and
 *	LAB_SYS_HIGH must be the highest value.
 */
#define	LAB_SYS_LOW		1
#define	LAB_OTHER		2
#define	LAB_SYS_HIGH		3
#endif /* SunB1 */


#define	MEDIUM_STR		100
#define	LARGE_STR		128

/*
 *	Media location types:
 */
#define	LOC_LOCAL		1
#define	LOC_REMOTE		2

#define	MAX_AUDIT_DIRS		8		/* max # of audit dirs */
#define	MAXDOMAINNAMELEN	256		/* max length of domain name */



/*
 **************************************************************************
 *
 *           	  ******  HOW BIG IS A MEGABYTE? ******
 *
 * 		ALL SUNINSTALL ENGINEERS MUST READ THIS.
 * 
 **************************************************************************
 *	
 *	Disk Manufacturers, in order to make their disks appear bigger than
 *	they really are, chose the convention of assuming that a Megabyte
 *	meant 1,000,000 (0xF4240) bytes, instead of its true meaning of
 *	1,048,576 (0x100000) bytes.
 *
 *	Anyway, because of this definition, all the hard disks are
 *	advertized as having XXX 1,000,000 bytes (disk industry megabytes),
 *	hence inflating the true space on the disk.  When customers used the
 *	suninstall disk form, they saw the "true megabyte" space on the disk
 *	and, being a smaller number, thought they were cheated and called
 *	the Sun US Answer Center.
 *
 *	Now, wishing to reduce calls in the Sun U.S. Answer Center, we (the
 *	suninstall engineers) have been instructed to use the Disk
 *	Manufacturer convention, using the megabyte of 1,000,000 bytes and
 *	not the true 1,048,576 bytes. So, we did.....
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	
 *	Now, a word about implentation.
 *
 *	We have introduced what the Macro DI_MEGABYTE, which is the Disk
 *	Industry Megabyte.
 *	
 *	All other disk size measurements, such as a kilobyte (1024 bytes) a
 *	block (512 bytes) and the like, are untouched.  Only the value of a
 *	MEGABYTE is changing.  AND this is only in the user interface, not
 *	the real workings.  It can't apply to the real workings like disk
 *	labeling, because a computers really work on power of 2's (which is
 *	the basis of the real MEGABYTE of 0x100000 (2^20) bytes).
 *
 *	add_client will still continue to use the MEGABYTE and not the
 *	DI_MEGABYTE, as was done in 4.0.3 for swap sizes.
 *
 *	In order to revert back to the true MEGABYTE for the user interface,
 *	all one must do is comment out the macro #define DI_MEGABYTE.
 *
 * 	WARNING  : to all future suninstall engineers.. DO NOT comment out
 * 		   the DI_MEGABYTE macro, UNTIL you have explicit
 *		   permission from superiors on the OS engineering team.
 *	
 * Implementation date : Tue Nov 14 1989
 *
 ****************************************************************************
 */ 		   


#define	MEGABYTE		0x100000L	/* one megabyte */

#define	DI_MEGABYTE		0xF4240L	/* Disk Industry Megabyte */

/*
 *	In keeping with integer division and multiplication, we always want
 *	to round up here to in conversions because rounding down will create
 *	unwanted zeroing in the lower meg numbers. An additional 1 meg in
 *	higher meg #'s is not big deal, except if we hit a 0.
 */

#ifdef DI_MEGABYTE
#	define DI_to_Real_Meg(a)   \
			(((a) == 0) ? 0 : (((a) * DI_MEGABYTE)/MEGABYTE) + 1)

#	define Real_to_DI_Meg(a)	((((a) * MEGABYTE)/DI_MEGABYTE))
#else
#	define DI_to_Real_Meg(a)	(a)
#	define Real_to_DI_Meg(a)	(a)
#endif


/*
 *	File system minimums:
 */
#define	MIN_USR			(45 * MEGABYTE)
#define	MIN_CLIENT		(MIN_XROOT + MIN_XSWAP)
#define	MIN_XROOT		(5 * MEGABYTE)
#define	MIN_XSWAP		(16 * MEGABYTE)

#define	N_CLIENTS		2		/* default # of clients */


/*
 *	Small disk size definition
 */

#define	SMALL_DSK_BYTES		(130 * MEGABYTE)

/*
 *	Partition defaults
 */

#define	TWO_PARTS		2
#define	THREE_PARTS		3


/*
 *	Operation types:
 */
#define	OP_INSTALL		1
#define	OP_UPGRADE		2

#define	SMALL_STR		32

/*
 *	Software choices
 */
#define	SOFT_ALL		1
#define	SOFT_DEF		2
#define	SOFT_OWN		3
#define	SOFT_REQ		4
#define	SOFT_SAVED		5

/*
 *	Software operations
 */
#define	SOFT_ADD		1
#define	SOFT_EDIT		2
#define	SOFT_REMOVE		3

/*
 *	System types:
 */
#define	SYS_STANDALONE		1
#define	SYS_SERVER		2
#define	SYS_DATALESS		3

#define	TINY_STR		16
#define	IDENTLEN		32
#define	UMASK			022		/* value for umask(2) */

/*
 *	NIS types:
 */
#define	YP_MASTER		1
#define	YP_SLAVE		2
#define	YP_CLIENT		3
#define	YP_NONE			4


/*
 *      add_key_entry()   replacement parameters
 */
#define KEY_ONLY                0       /* match on key only */
#define KEY_OR_VALUE            1       /* match on key or value */




/*
 *	Global string constants:
 */
extern	char		APPL_MEDIA_FILE[];
extern	char		ARCH_INFO[];
extern	char		ARCH_LIST[];
extern	char		AUDIT_CONTROL[];
extern	char		AUDIT_DIR[];
extern	char		AVAIL_ARCHES[];
extern	char		BOOTPARAMS[];
extern	char		CLIENT_LIST[];
extern	char		CLIENT_LIST_ALL[];
extern	char		CLIENT_STR[];
extern	char		CMDFILE[];
extern	char		DEFAULT_CLIENT_INFO[];
extern	char		DEFAULT_FS[];
extern	char		DEFAULT_NET_FS[];
extern	char		DEFAULT_SYS_INFO[];
extern	char		DEPENDENT_LIST[];
#ifdef SunB1
extern	char		DEVICE_CLEAN[];
extern	char		DEVICE_GROUPS[];
#endif /* SunB1 */
extern	char		DISK_INFO[];
extern	char		DISK_LIST[];
extern	char		EASYINSTALL[];
extern	char		ETHERS[];
extern	char		EXCLUDELIST[];
extern	char		EXPORTS[];
extern	char		FILE_DIR[];
extern	char		FSTAB[];
extern	char		FSTAB_TMP[];
extern	char		HOME[];
extern	char		HOSTS[];
#ifdef SunB1
extern	char		HOSTS_LABEL[];
#endif /* SunB1 */
extern	char		INETD_CONF[];
extern	char		INSTALL_BIN[];
extern	char		INSTALL_TAR_DIR[];
extern	char		LOAD_ARCH[];
extern	char		LOGFILE[];
extern	char		MEDIA_FILE[];
extern	char		MEDIA_LIST[];
extern	char		MOUNT_LIST[];
#ifdef SunB1
extern	char		NET_LABEL[];
#endif /* SunB1 */
extern	char		PASSWD[];
extern	char		PROTO_ROOT[];
extern	char		PWD_ADJUNCT[];
extern	char		RC_DOMAINNAME[];
extern	char *		RC_HEADER[];
extern	char		RC_HOSTNAME[];
extern	char		RC_IFCONFIG[];
extern	char		RC_NETMASK[];
extern	char		RC_RARPD[];
extern	char		RELEASE[];
extern	char		RMP_EXPORTS[];
extern	char		RMP_FILE_DIR[];
extern	char		RMP_HOSTS[];
#ifdef SunB1
extern	char		RMP_HOSTS_LABEL[];
extern	char		RMP_NET_LABEL[];
extern	char		RMP_SEC_DEV_DIR[];
#endif /* SunB1 */
extern	char		RMP_TTYTAB[];
extern	char		ROOT_MNT_PT[];
#ifdef SunB1
extern	char		SEC_DEV_DIR[];
#endif /* SunB1 */
extern	char		SEC_INFO[];
extern	char		SOFT_INFO[];
extern	char		STD_DEVS[];
extern	char		SYS_INFO[];
extern	char		TOOL_DIR[];
extern	char		TTYTAB[];
extern	char		XDRTOC[];
extern	char		ZONEINFO[];
extern	char		ZONEINFO_PATH[];

extern	char		STD_EXEC_PATH[];
extern	char		STD_KVM_PATH[];
extern	char		STD_SHARE_PATH[];




/*
 *	Global data structure definitions:
 */

typedef int			boolean;

/*
 *	Do the typedefs up front so the data structures can be in
 *	alphabetical order.
 */
typedef struct arch_info_t	arch_info;
typedef struct clnt_info_t	clnt_info;
typedef struct conv_t		conv;
typedef struct disk_info_t	disk_info;
typedef struct key_xlat_t	key_xlat;
typedef struct media_file_t	media_file;
typedef struct mnt_ent_t	mnt_ent;
typedef struct part_info_t	part_info;
typedef struct sec_info_t	sec_info;
typedef struct soft_info_t	soft_info;
typedef struct sys_info_t	sys_info;
typedef struct preserve_info_t	preserve_info;

/*
*	os_ident
*
*	a particular os identification
*/
struct os_ident {
	int  special_release;		/* if true, no sharing applies */
	char os_name[IDENTLEN];		/* sunos */
	char appl_arch[IDENTLEN];	/* sun3  */
	char impl_arch[IDENTLEN];	/* sun3x */
	char release[IDENTLEN];		/* 4.0.3BETA */
	char realization[IDENTLEN];	/* _PSR_A-ALPHA */
};
typedef struct os_ident		Os_ident;

/*
*	os_info
*
*	characterizes a particular os
*/
struct os_info {
	Os_ident os_ident;
	char exec_path[MAXPATHLEN];	/* /export/exec/arch */
	char kvm_path[MAXPATHLEN];	/* /export/exec/kvm/arch */
	char share_path[MAXPATHLEN];	/* /export/share */
};
typedef struct os_info Os_info;



/*
 *	Definition of an architecture information structure:
 *
 *		arch		- the architecture
 *		arch_str	- the architecture arpid
 *		exec_path	- path to executables
 *		kvm_path	- path to kernel executables
 *		next		- pointer to the next element
 */
struct arch_info_t {
	char		arch_str[MEDIUM_STR];
	Os_ident	os;
	struct arch_info_t *next;
};


/*
 *	Definition of a client information structure:
 *
 *		hostname	- name of the client
 *		arch		- architecture of the client
 *		choice		- client command choice
 *		ip		- internet address of the client
 *
 *		ip_minlab	- minimum label for ip (SunB1)
 *		ip_maxlab	- maximum label for ip (SunB1)
 *
 *		ether		- ethernet address of the client
 *		swap_size	- swap size of the client
 *		root_path	- path to root directory
 *		swap_path	- path to swap area
 *		home_path	- path to home directory
 *		exec_path	- path to executables
 *		kvm_path	- path to kernel executables
 *		mail_path	- path to mail area
 *		share_path	- path to shared area
 *		crash_path	- path to crash dump area
 *		yp_type		- type of Network Information Services
 *		domainname	- name of the NIS domain
 *		created		- has the client been created?  This is used
 *				  by calc_client() calls during multi-user
 *				  add_client operations
 */
struct clnt_info_t {
	char		hostname[MAXHOSTNAMELEN];
	int		arch;
	char		arch_str[MEDIUM_STR];
	int		choice;
	char		ip[SMALL_STR];
#ifdef SunB1
	int		ip_minlab;
	int		ip_maxlab;
#endif /* SunB1 */
	char		ether[SMALL_STR];
	char		swap_size[SMALL_STR];
	char		root_path[MAXPATHLEN];
	char		swap_path[MAXPATHLEN];
	char		home_path[MAXPATHLEN];
	char		exec_path[MAXPATHLEN];
	char		kvm_path[MAXPATHLEN];
	char		mail_path[MAXPATHLEN];
	char		share_path[MAXPATHLEN];
	char		crash_path[MAXPATHLEN];
	int		yp_type;
	char		domainname[MAXDOMAINNAMELEN];
	int		created;
	char		termtype[SMALL_STR];
	int		small_kernel;
};


/*
 *	Definition of a conversion structure:
 *
 *		conv_text	- conversion text
 *		conv_value	- conversion value
 */
struct conv_t {

	char *		conv_text;
	int		conv_value;
};


/*
 *	Definition of a key translation struct
 *
 *		key_name	- name of the key
 *		key_func	- function to translate the key value
 *				  into a code value
 *		code_func	- function to translate a code value
 *				  into a key value
 *		data_p		- pointer to the key value or
 *				  pointer to the code value
 */
struct key_xlat_t {
	char *		key_name;
	int		(*key_func)();
	char *		(*code_func)();
	char *		data_p;
};


/*
 *	Definition of a media file structure:
 *
 *		media_no	- media number
 *		file_no		- file number
 *		mf_name		- media file's name
 *		mf_select	- select the media file?
 *		mf_loaded	- media file already loaded?  This field is
 *				  used by calc_software() during multi-user
 *				  add_services operations.
 *		mf_size		- media file's size
 *		mf_kind		- kind of media file
 *		mf_iflag	- flag for INSTALLABLE kind
 *		mf_loadpt	- media file's load point
 *		mf_depcount	- number of dependencies
 *		mf_deplist	- list of dependents
 *		mf_depsel	- selected because it is a dependent
 *		mf_type		- type of media file (e.g., TAR)
 */
struct media_file_t {
	int		media_no;
	int		file_no;
	char *		mf_name;
	int		mf_select;
	int		mf_loaded;
	long		mf_size;
	int		mf_kind;
	int		mf_iflag;
	char *		mf_loadpt;
	int		mf_depcount;
	char **		mf_deplist;
	int		mf_depsel;
	int		mf_type;
};


/*
 *	Definitions of a mount entry:
 *
 *		partition	- name of the partition
 *		mount_pt	- pathname of the the mount point
 *		preserve	- preserve flag
 *
 *		fs_minlab	- file system minimum label (SunB1)
 *		fs_maxlab	- file system maximum label (SunB1)
 *
 *		count		- # of path elements
 */
struct mnt_ent_t {
	char	partition[TINY_STR];
	char	mount_pt[MAXPATHLEN];
	char	preserve;
#ifdef SunB1
	int		fs_minlab;
	int		fs_maxlab;
#endif /* SunB1 */
	int	count;
};


/*
 *	Definition of a partition information structure:
 *
 *		start_str	- starting cylinder string
 *		block_str	- block count string
 *		size_str	- size in display units string
 *		mount_pt	- mount point string
 *		preserve_flag	- preserve disk contents answer
 *		map_buf		- disk map buffer
 *		avail_bytes	- number of available bytes
 *
 *		fs_minlab	- file system minimum label (SunB1)
 *		fs_maxlab	- file system maximum label (SunB1)
 */
struct part_info_t {
	char		start_str[SMALL_STR];
	char		block_str[SMALL_STR];
	char		size_str[SMALL_STR];
	char		mount_pt[MAXPATHLEN];
	char		preserve_flag;
	struct dk_map	map_buf;
	long		avail_bytes;
#ifdef SunB1
	int		fs_minlab;
	int		fs_maxlab;
#endif /* SunB1 */
};


/*
 *	Definition of a disk information structure:
 *
 *		disk_name	- name of this disk
 *		label_source	- where to get the disk label from
 *		free_hog	- free hog partition
 *		display_unit	- which units to display in
 *		is_hot		- we are running on this disk
 *		swap_size	- size of swap on a hot disk
 *		geom_buf	- disk geometry buffer
 *		partitions	- the disk partitions
 *
 *		disk_minlab	- disk minimum label (SunB1)
 *		disk_maxlab	- disk maximum label (SunB1)
 *
 *	This definition must appear after part_info.
 */
struct disk_info_t {
	char		disk_name[TINY_STR];
	int		label_source;
	int		free_hog;
	int		display_unit;
	int		is_hot;
	long		swap_size;
	struct dk_geom	geom_buf;
	part_info	partitions[NDKMAP];
#ifdef SunB1
	int		disk_minlab;
	int		disk_maxlab;
#endif /* SunB1 */
};


/*
 *	Definition of a security information structure
 *
 *		hostname	- name of this host
 *		root_word	- root user password
 *		root_ck		- buffer for checking root user password
 *		audit_word	- audit user password
 *		audit_ck	- buffer for checking audit user password
 *		audit_flags	- audit flags value for AUDIT_CONTROL
 *		audit_min	- min-free value for AUDIT_CONTROL
 *		audit_dirs	- audit directories for AUDIT_CONTROL
 */
struct sec_info_t {
	char		hostname[MAXHOSTNAMELEN];
	char		root_word[TINY_STR];
	char		root_ck[TINY_STR];
	char		audit_word[TINY_STR];
	char		audit_ck[TINY_STR];
	char		audit_flags[LARGE_STR];
	char		audit_min[3];
	char		audit_dirs[MAX_AUDIT_DIRS][MAXPATHLEN];
};


/*
 *	Definition of a software information structure
 *
 *		arch		- current architecture
 *		arch_str	- architecture string.  If the size of this
 *				  field changes, then the local buffer must
 *				  change in lib/media_file.c.
 *		operation	- architecture operation
 *		exec_path	- current path to executables
 *		kvm_path	- current path to kernel executables
 *		share_path	- path to shared area
 *		choice		- software choice
 *		media_type	- generic media type, see MEDIAT_xxx
 *		media_flags	- (unused) space for media type flags
 *		media_dev	- token for media device, used in menus, etc.
 *		media_path	- full path to media device
 *		media_loc	- media location
 *		media_host	- media host name
 *		media_ip	- media host's IP addr
 *		media_vol	- media volume number
 *		media_count	- number of media files
 *		media_files	- the files we are interested in
 */
struct soft_info_t {
	int		arch;
	char		arch_str[MEDIUM_STR];
	int		operation;
	char		exec_path[MAXPATHLEN];
	char		kvm_path[MAXPATHLEN];
	char		share_path[MAXPATHLEN];
	int		choice;
	int		media_type;
	int		media_flags;
	int		media_dev;
	char		media_path[MAXPATHLEN];
	int		media_loc;
	char		media_host[MAXHOSTNAMELEN];
	char		media_ip[SMALL_STR];
	int		media_vol;
	int		media_count;
	media_file *	media_files;
};


/*
 *	Definition of a system information structure:
 *
 *		sys_type	- standalone, server or dataless
 *		ether_type	- displayed ethernet type
 *
 *		ethers		- array of ethernet interfaces, including
 *				  the hostname, ip address, interface name
 *		ip0_minlab	- minimum label for ip0 (SunB1)
 *		ip0_maxlab	- maximum label for ip0 (SunB1)
 *
 *		ip1_minlab	- minimum label for ip1 (SunB1)
 *		ip1_maxlab	- maximum label for ip1 (SunB1)
 *
 *		yp_type		- master, slave, client or none.  This field
 *				  is not used if 'ether_type' is ETHER_NONE.
 *		domainname	- domainname.  This field is not used if
 *				  'ether_type' is ETHER_NONE.
 *		op_type		- install or upgrade
 *		reboot		- reboot or noreboot
 *		rewind		- rewind or norewind
 *		arch		- sun2, sun3, etc.
 *		root		- root disk partition
 *		user		- user disk partition
 *		termtype	- entry in /etc/termcap
 *		timezone	- timezone name.  This field is not used
 *				  by a 'dataless' configuration.
 *		server		- for dataless configuration.  This field is
 *				  only used by a 'dataless configuration.
 *		server_ip	- for dataless configuration.  This field is
 *				  only used by a 'dataless configuration.
 *		kvm_path	- where the kvm lives.  This field is only
 *				  used by a 'dataless' configuration.
 *		exec_path	- partial exec path for dataless.  This field
 *				  is only used by a 'dataless configuration.
 */

struct ether_interface {
	char		hostname[MAXHOSTNAMELEN];
	char		interface_name[TINY_STR];
	char		internet_addr[SMALL_STR];
#ifdef SunB1
	int		ip_minlab;
	int		ip_maxlab;
#endif
};

struct sys_info_t {
	int		sys_type;
	int		ether_type;
	struct ether_interface ethers[MAX_ETHER_INTERFACES];

#define	hostnamex(i) ethers[i].hostname
#define	ether_namex(i) ethers[i].interface_name
#define	ipx(i) ethers[i].internet_addr

/*
** Name the first two interfaces -- for historical reasons
*/

#define	hostname0 hostnamex(0)
#define	hostname1 hostnamex(1)
#define	ether_name0 ether_namex(0)
#define	ether_name1 ether_namex(1)
#define	ip0 ipx(0)
#define	ip1 ipx(1)

#ifdef SunB1
#define	ip0_minlab ethers[0].ip_minlab
#define	ip1_minlab ethers[1].ip_minlab
#define	ip0_maxlab ethers[0].ip_maxlab
#define	ip1_maxlab ethers[1].ip_maxlab
#endif /* SunB1 */

	int		yp_type;
	char		domainname[MAXDOMAINNAMELEN];
	int		op_type;
	int		reboot;
	int		rewind;

	int		arch;
	char		arch_str[MEDIUM_STR];
	char		root[SMALL_STR];
	char		user[SMALL_STR];
	char		termtype[SMALL_STR];

	char		timezone[MEDIUM_STR];

	char		server[MAXHOSTNAMELEN];
	char 		server_ip[SMALL_STR];
	char		kvm_path[MAXPATHLEN];
	char		exec_path[MAXPATHLEN];
	int		static_sizing;
};

/*
 *      This structure holds a pointer to the disk pointer
 *      and an index to the partition.  Its not perfect but its
 *      the easiest way I see to pass the extra information I need
 */

struct preserve_info_t {
	char **	params;
	int	part;
	char   *preserve;
};

/*
 *	External functions:
 */
extern	char *		malloc();
extern	char *		sprintf();
extern	char *		strcpy();


extern	void		_log();
extern	void		add_key_entry();
#ifdef SunB1
extern	void		add_net_label();
#endif /* SunB1 */
extern	char *		appl_arch();
extern	char *		aprid_to_aid();
extern	char *		aprid_to_arid();
extern	char *		aprid_to_execpath();
extern	char *		aprid_to_iid();
extern	char *		aprid_to_irid();
extern	char *		aprid_to_kvmpath();
extern	char *		aprid_to_rid();
extern	char *		aprid_to_sharepath();
extern	char *		aprid_to_syspath();
extern	int		arch_major();
extern	int		arch_minor();
#ifdef SunB1
extern	int		at_sys_high();
#endif /* SunB1 */
extern	char *		basename();
extern	char *		blocks_to_str();
extern	int		calc_client();
extern	int		calc_disk();
extern	int		calc_software();
extern	int		ck_remote_path();
extern	int		check_client_terminal();
extern	int		check_remote_access();
extern	void		copy_install_bin();
extern	void		copy_tree();
extern	char *		cv_ans_to_str();
extern	char *		cv_arch_to_str();
extern	char *		cv_char_to_str();
extern	char *		cv_cpp_to_str();
extern	char *		cv_ether_to_str();
extern	char *		cv_iflag_to_str();
extern	char *		cv_int_to_str();
extern	char *		cv_kind_to_str();
#ifdef SunB1
extern	char *		cv_lab_to_str();
#endif /* SunB1 */
extern	char *		cv_long_to_str();
extern	char *		cv_media_to_str();
extern	int		cv_str_to_ans();
extern	int		cv_str_to_arch();
extern	int		cv_str_to_char();
extern	int		cv_str_to_cpp();
extern	int		cv_str_to_ether();
extern	int		cv_str_to_iflag();
extern	int		cv_str_to_int();
extern	int		cv_str_to_kind();
#ifdef SunB1
extern	int		cv_str_to_lab();
#endif /* SunB1 */
extern	int		cv_str_to_long();
extern	int		cv_str_to_media();
extern	int		cv_str_to_type();
extern	int		cv_str_to_yp();
extern	long		cv_swap_to_long();
extern	char *		cv_type_to_str();
extern	char *		cv_yp_to_str();
extern	int		delete_client_from_list();
extern	void		delete_blanks();
extern	char *		default_release();
extern	char *		dirname();
extern	int		disk_to_mount_list();
extern	int		elem_count();
extern	char *		err_mesg();
extern	int		execute();
extern	char *		extract_str();
extern	arch_info *	find_arch();
extern	char *		find_mf_part();
extern	char *		find_part();
extern	int		fill_os_ident();
#ifdef SunB1
extern	void		fix_devgroup();
#endif /* SunB1 */
extern	void		fix_passwd();
extern	void		free_arch_info();
extern	void		free_media_file();
extern	char *		get_arch();
extern	int		get_disk_config();
extern 	void		get_ethertypes();
extern	void		get_stdin();
extern	void		get_terminal();
#ifdef SunB1
extern	void		golabeld();
#endif /* SunB1 */
extern	void		ifconfig();
extern	char *		irid_to_aprid();
extern	int		is_server();
extern	int		is_miniroot();
extern	int		is_sec_loaded();
extern	void		log();
extern	void		MAKEDEV();
extern	int		make_mount_list();
extern	int		media_block_size();
extern	char *		media_dev_name();
extern	int		media_extract();
extern	int		media_fsf();
extern	int		media_read_file();
extern	int		media_rewind();
extern	int		menu_check_remote_access();
extern	void		menu_log();
extern	void		mk_localtime();
extern	void		mk_rc_domainname();
extern	void		mk_rc_hostname();
extern	void		mk_rc_ifconfig();
extern	void		mk_rc_netmask();
extern	void		mk_rc_rarpd();
extern	void		mkdir_path();
#ifdef SunB1
extern	void		mkldir_path();
#endif /* SunB1 */
extern	void		mklink();
extern	char *		mount_string();
extern	int		none_selected();
extern	char *		os_aprid();
extern	char *		os_arid();
extern	char *		os_ident_token();
extern	char *		os_name();
extern	char *		os_irid();
extern	void		please_check();
extern	char *		proto_root_path();
extern	void		rarpd();
extern	int		read_arch_info();
extern	int		read_clnt_info();
extern	int		read_disk_info();
extern	int		read_file();
extern	int		read_media_file();
extern	int		read_mount_list();
extern	int		read_sec_info();
extern	int		read_soft_info();
extern	int		read_sys_info();
extern	void		replace_key_entry();
extern	int		required_software();
extern	void		replace_value_entry();
extern	int		replace_media_file();
extern	void		reset_selected_media();
extern	boolean		same_appl_arch();
extern	boolean		same_arch_pair();
extern	boolean		same_impl_arch();
extern	boolean		same_os();
extern	boolean		same_realization();
extern	boolean		same_release();
extern	int		save_arch_info();
extern	int		save_clnt_info();
extern	int		save_cmdfile();
extern	int		save_disk_info();
extern	int		save_media_file();
extern	int		save_mount_list();
extern	int		save_sec_info();
extern	int		save_soft_info();
extern	int		save_sys_info();
extern	void		sig_trap();
extern	void		sort_mount_list();
extern	char *		std_cd_path();
extern	char *		std_execpath();
extern	char *		std_kvmpath();
extern	char *		std_sharepath();
extern	char *		std_syspath();
extern	int		suser();
extern	boolean		sys_has_release();
extern	int		toc_xlat();
extern	void		tune_audit();
extern	int		update_arch();
extern	int		update_bytes();
extern	void		update_parts();
extern	void		update_yp();
extern	char *		xlat_code();
extern	int		xlat_key();
extern	void		x_chdir();
extern	void		x_system();




/*
 *	External variables:
 */
#ifdef SunB1
extern	int		old_mac_state;		/* old mac-exempt state */
#endif /* SunB1 */
extern	char *		progname;


/*
 *	Macro functions:
 */

/*
 *	Convert kilobytes to blocks:
 *
 *		k - # of kilobytes
 *		g - disk geometry
 *
 *	Rounds number of kilobytes upto the next cylinder boundary.
 */
#define	Kb_to_blocks(k, g)	(blocks_to_cyls((k) * 2, g) \
				 * g.dkg_nhead * g.dkg_nsect)

/*
 *	Convert megabytes to blocks:
 *
 *		m - # of megabytes
 *		g - disk geometry
 *
 *	Rounds number of megabytes upto the next cylinder boundary.
 */
#define	Mb_to_blocks(m, g)	(blocks_to_cyls((m) * 2048, g) \
				 * g.dkg_nhead * g.dkg_nsect)

/*
 *	Convert blocks to kilobytes:
 *
 *		b - number of blocks
 *
 *	Divide by 2 since 'b' is the number of 512 byte blocks.
 */
#define	blocks_to_Kb(b)		((b) / 2)

/*
 *	Convert blocks to megabytes:
 *
 *		b - number of blocks
 *
 *	Divide by 2048 since 'b' is the number of 512 byte blocks.
 */
#define	blocks_to_Mb(b)		((b) / 2048)

/*
 *	Convert blocks to bytes:
 *
 *		b - number of blocks
 *
 *	There are 512 bytes in a block.
 */
#define	blocks_to_bytes(b)	((b) * 512)

/*
 *	Convert blocks to cylinders:
 *
 *		b - # of blocks
 *		g - disk geometry
 *
 *	Rounds the number of blocks upto the next cylinder boundary.
 */
#define	blocks_to_cyls(b, g)	(((b) + ((g.dkg_nhead) * (g.dkg_nsect) - 1)) \
				    / ((g.dkg_nhead) * (g.dkg_nsect)))

/*
 *	Convert bytes to blocks:
 *
 *		b - # of bytes
 *		g - disk geometry
 *
 *	Rounds number of bytes upto the next cylinder boundary.
 */
#define	bytes_to_blocks(b, g)	(blocks_to_cyls(((b) + 511) / 512, g) \
				 * g.dkg_nhead * g.dkg_nsect)

/*
 *	Convert cylinders to blocks:
 *
 *		c - # of cylinders
 *		g - disk geometry
 */
#define	cyls_to_blocks(c, g)	(c * g.dkg_nhead * g.dkg_nsect)

/*
 *	Return a directory prefix, if any.
 */
#define	dir_prefix()		is_miniroot() ? ROOT_MNT_PT : ""


#ifdef SunB1
/*
 *	Macros for toggling mac-exempt status:
 */
#define	macex_off()	(void) macexempt(old_mac_state)
#define	macex_on()	old_mac_state = macexempt(1)
#else
#define	macex_off()
#define	macex_on()
#endif /* SunB1 */


/*
 *	Determine number of blocks for given partition:
 *
 *		p - partition letter, e.g. 'a', 'b', ...
 */
#define	map_blk(p)	(disk_p->partitions[(p) - 'a'].map_buf.dkl_nblk)

/*
 *	Determine number of cylinders for given partition:
 *
 *		p - partition letter, e.g. 'a', 'b', ...
 */
#define	map_cyl(p)	(disk_p->partitions[(p) - 'a'].map_buf.dkl_cylno)

/*
 *	Round blocks to next cylinder boundary:
 *
 *		b - # of blocks
 *		g - disk geometry
 */
#define	rnd_blocks(b, g)	(blocks_to_cyls(b, g) * g.dkg_nhead \
				 * g.dkg_nsect)


/*
 *	The flash_menu_off routine will call redisplay if REDISPLAY
 *	is the calling parameter.  For some forms this has a side effect
 *	of flashing some messages on the screen.  This is because
 *	redisplay will show the confirm question so if a flash_menu_on
 *	follows we see the confirm for a little bit.  Those cases should
 *	use DONT_DISPLAY but then you better make sure that the confirm
 *	message is forced to be displayed at the proper point.
 */

#define	REDISPLAY	1
#define	NOREDISPLAY	0
#define CONFIRM_B	2	

/*
 *	This macro is used to add the filesystem overhead to the size
 *	of the files.  This fudge factor should cause the filesystem
 *	to end up 97-99% full in the worst case (as long as there is a
 *	freehog to steal from
 */

#define	FUDGE_SIZE(x)	(((x / 13) * 2) + x)

/*
 *      Add the space used by swap for filesystem overhead.  WE are aiming for
 *      about 109% full here which is why it is smaller than FUDGE_SIZE.  We
 *      really are worying about filesystem overhead for things such as inodes
 */

#define SWAP_FUDGE_SIZE(x) ((x / 12)  + x)

/*
 *      The special character in the version that specifies that this
 *      is a PSR release.  The character after this identifier is the
 *      name of the PSR release
 */

#define PSR_IDENT "_PSR_"


#endif _INSTALL_H_
