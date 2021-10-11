# @(#)Makefile 1.1 92/07/30 SMI

DESTDIR = 
CFLAGS = -g

ALL= glass testi1 tst1 tst2 tst3 tst4 tst5 tst6 tst7 tst8

LIBS= -lcgi -lsunwindow -lpixrect -lm

all: $(ALL)

testi1 := MORESRC = testsubsin.c
testi1: testsubsin.c

$(ALL): $$@.c
	$(CC) $(CFLAGS) -o $@ $@.c $(MORESRC) $(LIBS)

src: $(ALL:%=%.c)

clean:
	$(RM) $(ALL) *.o a.out core
