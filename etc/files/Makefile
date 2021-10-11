#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Files which live in the current directory and are copied to the destination.
#
FILES=	fbtab filetype format.dat fstab gettytab group hosts hosts.equiv \
	netmasks networks \
	passwd protocols publickey rc rc.boot rc.ip rc.local rpc inetd.conf \
	services svdtab syslog.conf ttytab

#
# Other files which happen to live in the current directory,
# yet they are installed in the destination as executable.
#
OTHERS=	rc.single

#
# Files which should exist but be empty in the destination
#
EMPTY=	motd mtab utmp xtab

#
# files which go into the secret unstub directory /etc/.install.d
SECRETS= re-preinstall-svr4 README.re-pre-svr4

all: $(FILES) $(OTHERS) $(SECRETS)

$(EMPTY):
	touch $@

install: $(FILES) $(OTHERS) $(EMPTY) $(SECRETS)
	install -d $(DESTDIR)/etc
	install -m 644 $(FILES) $(EMPTY) $(DESTDIR)/etc
#	window system kludge, sigh...
	chmod 666 $(DESTDIR)/etc/utmp
	install -m 755 $(OTHERS) $(DESTDIR)/etc
	install -d -g daemon -o root -m 755 $(DESTDIR)/etc/.install.d
	install -m 500 $(SECRETS) $(DESTDIR)/etc/.install.d


clean:
	$(RM) $(EMPTY)
