#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= gfxtool.c 
#
# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE=
CMDS= gfxtool
HDRSALL=
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= gfxtool.pg

.KEEP_STATE:

all: ${CMDS}

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

${CMDS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS} 

gprof:: ${MERGE_GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

fast_install: install_bins 

install: install_bins  install_sources

install_bins:  
	-@for i in ${CMDS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
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
