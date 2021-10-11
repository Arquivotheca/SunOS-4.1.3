#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#) Makefile 1.1 92/07/30 SMI; from UCB 5.3 3/29/86
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
# To profile, put -DPROF in DEFS and -pg in CFLAGS, and recompile.

.KEEP_STATE:
.FRC:

BINS = csh
BINDIR = $(DESTDIR)/usr/bin
LOCAL_HDRS = sh.h sh.local.h sh.dir.h param.h sh.char.h sh.proc.h sh.tconst.h
#DEFS=	-DTELL -DVFORK -DFILEC -DDBG -DTRACE
DEFS=	-DTELL -DVFORK -DFILEC 
#MBCHAR= -DMBCHAR	# Define this to make multibyte char version.
#CFLAGS=	$(DEFS) $(MBCHAR) -O
CFLAGS=	$(DEFS) $(MBCHAR)  -I/usr/src/include -O
LINK_LIBS=	-ltermlib

# doprnt.* removed from OBJS for Sun.
SRCS=	printf.c sh.c sh.char.c sh.dir.c sh.dol.c sh.err.c \
	sh.exec.c sh.exp.c sh.file.c sh.func.c sh.glob.c sh.hist.c sh.init.c \
	sh.lex.c sh.misc.c sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c \
	sh.time.c sh.tchar.c sh.tconst.c sh.debug.c

OBJS=	printf.o sh.o sh.char.o sh.dir.o sh.dol.o sh.err.o \
	sh.exec.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o sh.init.o \
	sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.proc.o sh.sem.o sh.set.o \
	sh.time.o sh.tchar.o sh.tconst.o sh.debug.o

.INIT:  $(HDRS) $(LOCAL_HDRS)

bins: $(BINS)

# Special massaging of C files for sharing of strings
.c.o:
	$(CC) -E $(CFLAGS) $*.c | xstr -c -
	$(CC) -c $(CFLAGS) x.c 
	mv -f x.o $*.o
	rm -f x.c


# strings.o must be last since it can change when previous files compile
csh: $(OBJS) strings.o
	rm -f csh
	$(CC) $(OBJS) strings.o -o csh $(LINK_LIBS)


# strings.o, sh.init.o, and sh.char.o are specially processed to be shared
strings.o: strings
	xstr
	$(CC) -c -R xs.c
	mv -f xs.o strings.o
	rm -f xs.c

strings: $(OBJS)

sh.char.o sh.init.o sh.tconst.o:
	$(CC) -E $(CFLAGS) $*.c | xstr -c -
	$(CC) $(CFLAGS) -c -R x.c
	mv -f x.o $*.o
	rm -f x.c

sh.tconst.h: sh.tconst.c make.sh.tconst.h.ed 	#sh.tconst.h is made 
	rm -f sh.tconst.h
	ed sh.tconst.c < make.sh.tconst.h.ed 	#from sh.tconst.c
	chmod 777 sh.tconst.h
	
install: $(BINS)
	install -m 775 -d $(BINDIR)
	install -s $(BINS) $(BINDIR)

install_h:

clean:
	-rm -rf $(LOCAL_BINS) $(BINS) $(LOCAL_LIBS) $(LIBS) $(OBJS) \
		core a.out install_bins sh.tconst.h strings x.c xs.c strings.o
