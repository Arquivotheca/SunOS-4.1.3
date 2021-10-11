#
#	Makefile for rfs installation utilities
#
#       @(#)  Makefile 1.1 92/07/30
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#
DESTDIR=

# Shell files which live in the current directory and are installed in the
# destination.
#
MISCFILES=toc Copyright

SHFILES=install_rfs verify_tapevol_arch extracting maketape

FILES=install_rfs verify_tapevol_arch extracting

.DEFAULT:
	sccs get $@

all:	${SHFILES} ${MISCFILES}

${SHFILES}:
	cp $@.sh $@
	chmod 755 $@

install: ${FILES}
	rm -rf ${DESTDIR}/usr/rfs/install
	mkdir ${DESTDIR}/usr/rfs/install
	for i in ${FILES}; do \
		(install -c -m 755 $$i ${DESTDIR}/usr/rfs/install/$$i); done

clean: 
	for i in ${SHFILES}; do \
		(rm -f $$i); done

FRC:
