#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		filter
#HDRSPUBLIC=	filter.h
CFILESLIB=	filter.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/filter.c ${FULLPATH}/filter.h

all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master
xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

filter.o: filter.c /usr/include/ctype.h /usr/include/strings.h \
	${INCLUDE}/sunwindow/io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	${INCLUDE}/sunwindow/string_utils.h \
	${INCLUDE}/sunwindow/filter.h 

clean: master.clean
