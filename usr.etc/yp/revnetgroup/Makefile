#
#  @(#)Makefile 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
#

HDRS = getgroup.h table.h util.h
SRCS = revnetgroup.c getgroup.c table.c util.c
OBJS = revnetgroup.o getgroup.o table.o util.o
CC   = /bin/cc

CFLAGS= -O
DESTDIR=

revnetgroup: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o revnetgroup

lint:
	lint $(SRCS)

clean: 
	rm -f revnetgroup  $(OBJS)

install: revnetgroup
	install -s revnetgroup $(DESTDIR)/usr/etc/yp


depend:
	cp /dev/null x.c
	for i in $(SRCS) ; do \
		(/bin/grep '^#[     ]*include' x.c $$i | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c/\.o/' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
 
# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
revnetgroup.o: /usr/include/stdio.h
revnetgroup.o: /usr/include/ctype.h
revnetgroup.o: /usr/include/strings.h
revnetgroup.o: /usr/include/pwd.h
revnetgroup.o: /usr/include/rpcsvc/ypclnt.h
revnetgroup.o: util.h
revnetgroup.o: table.h
revnetgroup.o: getgroup.h
getgroup.o: /usr/include/stdio.h
getgroup.o: /usr/include/ctype.h
getgroup.o: /usr/include/strings.h
getgroup.o: table.h
getgroup.o: util.h
getgroup.o: getgroup.h
table.o: /usr/include/ctype.h
table.o: util.h
table.o: table.h
util.o: /usr/include/stdio.h
util.o: util.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
