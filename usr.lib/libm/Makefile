#
# @(#)Makefile 1.1 92/07/30 SMI;
#

#	libm outer Makefile

MACH=	`mach`
INCLUDE=
CFLAGS=	-O4 $(INCLUDE)
ASFLAGS = $(INCLUDE)

SUN2OR3LIBS=	m68k/fsoft/libm.a m68k/fswitch/libm.a
SUN2LIBS=	m68k/fsky/libm.a
SUN3LIBS=	m68k/ffpa/libm.a m68k/f68881/libm.a
SUN4LIBS=	sparc/libm.a
RRLIBS=		i386/f80387/libm.a

.PRECIOUS:	$(SUN2OR3LIBS) $(SUN2LIBS) $(SUN3LIBS) $(SUN4LIBS) $(RRLIBS)

# 68881 needs to be installed on sun-2 in order to define f68881/libm.il which
# in turn is needed to make crt.

all install clean: FORCEMAKE
	if test $@ = clean ; then rm -f a.out core errs *.o; fi
	if test $(MACH) = mc68020 ; then \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/ffpa FP=ffpa INSTALLDIR=$(DESTDIR)/usr/lib/ffpa $@; \
	fi
	if test $(MACH) = mc68010 ; then \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/fsky FP=fsky INSTALLDIR=$(DESTDIR)/usr/lib/fsky $@; \
	fi
	if test $(MACH) = mc68010 -o $(MACH) = mc68020 ; then \
		(if test $@ = install ; then \
			(cd $(DESTDIR)/usr/lib ; if test ! -h libm.a ; then rm -f libm.a ; ln -s fsoft/libm.a ; fi) ; \
			(cd $(DESTDIR)/usr/lib ; if test ! -h libm_p.a ; then rm -f libm_p.a ; ln -s fsoft/libm_p.a ; fi) ; \
			(cd $(DESTDIR)/usr/lib ; if test ! -h libm.il ; then rm -f libm.il ; ln -s fsoft/libm.il ; fi) ; \
		fi) ; \
		(cd m68k; $(MAKE) -$(MAKEFLAGS) ) ; \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/f68881 FP=f68881 INSTALLDIR=$(DESTDIR)/usr/lib/f68881 $@; \
	$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/fsoft FP=fsoft INSTALLDIR=$(DESTDIR)/usr/lib/fsoft $@; \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/fswitch FP=fswitch INSTALLDIR=$(DESTDIR)/usr/lib/fswitch $@; \
	fi
	if test $(MACH) = sparc ; then \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=sparc FP=Dsparc INSTALLDIR='$(DESTDIR)/usr/lib' $@; \
	fi
	if test $(MACH) = i386 ; then \
		$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG= OBJDIR=i386/f80387 FP=Df80387 INSTALLDIR='$(DESTDIR)/usr/lib' $@; \
	fi

#
#  The following targets are invoked at the end of the lang/boot build
#
fsoft installfsoft:
	if test $(MACH) = mc68010 -o $(MACH) = mc68020 ; then \
		(cd m68k; $(MAKE) -$(MAKEFLAGS) ) ; \
	$(MAKE) -$(MAKEFLAGS) -f makelibm MACH='$(MACH)' CC='$(CC)' CFLAGS='$(CFLAGS)' ASFLAGS='$(ASFLAGS)' PLDFLAG=-X OBJDIR=m68k/fsoft FP=fsoft INSTALLDIR=$(DESTDIR)/usr/lib/fsoft $(@:fsoft=); \
	else \
		$(MAKE) $(@:fsoft=); \
	fi

FORCEMAKE:
