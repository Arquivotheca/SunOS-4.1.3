
#       Makefile for Sundiag tests.
#
#       @(#)Makefile 1.14 91/01/03 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
PROGRAM	=cg6test
INCLUDES =cg6test.h create_cosine.h create_sine.h macros.h wings.h \
	create_colors.h create_grid.h render_main.h  \
	$(INCDIR)/sundiag_rpcnums.h $(INCDIR)/sdrtns.h\
	lego.h lego-fbc-reg.h lego-thc-reg.h lego-tec-reg.h lego-dac-reg.h \
	lego-fhc-reg.h cg6var.h cg6fbc.h cg6tec.h cg6thc.h cg6reg.h \
	$(ONLINEINC)/libonline.h

SOURCES	=cg6test.c cg6_polygons.c cg6_lines.c \
	map.c desktop.c dbuf.c sine.c color_map.c render_main.c transform.c

DESTDIR =

#specify DBX=-g for dbx version
DBX	=-O

CFLAGS	= $(DBX) -I$(INCDIR)
LDFLAGS	= $(DBX)
LIBS	= -lsuntool -lsunwindow -lpixrect -lm
PIXLIB	=/usr/lego/pixrect/pixrect.latest.3X/libpixrect.a

OBJECTS =$(SOURCES:.c=.o)

.KEEP_STATE:
##### beginning of dependency lines #####

all: $(INCLUDES) $(PROGRAM)

# this '#' sign needs to be here

$(PROGRAM): $(OBJECTS)
	cc  $(LDFLAGS) -o $@ $(CFLAGS) $(OBJECTS) $(SDLIBS) $(LIBS) 

install: all FRC
	@if [ $(DESTDIR) ]; then \
		set -x; \
		install -s $(PROGRAM) $(DESTDIR); \
	else \
		set -x; \
		install $(PROGRAM) ../../bin; \
	fi

clean: FRC
	rm -f  $(PROGRAM) $(OBJECTS) core 

$(OBJECTS) : cg6test.h create_colors.h create_cosine.h create_grid.h \
		create_sine.h render_main.h wings.h

info: FRC
	sccs info

include ../../include/Makefile.end
