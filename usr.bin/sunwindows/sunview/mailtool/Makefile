#
# @(#)Makefile 1.1 92/07/30 Copyr 1985 Sun Micro
#

HDRSPRIVATE=		glob.h tool.h
HDRSPUBLIC=
CFILESALL=  main.c tool.c tool_support.c create_panels.c selection.c cmds.c panel_events.c generate_menus.c folder_menu.c reply.c mail.c subr.c vars.c
OBJFILES=tool.o tool_support.o create_panels.o selection.o cmds.o panel_events.o generate_menus.o folder_menu.o reply.o mail.o subr.o vars.o
NO_MAIN_OBJS= tool.o tool_support.o create_panels.o selection.o cmds.o panel_events.o generate_menus.o folder_menu.o reply.o mail.o subr.o vars.o

HDRSALL=$(HDRSPRIVATE) $(HDRSPUBLIC)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
USE_IMAGES=

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} mailtool

.c.o:	
	${CC} ${CFLAGS_NOT_ROI_DATA} ${USE_IMAGES} -c $<

include ../../Makefile.master

objsall: ${OBJFILES}

gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

mailtool:       glob.h tool.h main.c ${NO_MAIN_OBJS}
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -o mailtool main.c ${NO_MAIN_OBJS} ${LIBS}

mail.o: mail.c
	${CC} ${CFLAGS_NOT_ROI_DATA} -c mail.c

clean:	
	-rm -f core errs *.o *% *.BAK mailtool *.pg

fast_install: install
install: fmt_mail
	-for i in mailtool fmt_mail; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done

install_h:	

tags:   ${CFILESALL}
	ctags -w ${CFILESALL}
