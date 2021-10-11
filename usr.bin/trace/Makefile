#
#  @(#)Makefile 1.1 92/07/30 SMI
#
#  Make file for system call tracer.
#
SRCS=	trace.c sysinfo.c errname.c
OBJS=	$(SRCS:.c=.o)
HDRS=	trace.h

ASDIR= /usr/kvm
RELASDIR= ../kvm

CFLAGS= -O

.KEEP_STATE:

trace :	trace.h  $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

errname.c : /usr/include/sys/errno.h errname.sh
	/bin/sh errname.sh

all: trace

install: trace
	install -d -o bin -m 755 $(DESTDIR)/usr/bin
	install -d -o bin -m 755 $(DESTDIR)$(ASDIR)
	install -s trace $(DESTDIR)$(ASDIR)
	rm -f $(DESTDIR)/usr/bin/trace
	/usr/bin/ln -s $(RELASDIR)/trace $(DESTDIR)/usr/bin/trace

clean:
	$(RM)  core a.out trace $(OBJS) errname.c

tags:
	ctags $(HDRS) $(SRCS)
