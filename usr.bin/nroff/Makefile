#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 06/30/83
#
.DEFAULT:
	$(GET) $(GFLAGS) -G$< $(@D)/SCCS/s.$(@F)

DESTDIR =
SRCS = \
	ni.c nii.c n1.c n2.c \
	n3.c n4.c n5.c n6.c \
	n7.c n8.c n9.c n10.c \
	hytab.c ntab.c suftab.c \
	d.h tw.h v.h tdef.h sdef.h

T=../troff/
CFLAGS = -O -DNROFF -DVMUNIX -I$(T) 
c = ni.o nii.o n1.o n2.o n3.o n4.o n5.o n6.o n7.o n8.o n9.o n10.o
s = hytab.o ntab.o suftab.o

all:	nroff term

nroff: $c $s
	$(CC) -o nroff  $s $c  

ni.o:	$(T)ni.c $(T)tdef.h
	$(CC) $(CFLAGS) -c $(T)ni.c

nii.o:	$(T)nii.c $(T)tw.h $(T)sdef.h $(T)d.h $(T)v.h
	$(CC) $(CFLAGS) -c $(T)nii.c

n1.o:	$(T)n1.c
	$(CC) $(CFLAGS) -c $(T)n1.c

n2.o:	$(T)n2.c
	$(CC) $(CFLAGS) -c $(T)n2.c

n3.o:	$(T)n3.c
	$(CC) $(CFLAGS) -c $(T)n3.c

n4.o:	$(T)n4.c
	$(CC) $(CFLAGS) -c $(T)n4.c

n5.o:	$(T)n5.c
	$(CC) $(CFLAGS) -c $(T)n5.c

n7.o:	$(T)n7.c
	$(CC) $(CFLAGS) -c $(T)n7.c

n8.o:	$(T)n8.c
	$(CC) $(CFLAGS) -c $(T)n8.c

n9.o:	$(T)n9.c
	$(CC) $(CFLAGS) -c $(T)n9.c

hytab.o:	$(T)hytab.c
	$(CC) -R -c $(T)hytab.c

ntab.o:	$(T)ntab.c
	$(CC) -R -c $(T)ntab.c

suftab.o:	$(T)suftab.c
	$(CC) -R -c $(T)suftab.c

term:	FRC
	cd term; make ${MFLAGS}

FRC:

install: all
	install -s nroff ${DESTDIR}/usr/bin
	cd term; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	rm -f *.o nroff
	cd term; make ${MFLAGS} clean
