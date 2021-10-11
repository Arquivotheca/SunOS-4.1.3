#
# @(#)Makefile 1.1 92/07/30 SMI
#

LIBSPLUS=${LIBS} -lrpcsvc

HDRSPRIVATE=	traffic.h
HDRSPUBLIC=
HDRSALL=	${HDRSPRIVATE} ${HDRSPUBLIC}	
CFILESALL= 	traffic.c draw.c panel.c rpc.c
OBJFILES=	draw.o panel.o rpc.o
NO_MAIN_OBJS=	draw.o panel.o rpc.o

SRCSALL=$(CFILESALL) $(HDRSALL)

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} traffic

.c.o:
	${CC} $(CFLAGS_NOT_ROI_DATA) -c $<

include ../../Makefile.master

objsall: ${OBJFILES}

gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

traffic:     traffic.h  ${NO_MAIN_OBJS} traffic.c
	${CC} -DSTANDALONE ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o traffic \
	traffic.c ${NO_MAIN_OBJS} ${LIBSPLUS}

clean:	
	-rm -f core errs *.o *.pg *% *.BAK traffic

install:	
	-for i in traffic; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
			rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	done

install_h:	NONEXISTENT_SO_ALWAYS_YOUNGER
	-for i in ${HFILES}; do \
		cmp -s $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i || \
		install $$i ${DESTDIR}${PRODINCLUDEDIR}/$$i; \
	done

tags:   ${CFILESALL}
	ctags -w ${CFILESALL}

NONEXISTENT_SO_ALWAYS_YOUNGER:
