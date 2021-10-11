#
# @(#)Makefile 1.1 92/07/30 SMI
#

HDIR = nse nse_impl
SUBDIR = libtfs bin tfsd
LINTFLAGS = -buxz

all:	${HDIR} ${SUBDIR}

${HDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS}

lint:
	@for i in ${SUBDIR}; do \
		echo "(cd $$i; ${MAKE} ${MFLAGS} lint);"; \
		(cd $$i; ${MAKE} ${MFLAGS} LINTFLAGS=${LINTFLAGS} lint); \
	done

clean:
	@for i in ${SUBDIR}; do \
		echo "(cd $$i; ${MAKE} ${MFLAGS} clean);"; \
		(cd $$i; ${MAKE} ${MFLAGS} clean); \
	done

install:
	@for i in ${SUBDIR}; do \
		echo "(cd $$i; ${MAKE} ${MFLAGS} install);"; \
		(cd $$i; ${MAKE} ${MFLAGS} install); \
	done

FRC: ;
