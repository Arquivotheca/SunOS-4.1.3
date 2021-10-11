#	@(#)Makefile 1.1 92/07/30 SMI
#Warning: The micro-assembler can use the output of cc only if
#	  cc is called with one argument!

DEFS= const.h
SRCS= sps.u dps.u xtn.u com.u trig.u tran.u ldp.u reg.u mis.u
GET= sccs get
CFLAGS=-O

all: get_all_file tot.bin tot.map fpa_constants

tot.u: $(DEFS) $(SRCS)
	cat $(DEFS) $(SRCS) > temp
	/lib/cpp -P temp > tot.u

tot.bin: tot.u
	../microassembler/fpas -b tot.bin -r tot.map tot.u >tot.out

.SUFFIXES: .u .out

clean:
	rm -f tot.* temp const.h fpa_constants compiler_const.h

const.h: constants
	sed -f get_all_file constants > const.h

sources: $(DEFS) $(SRCS)
$(SRCS) constants get_all_file get_compiler_file:
	$(GET) $@

fpa_constants: constants
	grep "^[sdc]" constants >fpa_constants

install:	all
	install tot.bin ${DESTDIR}/usr/etc/fpa/fpa_micro_bin
	install tot.map ${DESTDIR}/usr/etc/fpa/fpa_micro_map
	install fpa_constants ${DESTDIR}/usr/etc/fpa/fpa_constants

compiler_const.h: constants
	sed -f get_compiler_file constants > compiler_const.h
