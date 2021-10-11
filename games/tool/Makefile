#
# @(#)Makefile 1.1 92/07/30 Copyr 1985 Sun Micro
#
#

.DEFAULT:
	sccs get -G$@ $@

CFLAGS=	-O
LIBS=	-lsuntool -lsunwindow -lpixrect -lm
CC= 	/bin/cc

OBJS =	chesstool.o pieces.o chessprog.o clock.o \
	gammontool.o human.o computer.o draw.o cubes.o dice.o \
	bkg.o evalmove.o score.o boggle.o lists.o sand.o \
	search.o decode.o input.o canfieldtool.o \
	life.o lifetool.o circle.o patterns.o prompt.o \
	menu.o misc.o pipe.o sendmove.o gamemerge.o

all: gamemerge boggledict gammonscore

boggledict: FRC
	cd boggletool; make boggledict

gammonscore: FRC
	cd gammontool; make gammonscore

FRC:

gamemerge: ${OBJS}
	$(CC) ${CFLAGS} -o gamemerge ${OBJS} ${LIBS}

chesstool.o: chesstool/chesstool.c chesstool/pieces.h chesstool/chesstool.h
	$(CC) $(CFLAGS) -c chesstool/chesstool.c
pieces.o: chesstool/pieces.c chesstool/pieces.h
	$(CC) $(CFLAGS) -c chesstool/pieces.c
chessprog.o: chesstool/chessprog.c chesstool/chesstool.h
	$(CC) $(CFLAGS) -c chesstool/chessprog.c
clock.o: chesstool/clock.c chesstool/chesstool.h chesstool/clockhands.h
	$(CC) $(CFLAGS) -c chesstool/clock.c
gammontool.o: gammontool/gammontool.c gammontool/defs.h \
	gammontool/bkgcodes.h gammontool/cursors.h \
	gammontool/score.h
	$(CC) $(CFLAGS) -c gammontool/gammontool.c
human.o: gammontool/human.c gammontool/defs.h
	$(CC) $(CFLAGS) -c gammontool/human.c
computer.o: gammontool/computer.c gammontool/defs.h gammontool/bkgcodes.h
	$(CC) $(CFLAGS) -c gammontool/computer.c
draw.o: gammontool/draw.c gammontool/defs.h
	$(CC) $(CFLAGS) -c gammontool/draw.c
cubes.o: gammontool/cubes.c
	$(CC) $(CFLAGS) -c gammontool/cubes.c
dice.o: gammontool/dice.c
	$(CC) $(CFLAGS) -c gammontool/dice.c
bkg.o: gammontool/bkg.c
	$(CC) $(CFLAGS) -c gammontool/bkg.c
evalmove.o: gammontool/evalmove.c
	$(CC) $(CFLAGS) -c gammontool/evalmove.c
score.o: gammontool/score.c
	$(CC) $(CFLAGS) -c gammontool/score.c
boggle.o: boggletool/boggle.c boggletool/defs.h boggletool/images.h
	$(CC) $(CFLAGS) -c boggletool/boggle.c
lists.o: boggletool/lists.c boggletool/defs.h
	$(CC) $(CFLAGS) -c boggletool/lists.c
sand.o: boggletool/sand.c boggletool/defs.h boggletool/outline.h \
	    boggletool/sandframes.h
	$(CC) $(CFLAGS) -c boggletool/sand.c
search.o: boggletool/search.c boggletool/defs.h
	$(CC) $(CFLAGS) -c boggletool/search.c
decode.o: canfieldtool/decode.c canfieldtool/defs.h
	$(CC) $(CFLAGS) -c canfieldtool/decode.c
input.o: canfieldtool/input.c canfieldtool/defs.h
	$(CC) $(CFLAGS) -c canfieldtool/input.c
canfieldtool.o: canfieldtool/canfieldtool.c canfieldtool/icon.h \
	canfieldtool/outline.h canfieldtool/defs.h
	$(CC) $(CFLAGS) -c canfieldtool/canfieldtool.c
menu.o: canfieldtool/menu.c canfieldtool/defs.h canfieldtool/text.h
	$(CC) $(CFLAGS) -c canfieldtool/menu.c
misc.o: canfieldtool/misc.c canfieldtool/defs.h canfieldtool/cards.h
	$(CC) $(CFLAGS) -c canfieldtool/misc.c
pipe.o: canfieldtool/pipe.c canfieldtool/defs.h
	$(CC) $(CFLAGS) -c canfieldtool/pipe.c
sendmove.o: canfieldtool/sendmove.c canfieldtool/defs.h
	$(CC) $(CFLAGS) -c canfieldtool/sendmove.c
lifetool.o: life/lifetool.c life/life.h
	$(CC) $(CFLAGS) -c life/lifetool.c
life.o: life/life.c life/life.h
	$(CC) $(CFLAGS) -c life/life.c
circle.o: life/circle.c
	$(CC) $(CFLAGS) -c life/circle.c
patterns.o: life/patterns.c life/life.h
	$(CC) $(CFLAGS) -c life/patterns.c
prompt.o: life/prompt.c life/ml.icon life/mr.icon life/cursor.pr life/mouse.pr
	$(CC) $(CFLAGS) -c life/prompt.c

clean:
	rm -f $(OBJS) gamemerge
install:
	install -s -m 4755 -o bin gamemerge $(DESTDIR)/usr/games/chesstool
	-ln -s chesstool $(DESTDIR)/usr/games/gammontool
	-ln -s chesstool $(DESTDIR)/usr/games/boggletool
	-ln -s chesstool $(DESTDIR)/usr/games/canfieldtool
	-ln -s chesstool $(DESTDIR)/usr/games/life
	install -s gammontool/gammonscore $(DESTDIR)/usr/games/gammonscore
	install boggletool/boggledict  $(DESTDIR)/usr/games/boggledict
