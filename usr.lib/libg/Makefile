#
# @(#)Makefile 1.1 92/07/30 SMI
#

LIBS= libg.a

LIBDIR= $(DESTDIR)/usr/lib

ARCH-mc68010= m68k
ARCH-mc68020= m68k
ARCH-sparc= sparc
ARCH= $(ARCH$(TARGET_MACH))

.KEEP_STATE:

all: $(LIBS)

$(LIBS): $(ARCH)/libg.s
	$(AS) -o $@ $(ARCH)/libg.s

install: $(LIBS)
	install -d $(LIBDIR)
	install -m 644 $(LIBS) $(LIBDIR)

clean:
	$(RM) $(LIBS)
