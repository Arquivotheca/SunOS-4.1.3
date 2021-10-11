#
# @(#)Makefile 1.1 92/07/30 Copyr 1985 Sun Micro
#

HDRSPRIVATE=
HDRSPUBLIC=
CFILESALL= fv.bind.c fv.file.c fv.info.c fv.rcp.c fv.util.c fv.cmd.c fv.find.c fv.listbox.c fv.regex.c fvabi.c fv.comm.c fv.font.c fv.main.c fv.seln.c fv.editops.c fv.glob.c fv.props.c fv.tree.c
OBJFILES= fv.bind.o fv.file.o fv.info.o fv.rcp.o fv.util.o fv.cmd.o fv.find.o fv.listbox.o fv.regex.o fv.comm.o fv.font.o fv.main.o fv.seln.o fv.editops.o fv.glob.o fv.props.o fv.tree.o
OBJFILES=
NO_MAIN_OBJS= fv.bind.o fv.file.o fv.info.o fv.rcp.o fv.util.o fv.cmd.o fv.find.o fv.listbox.o fv.regex.o fv.comm.o fv.font.o fv.seln.o fv.editops.o fv.glob.o fv.props.o fv.tree.o fv.main.o

HDRSALL=$(HDRSPRIVATE) $(HDRSPUBLIC)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
USE_IMAGES=

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} fileview

.c.o:	
	${CC} -DSV1 ${CFLAGS_NOT_ROI_DATA} ${USE_IMAGES} -c $<

include ../../Makefile.master

objsall: ${OBJFILES}

gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

INC=    fv.colormap.h fv.extern.h fv.glob.h fv.h fv.port.h
ICON=   app_list.icon documentI.icon move.cursor\
        application.icon docwin.icon openfolder.icon\
        applicationI.icon equal.icon openfolderI.icon\
        broken_list.icon folder.icon pin_in.icon\
        brokenlink.icon folderI.icon pin_out.icon\
        busy.cursor folder_list.icon plus.icon\
        button_stack.icon folderunexpl.icon sys_list.icon\
        copy.cursor folderwin.icon system.icon\
        doc_list.icon fv.icon systemI.icon\
        document.icon lock.icon

fileview:       $(INC) $(ICON) ${NO_MAIN_OBJS}
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -o fileview ${NO_MAIN_OBJS} ${LIBS}


clean:	
	-rm -f core errs *.o *% *.BAK fileview *.pg

fast_install: install
install:
	-for i in fileview; do \
	echo "installing fileview"; \
	if [ -f ${DESTDIR}/usr/bin/sunview1/fileview ]; then \
		rm -f ${DESTDIR}/usr/bin/sunview1/fileview; \
	fi; \
	install -m 755 $$i ${DESTDIR}/usr/bin/sunview1/$$i; \
	done
 

install_h:	

tags:   ${CFILESALL}
	ctags -w ${CFILESALL}
