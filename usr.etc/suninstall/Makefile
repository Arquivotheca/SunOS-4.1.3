#
#	Makefile for SunOS 4.x/MLS installation utilities
#
#       @(#)Makefile 1.1 92/07/30 SMI
#
#       Copyright (c) 1989 by Sun Microsystems, Inc.
#
include Makefile.master

# The -s strips the final executables
CFLAGS=		 -s $(G_CFLAGS) $(INCPATH) 
CDROM_MOUNT_PT=	$(TOOL_DIR)/tar
DESTDIR=
EXECS=		$(EXEC_NSTD) $(EXEC_STD)
FILES=		install.bin
ETC_DIR=	/etc/install
ETC_FILES=	default_sys_info default_client_info OWrelease
ETC_EXC_FILES=	EXCLUDELIST category.standalone category.repreinstalled \
				label_script
EXEC_CD_TAR=	cd_tar
EXEC_NSTD=      add_client config_host easyinstall get_arch_info  \
		get_client_info get_disk_info get_host_info get_sec_info \
		get_software_info run_configure suninstall sys-unconfig \
		toc_xlat verify_diskette extract_diskette xdrtoc \
		add_user xxx bpgetfile
EXEC_STD=	$(OBJ_STD:.o=)
# KARCH is for getcons, needed for correct romvec
KARCH:sh = arch -k
EXEC_ASP=	getcons
SHFILES=	install_small_kernel extract_files
HFILES=		client_impl.h disk_impl.h host_impl.h install.h main_impl.h \
		sec_impl.h soft_impl.h unpack_help.h unpack.h media.h
INCPATH=	-I$(MENU_DIR) -I../../include 
LIBS=		$(LIB_INS) $(LIB_MENU) $(LIB_XDR)
LIB_INS=	lib/libinstall.a
LIB_MENU=	$(MENU_DIR)/libmenu.a
#LIB_STD=	/usr/lib/libc.a
LIB_XDR=	$(MKTP_DIR)/xdr.a
LINTFLAGS=	$(G_LINTFLAGS)
LINT_INS=	lib/llib-linstall.ln
LINT_MENU=	$(MENU_DIR)/llib-lmenu.ln
#
#	MENU_DIR and MKTP_DIR must be relative path names
#
MENU_DIR=	libmenu
MENU_HFILES=	$(MENU_DIR)/menu.h
MKTP_DIR=	../../sundist/mktp
MKTP_HFILES=	$(MKTP_DIR)/mktp.h $(MKTP_DIR)/toc.h
OBJS=		$(OBJ_ARCH) $(OBJ_CD_TAR) $(OBJ_CLIENT) $(OBJ_DISK) \
		$(OBJ_EASY) $(OBJ_HOST) $(OBJ_SEC) $(OBJ_SOFT) $(OBJ_STD) \
		$(OBJ_SUNINS) menu_stub.o toc_xlat.o 
OBJ_ARCH=	arch_form.o 
OBJ_CD_TAR=	cd_tar.o
OBJ_CLIENT=	get_client_info.o client_form.o
OBJ_DISK=	get_disk_info.o disk_form.o
OBJ_HOST=	get_host_info.o host_form.o
OBJ_SEC=	get_sec_info.o sec_form.o
OBJ_STD=	add_services.o installation.o rm_client.o
OBJ_SOFT=	get_software_info.o soft_form.o 
OBJ_SUNINS=	getdate.o main.o main_menu.o tz_menu.o
OBJ_EASY=	unpack.o unpack_timezone.o \
		unpack_tape_util.o  getdate.o
SRCS=		$(SRC_ARCH) $(SRC_CD_TAR) $(SRC_CLIENT) $(SRC_DISK) \
		$(SRC_EASY) $(SRC_HOST) $(SRC_SEC) $(SRC_SOFT) $(SRC_STD) \
		$(SRC_SUNINS) menu_stub.c toc_xlat.c 
