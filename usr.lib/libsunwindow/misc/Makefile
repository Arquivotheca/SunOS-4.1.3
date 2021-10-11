#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		misc
#HDRSPUBLIC=	cms.h cms_grays.h cms_mono.h cms_rainbow.h cms_rgb.h sun.h \
		window_hs.h bitmask.h hashfn.h
#HDRSPRIVATE=	cursor_impl.h
CFILESLIB=	cmdline.c getlogindir.c setenv.c cursor.c bitmask.c hashfn.c \
		pf.c sv_malloc.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/cmdline.c ${FULLPATH}/getlogindir.c \
		${FULLPATH}/setenv.c ${FULLPATH}/cursor.c \
		${FULLPATH}/cms.h ${FULLPATH}/cms_grays.h \
		${FULLPATH}/cms_mono.h ${FULLPATH}/cms_rainbow.h \
		${FULLPATH}/cms_rgb.h ${FULLPATH}/sun.h \
		${FULLPATH}/window_hs.h ${FULLPATH}/cursor_impl.h

all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master
xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

cmdline.o: cmdline.c 
cursor.o: cursor.c /usr/include/varargs.h \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	${INCLUDE}/pixrect/pixrect.h \
	${INCLUDE}/pixrect/memvar.h \
	${INCLUDE}/sunwindow/rect.h \
	${INCLUDE}/sunwindow/win_cursor.h \
	${INCLUDE}/sunwindow/attr.h cursor_impl.h 
getlogindir.o: getlogindir.c /usr/include/stdio.h /usr/include/pwd.h 
setenv.o: setenv.c /usr/include/strings.h 
bitmask.o: bitmask.c bitmask.h
hashfn.o: hashfn.c hashfn.h

clean: master.clean
