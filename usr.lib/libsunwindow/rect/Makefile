#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		rect
#HDRSPUBLIC=	rect.h rectlist.h
CFILESLIB=	rect.c rectlist.c rect_util.c rect_data.c
SPCFILESLIB=	rect_data.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/rect.c ${FULLPATH}/rectlist.c \
		${FULLPATH}/rect_util.c ${FULLPATH}/rect.h \
		${FULLPATH}/rectlist.h
NO_XSTR_FILES= rect_data.c

all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $$(OBJS)

rect.o: rect.c ${INCLUDE}/sunwindow/rect.h \
	/usr/include/stdio.h 
rect_util.o: rect_util.c \
	${INCLUDE}/sunwindow/rect.h 
rectlist.o: rectlist.c /usr/include/stdio.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/rectlist.h 

clean: master.clean
