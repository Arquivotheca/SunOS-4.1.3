#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX
#

CC= /bin/cc
MAKE= make CC=$(CC)

ARCH=	`/bin/arch`

all:
	cd lib; $(MAKE)
	if [ $(ARCH) = "sun3" ]; then cd fparel; $(MAKE); fi
	if [ $(ARCH) = "sun4" ]; then cd fpurel; $(MAKE); fi
	cd sundiag; $(MAKE)

install:
	/bin/csh -c "if (-e $(DESTDIR)/usr/diag == 0) mkdir $(DESTDIR)/usr/diag"
	chown bin $(DESTDIR)/usr/diag
	chmod 755 $(DESTDIR)/usr/diag
	cd lib; $(MAKE) install DESTDIR=$(DESTDIR)
	if [ $(ARCH) = "sun3" ]; then \
		cd fparel; $(MAKE) install DESTDIR=$(DESTDIR); \
	fi
	if [ $(ARCH) = "sun4" ]; then \
		cd fpurel; $(MAKE) install DESTDIR=$(DESTDIR); \
	fi
	cd sundiag; $(MAKE) install DESTDIR=$(DESTDIR)

clean:
	cd lib; $(MAKE) clean
	if [ $(ARCH) = "sun3" ]; then cd fparel; $(MAKE) clean; fi
	if [ $(ARCH) = "sun4" ]; then cd fpurel; $(MAKE) clean; fi
	cd sundiag; $(MAKE) clean
