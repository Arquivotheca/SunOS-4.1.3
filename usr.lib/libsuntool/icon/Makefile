#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		icon
#HDRSPUBLIC=	icon.h icon_load.h 
HDRSPRIVATE=	
CFILESLIB= 	icon.c icon_object.c icon_load.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/icon.c ${FULLPATH}/icon_object.c \
		${FULLPATH}/icon_load.c ${FULLPATH}/icon.h \
		${FULLPATH}/icon_load.h
all: xall

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean

