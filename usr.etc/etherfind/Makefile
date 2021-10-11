#  @(#)Makefile 1.1 92/07/30 
# Makefile for Etherfind

BINS= etherfind
COMMON = filter.o ip.o ipprint.o rpcprint.o decprint.o atprint.o res_debug.o res_comp.o nfs_prot_xdr.o
OBJS = nit_streams.o tty_main.o parse.o  $(COMMON)
HDRS= etherfind.h ndio.h

CFLAGS = -O 

.INIT: $(HDRS)

.KEEP_STATE:

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

rpcprint.o: nfs_prot_xdr.c
nfs_prot_xdr.c: nfs_prot.x
	rpcgen -c -o nfs_prot_xdr.c nfs_prot.x
	rpcgen -h -o nfs_prot.h nfs_prot.x

clean:
	$(RM) $(BINS) $(OBJS)
