#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)dummy.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)dummy.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		dummy.c
 *
 *	Description:	The dummy program exists for verifying the
 *		loadability of the suninstall library.  It also gives
 *		a handle on the size of the library when loaded with
 *		the system libraries.
 */

#ifdef SunB1
#include <sys/types.h>
#include <sys/label.h>
#endif /* SunB1 */
#include <varargs.h>
#include "install.h"

#ifdef SunB1
int		old_mac_state;
#endif /* SunB1 */
char *		progname;


main()
{
	char *easyinstall;
	clnt_info client;

#ifdef SunB1
	old_mac_state = old_mac_state;
#endif /* SunB1 */
	progname = progname;

	(void) add_client_to_list((clnt_info *) 0);
	add_key_entry((char *) 0, (char *) 0, (char *) 0, 0);
	add_net_label((char *) 0, 0, 0, (char *) 0);
	(void) appl_arch((char *) 0, (char *) 0);
	(void) aprid_to_aid((char *) 0, (char *) 0);
	(void) aprid_to_button((char *) 0, (char *) 0);
	(void) aprid_to_execpath((char *) 0, (char *) 0);
	(void) aprid_to_kvmpath((char *) 0, (char *) 0);
	(void) aprid_to_sharepath((char *) 0, (char *) 0);
	(void) aprid_to_syspath((char *) 0, (char *) 0);
	(void) aprid_to_iid((char *) 0, (char *) 0);
	(void) aprid_to_rid((char *) 0, (char *) 0);
	(void) arch_major(0);
	(void) arch_minor(0);
#ifdef SunB1
	(void) at_sys_high((char *) 0);
#endif /* SunB1 */
	(void) basename((char *) 0);
	(void) blocks_to_str((disk_info *) 0, (daddr_t) 0);
	(void) calc_client((char *)0);
	(void) calc_disk((sys_info *) 0);
	(void) calc_software((arch_info *) 0, (char *) 0, (sys_info *) 0,
			     (char *) 0);
	(void) check_client_terminal((char *)0);
	(void) clean_yp(client.root_path, client.yp_type);
	copy_install_bin((char *) 0, (char *) 0, (char *) 0);
	copy_tree((char *) 0, (char *) 0);
	(char *) custom_default_kvmpath((char *)0, (char *)0);
	(void) cv_ans_to_str((int *) 0);
	(void) cv_arch_to_str((int *) 0);
	(void) cv_char_to_str((char *) 0);
	(void) cv_cpp_to_str((char **) 0);
	(void) cv_ether_to_str((int *) 0);
	(void) cv_iflag_to_str((int *) 0);
	(void) cv_int_to_str((int *) 0);
	(void) cv_kind_to_str((int *) 0);
#ifdef SunB1
	(void) cv_lab_to_str((int *) 0);
#endif /* SunB1 */
	(void) cv_long_to_str((long *) 0);
	(void) cv_media_to_str((int *) 0);
	(void) cv_str_to_ans((char *) 0, (int *) 0);
	(void) cv_str_to_arch((char *) 0, (int *) 0);
	(void) cv_str_to_char((char *) 0, (char *) 0);
	(void) cv_str_to_cpp((char *) 0, (char **) 0);
	(void) cv_str_to_ether((char *) 0, (int *) 0);
	(void) cv_str_to_iflag((char *) 0, (int *) 0);
	(void) cv_str_to_int((char *) 0, (int *) 0);
	(void) cv_str_to_kind((char *) 0, (int *) 0);
#ifdef SunB1
	(void) cv_str_to_lab((char *) 0, (int *) 0);
#endif /* SunB1 */
	(void) cv_str_to_long((char *) 0, (long *) 0);
	(void) cv_str_to_media((char *) 0, (int *) 0);
	(void) cv_str_to_yp((char *) 0, (int *) 0);
	(void) cv_swap_to_long((char *) 0);
	(void) cv_yp_to_str((int *) 0);
	(void) default_release((Os_ident *) 0, (char *) 0);
	delete_blanks((char *) 0);
	(void) delete_client_from_list((clnt_info *) 0);
	(void) delete_client_to_list((clnt_info *) 0);
	(void) dirname((char *) 0);
	(void) disk_to_mount_list((disk_info *) 0, (char *) 0);
	easyinstall = EASYINSTALL;
#ifdef lint
	easyinstall = easyinstall;
#endif
	(void) elem_count((char *) 0);
	(void) err_mesg(0);
	(void) execute((char **) 0);
	(void) find_arch((char *) 0, (arch_info *) 0);
	(void) find_mf_part((soft_info *) 0, (media_file *) 0, (sys_info *) 0);
	(void) find_part((char *) 0);
#ifdef SunB1
	fix_devgroup((char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0);
#endif /* SunB1 */
	fix_passwd((char *) 0, (sec_info *) 0);
	free_media_file((soft_info *) 0);
	(void) get_arch();
	(void) get_disk_config((disk_info *) 0);
	(void) get_existing_part((disk_info *) 0);
	(void)get_install_method();
	get_stdin((char *) 0);
	get_terminal((char *) 0);
#ifdef SunB1
	golabeld((sys_info *) 0);
#endif /* SunB1 */
	ifconfig((sys_info *) 0, (soft_info *)0, (int) 0);
	(void) irid_to_aprid((char *) 0, (char *) 0);
	info_split_media_file((char *) 0, (char *) 0, (soft_info *) 0);
	(void) is_miniroot();
	(void) is_running((char *) 0);
	(void) is_sec_loaded((char *)0);
	(void) is_server((char *) 0);
	(void) is_small_disk((disk_info *) 0);
	(void) is_small_size((disk_info *) 0);
	(char *)itoa(0,(char *) 0);
	log((char *) 0);
	MAKEDEV((char *) 0, (char *) 0);
	(void) make_mount_list((sys_info *) 0);
	(void) media_block_size((soft_info *) 0);
	(void) media_dev_name(0);
	(void) media_extract((soft_info *) 0, (sys_info *) 0,(media_file *) 0);
	(void) media_fsf((soft_info *) 0, 0, 0);
	(void) media_read_file((soft_info *) 0, (char *) 0, 0);
	(void) media_rewind((soft_info *) 0, 0);
	menu_check_remote_access((char *) 0);
	(void) merge_media((char *) 0, (char *) 0, (soft_info *) 0);
	(void) merge_media_file((char *) 0, (char *) 0, (soft_info *) 0);
	mkdir_path((char *) 0);
#ifdef SunB1
	mkldir_path((char *) 0, (blabel_t *) 0);
#endif /* SunB1 */
	mklink((char *) 0, (char *) 0);
	mk_rc_domainname((char *) 0, (char *) 0);
	mk_rc_hostname((char *) 0, (char *) 0, (char *) 0);
	mk_rc_ifconfig((sys_info *) 0, (char *) 0);
	mk_rc_netmask((sys_info *) 0, (char *) 0);
	mk_rc_rarpd((sys_info *) 0, (char *) 0);
	mk_localtime((sys_info *) 0, (char *) 0);
	none_selected((soft_info *) 0);
	(void) mount_string((soft_info *) 0);
	(char *)os_ident_token((Os_ident *) 0, (char *)0);
	(void) partitions();
	please_check((soft_info *) 0);
	rarpd((sys_info *) 0);
	(void) read_arch_info((char *) 0, (arch_info **) 0);
	(void) read_clnt_info((char *) 0, (clnt_info *) 0);
	(void) read_disk_info((char *) 0, (disk_info *) 0);
	(void) read_file((soft_info *) 0, 0, (char *) 0);
	(void) read_media_file((char *) 0, (soft_info *) 0);
	(void) read_mount_list((char *) 0, (mnt_ent *) 0);
	(void) read_sec_info((char *) 0, (sec_info *) 0);
	(void) read_soft_info((char *) 0, (soft_info *) 0);
	(void) read_sys_info((char *) 0, (sys_info *) 0);
	replace_key_entry((char *) 0, (char *) 0, (char *) 0);
	(void) replace_media_file((char *) 0, (soft_info *) 0);
	(void) same_os((Os_ident *) 0, (Os_ident *) 0);
	(void) save_arch_info((char *) 0, (arch_info *) 0);
	(void) save_clnt_info((char *) 0, (clnt_info *) 0);
	(void) save_cmdfile((char *) 0, (disk_info *) 0);
	(void) save_disk_info((char *) 0, (disk_info *) 0);
	(void) save_media_file((char *) 0, (soft_info *) 0);
	(void) save_mount_list((char *) 0, (mnt_ent *) 0);
	(void) save_sec_info((char *) 0, (sec_info *) 0);
	(void) save_soft_info((char *) 0, (soft_info *) 0);
	(void) save_sys_info((char *) 0, (sys_info *) 0);
	sig_trap(0);
	sort_mount_list((mnt_ent *) 0);
	(void) split_media((char *) 0, (char *) 0, (soft_info *) 0);
	(void) split_media_file ((char *) 0, (char *) 0, (soft_info *) 0);
	(void) std_execpath((Os_ident *) 0, (char *) 0);
	(void) std_kvmpath((Os_ident *) 0, (char *) 0);
	(void) std_sharepath((Os_ident *) 0, (char *) 0);
	(void) std_syspath((Os_ident *) 0, (char *) 0);
	(char *) strstr((char *) 0, (char *) 0);
	(char *) strstr_ignore((char *) 0, (char *) 0);
	(char *) strupr((char *) 0);
	(void) suser();
	(void) toc_xlat((sys_info *) 0, (char *) 0, (soft_info *) 0);
	tune_audit((clnt_info *) NULL, (char *) 0, (sec_info *) 0);
	(void) update_arch((char *) 0, (char *) 0);
	(void) update_bytes((char *) 0, (long) 0);
	update_parts((disk_info *) 0);
	update_yp((clnt_info *) 0);
	(void) xlat_code((key_xlat *) 0, 0);
	(void) xlat_key((char *) 0, (key_xlat *) 0, 0);
	x_chdir((char *) 0);
	x_system((char *) 0);
	_log((char *) 0, (va_list) 0);
	exit(0);
	/*NOTREACHED*/
} /* end main() */
