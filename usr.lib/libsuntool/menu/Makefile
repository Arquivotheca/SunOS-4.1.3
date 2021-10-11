#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		menu
#HDRSPUBLIC=	image.h walkmenu.h 
#HDRSPRIVATE=	image_impl.h walkmenu_impl.h
CFILESLIB= 	image.c walkmenu_public.c walkmenu_render.c \
		 walkmenu_set.c walkmenu_get.c walkmenu_data.c
SPCFILESLIB=	walkmenu_public.c walkmenu_set.c walkmenu_data.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/image.c ${FULLPATH}/walkmenu_public.c \
		${FULLPATH}/walkmenu_set.c ${FULLPATH}/walkmenu_get.c \
		${FULLPATH}/image.h ${FULLPATH}/walkmenu.h \
		${FULLPATH}/image_impl.h ${FULLPATH}/walkmenu_impl.h \
		${FULLPATH}/walkmenu_render.c 
NO_XSTR_FILES= walkmenu_data.c

all: xall

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean
