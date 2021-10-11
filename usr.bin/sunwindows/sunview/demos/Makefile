#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= canvas_demo.c cursor_demo.c framedemo.c \
	jumpdemo.c spheresdemo.c bouncedemo.c
	
# Moved to the attic: selections.c mousetool.c 

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE=

CMDS=			cursor_demo \
			framedemo \
			spheresdemo \
			canvas_demo \
			jumpdemo \
			bouncedemo

HDRSALL=
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= canvas_demo.pg cursor_demo.pg \
	jumpdemo.pg spheresdemo.pg bouncedemo.pg framedemo.pg

.KEEP_STATE:

all: ${CMDS} ${HDRSALL}

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

${CMDS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS} 

gprof:: ${MERGE_GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: 
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

fast_install: install_demos 

install: install_demos install_sources

install_demos:
	-for i in ${CMDS}; do \
		rm ${DESTDIR}/usr/demo/sunview1/$$i; \
		install -m 755 $$i ${DESTDIR}/usr/demo/sunview1/$$i; \
	done

install_sources: ${CFILES_USER} ${HDRSALL}
	-for i in ${CFILES_USER} ${HDRSALL} ; do \
		install -m 644 $$i ${DESTDIR}${SRCDIR}; \
	done

clean:
	rm -f ${CMDS} *.o *% core

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)
