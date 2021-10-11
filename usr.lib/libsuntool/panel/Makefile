#
# @(#)Makefile 1.1 92/07/30 SMI
#
WHOAMI=		panel
#HDRSPUBLIC=	panel.h
#HDRSPRIVATE=	panel_impl.h
CFILESLIB= 	panel_attr.c panel_line.c panel_notify.c \
		panel_select.c panel_text.c panel.c\
		panel_button.c panel_menu.c panel_public.c \
		panel_seln.c panel_tio.c \
		panel_choice.c panel_message.c panel_scroll.c \
		panel_slider.c panel_util.c \
		panel_create.c panel_paint.c panel_image.c panel_sw.c \
		panel_util_data.c panel_dclick.c
SPCFILESLIB=	panel_util_data.c panel_button.c panel_choice.c \
		panel_create.c panel_line.c panel_menu.c panel_message.c \
		panel_slider.c panel_text.c panel_util.c \
		panel.c panel_sw.c panel_dclick.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	$(CFILESLIB: %=$(FULLPATH)/%)
OLDTAGFILESALL=	${FULLPATH}/panel_attr.c ${FULLPATH}/panel_line.c \
		${FULLPATH}/panel_select.c ${FULLPATH}/panel_notify.c \
		${FULLPATH}/panel_text.c ${FULLPATH}/panel.c\
		${FULLPATH}/panel_button.c ${FULLPATH}/panel_menu.c \
		${FULLPATH}/panel_choice.c ${FULLPATH}/panel_public.c \
		${FULLPATH}/panel_seln.c ${FULLPATH}/panel_tio.c \
		${FULLPATH}/panel_message.c ${FULLPATH}/panel_scroll.c \
		${FULLPATH}/panel_slider.c ${FULLPATH}/panel_util.c \
		${FULLPATH}/panel_create.c ${FULLPATH}/panel_paint.c \
		${FULLPATH}/panel_image.c ${FULLPATH}/panel_sw.c \
		${FULLPATH}/panel_util_data.c \
		${FULLPATH}/panel.h ${FULLPATH}/panel_impl.h
NO_XSTR_FILES= panel_util_data.c

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

all: xall

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean
