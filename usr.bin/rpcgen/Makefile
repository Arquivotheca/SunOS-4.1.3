#
#	@(#)Makefile 1.1 92/07/30 (C) 1987 SMI
#
# Makefile for rpc protocol compiler
# Copyright (C) 1987, Sun Microsystems, Inc.
#
.KEEP_STATE:
SRCS= rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c rpc_scan.c \
	rpc_svcout.c rpc_tblout.c rpc_util.c
HDRS= rpc_util.h rpc_parse.h rpc_scan.h
OBJS= $(SRCS:.c=.o)

GOAL=rpcgen
CFLAGS = -O
DESTDIR=

.INIT: $(HDRS)

$(GOAL): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $@

install: $(GOAL)
	install -s $(GOAL) $(DESTDIR)/usr/bin
	
lint: $(SRCS) $(HDRS)
	lint $(SRCS)

clean:
	rm -f $(GOAL) $(OBJS)


