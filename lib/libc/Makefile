#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
# Sources are organized in a hierarchy according to function, OS-type,
# and machine.  At the highest level things with similar function are
# grouped together.  At the next level, if need be, they will be
# organized according to OS-type.  Machine dependent routines are
# located at the lowest level in the directory hierarchy.
#
# Routines common across all OS versions will be located in a commmon
# directory.  Routines compatible with particular OS versions will have
# their own directory, for instance: 4.1, 4.2, and sys5.  Machine
# level routines will be in subdirectories called mc68010, mc68020,
# sparc, i386, and vax.  Directories named m68k hold code common between
# mc68010 and mc68020 machines.
#
# The C startup code is now located in a directory parallel to libc
# since it does not go into libc.  The C run-time code, which is included
# in libc, is always machine dependent is located in crt with the 
# appropriate machine level subdirectories.
#
all:	xall

include Makefile.arch
include Makefile.master
MAKE_INCLUDE = Makefile.arch Makefile.master $(VERS$(TARGET_MACH)) install_libc.sh
INSTALL = install
DEBUGDIRS = gen/common
4.2DIRS = compat/4.1 gen/4.2 stdio/4.2 sys/4.2
SYS5DIRS = compat/sys5 gen/sys5 stdio/sys5 sys/sys5
COMMDIRS = compat/common crt des gen/common inet net rpc yp stdio/common sys/common
FASTDIRS = gen/common
ALLDIRS = $(4.2DIRS) $(SYS5DIRS) $(COMMDIRS)
GLOBAL_DATA = ctype_.o  errlst.o iob.o ieee_globals.o optind.o \
	rpc_commondata. siglist.o 
GLOBAL_DATA-mc68020 = errno.o fp_commons.o
GLOBAL_DATA-mc68010 = $(GLOBAL_DATA-mc68020)
GLOBAL_DATA-sparc = errno.o

# NOTE - Major version numbers are machine dependent. Furthermore the major
# numbers of the System 5 and Berkeley 4.2 based libraries will differ by
# one
VERS-mc68010 = vers-m68k
VERS-mc68020 = vers-m68k
VERS-sparc =  vers-sparc

libc_comm.so := SUBDIRS = $(COMMDIRS)
libc.so	:= SUBDIRS = $(4.2DIRS) gen/common stdio/common  
libc.a libc_p.a := SUBDIRS = $(4.2DIRS) $(COMMDIRS)
libcs5.so := SUBDIRS = $(SYS5DIRS) gen/common stdio/common 
libcs5.a libcs5_p.a := SUBDIRS = $(SYS5DIRS) $(COMMDIRS)
libbsdmalloc.a := SUBDIRS = $(FASTDIRS)

