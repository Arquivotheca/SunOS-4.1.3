#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		tool
#HDRSPUBLIC=	tool.h tool_hs.h tool_struct.h 
#HDRSPRIVATE=	tool_impl.h tool_commons.h
CFILESLIB= 	tool_commons.c tool_select.c tool_layout.c tool_pos.c \
		tool_display.c  tool_kbd.c tool_input.c \
		tool_create.c tool_destroy.c tool_install.c tool_begin.c\
		tla_find.c tla_get.c tla_parse.c tla_storage.c \
		tla_freelist.c tla_make.c tla_set.c tla_usage.c tool_bdry.c
SPCFILESLIB=	tla_parse.c tool_commons.c tla_make.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	$(CFILESLIB:%.c=$(FULLPATH)/%.c) \
		${FULLPATH}/tool_struct.h ${FULLPATH}/tool_impl.h
NO_XSTR_FILES= tool_commons.c

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
