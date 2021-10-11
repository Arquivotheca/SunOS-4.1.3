# @(#)Makefile 1.1 92/07/30 SMI

#
# Makefile for /lib/compile (driver for cc, f77, pc, m2c, lint, ...)
#

# -----------------------------------------------------------------------------

#		RELEASE=32 		   for  Release 3.x version.
#		RELEASE=40		   for  Release 4.0 version.
#		RELEASE=41		   for  Release 4.1 version.
RELEASE = 41

#		RELEASE_NAME=3.x	   for  Release 3.x version.
#		RELEASE_NAME=4.0	   for  Release 4.0 version.
#		RELEASE_NAME=4.1	   for  Release 4.1 version.
RELEASE_NAME = 4.1

#		SRCDIR=.                   for  Sys-V  version.
#		SRCDIR=../../lang/compile  for  UCB    version.
SRCDIR  = ../../lang/compile

#		ORIENTATION=               for   default version (/lib/compile).
#		ORIENTATION=-DFORCE_SYS5   for forced Sys-V version.
#		ORIENTATION=-DFORCE_UCB	   for forced  UCB  version.
ORIENTATION  = -DFORCE_UCB

#		VROOT=../vroot             for   Sys-V  version.
#		VROOT=../../lang/vroot     for   UCB    version.
VROOT	= ../../lang/vroot

#		INCLUDE=/usr/include 	   for  regular  3.x/4.0 version.
#		INCLUDE=/usr/5include	   for   Sys-V   3.x/4.0 version
#		INCLUDE=/usr/include	   for   Sys-V/UCB   4.1 version
INCLUDE	= /usr/include

#		LIB=/lib		   for  UCB     3.x  version
#		LIB=/usr/5lib		   for  Sys-V   3.x  version
#		LIB=/usr/lib		   for  UCB     4.0  version
#		LIB=/usr/5lib		   for  Sys-V   4.0  version
#		LIB=/usr/lib		   for  Sys-V   4.1  version
#		LIB=/usr/lib		   for  UCB     4.1  version
LIB	= /usr/lib

#		BIN=/bin 		   for  Release 3.x version.
#		BIN=/usr/bin		   for  Release 4.x version.
BIN	= /usr/bin


# -----------------------------------------------------------------------------

ROOT	=
DESTDIR	=
OWN	= bin
GRP	= bin
MODE	= 755
INSTALL	= install -c
INSTALL_CMD= ${INSTALL} -s -m ${MODE} -o ${OWN} -g ${GRP}
MAKEOBJ	= compile
ARCH	= `/bin/arch`

CPP	= /lib/cpp
LINT	= /usr/bin/lint

#
#		-DRELEASE=<n> is used for making a version to run natively
#		under SunOS release <n> (n=32 for 3.x, or 4x for 4.x).

# INCL_FLAGS are flags for preprocessor file-inclusion, typically -I<whatever>.
INCL_FLAGS= -I$(SRCDIR) -I$(VROOT) $(X_INC) -I${ROOT}${INCLUDE}

# FEATURES specify which features are to be compiled into the driver.
#
#   -DCROSS provides cross-compiler support.
#
#   -DNSE provides runtime support for NSE.
#	-DCROSS is a prerequisite for -DNSE.
#
#   -DBROWSER provides support for the Code Browser.
#	It has no prerequisite features.
#
# We include them all, by default.
FEATURES=  -DCROSS -DNSE -DBROWSER

# xCPPFLAGS are our flags for /lib/cpp.
xCPPFLAGS = ${INCL_FLAGS} -DRELEASE=$(RELEASE) $(ORIENTATION) $(FEATURES) $(X_DEF) -D$(ARCH)

COPTS	= -O
CFLAGS	= $(COPTS) $(xCPPFLAGS) 
LDFLAGS	= 
LIBS	=
LINTFLAGS = -x $(xCPPFLAGS)
xLINKS	= xcc xm2c xf77 xpc xlint
XLINKS	= Xcc Xm2c Xf77 Xpc Xlint


# testing variables (for running tests)
TEST	= run
TESTDIR2= test_dir2
XCROSS=sun2 sun3 sun3x sun4
DRIVERS=cc f77 lint m2c pc


#
#  Driver sources and headers.
#
ROCSRCS	= $(SRCDIR)/ro_data.c
CSRCS	= $(SRCDIR)/compile.c $(SRCDIR)/driver_version.c \
		$(SRCDIR)/rw_data.c $(SRCDIR)/ro_data.c \
		$(SRCDIR)/run_pass.c $(SRCDIR)/setup.c $(SRCDIR)/setup_cc.c \
		$(SRCDIR)/setup_f77.c $(SRCDIR)/setup_lint.c \
		$(SRCDIR)/setup_m2c.c $(SRCDIR)/setup_pc.c