SRC_ARCH=	$(OBJ_ARCH:.o=.c)
SRC_CD_TAR=	$(OBJ_CD_TAR:.o=.c)
SRC_CLIENT=	$(OBJ_CLIENT:.o=.c)
SRC_DISK=	$(OBJ_DISK:.o=.c)
SRC_HOST=	$(OBJ_HOST:.o=.c)
SRC_SEC=	$(OBJ_SEC:.o=.c)
SRC_SOFT=	$(OBJ_SOFT:.o=.c)
SRC_STD=	$(OBJ_STD:.o=.c)
SRC_SUNINS=	$(OBJ_SUNINS:.o=.c)
SRC_EASY=	$(OBJ_EASY:.o=.c)
TOOL_DIR=	/usr/etc/install
ASP_DIR=	/usr/kvm


all:		$(HFILES) $(LIBS) $(EXECS) $(EXEC_CD_TAR) $(FILES) \
		$(ETC_FILES) $(ETC_EXC_FILES) $(SHFILES) $(EXEC_ASP)

build:		build_$(SOFT_ARCH)

build_4.x:	FRC master_4.x
		rm -f Makefile.master
		ln -s master_4.x Makefile.master
		$(MAKE) $(MFLAGS) clean
		/bin/time $(MAKE) $(MFLAGS) lint > lint.log 2>&1
		/bin/time $(MAKE) $(MFLAGS) all > make.log 2>&1

build_MLS:	FRC master_MLS
		rm -f Makefile.master
		ln -s master_MLS Makefile.master
		$(MAKE) $(MFLAGS) clean
		/bin/time $(MAKE) $(MFLAGS) lint > lint.log 2>&1
		/bin/time $(MAKE) $(MFLAGS) all > make.log 2>&1

build_all:
		$(MAKE) $(MFLAGS) build_4.x
		install -d 4.x
		mv lint.log make.log *.lint 4.x
		mv $(EXECS) 4.x
		mv *.o 4.x
		install -d lib/4.x
		(cd lib; mv lint.log *.ln 4.x; \
			 mv *.o *.a 4.x)
		$(MAKE) $(MFLAGS) build_MLS

#
#	The following are standard targets:
#	
$(EXEC_STD) $(EXEC_CD_TAR): $$@.o menu_stub.o  $(LIB_INS)
		$(CC) $(CFLAGS) $@.o menu_stub.o -o $@ $(LIB_INS) \
			$(LIB_STD)

#
#	The following files have explicit make lines because they require
#	special handling
#
add_client: $$@.o $(LIB_INS) menu_stub.o
		$(CC) $(CFLAGS) $@.o menu_stub.o -o $@ \
			$(LIB_INS)   -lcurses -ltermlib \
			$(LIB_STD)

get_client_info: $(OBJ_CLIENT) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_CLIENT) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)
 
getcons:	getcons.c 
	$(CC) -D$(KARCH) $(CFLAGS) -I../../sys -o $@ getcons.c
 
get_disk_info:	$(OBJ_DISK) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_DISK) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)
 
get_host_info:	$(OBJ_HOST) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_HOST) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)
 
get_sec_info:	$(OBJ_SEC) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_SEC) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)
 
get_software_info: $(OBJ_SOFT) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_SOFT) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)
 
get_arch_info: $(OBJ_ARCH) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_ARCH) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)

suninstall:	$(OBJ_SUNINS) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_SUNINS) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)

easyinstall:	$(OBJ_EASY) $(LIB_INS) $(LIB_MENU)
		$(CC) $(CFLAGS) $(OBJ_EASY) -o $@ \
			$(LIB_INS) $(LIB_MENU) -lcurses -ltermlib \
			$(LIB_STD)

toc_xlat:	menu_stub.o toc_xlat.o $(LIB_INS) $(LIB_XDR)
		$(CC) $(CFLAGS) toc_xlat.o -o $@ \
			$(LIB_INS) $(LIB_XDR) $(LIB_MENU) -lcurses -ltermlib\
			$(LIB_STD)

xdrtoc: $(MKTP_DIR)/xdrtoc
	cp $(MKTP_DIR)/xdrtoc $@

$(MKTP_DIR)/xdrtoc:
	cd $(MKTP_DIR); $(MAKE) $(@F)

# these two diskette files should become one file and links to two names
verify_diskette: verify_diskette.c $(LIB_INS)
		$(CC) $(CFLAGS) verify_diskette.c -o $@ $(LIB_INS)

extract_diskette: verify_diskette.c extract_diskette.c $(LIB_INS)
		$(CC) $(CFLAGS) extract_diskette.c -o $@ $(LIB_INS)

