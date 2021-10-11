#
# @(#)Makefile 1.1 92/07/30 SMI
#
# CFILES_USER are C files distributed to the user
#
CFILES_USER= cmdtool.c 

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE=
CMDS= cmdtool
HDRSALL=
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
CFILESALL=  ${CFILES_USER} ${CFILES_NO_DISTRIBUTE}

MERGE_GPROFFILES= cmdtool.pg

.KEEP_STATE:

all: ${CMDS}

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

${CMDS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE -O $@.c -o $@ ${LIBS} 

gprof:: cmdtool.c
	${CC} ${LDFLAGS} -pg ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE cmdtool.c -o cmdtool ${GPROF_LIBS}
	
lint:: ${CFILESALL}
		${MAKE} ${MFLAGS} ${COMMONFLAGS} lint \

fast_install: install_bins 

install: install_bins install_sources

install_bins:  
	-@for i in shelltool; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		ln -s cmdtool ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/cmdtool ${DESTDIR}${PRODBINDIR}$$i; \
	done
	-@for i in ttytool; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}/usr/old/$$i ]; then \
			rm ${DESTDIR}/usr/old/$$i; \
		fi; \
		ln -s ../bin/sunview1/cmdtool ${DESTDIR}/usr/old/$$i; \
	done
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
