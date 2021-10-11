#
#	@(#)Makefile 1.1 92/07/30 SMI
#
# 	Makefile for rpc.lockd - the network lock manager
#

BINS = rpc.lockd
OBJS= prot_main.o tcp.o udp.o svc_udp.o xdr_nlm.o xdr_klm.o sm_monitor.o \
	prot_pklm.o prot_pnlm.o prot_msg.o prot_libr.o prot_alloc.o \
	prot_freeall.o prot_proc.o prot_lock.o prot_priv.o \
	prot_share.o hash.o
SRCS= $(OBJS:.o=.c)
HDRS= prot_lock.h prot_time.h priv_prot.h sm_res.h lockf.h klm_prot.h \
	nlm_prot.h sm_inter.h

LOCAL_LIBS= -lrpcsvc -I.
CFLAGS= -g -DPATCH_VERSION="\"@($\#) rpc.lockd patch 100075-08 92/02/28\""

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LOCAL_LIBS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
