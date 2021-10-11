#
# @(#)Makefile 1.1 92/07/30 SMI; from System III
#
.KEEP_STATE:
COPTS = -O
CFLAGS = $(COPTS) -DVMUNIX

OBJS =	clean.o curdir.o fdfopen.o  \
	imatch.o sindex.o lockit.o move.o patoi.o \
	repeat.o repl.o satoi.o trnslat.o \
	setsig.o sname.o strend.o substr.o \
	any.o xalloc.o xcreat.o xmsg.o \
	xopen.o xpipe.o xunlink.o move.o \
	cat.o dname.o fatal.o zero.o zeropad.o userexit.o


libPW.a:	$(OBJS)
	-rm -f libPW.a
	ar r libPW.a $(OBJS)
	ranlib libPW.a

install:	libPW.a

clean:
	-rm -f $(OBJS) errs core libPW.a
