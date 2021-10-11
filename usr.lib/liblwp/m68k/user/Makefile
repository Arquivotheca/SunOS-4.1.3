#
#	Copyright (C) 1987 Sun Microsystems, Inc.
#       @(#) Makefile 1.1 92/07/30 Copyr 1987 Sun Micro
#
# define LDEBUG to enable internal debugging and not hide global symbols

d-mc68020=mc68020
d-mc68010=mc68010
MACH= $(d$(TARGET_MACH))
#CPPOPTS= -Usparc -DLDEBUG -I.  -I../.. -Dmc68000 -D$(MACH)
CPPOPTS= -Usparc -I.  -I../.. -Dmc68000 -D$(MACH)
SYSDIR= ../..
LIBCDIR= $(SYSDIR)/libc
CFLAGS=	-O $(CPPOPTS)
LFLAGS=	$(CPPOPTS)
CPP= /lib/cpp -undef
SCCS= /usr/ucb/sccs
GREP= /bin/grep
CTAGS= /usr/ucb/ctags
LINT= /usr/bin/lint
AWK= awk
CC = /usr/bin/cc
RM= /bin/rm
AR= /bin/ar
RANLIB=  /usr/bin/ranlib
ECHO= /bin/echo
MV= /bin/mv
CP= /bin/cp
ED= /bin/ed
SED= /bin/sed
CAT= /bin/cat
MAKE= /bin/make
LN= /bin/ln
EX= /usr/ucb/ex
MKDIR= /bin/mkdir
INSTALL= /usr/bin/install
CHOWN= /usr/etc/chown
CHMOD= /bin/chmod
VGRIND= /usr/ucb/vgrind -Tlw
PSREV= /usr/doctools/bin/psrev
PSDIT= /usr/doctools/bin/psdit
PSLPR= /usr/ucb/lpr -Plw -m
LPR= /usr/ucb/lpr -Plw
LS= /bin/ls
LCOPTS=	-C -Dlint ${CPPOPTS}
LOPTS=	-hbxn
LINT1=	/usr/lib/lint/lint1
LINT2=	/usr/lib/lint/lint2
LTAIL=  ${GREP} -v 'struct/union .* never defined' | \
	${GREP} -v 'malloc' | \
	${GREP} -v 'on_exit' | \
	${GREP} -v 'possible pointer alignment problem' ; true
LTMP=	/usr/tmp/lint.lwp
LIBCLINT= /usr/lib/lint/llib-lc.ln
LINTDIR= ./linttmp
LIB=	./lib

# machine-independent sources
INDOFILES= schedule.o process.o asynch.o condvar.o monitor.o message.o \
	lwperror.o agent.o lwpclock.o alloc.o cntxt.o

INDCFILES=	schedule.c process.c condvar.c agent.c asynch.c	\
	message.c lwperror.c monitor.c lwpclock.c alloc.c cntxt.c

INDHFILES= agent.h asynch.h common.h condvar.h message.h \
	monitor.h schedule.h process.h queue.h clock.h \
	lwperror.h alloc.h trace.h cntxt.h 

LIBCOFILES= on_exit.o

LIBCCFILES= on_exit.c

LIBCHFILES= libc.h

# machine-dependent sources

MACHOFILES=	low.o machdep.o except.o lwputil.o \
		machsig.o stack.o malloc.o

MACHCFILES=	machdep.c except.c lwputil.c \
		machsig.c stack.c malloc.c

MACHHFILES=	except.h stackdep.h machdep.h machsig.h \
		mallint.h param.h

BUILTHFILES=	low.h lwpmachdep.h
MACHSFILES=	low.s

CFILES= $(INDCFILES:%= $(SYSDIR)/%) $(LIBCCFILES:%= $(LIBCDIR)/%) $(MACHCFILES)
SFILES= $(MACHSFILES)
NONBUILTHFILES= $(INDHFILES:%= $(SYSDIR)/%)\
	 $(LIBCHFILES:%= $(LIBCDIR)/%) $(MACHHFILES)
HFILES= $(NONBUILTHFILES) $(BUILTHFILES)


LIBOFILES= $(INDOFILES:%.o= $(LIB)/%.o) $(MACHOFILES:%.o= $(LIB)/%.o) \
	$(LIBCOFILES:%.o= $(LIB)/%.o)

