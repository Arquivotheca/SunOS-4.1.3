#
# Makefile for cdrom image creation utilities
#  @(#)Makefile 1.1 92/07/30 SMI    
#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#
# You can override the installation point by using:
#
#	make DESTDIR=string install		
#
ARCH:sh		= arch -k
DESTDIR		= ../bin
MKCDPROTO	= main.o util.o dlist.o dlist_path.o dlist_proto.o iso.o \
			label.o
INSTALLPART 	= installpart.o label.o
HDRS 		= iso_spec.h iso_impl.h hsfs_spec.h label.h
LABELIT 	= labelit.o
OFLAG		= -O
CFLAGS 		= $(OFLAG) -D$(ARCH)
ALLBIN		= mkcdproto installpart labelit

all: $(ALLBIN)

mkcdproto:	$(MKCDPROTO)
	$(CC) $(LDFLAGS) -o $@ $(MKCDPROTO) -lm

installpart: 	$(INSTALLPART)
	$(CC) $(LDFLAGS) -o $@ $(INSTALLPART) -lm

labelit:	$(LABELIT)
	$(CC) $(LDFLAGS) -o $@ $(LABELIT) 

$(MKCDPROTO) $(INSTALLPART) $(LABELIT):	$(HDRS)

install:	all
	if [ ! -d $(DESTDIR) ] ; then \
		mkdir $(DESTDIR) ;\
	fi ;\
	cp $(ALLBIN) $(DESTDIR)

###########################################

clean:
	-rm -f *.o $(ALLBIN) 
