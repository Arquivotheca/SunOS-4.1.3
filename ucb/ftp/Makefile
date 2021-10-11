#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.2 2/3/86
#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
ALL=	ftp
DESTDIR=
CFLAGS=	-O

all:	${ALL}

ftp:	cmds.o cmdtab.o ftp.o getpass.o glob.o main.o pclose.o ruserpass.o              domacro.o
	${CC} cmds.o cmdtab.o ftp.o getpass.o glob.o main.o pclose.o ruserpass.o         domacro.o -o ftp 

ftp.o:	/usr/include/arpa/ftp.h ftp_var.h
cmds.o:	/usr/include/arpa/ftp.h ftp_var.h
cmdtab.o: ftp_var.h
ftp.o:	ftp_var.h
main.o: ftp_var.h

install: all
	install -s ftp ${DESTDIR}/usr/ucb/ftp

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.?
