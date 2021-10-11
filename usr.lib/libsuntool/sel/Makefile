#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		sel
#HDRSPUBLIC=	selection.h selection_attributes.h selection_svc.h seln.h 
#HDRSPRIVATE=	selection_impl.h
CFILESLIB= 	selection.c sel_appl.c sel_clnt.c sel_common.c sel_debug.c \
		sel_svc.c sel_policy.c sel_writable_data.c sel_xdr.c
SPCFILESLIB=	sel_debug.c sel_writable_data.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/selection.c ${FULLPATH}/sel_appl.c \
		${FULLPATH}/sel_clnt.c ${FULLPATH}/sel_common.c \
		${FULLPATH}/sel_debug.c ${FULLPATH}/sel_svc.c \
		${FULLPATH}/sel_policy.c ${FULLPATH}/sel_writable_data.c \
		${FULLPATH}/sel_xdr.c ${FULLPATH}/selection.h \
		${FULLPATH}/selection_attributes.h ${FULLPATH}/seln.h \
		${FULLPATH}/selection_svc.h ${FULLPATH}/selection_impl.h
NO_XSTR_FILES= sel_writable_data.c

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

