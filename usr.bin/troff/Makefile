#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 1.4 03/30/83
#
DESTDIR =
SRCS	= \
	ni.c nii.c n1.c n2.c \
	n3.c n4.c n5.c t6.c \
	n7.c n8.c n9.c t10.c \
	hytab.c ntab.c suftab.c
HDRS	= \
	d.h tw.h v.h tdef.h sdef.h
CPP	= /lib/cpp
CPPFLAGS = -I.
CFLAGS	= -O -DVMUNIX
c	= ni.o nii.o n1.o n2.o n3.o n4.o n5.o t6.o n7.o n8.o n9.o t10.o
s	= hytab.o suftab.o tab3.o

troff: $c $s
	$(CC) -o troff $s $c  

hytab.o: hytab.c
	$(CC) -R -c hytab.c

suftab.o: suftab.c
	$(CC) -R -c suftab.c

install: troff
	install -s troff ${DESTDIR}/usr/bin

clean:
	rm -f *.o troff

depend: $(SRCS) $(HDRS)
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i  | \
		sed -e '/sysmacros/d' >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f makedep eddep

# DO NOT DELETE THIS LINE -- make depend uses it

ni.o: ni.c
ni.o: ./tdef.h
ni.o: /usr/include/signal.h
nii.o: nii.c
nii.o: ./tdef.h
nii.o: /usr/include/signal.h
nii.o: ./sdef.h
nii.o: ./d.h
nii.o: ./v.h
nii.o: /usr/include/sgtty.h
nii.o: /usr/include/sys/ioctl.h
nii.o: /usr/include/sys/ttychars.h
nii.o: /usr/include/sys/ttydev.h
n1.o: n1.c
n1.o: ./tdef.h
n1.o: /usr/include/signal.h
n1.o: /usr/include/sys/types.h
n1.o: /usr/include/sys/stat.h
n1.o: /usr/include/sys/time.h
n1.o: /usr/include/time.h
n1.o: ./d.h
n1.o: ./v.h
n1.o: ./sdef.h
n1.o: /usr/include/setjmp.h
n1.o: /usr/include/sgtty.h
n1.o: /usr/include/sys/ioctl.h
n1.o: /usr/include/sys/ttychars.h
n1.o: /usr/include/sys/ttydev.h
n2.o: n2.c
n2.o: ./tdef.h
n2.o: /usr/include/signal.h
n2.o: /usr/include/sgtty.h
n2.o: /usr/include/sys/ioctl.h
n2.o: /usr/include/sys/ttychars.h
n2.o: /usr/include/sys/ttydev.h
n2.o: ./d.h
n2.o: ./v.h
n2.o: ./sdef.h
n2.o: /usr/include/setjmp.h
n3.o: n3.c
n3.o: ./tdef.h
n3.o: /usr/include/signal.h
n3.o: ./d.h
n3.o: ./v.h
n3.o: ./sdef.h
n4.o: n4.c
n4.o: ./tdef.h
n4.o: /usr/include/signal.h
n4.o: ./d.h
n4.o: ./v.h
n4.o: ./sdef.h
n5.o: n5.c
n5.o: ./tdef.h
n5.o: /usr/include/signal.h
n5.o: /usr/include/sgtty.h
n5.o: /usr/include/sys/ioctl.h
n5.o: /usr/include/sys/ttychars.h
n5.o: /usr/include/sys/ttydev.h
n5.o: ./d.h
n5.o: ./v.h
n5.o: ./sdef.h
t6.o: t6.c
t6.o: ./tdef.h
t6.o: /usr/include/signal.h
t6.o: ./d.h
t6.o: ./v.h
n7.o: n7.c
n7.o: ./tdef.h
n7.o: /usr/include/signal.h
n7.o: ./d.h
n7.o: ./v.h
n7.o: ./sdef.h
n8.o: n8.c
n8.o: ./tdef.h
n8.o: /usr/include/signal.h
n9.o: n9.c
n9.o: ./tdef.h
n9.o: /usr/include/signal.h
n9.o: ./d.h
n9.o: ./v.h
t10.o: t10.c
t10.o: ./tdef.h
t10.o: /usr/include/signal.h
t10.o: ./d.h
t10.o: ./v.h
hytab.o: hytab.c
ntab.o: ntab.c
suftab.o: suftab.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
