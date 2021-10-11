#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R2 1.3
#
# mailx -- a modified version of a University of California at Berkeley
#	mail program
#
# for 4.2 Unix
#

.DEFAULT:
	sccs get -G$@ $@

VERSION=	SMI 4.0
HDR=	hdr
DESTDIR=

DEFINES= -DV7 -DVMUNIX -DOPTIM
CFLAGS=	-O -I${HDR} ${DEFINES}
LINTFLAGS= -I${HDR} ${DEFINES} -hb
LDFLAGS=

XSTR=	/usr/ucb/xstr
CC=	cc
RM=-/bin/rm
SIZE=/bin/size
MV=/bin/mv
ECHO=/bin/echo
LINT=/usr/bin/lint
CTAGS=/usr/ucb/ctags -w

OBJS=	version.o aux.o cmd1.o cmd2.o cmd3.o cmd4.o cmdtab.o collect.o \
	config.o edit.o fio.o getname.o hostname.o head.o v7.local.o lock.o \
	lex.o list.o main.o names.o optim.o popen.o quit.o send.o stralloc.o \
	temp.o tty.o vars.o lpaths.o extern.o str.o		# must be last

SRCS=	aux.c cmd1.c cmd2.c cmd3.c cmd4.c cmdtab.c collect.c config.c edit.c \
	fio.c getname.c head.c hostname.c v7.local.c lock.c lex.c list.c \
	main.c names.c optim.c popen.c quit.c send.c stralloc.c temp.c tty.c \
	vars.c lpaths.c extern.c

HDRS=	${HDR}/rcv.h ${HDR}/configdefs.h ${HDR}/def.h ${HDR}/glob.h \
	${HDR}/v7.local.h ${HDR}/local.h ${HDR}/uparm.h ${HDR}/sigretro.h

S=	${SRCS} version.c ${HDRS}

MISCFILES= misc/Mail.help misc/Mail.help.~ misc/Mail.rc

#
# Special massaging of C files for sharing of strings
#
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c 
	${MV} x.o $*.o
	@${RM} -f x.c

all:	Mail fmt

Mail:	${XSTR} $S ${OBJS}
	${RM} -f Mail
	${CC} ${LDFLAGS} -o Mail ${OBJS}
	@${SIZE} Mail

install:	all $(MISCFILES)
	install -s Mail ${DESTDIR}/usr/ucb/Mail
	cp misc/Mail.* ${DESTDIR}/usr/lib
	chmod 644 ${DESTDIR}/usr/lib/Mail.*
	install -s fmt ${DESTDIR}/usr/ucb/fmt
	cd ${DESTDIR}/usr/ucb; rm -f mail; ln Mail mail

version.o:	FRC version.c
	${CC} -c -R version.c
	${RM} -f version.c

version.c:
	${ECHO} \
	"char *version=\"Mail version ${VERSION} `date`\";"\
		> version.c

fmt:	fmt.c head.c
	${RM} -f fhead.c
	cp head.c fhead.c
	${CC} ${CFLAGS} fmt.c fhead.c -o fmt
	${RM} -f fmt.o fhead.o fhead.c
	size fmt

clean:
	${RM} -f *.o
	${RM} -f version.c a.out x.c xs.c core strings errs
	${RM} -f Mail fmt

lint:	version.c
	${LINT} ${LINTFLAGS} ${SRCS} version.c 2>&1 | \
	    grep -v 'possible pointer alignment problem' | \
	    grep -v 'arg.*used inconsistently	llib-lc' | \
	    grep -v 'returns value which is always ignored'
	${RM} -f version.c

str.o:	strings
	${XSTR}
	${CC} -R -c xs.c
	${MV} xs.o str.o
	${RM} xs.c

cmdtab.o:	cmdtab.c
	${CC} -c -R ${CFLAGS} cmdtab.c

tags:	$(SRCS)
	${CTAGS} $(SRCS);

FRC:
