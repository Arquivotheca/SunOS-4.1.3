#
# @(#)Makefile 1.1 92/07/30 SMI;
#
DESTDIR=
CFLAGS=-O -I. $(IFLAGS)
RM=/bin/rm -f
CP=/bin/cp

DIRS=rpcsvc

OBJS= bootparam_xdr.o etherxdr.o ipalloc.o ipalloc_clnt.o klm_prot.o \
	mountxdr.o nlm_prot.o pnprpc.o pnprpc_clnt.o sm_inter.o \
	rex_xdr.o rquotaxdr.o rstatxdr.o rusersxdr.o rwallxdr.o \
	showfh_xdr.o sprayxdr.o yppasswdxdr.o xcrypt.o publickey.o
HDRS= bootparam.h ether.h ipalloc.h klm_prot.h mount.h nlm_prot.h \
	pnprpc.h pwdnm.h rex.h rquota.h rstat.h rusers.h rwall.h \
	showfh.h sm_inter.h spray.h yppasswd.h

XFILES=	bootparam_prot.x ether.x ipalloc.x klm_prot.x mount.x nfs_prot.x \
	nlm_prot.x pnprpc.x \
	rex.x rnusers.x rquota.x rstat.x sm_inter.x spray.x yppasswd.x yp.x

librpcsvc.a: $(DIRS) $(OBJS)
	ar cr librpcsvc.a $(OBJS)
	ranlib librpcsvc.a

$(DIRS):
	if test ! -d $@; then mkdir $@; fi;

install:	librpcsvc.a install_h
	install -c -m 644 librpcsvc.a $(DESTDIR)/usr/lib
	ranlib ${DESTDIR}/usr/lib/librpcsvc.a

install_h: $(HDRS) $(XFILES)
	install -d -o bin -m 755 ${DESTDIR}/usr/include/rpcsvc
	install -m 444  $(XFILES) $(HDRS)  ${DESTDIR}/usr/include/rpcsvc

clean:
	rm -f *.o librpcsvc.a
	rm -fr rpcsvc
	rm -f pnprpc.[ch] ipalloc.[ch] ipalloc_clnt.c pnprpc_clnt.c

international:
	-rm -f xcrypt.o publickey.o
	$(CC) -c $(CFLAGS) xcrypt.c publickey.c
	ar rv $(DESTDIR)/usr/lib/librpcsvc.a xcrypt.o publickey.o
	ranlib $(DESTDIR)/usr/lib/librpcsvc.a
	-rm -f xcrypt.o publickey.o

sm_inter.o:	sm_inter.h
rex_xdr.o:	rex.h

bootparam_xdr.o:	rpcsvc/bootparam.h
etherxdr.o:	rpcsvc/ether.h
klm_prot.o:	rpcsvc/klm_prot.h
mountxdr.o:	rpcsvc/mount.h
nlm_prot.o:	rpcsvc/nlm_prot.h
rquotaxdr.o:	rpcsvc/rquota.h
rstatxdr.o:	rpcsvc/rstat.h
rusersxdr.o:	rpcsvc/rusers.h
rwallxdr.o:	rpcsvc/rwall.h
showfh_xdr.o:	rpcsvc/showfh.h
sprayxdr.o:	rpcsvc/spray.h
yppasswdxdr.o:	rpcsvc/yppasswd.h

rpcsvc/bootparam.h:	bootparam.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/ether.h:	ether.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/klm_prot.h:	klm_prot.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/mount.h:	mount.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/nlm_prot.h:	nlm_prot.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/rquota.h:	rquota.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/rstat.h:	rstat.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/rusers.h:	rusers.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/rwall.h:	rwall.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/showfh.h: showfh.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/spray.h:	spray.h;	$(RM) $@ ; $(CP) $? $@
rpcsvc/yppasswd.h:	yppasswd.h;	$(RM) $@ ; $(CP) $? $@

# XXX
ipalloc.h :	ipalloc.x
	-rm -f ipalloc.h
	rpcgen -h ipalloc.x -o ipalloc.h
	chmod -w ipalloc.h
ipalloc.c :	ipalloc.x ipalloc.h
	-rm -f ipalloc.c
	rpcgen -c ipalloc.x -o ipalloc.c
	chmod -w ipalloc.c
ipalloc_clnt.c:	ipalloc.x ipalloc.h
	-rm -f ipalloc_clnt.c
	rpcgen -l ipalloc.x -o ipalloc_clnt.c
	chmod -w ipalloc_clnt.c

pnprpc.h :	pnprpc.x
	-rm -f pnprpc.h
	rpcgen -h pnprpc.x -o pnprpc.h
	chmod -w pnprpc.h
pnprpc.c :	pnprpc.x pnprpc.h
	-rm -f pnprpc.c
	rpcgen -c pnprpc.x -o pnprpc.c
	chmod -w pnprpc.c
pnprpc_clnt.c:	pnprpc.x pnprpc.h
	-rm -f pnprpc_clnt.c
	rpcgen -l pnprpc.x -o pnprpc_clnt.c
	chmod -w pnprpc_clnt.c
