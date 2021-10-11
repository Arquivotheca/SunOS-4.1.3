#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
CFILES_USER= clock.c perfmeter.c meter.c clockhands.c \
	clockhands.rom.c perfmeter_hands.c

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE= perfmon.c 

UNIQUE_INSTALL=		perfmon 

UNIQUE_CMDS=		perfmeter \
			clock

CMDS= ${UNIQUE_INSTALL} ${UNIQUE_CMDS}

HDRSALL= meter.h clockhands.h
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= clock.pg perfmeter.pg meter.pg clockhands.pg \
	clockhands.rom.pg perfmeter_hands.pg

OBJFILES=		clockhands.o clockhands.rom.o \
			meter.o

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} ${CMDS}

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

objsall: ${OBJFILES}

${UNIQUE_INSTALL}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c -o $@ ${LIBS}

gprof:: ${MERGE_GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

clock:	 clockhands.h clock.c clockhands.o clockhands.rom.o
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE clock.c \
	clockhands.o clockhands.rom.o  -o clock ${LIBS}

perfmeter: meter.h perfmeter.c meter.o perfmeter_hands.o
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE perfmeter.c \
	meter.o perfmeter_hands.o -o perfmeter ${LIBS} -lrpcsvc

fast_install: install_bins 

install: install_bins install_sources

install_bins:  
	-@for i in ${UNIQUE_CMDS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in perfmon; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}/usr/old/$$i ]; then \
			rm ${DESTDIR}/usr/old/$$i; \
		fi; \
		install -m 2755 $$i ${DESTDIR}/usr/old/$$i; \
		chgrp kmem ${DESTDIR}/usr/old/$$i; \
	done
	-@for i in clocktool; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}/usr/old/$$i ]; then \
			rm ${DESTDIR}/usr/old/$$i; \
		fi; \
		ln -s ../bin/sunview1/clock ${DESTDIR}/usr/old/$$i; \
	done

install_sources: ${CFILES_USER} ${HDRSALL}
	install -m 644 ${CFILES_USER} ${DESTDIR}${SRCDIR}
	install -m 444 ${HDRSALL} ${DESTDIR}${SRCDIR}

clean:
	rm -f ${CMDS} *.o *% core

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)