SRCS= $(INDCFILES:%= $(SYSDIR)/%) $(INDHFILES:%= $(SYSDIR)/%) \
	$(LIBCCFILES:%= $(LIBCDIR)/%) $(LIBCHFILES:%= $(LIBCDIR)/%) \
	$(MACHCFILES) $(MACHSFILES) $(MACHHFILES)

LFILES= $(INDCFILES:%.c= ${LINTDIR}/%.L)   \
	 $(MACHCFILES:%.c= ${LINTDIR}/%.L) \
	 $(LIBCCFILES:%.c= ${LINTDIR}/%.L)

liblwp.a: ../../lwp.h $(LIB) $(HFILES) $(LIBOFILES) check.h Locore.c
	$(AR) rvu liblwp.a $(LIBOFILES)
	$(RANLIB) liblwp.a

$(LIB):
	test -d $(LIB) || $(MKDIR) $(LIB)

machlink:
	test -d machlwp || ln -s . machlwp

install: liblwp.a
	$(INSTALL) -c -m 644 liblwp.a $(DESTDIR)/usr/lib
	$(RANLIB) $(DESTDIR)/usr/lib/liblwp.a

install_h: machlink $(HFILES) check.h stackdep.h lwpmachdep.h
	$(INSTALL) -c -m 444 check.h $(DESTDIR)/usr/include/lwp/m68k_chk.h
	$(INSTALL) -c -m 444 lwpmachdep.h $(DESTDIR)/usr/include/lwp/m68k_machdep.h
	$(INSTALL) -c -m 444 stackdep.h $(DESTDIR)/usr/include/lwp/m68k_stackdep.h


tags:	liblwp.a FRC
	$(CTAGS) -wt ${SRCS}

print:
	$(VGRIND) -t Makefile genassym.c checkassym.c minstk.c		\
		`$(LS) $(HFILES)` check.h				\
		`$(LS) $(SFILES)`					\
		`$(LS) $(CFILES)` 					\
		| $(PSDIT) | $(PSREV) | $(PSLPR)

