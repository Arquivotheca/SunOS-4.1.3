#
# Makefile for sun4m FakePROM stuff
#

include ./Makefile.master

#MACHINES = galaxy sunergy campus2
MACHINES = galaxy sunergy
#MACHIDS= 0x71 0x72 0x73
MACHIDS= 0x71 0x73

SUBDIRS = ${EEPROMDIR} ${FAKEPROM} ${MACHINES}

all: FRC ${MACHINES}

${FAKEPROM}: FRC
	cd ${FAKEPROM}; $(MAKE)

${MACHINES}: FRC ${FAKEPROM}
	cd $@; $(MAKE)

install: FRC ${FAKEPROM}
	-for i in ${MACHINES}; do (cd $$i; $(MAKE) install); done

clean:
	-rm -f .make.state *~ #*#
	-for i in ${SUBDIRS} test; do (cd $$i; $(MAKE) clean); done
	
FRC:

eeprom:	FRC
	-set ${MACHIDS}; for i in ${MACHINES}; do (cd eeprom; $(MAKE) \
	MACHNAME=$$i MACH=$$1); shift; done




