#
# @(#)Makefile 1.1 92/07/30 SMI
#
BINS= rpc.bootparamd
OBJS= bootparam_svc.o bootparam_subr.o bootparam_lib.o
SRCS= $(OBJS:.o=.c)
LINTFLAGS= -bnuvx

CFLAGS = -O
LIBS = -lrpcsvc

.KEEP_STATE:

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -m 755 -o root $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)

tags: $(SRCS)
	ctags -tw $(SRCS)

lint: $(SRCS)
	$(LINT.c) $(SRCS)

ref: tags
	sed 's,	/.*,,' tags | \
	awk ' { printf("%-26s%-16s%s\n", $$1, $$2, $$3) }' > $@

print: $(SRCS)
	r $? | $P
	touch $@