HDRS	= $(SRCDIR)/driver.h $(VROOT)/report.h $(VROOT)/vroot.h

SRCS = $(ROSRCS) $(CSRCS) $(HDRS)

#
# Driver objects.
#
LINTS	= compile.ln rw_data.ln ro_data.ln run_pass.ln setup.ln setup_cc.ln \
		setup_f77.ln setup_lint.ln setup_m2c.ln setup_pc.ln
ROOBJS	= ro_data.o
COBJS	= compile.o driver_version.o rw_data.o run_pass.o setup.o setup_cc.o \
		setup_f77.o setup_lint.o setup_m2c.o setup_pc.o
OBJS	= $(ROOBJS) $(COBJS)

#
#  Production rules for driver.
#
all:	${MAKEOBJ}

# "make bootstrap" can be used to make a native driver, on a pre-SunOS4.1
# system.  See the "boostrap := <etc>" line above.
bootstrap:	${MAKEOBJ}

compile: $(OBJS) $(VROOT)/vroot.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(VROOT)/vroot.a

$(XLINKS) $(xLINKS):
	rm -f $@;  ln -s compile $@ 

xlinks:	$(xLINKS)

Xlinks:	$(XLINKS)

$(COBJS):	$(HDRS)
	$(CC) $(CFLAGS) -c -o $(@F) $(SRCDIR)/`basename $(@F) .o`.c

$(VROOT)/vroot.a $(VROOT)/report.h $(VROOT)/vroot.h:
	cd $(@D); $(MAKE) $(@F)

FRC:

.c.ln:
	$(LINT) $(LINTFLAGS) -i $*.c

# read-only (sharable) data
ro_data.o:	$(SRCDIR)/ro_data.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/ro_data.c -R

lint:	$(LINTS)
	lint $(LINTS) $(LINTFLAGS)

lang_info:	FRC
	rm -f lang_info; \
	echo "# For use by compiler drivers."		 > lang_info; \
	echo "# prod	version	OS-version"		 >>lang_info; \
	echo "c	${RELEASE_NAME}	${RELEASE_NAME}"	 >>lang_info; \
	echo "lint	${RELEASE_NAME}	${RELEASE_NAME}" >>lang_info

install: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)$(LIB)/compile

install_lang_info:  lang_info
	$(INSTALL) -m 644 -o ${OWN} -g ${GRP} lang_info \
			$(DESTDIR)/usr/lib/lang_info

install_cc: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)$(BIN)/cc

install_lint: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)/usr/bin/lint

install_f77: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)/usr/bin/f77
	@echo "**Must add \"fortran\" entry to $(DESTDIR)/usr/lib/lang_info**"

install_m2c: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)/usr/bin/m2c
	@echo "**Must add \"modula2\" entry to $(DESTDIR)/usr/lib/lang_info**"

install_pc: ${MAKEOBJ}  install_lang_info
	$(INSTALL_CMD) -s compile $(DESTDIR)/usr/ucb/pc
	@echo "**Must add \"pascal\" entry to $(DESTDIR)/usr/lib/lang_info**"

tags:  $(SRCS) $(VROOT)/*.c
	ctags $(SRCS) $(VROOT)/*.c


# Run tests for /lib/compile
test:	build	
	@cd ${TESTDIR2} ; \
	make ${TEST} RELEASE=${RELEASE} DRIVERS="${DRIVERS}" XCROSS="${XCROSS}"

# Build tests for /lib/compile
build:
	@if [ ! -d ${TESTDIR2} ] ; then mkdir ${TESTDIR2} ; fi
	@if [ ! -h ${TESTDIR2}/SCCS ] ; then ln -s ../SCCS/../${TESTDIR2}/SCCS ${TESTDIR2}/SCCS ; fi

#
# Miscellaneous production rules.
#

clean:
	rm -f $(OBJS) compile ocompile $(LINTS) *.BAK tags
	rm -f gen gen.o syscall.a
# leave the links in for testing, they don't cause problems
#	rm -f $(xLINKS) $(XLINKS)

depend: $(SRCS)
	rm -f makedep
	for i in $(CSRCS); do \
	    ( $(CPP) -M $(xCPPFLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

compile.o: ../../lang/compile/compile.c
compile.o: ../../lang/compile/driver.h
compile.o: /usr/include/stdio.h
compile.o: /usr/include/sys/types.h
compile.o: /usr/include/sys/stdtypes.h
compile.o: /usr/include/sys/sysmacros.h
compile.o: /usr/include/sys/time.h
compile.o: /usr/include/time.h
compile.o: /usr/include/sys/stdtypes.h
compile.o: /usr/include/sys/resource.h
compile.o: /usr/include/sys/param.h
compile.o: /usr/include/machine/param.h
compile.o: /usr/include/sys/signal.h
compile.o: /usr/include/vm/faultcode.h
compile.o: /usr/include/sys/stdtypes.h
compile.o: /usr/include/sys/types.h
compile.o: ../../lang/vroot/report.h
compile.o: /usr/include/stdio.h
compile.o: ../../lang/vroot/vroot.h
compile.o: /usr/include/stdio.h
compile.o: /usr/include/alloca.h
compile.o: /usr/include/signal.h
compile.o: /usr/include/sys/signal.h
compile.o: /usr/include/sys/types.h
compile.o: /usr/include/sys/stat.h
compile.o: /usr/include/sys/types.h
compile.o: ../../lang/compile/../browser/cblib/src/cb_init.h
compile.o: ../../lang/compile/../browser/cblib/src/array.h
compile.o: ../../lang/compile/../browser/cblib/src/cb_heap.h
compile.o: ../../lang/compile/../browser/cblib/src/cb_portability.h
compile.o: ../../lang/compile/../browser/cblib/src/cb_directory.h
compile.o: ../../lang/compile/../browser/cblib/src/hash.h
compile.o: ../../lang/compile/../browser/cblib/src/cb_literals.h
compile.o: /usr/include/alloca.h
driver_version.o: ../../lang/compile/driver_version.c
driver_version.o: /usr/include/stdio.h
driver_version.o: /usr/include/sys/file.h
driver_version.o: /usr/include/sys/types.h
driver_version.o: /usr/include/sys/stdtypes.h
driver_version.o: /usr/include/sys/sysmacros.h
driver_version.o: /usr/include/sys/fcntlcom.h
driver_version.o: /usr/include/sys/stdtypes.h
driver_version.o: /usr/include/sys/stat.h
driver_version.o: /usr/include/sys/types.h
driver_version.o: ../../lang/compile/driver.h
driver_version.o: /usr/include/stdio.h
driver_version.o: /usr/include/sys/types.h
driver_version.o: /usr/include/sys/time.h
driver_version.o: /usr/include/time.h
driver_version.o: /usr/include/sys/stdtypes.h
driver_version.o: /usr/include/sys/resource.h
driver_version.o: /usr/include/sys/param.h
driver_version.o: /usr/include/machine/param.h
driver_version.o: /usr/include/sys/signal.h
driver_version.o: /usr/include/vm/faultcode.h
driver_version.o: /usr/include/sys/stdtypes.h
driver_version.o: /usr/include/sys/types.h
driver_version.o: ../../lang/vroot/report.h
driver_version.o: /usr/include/stdio.h
driver_version.o: ../../lang/vroot/vroot.h
driver_version.o: /usr/include/stdio.h
driver_version.o: /usr/include/alloca.h
rw_data.o: ../../lang/compile/rw_data.c
rw_data.o: ../../lang/compile/driver.h
rw_data.o: /usr/include/stdio.h
rw_data.o: /usr/include/sys/types.h
rw_data.o: /usr/include/sys/stdtypes.h
rw_data.o: /usr/include/sys/sysmacros.h
rw_data.o: /usr/include/sys/time.h
rw_data.o: /usr/include/time.h
rw_data.o: /usr/include/sys/stdtypes.h
rw_data.o: /usr/include/sys/resource.h
rw_data.o: /usr/include/sys/param.h
rw_data.o: /usr/include/machine/param.h
rw_data.o: /usr/include/sys/signal.h
rw_data.o: /usr/include/vm/faultcode.h
rw_data.o: /usr/include/sys/stdtypes.h
rw_data.o: /usr/include/sys/types.h
rw_data.o: ../../lang/vroot/report.h
rw_data.o: /usr/include/stdio.h
rw_data.o: ../../lang/vroot/vroot.h
rw_data.o: /usr/include/stdio.h
rw_data.o: /usr/include/alloca.h
ro_data.o: ../../lang/compile/ro_data.c
ro_data.o: ../../lang/compile/driver.h
ro_data.o: /usr/include/stdio.h
ro_data.o: /usr/include/sys/types.h
ro_data.o: /usr/include/sys/stdtypes.h
ro_data.o: /usr/include/sys/sysmacros.h
ro_data.o: /usr/include/sys/time.h
ro_data.o: /usr/include/time.h
ro_data.o: /usr/include/sys/stdtypes.h
ro_data.o: /usr/include/sys/resource.h
ro_data.o: /usr/include/sys/param.h
ro_data.o: /usr/include/machine/param.h
ro_data.o: /usr/include/sys/signal.h
ro_data.o: /usr/include/vm/faultcode.h
ro_data.o: /usr/include/sys/stdtypes.h
ro_data.o: /usr/include/sys/types.h
ro_data.o: ../../lang/vroot/report.h
ro_data.o: /usr/include/stdio.h
ro_data.o: ../../lang/vroot/vroot.h
ro_data.o: /usr/include/stdio.h
ro_data.o: /usr/include/alloca.h
run_pass.o: ../../lang/compile/run_pass.c
run_pass.o: ../../lang/compile/driver.h
run_pass.o: /usr/include/stdio.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: /usr/include/sys/stdtypes.h
run_pass.o: /usr/include/sys/sysmacros.h
run_pass.o: /usr/include/sys/time.h
run_pass.o: /usr/include/time.h
run_pass.o: /usr/include/sys/stdtypes.h
run_pass.o: /usr/include/sys/resource.h
run_pass.o: /usr/include/sys/param.h
run_pass.o: /usr/include/machine/param.h
run_pass.o: /usr/include/sys/signal.h
run_pass.o: /usr/include/vm/faultcode.h
run_pass.o: /usr/include/sys/stdtypes.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: ../../lang/vroot/report.h
run_pass.o: /usr/include/stdio.h
run_pass.o: ../../lang/vroot/vroot.h
run_pass.o: /usr/include/stdio.h
run_pass.o: /usr/include/alloca.h
run_pass.o: /usr/include/sys/wait.h
run_pass.o: /usr/include/sys/stdtypes.h
run_pass.o: /usr/include/sys/file.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: /usr/include/sys/fcntlcom.h
run_pass.o: /usr/include/sys/stdtypes.h
run_pass.o: /usr/include/sys/stat.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: /usr/include/ctype.h
setup.o: ../../lang/compile/setup.c
setup.o: ../../lang/compile/driver.h
setup.o: /usr/include/stdio.h
setup.o: /usr/include/sys/types.h
setup.o: /usr/include/sys/stdtypes.h
setup.o: /usr/include/sys/sysmacros.h
setup.o: /usr/include/sys/time.h
setup.o: /usr/include/time.h
setup.o: /usr/include/sys/stdtypes.h
setup.o: /usr/include/sys/resource.h
setup.o: /usr/include/sys/param.h
setup.o: /usr/include/machine/param.h
setup.o: /usr/include/sys/signal.h
setup.o: /usr/include/vm/faultcode.h
setup.o: /usr/include/sys/stdtypes.h
setup.o: /usr/include/sys/types.h
setup.o: ../../lang/vroot/report.h
setup.o: /usr/include/stdio.h
setup.o: ../../lang/vroot/vroot.h
setup.o: /usr/include/stdio.h
setup.o: /usr/include/alloca.h
setup_cc.o: ../../lang/compile/setup_cc.c
setup_cc.o: ../../lang/compile/driver.h
setup_cc.o: /usr/include/stdio.h
setup_cc.o: /usr/include/sys/types.h
setup_cc.o: /usr/include/sys/stdtypes.h
setup_cc.o: /usr/include/sys/sysmacros.h
setup_cc.o: /usr/include/sys/time.h
setup_cc.o: /usr/include/time.h
setup_cc.o: /usr/include/sys/stdtypes.h
setup_cc.o: /usr/include/sys/resource.h
setup_cc.o: /usr/include/sys/param.h
setup_cc.o: /usr/include/machine/param.h
setup_cc.o: /usr/include/sys/signal.h
setup_cc.o: /usr/include/vm/faultcode.h
setup_cc.o: /usr/include/sys/stdtypes.h
setup_cc.o: /usr/include/sys/types.h
setup_cc.o: ../../lang/vroot/report.h
setup_cc.o: /usr/include/stdio.h
setup_cc.o: ../../lang/vroot/vroot.h
setup_cc.o: /usr/include/stdio.h
setup_cc.o: /usr/include/alloca.h
setup_f77.o: ../../lang/compile/setup_f77.c
setup_f77.o: ../../lang/compile/driver.h
setup_f77.o: /usr/include/stdio.h
setup_f77.o: /usr/include/sys/types.h
setup_f77.o: /usr/include/sys/stdtypes.h
setup_f77.o: /usr/include/sys/sysmacros.h
setup_f77.o: /usr/include/sys/time.h
setup_f77.o: /usr/include/time.h
setup_f77.o: /usr/include/sys/stdtypes.h
setup_f77.o: /usr/include/sys/resource.h
setup_f77.o: /usr/include/sys/param.h
setup_f77.o: /usr/include/machine/param.h
setup_f77.o: /usr/include/sys/signal.h
setup_f77.o: /usr/include/vm/faultcode.h
setup_f77.o: /usr/include/sys/stdtypes.h
setup_f77.o: /usr/include/sys/types.h
setup_f77.o: ../../lang/vroot/report.h
setup_f77.o: /usr/include/stdio.h
setup_f77.o: ../../lang/vroot/vroot.h
setup_f77.o: /usr/include/stdio.h
setup_f77.o: /usr/include/alloca.h
setup_lint.o: ../../lang/compile/setup_lint.c
setup_lint.o: ../../lang/compile/driver.h
setup_lint.o: /usr/include/stdio.h
setup_lint.o: /usr/include/sys/types.h
setup_lint.o: /usr/include/sys/stdtypes.h
setup_lint.o: /usr/include/sys/sysmacros.h
setup_lint.o: /usr/include/sys/time.h
setup_lint.o: /usr/include/time.h
setup_lint.o: /usr/include/sys/stdtypes.h
setup_lint.o: /usr/include/sys/resource.h
setup_lint.o: /usr/include/sys/param.h
setup_lint.o: /usr/include/machine/param.h
setup_lint.o: /usr/include/sys/signal.h
setup_lint.o: /usr/include/vm/faultcode.h
setup_lint.o: /usr/include/sys/stdtypes.h
setup_lint.o: /usr/include/sys/types.h
setup_lint.o: ../../lang/vroot/report.h
setup_lint.o: /usr/include/stdio.h
setup_lint.o: ../../lang/vroot/vroot.h
setup_lint.o: /usr/include/stdio.h
setup_lint.o: /usr/include/alloca.h
setup_lint.o: /usr/include/sys/file.h
setup_lint.o: /usr/include/sys/types.h
setup_lint.o: /usr/include/sys/fcntlcom.h
setup_lint.o: /usr/include/sys/stdtypes.h
setup_lint.o: /usr/include/sys/stat.h
setup_lint.o: /usr/include/sys/types.h
setup_m2c.o: ../../lang/compile/setup_m2c.c
setup_m2c.o: ../../lang/compile/driver.h
setup_m2c.o: /usr/include/stdio.h
setup_m2c.o: /usr/include/sys/types.h
setup_m2c.o: /usr/include/sys/stdtypes.h
setup_m2c.o: /usr/include/sys/sysmacros.h
setup_m2c.o: /usr/include/sys/time.h
setup_m2c.o: /usr/include/time.h
setup_m2c.o: /usr/include/sys/stdtypes.h
setup_m2c.o: /usr/include/sys/resource.h
setup_m2c.o: /usr/include/sys/param.h
setup_m2c.o: /usr/include/machine/param.h
setup_m2c.o: /usr/include/sys/signal.h
setup_m2c.o: /usr/include/vm/faultcode.h
setup_m2c.o: /usr/include/sys/stdtypes.h
setup_m2c.o: /usr/include/sys/types.h
setup_m2c.o: ../../lang/vroot/report.h
setup_m2c.o: /usr/include/stdio.h
setup_m2c.o: ../../lang/vroot/vroot.h
setup_m2c.o: /usr/include/stdio.h
setup_m2c.o: /usr/include/alloca.h
setup_pc.o: ../../lang/compile/setup_pc.c
setup_pc.o: ../../lang/compile/driver.h
setup_pc.o: /usr/include/stdio.h
setup_pc.o: /usr/include/sys/types.h
setup_pc.o: /usr/include/sys/stdtypes.h
setup_pc.o: /usr/include/sys/sysmacros.h
setup_pc.o: /usr/include/sys/time.h
setup_pc.o: /usr/include/time.h
setup_pc.o: /usr/include/sys/stdtypes.h
setup_pc.o: /usr/include/sys/resource.h
setup_pc.o: /usr/include/sys/param.h
setup_pc.o: /usr/include/machine/param.h
setup_pc.o: /usr/include/sys/signal.h
setup_pc.o: /usr/include/vm/faultcode.h
setup_pc.o: /usr/include/sys/stdtypes.h
setup_pc.o: /usr/include/sys/types.h
setup_pc.o: ../../lang/vroot/report.h
setup_pc.o: /usr/include/stdio.h
setup_pc.o: ../../lang/vroot/vroot.h
setup_pc.o: /usr/include/stdio.h
setup_pc.o: /usr/include/alloca.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
