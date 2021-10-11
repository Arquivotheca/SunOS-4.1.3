#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		misc
#HDRSPUBLIC=	emptysw.h expand_name.h fullscreen.h menu.h msgsw.h sunview.h
#HDRSPRIVATE=	cheap_text.h text_obj.h
CFILESLIB= 	msgsw.c emptysw.c demorandom.c cheap_text.c fullscreen.c \
		menu.c prompt.c expand_name.c expand_path.c cursor_confirm.c \
		misc_data.c
SPCFILESLIB=	misc_data.c cursor_confirm.c emptysw.c expand_name.c \
		menu.c demorandom.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/msgsw.c ${FULLPATH}/emptysw.c \
		${FULLPATH}/demorandom.c ${FULLPATH}/cheap_text.c \
		${FULLPATH}/fullscreen.c ${FULLPATH}/menu.c \
		${FULLPATH}/prompt.c ${FULLPATH}/expand_name.c \
		${FULLPATH}/expand_path.c ${FULLPATH}/cursor_confirm.c \
		${FULLPATH}/emptysw.h ${FULLPATH}/expand_name.h \
		${FULLPATH}/fullscreen.h ${FULLPATH}/menu.h \
		${FULLPATH}/msgsw.h ${FULLPATH}/sunview.h \
		${FULLPATH}/cheap_text.h ${FULLPATH}/misc_data.c 
NO_XSTR_FILES= misc_data.c

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