clean:
	$(RM) -f liblwp.a $(LIB)/*.o *errs tags linted made $(LINTDIR)/*.L \
	check.h lwpmachdep.h low.h

check.h: ../../lwp.h $(HFILES) checkassym.c check1.h
	$(RM) -f check.h
	$(CC) $(CFLAGS) checkassym.c
	$(CP) check1.h check.h
	$(CHMOD) 664 check.h
	a.out >> check.h
	$(CHMOD) 444 check.h
	$(RM) -f a.out

lwpmachdep.h:  lwpmachdep1.h $(NONBUILTHFILES) minstk.c
	$(RM) -f lwpmachdep.h
	$(CC) $(CFLAGS) minstk.c
	$(CP) lwpmachdep1.h lwpmachdep.h
	$(CHMOD) 664 lwpmachdep.h
	a.out >> lwpmachdep.h
	$(CHMOD) 444 lwpmachdep.h
	$(RM) -f a.out

low.h:  $(NONBUILTHFILES) lwpmachdep.h genassym.c low1.h machdep.h
	$(RM) -f low.h
	$(CC) $(CFLAGS) genassym.c
	$(CP) low1.h low.h
	$(CHMOD) 664 low.h
	a.out >> low.h
	$(CHMOD) 444 low.h
	$(RM) -f a.out

$(LIB)/low.o:	low.h low.s
	$(CPP) $(CPPOPTS) low.s > /tmp/lowcpp.s
	$(CC) -c /tmp/lowcpp.s -o $(LIB)/low.o
	$(RM) -f /tmp/lowcpp.s

depend: liblwp.a makedep
	$(SCCS) edit Makefile
	$(ECHO) '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	$(ECHO) '$$r makedep' >>eddep
	$(ECHO) 'w' >>eddep
	$(CP) Makefile Makefile.bak
	$(EX) - Makefile < eddep
	$(RM) eddep makedep makedeperrs
	$(SCCS) delget -y"" Makefile
 
makedep: FRC
	@cat /dev/null >makedep
	-(for i in ${CFILES} ; do						\
	  ${CPP} -M ${CPPOPTS} $$i | $(SED)					\
	  -e 's,^\(.*\)\.o:,$(LIB)/\1.o $(LINTDIR)/\1.L:,' >>makedep; done)	\
	  2>>makedeperrs
	${CPP} -M ${CPPOPTS} Locore.c | $(SED)					\
	  -e 's,^\(.*\)\.o:,$(LINTDIR)/\1.L:,' >>makedep 2>>makedeperrs
	@cat makedeperrs >&- 1>&2
	@(if [ -s makedeperrs ]; then false; fi)

FRC:

$(LINTDIR)/Locore.L: Locore.c
	@$(ECHO) Locore.c
	@-(${CPP} ${LCOPTS} Locore.c | \
	${LINT1} ${LOPTS} > $(LINTDIR)/Locore.L ) 2>&1 | ${LTAIL}


lint: liblwp.a $(LINTDIR) $(LINTDIR)/Locore.L ${LFILES}
	@$(MAKE) lint2 LTMP=${LTMP}.$$$$	#trick to define tmp file once

$(LINTDIR):
	test -d $(LINTDIR) || $(MKDIR) $(LINTDIR)

lint2:
	@$(CAT) $(LINTDIR)/Locore.L ${LFILES} > ${LTMP}
	@$(CAT) $(LIBCLINT) >> ${LTMP}
	@$(ECHO) "Global Cross-checks:"
	@$(LINT2) ${LTMP} ${LOPTS} | ${LTAIL}
	@$(RM) -f ${LTMP}

$(LIB)/%.o:	%.c
	$(CC) $(CFLAGS) -c  $< -o $@

$(LIB)/%.o:	$(LIBCDIR)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@

$(LIB)/%.o:	$(SYSDIR)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@


${LINTDIR}/%.L : %.c
	@$(ECHO) $<
	@-(${CPP} ${LCOPTS} $< | \
	${LINT1} ${LOPTS} > $@ ) 2>&1 | ${LTAIL}

${LINTDIR}/%.L : $(LIBCDIR)/%.c
	@$(ECHO) $<
	@-(${CPP} ${LCOPTS} $< | \
	${LINT1} ${LOPTS} > $@ ) 2>&1 | ${LTAIL}

${LINTDIR}/%.L : $(SYSDIR)/%.c
	@$(ECHO) $<
	@-(${CPP} ${LCOPTS} $< | \
	${LINT1} ${LOPTS} > $@ ) 2>&1 | ${LTAIL}

# DO NOT DELETE THIS LINE -- make depend uses it
./lib/schedule.o ./linttmp/schedule.L: ../../schedule.c
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/common.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/types.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/sysmacros.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/trace.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/lwp.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/time.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/time.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/varargs.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/queue.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/asynch.h
./lib/schedule.o ./linttmp/schedule.L: ./machlwp/machsig.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/wait.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/time.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/sys/resource.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/signal.h
./lib/schedule.o ./linttmp/schedule.L: /usr/include/vm/faultcode.h
./lib/schedule.o ./linttmp/schedule.L: ./machlwp/machdep.h
./lib/schedule.o ./linttmp/schedule.L: ./machlwp/lwpmachdep1.h
./lib/schedule.o ./linttmp/schedule.L: ./machlwp/stackdep.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/cntxt.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/lwperror.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/schedule.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/alloc.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/agent.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/clock.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/condvar.h
./lib/schedule.o ./linttmp/schedule.L: ./machlwp/except.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/message.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/process.h
./lib/schedule.o ./linttmp/schedule.L: ../../lwp/monitor.h
./lib/process.o ./linttmp/process.L: ../../process.c
./lib/process.o ./linttmp/process.L: ../../lwp/common.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/types.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/sysmacros.h
./lib/process.o ./linttmp/process.L: ../../lwp/trace.h
./lib/process.o ./linttmp/process.L: ../../lwp/lwp.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/time.h
./lib/process.o ./linttmp/process.L: /usr/include/time.h
./lib/process.o ./linttmp/process.L: /usr/include/varargs.h
./lib/process.o ./linttmp/process.L: ../../lwp/queue.h
./lib/process.o ./linttmp/process.L: ../../lwp/asynch.h
./lib/process.o ./linttmp/process.L: ./machlwp/machsig.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/wait.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/time.h
./lib/process.o ./linttmp/process.L: /usr/include/sys/resource.h
./lib/process.o ./linttmp/process.L: /usr/include/signal.h
./lib/process.o ./linttmp/process.L: /usr/include/vm/faultcode.h
./lib/process.o ./linttmp/process.L: ./machlwp/machdep.h
./lib/process.o ./linttmp/process.L: ./machlwp/lwpmachdep1.h
./lib/process.o ./linttmp/process.L: ./machlwp/stackdep.h
./lib/process.o ./linttmp/process.L: ../../lwp/lwperror.h
./lib/process.o ./linttmp/process.L: ../../lwp/cntxt.h
./lib/process.o ./linttmp/process.L: ../../lwp/message.h
./lib/process.o ./linttmp/process.L: ../../lwp/process.h
./lib/process.o ./linttmp/process.L: ../../lwp/schedule.h
./lib/process.o ./linttmp/process.L: ../../lwp/alloc.h
./lib/process.o ./linttmp/process.L: ../../lwp/condvar.h
./lib/process.o ./linttmp/process.L: ../../lwp/monitor.h
./lib/process.o ./linttmp/process.L: ../../lwp/agent.h
./lib/process.o ./linttmp/process.L: ./machlwp/except.h
./lib/condvar.o ./linttmp/condvar.L: ../../condvar.c
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/common.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/types.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/sysmacros.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/trace.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/lwp.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/time.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/time.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/varargs.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/queue.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/asynch.h
./lib/condvar.o ./linttmp/condvar.L: ./machlwp/machsig.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/wait.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/time.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/sys/resource.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/signal.h
./lib/condvar.o ./linttmp/condvar.L: /usr/include/vm/faultcode.h
./lib/condvar.o ./linttmp/condvar.L: ./machlwp/machdep.h
./lib/condvar.o ./linttmp/condvar.L: ./machlwp/lwpmachdep1.h
./lib/condvar.o ./linttmp/condvar.L: ./machlwp/stackdep.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/queue.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/cntxt.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/lwperror.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/message.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/process.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/schedule.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/monitor.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/condvar.h
./lib/condvar.o ./linttmp/condvar.L: ../../lwp/alloc.h
./lib/agent.o ./linttmp/agent.L: ../../agent.c
./lib/agent.o ./linttmp/agent.L: ../../lwp/common.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/types.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/sysmacros.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/trace.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/lwp.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/time.h
./lib/agent.o ./linttmp/agent.L: /usr/include/time.h
./lib/agent.o ./linttmp/agent.L: /usr/include/varargs.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/queue.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/asynch.h
./lib/agent.o ./linttmp/agent.L: ./machlwp/machsig.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/wait.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/time.h
./lib/agent.o ./linttmp/agent.L: /usr/include/sys/resource.h
./lib/agent.o ./linttmp/agent.L: /usr/include/signal.h
./lib/agent.o ./linttmp/agent.L: /usr/include/vm/faultcode.h
./lib/agent.o ./linttmp/agent.L: ./machlwp/machdep.h
./lib/agent.o ./linttmp/agent.L: ./machlwp/lwpmachdep1.h
./lib/agent.o ./linttmp/agent.L: ./machlwp/stackdep.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/cntxt.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/lwperror.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/message.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/process.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/schedule.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/agent.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/alloc.h
./lib/agent.o ./linttmp/agent.L: ../../lwp/monitor.h
./lib/asynch.o ./linttmp/asynch.L: ../../asynch.c
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/common.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/types.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/sysmacros.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/trace.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/lwp.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/time.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/time.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/varargs.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/queue.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/asynch.h
./lib/asynch.o ./linttmp/asynch.L: ./machlwp/machsig.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/wait.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/time.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/sys/resource.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/signal.h
./lib/asynch.o ./linttmp/asynch.L: /usr/include/vm/faultcode.h
./lib/asynch.o ./linttmp/asynch.L: ./machlwp/machdep.h
./lib/asynch.o ./linttmp/asynch.L: ./machlwp/lwpmachdep1.h
./lib/asynch.o ./linttmp/asynch.L: ./machlwp/stackdep.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/cntxt.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/lwperror.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/message.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/process.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/schedule.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/agent.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/alloc.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/clock.h
./lib/asynch.o ./linttmp/asynch.L: ../../lwp/monitor.h
./lib/message.o ./linttmp/message.L: ../../message.c
./lib/message.o ./linttmp/message.L: ../../lwp/common.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/types.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/sysmacros.h
./lib/message.o ./linttmp/message.L: ../../lwp/trace.h
./lib/message.o ./linttmp/message.L: ../../lwp/lwp.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/time.h
./lib/message.o ./linttmp/message.L: /usr/include/time.h
./lib/message.o ./linttmp/message.L: /usr/include/varargs.h
./lib/message.o ./linttmp/message.L: ../../lwp/queue.h
./lib/message.o ./linttmp/message.L: ../../lwp/asynch.h
./lib/message.o ./linttmp/message.L: ./machlwp/machsig.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/wait.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/time.h
./lib/message.o ./linttmp/message.L: /usr/include/sys/resource.h
./lib/message.o ./linttmp/message.L: /usr/include/signal.h
./lib/message.o ./linttmp/message.L: /usr/include/vm/faultcode.h
./lib/message.o ./linttmp/message.L: ./machlwp/machdep.h
./lib/message.o ./linttmp/message.L: ./machlwp/lwpmachdep1.h
./lib/message.o ./linttmp/message.L: ./machlwp/stackdep.h
./lib/message.o ./linttmp/message.L: ../../lwp/cntxt.h
./lib/message.o ./linttmp/message.L: ../../lwp/lwperror.h
./lib/message.o ./linttmp/message.L: ../../lwp/message.h
./lib/message.o ./linttmp/message.L: ../../lwp/process.h
./lib/message.o ./linttmp/message.L: ../../lwp/schedule.h
./lib/message.o ./linttmp/message.L: ../../lwp/agent.h
./lib/message.o ./linttmp/message.L: ../../lwp/alloc.h
./lib/message.o ./linttmp/message.L: ../../lwp/monitor.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwperror.c
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/common.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/types.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/sysmacros.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/trace.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/lwp.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/time.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/time.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/varargs.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/queue.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/asynch.h
./lib/lwperror.o ./linttmp/lwperror.L: ./machlwp/machsig.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/wait.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/time.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/sys/resource.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/signal.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/vm/faultcode.h
./lib/lwperror.o ./linttmp/lwperror.L: ./machlwp/machdep.h
./lib/lwperror.o ./linttmp/lwperror.L: ./machlwp/lwpmachdep1.h
./lib/lwperror.o ./linttmp/lwperror.L: ./machlwp/stackdep.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/cntxt.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/lwperror.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/message.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/process.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/schedule.h
./lib/lwperror.o ./linttmp/lwperror.L: ../../lwp/monitor.h
./lib/lwperror.o ./linttmp/lwperror.L: /usr/include/stdio.h
./lib/monitor.o ./linttmp/monitor.L: ../../monitor.c
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/common.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/types.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/sysmacros.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/trace.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/lwp.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/time.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/time.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/varargs.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/queue.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/asynch.h
./lib/monitor.o ./linttmp/monitor.L: ./machlwp/machsig.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/wait.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/time.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/sys/resource.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/signal.h
./lib/monitor.o ./linttmp/monitor.L: /usr/include/vm/faultcode.h
./lib/monitor.o ./linttmp/monitor.L: ./machlwp/machdep.h
./lib/monitor.o ./linttmp/monitor.L: ./machlwp/lwpmachdep1.h
./lib/monitor.o ./linttmp/monitor.L: ./machlwp/stackdep.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/cntxt.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/lwperror.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/message.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/process.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/schedule.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/asynch.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/monitor.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/condvar.h
./lib/monitor.o ./linttmp/monitor.L: ../../lwp/alloc.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwpclock.c
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/common.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/types.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/sysmacros.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/trace.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/lwp.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/time.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/time.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/varargs.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/queue.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/asynch.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ./machlwp/machsig.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/wait.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/time.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/sys/resource.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/signal.h
./lib/lwpclock.o ./linttmp/lwpclock.L: /usr/include/vm/faultcode.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ./machlwp/machdep.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ./machlwp/lwpmachdep1.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ./machlwp/stackdep.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/queue.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/cntxt.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/lwperror.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/message.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/process.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/alloc.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/clock.h
./lib/lwpclock.o ./linttmp/lwpclock.L: ../../lwp/monitor.h
./lib/alloc.o ./linttmp/alloc.L: ../../alloc.c
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/common.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/types.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/sysmacros.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/trace.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/lwp.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/time.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/time.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/varargs.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/queue.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/asynch.h
./lib/alloc.o ./linttmp/alloc.L: ./machlwp/machsig.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/wait.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/time.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/sys/resource.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/signal.h
./lib/alloc.o ./linttmp/alloc.L: /usr/include/vm/faultcode.h
./lib/alloc.o ./linttmp/alloc.L: ./machlwp/machdep.h
./lib/alloc.o ./linttmp/alloc.L: ./machlwp/lwpmachdep1.h
./lib/alloc.o ./linttmp/alloc.L: ./machlwp/stackdep.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/cntxt.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/lwperror.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/message.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/process.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/schedule.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/alloc.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/condvar.h
./lib/alloc.o ./linttmp/alloc.L: ../../lwp/monitor.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../cntxt.c
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/common.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/types.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/sysmacros.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/trace.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/lwp.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/time.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/time.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/varargs.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/queue.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/asynch.h
./lib/cntxt.o ./linttmp/cntxt.L: ./machlwp/machsig.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/wait.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/time.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/sys/resource.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/signal.h
./lib/cntxt.o ./linttmp/cntxt.L: /usr/include/vm/faultcode.h
./lib/cntxt.o ./linttmp/cntxt.L: ./machlwp/machdep.h
./lib/cntxt.o ./linttmp/cntxt.L: ./machlwp/lwpmachdep1.h
./lib/cntxt.o ./linttmp/cntxt.L: ./machlwp/stackdep.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/queue.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/lwperror.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/cntxt.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/message.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/process.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/schedule.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/alloc.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/condvar.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/monitor.h
./lib/cntxt.o ./linttmp/cntxt.L: ../../lwp/agent.h
./lib/cntxt.o ./linttmp/cntxt.L: ./machlwp/except.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../libc/on_exit.c
./lib/on_exit.o ./linttmp/on_exit.L: ../../common.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/types.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/sysmacros.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../lwp/trace.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../lwp/lwp.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/time.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/time.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/varargs.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../queue.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../asynch.h
./lib/on_exit.o ./linttmp/on_exit.L: ./machsig.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/wait.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/time.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/sys/resource.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/signal.h
./lib/on_exit.o ./linttmp/on_exit.L: /usr/include/vm/faultcode.h
./lib/on_exit.o ./linttmp/on_exit.L: ./machdep.h
./lib/on_exit.o ./linttmp/on_exit.L: ./lwpmachdep1.h
./lib/on_exit.o ./linttmp/on_exit.L: ./stackdep.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../lwperror.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../cntxt.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../message.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../process.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../schedule.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../alloc.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../condvar.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../monitor.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../agent.h
./lib/on_exit.o ./linttmp/on_exit.L: ../../libc/libc.h
./lib/machdep.o ./linttmp/machdep.L: machdep.c
./lib/machdep.o ./linttmp/machdep.L: ../../common.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/types.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/sysmacros.h
./lib/machdep.o ./linttmp/machdep.L: ../../lwp/trace.h
./lib/machdep.o ./linttmp/machdep.L: ../../lwp/lwp.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/time.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/time.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/varargs.h
./lib/machdep.o ./linttmp/machdep.L: ../../queue.h
./lib/machdep.o ./linttmp/machdep.L: ../../asynch.h
./lib/machdep.o ./linttmp/machdep.L: ./machsig.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/wait.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/time.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/sys/resource.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/signal.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/vm/faultcode.h
./lib/machdep.o ./linttmp/machdep.L: ./machdep.h
./lib/machdep.o ./linttmp/machdep.L: ./lwpmachdep1.h
./lib/machdep.o ./linttmp/machdep.L: ./stackdep.h
./lib/machdep.o ./linttmp/machdep.L: ../../cntxt.h
./lib/machdep.o ./linttmp/machdep.L: ../../lwperror.h
./lib/machdep.o ./linttmp/machdep.L: ../../message.h
./lib/machdep.o ./linttmp/machdep.L: ../../process.h
./lib/machdep.o ./linttmp/machdep.L: ../../schedule.h
./lib/machdep.o ./linttmp/machdep.L: ../../agent.h
./lib/machdep.o ./linttmp/machdep.L: ../../alloc.h
./lib/machdep.o ./linttmp/machdep.L: ../../monitor.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/signal.h
./lib/machdep.o ./linttmp/machdep.L: ../../libc/libc.h
./lib/machdep.o ./linttmp/machdep.L: /usr/include/machine/psl.h
./lib/except.o ./linttmp/except.L: except.c
./lib/except.o ./linttmp/except.L: ../../common.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/types.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/sysmacros.h
./lib/except.o ./linttmp/except.L: ../../lwp/trace.h
./lib/except.o ./linttmp/except.L: ../../lwp/lwp.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/time.h
./lib/except.o ./linttmp/except.L: /usr/include/time.h
./lib/except.o ./linttmp/except.L: /usr/include/varargs.h
./lib/except.o ./linttmp/except.L: ../../queue.h
./lib/except.o ./linttmp/except.L: ../../asynch.h
./lib/except.o ./linttmp/except.L: ./machsig.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/wait.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/time.h
./lib/except.o ./linttmp/except.L: /usr/include/sys/resource.h
./lib/except.o ./linttmp/except.L: /usr/include/signal.h
./lib/except.o ./linttmp/except.L: /usr/include/vm/faultcode.h
./lib/except.o ./linttmp/except.L: ./machdep.h
./lib/except.o ./linttmp/except.L: ./lwpmachdep1.h
./lib/except.o ./linttmp/except.L: ./stackdep.h
./lib/except.o ./linttmp/except.L: ../../queue.h
./lib/except.o ./linttmp/except.L: ../../cntxt.h
./lib/except.o ./linttmp/except.L: ../../lwperror.h
./lib/except.o ./linttmp/except.L: ../../message.h
./lib/except.o ./linttmp/except.L: ../../process.h
./lib/except.o ./linttmp/except.L: ../../schedule.h
./lib/except.o ./linttmp/except.L: ../../alloc.h
./lib/except.o ./linttmp/except.L: ./except.h
./lib/except.o ./linttmp/except.L: ../../monitor.h
./lib/lwputil.o ./linttmp/lwputil.L: lwputil.c
./lib/lwputil.o ./linttmp/lwputil.L: ../../common.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/types.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/sysmacros.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../lwp/trace.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../lwp/lwp.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/time.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/time.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/varargs.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../queue.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../asynch.h
./lib/lwputil.o ./linttmp/lwputil.L: ./machsig.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/wait.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/time.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/sys/resource.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/signal.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/vm/faultcode.h
./lib/lwputil.o ./linttmp/lwputil.L: ./machdep.h
./lib/lwputil.o ./linttmp/lwputil.L: ./lwpmachdep1.h
./lib/lwputil.o ./linttmp/lwputil.L: ./stackdep.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../cntxt.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../lwperror.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../message.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../process.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../monitor.h
./lib/lwputil.o ./linttmp/lwputil.L: ../../schedule.h
./lib/lwputil.o ./linttmp/lwputil.L: /usr/include/stdio.h
./lib/machsig.o ./linttmp/machsig.L: machsig.c
./lib/machsig.o ./linttmp/machsig.L: /usr/include/signal.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/vm/faultcode.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/time.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/time.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/errno.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/errno.h
./lib/machsig.o ./linttmp/machsig.L: ../../common.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/types.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/sysmacros.h
./lib/machsig.o ./linttmp/machsig.L: ../../lwp/trace.h
./lib/machsig.o ./linttmp/machsig.L: ../../lwp/lwp.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/varargs.h
./lib/machsig.o ./linttmp/machsig.L: ../../queue.h
./lib/machsig.o ./linttmp/machsig.L: ../../asynch.h
./lib/machsig.o ./linttmp/machsig.L: ./machsig.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/wait.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/time.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/sys/resource.h
./lib/machsig.o ./linttmp/machsig.L: ./machdep.h
./lib/machsig.o ./linttmp/machsig.L: ./lwpmachdep1.h
./lib/machsig.o ./linttmp/machsig.L: ./stackdep.h
./lib/machsig.o ./linttmp/machsig.L: ../../cntxt.h
./lib/machsig.o ./linttmp/machsig.L: ../../lwperror.h
./lib/machsig.o ./linttmp/machsig.L: ../../message.h
./lib/machsig.o ./linttmp/machsig.L: ../../process.h
./lib/machsig.o ./linttmp/machsig.L: ../../schedule.h
./lib/machsig.o ./linttmp/machsig.L: ../../agent.h
./lib/machsig.o ./linttmp/machsig.L: ../../alloc.h
./lib/machsig.o ./linttmp/machsig.L: ../../monitor.h
./lib/machsig.o ./linttmp/machsig.L: ../../clock.h
./lib/machsig.o ./linttmp/machsig.L: /usr/include/syscall.h
./lib/stack.o ./linttmp/stack.L: stack.c
./lib/stack.o ./linttmp/stack.L: ../../common.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/types.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/sysmacros.h
./lib/stack.o ./linttmp/stack.L: ../../lwp/trace.h
./lib/stack.o ./linttmp/stack.L: ../../lwp/lwp.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/time.h
./lib/stack.o ./linttmp/stack.L: /usr/include/time.h
./lib/stack.o ./linttmp/stack.L: /usr/include/varargs.h
./lib/stack.o ./linttmp/stack.L: ../../queue.h
./lib/stack.o ./linttmp/stack.L: ../../asynch.h
./lib/stack.o ./linttmp/stack.L: ./machsig.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/wait.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/time.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/resource.h
./lib/stack.o ./linttmp/stack.L: /usr/include/signal.h
./lib/stack.o ./linttmp/stack.L: /usr/include/vm/faultcode.h
./lib/stack.o ./linttmp/stack.L: ./machdep.h
./lib/stack.o ./linttmp/stack.L: ./lwpmachdep1.h
./lib/stack.o ./linttmp/stack.L: ./stackdep.h
./lib/stack.o ./linttmp/stack.L: ../../cntxt.h
./lib/stack.o ./linttmp/stack.L: ../../lwperror.h
./lib/stack.o ./linttmp/stack.L: ../../message.h
./lib/stack.o ./linttmp/stack.L: ../../process.h
./lib/stack.o ./linttmp/stack.L: ../../schedule.h
./lib/stack.o ./linttmp/stack.L: ../../alloc.h
./lib/stack.o ./linttmp/stack.L: ../../condvar.h
./lib/stack.o ./linttmp/stack.L: ../../monitor.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/mman.h
./lib/stack.o ./linttmp/stack.L: /usr/include/sys/types.h
./lib/malloc.o ./linttmp/malloc.L: malloc.c
./lib/malloc.o ./linttmp/malloc.L: ./mallint.h
./lib/malloc.o ./linttmp/malloc.L: /usr/include/malloc.h
./lib/malloc.o ./linttmp/malloc.L: /usr/include/errno.h
./lib/malloc.o ./linttmp/malloc.L: /usr/include/sys/errno.h
./linttmp/Locore.L: Locore.c
./linttmp/Locore.L: ../../common.h
./linttmp/Locore.L: /usr/include/sys/types.h
./linttmp/Locore.L: /usr/include/sys/sysmacros.h
./linttmp/Locore.L: ../../lwp/trace.h
./linttmp/Locore.L: ../../lwp/lwp.h
./linttmp/Locore.L: /usr/include/sys/time.h
./linttmp/Locore.L: /usr/include/time.h
./linttmp/Locore.L: /usr/include/varargs.h
./linttmp/Locore.L: ../../queue.h
./linttmp/Locore.L: ../../asynch.h
./linttmp/Locore.L: ./machsig.h
./linttmp/Locore.L: /usr/include/sys/wait.h
./linttmp/Locore.L: /usr/include/sys/time.h
./linttmp/Locore.L: /usr/include/sys/resource.h
./linttmp/Locore.L: /usr/include/signal.h
./linttmp/Locore.L: /usr/include/vm/faultcode.h
./linttmp/Locore.L: ./machdep.h
./linttmp/Locore.L: ./lwpmachdep1.h
./linttmp/Locore.L: ./stackdep.h
./linttmp/Locore.L: ../../lwperror.h
./linttmp/Locore.L: ../../cntxt.h
./linttmp/Locore.L: ../../message.h
./linttmp/Locore.L: ../../process.h
./linttmp/Locore.L: ../../schedule.h
./linttmp/Locore.L: ../../alloc.h
./linttmp/Locore.L: ../../condvar.h
./linttmp/Locore.L: ../../monitor.h
./linttmp/Locore.L: ../../agent.h
./linttmp/Locore.L: ./except.h
./linttmp/Locore.L: ../../libc/libc.h
./linttmp/Locore.L: /usr/include/malloc.h
./linttmp/Locore.L: /usr/include/sys/mman.h
./linttmp/Locore.L: /usr/include/sys/types.h
