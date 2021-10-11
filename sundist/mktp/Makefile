#
# @(#)Makefile 1.1 92/07/30 Copyr 1987 Sun Micro
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
# You can override the installation point by a combination
# of using:
#
#	make DESTDIR=string install
#
DESTDIR	= ../bin
AR	= ar
RPCCOM	= rpcgen
DEFS	=
OFLAG	= -O
CFLAGS	= $(OFLAG) $(DEFS) $(INCS)
TOC	= toc.o xdrio.o xdr_custom.o
MSUB	= mktp.c parse_input.c calc_sizes.c fit_file.c make_distribution.c \
		edittoc.c tapeblock.c
LSUB	= toc.c screensub.c tocsub.c volsub.c sizesub.c dump_tables.c \
		get_tables.c global_data.c xdrio.c xdr_custom.c
SUBOBJ	= screensub.o tocsub.o volsub.o sizesub.o dump_tables.o \
		get_tables.o global_data.o
LIBXDR	= xdr.a
LIBSUB	= subr.a
LIBS	= $(LIBSUB) $(LIBXDR)
TLIB	= $(LDSEARCH) -lcurses -ltermlib
LLIB	= llib-lmktplint.ln
ALLBIN	= mktp parse_input calc_sizes fit_file make_distribution tapeblock \
		edittoc tapefile xdrtoc 
SHELLSC	= makeboot makesaexec tapeimage disksize
DOC	= README mktp.doc
ALL	= $(DOC) $(ALLBIN)

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

all:	$(ALLBIN) ${SHELLSC}

mktp:	mktp.o parser.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ mktp.o $(LIBS) $(TLIB)

parse_input:	parse_input.o parser.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ parse_input.o parser.o  $(LIBS)

calc_sizes:	calc_sizes.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ calc_sizes.o $(LIBS)

fit_file:	fit_file.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ fit_file.o $(LIBS)

make_distribution:	make_distribution.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ make_distribution.o $(LIBS) $(TLIB)

edittoc:	edittoc.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ edittoc.o $(LIBS) $(TLIB)

tapeblock:	tapeblock.c
	$(CC) -o $@ $(OFLAG) $?

tapefile:	tapefile.c
	$(CC) -o $@ $(OFLAG) $?

xdrtoc:	xdrtoc.o $(LIBS)
	$(CC) -o $@ xdrtoc.o $(LIBS)


makeboot:	makeboot.sh	;	cp $? $@; chmod +x $@

makesaexec:	makesaexec.sh	;	cp $? $@; chmod +x $@

tapeimage:	tapeimage.sh	;	cp $? $@; chmod +x $@

disksize:	disksize.sh	;	cp $? $@; chmod +x $@

###############################

docs:	$(DOC)
	tbl mktp.doc | troff -ms

###############################

install:	all
	if [ ! -d $(DESTDIR) ] ; then mkdir $(DESTDIR) ; fi
	cp $(ALLBIN) $(SHELLSC) $(DESTDIR)


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

README:	SCCS/s.README
	$(GET) $?

mktp.doc:	SCCS/s.mktp.doc
	$(GET) $?

###############################

clean:
	-rm -f *.o *.a $(ALLBIN) $(SHELLSC) $(LLIB) scanner.c \
		y.tab.h toc.c toc.h
