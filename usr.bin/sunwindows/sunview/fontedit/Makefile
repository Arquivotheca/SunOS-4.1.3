#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Makefile for fontedit - a vfont font editor.

CFLAGS = -O
OFILES = fted_main.o button_build.o button_char.o button_scroll.o\
         edit_display.o  edit_pad.o init.o input.o fted_draw.o \
	 line.o echo.o write_font.o util.o \
	 slider.o button.o undo.o patches.o

#every .o except for the one defining main/fontedit_main	 
NO_MAIN_OBJS= button_build.o button_char.o button_scroll.o\
         edit_display.o  edit_pad.o init.o input.o fted_draw.o \
	 line.o echo.o write_font.o util.o \
	 slider.o button.o undo.o patches.o

CFILESALL = fted_main.c button_build.c button_char.c button_scroll.c\
         edit_display.c edit_pad.c init.c input.c fted_draw.c patches.c \
	 line.c echo.c write_font.c util.c slider.c button.c undo.c
HDRSPRIVATE = action_table.h button.h edit.h externs.h fontedit.h other_hs.h patches.h slider.h
HDRSPUBLIC=
HDRSALL= $(HDRSPRIVATE) $(HDRSPUBLIC)
SRC    = $(HDRSALL) $(CFILESALL)

EXTERN_OFILES = 

CFLAGS_NOT_ROI_DATA= $(CFLAGS) $(DBX) $(CFLAGS_LOCAL) $(CPUFLAG)

.KEEP_STATE:

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

all: ${HDRSALL} ${OFILES} fontedit

include ../../Makefile.master

objsall: ${OFILES}

gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

fontedit: ${HDRSALL} $(NO_MAIN_OBJS) fted_main.c
	${CC} -DSTANDALONE ${LDFLAGS} $(CFLAGS) -o fontedit $(NO_MAIN_OBJS) \
	$(CPUFLAG) fted_main.c $(EXTERN_OFILES) $(LIBS)

clean:
	rm -f *.o *.pg core fontedit errs

fast_install: install
install:
	-for i in fontedit; do \
		echo "installing fontedit"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/fontedit ]; then \
			rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/fontedit; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);
