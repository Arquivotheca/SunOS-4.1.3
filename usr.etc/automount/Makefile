#
#  @(#)Makefile 1.1 92/07/30 SMI
#
# Make file for automount
#
BINS= automount

OBJS= nfs_prot.o nfs_server.o nfs_trace.o nfs_cast.o \
	auto_main.o auto_look.o auto_proc.o auto_node.o \
	auto_mount.o auto_all.o
SRCS= $(OBJS:.o=.c)
HDRS= automount.h

.KEEP_STATE:

.INIT: $(HDRS)

CFLAGS= -O
LINTFLAGS= -hbax

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) -lrpcsvc 

nfs_prot.c: nfs_prot.h nfs_prot.x
	rpcgen -c nfs_prot.x -o $@

nfs_prot.h: nfs_prot.x
	rpcgen -h nfs_prot.x -o $@

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

tags: $(SRCS)
	ctags $(SRCS)

lint: $(SRCS)
	$(LINT.c) $(SRCS)
clean:
	$(RM) $(BINS) $(OBJS) nfs_prot.c nfs_prot.h
