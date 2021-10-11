#	@(#)Makefile 1.1 92/07/30  SMI from UCB 3.3 86/03/03
#
# Copyright (c) 1985 Regents of the University of California
#	All Rights Reserved
#
# Berkeley Internet name server
#
DEST=/usr/etc
CFLAGS= -O -DDEBUG
INCPATH= 
LDFLAGS=
LIBS= -lresolv
CC= /bin/cc


# The program itself
#
PROG=in.named

# Sources
#
SRCS= db_dump.c db_load.c db_lookup.c db_reload.c db_save.c db_update.c \
	ns_forw.c ns_init.c ns_main.c ns_maint.c ns_req.c ns_resp.c \
	ns_sort.c ns_subr.c version.c

# Objects
#
OBJS= db_dump.o db_load.o db_lookup.o db_reload.o db_save.o db_update.o \
	ns_forw.o ns_init.o ns_main.o ns_maint.o ns_req.o ns_resp.o \
	ns_sort.o ns_subr.o version.o

# Header files
#
HDRS=ns.h db.h	

all: ${PROG} ${PROG}-xfer

${PROG}: ${OBJS} ${HDRS} 
	$(CC) ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	cd tools; make ${MFLAGS}

${PROG}-xfer: xfer.o ns_subr.o
	$(CC) ${LDFLAGS} -o ${PROG}-xfer xfer.o ns_subr.o ${LIBS}

${OBJS}:
	$(CC) ${CFLAGS} ${INCPATH} -c $*.c

install: ${PROG}
	install -s ${PROG} ${DESTDIR}${DEST}/${PROG}
	install -s ${PROG}-xfer ${DESTDIR}${DEST}/${PROG}-xfer
	cd tools; make ${MFLAGS} DESTDIR=${DESTDIR} install

tags:
	ctags ${SRCS}

clean:
	rm -f a.out core ${OBJS} ${PROG}
	cd tools; make ${MFLAGS} clean

lint:
	lint ${SRCS}

depend:
	$(CC) -M ${INCPATH} ${SRCS} | \
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


db_dump.o: db_dump.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_dump.o: /usr/include/sys/time.h /usr/include/time.h
db_dump.o: /usr/include/netinet/in.h /usr/include/netdb.h /usr/include/stdio.h
db_dump.o: /usr/include/syslog.h /usr/include/arpa/nameser.h ./ns.h ./db.h
db_load.o: db_load.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_load.o: /usr/include/sys/time.h /usr/include/time.h
db_load.o: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/syslog.h
db_load.o: /usr/include/ctype.h /usr/include/netdb.h
db_load.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
db_lookup.o: db_lookup.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_lookup.o: /usr/include/stdio.h /usr/include/arpa/nameser.h ./db.h
db_reload.o: db_reload.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_reload.o: /usr/include/netinet/in.h /usr/include/stdio.h
db_reload.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
db_save.o: db_save.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_save.o: /usr/include/stdio.h /usr/include/syslog.h ./db.h
db_update.o: db_update.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
db_update.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/stdio.h
db_update.o: /usr/include/syslog.h /usr/include/arpa/nameser.h ./db.h
ns_forw.o: ns_forw.c /usr/include/stdio.h /usr/include/sys/types.h
ns_forw.o: /usr/include/sys/sysmacros.h /usr/include/sys/socket.h
ns_forw.o: /usr/include/sys/time.h /usr/include/time.h
ns_forw.o: /usr/include/netinet/in.h /usr/include/syslog.h
ns_forw.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
ns_init.o: ns_init.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ns_init.o: /usr/include/sys/socket.h /usr/include/sys/time.h
ns_init.o: /usr/include/time.h /usr/include/netinet/in.h /usr/include/stdio.h
ns_init.o: /usr/include/syslog.h /usr/include/ctype.h
ns_init.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
ns_main.o: ns_main.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ns_main.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h
ns_main.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/wait.h
ns_main.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
ns_main.o: /usr/include/sys/ttydev.h /usr/include/sgtty.h
ns_main.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
ns_main.o: /usr/include/stdio.h /usr/include/syslog.h /usr/include/errno.h
ns_main.o: /usr/include/sys/errno.h /usr/include/signal.h
ns_main.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
ns_maint.o: ns_maint.c /usr/include/sys/types.h /usr/include/sys/sysmacros.h
ns_maint.o: /usr/include/sys/socket.h /usr/include/sys/time.h
ns_maint.o: /usr/include/time.h /usr/include/netinet/in.h /usr/include/stdio.h
ns_maint.o: /usr/include/syslog.h /usr/include/arpa/nameser.h ./ns.h ./db.h
ns_req.o: ns_req.c /usr/include/stdio.h /usr/include/sys/types.h
ns_req.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
ns_req.o: /usr/include/time.h /usr/include/netinet/in.h /usr/include/syslog.h
ns_req.o: /usr/include/fcntl.h /usr/include/arpa/nameser.h ./ns.h ./db.h
ns_resp.o: ns_resp.c /usr/include/stdio.h /usr/include/sys/types.h
ns_resp.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
ns_resp.o: /usr/include/time.h /usr/include/sys/socket.h
ns_resp.o: /usr/include/netinet/in.h /usr/include/syslog.h
ns_resp.o: /usr/include/arpa/nameser.h ./ns.h ./db.h
cistrcmp.o: cistrcmp.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
