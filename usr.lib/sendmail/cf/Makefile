######################################################################
#
#	Makefile for Sendmail configuration files
#
#	@(#)Makefile 1.1 92/07/30 SMI; from UCB 3.20	5/1/83
#
######################################################################

UWSRCS=	dbvax.mc
SUNSRCS=main.mc \
	subsidiary.mc
GENSRCS=base.m4 \
	berkm.m4 \
	cchosts.m4 \
	compat.m4 \
	ddnm.m4 \
	ether.m4 \
	etherm.m4 \
	fudge.m4 \
	ibmm.m4 \
	localm.m4 \
	mmdfm.m4 \
	suucpm.m4 \
	uucpm.m4 \
	uucpproto.mc \
	version.m4 \
	zerobase.m4
SRCS=	$(GENSRCS) $(UWSRCS)

UWALL=	dbvax.cf
SUNALL=	main.cf \
	subsidiary.cf
PROTOS=	 uucpproto.cf
ALL=	$(SUNALL)

GET=	sccs get

.DEFAULT:
	sccs get -G$@ $@

.SUFFIXES: .mc .cf

.mc.cf:
	m4 $*.mc > $*.cf

all: $(ALL)

############################
#   special dependencies   #
############################

# all Sun hosts
$(SUNALL): sunbase.m4

# pessimize -- basic dependencies
$(ALL): etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 zerobase.m4

# hosts using UUCP
main.cf: suucpm.m4 uucpm.m4 compat.m4
subsidiary.cf: suucpm.m4 uucpm.m4 compat.m4
ucbvax.cf: uucpm.m4 compat.m4
uucpproto.cf: uucpm.m4 compat.m4

# vanilla dependencies
main.cf:	main.mc
subsidiary.cf:	subsidiary.mc

#####################
#   administrivia   #
#####################

sources: $(SRCS)

clean:
	rm -f $(ALL) a.out core make.out errs
	rm -f ,*
