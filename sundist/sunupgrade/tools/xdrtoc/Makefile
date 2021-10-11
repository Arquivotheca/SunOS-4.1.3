#
# @(#)Makefile 1.35 89/12/26 Copyr 1987 Sun Micro
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
AR	= ar
RPCCOM	= rpcgen
DEFS	=
OFLAG	= -O
CFLAGS	= $(OFLAG) $(DEFS) $(INCS)
#
LDFLAGS=-Bstatic
MKTPSRCDIR=../../../../sundist/mktp
#

LSUB	= toc.c screensub.c tocsub.c volsub.c sizesub.c dump_tables.c \
	get_tables.c global_data.c xdrio.c xdr_custom.c

GETLSUB	= screensub.c tocsub.c volsub.c sizesub.c dump_tables.c \
	get_tables.c global_data.c xdrio.c xdr_custom.c xdrtoc.c

HFILES=xdr_custom.h mktp.h

TOC	= toc.o xdrio.o xdr_custom.o
SUBOBJ	= screensub.o tocsub.o volsub.o sizesub.o dump_tables.o \
	get_tables.o global_data.o

CLEANFILES= dump_tables.c dump_tables.o get_tables.c get_tables.o global_data.c \
	global_data.o mktp.h screensub.c screensub.o sizesub.c sizesub.o \
	subr.a toc.c toc.h toc.o toc.x tocsub.c tocsub.o volsub.c \
	volsub.o xdr.a xdr_custom.c xdr_custom.h xdr_custom.o xdrio.c \
	xdrio.o xdrtoc xdrtoc.c xdrtoc.o

LIBXDR	= xdr.a
LIBSUB	= subr.a
LIBS	= $(LIBSUB) $(LIBXDR)
TLIB	= $(LDSEARCH) -lcurses -ltermlib
LLIB	= llib-lmktplint.ln

###############################
#
# rules for handle rpc dependency 
#

.SUFFIXES: .x .x~

.x.h:
	$(RPCCOM) -h $*.x -o $@

.x.c:
	$(RPCCOM) -c $*.x -o $@


###############################

all:	xdrtoc

xdrtoc:	xdrtoc.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ xdrtoc.o $(LIBS)


###############################
#
# get the sources
#
${GETLSUB}:
	cp ${MKTPSRCDIR}/$@ .

toc.x:
	cp ${MKTPSRCDIR}/$@ .

${HFILES}:
	cp ${MKTPSRCDIR}/$@ .

###############################
#
# Libraries
#

$(LIBXDR):	$(TOC)
	$(AR) cr $@ $(TOC) && ranlib $@

$(LIBSUB):	$(SUBOBJ)
	$(AR) cr $@ $(SUBOBJ) && ranlib $@


###############################
# pieces for the parser...
#

y.tab.h:	parser.y
	$(YACC) -d parser.y && rm -f y.tab.c

parser.o:	parser.y scanner.c

scanner.c:	scanner.l toc.h

#
###############################
#
#	lint section
#
lint:	$(LLIB)
	for i in $(MSUB); \
		do echo $$i: && lint llib-lmktplint.ln $(TLIB) $$i; done

$(LLIB):	toc.h $(LSUB)
	lint -Cmktplint $(LSUB) $(TLIB)

###############################


toc.h:		toc.x xdr_custom.h
toc.c:		toc.x toc.h
mktp.h:

mktp.o:		mktp.h toc.h
parse_input.o:	mktp.h toc.h
calc_sizes.o:	mktp.h toc.h
fit_file.o:	mktp.h toc.h
make_distribution.o:	mktp.h toc.h
edittoc.o:		mktp.h toc.h
global_data.o:	mktp.h toc.h
xdr_custom.o:	toc.h
xdrio.o:	toc.h
dump_tables.o:	toc.h
get_tables.o:	toc.h
tocsub.o:	mktp.h toc.h
volsub.o:	mktp.h toc.h
sizesub.o:	mktp.h toc.h
screensub.o:
xdrtoc.o:	mktp.h toc.h

###############################

clean:
	for i in ${CLEANFILES}; do \
		rm -f $$i; done

install:
		install -s -m 755 xdrtoc ${DESTDIR}/toolkit/xdrtoc




