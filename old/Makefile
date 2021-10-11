#
# @(#)Makefile 1.1 92/08/05 SMI
#
DESTDIR=
CFLAGS=	-O
LDFLAGS=
MAKE=/usr/bin/make

.KEEP_STATE:

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	compact eyacc make filemerge

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	sun3cvt pti prmail syslog

all:	${SUBDIR} ${STD}

${SUBDIR}: FRC
	cd $@ && $(MAKE) $(MFLAGS)

${STD}: $$@.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c

install: 
	install -d -o bin -m 755 ${DESTDIR}/usr/old
	for i in ${SUBDIR}; \
		do (cd $$i && $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} $@); done
	install -s -o bin ${STD} ${DESTDIR}/usr/old

clean: FRC
	$(RM) a.out core *.s *.o $(STD)
	for i in ${SUBDIR}; do (cd $$i && $(MAKE) $(MFLAGS) $@); done

FRC:
