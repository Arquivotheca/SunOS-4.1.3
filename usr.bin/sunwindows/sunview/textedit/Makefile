#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= textedit.c 
#
CFILES_NO_DISTRIBUTE=

CMDS=		textedit
HDRSPRIVATE=
HDRSPUBLIC=

HDRSALL= ${HDRSPRIVATE} ${HDRSPUBLIC}
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= textedit.pg 

OBJFILES=

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} ${CMDS}

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

${CMDS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS} 

gprof:: textedit.c
	${CC} ${LDFLAGS} -pg ${CFLAGS_NOT_ROI_DATA} -o textedit -DSTANDALONE textedit.c ${GPROF_LIBS}

lint::  ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

fast_install: install_bins

install: install_bins install_sources

install_bins:
	-@for i in ${CMDS} ; do \
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
	rm -f ${CMDS} *.o *%

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)
