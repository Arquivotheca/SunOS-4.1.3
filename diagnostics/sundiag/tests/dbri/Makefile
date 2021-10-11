#ident   "@(#)Makefile 1.1 92/07/30 SMI"

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros

# specify "REV=" if built for 4.0.X
REV=NEW

# specify DBX=-g for dbx version
DBX=-O

DESTDIR =
PROGRAM = audbri
#DATAFILES = audbri_hdli.data audbri_loli.data audbri_sbmic.data music.au
DATAFILES = music.au
INCLUDES= audioio.h audbri.h audbri_msg.h audio_filehdr.h audbri_g711.h \
	archdep.h $(INCDIR)/sdrtns.h $(ONLINEINC)/libonline.h
LIBS	= -lm

SOURCES	= audbri.c audbri_globals.c audbri_msg.c  audbri_tests.c \
	audbri_utils.c audbri_fft.c audbri_g711.c audbri_calc.c
OBJECTS	= $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

CFLAGS	= $(DBX) -D$(REV) -D`arch` -I$(INCDIR)
LDFLAGS	= $(DBX)
LINTFLAGS= -D$(REV) -D`arch` -I$(INCDIR)

.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(DATAFILES) $(INCLUDES) $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	cc $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

install: all FRC
	@if [ $(DESTDIR) ]; then \
	  set -x; \
	  install -s $(PROGRAM) $(DESTDIR); \
	  install $(DATAFILES) $(DESTDIR); \
	else \
	  set -x; \
	  install $(PROGRAM) $(DATAFILES) ../../bin; \
	fi
		
clean: FRC
	rm -f $(PROGRAM) $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES) $(LIBS)

info: FRC
	sccs info

include ../../include/Makefile.end
