#
#	Makefile for SunOS 4.x/MLS installation libraries
#
#	@(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright (c) 1989 by Sun Microsystems, Inc.
#
include ../Makefile.master

CFLAGS=		$(G_CFLAGS) $(INCPATH)
HFILES=		../install.h ../unpack_help.h
INCPATH=	-I.. -I$(MENU_DIR) -I../../../include 
LIB=		libinstall.a
LIB_MENU=	$(MENU_DIR)/libmenu.a
#LIB_STD=	../../../lib/libc/libc.a
LINTFLAGS=	$(G_LINTFLAGS)
LINT_LIB=	llib-linstall.ln
LINT_MENU=	$(MENU_DIR)/llib-lmenu.ln
MENU_DIR=	../libmenu
MENU_HFILES=	$(MENU_DIR)/menu.h
MKTP_DIR=	../../../sundist/mktp
MKTP_HFILES=	$(MKTP_DIR)/mktp.h $(MKTP_DIR)/toc.h

media_io.o cv_type.o depend := INCPATH += -I$(MKTP_DIR)

#
#	Objects common across all architectures
#
OBJS_COMM= \
		add_key_entry.o \
		apr.o \
		arch_info.o \
		basename.o \
		blocks_to_str.o \
		calc_client.o \
		calc_disk.o \
		calc_software.o \
		cd_file.o \
		check_terminal.o \
		clean_yp.o \
		clnt_info.o \
		cmdfile.o \
		copy_bins.o \
		copy_tree.o \
		cv_ans.o \
		cv_arch.o \
		cv_char.o \
		cv_cpp.o \
		cv_ether.o \
		cv_iflag.o \
		cv_int.o \
		cv_kind.o \
		cv_long.o \
		cv_media.o \
		cv_swap.o \
		cv_type.o \
		cv_yp.o \
		delete_blanks.o \
		delete_client_from_list.o \
		dirname.o \
		disk_config.o \
		disk_info.o \
		disk_to_mnt.o \
		err_mesg.o \
		execute.o \
		find_arch.o \
		find_mf_part.o \
		find_part.o \
		fix_passwd.o \
		get_arch.o \
		get_ethertypes.o \
		get_install_method.o \
		get_stdin.o \
		get_terminal.o \
		terminal.o \
		ifconfig.o \
		init_info.o \
		is_server.o \
		is_miniroot.o \
		is_running.o \
		is_scsi_tape.o \
		is_sec_loaded.o \
		is_small_disk.o \
		log.o \
		make_mount.o \
		makedev.o \
		media_file.o \
		media_io.o \
		media_select.o \
		merge_media_file.o \
		mk_domain.o \
		mk_hostname.o \
		mk_localtime.o \
		mkdir_path.o \
		mklink.o \
		mount_list.o \
		naming.o \
		rarpd.o \
		remote_access.o \
		read_file.o \
		scsi_block_size.o \
		sec_info.o \
		sig_trap.o \
		soft_info.o \
		string_op.o \
		strings.o \
		suser.o \
		sys_info.o \
		toc_xlat.o \
		tune_audit.o \
		update_arch.o \
		update_bytes.o \
		update_parts.o \
		update_yp.o \
		x_chdir.o \
		x_system.o \
		xlat.o
#
#	Objects just for SunOS MLS
#
OBJS_MLS= \
		add_net_lab.o \
		at_sys_high.o \
		cv_lab.o \
		fix_devgroup.o \
		golabeld.o \
		mkldir_path.o

SRCS_COMM=	$(OBJS_COMM:.o=.c)
SRCS_MLS=	$(OBJS_MLS:.o=.c)


all:		lib

lib:		$(LIB)

$(LIB):		$(OBJS)
		ar u $(LIB) $?
		ranlib $(LIB)

$(HFILES):
		cd $(@D); sccs get $(@F)

$(MENU_HFILES):
		cd $(@D); sccs get $(@F)

$(MKTP_HFILES):
		cd $(@D); $(MAKE) $(MFLAGS) $(@F)

