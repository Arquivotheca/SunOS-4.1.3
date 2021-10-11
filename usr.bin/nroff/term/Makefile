#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.6 86/05/15
#
# Possible options:
#	make		compile source for terminal tables below
#	make <term>	compile table for a specific <term>
#	make install	move tables to ${DEST}
#	make links	make links for ease of documentation
#	make clean	remove tab*.o files lying around

ALL=	37 lpr 300 300-12 302 302-12 382 382-12 450 450-12 833 833-12 \
	epson itoh itoh12 nec nec12 nec-t nec25-t qume qume12 xerox xerox12 \
	x-ecs x-ecs12 man

ALLTAB=	$(ALL:%=tab%)
ALLDST=	$(ALLTAB:%=$(DEST)/%)

DESTDIR=
DEST=	${DESTDIR}/usr/lib/term

all:	$(ALLTAB)

$(ALLTAB): $$(@).c
	$(CC) -c $(CFLAGS) $@.c
	$(LD) -N -Tdata 0 $@.o -o $@
	@$(RM) $@.o

install: ins_dir ins_tabs ins_links

ins_dir: README
	install -d -o bin -m 755 $(DEST)
	install -c -m 644 README $(DEST)/README

ins_tabs: $(ALLTAB)
	install -s -m 644 $(ALLTAB) $(DEST)

ins_links: FRC
	$(RM) ${DEST}/tabtn300; ln ${DEST}/tablpr ${DEST}/tabtn300
	$(RM) ${DEST}/tabcrt; ln ${DEST}/tablpr ${DEST}/tabcrt
	$(RM) ${DEST}/tab300s; ln ${DEST}/tab302 ${DEST}/tab300s
	$(RM) ${DEST}/tab300s-12; ln ${DEST}/tab302-12 ${DEST}/tab300s-12
	$(RM) ${DEST}/tabdtc; ln ${DEST}/tab302 ${DEST}/tabdtc
	$(RM) ${DEST}/tabdtc12; ln ${DEST}/tab302-12 ${DEST}/tabdtc12
	$(RM) ${DEST}/tabipsi; ln ${DEST}/tab450 ${DEST}/tabipsi
	$(RM) ${DEST}/tabipsi12; ln ${DEST}/tab450-12 ${DEST}/tabipsi12

clean: FRC
	$(RM) tab* *.o a.out errs

FRC:

tab300-12:	code.300
tab300:		code.300
tab302-12:	code.300
tab302:		code.300
tab382-12:	code.300
tab382:		code.300
tab450-12:	code.300
tab450:		code.300
tab833-12:	code.aj833
tab833:		code.aj833
tabitoh:	code.itoh
tabitoh12:	code.itoh
tabnec:		code.nec
tabnec12:	code.nec
tabx-ecs:	code.x-ecs
tabx-ecs12:	code.x-ecs
tabxerox:	code.xerox
tabxerox12:	code.xerox
