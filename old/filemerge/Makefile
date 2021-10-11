#
# @(#)Makefile 1.1 92/08/05 SMI
#

BIN= filemerge
OBJS= filemerge.o sdiff.o popup.o util.o glyph.o estream.o conflib.o
HDRS= bar.pr barb.pr cursor.pr diff.h diff.icon filemerge.icon greater.pr \
	greaterb.pr less.pr lessb.pr minus.pr minusb.pr ml.icon \
	mr.icon plus.pr plusb.pr

INC= -I/usr/src/usr.lib/libsuntool/include
OPTIM= -O
CFLAGS= $(OPTIM)

LIBS = /usr/lib/debug/malloc.o -lsuntool -lsunwindow -lpixrect

.KEEP_STATE:

.INIT: $(HDRS)

$(BIN):	$(OBJS) 
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

estream.o filemerge.o := CFLAGS += $(INC)

install: $(BIN)
	install -s $(BIN) $(DESTDIR)/usr/old

clean: FRC
	$(RM) *.o $(BIN) a.out core errs 

FRC:
