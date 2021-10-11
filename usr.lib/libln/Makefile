#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 85/05/30
#

LIBS= libln.a

LIBDIR= $(DESTDIR)/usr/lib

OBJS= allprint.o main.o reject.o yyless.o yywrap.o
SRCS= $(OBJS:.o=.c)

CFLAGS= -O

.KEEP_STATE:

all: $(LIBS)

$(LIBS): $(OBJS)
	ar rc $@ $?
	ranlib $@

install: $(LIBS)
	install -d $(LIBDIR)
	install -m 644 $(LIBS) $(LIBDIR)
	$(RM) $(LIBDIR)/libl.a
	ln $(LIBDIR)/libln.a $(LIBDIR)/libl.a
	ranlib $(LIBDIR)/$(LIBS)

clean:
	$(RM) $(OBJS) $(LIBS) tags

tags: $(SRCS)
	cwd=`pwd`; \
	for i in $(SRCS); do \
		ctags -a -f tags $$cwd/$$i; \
	done
