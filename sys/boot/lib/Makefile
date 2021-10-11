#
# @(#)Makefile 1.1 92/07/30
#

ARCH:sh = arch -k

m-mc68010 = m68k
m-mc68020 = m68k
m-sparc = sparc
m-sunrise = sparc

MACH = $(m$(TARGET_MACH))

LIBSA=	libsa.a
LIBXX=	libxx.a

# All the real work gets done in ./$(ARCH)
# Also, that's where the libraries wind up
all $(LIBSA) $(LIBXX) clean lint install install_h:
	cd $(ARCH); $(MAKE) $(MFLAGS) $@
