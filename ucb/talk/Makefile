#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 1.3 83/06/23
#
CFLAGS=-O
DOBJECT = in.talkd.o announce.o process.o table.o print.o
TOBJECT = talk.o get_names.o display.o io.o ctl.o init_disp.o\
	  msgs.o get_addrs.o ctl_transact.o invite.o look_up.o
DSOURCE = in.talkd.c announce.c process.c table.c print.c
TSOURCE = talk.c get_names.c display.c io.c ctl.c init_disp.c\
	  msgs.c get_addrs.c ctl_transact.c invite.c look_up.c
INCLUDE = talk.h ctl.h talk_ctl.h
SOURCE = ${DSOURCE} ${TSOURCE}

all:	talk in.talkd

in.talkd:	${DOBJECT}
	${CC} ${CFLAGS} -o in.talkd ${DOBJECT}

talk:	${TOBJECT}
	${CC} -o talk ${TOBJECT} -lcurses -ltermlib

${DOBJECT} : ctl.h
${TOBJECT} : talk.h ctl.h talk_ctl.h

install: talk in.talkd
	install -g tty -m 2755 -s talk ${DESTDIR}/usr/ucb/talk
	install -s in.talkd ${DESTDIR}/usr/etc/in.talkd

lint:
	lint ${DSOURCE}

clean:
	rm -f *.o talk in.talkd

tags:	${SOURCE} ${INCLUDE}
	ctags ${SOURCE} ${INCLUDE}
