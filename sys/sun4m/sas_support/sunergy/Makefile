#
# @(#)Makefile	1.1 8/6/90 SMI
#

#
# Makefile for Sunergy FakePROM
#

include ../Makefile.master

MACH:sh = basename `/bin/pwd`
MOD = ssparc 

MYEEPROM=eeprom.${MACH}
PROG=${MACH}.${FAKEPROM}
RUNFILE=${MACH}.file
MACHFILE= ${MACH}.setup
MOBJS= ${MACH}_tree.o ${MOD}_tree.o start.o

all:	$(MOBJS)
	ld -N -T ${MONSTART} -o ${PROG} $(FLOBJS) $(MOBJS) -lc
	rm -rf ${ROMPFILE}
	echo "load	ram	${PROG} ${MONPSTART}" > ${ROMPFILE}
	echo "symtab	add	${PROG} 0xFFD00000 0xFFE00000" >> ${ROMPFILE}
	-rm -rf ${RUNFILE}
	cat ${FILE}/${FILE1} > ${RUNFILE}
	cat ${FILE}/${MACHFILE} >> ${RUNFILE}
	cat ${ROMPFILE} >> ${RUNFILE}
	cat ${FILE}/${FILE2} >> ${RUNFILE}
	-rm -f ${ROMPFILE}

install: all
	@if [ ! -n "${RUNDIR}" ] ; then \
		echo Must set RUNDIR in environment before install; \
	else \
		cp ${RUNFILE} ${PROG} ${MYEEPROM} ../${FAKEPROM}/${FAKEPROMPTE} $${RUNDIR}; \
		(cd $$RUNDIR; rm -rf ${EEPROM}; ln -s ${MYEEPROM} ${EEPROM}); \
		echo install complete for ${MACH} to $${RUNDIR}; \
	fi;


clean:
	rm -rf $(MOBJS) $(PROG) ${ROMPFILE} ${RUNFILE} *~ #*#

