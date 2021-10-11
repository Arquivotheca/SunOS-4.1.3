#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
HDRS = yp_prot.h ypclnt.h ypupdate_prot.h ypv1_prot.h
SRCS = ypprot_err.c yp_bind.c yperr_string.c \
	yp_enum.c yp_match.c ypxdr.c \
	yp_all.c yp_master.c yp_order.c ypv1_xdr.c ypmaint_xdr.c \
	yp_update.c ypupdate_prot.c
OBJS = $(SRCS:%.c=$(VARIANT)/%.o)

all: xall

include ../Makefile.arch
include ../Makefile.master
#CFLAGS += -R
LDFLAGS += -assert pure-text

XFILES = ypupdate_prot.x

xall : $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../strings strings

.INIT: $(HDRS)

.SUFFIXES: .x

#
# The following rules generate the .h file and XDR routines from the
# rpcgen source. The ypupdate_prot.h file lives in /usr/include/rpcsvc
# but is built here. Install will move it over to the proto directory.
# in the mean time, objects built here, look for it here. Further, when
# building ypupdated it should look here too, but doesn't at the moment.
#
.x.c:
	rpcgen -c -o $@ $< 

#
# It would be nice to use a ".x.h" rule for this, but that causes problems.
# If you use a suffix rule, make tries to build /usr/include/rpcsvc/%.h
# from /usr/include/rpcsvc/%.x, which fails since most users don't have
# write permission in /usr/include.
#
ypupdate_prot.h: ypupdate_prot.x
	rm -f $@
	rpcgen -h -o $@  $?

ypupdate_prot.c: ypupdate_prot.x
 
install: $(XFILES) $(HDRS)
	install -d -o bin -m 755 ${DESTDIR}/usr/include/rpcsvc
	install -m 444 $(XFILES) $(HDRS) ${DESTDIR}/usr/include/rpcsvc

tags: $(SRCS) $(KSRC) $(HDRS)
	ctags -tw $(SRCS) $(KSRC) $(HDRS)

ref: tags
	sed 's,	/.*,,' tags | \
	awk ' { printf("%-26s%-16s%s\n", $$1, $$2, $$3) }' > ref

lint:
	lint -bnuvx $(SRCS)

print:
	pr $(HDRS) $(SRCS) $(KSRC) | lpr -Pvp

clean: master.clean
