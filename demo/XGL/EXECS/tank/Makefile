#
# @(#)Makefile 1.1 92/07/30 SMI
#

#
#  Makefile for the tank subdir
#

MAKEFILE = Makefile
DEMODIR = ${DESTDIR}/usr/demo/XGL/EXECS/tank

OWNER = bin
INSTALLX = install -o $(OWNER) 
INSTALL = $(INSTALLX) -m 644
INSTALLD = $(INSTALLX) -d -m 755
INSTALLXGL = $(INSTALLX) -m 755
INSTALLDATA = $(INSTALLX) -m 444

PROGS = tank
DATA = angelfish.char angelfish.vff bubble.vff crazy.char crazy.vff fractal.vff \
	goldfish.char goldfish.vff shark.char shark.vff tank.about_text \
	tank.help_text

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

