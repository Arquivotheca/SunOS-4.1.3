#
# @(#)Makefile 1.1 92/07/30 SMI
#


HDRSPRIVATE= iconedit.h
HDRSPUBLIC=

CFILESALL= iconedit_canvas.c iconedit_main.c iconedit_mpr.c iconedit_panel.c \
	iconedit_browse.c

OBJFILES= iconedit_canvas.o iconedit_mpr.o iconedit_panel.o \
	  iconedit_browse.o

NO_MAIN_OBJS= iconedit_canvas.o iconedit_browse.o iconedit_mpr.o \
		iconedit_panel.o
HDRSALL=	$(HDRSPRIVATE) $(HDRSPUBLIC)

.KEEP_STATE:

.c.o:
	${CC} ${CFLAGS_NOT_ROI_DATA} -c $<

include ../../Makefile.master

all: ${HDRSALL} ${OBJFILES} iconedit

objsall: ${OBJFILES}

gprof:: ${GPROFFILESALL}
	${AR} uvl ../../${GPROF_MERGE_LIB} ${GPROFFILESALL}

iconedit: iconedit.h ${NO_MAIN_OBJS} iconedit_main.c 
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o iconedit -DSTANDALONE \
	iconedit_main.c ${NO_MAIN_OBJS} ${LIBS} -lm

clean:
	rm -f *.o core iconedit

fast_install: install_bins
install: install_bins install_sources 

install_bins:
	echo "install iconedit"
	rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/iconedit
	install -m 755 iconedit ${DESTDIR}${PRODBINDIR}${LINKDIR}
	rm -f ${DESTDIR}${PRODBINDIR}iconedit
	ln -s ${LINKDIR}/iconedit ${DESTDIR}${PRODBINDIR}iconedit


install_sources: Makefile.customer
	install -d -m 755 ${DESTDIR}${SRCDIR}/iconedit
	install -m 644 Makefile.customer ${DESTDIR}${SRCDIR}/iconedit/Makefile
	install -m 644 ${CFILESALL} ${DESTDIR}${SRCDIR}/iconedit
	install -m 444 ${HDRSALL} ${DESTDIR}${SRCDIR}/iconedit

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

