#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		ttysw
#HDRSPUBLIC=	ttysw.h ttytlsw.h tty.h
#HDRSPRIVATE=	charimage.h charscreen.h ttyansi.h ttysw_impl.h ttytlsw_impl.h

CFILESLIB=	cim_change.c cim_size.c csr_change.c csr_init.c         \
		csr_sig.c ttyansi.c ttysw_gtty.c ttysw_init.c \
		ttysw_notify.c ttysw_stty.c ttysw_tio.c ttysw_tsw.c     \
		ttytlsw.c ttytlsw_notify.c ttytlsw_tio.c ttyselect.c 	\
		ttysw_main.c ttysw_menu.c ttysw_mapkey.c tty.c ttysw_modes.c \
		ttysw_entity_stream.c ttysw_data.c cmdsw_notify.c
SPCFILESLIB=	ttysw_menu.c ttyselect.c ttysw_init.c ttysw_main.c \
		ttysw_mapkey.c ttyansi.c csr_change.c ttysw_data.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/cim_change.c ${FULLPATH}/cim_size.c \
		${FULLPATH}/csr_change.c ${FULLPATH}/csr_init.c         \
		${FULLPATH}/csr_sig.c ${FULLPATH}/ttyansi.c \
		${FULLPATH}/ttysw_gtty.c ${FULLPATH}/cmdsw_notify.c \
		${FULLPATH}/ttysw_notify.c ${FULLPATH}/ttysw_init.c \
		${FULLPATH}/ttysw_stty.c ${FULLPATH}/ttysw_tio.c \
		${FULLPATH}/ttysw_entity_stream.c ${FULLPATH}/ttysw_tsw.c \
		${FULLPATH}/ttytlsw.c ${FULLPATH}/ttytlsw_notify.c \
		${FULLPATH}/ttytlsw_tio.c ${FULLPATH}/ttyselect.c \
		${FULLPATH}/ttysw_main.c ${FULLPATH}/ttysw_menu.c \
		${FULLPATH}/ttysw_mapkey.c ${FULLPATH}/tty.c \
		${FULLPATH}/ttysw.h ${FULLPATH}/ttysw_modes.c \
		${FULLPATH}/ttytlsw.h ${FULLPATH}/tty.h \
		${FULLPATH}/charimage.h ${FULLPATH}/charscreen.h \
		${FULLPATH}/ttyansi.h ${FULLPATH}/ttysw_impl.h \
		${FULLPATH}/ttytlsw_impl.h ${FULLPATH}/ttysw_data.c
NO_XSTR_FILES= ttysw_data.c

all: xall

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

clean: master.clean

install::
	cmp -s ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttysw_impl.h || \
	${INSTALL} ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttysw_impl.h; \
	cmp -s ttytlsw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttytlsw_impl.h || \
	${INSTALL} ttysw_impl.h \
	${DESTDIR}${PRODINCLUDEDIR}/.UNSUPPORTED.ttytlsw_impl.h;


