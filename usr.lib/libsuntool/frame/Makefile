#
# @(#)Makefile 1.1 92/07/30 SMI
#
WHOAMI=		frame
#HDRSPUBLIC=	frame.h
#HDRSPRIVATE=	frame_impl.h ../tool_impl.h
CFILESLIB=	frame.c frame_get.c frame_set.c frame_layout.c frame_shadow.c
OBJS=	$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL= 	$(CFILESLIB:%.c=$(FULLPATH)/%.c) \
		$(FULLPATH)/frame.h $(FULLPATH)/frame_impl.h
all: xall

# include the common stuff from ../Makefile.master
include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

xall: $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean

#../tool_impl.h:
#	if [ ! -f ../tool_impl.h ]; then \
#		ln -s tool/tool_impl.h ../tool_impl.h; \
#	fi
