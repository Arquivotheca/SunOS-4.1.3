#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)strings.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)strings.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		strings.
 *
 *	Description:	This file contains all the global strings for
 *		suninstall.
 */


/*
 *	Global string constants:
 */
char		APPL_MEDIA_FILE[] =	"/etc/install/appl_media_file";
char		ARCH_INFO[] =		"/etc/install/arch_info";
char		ARCH_LIST[] =		"/etc/install/arch_list";
#ifdef SunB1
char		AUDIT_CONTROL[] =	"/etc/security/audit_control";
#else
char		AUDIT_CONTROL[] =	"/etc/security/audit/audit_control";
#endif /* SunB1 */
char		AUDIT_DIR[] =		"/etc/security/audit";
char		AVAIL_ARCHES[] =	"/usr/etc/install/tar/avail_arches";
char		BOOTPARAMS[] =		"/etc/bootparams";
char		CD_EXPORT_DIR[] =	"/usr/etc/install/tar/export";
char		CLIENT_LIST[] =		"/etc/install/client_list";
char		CLIENT_LIST_ALL[] =	"/etc/install/client_list.all";
char		CLIENT_STR[] =		"/etc/install/client";
char		CMDFILE[] =		"/etc/install/cmdfile";
char		DEFAULT_CLIENT_INFO[] =	"/etc/install/default_client_info";
#ifdef SunB1
char		DEFAULT_FS[] =		"lfs";
char		DEFAULT_NET_FS[] = 	"lnfs";
#else
char		DEFAULT_FS[] =		"4.2";
char		DEFAULT_NET_FS[] =	"nfs";
#endif /* SunB1 */
char		DEFAULT_SYS_INFO[] =	"/etc/install/default_sys_info";
char		DEPENDENT_LIST[] =	"/etc/install/dependent_list";
#ifdef SunB1
char		DEVICE_CLEAN[] =	"device_clean";
char		DEVICE_GROUPS[] =	"/etc/security/device_groups";
#endif /* SunB1 */
char		DISK_INFO[] =		"/etc/install/disk_info";
char		DISK_LIST[] =		"/etc/install/disk_list";
char 		EASYINSTALL[] = 	"/usr/etc/install/easyinstall";
char		ETHERS[] =		"/etc/ethers";
char		EXCLUDELIST[] =		"/etc/install/EXCLUDELIST";
char		EXPORTS[] =		"/etc/exports";
char		FILE_DIR[] =		"/etc/install";
char		FSTAB[] =		"/etc/fstab";
char		FSTAB_TMP[] =		"/etc/fstab.tmp";
char		HOME[] =		"/export/home";
char		HOSTS[] =		"/etc/hosts";
#ifdef SunB1
char		HOSTS_LABEL[] =		"/etc/hosts.label";
#endif /* SunB1 */
char		INETD_CONF[] =		"/etc/inetd.conf";
char		INSTALL_BIN[] =		"/etc/install/install.bin";
char		INSTALL_TAR_DIR[] =	"/usr/etc/install/tar";
char		LOAD_ARCH[] =		"/etc/install/load_arch";
char		LOGFILE[] =		"/etc/install/suninstall.log";
char		MEDIA_FILE[] =		"/etc/install/media_file";
char		MEDIA_LIST[] =		"/etc/install/media_list";
char		MOUNT_LIST[] =		"/etc/install/mount_list";
#ifdef SunB1
char		NET_LABEL[] =		"/etc/net.label";
#endif /* SunB1 */
char		PASSWD[] =		"/etc/passwd";
char		PROTO_ROOT[] =		"/proto.root";
char		PWD_ADJUNCT[] =		"/etc/security/passwd.adjunct";
char		RC_DOMAINNAME[] =	"/etc/defaultdomain";
char *		RC_HEADER[] = {
	"\n",
	"#\n",
	"#\tThis file was created by the suninstall program.\n",
	"#\tAdditional interfaces that are not supported by suninstall\n",
	"#\tcan be added to this file.\n",
	"#\n",

	(char *) 0
};
char		RC_HOSTNAME[] =		"/etc/hostname";
#ifdef 	SUN_4_0
char		RC_IFCONFIG[]	=	"/etc/rc.ifconfig";
char		RC_NETMASK[]	=	"/etc/rc.netmask";
char		RC_RARPD[]	=	"/etc/rc.rarpd";
#endif	SUN_4_0
char		RELEASE[] =		"/etc/install/release";
char		RMP_EXPORTS[] =		"/a/etc/exports";
char		RMP_FILE_DIR[] =	"/a/etc/install";
char		RMP_HOSTS[] =		"/a/etc/hosts";
#ifdef SunB1
char		RMP_HOSTS_LABEL[] =	"/a/etc/hosts.label";
char		RMP_NET_LABEL[] =	"/a/etc/net.label";
char		RMP_SEC_DEV_DIR[] =	"/a/etc/security/dev";
#endif /* SunB1 */
char		RMP_TTYTAB[] =		"/a/etc/ttytab";
char		ROOT_MNT_PT[] =		"/a";
#ifdef SunB1
char		SEC_DEV_DIR[] =		"/etc/security/dev";
#endif /* SunB1 */
char		SEC_INFO[] =		"/etc/install/sec_info";
char		SOFT_INFO[] =		"/etc/install/soft_info";
char		STD_DEVS[] =		"std pty0 pty1 pty2 win0 win1 win2";
char		SYS_INFO[] =		"/etc/install/sys_info";
char		TTYTAB[]  =		"/etc/ttytab";
char		TOOL_DIR[] =		"/usr/etc/install";
char		XDRTOC[] =		"/etc/install/XDRTOC";
char		ZONEINFO[] =		"/usr/share/lib/zoneinfo";
char		ZONEINFO_PATH[] =	"/lib/zoneinfo";

char		STD_EXEC_PATH[] =	"/export/exec";
char		STD_KVM_PATH[] =	"/export/exec/kvm";
char		STD_SHARE_PATH[] =	"/export/share";

#ifdef lint
/*
 *	This array is to keep lint from complaining about the strings.
 */

static	char *		dummy[] = {
	(char *) dummy,
	ARCH_INFO,
	ARCH_LIST,
	AUDIT_CONTROL,
	AUDIT_DIR,
	AVAIL_ARCHES,
	BOOTPARAMS,
	CD_EXPORT_DIR,
	CLIENT_LIST,
	CLIENT_LIST_ALL,
	CLIENT_STR,
	CMDFILE,
	DEFAULT_CLIENT_INFO,
	DEFAULT_FS,
	DEFAULT_NET_FS,
	DEFAULT_SYS_INFO,
	DEPENDENT_LIST,
#ifdef SunB1
	DEVICE_CLEAN,
	DEVICE_GROUPS,
#endif /* SunB1 */
	DISK_INFO,
	DISK_LIST,
	ETHERS,
	EXCLUDELIST,
	EXPORTS,
	FILE_DIR,
	FSTAB,
	FSTAB_TMP,
	HOME,
	HOSTS,
#ifdef SunB1
	HOSTS_LABEL,
#endif /* SunB1 */
	INETD_CONF,
	INSTALL_BIN,
	LOAD_ARCH,
	LOGFILE,
	MEDIA_FILE,
	MEDIA_LIST,
	MOUNT_LIST,
#ifdef SunB1
	NET_LABEL,
#endif /* SunB1 */
	PASSWD,
	PROTO_ROOT,
	PWD_ADJUNCT,
	RC_DOMAINNAME,
	(char *) RC_HEADER,
	RC_HOSTNAME,
#ifdef SUN_4_0
	RC_IFCONFIG,
	RC_NETMASK,
	RC_RARPD,
#endif SUN_4_0
	RELEASE,
	RMP_EXPORTS,
	RMP_FILE_DIR,
	RMP_HOSTS,
#ifdef SunB1
	RMP_HOSTS_LABEL,
	RMP_NET_LABEL,
	RMP_SEC_DEV_DIR,
#endif /* SunB1 */
	RMP_TTYTAB,
	ROOT_MNT_PT,
#ifdef SunB1
	SEC_DEV_DIR,
#endif /* SunB1 */
	SEC_INFO,
	SOFT_INFO,
	STD_DEVS,
	STD_EXEC_PATH,
	STD_KVM_PATH,
	STD_SHARE_PATH,
	SYS_INFO,
	TOOL_DIR,
	TTYTAB,
	XDRTOC,
	ZONEINFO,
	0
};
#endif lint
