#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		string_utils
#HDRSPUBLIC=	string_utils.h
CFILESLIB=	string_utils.c white_space.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/string_utils.c ${FULLPATH}/white_space.c \
		${FULLPATH}/string_utils.h
all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master
xall : $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

string_utils.o: string_utils.c /usr/include/ctype.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h \
	${INCLUDE}/sunwindow/string_utils.h 
white_space.o: white_space.c string_utils.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h 

clean: master.clean