$(ETC_FILES):	
		sccs get $(ETC_FILES)

$(ETC_EXC_FILES):
		sccs get $(@F)

 
toc_xlat.o toc_xlat.lint depend := INCPATH += -I$(MKTP_DIR)
 
# for the 4.1.3 hidden 5.x preinstall stub loader
bpgetfile: bpgetfile.c
	$(CC) $(CFLAGS) -O -DOLDOS=1 -Drpc_call=callrpc -Drpc_broadcast=clnt_broadcast \
		bpgetfile.c -lrpcsvc -o bpgetfile

xxx: xxx.c
	${CC} -Bstatic ${CFLAGS} -o xxx xxx.c

###############################
#
# Libraries
#

$(LIB_INS):	FRC
		cd $(@D); \
		$(MAKE) $(MFLAGS) G_CFLAGS="$(G_CFLAGS)" \
			MENU_DIR=../$(MENU_DIR) \
			SOFT_ARCH=$(SOFT_ARCH) lib

$(LIB_MENU):	FRC
		cd $(@D); $(MAKE) $(MFLAGS) $(@F)

$(LIB_XDR):	FRC
		cd $(@D); $(MAKE) $(MFLAGS) $(@F) CC=cc INCS=

###############################
#
# External header file targets:
#

$(MENU_HFILES):
		cd $(@D); sccs get $(@F)

$(MKTP_HFILES):
		cd $(@D); sccs get $(@F)

###############################
#
# Administrative targets:
#

lint:		lib.lint libmenu.lint \
		add_client.lint add_services.lint installation.lint \
		get_client_info.lint get_disk_info.lint get_host_info.lint \
		get_sec_info.lint get_software_info.lint suninstall.lint \
		easyinstall.lint toc_xlat.lint get_arch_info.lint cd_tar.lint

lib.lint: $(HFILES)
		cd lib; $(MAKE) $(MFLAGS) G_LINTFLAGS="$(G_LINTFLAGS)" \
			MENU_DIR=../$(MENU_DIR) \
			SOFT_ARCH=$(SOFT_ARCH) lint

libmenu.lint:
		cd libmenu; $(MAKE) $(MFLAGS) lint

$(LINT_INS):	$(HFILES) $(MENU_HFILES) FRC
		cd $(@D); $(MAKE) $(MFLAGS) G_LINTFLAGS="$(G_LINTFLAGS)" \
			MENU_DIR=../$(MENU_DIR) \
			SOFT_ARCH=$(SOFT_ARCH) $(@F)

$(LINT_MENU): $(MENU_HFILES) FRC
		cd $(@D); $(MAKE) $(MFLAGS) $(@F)

add_client.lint: $(LINT_INS) $(LINT_MENU) add_client.c menu_stub.c
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			add_client.c menu_stub.c > $@

add_services.lint: $(LINT_INS) $(LINT_MENU) add_services.c menu_stub.c
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			add_services.c menu_stub.c > $@

cd_tar.lint: $(LINT_INS) $(LINT_MENU) $(SRC_CD_TAR)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_ARCH) > $@

get_arch_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_ARCH)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_ARCH) > $@

get_client_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_CLIENT)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_CLIENT) > $@

get_disk_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_DISK)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_DISK) > $@

get_host_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_HOST)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_HOST) > $@

get_sec_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_SEC)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_SEC) > $@

get_software_info.lint: $(LINT_INS) $(LINT_MENU) $(SRC_SOFT)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_SOFT) > $@

installation.lint: $(LINT_INS) $(LINT_MENU) installation.c menu_stub.c
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			installation.c menu_stub.c > $@

suninstall.lint: $(LINT_INS) $(LINT_MENU) $(SRC_SUNINS)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_SUNINS) > $@

easyinstall.lint: $(LINT_INS) $(LINT_MENU) $(SRC_EASY)
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses -ltermlib \
			$(LINT_INS) $(LINT_MENU) \
			$(SRC_EASY) > $@

toc_xlat.lint: $(MKTP_HFILES) $(LINT_INS) $(LINT_MENU) menu_stub.c toc_xlat.c
		$(LINT) $(LINTFLAGS) $(INCPATH) -lcurses \
			$(LINT_INS) $(LINT_MENU) \
			menu_stub.c toc_xlat.c > $@

