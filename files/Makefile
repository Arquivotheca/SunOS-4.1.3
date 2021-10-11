#	Makefile	1.1	92/07/30
#

# Files which live in the current directory and are copied to the destination.
#
FILES=	cshrc login profile

# Files which should exist but be empty in the destination
#
EMPTY=	rhosts

ALL=	${FILES} MAKEDEV MAKEHOSTS

all:	${ALL}

${ALL}:
	sccs get $@

install: ${ALL}
	for i in ${FILES}; do (install -c -m 644 $$i ${DESTDIR}/.$$i); done
	for i in ${EMPTY}; do \
		(touch ${DESTDIR}/.$$i; chmod 644 ${DESTDIR}/.$$i); done
	install -c -m 755 MAKEDEV ${DESTDIR}/dev
	install -c -m 755 MAKEHOSTS ${DESTDIR}/usr/hosts

clean:

FRC:
