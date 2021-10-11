#
# @(#)Makefile 1.1 92/07/30 SMI
#

#
# Makefile for /lib/compile (driver for cc, f77, pc, m2c, lint, ...)
#

# -----------------------------------------------------------------------------

#		RELEASE=32 		   for  Release 3.x version.
#		RELEASE=40		   for  Release 4.x version.
RELEASE = 40

#		SRCDIR=.                   for  regular  version.
#		SRCDIR=../../lang/compile  for   Sys-V 3.x,4.0 version.
#		SRCDIR=../../lang/compile  for    4BSD 4.1 version.
SRCDIR  = ../../lang/compile

#		S5EMUL=                    for  regular 3.x, 4.0 version.
#		S5EMUL=-DS5EMUL            for   Sys-V  3.x, 4.0 version.
#		S5EMUL=-DS5EMUL            for  regular 4.1 version.
#		S5EMUL=                    for    4BSD  4.1 version.
S5EMUL  =

#		VROOT=../vroot             for  regular  version.
#		VROOT=../../lang/vroot     for   Sys-V 3.x,4.0  version.
#		VROOT=../../lang/vroot     for    4BSD 4.1  version.
VROOT	= ../../lang/vroot

#		LIB=/lib		   for  regular 3.x  version
#		LIB=/usr/5lib		   for   Sys-V  3.x  version
#		LIB=/usr/lib		   for  regular 4.0  version
#		LIB=/usr/5lib		   for   Sys-V  4.0  version
#		LIB=/usr/lib		   for  regular 4.1  version
#		LIB=/usr/ucblib		   for    4BSD  4.1  version
LIB	= /usr/ucblib

# -----------------------------------------------------------------------------

ARCH	= /bin/arch
CPP	= /lib/cpp
LINT	= /usr/bin/lint
#		HANDLE_R32 is for making a version which will understand
#		R3.2 directory structure as well as R4.0 directory structure.
#		This is needed for cross-compilation.
xCPPFLAGS = -D`$(ARCH)` -I$(SRCDIR) -I$(VROOT) -DRELEASE=$(RELEASE) -DHANDLE_R32 $(S5EMUL)
COPTS	= -O
CFLAGS	= $(COPTS) $(xCPPFLAGS)
LDFLAGS	= 
LIBS	=
LINTFLAGS = -x $(xCPPFLAGS)
INSTALL	= install
XLINKS	= xcc xm2c xf77 xpc xlint

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
LINTS	= compile.ln driver_version.ln rw_data.ln ro_data.ln run_pass.ln \
		setup.ln setup_cc.ln \
		setup_f77.ln setup_lint.ln setup_m2c.ln setup_pc.ln
ROOBJS	= ro_data.o
COBJS	= compile.o driver_version.o rw_data.o run_pass.o setup.o setup_cc.o \
		setup_f77.o setup_lint.o setup_m2c.o setup_pc.o
OBJS	= $(ROOBJS) $(COBJS)

#
#  Production rules for driver.
#

compile: $(OBJS) $(VROOT)/vroot.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(VROOT)/vroot.a

$(XLINKS):
	rm -f $@;  ln -s compile $@ 

xlinks:	$(XLINKS)

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

install: compile
	$(INSTALL) -s compile $(DESTDIR)$(LIB)/compile

tags:  $(SRCS)
	ctags $(SRCS)

testdir:
	TEST=dtest.`arch`; \
	if [ ! -d $${TEST} ] ; \
	then \
		set -x; \
		mkdir $${TEST} ; \
		ln -s  ../SCCS/../$${TEST}/SCCS  SCCS; \
		(cd $${TEST}; /usr/ucb/sccs get SCCS) ; \
	fi ; \
	for CMD in cc f77 pc m2c lint; \
	do \
		( cd $${TEST}; \
		  if [ ! -f $${CMD} ] ; then ln -s ../compile $${CMD}; fi ; \
		) ; \
	done

buildtest:	dtest compile testdir
	TEST=dtest.`arch`; \
	for CMD in cc f77 pc m2c lint; \
	do \
		( cd $${TEST}; \
		  for FILE in $${CMD}*.dtest ; \
		  do \
		  	chmod u+w   $${FILE} ; \
		  	../dtest -b $${FILE} ; \
		  	chmod a-w   $${FILE} ; \
		  done ; \
		) ; \
	done

test:	dtest compile testdir
	TEST=dtest.`arch`; \
	for CMD in cc f77 pc m2c lint; \
	do \
		( cd $${TEST}; \
		  for FILE in $${CMD}*.dtest ; \
		  do \
		  	chmod u+w   $${FILE} ; \
		  	../dtest    $${FILE} ; \
		  	chmod a-w   $${FILE} ; \
		  done ; \
		) ; \
	done

#
# Miscellaneous production rules.
#

clean:
	rm -f $(OBJS) compile ocompile $(LINTS) *.BAK tags
	rm -f gen gen.o syscall.a
	rm -f $(XLINKS)

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
compile.o: /usr/include/sys/sysmacros.h
compile.o: /usr/include/sys/time.h
compile.o: /usr/include/time.h
compile.o: /usr/include/sys/resource.h
compile.o: /usr/include/sys/param.h
compile.o: /usr/include/machine/param.h
compile.o: /usr/include/sys/signal.h
compile.o: /usr/include/vm/faultcode.h
compile.o: /usr/include/sys/types.h
compile.o: ../../lang/vroot/report.h
compile.o: /usr/include/stdio.h
compile.o: ../../lang/vroot/vroot.h
compile.o: /usr/include/stdio.h
compile.o: /usr/include/alloca.h
compile.o: /usr/include/signal.h
compile.o: /usr/include/sys/types.h
compile.o: /usr/include/sys/stat.h
compile.o: /usr/include/alloca.h
driver_version.o: ../../lang/compile/driver_version.c
driver_version.o: /usr/include/stdio.h
driver_version.o: /usr/include/sys/file.h
driver_version.o: /usr/include/sys/fcntlcom.h
driver_version.o: ../../lang/compile/driver.h
driver_version.o: /usr/include/stdio.h
driver_version.o: /usr/include/sys/types.h
driver_version.o: /usr/include/sys/sysmacros.h
driver_version.o: /usr/include/sys/time.h
driver_version.o: /usr/include/time.h
driver_version.o: /usr/include/sys/resource.h
driver_version.o: /usr/include/sys/param.h
driver_version.o: /usr/include/machine/param.h
driver_version.o: /usr/include/sys/signal.h
driver_version.o: /usr/include/vm/faultcode.h
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
rw_data.o: /usr/include/sys/sysmacros.h
rw_data.o: /usr/include/sys/time.h
rw_data.o: /usr/include/time.h
rw_data.o: /usr/include/sys/resource.h
rw_data.o: /usr/include/sys/param.h
rw_data.o: /usr/include/machine/param.h
rw_data.o: /usr/include/sys/signal.h
rw_data.o: /usr/include/vm/faultcode.h
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
ro_data.o: /usr/include/sys/sysmacros.h
ro_data.o: /usr/include/sys/time.h
ro_data.o: /usr/include/time.h
ro_data.o: /usr/include/sys/resource.h
ro_data.o: /usr/include/sys/param.h
ro_data.o: /usr/include/machine/param.h
ro_data.o: /usr/include/sys/signal.h
ro_data.o: /usr/include/vm/faultcode.h
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
run_pass.o: /usr/include/sys/sysmacros.h
run_pass.o: /usr/include/sys/time.h
run_pass.o: /usr/include/time.h
run_pass.o: /usr/include/sys/resource.h
run_pass.o: /usr/include/sys/param.h
run_pass.o: /usr/include/machine/param.h
run_pass.o: /usr/include/sys/signal.h
run_pass.o: /usr/include/vm/faultcode.h
run_pass.o: /usr/include/sys/types.h
run_pass.o: ../../lang/vroot/report.h
run_pass.o: /usr/include/stdio.h
run_pass.o: ../../lang/vroot/vroot.h
run_pass.o: /usr/include/stdio.h
run_pass.o: /usr/include/alloca.h
run_pass.o: /usr/include/sys/wait.h
run_pass.o: /usr/include/sys/file.h
run_pass.o: /usr/include/sys/fcntlcom.h
run_pass.o: /usr/include/ctype.h
setup.o: ../../lang/compile/setup.c
setup.o: ../../lang/compile/driver.h
setup.o: /usr/include/stdio.h
setup.o: /usr/include/sys/types.h
setup.o: /usr/include/sys/sysmacros.h
setup.o: /usr/include/sys/time.h
setup.o: /usr/include/time.h
setup.o: /usr/include/sys/resource.h
setup.o: /usr/include/sys/param.h
setup.o: /usr/include/machine/param.h
setup.o: /usr/include/sys/signal.h
setup.o: /usr/include/vm/faultcode.h
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
setup_cc.o: /usr/include/sys/sysmacros.h
setup_cc.o: /usr/include/sys/time.h
setup_cc.o: /usr/include/time.h
setup_cc.o: /usr/include/sys/resource.h
setup_cc.o: /usr/include/sys/param.h
setup_cc.o: /usr/include/machine/param.h
setup_cc.o: /usr/include/sys/signal.h
setup_cc.o: /usr/include/vm/faultcode.h
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
setup_f77.o: /usr/include/sys/sysmacros.h
setup_f77.o: /usr/include/sys/time.h
setup_f77.o: /usr/include/time.h
setup_f77.o: /usr/include/sys/resource.h
setup_f77.o: /usr/include/sys/param.h
setup_f77.o: /usr/include/machine/param.h
setup_f77.o: /usr/include/sys/signal.h
setup_f77.o: /usr/include/vm/faultcode.h
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
setup_lint.o: /usr/include/sys/sysmacros.h
setup_lint.o: /usr/include/sys/time.h
setup_lint.o: /usr/include/time.h
setup_lint.o: /usr/include/sys/resource.h
setup_lint.o: /usr/include/sys/param.h
setup_lint.o: /usr/include/machine/param.h
setup_lint.o: /usr/include/sys/signal.h
setup_lint.o: /usr/include/vm/faultcode.h
setup_lint.o: /usr/include/sys/types.h
setup_lint.o: ../../lang/vroot/report.h
setup_lint.o: /usr/include/stdio.h
setup_lint.o: ../../lang/vroot/vroot.h
setup_lint.o: /usr/include/stdio.h
setup_lint.o: /usr/include/alloca.h
setup_lint.o: /usr/include/sys/file.h
setup_lint.o: /usr/include/sys/fcntlcom.h
setup_m2c.o: ../../lang/compile/setup_m2c.c
setup_m2c.o: ../../lang/compile/driver.h
setup_m2c.o: /usr/include/stdio.h
setup_m2c.o: /usr/include/sys/types.h
setup_m2c.o: /usr/include/sys/sysmacros.h
setup_m2c.o: /usr/include/sys/time.h
setup_m2c.o: /usr/include/time.h
setup_m2c.o: /usr/include/sys/resource.h
setup_m2c.o: /usr/include/sys/param.h
setup_m2c.o: /usr/include/machine/param.h
setup_m2c.o: /usr/include/sys/signal.h
setup_m2c.o: /usr/include/vm/faultcode.h
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
setup_pc.o: /usr/include/sys/sysmacros.h
setup_pc.o: /usr/include/sys/time.h
setup_pc.o: /usr/include/time.h
setup_pc.o: /usr/include/sys/resource.h
setup_pc.o: /usr/include/sys/param.h
setup_pc.o: /usr/include/machine/param.h
setup_pc.o: /usr/include/sys/signal.h
setup_pc.o: /usr/include/vm/faultcode.h
setup_pc.o: /usr/include/sys/types.h
setup_pc.o: ../../lang/vroot/report.h
setup_pc.o: /usr/include/stdio.h
setup_pc.o: ../../lang/vroot/vroot.h
setup_pc.o: /usr/include/stdio.h
setup_pc.o: /usr/include/alloca.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
