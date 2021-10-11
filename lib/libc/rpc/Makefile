# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
include ../Makefile.arch
include ../Makefile.master
CPPFLAGS = -I..
CFLAGS += $(CPPFLAGS) 
LDFLAGS += -assert pure-text

SRCS = auth_des.c auth_none.c auth_unix.c authdes_prot.c authunix_prot.c \
	clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c clnt_tcp.c \
	clnt_udp.c rpc_dtablesize.c get_myaddress.c key_call.c key_prot.c \
	netname.c openchild.c \
	rtime.c pmap_clnt.c pmap_getmaps.c pmap_getport.c pmap_prot.c \
	pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_commondata.c rpc_callmsg.c \
	svc.c svcauth_des.c svc_auth.c svc_auth_unix.c svc_raw.c svc_run.c \
	svc_simple.c svc_tcp.c svc_udp.c xdr.c xdr_array.c xdr_float.c \
	xdr_mem.c xdr_rec.c xdr_reference.c xdr_stdio.c

OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

HDRS = auth.h auth_des.h auth_unix.h clnt.h key_prot.h  pmap_clnt.h \
	pmap_prot.h pmap_rmt.h raw.h rpc.h rpc_msg.h svc.h svc_auth.h \
	types.h xdr.h

XFILES= key_prot.x

.KEEP_STATE:

all : xall

xall : $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../strings strings

.INIT: $(HDRS)

install: $(HDRS) $(XFILES)
	install -d -o bin -m 755 ${DESTDIR}/usr/include/rpc
	install -m 444 $(HDRS) $(XFILES) ${DESTDIR}/usr/include/rpc

tags: $(SRCS) $(HDRS) 
	ctags -tw $(SRCS) $(HDRS)

ref: tags
	sed 's,	/.*,,' tags | \
	awk ' { printf("%-26s%-16s%s\n", $$1, $$2, $$3) }' > ref

lint: $(SRCS) $(HDRS)
	$(LINT.c) $(SRCS) 

clean: master.clean
# We comment out .SUFFIXES because key_prot.[ch] are checked in to
# SCCS. Both of these files were actually compiled using rpcgen, but
# since the kernel build does not use rpcgen, we must check them into
# SCCS. One day config will understand other languages besides C, and
# we can take this comment out.
#.SUFFIXES: .x .x~

.x.c:
	rpcgen -c $< | \
	sed 's/^#include \"$*\.h\"/#include <rpc\/$*\.h>/' > $@

.x.h:
	rpcgen -h $< > $@

#
# For making a release of the user rpc code to the net, we attach
# a disclaimer to each file and remove the KERNEL portions. 
# NOTE: unifdef does not understand the "defined(ident)" syntax,
# so be sure and do these parts by hand.
# Also, note that this Makefile is non-standard, and cannot be
# used as is for a distribution to non-Sun sites.
#
DISTUSER= distuser
${DISTUSER}: $(SRCS) $(HDRS) $(XFILES) disclaimer
	-mkdir ${DISTUSER}
	@for i in $(SRCS) $(HDRS) $(XFILES) ; do \
		echo $$i; \
		cp disclaimer ${DISTUSER}/$$i; \
		chmod +w ${DISTUSER}/$$i; \
		unifdef -UKERNEL $$i \
		| sed -e "s,./. <> ./,," >> ${DISTUSER}/$$i; \
	done

depend: $(SRCS) $(HDRS)
	@${CC} ${CFLAGS} -M ${SRC} > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp Makefile makefile.bak
	@ed - Makefile < eddep
	@rm eddep makedep makefile.bak

# DO NOT DELETE THIS LINE
auth_des.c:
auth_des.o: auth_des.c
auth_des.o: /usr/include/sys/types.h
auth_des.o: /usr/include/sys/sysmacros.h
auth_des.o: /usr/include/sys/time.h
auth_des.o: /usr/include/time.h
auth_des.o: /usr/include/sys/socket.h
auth_des.o: /usr/include/des_crypt.h
auth_des.o: ../rpc/types.h
auth_des.o: /usr/include/sys/types.h
auth_des.o: ../rpc/auth.h
auth_des.o: ../rpc/auth_des.h
auth_des.o: ../rpc/xdr.h
auth_none.c:
auth_none.o: auth_none.c
auth_none.o: ../rpc/types.h
auth_none.o: /usr/include/sys/types.h
auth_none.o: /usr/include/sys/sysmacros.h
auth_none.o: ../rpc/xdr.h
auth_none.o: ../rpc/auth.h
auth_unix.c:
auth_unix.o: auth_unix.c
auth_unix.o: /usr/include/stdio.h
auth_unix.o: /usr/include/sys/time.h
auth_unix.o: /usr/include/time.h
auth_unix.o: ../rpc/types.h
auth_unix.o: /usr/include/sys/types.h
auth_unix.o: /usr/include/sys/sysmacros.h
auth_unix.o: ../rpc/xdr.h
auth_unix.o: ../rpc/auth.h
auth_unix.o: ../rpc/auth_unix.h
authdes_prot.c:
authdes_prot.o: authdes_prot.c
authdes_prot.o: /usr/include/sys/types.h
authdes_prot.o: /usr/include/sys/sysmacros.h
authdes_prot.o: /usr/include/sys/time.h
authdes_prot.o: /usr/include/time.h
authdes_prot.o: ../rpc/types.h
authdes_prot.o: /usr/include/sys/types.h
authdes_prot.o: ../rpc/xdr.h
authdes_prot.o: ../rpc/auth.h
authdes_prot.o: ../rpc/auth_des.h
authunix_prot.c:
authunix_prot.o: authunix_prot.c
authunix_prot.o: ../rpc/types.h
authunix_prot.o: /usr/include/sys/types.h
authunix_prot.o: /usr/include/sys/sysmacros.h
authunix_prot.o: ../rpc/xdr.h
authunix_prot.o: ../rpc/auth.h
authunix_prot.o: ../rpc/auth_unix.h
clnt_perror.c:
clnt_perror.o: clnt_perror.c
clnt_perror.o: /usr/include/stdio.h
clnt_perror.o: ../rpc/types.h
clnt_perror.o: /usr/include/sys/types.h
clnt_perror.o: /usr/include/sys/sysmacros.h
clnt_perror.o: ../rpc/auth.h
clnt_perror.o: ../rpc/clnt.h
clnt_raw.c:
clnt_raw.o: clnt_raw.c
clnt_raw.o: ../rpc/rpc.h
clnt_raw.o: ../rpc/types.h
clnt_raw.o: /usr/include/sys/types.h
clnt_raw.o: /usr/include/sys/sysmacros.h
clnt_raw.o: /usr/include/netinet/in.h
clnt_raw.o: ../rpc/xdr.h
clnt_raw.o: ../rpc/auth.h
clnt_raw.o: ../rpc/clnt.h
clnt_raw.o: ../rpc/rpc_msg.h
clnt_raw.o: ../rpc/auth_unix.h
clnt_raw.o: ../rpc/svc.h
clnt_raw.o: ../rpc/svc_auth.h
clnt_raw.o: /usr/include/sys/time.h
clnt_raw.o: /usr/include/time.h
clnt_simple.c:
clnt_simple.o: clnt_simple.c
clnt_simple.o: /usr/include/stdio.h
clnt_simple.o: ../rpc/rpc.h
clnt_simple.o: ../rpc/types.h
clnt_simple.o: /usr/include/sys/types.h
clnt_simple.o: /usr/include/sys/sysmacros.h
clnt_simple.o: /usr/include/netinet/in.h
clnt_simple.o: ../rpc/xdr.h
clnt_simple.o: ../rpc/auth.h
clnt_simple.o: ../rpc/clnt.h
clnt_simple.o: ../rpc/rpc_msg.h
clnt_simple.o: ../rpc/auth_unix.h
clnt_simple.o: ../rpc/svc.h
clnt_simple.o: ../rpc/svc_auth.h
clnt_simple.o: /usr/include/sys/socket.h
clnt_simple.o: /usr/include/sys/time.h
clnt_simple.o: /usr/include/time.h
clnt_simple.o: /usr/include/netdb.h
clnt_simple.o: /usr/include/strings.h
clnt_tcp.c:
clnt_tcp.o: clnt_tcp.c
clnt_tcp.o: /usr/include/stdio.h
clnt_tcp.o: ../rpc/rpc.h
clnt_tcp.o: ../rpc/types.h
clnt_tcp.o: /usr/include/sys/types.h
clnt_tcp.o: /usr/include/sys/sysmacros.h
clnt_tcp.o: /usr/include/netinet/in.h
clnt_tcp.o: ../rpc/xdr.h
clnt_tcp.o: ../rpc/auth.h
clnt_tcp.o: ../rpc/clnt.h
clnt_tcp.o: ../rpc/rpc_msg.h
clnt_tcp.o: ../rpc/auth_unix.h
clnt_tcp.o: ../rpc/svc.h
clnt_tcp.o: ../rpc/svc_auth.h
clnt_tcp.o: /usr/include/sys/socket.h
clnt_tcp.o: /usr/include/sys/time.h
clnt_tcp.o: /usr/include/time.h
clnt_tcp.o: /usr/include/netdb.h
clnt_tcp.o: /usr/include/errno.h
clnt_tcp.o: /usr/include/sys/errno.h
clnt_tcp.o: ../rpc/pmap_clnt.h
clnt_udp.c:
clnt_udp.o: clnt_udp.c
clnt_udp.o: /usr/include/stdio.h
clnt_udp.o: ../rpc/rpc.h
clnt_udp.o: ../rpc/types.h
clnt_udp.o: /usr/include/sys/types.h
clnt_udp.o: /usr/include/sys/sysmacros.h
clnt_udp.o: /usr/include/netinet/in.h
clnt_udp.o: ../rpc/xdr.h
clnt_udp.o: ../rpc/auth.h
clnt_udp.o: ../rpc/clnt.h
clnt_udp.o: ../rpc/rpc_msg.h
clnt_udp.o: ../rpc/auth_unix.h
clnt_udp.o: ../rpc/svc.h
clnt_udp.o: ../rpc/svc_auth.h
clnt_udp.o: /usr/include/sys/socket.h
clnt_udp.o: /usr/include/sys/time.h
clnt_udp.o: /usr/include/time.h
clnt_udp.o: /usr/include/sys/ioctl.h
clnt_udp.o: /usr/include/sys/ttychars.h
clnt_udp.o: /usr/include/sys/ttydev.h
clnt_udp.o: /usr/include/netdb.h
clnt_udp.o: /usr/include/errno.h
clnt_udp.o: /usr/include/sys/errno.h
clnt_udp.o: ../rpc/pmap_clnt.h
get_myaddress.c:
get_myaddress.o: get_myaddress.c
get_myaddress.o: ../rpc/types.h
get_myaddress.o: /usr/include/sys/types.h
get_myaddress.o: /usr/include/sys/sysmacros.h
get_myaddress.o: ../rpc/pmap_prot.h
get_myaddress.o: /usr/include/sys/socket.h
get_myaddress.o: /usr/include/stdio.h
get_myaddress.o: /usr/include/net/if.h
get_myaddress.o: /usr/include/sys/ioctl.h
get_myaddress.o: /usr/include/sys/ttychars.h
get_myaddress.o: /usr/include/sys/ttydev.h
get_myaddress.o: /usr/include/arpa/inet.h
get_myaddress.o: /usr/include/netinet/in.h
key_call.c:
key_call.o: key_call.c
key_call.o: /usr/include/sys/param.h
key_call.o: /usr/include/machine/param.h
key_call.o: /usr/include/sys/signal.h
key_call.o: /usr/include/sys/types.h
key_call.o: /usr/include/sys/sysmacros.h
key_call.o: /usr/include/sys/socket.h
key_call.o: /usr/include/sys/time.h
key_call.o: /usr/include/time.h
key_call.o: ../rpc/rpc.h
key_call.o: ../rpc/types.h
key_call.o: /usr/include/sys/types.h
key_call.o: /usr/include/netinet/in.h
key_call.o: ../rpc/xdr.h
key_call.o: ../rpc/auth.h
key_call.o: ../rpc/clnt.h
key_call.o: ../rpc/rpc_msg.h
key_call.o: ../rpc/auth_unix.h
key_call.o: ../rpc/svc.h
key_call.o: ../rpc/svc_auth.h
key_call.o: ../rpc/key_prot.h
key_call.o: /usr/include/stdio.h
key_call.o: /usr/include/sys/wait.h
key_prot.c:
key_prot.o: key_prot.c
key_prot.o: ../rpc/rpc.h
key_prot.o: ../rpc/types.h
key_prot.o: /usr/include/sys/types.h
key_prot.o: /usr/include/sys/sysmacros.h
key_prot.o: /usr/include/netinet/in.h
key_prot.o: ../rpc/xdr.h
key_prot.o: ../rpc/auth.h
key_prot.o: ../rpc/clnt.h
key_prot.o: ../rpc/rpc_msg.h
key_prot.o: ../rpc/auth_unix.h
key_prot.o: ../rpc/svc.h
key_prot.o: ../rpc/svc_auth.h
key_prot.o: ../rpc/key_prot.h
key_prot.o: /usr/include/sys/param.h
key_prot.o: /usr/include/machine/param.h
key_prot.o: /usr/include/sys/signal.h
key_prot.o: /usr/include/sys/types.h
netname.c:
netname.o: netname.c
netname.o: /usr/include/sys/param.h
netname.o: /usr/include/machine/param.h
netname.o: /usr/include/sys/signal.h
netname.o: /usr/include/sys/types.h
netname.o: /usr/include/sys/sysmacros.h
netname.o: ../rpc/rpc.h
netname.o: ../rpc/types.h
netname.o: /usr/include/sys/types.h
netname.o: /usr/include/netinet/in.h
netname.o: ../rpc/xdr.h
netname.o: ../rpc/auth.h
netname.o: ../rpc/clnt.h
netname.o: ../rpc/rpc_msg.h
netname.o: ../rpc/auth_unix.h
netname.o: ../rpc/svc.h
netname.o: ../rpc/svc_auth.h
netname.o: /usr/include/ctype.h
netname.o: /usr/include/netdb.h
openchild.c:
openchild.o: openchild.c
openchild.o: /usr/include/stdio.h
rtime.c:
rtime.o: rtime.c
rtime.o: /usr/include/sys/types.h
rtime.o: /usr/include/sys/sysmacros.h
rtime.o: /usr/include/sys/socket.h
rtime.o: /usr/include/sys/time.h
rtime.o: /usr/include/time.h
rtime.o: /usr/include/sys/errno.h
rtime.o: /usr/include/netinet/in.h
rtime.o: /usr/include/stdio.h
pmap_clnt.c:
pmap_clnt.o: pmap_clnt.c
pmap_clnt.o: ../rpc/rpc.h
pmap_clnt.o: ../rpc/types.h
pmap_clnt.o: /usr/include/sys/types.h
pmap_clnt.o: /usr/include/sys/sysmacros.h
pmap_clnt.o: /usr/include/netinet/in.h
pmap_clnt.o: ../rpc/xdr.h
pmap_clnt.o: ../rpc/auth.h
pmap_clnt.o: ../rpc/clnt.h
pmap_clnt.o: ../rpc/rpc_msg.h
pmap_clnt.o: ../rpc/auth_unix.h
pmap_clnt.o: ../rpc/svc.h
pmap_clnt.o: ../rpc/svc_auth.h
pmap_clnt.o: ../rpc/pmap_prot.h
pmap_clnt.o: ../rpc/pmap_clnt.h
pmap_clnt.o: /usr/include/sys/time.h
pmap_clnt.o: /usr/include/time.h
pmap_getmaps.c:
pmap_getmaps.o: pmap_getmaps.c
pmap_getmaps.o: ../rpc/rpc.h
pmap_getmaps.o: ../rpc/types.h
pmap_getmaps.o: /usr/include/sys/types.h
pmap_getmaps.o: /usr/include/sys/sysmacros.h
pmap_getmaps.o: /usr/include/netinet/in.h
pmap_getmaps.o: ../rpc/xdr.h
pmap_getmaps.o: ../rpc/auth.h
pmap_getmaps.o: ../rpc/clnt.h
pmap_getmaps.o: ../rpc/rpc_msg.h
pmap_getmaps.o: ../rpc/auth_unix.h
pmap_getmaps.o: ../rpc/svc.h
pmap_getmaps.o: ../rpc/svc_auth.h
pmap_getmaps.o: ../rpc/pmap_prot.h
pmap_getmaps.o: ../rpc/pmap_clnt.h
pmap_getmaps.o: /usr/include/sys/socket.h
pmap_getmaps.o: /usr/include/sys/time.h
pmap_getmaps.o: /usr/include/time.h
pmap_getmaps.o: /usr/include/netdb.h
pmap_getmaps.o: /usr/include/stdio.h
pmap_getmaps.o: /usr/include/errno.h
pmap_getmaps.o: /usr/include/sys/errno.h
pmap_getmaps.o: /usr/include/net/if.h
pmap_getmaps.o: /usr/include/sys/ioctl.h
pmap_getmaps.o: /usr/include/sys/ttychars.h
pmap_getmaps.o: /usr/include/sys/ttydev.h
pmap_getport.c:
pmap_getport.o: pmap_getport.c
pmap_getport.o: ../rpc/rpc.h
pmap_getport.o: ../rpc/types.h
pmap_getport.o: /usr/include/sys/types.h
pmap_getport.o: /usr/include/sys/sysmacros.h
pmap_getport.o: /usr/include/netinet/in.h
pmap_getport.o: ../rpc/xdr.h
pmap_getport.o: ../rpc/auth.h
pmap_getport.o: ../rpc/clnt.h
pmap_getport.o: ../rpc/rpc_msg.h
pmap_getport.o: ../rpc/auth_unix.h
pmap_getport.o: ../rpc/svc.h
pmap_getport.o: ../rpc/svc_auth.h
pmap_getport.o: ../rpc/pmap_prot.h
pmap_getport.o: ../rpc/pmap_clnt.h
pmap_getport.o: /usr/include/sys/socket.h
pmap_getport.o: /usr/include/sys/time.h
pmap_getport.o: /usr/include/time.h
pmap_getport.o: /usr/include/net/if.h
pmap_prot.c:
pmap_prot.o: pmap_prot.c
pmap_prot.o: ../rpc/types.h
pmap_prot.o: /usr/include/sys/types.h
pmap_prot.o: /usr/include/sys/sysmacros.h
pmap_prot.o: ../rpc/xdr.h
pmap_prot.o: ../rpc/pmap_prot.h
pmap_prot2.c:
pmap_prot2.o: pmap_prot2.c
pmap_prot2.o: ../rpc/types.h
pmap_prot2.o: /usr/include/sys/types.h
pmap_prot2.o: /usr/include/sys/sysmacros.h
pmap_prot2.o: ../rpc/xdr.h
pmap_prot2.o: ../rpc/pmap_prot.h
pmap_rmt.c:
pmap_rmt.o: pmap_rmt.c
pmap_rmt.o: ../rpc/rpc.h
pmap_rmt.o: ../rpc/types.h
pmap_rmt.o: /usr/include/sys/types.h
pmap_rmt.o: /usr/include/sys/sysmacros.h
pmap_rmt.o: /usr/include/netinet/in.h
pmap_rmt.o: ../rpc/xdr.h
pmap_rmt.o: ../rpc/auth.h
pmap_rmt.o: ../rpc/clnt.h
pmap_rmt.o: ../rpc/rpc_msg.h
pmap_rmt.o: ../rpc/auth_unix.h
pmap_rmt.o: ../rpc/svc.h
pmap_rmt.o: ../rpc/svc_auth.h
pmap_rmt.o: ../rpc/pmap_prot.h
pmap_rmt.o: ../rpc/pmap_clnt.h
pmap_rmt.o: /usr/include/sys/socket.h
pmap_rmt.o: /usr/include/sys/time.h
pmap_rmt.o: /usr/include/time.h
pmap_rmt.o: /usr/include/stdio.h
pmap_rmt.o: /usr/include/errno.h
pmap_rmt.o: /usr/include/sys/errno.h
pmap_rmt.o: /usr/include/net/if.h
pmap_rmt.o: /usr/include/sys/ioctl.h
pmap_rmt.o: /usr/include/sys/ttychars.h
pmap_rmt.o: /usr/include/sys/ttydev.h
pmap_rmt.o: /usr/include/arpa/inet.h
rpc_prot.c:
rpc_prot.o: rpc_prot.c
rpc_prot.o: /usr/include/sys/param.h
rpc_prot.o: /usr/include/machine/param.h
rpc_prot.o: /usr/include/sys/signal.h
rpc_prot.o: /usr/include/sys/types.h
rpc_prot.o: /usr/include/sys/sysmacros.h
rpc_prot.o: ../rpc/rpc.h
rpc_prot.o: ../rpc/types.h
rpc_prot.o: /usr/include/sys/types.h
rpc_prot.o: /usr/include/netinet/in.h
rpc_prot.o: ../rpc/xdr.h
rpc_prot.o: ../rpc/auth.h
rpc_prot.o: ../rpc/clnt.h
rpc_prot.o: ../rpc/rpc_msg.h
rpc_prot.o: ../rpc/auth_unix.h
rpc_prot.o: ../rpc/svc.h
rpc_prot.o: ../rpc/svc_auth.h
rpc_callmsg.c:
rpc_callmsg.o: rpc_callmsg.c
rpc_callmsg.o: /usr/include/sys/param.h
rpc_callmsg.o: /usr/include/machine/param.h
rpc_callmsg.o: /usr/include/sys/signal.h
rpc_callmsg.o: /usr/include/sys/types.h
rpc_callmsg.o: /usr/include/sys/sysmacros.h
rpc_callmsg.o: ../rpc/rpc.h
rpc_callmsg.o: ../rpc/types.h
rpc_callmsg.o: /usr/include/sys/types.h
rpc_callmsg.o: /usr/include/netinet/in.h
rpc_callmsg.o: ../rpc/xdr.h
rpc_callmsg.o: ../rpc/auth.h
rpc_callmsg.o: ../rpc/clnt.h
rpc_callmsg.o: ../rpc/rpc_msg.h
rpc_callmsg.o: ../rpc/auth_unix.h
rpc_callmsg.o: ../rpc/svc.h
rpc_callmsg.o: ../rpc/svc_auth.h
svc.c:
svc.o: svc.c
svc.o: /usr/include/sys/errno.h
svc.o: /usr/include/sys/time.h
svc.o: /usr/include/time.h
svc.o: ../rpc/rpc.h
svc.o: ../rpc/types.h
svc.o: /usr/include/sys/types.h
svc.o: /usr/include/sys/sysmacros.h
svc.o: /usr/include/netinet/in.h
svc.o: ../rpc/xdr.h
svc.o: ../rpc/auth.h
svc.o: ../rpc/clnt.h
svc.o: ../rpc/rpc_msg.h
svc.o: ../rpc/auth_unix.h
svc.o: ../rpc/svc.h
svc.o: ../rpc/svc_auth.h
svc.o: ../rpc/pmap_clnt.h
svcauth_des.c:
svcauth_des.o: svcauth_des.c
svcauth_des.o: /usr/include/des_crypt.h
svcauth_des.o: /usr/include/sys/types.h
svcauth_des.o: /usr/include/sys/sysmacros.h
svcauth_des.o: /usr/include/sys/time.h
svcauth_des.o: /usr/include/time.h
svcauth_des.o: /usr/include/sys/param.h
svcauth_des.o: /usr/include/machine/param.h
svcauth_des.o: /usr/include/sys/signal.h
svcauth_des.o: /usr/include/sys/types.h
svcauth_des.o: /usr/include/netinet/in.h
svcauth_des.o: ../rpc/types.h
svcauth_des.o: /usr/include/sys/types.h
svcauth_des.o: ../rpc/xdr.h
svcauth_des.o: ../rpc/auth.h
svcauth_des.o: ../rpc/auth_des.h
svcauth_des.o: ../rpc/svc_auth.h
svcauth_des.o: ../rpc/svc.h
svcauth_des.o: ../rpc/rpc_msg.h
svc_auth.c:
svc_auth.o: svc_auth.c
svc_auth.o: ../rpc/rpc.h
svc_auth.o: ../rpc/types.h
svc_auth.o: /usr/include/sys/types.h
svc_auth.o: /usr/include/sys/sysmacros.h
svc_auth.o: /usr/include/netinet/in.h
svc_auth.o: ../rpc/xdr.h
svc_auth.o: ../rpc/auth.h
svc_auth.o: ../rpc/clnt.h
svc_auth.o: ../rpc/rpc_msg.h
svc_auth.o: ../rpc/auth_unix.h
svc_auth.o: ../rpc/svc.h
svc_auth.o: ../rpc/svc_auth.h
svc_auth_unix.c:
svc_auth_unix.o: svc_auth_unix.c
svc_auth_unix.o: /usr/include/stdio.h
svc_auth_unix.o: /usr/include/sys/time.h
svc_auth_unix.o: /usr/include/time.h
svc_auth_unix.o: ../rpc/rpc.h
svc_auth_unix.o: ../rpc/types.h
svc_auth_unix.o: /usr/include/sys/types.h
svc_auth_unix.o: /usr/include/sys/sysmacros.h
svc_auth_unix.o: /usr/include/netinet/in.h
svc_auth_unix.o: ../rpc/xdr.h
svc_auth_unix.o: ../rpc/auth.h
svc_auth_unix.o: ../rpc/clnt.h
svc_auth_unix.o: ../rpc/rpc_msg.h
svc_auth_unix.o: ../rpc/auth_unix.h
svc_auth_unix.o: ../rpc/svc.h
svc_auth_unix.o: ../rpc/svc_auth.h
svc_raw.c:
svc_raw.o: svc_raw.c
svc_raw.o: ../rpc/rpc.h
svc_raw.o: ../rpc/types.h
svc_raw.o: /usr/include/sys/types.h
svc_raw.o: /usr/include/sys/sysmacros.h
svc_raw.o: /usr/include/netinet/in.h
svc_raw.o: ../rpc/xdr.h
svc_raw.o: ../rpc/auth.h
svc_raw.o: ../rpc/clnt.h
svc_raw.o: ../rpc/rpc_msg.h
svc_raw.o: ../rpc/auth_unix.h
svc_raw.o: ../rpc/svc.h
svc_raw.o: ../rpc/svc_auth.h
svc_simple.c:
svc_simple.o: svc_simple.c
svc_simple.o: /usr/include/stdio.h
svc_simple.o: ../rpc/rpc.h
svc_simple.o: ../rpc/types.h
svc_simple.o: /usr/include/sys/types.h
svc_simple.o: /usr/include/sys/sysmacros.h
svc_simple.o: /usr/include/netinet/in.h
svc_simple.o: ../rpc/xdr.h
svc_simple.o: ../rpc/auth.h
svc_simple.o: ../rpc/clnt.h
svc_simple.o: ../rpc/rpc_msg.h
svc_simple.o: ../rpc/auth_unix.h
svc_simple.o: ../rpc/svc.h
svc_simple.o: ../rpc/svc_auth.h
svc_simple.o: /usr/include/sys/socket.h
svc_simple.o: /usr/include/sys/time.h
svc_simple.o: /usr/include/time.h
svc_simple.o: /usr/include/netdb.h
svc_tcp.c:
svc_tcp.o: svc_tcp.c
svc_tcp.o: /usr/include/stdio.h
svc_tcp.o: ../rpc/rpc.h
svc_tcp.o: ../rpc/types.h
svc_tcp.o: /usr/include/sys/types.h
svc_tcp.o: /usr/include/sys/sysmacros.h
svc_tcp.o: /usr/include/netinet/in.h
svc_tcp.o: ../rpc/xdr.h
svc_tcp.o: ../rpc/auth.h
svc_tcp.o: ../rpc/clnt.h
svc_tcp.o: ../rpc/rpc_msg.h
svc_tcp.o: ../rpc/auth_unix.h
svc_tcp.o: ../rpc/svc.h
svc_tcp.o: ../rpc/svc_auth.h
svc_tcp.o: /usr/include/sys/socket.h
svc_tcp.o: /usr/include/sys/time.h
svc_tcp.o: /usr/include/time.h
svc_tcp.o: /usr/include/errno.h
svc_tcp.o: /usr/include/sys/errno.h
svc_udp.c:
svc_udp.o: svc_udp.c
svc_udp.o: /usr/include/stdio.h
svc_udp.o: ../rpc/rpc.h
svc_udp.o: ../rpc/types.h
svc_udp.o: /usr/include/sys/types.h
svc_udp.o: /usr/include/sys/sysmacros.h
svc_udp.o: /usr/include/netinet/in.h
svc_udp.o: ../rpc/xdr.h
svc_udp.o: ../rpc/auth.h
svc_udp.o: ../rpc/clnt.h
svc_udp.o: ../rpc/rpc_msg.h
svc_udp.o: ../rpc/auth_unix.h
svc_udp.o: ../rpc/svc.h
svc_udp.o: ../rpc/svc_auth.h
svc_udp.o: /usr/include/sys/socket.h
svc_udp.o: /usr/include/errno.h
svc_udp.o: /usr/include/sys/errno.h
xdr.c:
xdr.o: xdr.c
xdr.o: /usr/include/stdio.h
xdr.o: ../rpc/types.h
xdr.o: /usr/include/sys/types.h
xdr.o: /usr/include/sys/sysmacros.h
xdr.o: ../rpc/xdr.h
xdr_array.c:
xdr_array.o: xdr_array.c
xdr_array.o: /usr/include/stdio.h
xdr_array.o: ../rpc/types.h
xdr_array.o: /usr/include/sys/types.h
xdr_array.o: /usr/include/sys/sysmacros.h
xdr_array.o: ../rpc/xdr.h
xdr_float.c:
xdr_float.o: xdr_float.c
xdr_float.o: /usr/include/stdio.h
xdr_float.o: ../rpc/types.h
xdr_float.o: /usr/include/sys/types.h
xdr_float.o: /usr/include/sys/sysmacros.h
xdr_float.o: ../rpc/xdr.h
xdr_mem.c:
xdr_mem.o: xdr_mem.c
xdr_mem.o: ../rpc/types.h
xdr_mem.o: /usr/include/sys/types.h
xdr_mem.o: /usr/include/sys/sysmacros.h
xdr_mem.o: ../rpc/xdr.h
xdr_mem.o: /usr/include/netinet/in.h
xdr_rec.c:
xdr_rec.o: xdr_rec.c
xdr_rec.o: /usr/include/stdio.h
xdr_rec.o: ../rpc/types.h
xdr_rec.o: /usr/include/sys/types.h
xdr_rec.o: /usr/include/sys/sysmacros.h
xdr_rec.o: ../rpc/xdr.h
xdr_rec.o: /usr/include/sys/time.h
xdr_rec.o: /usr/include/time.h
xdr_rec.o: /usr/include/netinet/in.h
xdr_reference.c:
xdr_reference.o: xdr_reference.c
xdr_reference.o: /usr/include/stdio.h
xdr_reference.o: ../rpc/types.h
xdr_reference.o: /usr/include/sys/types.h
xdr_reference.o: /usr/include/sys/sysmacros.h
xdr_reference.o: ../rpc/xdr.h
xdr_stdio.c:
xdr_stdio.o: xdr_stdio.c
xdr_stdio.o: ../rpc/types.h
xdr_stdio.o: /usr/include/sys/types.h
xdr_stdio.o: /usr/include/sys/sysmacros.h
xdr_stdio.o: /usr/include/stdio.h
xdr_stdio.o: ../rpc/xdr.h
