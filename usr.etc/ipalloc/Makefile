#
# @(#)Makefile 1.1 92/07/30 SMI
# Copyright 11/8/87 Sun Microsystems Inc.
#

DESTDIR=	/
INCLUDE=	/usr/include

RPCSVC =	../../usr.lib/librpcsvc

DEFINES =
INCLUDES =	-I$(RPCSVC) -I$(INCLUDE)
CFLAGS =	$(INCLUDES) $(DEFINES)

LINTFLAGS =     -abchxz $(DEFINES) $(INCLUDES)

RPC_IPALLOCD_FILES =	rpc.ipallocd.o ipalloc_svr.o netrange.o
RPC_IPALLOCD_LINT =	$(RPC_IPALLOCD_FILES:.o=.ln)

IPALLOC_X =	$(RPCSVC)/ipalloc.x

LINTFILES =		rpc.ipallocd.lint


rpc.ipallocd:	$(RPC_IPALLOCD_FILES)
	$(CC) $(LDFLAGS) -o rpc.ipallocd $(RPC_IPALLOCD_FILES) \
		$(RPCSVC)/librpcsvc.a

all:		rpc.ipallocd lint
lint:		$(LINTFILES)

rpc.ipallocd.lint:	$(RPC_IPALLOCD_LINT)
	$(LINT) -u $(LINT_FLAGS) $(RPC_IPALLOCD_LINT) 2>&1 > rpc.ipallocd.lint

clean:
	-rm -f rpc.ipallocd *.o *.ln ipalloc_svr.c $(LINTFILES)

install:	rpc.ipallocd
	install -s rpc.ipallocd $(DESTDIR)/usr/etc


ipalloc_svr.o:	$(IPALLOC_X)
rpc.ipallocd.o netrange.o : netrange.h

ipalloc_svr.c:	$(IPALLOC_X)
	-rm -f ipalloc_svr.c
	rpcgen -s tcp $(IPALLOC_X) \
		| sed -e "s.main().serverstub_main()." > ipalloc_svr.c
	chmod -w ipalloc_svr.c

.c.ln:
	$(LINT) -iu $(LINTFLAGS) $<
	@echo ''
