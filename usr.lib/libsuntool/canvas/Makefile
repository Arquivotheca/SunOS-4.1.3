#
# @(#)Makefile 1.1 92/07/30 SMI
#
WHOAMI=		canvas
#HDRSPUBLIC=	canvas.h
#HDRSPRIVATE=	canvas_impl.h
CFILESLIB= 	canvas.c canvas_scroll.c canvas_repaint.c canvas_resize.c
SPCFILESLIB=	canvas.c
OBJS =	$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/canvas.c ${FULLPATH}/canvas_scroll.c \
		${FULLPATH}/canvas_repnt.c ${FULLPATH}/canvas_resze.c \
		${FULLPATH}/canvas.h ${FULLPATH}/canvas_impl.h \
		$(FULLPATH)/frame.h $(FULLPATH)/frame_impl.h
all: xall

# include the common stuff from ../Makefile.master
include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =


xall: $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean

