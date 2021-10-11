#
# @(#)Makefile.src.release 1.3 86/09/13 SMI
#

#
#  Makefile for the EXECS sub-directory
#

MAKEFILE = Makefile
EXECDIR = usr/demo/SUNVIEW/EXECS


INSTALL = install -c

target:

all: FRC

clean: FRC
	rm -f *.o errs core 

install: $(MAKEFILE) FRC
	@echo "Installing EXEC Makefile"
	$(INSTALL) -m 644 -o $(OWNER) \
	   		$(MAKEFILE)  ${DESTDIR}/$(EXECDIR)


FRC:
