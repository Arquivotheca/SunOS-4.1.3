#
# @(#)Makefile 1.1 92/07/30 (C) 1985 SMI
#
LINTBINS= makedbm mkalias stdethers stdhosts udpublickey \
	yppoll ypset 
BINS=$(LINTBINS) ypxfrd ypserv ypxfr yppush
#delete NDBM when ndbm in libc gets straight (say in beta) 
NDBM= ndbm.o
YPSERVOBJ= ypserv.o ypserv_ancil.o ypserv_map.o ypserv_proc.o ypserv_dbmtrak.o 
YPXFRDOBJ= ypxfrd_svc.o ypxfrd_subr.o ypxfrd_xdr.o 
YPXFROBJ= ypxfr.o ypxfrd_client.o ypxfrd_xdr.o 
YPPUSHOBJ= yppush.o yp_get_master.o yp_enum_master.o
NDBMSRC=/usr/src/lib/libc/gen/common/ndbm.c
LIBRARIES=asynch_resolver
LINTSRCS= $(LINTBINS:%=%.c)
YPSERVSRCS= $(YPSERVOBJ:.o=.c) $(NDBMSRC)
YPXFRDSRCS= $(YPXFRDOBJ:.o=.c) $(NDBMSRC)
YPXFRSRCS= $(YPXFROBJ:.o=.c) $(NDBMSRC)
YPPUSHSRCS= $(YPPUSHOBJ:.o=.c)
SRCS= $(LINTSRCS) $(YPSERVSRCS) $(YPXFRDSRCS) $(YPXFRSRCS) $(YPPUSHSRCS)
HDRS= ypdefs.h ypsym.h
SUBDIRS= revnetgroup mknetid
CFLAGS= -O
SCRIPTS= ypinit ypxfr_1perday ypxfr_2perday ypxfr_1perhour
OTHERS= make.script updaters

install := TARGET= install DESTDIR=$(DESTDIR)
clean   := TARGET= clean

.KEEP_STATE:

.INIT: $(HDRS)

all: $(LIBRARIES) $(BINS) $(OTHERS) $(SUBDIRS)

#delete this before beta
$(NDBM): $(NDBMSRC)
	cc $(CFLAGS) -c $(NDBMSRC) -o $(NDBM)
#end of delete this

ypserv: $(YPSERVOBJ)  $(NDBM) $(LIBRARIES)
	$(LINK.c) -o $@ $(YPSERVOBJ) $(NDBM)   asynch_resolver/libnres.a  -lresolv

ypxfrd: $(YPXFRDOBJ)	$(NDBM)   
	$(LINK.c) -o $@ $(YPXFRDOBJ) $(NDBM)   

ypxfr: $(YPXFROBJ)	$(NDBM)   
	$(LINK.c) -o $@  $(YPXFROBJ) $(NDBM)   

yppush: $(YPPUSHOBJ)	
	$(LINK.c) -o $@   $(YPPUSHOBJ) 

ypxfrd.h:	ypxfrd.x
	rpcgen ypxfrd.x	

ypxfrd_client.o:	ypxfrd.h
	cc $(CFLAGS)	-c ypxfrd_client.c

ypxfrd_subr.o:	ypxfrd.h
	cc $(CFLAGS)	-c ypxfrd_subr.c

ypxfrd_xdr.o:	ypxfrd.h
	cc $(CFLAGS)	-c ypxfrd_xdr.c

ypxfrd_svc.o:	ypxfrd.h
	cc $(CFLAGS) -Dmain=_main -c ypxfrd_svc.c

makedbm: $$@.o
	$(LINK.c) -o $@ $@.o -ldbm

install: mkdirs $(BINS) $(SCRIPTS) $(OTHERS) $(SUBDIRS)
	install -s $(BINS) $(DESTDIR)/usr/etc/yp
	cd $(DESTDIR)/usr/etc/yp; mv ypserv ..;mv ypxfrd ..
	install $(SCRIPTS) $(DESTDIR)/usr/etc/yp
	install make.script $(DESTDIR)/var/yp/Makefile
	install make.script $(DESTDIR)/usr/lib/NIS.Makefile
	install updaters $(DESTDIR)/var/yp

mkdirs:
	install -o bin -m 755 -d $(DESTDIR)/usr/etc/yp
	install -o bin -m 755 -d $(DESTDIR)/var/yp
	install -o bin -m 755 -d $(DESTDIR)/var/yp/binding

clean: $(SUBDIRS) FRC
	$(RM) $(BINS) $(BINS:%=%.o) $(YPSERVOBJ) $(SCRIPTS) \
		mkdir_done

$(SUBDIRS): FRC
	cd $@ ; $(MAKE) $(TARGET)

$(LIBRARIES): FRC
	cd $@ ; $(MAKE) $(TARGET)

lint: $(SRCS)
	$(LINT.c)   $(LINTSRCS) 
	$(LINT.c)   $(YPSERVSRCS)
	$(LINT.c)   $(YPXFRDSRCS)
	$(LINT.c)   $(YPXFRSRCS)
	$(LINT.c)   $(YPPUSHSRCS)

print: $(SRCS)
	pr $(SRCS) | lpr
FRC:
