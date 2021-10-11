#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		attr
#HDRSPUBLIC=	attr.h
#HDRSPRIVATE = attr_impl.h
CFILESLIB=	attr.c attr_copy.c attr_cu.c attr_util.c
SPCFILESLIB=	attr.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/attr.c ${FULLPATH}/attr_copy.c \
		${FULLPATH}/attr_cu.c ${FULLPATH}/attr.h \
		${FULLPATH}/attr_util.c

all: xall

#include common stuff from ../Makefile.master
include ../Makefile.master

xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

attr.o: attr.c /usr/include/stdio.h attr_impl.h \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	/usr/include/varargs.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/sun.h 
attr_copy.o: attr_copy.c ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	${INCLUDE}/sunwindow/attr.h 
attr_cu.o: attr_cu.c \
	${INCLUDE}/pixrect/pixrect_hs.h \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	${INCLUDE}/pixrect/pixrect.h \
	${INCLUDE}/pixrect/pr_planegroups.h \
	${INCLUDE}/pixrect/pr_util.h \
	${INCLUDE}/pixrect/bw1var.h \
	${INCLUDE}/pixrect/cg1var.h \
	${INCLUDE}/pixrect/bw2var.h \
	${INCLUDE}/pixrect/cg2var.h \
	${INCLUDE}/pixrect/gp1var.h \
	${INCLUDE}/pixrect/cg4var.h \
	${INCLUDE}/pixrect/memvar.h \
	${INCLUDE}/pixrect/pixfont.h \
	${INCLUDE}/pixrect/traprop.h \
	${INCLUDE}/pixrect/pr_line.h \
	${INCLUDE}/rasterfile.h \
	${INCLUDE}/pixrect/pr_io.h \
	${INCLUDE}/sunwindow/attr.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h 
attr_portable.o: attr_portable.c attr_impl.h \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	/usr/include/varargs.h \
	${INCLUDE}/sunwindow/attr.h 
attr_util.o: attr_util.c ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	/usr/include/varargs.h \
	${INCLUDE}/sunwindow/attr.h 

clean: master.clean
