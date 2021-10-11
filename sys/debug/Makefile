#
# @(#)Makefile 1.1 92/07/30 SMI
#
HDIR=$(DESTDIR)/usr/include/debug
HFILES= debug.h debugger.h
FILES= $(HFILES) main.c support.c

all: $(FILES)

clean:

install: $(FILES)

install_h: $(FILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
