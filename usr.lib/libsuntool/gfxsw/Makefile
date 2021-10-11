#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		gfxsw
#HDRSPUBLIC=	gfx_hs.h gfxsw.h 
#HDRSPRIVATE=	gfxswimpl.h
CFILESLIB= 	gfxsw.c gfxsw_nocur.c gfxsw_input.c
SPCFILESLIB=	gfxsw_nocur.c
OBJS= 		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/gfxsw.c ${FULLPATH}/gfxsw_nocur.c \
		${FULLPATH}/gfxsw_input.c ${FULLPATH}/gfxswimpl.h \
		${FULLPATH}/gfx_hs.h ${FULLPATH}/gfxsw.h

all: xall

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean

