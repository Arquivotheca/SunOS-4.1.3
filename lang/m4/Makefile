#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 1.1 12/15/82
#

CPP_FLAGS=-DSUNPRO -I$(VROOT)
CPP=/lib/cpp
CFLAGS=-O $(CPP_FLAGS)
VROOT=../vroot
HDRS= $(VROOT)/report.h $(VROOT)/vroot.h
OBJS= m4.o m4y.o

m4:	$(OBJS) $(VROOT)/vroot.a
	$(CC) -o m4 $(OBJS) $(VROOT)/vroot.a

$(HDRS) $(VROOT)/vroot.a:
	cd $(@D); $(MAKE) $(@F)

install: m4
	install -s m4 $(DESTDIR)/usr/bin

clean:
	rm -f $(OBJS) m4 a.out core errs m4y.c

depend: $(OBJS:.o=.c) $(HDRS)
	rm -f makedep
	for i in $(OBJS:.o=.c); do \
	    ( $(CPP) -M $(CPP_FLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
m4.o: m4.c
m4.o: /usr/include/stdio.h
m4.o: /usr/include/signal.h
m4.o: /usr/include/sys/file.h
m4.o: /usr/include/sys/fcntl.h
m4.o: ../vroot/report.h
m4.o: /usr/include/stdio.h
m4.o: ../vroot/vroot.h
m4.o: /usr/include/stdio.h
m4y.o: m4y.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