libc_comm.so	:= DIRS = $(SUBDIRS:%=%/shared/*.o)
libc.so		:= DIRS = $(SUBDIRS:%=%/shared_4.2/*.o)
libc.a 		:= DIRS = $(4.2DIRS:%=%/obj/*.o) $(COMMDIRS:%=%/obj/*.o) 
libc_p.a 	:= DIRS = $(4.2DIRS:%=%/obj_p/*.o) $(COMMDIRS:%=%/obj_p/*.o)
libcs5.a 	:= DIRS = $(SYS5DIRS:%=%/s5/*.o) $(COMMDIRS:%=%/s5/*.o)
libcs5_p.a 	:= DIRS = $(SYS5DIRS:%=%/s5_p/*.o) $(COMMDIRS:%=%/s5_p/*.o)
libcs5.so	:= DIRS = $(SUBDIRS:%=%/shared_s5/*.o)
libbsdmalloc.a 	:= DIRS = $(FASTDIRS:%=%/obj_f/*.o) 

BUILD = ar rv $(LIB) $?; ranlib $(LIB)

# expect "ln -s ../shared/*.o ../shared_4.2/." to have error with
# filenames collision but the command line is set up to handle it.
libc.so		:= BUILD = rm -rf shared_4.2; mkdir shared_4.2; \
		ln $(DIRS) shared_4.2; \
		cd shared;\
		(ln -s ../shared/*.o ../shared_4.2/. || true); \
		cd ..;\
		xstr -l libc_xstr;\
		$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -R -c -pic xs.c;\
		mv xs.o shared_4.2;\
		$(LD) -Bsymbolic=object_globals -assert pure-text `objsort lorder-sparc shared_4.2` -ldl;\
		mv a.out $(LIB); \
		ar rv libc_pic.a shared_4.2/*.o; ranlib libc_pic.a

libcs5.so	:= BUILD = rm -rf shared_s5; mkdir shared_s5;  \
		ln $(DIRS) shared_s5; \
		cd shared;\
		(ln -s ../shared/*.o ../shared_s5/. || true); \
		cd ..;\
		xstr -l libc_xstr;\
		$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -R -c -pic xs.c;\
		mv xs.o shared_s5;\
		$(LD) -Bsymbolic=object_globals -assert pure-text `objsort lorder-sparc shared_s5` -ldl;\
		mv a.out $(LIB);\
		ar rv libcs5_pic.a shared_s5/*.o; ranlib libcs5_pic.a

libc_comm.so	:= BUILD = rm -rf shared; mkdir shared; ln $(DIRS) shared;

XSTR = strings
MEMBERS =  $(LIB)($(DIRS))
libc.so 	:= MEMBERS = libc_comm.so $(DIRS)
libcs5.so 	:= MEMBERS = libc_comm.so $(DIRS)
libc_comm.so 	:= MEMBERS = $(DIRS)

SHLIBS = shlib.etc/Makefile shlib.etc/README shlib.etc/awkfile

.INIT: $(MAKE_INCLUDE) lorder-sparc objsort object_globals

.PRECIOUS: $(LIBS) 

xall: $(MAKE_INCLUDE) $(LIBS) $(LIBS_SA)

$(LIBS) : $(XSTR) $$(SUBDIRS) $$@_BUILT

LIB_BUILT = $(LIBS:%=%_BUILT)
$(LIB_BUILT) :  $$(MEMBERS)
	$(BUILD)
	touch $@

$$(LIB)(%.o) : %.o
	@true

$$(DIRS):
	@true

$(XSTR) : FRC
	@if [ ! -f strings ]; then \
		make clean;\
	fi

$(4.2DIRS) $(SYS5DIRS) $(COMMDIRS) : FRC
	cd $@; $(MAKE) $(MFLAGS) $(LIB)

libc.sa: libc.a
	ar x libc.a $(GLOBAL_DATA) $(GLOBAL_DATA$(TARGET_MACH)) 
	ar rv $@ $(GLOBAL_DATA) $(GLOBAL_DATA$(TARGET_MACH))
	ranlib $@
	
libc_pic.a : libc.so
	ar rv $@ $(4.2DIRS:%=%/shared_4.2/*.o) $(COMMDIRS:%=%/shared/*.o)
	ranlib $@

libcs5.sa: libcs5.a
	ar x libcs5.a $(GLOBAL_DATA) $(GLOBAL_DATA$(TARGET_MACH)) 
	ar rv $@ $(GLOBAL_DATA) $(GLOBAL_DATA$(TARGET_MACH))
	ranlib $@
	
libcs5_pic.a: libcs5.so
	ar rv $@ $(SYS5DIRS:%=%/shared_s5/*.o) $(COMMDIRS:%=%/shared/*.o);
	ranlib $@

clean:
	-for i in ${ALLDIRS}; do (cd $$i; $(MAKE) clean); done
	-for i in ${DEBUGDIRS}; do (cd $$i/debug; $(MAKE) clean); done
	rm -rf tmp version *.o strings libc_pic.a libcs5_pic.a
	rm -rf libc.a libc_p.a libc.so libc.sa libbsdmalloc.a
	rm -rf libcs5.a libcs5_p.a libcs5.so libcs5.sa libc_comm.so

international:
	$(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) IFLAGS="$(IFLAGS)" \
	    LDFLAGS="$(LDFLAGS)" -f Makefile.inter

install: $(VERS$(TARGET_MACH)) $(SHLIBS)
	${INSTALL} -d -m 755 ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 lorder-sparc ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} objsort ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 shlib.etc/Makefile ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 shlib.etc/README ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 shlib.etc/awkfile ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 libc_pic.a ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 libcs5_pic.a ${DESTDIR}/usr/lib/shlib.etc
	${INSTALL} -m 644 libcs5.a ${DESTDIR}/usr/5lib/libc.a
	ranlib ${DESTDIR}/usr/5lib/libc.a
	${INSTALL} -m 644 libcs5_p.a ${DESTDIR}/usr/5lib/libc_p.a
	ranlib ${DESTDIR}/usr/5lib/libc_p.a
	${INSTALL} -m 644 libc.a ${DESTDIR}/usr/lib/libc.a
	ranlib ${DESTDIR}/usr/lib/libc.a
	${INSTALL} -m 644 libc_p.a ${DESTDIR}/usr/lib/libc_p.a
	ranlib ${DESTDIR}/usr/lib/libc_p.a
	${INSTALL} -m 644 libbsdmalloc.a ${DESTDIR}/usr/lib/libbsdmalloc.a
	ranlib ${DESTDIR}/usr/lib/libbsdmalloc.a
	if [ -s $(VERS$(TARGET_MACH)) ]; then \
		sh -x install_libc.sh $(INSTALL) $(VERS$(TARGET_MACH)) $(DESTDIR); \
	fi;
	-for i in ${DEBUGDIRS}; do \
		(cd $$i/debug; $(MAKE) install DESTDIR=$(DESTDIR)); \
	 done

FRC:
