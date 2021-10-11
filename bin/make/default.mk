# @(#)default.mk	1.1 SMI Copyright 1986

SUFFIXES = .o .c .c~ .cc .cc~ .s .s~ .S .S~ .ln .f .f~ .F .F~ .l .l~ \
	.mod .mod~ .sym .def .def~ .p .p~ .r .r~ .y .y~ .h .h~ .sh .sh~ \
	.cps .cps~
.SUFFIXES: $(SUFFIXES)

# OUTPUT_OPTION should be defined to "-o $@" when
# the default rules are used for non-local files.
OUTPUT_OPTION=

#	C language section.
CC=cc
CFLAGS=
CPPFLAGS=
LINT=lint
LINTFLAGS=
COMPILE.c=$(CC) $(CFLAGS) $(CPPFLAGS) -target $(TARGET_ARCH:-%=%) -c
LINK.c=$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -target $(TARGET_ARCH:-%=%)
LINT.c=$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
.c:
	$(LINK.c) -o $@ $< $(LDLIBS)
.c.ln:
	$(LINT.c) $(OUTPUT_OPTION) -i $<
.c.o:
	$(COMPILE.c) $(OUTPUT_OPTION) $<
.c.a:
	$(COMPILE.c) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	C language section. yacc.
YACC=yacc
YFLAGS=
YACC.y=$(YACC) $(YFLAGS)
.y:
	$(YACC.y) $<
	$(LINK.c) -o $@ y.tab.c $(LDLIBS)
	$(RM) y.tab.c
.y.c:
	$(YACC.y) $<
	mv y.tab.c $@
.y.ln:
	$(YACC.y) $<
	$(LINT.c) -o $@ -i y.tab.c
	$(RM) y.tab.c
.y.o:
	$(YACC.y) $<
	$(COMPILE.c) -o $@ y.tab.c
	$(RM) y.tab.c

#	C language section. lex.
LEX=lex
LFLAGS=
LEX.l=$(LEX) $(LFLAGS) -t
.l:
	$(RM) $*.c
	$(LEX.l) $< > $*.c
	$(LINK.c) -o $@ $*.c -ll $(LDLIBS)
	$(RM) $*.c
.l.c :
	$(RM) $@
	$(LEX.l) $< > $@
.l.ln:
	$(RM) $*.c
	$(LEX.l) $< > $*.c
	$(LINT.c) -o $@ -i $*.c
	$(RM) $*.c
.l.o:
	$(RM) $*.c
	$(LEX.l) $< > $*.c
	$(COMPILE.c) -o $@ $*.c
	$(RM) $*.c

#       C++ language section.
CCC=CC
CCFLAGS=
COMPILE.cc=$(CCC) $(CCFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.cc=$(CCC) $(CCFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
.cc:
	$(LINK.cc) -o $@ $< $(LDLIBS)
.cc.o:
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
.cc.a:
	$(COMPILE.cc) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	FORTRAN section.
FC=f77
FFLAGS=
COMPILE.f=$(FC) $(FFLAGS) $(TARGET_ARCH) -c
LINK.f=$(FC) $(FFLAGS) $(LDFLAGS) $(TARGET_ARCH)
COMPILE.F=$(FC) $(FFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.F=$(FC) $(FFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
.f:
	$(LINK.f) -o $@ $< $(LDLIBS)
.f.o:
	$(COMPILE.f) $(OUTPUT_OPTION) $<
.f.a:
	$(COMPILE.f) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%
.F:
	$(LINK.F) -o $@ $< $(LDLIBS)
.F.o:
	$(COMPILE.F) $(OUTPUT_OPTION) $<
.F.a:
	$(COMPILE.F) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	FORTRAN section. ratfor.
RFLAGS=
COMPILE.r=$(FC) $(FFLAGS) $(RFLAGS) $(TARGET_ARCH) -c
LINK.r=$(FC) $(FFLAGS) $(RFLAGS) $(LDFLAGS) $(TARGET_ARCH)
.r:
	$(LINK.r) -o $@ $< $(LDLIBS)
.r.o:
	$(COMPILE.r) $(OUTPUT_OPTION) $<
.r.a:
	$(COMPILE.r) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	Modula-2 section.
M2C=m2c
M2FLAGS=
MODFLAGS=
DEFFLAGS=
COMPILE.def=$(M2C) $(M2FLAGS) $(DEFFLAGS) $(TARGET_ARCH)
COMPILE.mod=$(M2C) $(M2FLAGS) $(MODFLAGS) $(TARGET_ARCH)
.def.sym:
	$(COMPILE.def) -o $@ $<
.mod:
	$(COMPILE.mod) -o $@ -e $@ $<
.mod.o:
	$(COMPILE.mod) -o $@ $<
.mod.a:
	$(COMPILE.mod) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	Pascal section.
PC=pc
PFLAGS=
COMPILE.p=$(PC) $(PFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
LINK.p=$(PC) $(PFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
.p:
	$(LINK.p) -o $@ $< $(LDLIBS)
.p.o:
	$(COMPILE.p) $(OUTPUT_OPTION) $<
.p.a:
	$(COMPILE.p) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	Assembly section.
AS=as
ASFLAGS=
COMPILE.s=$(AS) $(ASFLAGS) $(TARGET_MACH)
COMPILE.S=$(CC) $(ASFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
.s.o:
	$(COMPILE.s) -o $@ $<
.s.a:
	$(COMPILE.s) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%
.S.o:
	$(COMPILE.S) -o $@ $<
.S.a:
	$(COMPILE.S) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

#	Shell section.
.sh:
	$(RM) $@
	cat $< > $@
	chmod +x $@

#	NeWS section
CPS=cps
CPSFLAGS=
.cps.h:
	$(CPS) $(CPSFLAGS) $*.cps

#	Miscellaneous section.
LD=ld
LDFLAGS=
LDLIBS=
MAKE=make
RM=rm -f
AR=ar
ARFLAGS=rv
GET=/usr/sccs/get
GFLAGS=

markfile.o:	markfile
	echo "static char _sccsid[] = \"`grep @'(#)' markfile`\";" > markfile.c
	cc -c markfile.c
	$(RM) markfile.c

SCCSFLAGS=
SCCSGETFLAGS=-s
.SCCS_GET:
	?sccs $(SCCSFLAGS) get $(SCCSGETFLAGS) $@ -G$@
