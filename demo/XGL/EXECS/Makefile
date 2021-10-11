#
#@(#)Makefile 1.1 92/07/30 SMI
#

#
#  Makefile for the EXECS sub-directory
#

MAKEFILE = Makefile
README = README
EXECDIR = ${DESTDIR}/usr/demo/XGL/EXECS
SUBDIR = 
XGL_EXEC = 

OWNER = bin
INSTALLX = install -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLD = $(INSTALLX) -d -m 755
INSTALLXGL = $(INSTALLX) -m 755

target:

all: FRC

clean: FRC
	rm -f errs core 

install: $(MAKEFILE) $(README) FRC
	$(INSTALLD) $(EXECDIR)
	$(INSTALL) -m 644 $(MAKEFILE)  $(EXECDIR)
	$(INSTALL) -m 644 $(README)  $(EXECDIR)
#	$(INSTALLXGL) $(XGL_EXEC) $(EXECDIR)
#	for i in $(SUBDIR); do \
#		$(INSTALLD) $(EXECDIR)/$$i; \
#		(cd $$i && $(MAKE) $(MFLAGS) $@ $(PASSFLAGS)); \
#	done

FRC:
