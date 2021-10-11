#
# @(#) tgt.mk 1.1 92/07/30 SMI; from UCB 4.3 6/11/83
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
#	TGT, FLG, LIB, and DST are filled in by the invoker
TGT=
FLG=
LIB=
DST=
CFLAGS=	-O -DTELL -DVMUNIX -Ddebug -DVFORK ${FLG}
XSTR=	/usr/ucb/xstr
ED=	-ed
AS=	-as
RM=	-rm
LIBES=	${LIB}
SCCS=	/usr/ucb/sccs
CTAGS=	/usr/ucb/ctags

SRC=	alloc.c printf.c tenex.c sh.c sh.dir.c sh.dol.c sh.err.c sh.exec.c \
	sh.exp.c sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c sh.misc.c \
	sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c sh.sig.c sh.time.c
# strings.o must be last in OBJS since it can change when previous files compile
OBJS=	alloc.o printf.o tenex.o sh.o sh.dir.o sh.dol.o sh.err.o sh.exec.o \
	sh.exp.o sh.func.o sh.glob.o sh.hist.o sh.init.o sh.lex.o sh.misc.o \
	sh.parse.o sh.print.o sh.proc.o sh.sem.o sh.set.o sh.sig.o sh.time.o \
	strings.o

# Special massaging of C files for sharing of strings
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c 
	mv x.o $*.o

all:	${TGT}

${TGT}: ${OBJS} sh.local.h
	rm -f $@
	cc ${OBJS} -o $@ ${LIBES}

${TGT}.prof: ${OBJS} sh.prof.o sh.local.h mcrt0.o
	rm -f $@.prof
	ld -X mcrt0.o ${OBJS} -o $@.prof ${LIBES} -lc

sh.o.prof:
	cp sh.c sh.prof.c
	cc -c ${CFLAGS} -DPROF sh.prof.c

# need an old doprnt, whose output we can trap
# doprnt.o: doprnt.c
# 	cc -E doprnt.c > doprnt.s
# 	as -o doprnt.o doprnt.s
# 	rm -f doprnt.s

# strings.o and sh.init.o are specially processed to be shared
strings.o: strings
	${XSTR}
	${CC} -c -R xs.c
	mv xs.o strings.o

sh.init.o: sh.init.c
	${CC} -E ${CFLAGS} sh.init.c | ${XSTR} -c -
	${CC} ${CFLAGS} -c -R x.c
	mv x.o sh.init.o
	
install: ${TGT} sh.local.h
	install -s ${TGT} ${DESTDIR}/${DST}/${TGT}

clean:
	${RM} -f a.out strings x.c xs.c ${TGT} errs
	${RM} -f *.o sh.prof.c
	${SCCS} clean
	${SCCS} get Makefile tgt.mk

lint:
	lint ${CFLAGS} ${SRC}

tags:	/tmp
	${CTAGS} ${SRC}

alloc.c:	SCCS/s.alloc.c
	${SCCS} get $@
printf.c:	SCCS/s.printf.c
	${SCCS} get $@
tenex.c:	SCCS/s.tenex.c
	${SCCS} get $@
sh.c:		SCCS/s.sh.c
	${SCCS} get $@
sh.dir.c:	SCCS/s.sh.dir.c
	${SCCS} get $@
sh.dol.c:	SCCS/s.sh.dol.c
	${SCCS} get $@
sh.err.c:	SCCS/s.sh.err.c
	${SCCS} get $@
sh.exec.c:	SCCS/s.sh.exec.c
	${SCCS} get $@
sh.exp.c:	SCCS/s.sh.exp.c
	${SCCS} get $@
sh.func.c:	SCCS/s.sh.func.c
	${SCCS} get $@
sh.glob.c:	SCCS/s.sh.glob.c
	${SCCS} get $@
sh.hist.c:	SCCS/s.sh.hist.c
	${SCCS} get $@
sh.init.c:	SCCS/s.sh.init.c
	${SCCS} get $@
sh.lex.c:	SCCS/s.sh.lex.c
	${SCCS} get $@
sh.misc.c:	SCCS/s.sh.misc.c
	${SCCS} get $@
sh.parse.c:	SCCS/s.sh.parse.c
	${SCCS} get $@
sh.print.c:	SCCS/s.sh.print.c
	${SCCS} get $@
sh.proc.c:	SCCS/s.sh.proc.c
	${SCCS} get $@
sh.sem.c:	SCCS/s.sh.sem.c
	${SCCS} get $@
sh.set.c:	SCCS/s.sh.set.c
	${SCCS} get $@
sh.sig.c:	SCCS/s.sh.sig.c
	${SCCS} get $@
sh.time.c:	SCCS/s.sh.time.c
	${SCCS} get $@
param.h:	SCCS/s.sh.dir.h
	${SCCS} get $@
sh.dir.h:	SCCS/s.sh.dir.h
	${SCCS} get $@
sh.h:		SCCS/s.sh.h
	${SCCS} get $@
sh.local.h:	SCCS/s.sh.local.h
	${SCCS} get $@
sh.proc.h:	SCCS/s.sh.proc.h
	${SCCS} get $@
#doprnt
#malloc

alloc.o: alloc.c sh.local.h 
doprnt.o: doprnt.c 
malloc.o: malloc.c 
printf.o: printf.c /usr/include/varargs.h /usr/include/ctype.h param.h 
sh.o: sh.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/sgtty.h /usr/include/pwd.h 
sh.dir.o: sh.dir.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h sh.dir.h 
sh.dol.o: sh.dol.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.err.o: sh.err.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/sgtty.h 
sh.exec.o: sh.exec.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/dir.h 
sh.exp.o: sh.exp.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.func.o: sh.func.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/sgtty.h 
sh.glob.o: sh.glob.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/dir.h 
sh.hist.o: sh.hist.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.init.o: sh.init.c sh.local.h 
sh.lex.o: sh.lex.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sgtty.h \
	/usr/include/sys/ioctl.h /usr/include/sys/ttychars.h \
	/usr/include/sys/ttydev.h 
sh.misc.o: sh.misc.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.parse.o: sh.parse.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.print.o: sh.print.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/sgtty.h 
sh.proc.o: sh.proc.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h sh.dir.h sh.proc.h \
	/usr/include/sys/wait.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/sgtty.h 
sh.sem.o: sh.sem.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h sh.proc.h \
	/usr/include/sys/ioctl.h /usr/include/sys/ttychars.h \
	/usr/include/sys/ttydev.h /usr/include/sgtty.h 
sh.set.o: sh.set.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
sh.sig.o: sh.sig.c /usr/include/signal.h 
sh.time.o: sh.time.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h 
tenex.o: tenex.c sh.h /usr/include/errno.h /usr/include/setjmp.h \
	/usr/include/signal.h /usr/include/sys/param.h \
	/usr/include/machine/param.h /usr/include/sys/types.h \
	/usr/include/sys/stat.h /usr/include/sys/time.h \
	/usr/include/time.h /usr/include/sys/resource.h \
	/usr/include/sys/times.h sh.local.h /usr/include/sys/dir.h \
	/usr/include/sgtty.h /usr/include/sys/ioctl.h \
	/usr/include/sys/ttychars.h /usr/include/sys/ttydev.h \
	/usr/include/pwd.h 
