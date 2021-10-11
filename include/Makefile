#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.3 83/07/10
#
# Doing a make install builds /usr/include
#
# Define SHARED to indicate whether you want
# symbolic links to the system source (``symlinks''),
# or a separate copy (``copies'').
# (latter useful in environments where it's
# not possible to keep /sys publicly readable)
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#
ARCH:sh = arch -k
DESTDIR=
CHOWN=/usr/etc/chown
SUBDIRS=arpa protocols
#
SUNVIEW_SUBDIRS=suntool sunwindow
STD=	a.out.h alloca.h ar.h auevents.h \
	dbm.h dirent.h disktab.h errno.h exportent.h \
	floatingpoint.h fstab.h ftw.h grp.h grpadj.h\
	langinfo.h lastlog.h link.h \
	locale.h math.h memory.h mntent.h mon.h mp.h \
	nan.h ndbm.h netdb.h\
	nlist.h prof.h pwdadj.h \
	ranlib.h rasterfile.h regexp.h resolv.h \
	search.h setjmp.h sgtty.h signal.h stab.h stddef.h string.h \
	strings.h struct.h sysexits.h tar.h ttyent.h tzfile.h \
	ustat.h utime.h utmp.h values.h vfont.h vfork.h
LINKS=	limits.h poll.h stropts.h syscall.h syslog.h termios.h \
	termio.h unistd.h varargs.h
MACHINE=sun
LIBDIRS=cgi cgi77 core curses kvm pc pixrect rpcsvc
# BUILTDIR is for stuff that requires system directories to be installed
# because this stuff is compiled and includes system .h files.
# This is a tricky one if there are dependencies on the C compiler.
BUILTDIR=lwp
NETDIRS=net netinet
SYSDIRS=${NETDIRS} nfs ufs debug m68k sys vm tfs
MACHINEDIRS=sun sun3 sun3x sun4 sun4c sun4m sundev sunif
RPCDIRS=rpc yp des
SYS=../sys
MON=${SYS}/stand/mon
SHARED=	copies
UCBDIR=../ucbinclude
SYSVDIR=../5include

all: ${STD}

install: ${SHARED} ${STD}
	install -d -o bin -m 755 ${DESTDIR}/usr/include
	install -d -o bin -m 755 ${DESTDIR}/usr/5include
	install -m 444 ${STD} ${DESTDIR}/usr/include
	for i in ${SUBDIRS}; do \
		( cd $$i; \
		$(MAKE) ${MFLAGS} install DESTDIR=$(DESTDIR) CHOWN=$(CHOWN) ); \
	done
	for i in ${LIBDIRS}; do \
		( cd ../usr.lib/lib$$i; \
		$(MAKE) ${MFLAGS} install_h DESTDIR=$(DESTDIR) CHOWN=$(CHOWN));\
	done
	for i in ${SUNVIEW_SUBDIRS}; do \
		( cd ../usr.lib/lib$$i; \
		$(MAKE) ${MFLAGS} install_h DESTDIR=${DESTDIR} CHOWN=$(CHOWN));\
	done
	for i in ${RPCDIRS}; do \
		( cd ../lib/libc/$$i; \
		$(MAKE) ${MFLAGS} install DESTDIR=${DESTDIR} CHOWN=$(CHOWN) ); \
	done
	for i in ${LINKS}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	cd ../usr.bin/sunwindows/images; \
		$(MAKE) ${MFLAGS} install DESTDIR=$(DESTDIR) CHOWN=$(CHOWN);
	install -d ${DESTDIR}/usr/kvm
	rm -f ${DESTDIR}/usr/kvm/machine;
#
# bug#1054689.  This link is temporary.  We remove it later and use
# a link to ./sys/${ARCH}.
#
	ln -s ../include/${ARCH} ${DESTDIR}/usr/kvm/machine;
	rm -f ${DESTDIR}/usr/include/machine;
	ln -s ../kvm/machine ${DESTDIR}/usr/include/machine;
	rm -f ${DESTDIR}/usr/include/frame.h;
	ln -s machine/frame.h ${DESTDIR}/usr/include/frame.h
	cd $(UCBDIR);\
		$(MAKE) install DESTDIR=$(DESTDIR);
	cd $(SYSVDIR);\
		$(MAKE) install DESTDIR=$(DESTDIR);
	for i in ${BUILTDIR}; do \
		( cd ../usr.lib/lib$$i; \
		$(MAKE) ${MFLAGS} install_h DESTDIR=$(DESTDIR) CHOWN=$(CHOWN));\
	done

symlinks:
	-for i in ${SYSDIRS} ${MACHINEDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		ln -s ${SYS}/$$i ${DESTDIR}/usr/include/$$i; \
	done
	rm -rf ${DESTDIR}/usr/include/mon
	ln -s ${MON}/h ${DESTDIR}/usr/include/mon

copies:
	cd ${SYS}; \
		$(MAKE) ${MFLAGS} install_h DESTDIR=$(DESTDIR) CHOWN=$(CHOWN) \
			MACHINE=$(MACHINE)

clean:

#
# Kludges to make Makefile run in a empty directory
#
sysexits.h: ../usr.lib/sendmail/include/sysexits.h
	rm -f $@
	ln -s $? $@
