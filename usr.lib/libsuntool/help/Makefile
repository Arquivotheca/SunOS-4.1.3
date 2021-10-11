#
# @(#)Makefile 1.1 92/07/30
#
WHOAMI=		help
#HDRSPUBLIC=	help.h
HDRSPRIVATE=	
CFILES=		help.c help_file.c help_rpc.c
CFILESLIB= 	help.c help_file.c help_rpc.c
SPCFILESLIB=	help.c help_rpc.c
OBJS=   $(CFILESLIB:%.c=$(VARIANT)/%.o)

TAGFILESALL=	${FULLPATH}/help.c \
		${FULLPATH}/help_file.c \
		${FULLPATH}/help_rpc.c \
		${FULLPATH}/help.h

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
