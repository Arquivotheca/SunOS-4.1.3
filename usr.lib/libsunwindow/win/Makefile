#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		win
#HDRSPUBLIC=	win_environ.h win_input.h win_ioctl.h win_notify.h \
#		win_screen.h win_struct.h win_lock.h \
#		win_cursor.h win_enum.h win_keymap.h
#HDRSPRIVATE=	win_impl.h
MISC = 		proto.k
CFILESLIB=	win_cursor.c win_environ.c win_ttyenv.c \
		win_geometry.c win_global.c win_getscr.c win_blanket.c \
		win_enum.c win_screenadj.c win_central.c \
		win_input.c win_enum_ind.c win_screen.c win_treeop.c \
		win_cms.c win_axe_ind.c win_misc.c win_is_ind.c \
		win_set_ind.c win_bell.c win_get_alarm.c win_alarm.c \
		win_keymap.c win_misc_data.c
NO_XSTR_FILES= win_misc_data.c
		
#	win_error is initialized to a default and then changed in
#	win_errorhandler in win_misc_data.c.
SPCFILESLIB=	win_alarm.c win_keymap.c win_misc_data.c win_screen.c  \
		win_input.c win_ttyenv.c win_treeop.c
$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)

TAGFILESALL=	${FULLPATH}/win_cursor.c ${FULLPATH}/win_environ.c \
		${FULLPATH}/win_enum.c ${FULLPATH}/win_ttyenv.c \
		${FULLPATH}/win_geometry.c ${FULLPATH}/win_global.c \
		${FULLPATH}/win_getscr.c ${FULLPATH}/win_blanket.c \
		${FULLPATH}/win_screenadj.c ${FULLPATH}/win_central.c \
		${FULLPATH}/win_input.c ${FULLPATH}/win_enum_ind.c \
		${FULLPATH}/win_screen.c ${FULLPATH}/win_treeop.c \
		${FULLPATH}/win_is_ind.c ${FULLPATH}/win_cms.c\
		${FULLPATH}/win_axe_ind.c ${FULLPATH}/win_misc.c \
		${FULLPATH}/win_set_ind.c ${FULLPATH}/win_bell.c \
		${FULLPATH}/win_get_alarm.c ${FULLPATH}/win_alarm.c \
		${FULLPATH}/win_keymap.c \
		${FULLPATH}/win_environ.h ${FULLPATH}/win_input.h \
		${FULLPATH}/win_ioctl.h ${FULLPATH}/win_notify.h \
		${FULLPATH}/win_screen.h ${FULLPATH}/win_struct.h \
		${FULLPATH}/win_cursor.h ${FULLPATH}/win_lock.h \
		${FULLPATH}/win_enum.h ${FULLPATH}/win_impl.h \
		${FULLPATH}/win_keymap.h ${FULLPATH}/win_misc_data.c

all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master
xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $(MISC) $$(OBJS)

	
clean: master.clean

win_keymap.o:  win_keymap.c
	$(CC)  $(CFLAGS_NOT_ROI_DATA) $(ROI_DATA) $(CFLAGS$(TARGET_MACH)) -c $<
