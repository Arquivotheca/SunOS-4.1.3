#
#      @(#)Makefile 1.1 92/07/30 Copyright  Sun Microsytems.
#

include ../../include/Makefile.macros

# specify "REV=" if build for 4.0.x
REV=NEW
TACC	=	tacc

PROGRAM =	taactest
DESTDIR	=
LIBS    =
INCDIR  =	../../include
INCLUDES=	$(INCDIR)/sdrtns.h
CFLAGS  =	-I$(INCDIR)
TACCFLAGS=	-c -fsingle

OBJ2FILES=	testall.o tallmap.o testsqcc.o testalureg.o testobjects.o\
		testallabs.o

TESTOBJECTS=	taacmsgs.o testmcram.o testaluop.o testbs.o testfpop.o\
		testlu.o lut.o luval.o testma.o mult44.o testrdrc.o testvec.o\
		testvport.o testvidregs.o testint.o testvram.o testsram.o\
		testsqst.o testbt.o testacp.o testpoly.o

all: FRC
	@if [ `arch` = "sun3" ]; then \
		if [ "$(REV)" = "NEW" ]; then \
			echo Using taactest.sun3.41; \
			rm -f taactest; \
			make taactest.sun3.41; \
			cp taactest.sun3.41 taactest; \
			chmod 775 taactest; \
		else \
			echo Using taactest.sun3.4x; \
			rm -f taactest; \
			make taactest.sun3.4x; \
			cp taactest.sun3.4x taactest; \
			chmod 775 taactest; \
		fi \
	elif [ `arch` = "sun4" ]; then \
		if [ "$(REV)" = "NEW" ]; then \
			echo Using taactest.sun4.41; \
			rm -f taactest; \
			make taactest.sun4.41; \
			cp taactest.sun4.41 taactest; \
			chmod 775 taactest; \
		else \
			echo Using taactest.sun4.4x; \
			rm -f taactest; \
			make taactest.sun4.4x; \
			cp taactest.sun4.4x taactest; \
			chmod 775 taactest; \
		fi \
	fi

taactest: $(OBJ2FILES) $(TESTLIB)
	cc -g -o taactest $(OBJ2FILES) -ltaac1 -lm $(SDLIBS) $(LIBS)


testall.o: testall.c varblock.h globals.h programs.h
	cc $(CFLAGS) -g -c testall.c
tallmap.o: tallmap.c testall.h varblock.h globals.h
	cc -g -c tallmap.c
testobjects.o: $(TESTOBJECTS)
	ld -r -o testobjects.o $(TESTOBJECTS)

taacmsgs.o: taacmsgs.c
	cc -g -c taacmsgs.c
testmcram.o: testmcram.c varblock.h globals.h mcramchiploc.h
	cc $(CFLAGS) -g -c testmcram.c
testalureg.o: testalureg.c varblock.h globals.h testall.h
	cc $(CFLAGS) -g -c testalureg.c
testaluop.o: testaluop.c varblock.h globals.h aluop.h
	cc $(CFLAGS) -g -c testaluop.c
testbs.o: testbs.c varblock.h globals.h bsop.h
	cc $(CFLAGS) -g -c testbs.c
testfpop.o: testfpop.c varblock.h globals.h fpop.h fpopfunc.h
	cc $(CFLAGS) -g -c testfpop.c
testlu.o: testlu.c varblock.h globals.h
	cc $(CFLAGS) -g -c testlu.c
testma.o: testma.c varblock.h globals.h
	cc $(CFLAGS) -g -c testma.c
testrdrc.o: testrdrc.c varblock.h globals.h
	cc $(CFLAGS) -g -c testrdrc.c
testvec.o: testvec.c varblock.h globals.h
	cc $(CFLAGS) -g -c testvec.c
testvport.o: testvport.c varblock.h globals.h testvportdefs.h vramchiploc.h
	cc $(CFLAGS) -g -c testvport.c
testvidregs.o: testvidregs.c varblock.h globals.h vidregs.h
	cc $(CFLAGS) -g -c testvidregs.c
testint.o: testint.c varblock.h globals.h
	cc $(CFLAGS) -g -c testint.c
testvram.o: testvram.c varblock.h globals.h vramchiploc.h
	cc $(CFLAGS) -g -c testvram.c
