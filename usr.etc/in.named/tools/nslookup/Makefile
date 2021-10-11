#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile 1.1 92/07/30 SMI from UCB 5.3 1/16/86
#
DESTDIR=
HELP=   nslookup.help
DEFS=
INCPATH= 
CFLAGS=	-O  ${DEFS}
LIBS=	-lresolv -ll
LFLAGS=	
CC= /bin/cc

# The name server assistant
#
PROG=	nslookup

# Sources
#
SRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c 

# Objects
#
OBJS=	main.o getinfo.o debug.o send.o skip.o list.o subr.o lex.yy.o 

# Header files
#
HDRS=	res.h

${PROG}:  ${HDRS} ${OBJS}
	$(CC) ${LFLAGS} -o ${PROG} ${OBJS} ${LIBS}

${OBJS}: 
	$(CC) ${CFLAGS} ${INCPATH} -c $*.c

lex.yy.c: commands.l
	${LEX} commands.l

install: ${PROG} ${HELP}
	install -s ${PROG} ${DESTDIR}/usr/etc/${PROG}
	install -c -m 644 ${HELP} ${DESTDIR}/usr/lib/${HELP}

clean:
	rm -f a.out core lex.yy.c ${OBJS} ${PROG}

lint:
	lint -hn ${INCPATH} ${DEFS} ${SRCS} 

depend:
	$(CC) -E ${INCPATH} ${SRCS} | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
