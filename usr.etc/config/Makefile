#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.3 2/18/86

#
# Architecture-specific programs
#
ASP= config

#
# Install directory for architecture-specific programs, absolute path
#
ASDIR= /usr/kvm

#
# Install directory for architecture-specific programs, relative to /usr/etc
#
RELASDIR= ../kvm

SRCS=	y.tab.c main.c lex.yy.c mkioconf.c mkmakefile.c \
	mkglue.c mkheaders.c mkbootconf.c sccsnames.c
OBJS=   $(SRCS:.c=.o)
HDRS=	config.h

CFLAGS=-O

.INIT: $(HDRS)

.KEEP_STATE:

$(ASP): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) -ll

lex.yy.c: config.l
	$(LEX) config.l

y.tab.h y.tab.c: config.y
	$(YACC) -d config.y

tags: $(SRCS)
	ctags -tw $(SRCS) $(HDRS)

clean:
	$(RM) $(ASP) $(OBJS) lex.yy.c y.tab.c y.tab.h tags

install: $(ASP)
	install -d -o bin -m 755 $(DESTDIR)$(ASDIR)
	@set -x; for i in $(ASP); do (install -s $$i $(DESTDIR)$(ASDIR)/$$i; \
		rm -f $(DESTDIR)/usr/etc/$$i; \
		/usr/bin/ln -s $(RELASDIR)/$$i $(DESTDIR)/usr/etc/$$i); \
	done

lint: $(SRCS) $(HDRS)
	$(LINT.c) $(SRCS)