testsram.o: testsram.c varblock.h globals.h sramchiploc.h
	cc $(CFLAGS) -g -c testsram.c
testsqcc.o: testsqcc.c varblock.h globals.h testall.h
	cc $(CFLAGS) -g -c testsqcc.c
testsqst.o: testsqst.c varblock.h globals.h
	cc $(CFLAGS) -g -c testsqst.c
testbt.o: testbt.c varblock.h globals.h
	cc $(CFLAGS) -g -c testbt.c
testacp.o: testacp.c varblock.h globals.h
	cc $(CFLAGS) -g -c testacp.c
testpoly.o: testpoly.c varblock.h globals.h polygons.h
	cc $(CFLAGS) -g -c testpoly.c
lut.o: lut.c
	cc -g -c lut.c
luval.o: luval.c
	cc -g -c luval.c

OBJS=	testall.obj testalureg.obj testaluop.obj testbs.obj testma.obj\
	testrdrc.obj testvec.obj testfpop.obj testvport.obj testlu.obj\
	support.obj testmcram.obj testvram.obj testsram.obj testsqcc.obj\
	testsqst.obj testbt.obj testacp.obj testvidregs.obj testint.obj\
	testpoly.obj fbclr.obj fbchecksum.obj aluopphase.obj aluregphase.obj\
	bsphase.obj fpopphase1.obj fpopphase2.obj maphase.obj rdrcphase.obj\
	pixels.obj colors.obj

testallabs.o: testall.abs
	taabs2o testall.abs testallabs.o

testall.h: testall.abs
	tamakedef testall.map testall.h

testall.abs: $(OBJS)
	talink $(OBJS) -ltaac1

.SUFFIXES:	.tc~	.tc	.obj



.tc~.tc:
	$(GET) -G$*.tc $(GFLAGS) $<



.tc~.obj:
	$(GET) -G$*.tc $(GFLAGS) $<
	$(TACC) $(TACCFLAGS) $*.tc


.tc.obj:
	$(TACC) $(TACCFLAGS) $*.tc


testall.obj:	programs.h

testvport.obj:	testvportdefs.h

testbt.obj testacp.obj testpoly.obj:	mydefs.h

pixels.obj:	tavideo.h


aluopphase.obj: aluopphase.asm
	tasm aluopphase.asm
aluopphase.asm: aluopmkphase
	aluopmkphase > aluopphase.asm
aluopmkphase: aluopmkphase.o
	cc -o aluopmkphase aluopmkphase.o
aluopmkphase.o: aluopmkphase.c aluop.h
	cc -c aluopmkphase.c
aluregphase.obj: aluregphase.tc
	tacc -c aluregphase.tc
bsphase.obj: bsphase.asm
	tasm bsphase.asm
bsphase.asm: bsmkphase
	bsmkphase > bsphase.asm
bsmkphase: bsmkphase.o
	cc -o bsmkphase bsmkphase.o
bsmkphase.o: bsmkphase.c bsop.h
	cc -c bsmkphase.c
fpopphase2.obj: fpopphase2.asm
	tasm fpopphase2.asm
fpopphase2.asm: fpopmkphase
	fpopmkphase > fpopphase2.asm
fpopmkphase: fpopmkphase.o
	cc -o fpopmkphase fpopmkphase.o
fpopmkphase.o: fpopmkphase.c fpop.h
	cc -c fpopmkphase.c
maphase.obj: maphase.asm
	tasm maphase.asm
maphase.asm: mamkphase
	mamkphase > maphase.asm
mamkphase: mamkphase.o
	cc -o mamkphase mamkphase.o
mamkphase.o: mamkphase.c maop.h
	cc -c mamkphase.c
mult44.o: mult44.c
	cc -g -c mult44.c

rdrcphase.obj: rdrcphase.asm
	tasm rdrcphase.asm
rdrcphase.asm: rdrcmkphase
	rdrcmkphase > rdrcphase.asm
rdrcmkphase: rdrcmkphase.o
	cc -o rdrcmkphase rdrcmkphase.o
rdrcmkphase.o: rdrcmkphase.c rdrcop.h
	cc -c rdrcmkphase.c

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) ../../bin; \
	fi

clean: FRC
	-rm -f *.asm *.o *.obj *.map *.lst *.abs *mkphase testall.h

lint:

info: FRC
	sccs info

include ../../include/Makefile.end
