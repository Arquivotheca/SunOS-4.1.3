#
# @(#)Makefile 1.1 92/07/30 Copyright Sun Micro 1986
#
# Makefile for rex - the remote execution server

BINS= under where on rexd
SRCS= rexd.c mount_nfs.c on.c where.c unix_login.c 
OBJS= rexd.o mount_nfs.o on.o where.o unix_login.o under.o where_main.o

CFLAGS = -O
CPPFLAGS= -I/usr/include/rpcsvc
LIBS = -lrpcsvc

.KEEP_STATE:

all: $(BINS)

rexd: $$@.o mount_nfs.o unix_login.o
	$(LINK.c) -o $@ $@.o mount_nfs.o unix_login.o $(LIBS)

on: $$@.o where.o
	$(LINK.c) -o $@ $@.o where.o $(LIBS)

install: rexd on
	install -d $(DESTDIR)/usr/etc
	install -d $(DESTDIR)/usr/bin
	install -s rexd $(DESTDIR)/usr/etc/rpc.rexd
	install -s on $(DESTDIR)/usr/bin

under: $$@.o mount_nfs.o
	$(LINK.c) -o $@ $@.o mount_nfs.o $(LIBS)

where: $$@.o where_main.o
	$(LINK.c) -o $@ $@.o where_main.o $(LIBS)

tags: $(SRCS)
	ctags $(SRCS)

clean:
	$(RM) $(BINS) $(OBJS)