install: 	all
		install -d -m 755 $(DESTDIR)$(TOOL_DIR)
		install -d -m 755 $(DESTDIR)$(ASP_DIR)
		for i in $(EXECS) $(FILES) $(SHFILES); do \
			(install -c -m 755 $$i ${DESTDIR}$(TOOL_DIR)/$$i); \
		done
		for i in $(EXEC_ASP) ; do \
			(install -c -m 755 $$i ${DESTDIR}$(ASP_DIR)/$$i; \
			chgrp kmem ${DESTDIR}$(ASP_DIR)/$$i; \
			chmod g+s ${DESTDIR}$(ASP_DIR)/$$i); \
		done
		-(ln -s easyinstall ${DESTDIR}$(TOOL_DIR)/sys-config)
		-(ln -s extract_diskette ${DESTDIR}$(TOOL_DIR)/show_diskette)
		-(ln -s extract_files ${DESTDIR}$(TOOL_DIR)/list_files)
		-(ln -s easyinstall ${DESTDIR}$(TOOL_DIR)/re-preinstall)
		install -d -m 755 ${DESTDIR}$(CDROM_MOUNT_PT)
		install -d -m 755 $(DESTDIR)$(ETC_DIR)
		for i in $(ETC_FILES) $(ETC_EXC_FILES); do \
			(install -c -m 755 $$i ${DESTDIR}$(ETC_DIR)/$$i); \
		done



clean:
		rm -f $(EXECS) $(OBJS) $(FILES) $(ETC_FILES) \
		$(ETC_EXC_FILES) $(SHFILES)
		rm -f getdate.c *.lint
		cd lib; $(MAKE) $(MFLAGS) clean
		cd libmenu; $(MAKE) $(MFLAGS) clean

depend: 	$(SRCS) $(HFILES) $(MENU_HFILES) $(MKTP_HFILES)
		rm -f makedep
		for i in $(SRCS); do \
			$(CC) -M $(INCPATH) $$i | \
			sed -e 's#/sun[1-9][0-9]*[a-zA-Z]*/#/machine/#g' | \
			sort |  uniq | \
			awk ' \
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
			END { print rec }'; \
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

FRC:

.DEFAULT:
		sccs get $@

.PRECIOUS:	$(LIB_INS) $(LIB_MENU) $(LIB_XDR) $(LINT_INS) $(LINT_MENU)

