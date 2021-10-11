#
# @(#)Makefile 1.1 92/07/30 SMI
#
WHOAMI=		text
#ENTITYHDRS= 	entity_interpreter.h entity_stream.h entity_view.h ev_impl.h
#MISCHDRS=	primal.h ps_impl.h finger_table.h
#TEXTHDRS=	textsw_impl.h
#HDRSPRIVATE=	$(ENTITYHDRS) $(MISCHDRS) $(TEXTHDRS)
#HDRSPUBLIC=	textsw.h text.h

ESCFILESLIB=	es_file.c es_mem.c es_util.c es_attr.c ps_impl.c
EVCFILESLIB=	ev_display.c ev_op_bdry.c ev_edit.c ev_once.c ev_attr.c \
			    ev_field.c ev_update.c
MISCCFILESLIB=	es_copy_file.c ei_plain_text.c ei_attr.c finger_table.c
TEXTCFILESLIB=	textsw_once.c textsw_input.c textsw_edit.c textsw_menu.c \
		textsw_display.c textsw_event.c textsw_find.c textsw_file.c \
		textsw_selection.c textsw_seln_svc.c textsw_again.c \
		textsw_get_key.c textsw_put_key.c textsw_filter.c text.c\
		textsw_attr.c textsw_dbx.c textsw_tool.c textsw_scroll.c \
		textsw_field.c textsw_caret.c textsw_search.c \
		textsw_extrasmenu.c
CFILESLIB= ${ESCFILESLIB} ${EVCFILESLIB} ${MISCCFILESLIB} ${TEXTCFILESLIB}
SPCFILESLIB=	ei_plain_text.c es_file.c es_mem.c ev_once.c \
		textsw_again.c textsw_attr.c textsw_field.c \
		textsw_filter.c textsw_input.c textsw_once.c textsw_search.c \
		textsw_seln_svc.c ps_impl.c textsw_file.c textsw_menu.c \
		pw_impl.c textsw_event.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL= $(CFILESLIB: %=$(FULLPATH)/%)

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
