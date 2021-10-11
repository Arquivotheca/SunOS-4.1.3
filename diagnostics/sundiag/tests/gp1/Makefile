#	Makefile for gp1.
#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
PROGRAM= gpmtest
DESTDIR=
INCLUDES= gp1.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS   =

SOURCES= gpmtest.c
OBJECTS= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

DBX=-O
# specify DBX=-g for dbx version

CFLAGS= $(DBX) -I${INCDIR} -D`arch`
LDFILES= $(DBX)
LINTFLAGS= -I$(INCDIR)

UCODE=  gpmtest.ppfifo.2p               gpmtest.pp_29116.2p \
        gpmtest.all.2p                  gpmtest.ppprom.2p \
        gpmtest.allbutgb.2p             gpmtest.scrpad.2p \
        gpmtest.fifo_vme.2p             gpmtest.shmem.2p \
        gpmtest.fifo_vme_dec.2p         gpmtest.vme_byte.2p \
        gpmtest.fpalu.2p                gpmtest.vme_read.2p \
        gpmtest.fpmult.2p               gpmtest.vme_read_byte.2p \
        gpmtest.fprega.2p               gpmtest.vp_29116.2p \
        gpmtest.fpregb.2p               gpmtest.vpprom.2p \
        gpmtest.gbnorm.2p               gpmtest.xoperand.2p \
        gpmtest.gbrmw.2p                gpmtest.yoperand.2p \
        gpmtest.int_flag.2p

.KEEP_STATE:

##### beginning of dependency lines #####

all:	${INCLUDES} ${PROGRAM} $(UCODE)

$(PROGRAM): $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

install: all
	@if [ $(DESTDIR) ]; then \
          set -x; \
          install -s $(PROGRAM) ${DESTDIR}; \
          cp $(UCODE) ${DESTDIR}; \
          chmod u+w ${DESTDIR}/*2p; \
        else \
          set -x; \
          install $(PROGRAM) ../../bin; \
          cp $(UCODE) ../../bin; \
          chmod u+w ../../bin/*2p; \
        fi

clean: FRC
	rm -f ${PROGRAM} $(OBJECTS) $(LINTFILES) core 

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES)

info: FRC
	sccs info
include ../../include/Makefile.end
