#
# @(#)Makefile 1.1 92/07/30 SMI;
#
# Warning: The micro-assembler can use the output of cc only if
#	  cc is called with one argument!

DEFS= const+.h
SRCS=	diag.u misc.u sp.u dp.u xsp.u xdp.u cr.u direct.u\
	datan.u dexp.u dexpm.u dlnx.u dlnxp.u satan.u sexp.u\
	sincos.u slnx.u slnxp.u ssincos.u sexpm.u
GET= sccs get
CFLAGS=-O

all: get_all_file tot+.bin fpa_constants+

tot+.u: $(DEFS) $(SRCS)
	cat $(DEFS) $(SRCS) > temp
	/lib/cpp -P temp > tot+.u

tot+.bin: tot+.u
	../microassembler+/fpas+ -b tot+.bin tot+.u >tot+.out

.SUFFIXES: .u .out

clean:
	rm -f tot+.* temp const+.h fpa_constants+ compiler_const+.h

const+.h: constants+
	sed -f get_all_file constants+ > const+.h

sources: $(DEFS) $(SRCS)
$(SRCS) constants+ get_all_file get_compiler_file:
	$(GET) $@

fpa_constants+: constants+
	grep "^[sdc]" constants+ >fpa_constants+

install:	all
	install tot+.bin ${DESTDIR}/usr/etc/fpa/fpa_micro_bin+
	install fpa_constants+ ${DESTDIR}/usr/etc/fpa/fpa_constants+

compiler_const+.h: constants+
	sed -f get_compiler_file constants > compiler_const.h
