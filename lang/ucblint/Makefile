#
# @(#)Makefile 1.1 92/07/30 SMI
#

DESTDIR=

all: FRC
	@set -x;case `mach` in \
	mc68010|mc68020)\
		cd m68k; $(MAKE) $(MFLAGS) CC="${CC}" AS="${AS}"\
		;;\
	sparc)\
		cd sparc; $(MAKE) $(MFLAGS) CC="${CC}" AS="${AS}"\
		;;\
	esac

xm68k: FRC
	cd m68k; make ${MFLAGS} CC="${CC}" AS="${AS}"

xsparc: FRC
	cd sparc; make ${MFLAGS} CC="${CC}" AS="${AS}"

xall: FRC
	cd m68k; make ${MFLAGS} CC="${CC}" AS="${AS}"
	cd sparc; make ${MFLAGS} CC="${CC}" AS="${AS}"

install: FRC
	@set -x;case `mach` in \
	mc68010|mc68020)\
		cd m68k; $(MAKE) $(MFLAGS) DESTDIR=${DESTDIR} install\
		;;\
	sparc)\
		cd sparc; $(MAKE) $(MFLAGS) DESTDIR=${DESTDIR} install\
		;;\
	esac

clean:
	@set -x;case `mach` in \
	mc68010|mc68020)\
		cd m68k; $(MAKE) $(MFLAGS) clean\
		;;\
	sparc)\
		cd sparc; $(MAKE) $(MFLAGS) clean\
		;;\
	esac

FRC:

