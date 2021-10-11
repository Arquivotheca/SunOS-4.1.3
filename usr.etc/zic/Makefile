#
# @(#)Makefile 1.1 92/07/30 SMI; from Arthur Olson's 6.1
#

# The setting for LOCALTIME is the time zone that is "installed by
# default" - that is, the time zone that'll be the standard one if
# SunOS is read in from the tape and started up.  "suninstall"
# requires you to specify the appropriate time zone when you install
# SunOS, and since any particular choice will be incorrect for most
# sites, "factory" is probably the best choice, as it will cause the
# time zone name to be an indication that no time zone was specified.

LOCALTIME= 	Factory

# The setting for POSIXRULES is the time zone that is, by default, the
# one that's used as a "template" set of rules for POSIX-style time zone
# settings that don't specify the rules in TZ (for example, PST8PDT).
# (When a POSIX-style environment variable is handled, the rules in the template
# file are used to determine "spring forward" and "fall back" days and
# times; the environment variable itself specifies GMT offsets of standard and
# summer time.)
# Since the US Federal rules are what are used by most systems if you
# specify a POSIX-style TZ environment variable, US/Eastern is probably
# the best choice.

POSIXRULES=	US/Eastern

CFLAGS=	-O

LINTFLAGS= -hbaxc

################################################################################

BINS=		zic zdump tzsetup
TZCOBJS=	zic.o scheck.o ialloc.o
TZCSRCS=	$(TZCOBJS:.o=.c)
TZDOBJS=	zdump.o ialloc.o
TZDSRCS=	$(TZDOBJS:.o=.c)
YDATA=		africa antarctica asia australasia \
		europe northamerica southamerica pacificnew etcetera factory
SDATA=		solar87 solar88 solar89
TDATA=		$(YDATA) $(SDATA)
USNO=		usno1988 usno1989
FILES=		$(TDATA) $(USNO)
CFLAGS=		-I/usr/src/include
.KEEP_STATE:

all:		$(BINS) $(FILES)

zdump:		$(TZDOBJS)
		$(LINK.c) -o $@ $(TZDOBJS) $(LDLIBS)

zic:		$(TZCOBJS)
		$(LINK.c) -o $@ $(TZCOBJS) $(LDLIBS)

install:	$(BINS) $(FILES)
		install -d -o bin -m 755 $(DESTDIR)/usr/share/lib/zoneinfo
		install -m 644 $(FILES) $(DESTDIR)/usr/share/lib/zoneinfo
		-./zic -s -d $(DESTDIR)/usr/share/lib/zoneinfo $(YDATA)
		-./zic -s -d $(DESTDIR)/usr/share/lib/zoneinfo $(SDATA)
		-./zic -d $(DESTDIR)/usr/share/lib/zoneinfo -l $(LOCALTIME) -p $(POSIXRULES)
		install -d $(DESTDIR)/usr/lib
		$(RM) $(DESTDIR)/usr/lib/zoneinfo
		ln -s ../share/lib/zoneinfo $(DESTDIR)/usr/lib/zoneinfo
		install -d $(DESTDIR)/usr/etc
		install -s $(BINS) $(DESTDIR)/usr/etc

clean:
		$(RM) $(BINS) $(TZCOBJS) $(TZDOBJS)

lint:		$(TZCSRCS) $(TZDSRCS)
		$(LINT.c) $(TZCSRCS)
		$(LINT.c) $(TZDSRCS)
