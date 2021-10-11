#
# @(#)Makefile 1.1 92/07/30 SMI from UCB 4.4 83/09/10
#
BINS= fsck
OBJS= dir.o inode.o main.o preen.o pass1.o pass1b.o pass2.o \
	pass3.o pass4.o pass5.o setup.o utilities.o
UFSOBJS= ufs_subr.o ufs_tables.o

UFSDIR= ../../sys/ufs
SRCS= $(OBJS:.o=.c) $(UFSOBJS:%.o=$(UFSDIR)/%.c)
HDRS= fsck.h

CPPFLAGS= -I../../sys
CFLAGS= -O
LDFLAGS += -Bstatic

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS) $(UFSOBJS)
	$(LINK.c) -o $@ $(OBJS) $(UFSOBJS)

$(UFSOBJS): $(UFSDIR)/$$(@:.o=.c)
	$(COMPILE.c) -o $@ $(UFSDIR)/$(@:.o=.c)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS) $(UFSOBJS)

lint: $(SRCS)
	$(LINT.c) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)