# DO NOT DELETE THIS LINE -- make depend uses it
add_services.o: ./install.h ./media.h /usr/include/errno.h
add_services.o: /usr/include/machine/param.h /usr/include/stdio.h
add_services.o: /usr/include/string.h /usr/include/sun/dkio.h
add_services.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
add_services.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
add_services.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
add_services.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
add_services.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
add_services.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
add_services.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
add_services.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_services.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
add_services.o: /usr/include/sysexits.h /usr/include/vm/faultcode.h
add_services.o: add_services.c libmenu/menu.h
arch_form.o: ./install.h ./media.h ./soft_impl.h /usr/include/errno.h
arch_form.o: /usr/include/machine/param.h /usr/include/stdio.h
arch_form.o: /usr/include/string.h /usr/include/sun/dkio.h
arch_form.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
arch_form.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
arch_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
arch_form.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
arch_form.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
arch_form.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
arch_form.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
arch_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
arch_form.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
arch_form.o: /usr/include/vm/faultcode.h arch_form.c libmenu/menu.h
cd_tar.o: ./install.h /usr/include/errno.h /usr/include/machine/param.h
cd_tar.o: /usr/include/stdio.h /usr/include/string.h /usr/include/sun/dkio.h
cd_tar.o: /usr/include/sun/dklabel.h /usr/include/sys/dir.h
cd_tar.o: /usr/include/sys/errno.h /usr/include/sys/filio.h
cd_tar.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
cd_tar.o: /usr/include/sys/param.h /usr/include/sys/signal.h
cd_tar.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
cd_tar.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
cd_tar.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
cd_tar.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
cd_tar.o: /usr/include/sys/types.h /usr/include/unistd.h
cd_tar.o: /usr/include/vm/faultcode.h cd_tar.c
client_form.o: ./client_impl.h ./install.h /usr/include/errno.h
client_form.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
client_form.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
client_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
client_form.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
client_form.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
client_form.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
client_form.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
client_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
client_form.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
client_form.o: /usr/include/vm/faultcode.h client_form.c libmenu/menu.h
disk_form.o: ./disk_impl.h ./install.h /usr/include/machine/param.h
disk_form.o: /usr/include/stdio.h /usr/include/string.h /usr/include/sun/dkio.h
disk_form.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
disk_form.o: /usr/include/sys/file.h /usr/include/sys/filio.h
disk_form.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disk_form.o: /usr/include/sys/param.h /usr/include/sys/signal.h
disk_form.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
disk_form.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
disk_form.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
disk_form.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
disk_form.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h disk_form.c
disk_form.o: libmenu/menu.h libmenu/menu_impl.h
get_client_info.o: ./client_impl.h ./install.h /usr/include/arpa/inet.h
get_client_info.o: /usr/include/machine/param.h /usr/include/net/if.h
get_client_info.o: /usr/include/net/if_arp.h /usr/include/netdb.h
get_client_info.o: /usr/include/netinet/if_ether.h /usr/include/netinet/in.h
get_client_info.o: /usr/include/stdio.h /usr/include/string.h
get_client_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_client_info.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
get_client_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_client_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_client_info.o: /usr/include/sys/signal.h /usr/include/sys/socket.h
get_client_info.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
get_client_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_client_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_client_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_client_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_client_info.o: get_client_info.c libmenu/menu.h
get_disk_info.o: ./disk_impl.h ./install.h /usr/include/machine/param.h
get_disk_info.o: /usr/include/stdio.h /usr/include/string.h
get_disk_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_disk_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_disk_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_disk_info.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
get_disk_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_disk_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_disk_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_disk_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_disk_info.o: get_disk_info.c libmenu/menu.h
get_host_info.o: ./host_impl.h ./install.h /usr/include/ctype.h
get_host_info.o: /usr/include/machine/param.h /usr/include/net/if.h
get_host_info.o: /usr/include/stdio.h /usr/include/string.h
get_host_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_host_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_host_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_host_info.o: /usr/include/sys/signal.h /usr/include/sys/socket.h
get_host_info.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
get_host_info.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
get_host_info.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_host_info.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
get_host_info.o: /usr/include/vm/faultcode.h get_host_info.c libmenu/menu.h
get_sec_info.o: ./install.h ./sec_impl.h /usr/include/machine/param.h
get_sec_info.o: /usr/include/stdio.h /usr/include/string.h
get_sec_info.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
get_sec_info.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_sec_info.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
get_sec_info.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
get_sec_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_sec_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_sec_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_sec_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_sec_info.o: get_sec_info.c libmenu/menu.h
get_software_info.o: ./install.h ./media.h ./soft_impl.h
get_software_info.o: /usr/include/machine/param.h /usr/include/stdio.h
get_software_info.o: /usr/include/string.h /usr/include/sun/dkio.h
get_software_info.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
get_software_info.o: /usr/include/sys/file.h /usr/include/sys/filio.h
get_software_info.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_software_info.o: /usr/include/sys/param.h /usr/include/sys/signal.h
get_software_info.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
get_software_info.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
get_software_info.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_software_info.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
get_software_info.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
get_software_info.o: get_software_info.c libmenu/menu.h
getdate.o: /usr/include/ctype.h /usr/include/sys/stdtypes.h
getdate.o: /usr/include/ctype.h /usr/include/sys/stdtypes.h
getdate.o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
getdate.o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
getdate.o: /usr/include/time.h getdate.c
getdate.o: /usr/include/time.h getdate.c
host_form.o: ./host_impl.h ./install.h /usr/include/machine/param.h
host_form.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
host_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
host_form.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
host_form.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
host_form.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
host_form.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
host_form.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
host_form.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h host_form.c
host_form.o: libmenu/menu.h
installation.o: ./install.h /usr/include/ctype.h /usr/include/errno.h
installation.o: /usr/include/machine/param.h /usr/include/stdio.h
installation.o: /usr/include/string.h /usr/include/sun/dkio.h
installation.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
installation.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
installation.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
installation.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
installation.o: /usr/include/sys/reboot.h /usr/include/sys/signal.h
installation.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
installation.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
installation.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
installation.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
installation.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
installation.o: installation.c libmenu/menu.h
main.o: ./install.h ./main_impl.h /usr/include/machine/param.h
main.o: /usr/include/stdio.h /usr/include/string.h /usr/include/sun/dkio.h
main.o: /usr/include/sun/dklabel.h /usr/include/sys/fcntlcom.h
main.o: /usr/include/sys/file.h /usr/include/sys/filio.h
main.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
main.o: /usr/include/sys/param.h /usr/include/sys/signal.h
main.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
main.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
main.o: /usr/include/sys/time.h /usr/include/sys/ttold.h
main.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
main.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
main.o: /usr/include/sysexits.h /usr/include/time.h /usr/include/vm/faultcode.h
main.o: libmenu/menu.h main.c
main_menu.o: ./install.h ./main_impl.h /usr/include/machine/param.h
main_menu.o: /usr/include/signal.h /usr/include/stdio.h /usr/include/sun/dkio.h
main_menu.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
main_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
main_menu.o: /usr/include/sys/param.h /usr/include/sys/signal.h
main_menu.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
main_menu.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
main_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
main_menu.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
main_menu.o: /usr/include/vm/faultcode.h libmenu/menu.h libmenu/menu_impl.h
main_menu.o: main_menu.c
menu_stub.o: ./install.h /usr/include/machine/param.h /usr/include/stdio.h
menu_stub.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
menu_stub.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
menu_stub.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
menu_stub.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
menu_stub.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
menu_stub.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
menu_stub.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
menu_stub.o: /usr/include/sys/types.h /usr/include/varargs.h
menu_stub.o: /usr/include/vm/faultcode.h menu_stub.c
rm_client.o: ./install.h /usr/include/dirent.h /usr/include/machine/param.h
rm_client.o: /usr/include/net/if.h /usr/include/net/if_arp.h
rm_client.o: /usr/include/netdb.h /usr/include/netinet/if_ether.h
rm_client.o: /usr/include/netinet/in.h /usr/include/stdio.h
rm_client.o: /usr/include/strings.h /usr/include/sun/dkio.h
rm_client.o: /usr/include/sun/dklabel.h /usr/include/sys/dirent.h
rm_client.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
rm_client.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
rm_client.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
rm_client.o: /usr/include/sys/signal.h /usr/include/sys/socket.h
rm_client.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
rm_client.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
rm_client.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
rm_client.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
rm_client.o: /usr/include/sys/types.h /usr/include/sysexits.h
rm_client.o: /usr/include/vm/faultcode.h rm_client.c
sec_form.o: ./install.h ./sec_impl.h /usr/include/errno.h
sec_form.o: /usr/include/machine/param.h /usr/include/string.h
sec_form.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
sec_form.o: /usr/include/sys/audit.h /usr/include/sys/errno.h
sec_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
sec_form.o: /usr/include/sys/ioctl.h /usr/include/sys/label.h
sec_form.o: /usr/include/sys/param.h /usr/include/sys/signal.h
sec_form.o: /usr/include/sys/sockio.h /usr/include/sys/stat.h
sec_form.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
sec_form.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
sec_form.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
sec_form.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h libmenu/menu.h
sec_form.o: sec_form.c
soft_form.o: ./install.h ./media.h ./soft_impl.h /usr/include/errno.h
soft_form.o: /usr/include/machine/param.h /usr/include/stdio.h
soft_form.o: /usr/include/string.h /usr/include/sun/dkio.h
soft_form.o: /usr/include/sun/dklabel.h /usr/include/sys/errno.h
soft_form.o: /usr/include/sys/fcntlcom.h /usr/include/sys/file.h
soft_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
soft_form.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
soft_form.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
soft_form.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
soft_form.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
soft_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
soft_form.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
soft_form.o: /usr/include/vm/faultcode.h libmenu/menu.h libmenu/menu_impl.h
soft_form.o: soft_form.c
toc_xlat.o: ../../sundist/mktp/mktp.h ../../sundist/mktp/toc.h
toc_xlat.o: ../../sundist/mktp/xdr_custom.h ./install.h /usr/include/ctype.h
toc_xlat.o: /usr/include/machine/param.h /usr/include/malloc.h
toc_xlat.o: /usr/include/netinet/in.h /usr/include/rpc/auth.h
toc_xlat.o: /usr/include/rpc/auth_des.h /usr/include/rpc/auth_unix.h
toc_xlat.o: /usr/include/rpc/clnt.h /usr/include/rpc/rpc.h
toc_xlat.o: /usr/include/rpc/rpc_msg.h /usr/include/rpc/svc.h
toc_xlat.o: /usr/include/rpc/svc_auth.h /usr/include/rpc/types.h
toc_xlat.o: /usr/include/rpc/xdr.h /usr/include/stdio.h /usr/include/sun/dkio.h
toc_xlat.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
toc_xlat.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
toc_xlat.o: /usr/include/sys/param.h /usr/include/sys/signal.h
toc_xlat.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
toc_xlat.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
toc_xlat.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
toc_xlat.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
toc_xlat.o: /usr/include/sys/types.h /usr/include/time.h
toc_xlat.o: /usr/include/vm/faultcode.h toc_xlat.c
tz_menu.o: ./install.h ./main_impl.h /usr/include/machine/param.h
tz_menu.o: /usr/include/string.h /usr/include/sun/dkio.h
tz_menu.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
tz_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
tz_menu.o: /usr/include/sys/param.h /usr/include/sys/signal.h
tz_menu.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
tz_menu.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
tz_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
tz_menu.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
tz_menu.o: /usr/include/vm/faultcode.h libmenu/menu.h tz_menu.c
unpack.o: ./install.h ./media.h ./unpack.h ./unpack_help.h /usr/include/ctype.h
unpack.o: /usr/include/curses.h /usr/include/dirent.h /usr/include/errno.h
unpack.o: /usr/include/machine/param.h /usr/include/net/if.h
unpack.o: /usr/include/sgtty.h /usr/include/stdio.h /usr/include/strings.h
unpack.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
unpack.o: /usr/include/sys/dirent.h /usr/include/sys/errno.h
unpack.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
unpack.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
unpack.o: /usr/include/sys/reboot.h /usr/include/sys/signal.h
unpack.o: /usr/include/sys/socket.h /usr/include/sys/sockio.h
unpack.o: /usr/include/sys/stat.h /usr/include/sys/stdtypes.h
unpack.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
unpack.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
unpack.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
unpack.o: /usr/include/sys/types.h /usr/include/sys/wait.h /usr/include/time.h
unpack.o: /usr/include/vm/faultcode.h unpack.c
unpack_tape_util.o: ./install.h ./media.h ./unpack.h
unpack_tape_util.o: /usr/include/machine/param.h /usr/include/sun/dkio.h
unpack_tape_util.o: /usr/include/sun/dklabel.h /usr/include/sys/filio.h
unpack_tape_util.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
unpack_tape_util.o: /usr/include/sys/param.h /usr/include/sys/signal.h
unpack_tape_util.o: /usr/include/sys/sockio.h /usr/include/sys/stdtypes.h
unpack_tape_util.o: /usr/include/sys/sysmacros.h /usr/include/sys/ttold.h
unpack_tape_util.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
unpack_tape_util.o: /usr/include/sys/ttydev.h /usr/include/sys/types.h
unpack_tape_util.o: /usr/include/vm/faultcode.h unpack_tape_util.c
unpack_timezone.o: ./install.h ./unpack.h ./unpack_help.h /usr/include/curses.h
unpack_timezone.o: /usr/include/machine/param.h /usr/include/machine/setjmp.h
unpack_timezone.o: /usr/include/setjmp.h /usr/include/sgtty.h
unpack_timezone.o: /usr/include/signal.h /usr/include/stdio.h
unpack_timezone.o: /usr/include/sun/dkio.h /usr/include/sun/dklabel.h
unpack_timezone.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
unpack_timezone.o: /usr/include/sys/ioctl.h /usr/include/sys/param.h
unpack_timezone.o: /usr/include/sys/signal.h /usr/include/sys/sockio.h
unpack_timezone.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
unpack_timezone.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
unpack_timezone.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
unpack_timezone.o: /usr/include/sys/types.h /usr/include/vm/faultcode.h
unpack_timezone.o: unpack_timezone.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