#
#	Administrative targets:
#
clean:
		rm -f $(LIB) $(LINT_LIB) $(OBJS_COMM) $(OBJS_MLS)
		rm -f dummy dummy.o lint.log

depend:		$(SRCS)
		rm -f makedep
		for i in $(SRCS); do \
			$(CC) -M $(INCPATH) $$i | \
			sed -e 's#/sun[1-9][0-9]*[a-zA-Z]*/#/machine/#g' | \
			sort | uniq | \
			awk '\
			{ \
				if ($$1 != prev) { \
					if (rec != "") \
						print rec; \
					rec = $$0; \
					prev = $$1; \
				} \
				else if (length(rec $$2) > 78) { \
					print rec; \
					rec = $$0; \
				} \
				else \
					rec = rec " " $$2 \
			} \
			END { print rec } '; \
		done | sort > makedep
		echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

#
#	The dummy program exists for verifying the loadability of the
#	suninstall library.  It also gives a handle on the size of the
#	library when loaded with the system library.
#
dummy:		dummy.o $(LIB) $(LIB_MENU)
		$(CC) -o dummy dummy.o $(LIB) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)

lint:		lint.log

lint.log:	$(HFILES) $(MENU_HFILES) $(SRCS) $(MKTP_HFILES) \
			  dummy.c $(LINT_MENU)
		$(LINT) $(LINTFLAGS) $(INCPATH) -I$(MKTP_DIR) \
		-lcurses $(LINT_MENU) $(SRCS) dummy.c > lint.log 2>&1

lintlib:	$(LINT_LIB)

$(LINT_LIB):	$(HFILES) $(MENU_HFILES) $(SRCS)
		$(LINT) $(LINTFLAGS) $(INCPATH) -Cinstall $(SRCS)

$(LIB_MENU):	FRC
		cd $(@D); $(MAKE) $(MFLAGS) $(@F)

$(LINT_MENU):	FRC
		cd $(@D); $(MAKE) $(MFLAGS) $(@F)

FRC:

.PRECIOUS:	$(LIB) $(LINT_MENU)

