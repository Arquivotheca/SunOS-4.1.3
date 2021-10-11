#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		wmgr
#HDRSPUBLIC=	wmgr.h 
#HDRSPRIVATE=	wmgr_policy.h ../tool_impl.h
CFILESLIB= 	wmgr_cursors.c wmgr_images.c wmgr_walkmenu.c wmgr_menu.c \
		wmgr_rect.c wmgr_state.c wmgr_data.c \
		wmgr_policy.c wmgr_findspace.c wmgr_attic.c wmgr_confirm.c
SPCFILESLIB=	wmgr_data.c wmgr_cursors.c wmgr_policy.c wmgr_rect.c \
		wmgr_walkmenu.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/wmgr_cursors.c ${FULLPATH}/wmgr_walkmenu.c \
		${FULLPATH}/wmgr_rect.c ${FULLPATH}/wmgr_state.c \
		${FULLPATH}/wmgr_policy.c ${FULLPATH}/wmgr_menu.c \
		${FULLPATH}/wmgr_findspace.c ${FULLPATH}/wmgr_attic.c \
		${FULLPATH}/wmgr.h ${FULLPATH}/wmgr_policy.h \
		${FULLPATH}/wmgr_data.c 
NO_XSTR_FILES= wmgr_data.c wmgr_walkmenu.c

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

#../tool_impl.h:
#	if [ ! -f ../tool_impl.h ]; then \
#		ln -s tool/tool_impl.h ../tool_impl.h; \
#	fi
