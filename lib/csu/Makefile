#
# @(#)Makefile 1.1 92/07/30 SMI
#
all:
	@set -x;\
	case `mach` in \
	sparc)	 cd sparc; $(MAKE) $(MFLAGS);;\
	vax)		 cd vax; $(MAKE) $(MFLAGS);;\
	mc68010|mc68020) cd m68k; $(MAKE) $(MFLAGS);;\
	esac;

clean:
	@set -x;\
	case `mach` in \
	sparc)	 cd sparc; $(MAKE) $(MFLAGS) clean;;\
	vax)		 cd vax; $(MAKE) $(MFLAGS) clean;;\
	mc68010|mc68020) cd m68k; $(MAKE) $(MFLAGS) clean;;\
	esac;
install:
	@set -x;\
	case `mach` in \
	sparc)	 cd sparc; $(MAKE) $(MFLAGS) install;;\
	vax)		 cd vax; $(MAKE) $(MFLAGS) install;;\
	mc68010|mc68020) cd m68k; $(MAKE) $(MFLAGS) install;;\
	esac;
