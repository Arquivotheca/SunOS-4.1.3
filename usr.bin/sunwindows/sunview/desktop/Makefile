#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= suntools.c suntools_menu.c selection_svc.c
#
# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE= get_selection.c sv_acquire.c sv_release.c
HDRSPRIVATE=
HDRSPUBLIC=

BINS=			selection_svc \
			get_selection

SETUID_BINS=		sv_acquire \
			sv_release

UNIQUE_BINS=		sunview

CMDS= ${BINS} ${UNIQUE_BINS}

HDRSALL= ${HDRSPRIVATE} ${HDRSPUBLIC}
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile

CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= suntools.pg suntools_menu.pg selection_svc.pg \
		get_selection.pg sv_acquire.pg sv_release.pg

OBJFILES=	 suntools_menu.o

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} ${CMDS} ${SETUID_BINS}

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

include ../../Makefile.master

${BINS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS} 

${SETUID_BINS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} $@.c -o $@

sunview: suntools.c suntools_menu.o
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE suntools.c \
	suntools_menu.o -o sunview ${LIBS}

gprof:: ${MERGE_GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint::  ${CFILESALL}
		${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

FRC:
# non-existent so always younger

fast_install: install_bins

install: install_bins install_sources

install_bins:  
	-@for i in ${BASETOOLS_BIN}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		ln -s sunview ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/sunview ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in ${CMDS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in ${SETUID_BINS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -o root -m 4755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
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
