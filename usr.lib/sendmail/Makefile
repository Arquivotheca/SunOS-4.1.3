#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 3.11 07/09/83
#
#
#	Makefile for sendmail base directory
#

ALL=	src/sendmail cf/main.cf cf/subsidiary.cf lib/sendmail.hf lib/aliases
DIRS=	$(DESTDIR)/var/spool/mqueue $(DESTDIR)/var/spool/mail
SENDMAIL=$(DESTDIR)/usr/lib/sendmail

M4FILES= md/config.m4

all: $(MAKEFILES)
	cd lib; $(MAKE) ${MFLAGS} all
	cd src; $(MAKE) ${MFLAGS} all
	cd aux; $(MAKE) ${MFLAGS} all
	cd cf;  $(MAKE) ${MFLAGS} all

install: all $(ALL) $(DIRS)
	cd src; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install
	install -c -m 444	lib/sendmail.hf	$(SENDMAIL).hf
#	install -c -m 660	/dev/null	$(SENDMAIL).fc
	install -c -m 444	cf/subsidiary.cf $(SENDMAIL).subsidiary.cf
	install -c -m 444	cf/main.cf	$(SENDMAIL).main.cf
	install -c -m 644	cf/subsidiary.cf $(DESTDIR)/etc/sendmail.cf
	install -c -m 644	lib/aliases	$(DESTDIR)/etc/aliases
	install -c -s -m 755	aux/vacation	$(DESTDIR)/usr/ucb/vacation
	install -c -s -m 755	aux/mconnect	$(DESTDIR)/usr/etc/mconnect
	install -c -s -m 755	aux/mailstats	$(DESTDIR)/usr/etc/mailstats
	rm -f					$(DESTDIR)/usr/ucb/newaliases
	ln -s		../lib/sendmail		$(DESTDIR)/usr/ucb/newaliases
	rm -f					$(DESTDIR)/usr/lib/aliases
	ln -s		../../etc/aliases	$(DESTDIR)/usr/lib/aliases
	$(DESTDIR)/usr/ucb/newaliases -oA$(DESTDIR)/etc/aliases

$(DESTDIR)/var/spool/mqueue:
	install -d -m 750 $(DESTDIR)/var/spool/mqueue

$(DESTDIR)/var/spool/mail:
	install -d -m 1777 $(DESTDIR)/var/spool/mail

clean: $(MAKEFILES)
	cd lib; $(MAKE) ${MFLAGS} clean
	cd src; $(MAKE) ${MFLAGS} clean
	cd cf;  $(MAKE) ${MFLAGS} clean
	cd aux; $(MAKE) ${MFLAGS} clean
