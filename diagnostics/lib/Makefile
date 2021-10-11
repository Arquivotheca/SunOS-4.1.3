#
#      @(#)Makefile	1.1 7/30/92  SMI 
#
.DEFAULT:
	sccs get -G$@ $@

OS =
#specify "OS=" for 4.x build

INCDIR	= include
INCLUDES = $(INCDIR)/libonline.h
ONLINELIB = libonline.a

LIBS	= libxdr.a libutil.a libucb.a
SRCS    =
OBJS    = $(SRCS:.c=.o)
ARCH	= sun4
#this is for 5.0 only since arch is not available

LIBDIR  = libxdr libutil include libucb

DBX	=
DBX2    = $(DBX:-g=DBX=-g)
# = -g for dbx version
CFLAGS	= $(DBX) -D$(OS) -D$(ARCH) -I$(INCDIR)

.KEEP_STATE:
.PRECIOUS: libonline.a

##### beginning of dependency lines #####

all: $(ONLINELIB)
$(ONLINELIB): $(INCLUDES) $(LIBS) $(OBJS)
	-rm -rf tmp
	-mkdir	tmp
#	-cp $(OBJS) tmp
	set -x; for i in $(LIBS) ; \
	     do (cd tmp; ar x ../$$i) ; done
	(cd tmp; rm -f __.SYMDEF; ar cr ../$@ `lorder * | tsort`)
	@if [ $(OS) ] ; then set -x ;\
	else set -x ; ranlib $@ ; fi
	-rm -rf tmp $(LIBS)

$(LIBS): FRC
	-cd $(@:.a=); $(MAKE) $(DBX2) OS=$(OS)
	-rm -f $@
	-ln -s $(@:.a=)/$@ $@ 

install: all FRC

library: all FRC
	-cd include;\
	 echo -n 'New Version Log on ' > tmp; date >> tmp;\
	 echo -n 'Modified from last ' >> tmp;\
	 sccs what libversion >> tmp;\
	 sccs edit libversion;\
	 echo >> libversion;\
	 cat tmp >> libversion;\
	 echo 'Reasons for this new version:' >> libversion;\
	 echo '- Please log reasons for this new version (use ^D to get out):';\
	 cat >> libversion; set -x;\
	 sccs delget libversion;\
	 rm -f tmp
	 

clean: FRC
	@for i in $(LIBDIR); do (set -x; cd $$i; make $(MFLAGS) clean); done

lint: FRC
	@for i in $(LIBDIR); do (set -x; cd $$i; make $(MFLAGS) lint); done

info: FRC
	sccs info
	@for i in $(LIBDIR); do (set -x; cd $$i; make $(MFLAGS) info); done

FRC:
