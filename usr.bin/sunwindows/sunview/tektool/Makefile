#
# @(#)Makefile 1.1 92/07/30 SMI
#

CFILES_TEKTOOL=tektool.c
CFILES_TEKSW=tek.c teksw.c teksw_ui.c teksw_data.c
INSDIR=usr/bin/sunview1
LINK=sunview1

# CFILES_USER are C files distributed to the user
CFILES_USER=

# CFILES_NO_DISTRIBUTE are non-distributed C files
CFILES_NO_DISTRIBUTE=$(CFILES_TEKTOOL) $(CFILES_TEKSW)

HDRSPRIVATE=tek.h teksw.h teksw_imp.h
HDRSPUBLIC=

CFILESALL=$(CFILES_USER) $(CFILES_NO_DISTRIBUTE) 
HDRSALL=$(HDRSPRIVATE) $(HDRSPUBLIC)
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
OBJFILES_TEKSW=$(CFILES_TEKSW:%.c=%.o)

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} tektool

include ../../Makefile.master

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

tektool: teksw.h $(OBJFILES_TEKSW) $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -DSTANDALONE $@.c ${OBJFILES_TEKSW} -o $@ ${LIBS}


gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof 

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

FRC:
# non-existent so always younger

fast_install: install_bins 

install: install_bins install_sources

install_bins: 
	-@for i in tektool; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}/${INSDIR}/$$i ]; then \
			rm ${DESTDIR}/${INSDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}/${INSDIR}/$$i; \
		ln -s ${LINK}/$$i ${DESTDIR}/usr/bin/$$i; \
	done

install_sources:
#	-for i in ${CFILES_USER} ${HDRSPUBLIC} ; do \
#		install -m 644 $$i ${DESTDIR}${SRCDIR}; \
#	done

clean:
	rm -f *.o core *.pg tektool tags

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)
