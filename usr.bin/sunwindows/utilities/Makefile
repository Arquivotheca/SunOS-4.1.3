#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= overview.c swin.c switcher.c toolplaces.c

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE= lockscreen.c lockscreen_default.c \
		adjacentscreens.c clear_functions.c \
		setkeys.c set_alarm.c get_alarm.c ring_alarm.c

BINS=			switcher \
			lockscreen \
			lockscreen_default \
			overview \
			swin \
			adjacentscreens \
			clear_functions \
			set_alarm \
			get_alarm \
			ring_alarm

LIBKVM_CMDS=		toolplaces

OLDBINS= 		setkeys

CMDS= ${BINS} ${OLDBINS} ${LIBKVM_CMDS}

HDRSPRIVATE=
HDRSPUBLIC=
HDRSALL= ${HDRSPRIVATE} ${HDRSPUBLIC}
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
TERMLIB=	-ltermcap

CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

GPROFFILES= swin.pg switcher.pg lockscreen.pg lockscreen_default.pg \
	adjacentscreens.pg clear_functions.pg overview.pg setkeys.pg \
	toolplaces.pg

OBJFILES=

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} ${CMDS}

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

include ../Makefile.master

${BINS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS}

${LIBKVM_CMDS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS} -lkvm

gprof:: ${GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

setkeys:  setkeys.c keytables.o
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} ${LDFLAGS} -o setkeys setkeys.c \
	keytables.o ${TERMLIB}

keytables.o: ../../../sys/sundev/keytables.c
	$(CC) -I${INCLUDE} -o $@  -c ../../../sys/sundev/keytables.c

FRC:
# non-existent so always younger

fast_install: install_bins

install: install_bins install_sources

install_bins:  
	-@for i in ${BINS} ; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s  ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in toolplaces; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 2755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		chgrp kmem ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in ${OLDBINS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}/usr/old/$$i ]; then \
			rm -f ${DESTDIR}/usr/old/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}/usr/old/$$i; \
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