# DO NOT DELETE THIS LINE -- make depend uses it
add_key_entry.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
add_key_entry.o: /usr/include/machine/param.h /usr/include/stdio.h
add_key_entry.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
add_key_entry.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
add_key_entry.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_key_entry.o: /usr/include/sys/param.h /usr/include/sys/signal.h
add_key_entry.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
add_key_entry.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
add_key_entry.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_key_entry.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
add_key_entry.o: /usr/include/vm/faultcode.h add_key_entry.c
apr.o: ../../../include/string.h ../install.h /usr/include/machine/param.h
apr.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
apr.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
apr.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
apr.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
apr.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
apr.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
apr.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
apr.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h apr.c
arch_info.o: ../install.h /usr/include/machine/param.h /usr/include/stdio.h
arch_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
arch_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
arch_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
arch_info.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
arch_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
arch_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
arch_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
arch_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h arch_info.c
basename.o: ../../../include/string.h /usr/include/sys/stdtypes.h basename.c
blocks_to_str.o: ../install.h /usr/include/machine/param.h
blocks_to_str.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
blocks_to_str.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
blocks_to_str.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
blocks_to_str.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
blocks_to_str.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
blocks_to_str.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
blocks_to_str.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
blocks_to_str.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
blocks_to_str.o: blocks_to_str.c
calc_client.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
calc_client.o: /usr/include/stdio.h /usr/include/sun/dkio.h
calc_client.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
calc_client.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
calc_client.o: /usr/include/sys/param.h /usr/include/sys/signal.h
calc_client.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
calc_client.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
calc_client.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
calc_client.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
calc_client.o: /usr/include/vm/faultcode.h calc_client.c
calc_disk.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
calc_disk.o: /usr/include/stdio.h /usr/include/sun/dkio.h
calc_disk.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
calc_disk.o: /usr/include/sys/file.h /usr/include/sys/filio.h
calc_disk.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
calc_disk.o: /usr/include/sys/param.h /usr/include/sys/signal.h
calc_disk.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
calc_disk.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
calc_disk.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
calc_disk.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
calc_disk.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h calc_disk.c
calc_software.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
calc_software.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
calc_software.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
calc_software.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
calc_software.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
calc_software.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
calc_software.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
calc_software.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
calc_software.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
calc_software.o: calc_software.c
cd_file.o: ../../../include/string.h ../install.h /usr/include/machine/param.h
cd_file.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
cd_file.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
cd_file.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
cd_file.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
cd_file.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
cd_file.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
cd_file.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
cd_file.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h cd_file.c
check_terminal.o: ../install.h /usr/include/machine/param.h
check_terminal.o: /usr/include/stdio.h /usr/include/sun/dkio.h
check_terminal.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
check_terminal.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
check_terminal.o: /usr/include/sys/param.h /usr/include/sys/signal.h
check_terminal.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
check_terminal.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
check_terminal.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
check_terminal.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
check_terminal.o: /usr/include/vm/faultcode.h check_terminal.c
clean_yp.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
clean_yp.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
clean_yp.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clean_yp.o: /usr/include/sys/param.h /usr/include/sys/signal.h
clean_yp.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
clean_yp.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
clean_yp.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clean_yp.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
clean_yp.o: /usr/include/vm/faultcode.h clean_yp.c
clnt_info.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
clnt_info.o: /usr/include/stdio.h /usr/include/sun/dkio.h
clnt_info.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
clnt_info.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clnt_info.o: /usr/include/sys/param.h /usr/include/sys/signal.h
clnt_info.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
clnt_info.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
clnt_info.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clnt_info.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
clnt_info.o: /usr/include/vm/faultcode.h clnt_info.c
cmdfile.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
cmdfile.o: /usr/include/stdio.h /usr/include/sun/dkio.h
cmdfile.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cmdfile.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cmdfile.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cmdfile.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cmdfile.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cmdfile.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cmdfile.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cmdfile.o: /usr/include/vm/faultcode.h cmdfile.c
copy_bins.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
copy_bins.o: /usr/include/machine/param.h /usr/include/stdio.h
copy_bins.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
copy_bins.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
copy_bins.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
copy_bins.o: /usr/include/sys/param.h /usr/include/sys/signal.h
copy_bins.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
copy_bins.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
copy_bins.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
copy_bins.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
copy_bins.o: /usr/include/vm/faultcode.h copy_bins.c
copy_tree.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
copy_tree.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
copy_tree.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
copy_tree.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
copy_tree.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
copy_tree.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
copy_tree.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
copy_tree.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
copy_tree.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h copy_tree.c
cv_ans.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_ans.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_ans.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_ans.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_ans.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_ans.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_ans.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_ans.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_ans.o: /usr/include/vm/faultcode.h cv_ans.c
cv_arch.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_arch.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_arch.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_arch.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_arch.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_arch.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_arch.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_arch.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_arch.o: /usr/include/vm/faultcode.h cv_arch.c
cv_char.o: cv_char.c
cv_cpp.o: ../../../include/string.h /usr/include/sys/stdtypes.h cv_cpp.c
cv_ether.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_ether.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_ether.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_ether.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_ether.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_ether.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_ether.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_ether.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_ether.o: /usr/include/vm/faultcode.h cv_ether.c
cv_iflag.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_iflag.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_iflag.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_iflag.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_iflag.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_iflag.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_iflag.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_iflag.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_iflag.o: /usr/include/vm/faultcode.h cv_iflag.c
cv_int.o: cv_int.c
cv_kind.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_kind.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_kind.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_kind.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_kind.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_kind.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_kind.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_kind.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_kind.o: /usr/include/vm/faultcode.h cv_kind.c
cv_long.o: cv_long.c
cv_media.o: ../install.h ../media.h /usr/include/machine/param.h
cv_media.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
cv_media.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
cv_media.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
cv_media.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
cv_media.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
cv_media.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
cv_media.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
cv_media.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h cv_media.c
cv_swap.o: ../install.h ../libmenu/menu.h /usr/include/ctype.h
cv_swap.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_swap.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_swap.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_swap.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_swap.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_swap.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_swap.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_swap.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_swap.o: /usr/include/vm/faultcode.h cv_swap.c
cv_type.o: ../../../sundist/mktp/toc.h ../../../sundist/mktp/xdr_custom.h
cv_type.o: ../install.h /usr/include/machine/param.h /usr/include/malloc.h
cv_type.o: /usr/include/netinet/in.h /usr/include/rpc/auth.h
cv_type.o: /usr/include/rpc/auth_des.h /usr/include/rpc/auth_unix.h
cv_type.o: /usr/include/rpc/clnt.h /usr/include/rpc/rpc.h
cv_type.o: /usr/include/rpc/rpc_msg.h /usr/include/rpc/svc.h
cv_type.o: /usr/include/rpc/svc_auth.h /usr/include/rpc/types.h
cv_type.o: /usr/include/rpc/xdr.h /usr/include/stdio.h /usr/include/sun/dkio.h
cv_type.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_type.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_type.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_type.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_type.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
cv_type.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
cv_type.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
cv_type.o: /usr/include/sys/types.h /usr/include/time.h
cv_type.o: /usr/include/vm/faultcode.h cv_type.c
cv_yp.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
cv_yp.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
cv_yp.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cv_yp.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cv_yp.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
cv_yp.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
cv_yp.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
cv_yp.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
cv_yp.o: /usr/include/vm/faultcode.h cv_yp.c
delete_blanks.o: ../../../include/string.h /usr/include/ctype.h
delete_blanks.o: /usr/include/stdio.h /usr/include/sys/stdtypes.h
delete_blanks.o: delete_blanks.c
delete_client_from_list.o: ../install.h ../libmenu/menu.h
delete_client_from_list.o: /usr/include/machine/param.h /usr/include/stdio.h
delete_client_from_list.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
delete_client_from_list.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
delete_client_from_list.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
delete_client_from_list.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
delete_client_from_list.o: /usr/include/sys/stdtypes.h
delete_client_from_list.o: /usr/include/sys/sysmacros.h
delete_client_from_list.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
delete_client_from_list.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
delete_client_from_list.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
delete_client_from_list.o: delete_client_from_list.c
dirname.o: ../../../include/string.h /usr/include/machine/param.h
dirname.o: /usr/include/sys/param.h /usr/include/sys/signal.h
dirname.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
dirname.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h dirname.c
disk_config.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
disk_config.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
disk_config.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
disk_config.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
disk_config.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
disk_config.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
disk_config.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
disk_config.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disk_config.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
disk_config.o: /usr/include/vm/faultcode.h disk_config.c
disk_info.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
disk_info.o: /usr/include/stdio.h /usr/include/sun/dkio.h
disk_info.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
disk_info.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disk_info.o: /usr/include/sys/param.h /usr/include/sys/signal.h
disk_info.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
disk_info.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
disk_info.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disk_info.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
disk_info.o: /usr/include/vm/faultcode.h disk_info.c
disk_to_mnt.o: ../../../include/string.h ../install.h ../libmenu/menu.h
disk_to_mnt.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
disk_to_mnt.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
disk_to_mnt.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disk_to_mnt.o: /usr/include/sys/param.h /usr/include/sys/signal.h
disk_to_mnt.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
disk_to_mnt.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
disk_to_mnt.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disk_to_mnt.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
disk_to_mnt.o: /usr/include/vm/faultcode.h disk_to_mnt.c
err_mesg.o: ../../../include/errno.h /usr/include/sys/errno.h err_mesg.c
execute.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
execute.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
execute.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
execute.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
execute.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
execute.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
execute.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
execute.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
execute.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
execute.o: /usr/include/sys/types.h /usr/include/sys/wait.h
execute.o: /usr/include/vm/faultcode.h execute.c
find_arch.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
find_arch.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
find_arch.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_arch.o: /usr/include/sys/param.h /usr/include/sys/signal.h
find_arch.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
find_arch.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
find_arch.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_arch.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
find_arch.o: /usr/include/vm/faultcode.h find_arch.c
find_mf_part.o: ../install.h /usr/include/machine/param.h
find_mf_part.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
find_mf_part.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
find_mf_part.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
find_mf_part.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
find_mf_part.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
find_mf_part.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
find_mf_part.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
find_mf_part.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
find_mf_part.o: find_mf_part.c
find_part.o: ../../../include/string.h ../install.h ../libmenu/menu.h
find_part.o: /usr/include/machine/param.h /usr/include/stdio.h
find_part.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
find_part.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
find_part.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
find_part.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
find_part.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
find_part.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
find_part.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
find_part.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h find_part.c
fix_passwd.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
fix_passwd.o: /usr/include/stdio.h /usr/include/sun/dkio.h
fix_passwd.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
fix_passwd.o: /usr/include/sys/file.h /usr/include/sys/filio.h
fix_passwd.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
fix_passwd.o: /usr/include/sys/param.h /usr/include/sys/signal.h
fix_passwd.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
fix_passwd.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
fix_passwd.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
fix_passwd.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
fix_passwd.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h fix_passwd.c
get_arch.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
get_arch.o: /usr/include/stdio.h /usr/include/sun/dkio.h
get_arch.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
get_arch.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_arch.o: /usr/include/sys/param.h /usr/include/sys/signal.h
get_arch.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
get_arch.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
get_arch.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_arch.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
get_arch.o: /usr/include/vm/faultcode.h get_arch.c
get_install_method.o: ../install.h ../unpack_help.h
get_install_method.o: /usr/include/machine/param.h /usr/include/stdio.h
get_install_method.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_install_method.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_install_method.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_install_method.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
get_install_method.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_install_method.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_install_method.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_install_method.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_install_method.o: get_install_method.c
get_stdin.o: ../install.h /usr/include/machine/param.h /usr/include/stdio.h
get_stdin.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_stdin.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_stdin.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_stdin.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
get_stdin.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_stdin.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_stdin.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_stdin.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h get_stdin.c
get_terminal.o: ../../../include/string.h ../install.h ../libmenu/menu.h
get_terminal.o: /usr/include/machine/param.h /usr/include/stdio.h
get_terminal.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_terminal.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_terminal.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_terminal.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
get_terminal.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_terminal.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_terminal.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_terminal.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_terminal.o: get_terminal.c
terminal.o: ../../../include/string.h ../install.h ../libmenu/menu.h
terminal.o: /usr/include/machine/param.h /usr/include/stdio.h
terminal.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
terminal.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
terminal.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
terminal.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
terminal.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
terminal.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
terminal.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
terminal.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
terminal.o: terminal.c
ifconfig.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
ifconfig.o: /usr/include/stdio.h /usr/include/sun/dkio.h
ifconfig.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
ifconfig.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
ifconfig.o: /usr/include/sys/param.h /usr/include/sys/signal.h
ifconfig.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
ifconfig.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
ifconfig.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
ifconfig.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
ifconfig.o: /usr/include/vm/faultcode.h ifconfig.c
is_miniroot.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
is_miniroot.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
is_miniroot.o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
is_miniroot.o: is_miniroot.c
is_running.o: /usr/include/machine/param.h /usr/include/stdio.h
is_running.o: /usr/include/sys/param.h /usr/include/sys/signal.h
is_running.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
is_running.o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
is_running.o: /usr/include/vm/faultcode.h is_running.c
is_scsi_tape.o: ../../../include/strings.h is_scsi_tape.c
is_sec_loaded.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
is_sec_loaded.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
is_sec_loaded.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
is_sec_loaded.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
is_sec_loaded.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
is_sec_loaded.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
is_sec_loaded.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
is_sec_loaded.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
is_sec_loaded.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
is_sec_loaded.o: is_sec_loaded.c
is_server.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
is_server.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
is_server.o: /usr/include/sys/file.h /usr/include/sys/filio.h
is_server.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
is_server.o: /usr/include/sys/param.h /usr/include/sys/signal.h
is_server.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
is_server.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
is_server.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
is_server.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
is_server.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h is_server.c
is_small_disk.o: ../disk_impl.h ../install.h ../libmenu/menu.h
is_small_disk.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
is_small_disk.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
is_small_disk.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
is_small_disk.o: /usr/include/sys/param.h /usr/include/sys/signal.h
is_small_disk.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
is_small_disk.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
is_small_disk.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
is_small_disk.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
is_small_disk.o: /usr/include/vm/faultcode.h is_small_disk.c
log.o: ../../../include/errno.h ../install.h /usr/include/machine/param.h
log.o: /usr/include/stdio.h /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
log.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
log.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
log.o: /usr/include/sys/param.h /usr/include/sys/signal.h
log.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
log.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
log.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
log.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
log.o: /usr/include/varargs.h /usr/include/vm/faultcode.h log.c
make_mount.o: ../../../include/string.h ../install.h ../libmenu/menu.h
make_mount.o: /usr/include/machine/param.h /usr/include/stdio.h
make_mount.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
make_mount.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
make_mount.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
make_mount.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
make_mount.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
make_mount.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
make_mount.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
make_mount.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h make_mount.c
makedev.o: ../../../include/string.h ../install.h ../libmenu/menu.h ../media.h
makedev.o: /usr/include/machine/param.h /usr/include/stdio.h
makedev.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
makedev.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
makedev.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
makedev.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
makedev.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
makedev.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
makedev.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
makedev.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h makedev.c
media_file.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
media_file.o: /usr/include/stdio.h /usr/include/sun/dkio.h
media_file.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
media_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
media_file.o: /usr/include/sys/param.h /usr/include/sys/signal.h
media_file.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
media_file.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
media_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
media_file.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
media_file.o: /usr/include/vm/faultcode.h media_file.c
media_io.o: ../../../sundist/mktp/mktp.h ../../../sundist/mktp/toc.h
media_io.o: ../../../sundist/mktp/xdr_custom.h ../install.h ../libmenu/menu.h
media_io.o: ../media.h /usr/include/machine/param.h /usr/include/malloc.h
media_io.o: /usr/include/netinet/in.h /usr/include/rpc/auth.h
media_io.o: /usr/include/rpc/auth_des.h /usr/include/rpc/auth_unix.h
media_io.o: /usr/include/rpc/clnt.h /usr/include/rpc/rpc.h
media_io.o: /usr/include/rpc/rpc_msg.h /usr/include/rpc/svc.h
media_io.o: /usr/include/rpc/svc_auth.h /usr/include/rpc/types.h
media_io.o: /usr/include/rpc/xdr.h /usr/include/stdio.h /usr/include/sun/dkio.h
media_io.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
media_io.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
media_io.o: /usr/include/sys/param.h /usr/include/sys/signal.h
media_io.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
media_io.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
media_io.o: /usr/include/sys/time.h /usr/include/sys/ttold.h
media_io.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
media_io.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
media_io.o: /usr/include/time.h /usr/include/unistd.h
media_io.o: /usr/include/vm/faultcode.h media_io.c
media_select.o: ../install.h /usr/include/machine/param.h
media_select.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
media_select.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
media_select.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
media_select.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
media_select.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
media_select.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
media_select.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
media_select.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
media_select.o: media_select.c
merge_media_file.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
merge_media_file.o: /usr/include/stdio.h /usr/include/sun/dkio.h
merge_media_file.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
merge_media_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
merge_media_file.o: /usr/include/sys/param.h /usr/include/sys/signal.h
merge_media_file.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
merge_media_file.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
merge_media_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
merge_media_file.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
merge_media_file.o: /usr/include/vm/faultcode.h merge_media_file.c
mk_domain.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
mk_domain.o: /usr/include/machine/param.h /usr/include/stdio.h
mk_domain.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
mk_domain.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
mk_domain.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
mk_domain.o: /usr/include/sys/param.h /usr/include/sys/signal.h
mk_domain.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
mk_domain.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
mk_domain.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
mk_domain.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
mk_domain.o: /usr/include/vm/faultcode.h mk_domain.c
mk_hostname.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
mk_hostname.o: /usr/include/machine/param.h /usr/include/stdio.h
mk_hostname.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
mk_hostname.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
mk_hostname.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
mk_hostname.o: /usr/include/sys/param.h /usr/include/sys/signal.h
mk_hostname.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
mk_hostname.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
mk_hostname.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
mk_hostname.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
mk_hostname.o: /usr/include/vm/faultcode.h mk_hostname.c
mk_localtime.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
mk_localtime.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
mk_localtime.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
mk_localtime.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
mk_localtime.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
mk_localtime.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
mk_localtime.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
mk_localtime.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
mk_localtime.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
mk_localtime.o: mk_localtime.c
mkdir_path.o: ../../../include/errno.h ../../../include/string.h ../install.h
mkdir_path.o: ../libmenu/menu.h /usr/include/machine/param.h
mkdir_path.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
mkdir_path.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
mkdir_path.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
mkdir_path.o: /usr/include/sys/param.h /usr/include/sys/signal.h
mkdir_path.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
mkdir_path.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
mkdir_path.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
mkdir_path.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
mkdir_path.o: /usr/include/vm/faultcode.h mkdir_path.c
mklink.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
mklink.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
mklink.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
mklink.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
mklink.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
mklink.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
mklink.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
mklink.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
mklink.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
mklink.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h mklink.c
mount_list.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
mount_list.o: /usr/include/stdio.h /usr/include/sun/dkio.h
mount_list.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
mount_list.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
mount_list.o: /usr/include/sys/param.h /usr/include/sys/signal.h
mount_list.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
mount_list.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
mount_list.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
mount_list.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
mount_list.o: /usr/include/vm/faultcode.h mount_list.c
naming.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
naming.o: /usr/include/stdio.h /usr/include/sun/dkio.h
naming.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
naming.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
naming.o: /usr/include/sys/param.h /usr/include/sys/signal.h
naming.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
naming.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
naming.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
naming.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
naming.o: /usr/include/vm/faultcode.h naming.c
rarpd.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
rarpd.o: /usr/include/stdio.h /usr/include/sun/dkio.h
rarpd.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
rarpd.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
rarpd.o: /usr/include/sys/param.h /usr/include/sys/signal.h
rarpd.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
rarpd.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
rarpd.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
rarpd.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
rarpd.o: /usr/include/vm/faultcode.h rarpd.c
read_file.o: ../install.h /usr/include/machine/param.h /usr/include/sun/dkio.h
read_file.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
read_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
read_file.o: /usr/include/sys/param.h /usr/include/sys/signal.h
read_file.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
read_file.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
read_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
read_file.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
read_file.o: /usr/include/vm/faultcode.h read_file.c
remote_access.o: ../install.h /usr/include/machine/param.h /usr/include/stdio.h
remote_access.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
remote_access.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
remote_access.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
remote_access.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
remote_access.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
remote_access.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
remote_access.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
remote_access.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
remote_access.o: remote_access.c
scsi_block_size.o: ../install.h ../media.h /usr/include/fcntl.h
scsi_block_size.o: /usr/include/machine/param.h /usr/include/stdio.h
scsi_block_size.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
scsi_block_size.o: /usr/include/sys/fcntlcom.h /usr/include/sys/filio.h
scsi_block_size.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
scsi_block_size.o: /usr/include/sys/mtio.h /usr/include/sys/param.h
scsi_block_size.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
scsi_block_size.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
scsi_block_size.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
scsi_block_size.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
scsi_block_size.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
scsi_block_size.o: /usr/include/vm/faultcode.h scsi_block_size.c
sec_info.o: ../../../include/string.h ../install.h ../libmenu/menu.h
sec_info.o: /usr/include/machine/param.h /usr/include/stdio.h
sec_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
sec_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
sec_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
sec_info.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
sec_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
sec_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
sec_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
sec_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h sec_info.c
sig_trap.o: ../../../include/sgtty.h ../../../include/signal.h ../install.h
sig_trap.o: ../libmenu/menu.h /usr/include/curses.h
sig_trap.o: /usr/include/machine/param.h /usr/include/stdio.h
sig_trap.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
sig_trap.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
sig_trap.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
sig_trap.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
sig_trap.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
sig_trap.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
sig_trap.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
sig_trap.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h sig_trap.c
soft_info.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
soft_info.o: /usr/include/stdio.h /usr/include/sun/dkio.h
soft_info.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
soft_info.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
soft_info.o: /usr/include/sys/param.h /usr/include/sys/signal.h
soft_info.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
soft_info.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
soft_info.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
soft_info.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
soft_info.o: /usr/include/vm/faultcode.h soft_info.c
string_op.o: ../../../include/string.h /usr/include/ctype.h
string_op.o: /usr/include/stdio.h /usr/include/sys/stdtypes.h string_op.c
strings.o: strings.c
suser.o: suser.c
sys_info.o: ../../../include/string.h ../install.h ../libmenu/menu.h
sys_info.o: /usr/include/machine/param.h /usr/include/stdio.h
sys_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
sys_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
sys_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
sys_info.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
sys_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
sys_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
sys_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
sys_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h sys_info.c
toc_xlat.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
toc_xlat.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
toc_xlat.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
toc_xlat.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
toc_xlat.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
toc_xlat.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
toc_xlat.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
toc_xlat.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
toc_xlat.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
toc_xlat.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
toc_xlat.o: /usr/include/sys/wait.h /usr/include/vm/faultcode.h toc_xlat.c
tune_audit.o: ../../../include/errno.h ../../../include/string.h ../install.h
tune_audit.o: ../libmenu/menu.h /usr/include/machine/param.h
tune_audit.o: /usr/include/stdio.h /usr/include/sun/dkio.h
tune_audit.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
tune_audit.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
tune_audit.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
tune_audit.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
tune_audit.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
tune_audit.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
tune_audit.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
tune_audit.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h tune_audit.c
update_arch.o: ../../../include/string.h ../install.h ../libmenu/menu.h
update_arch.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
update_arch.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
update_arch.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
update_arch.o: /usr/include/sys/param.h /usr/include/sys/signal.h
update_arch.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
update_arch.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
update_arch.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
update_arch.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
update_arch.o: /usr/include/vm/faultcode.h update_arch.c
update_bytes.o: ../../../include/string.h ../install.h ../libmenu/menu.h
update_bytes.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
update_bytes.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
update_bytes.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
update_bytes.o: /usr/include/sys/param.h /usr/include/sys/signal.h
update_bytes.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
update_bytes.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
update_bytes.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
update_bytes.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
update_bytes.o: /usr/include/vm/faultcode.h update_bytes.c
update_parts.o: ../../../include/string.h ../install.h
update_parts.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
update_parts.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
update_parts.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
update_parts.o: /usr/include/sys/param.h /usr/include/sys/signal.h
update_parts.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
update_parts.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
update_parts.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
update_parts.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
update_parts.o: /usr/include/vm/faultcode.h update_parts.c
update_yp.o: ../install.h /usr/include/machine/param.h /usr/include/stdio.h
update_yp.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
update_yp.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
update_yp.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
update_yp.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
update_yp.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
update_yp.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
update_yp.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
update_yp.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
update_yp.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
update_yp.o: /usr/include/vm/faultcode.h update_yp.c
x_chdir.o: ../../../include/errno.h ../install.h ../libmenu/menu.h
x_chdir.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
x_chdir.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
x_chdir.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
x_chdir.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
x_chdir.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
x_chdir.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
x_chdir.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
x_chdir.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
x_chdir.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h x_chdir.c
x_system.o: ../install.h ../libmenu/menu.h /usr/include/machine/param.h
x_system.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
x_system.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
x_system.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
x_system.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
x_system.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
x_system.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
x_system.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
x_system.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h x_system.c
xlat.o: ../../../include/string.h ../install.h /usr/include/machine/param.h
xlat.o: /usr/include/stdio.h /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
xlat.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
xlat.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
xlat.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
xlat.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
xlat.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
xlat.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
xlat.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h xlat.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
