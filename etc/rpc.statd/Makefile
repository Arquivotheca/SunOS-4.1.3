#
# @(#)Makefile 1.1 92/07/30 Copyright Sun Micro 1986
#
# Makefile for rpc.statd - the network status monitor

BINS= rpc.statd
OBJS= sm_svc.o sm_proc.o sm_statd.o
LOCKOBJS= tcp.o
LOCKSRCS= ../rpc.lockd/tcp.c
SRCS= $(OBJS:.o=.c) $(LOCKSRCS)

HDRS= sm_statd.h  

LIBS= -lrpcsvc
CFLAGS= -O

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS) $(LOCKOBJS)
	$(LINK.c) -o $@ $(OBJS) $(LOCKOBJS) $(LIBS)

$(LOCKOBJS): $(LOCKSRCS)
	$(COMPILE.c) -o $@ $(LOCKSRCS)

clean:
	$(RM) $(BINS) $(OBJS) $(LOCKOBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc
