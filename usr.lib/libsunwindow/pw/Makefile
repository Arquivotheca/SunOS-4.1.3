#
#	@(#)Makefile  92/07/30 1.1 SMI
#
#HDRSPUBLIC=	pixwin.h pw_util.h pw_dblbuf.h

all : xall

WHOAMI=		pw
#HDRSPRIVATE= 	cursor_impl.h
CFILESLIB=	pw_access.c pw_cms.c pw_copy.c pw_get.c pw_put.c pw_read.c \
		pw_util.c pw_vector.c pw_rop.c pw_batchrop.c \
		pw_line.c pw_text.c pw_minlock.c pw_shared.c \
		pw_polygon2.c pw_traprop.c pw_polyline.c \
		pw_rotcmap.c pw_batch.c pw_polypoint.c pw_dbl.c \
		pw_rop_data.c
SPCFILESLIB=	pw_text.c pw_rop_data.c pw_cms.c pw_access.c
SFILES = 	$(ARCH)/pw_test_set.s
NO_XSTR_FILES= pw_rop_data.c

include ../Makefile.arch
include ../Makefile.master

TAGFILESALL=	${FULLPATH}/pw_access.c ${FULLPATH}/pw_cms.c \
		${FULLPATH}/pw_copy.c ${FULLPATH}/pw_get.c \
		${FULLPATH}/pw_put.c ${FULLPATH}/pw_read.c \
		${FULLPATH}/pw_util.c ${FULLPATH}/pw_vector.c \
		${FULLPATH}/pw_rop.c ${FULLPATH}/pw_batchrop.c \
		${FULLPATH}/pw_line.c ${FULLPATH}/pw_text.c \
		${FULLPATH}/pw_minlock.c ${FULLPATH}/pw_shared.c \
		${FULLPATH}/pw_polygon2.c ${FULLPATH}/pw_traprop.c \
		${FULLPATH}/pw_polyline.c \
		${FULLPATH}/pw_rotcmap.c ${FULLPATH}/pw_batch.c \
		${FULLPATH}/pw_polypoint.c ${FULLPATH}/pixwin.h \
		${FULLPATH}/pw_util.h ${FULLPATH}/pw_dbl.c

OBJS =  $(SFILES:$(ARCH)/%.s=$(VARIANT)/%.o) $(CFILESLIB:%.c=$(VARIANT)/%.o) 

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

xall : $$(LIBS)

$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $(SFILES) $$(OBJS)

#chain.h:
#	if [ ! -f ../../libcgi/chain.h ]; then \
#		(cd ../../libcgi; sccs get chain.h;) \
#	fi; \
#	if [ ! -f chain.h ]; then \
#		ln -s ../../libcgi/chain.h chain.h; \
#	fi
#
#cursor_impl.h:
#	if [ ! -s cursor_impl.h ]; then \
#		ln -s ../misc/cursor_impl.h cursor_impl.h; \
#	fi

clean: master.clean
