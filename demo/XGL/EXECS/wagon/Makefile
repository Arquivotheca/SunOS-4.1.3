#
# @(#)Makefile 1.1 92/07/30 SMI
#

#
#  Makefile for the wagon subdir
#

MAKEFILE = Makefile
DEMODIR = ${DESTDIR}/usr/demo/XGL/EXECS/wagon

OWNER = bin
INSTALLX = install -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLD = $(INSTALLX) -d -m 755
INSTALLXGL = $(INSTALLX) -m 755
INSTALLDATA = $(INSTALLX) -m 444

PROGS = main.ow
DATA = wagonaxel.pg wagonbody.pg wagonhandle.pg wagonhandle2.pg wagonrim.pg wagontire.pg

target:

all: FRC

clean: FRC
	rm -f errs core 

install: $(MAKEFILE) FRC
	$(INSTALLD) $(DEMODIR)
	$(INSTALL) $(MAKEFILE)  $(DEMODIR)
	$(INSTALLXGL) $(PROGS) $(DEMODIR)
	$(INSTALLDATA) $(DATA) $(DEMODIR)

FRC:
