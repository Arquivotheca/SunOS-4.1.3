#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.1 85/04/05
#
.KEEP_STATE:
.FRC:

BINS= sed
BINDIR= $(DESTDIR)/usr/5bin

LOCAL_HDRS = sed.h

SRCS= sed0.c sed1.c
OBJS= sed0.o sed1.o

CFLAGS=-O -DS5EMUL

.INIT:  $(HDRS) $(LOCAL_HDRS)

bins : $(BINS)

$(LOCAL_BINS) $(BINS) : $$(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LINK_LIBS)

install: $(BINS)
	install -d $(BINDIR)
	install -s $(BINS) $(BINDIR)

install_h:

clean: 
	-rm -rf $(LOCAL_BINS) $(BINS) $(LOCAL_LIBS) $(LIBS) $(OBJS) \
		core a.out
