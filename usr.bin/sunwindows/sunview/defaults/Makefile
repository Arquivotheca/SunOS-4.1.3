#
# @(#)Makefile 1.1 92/07/30 SMI
#
DEFAULTS_CMDS= defaults_to_indentpro indentpro_to_defaults scrolldefaults \
		defaultsedit stty_from_defaults \
		mailrc_to_defaults defaults_to_mailrc defaults_from_input \
		input_from_defaults
HDRS=
HDRSPRIVATE=	m2d_def.h setkey_tables.h
HDRSPUBLIC=
TABLE_SRCS=	klunker_tables.c no_sunview_tables.c sun1_tables.c \
		sun2+_tables.c
CFILES= 	defaultsedit.c stty_from_defaults.c  \
		defaults_from_input.c input_from_defaults.c \
		scrolldefaults.c m2d_main.c m2d_defaults.c m2d_vars.c \
		d2m_main.c m2d_execute.c \
		defaults_to_indentpro.c indentpro_to_defaults.c
TABLE_OBJS=	klunker_tables.o no_sunview_tables.o sun1_tables.o \
		sun2+_tables.o
OBJFILES=	$(TABLE_OBJS) \
		m2d_defaults.o m2d_vars.o \
		m2d_execute.o
CFILESALL=	${CFILES} ${TABLE_SRCS}
HDRSALL= ${HDRSPRIVATE} ${HDRSPUBLIC}
SRCSALL= $(CFILESALL) $(HDRSALL) Makefile

DEFAULTSFILES=	*.d

.KEEP_STATE:

.c.o:
	${CC} ${CFLAGS_NOT_ROI_DATA} -c $<

include ../../Makefile.master

objsall: ${OBJFILES}

all: ${HDRSALL} ${OBJFILES} ${DEFAULTS_CMDS}

gprof:: ${GPROFFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

defaultsedit:       defaultsedit.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o defaultsedit -DSTANDALONE \
	defaultsedit.c ${LIBS}

stty_from_defaults: stty_from_defaults.c    
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o stty_from_defaults \
	-DSTANDALONE stty_from_defaults.c ${LIBS}  

scrolldefaults: scrolldefaults.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o scrolldefaults -DSTANDALONE \
	scrolldefaults.c ${LIBS}

mailrc_to_defaults: m2d_def.h m2d_main.c d2m_main.o m2d_execute.o m2d_vars.o m2d_defaults.o
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA}  -o mailrc_to_defaults \
	-DSTANDALONE m2d_main.c d2m_main.o m2d_execute.o m2d_vars.o \
	m2d_defaults.o ${LIBS}

defaults_to_mailrc: d2m_main.c m2d_execute.o m2d_vars.o m2d_defaults.o
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} d2m_main.c m2d_execute.o \
	m2d_vars.o m2d_defaults.o -o defaults_to_mailrc -DSTANDALONE $(LIBS)

defaults_from_input: defaults_from_input.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o defaults_from_input \
	-DSTANDALONE defaults_from_input.c ${LIBS}

input_from_defaults: setkey_tables.h input_from_defaults.c ${TABLE_OBJS}
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o input_from_defaults \
	-DSTANDALONE input_from_defaults.c ${TABLE_OBJS} ${LIBS}

defaults_to_indentpro: defaults_to_indentpro.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o defaults_to_indentpro \
	-DSTANDALONE defaults_to_indentpro.c ${LIBS}

indentpro_to_defaults: indentpro_to_defaults.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o indentpro_to_defaults \
	-DSTANDALONE indentpro_to_defaults.c ${LIBS}

clean:	
	-rm -f core errs *.o *.pg *% *.BAK ${DEFAULTS_CMDS}

fast_install: install

install: install_d
	   -@for i in ${DEFAULTS_CMDS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
		   rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	   done

install_d: lib/${DEFAULTSFILES}
	install -d -m 755 ${DESTDIR}/usr/lib/defaults
	cd lib; \
	for i in ${DEFAULTSFILES} ; do \
		install -m 644 $$i ${DESTDIR}/usr/lib/defaults; \
	done 

tags:
	ctags -w ${CFILESALL}
