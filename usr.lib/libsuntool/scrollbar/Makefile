#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		scrollbar
#HDRSPUBLIC=	scrollbar.h 
#HDRSPRIVATE=	scrollbar_impl.h
CFILESLIB= 	scrollbar_event.c scrollbar_public.c scrollbar_paint.c \
		scrollbar_data.c
SPCFILESLIB=	scrollbar_event.c scrollbar_public.c scrollbar_paint.c \
		scrollbar_data.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/scrollbar_event.c ${FULLPATH}/scrollbar_public.c \
		${FULLPATH}/scrollbar_paint.c  ${FULLPATH}/scrollbar_data.c \
		${FULLPATH}/scrollbar_impl.h ${FULLPATH}/scrollbar.h
NO_XSTR_FILES= scrollbar_data.c

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


