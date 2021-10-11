#
# @(#)Makefile 1.1 92/07/30 SMI
#
CDBX=	-O
ARCH:sh = arch -k
CPPOPTS= -D$(ARCH)
INCOPTS= -I../../sys -I../../sys/$(ARCH)
CFLAGS=	$(CPPOPTS) $(INCOPTS) $(CDBX)
ARFLAGS=rv
LIB=	libkvm.a
PLIB=	libkvm_p.a
SLIB=	libkvm.so
ODIR=	obj
PDIR=	obj_p
SDIR=	shared
DIRS=	$(ODIR) $(PDIR) $(SDIR)
HDRS=	kvm.h
ALLHDRS= kvm_impl.h $(HDRS)

VERS-mc68010 = version
VERS-mc68020 = $(VERS-mc68010)
VERS-sparc =$(VERS-mc68010)

$(LIB) 	:= DIR 	= $(ODIR)
$(PLIB) := DIR 	= $(PDIR)
$(SLIB)	:= DIR	= $(SDIR)

MACHINES= iAPX286 i386 m68k mc68010 mc68020 pdp11 sparc sun \
	u370 u3b u3b2 u3b5 u3b15 vax
ARCHITECTURE= sun2 sun3 sun3x sun4 sun4c sun4m sun386
RELBINDIR=../bin
TRUE=$(RELBINDIR)/true
FALSE=$(RELBINDIR)/false

SRCS=	kvmopen.c kvmnlist.c kvmrdwr.c kvmgetu.c kvmgetcmd.c kvmnextproc.c
OBJS=	$(SRCS:%.c=$$(DIR)/%.o)
SOBJS=	$(SRCS:%.c=$(SDIR)/%.o)

ALL=	opt

debug	:=	CDBX= -D_KVM_DEBUG=0 -g
$(PLIB)	:=	CFLAGS += -p
$(SLIB)	:=	CFLAGS += -pic
$$(DIR)/%.o:	%.c
	$(CC) $(CFLAGS) -o $@ -c $<

.KEEP_STATE:
.PRECIOUS: $(LIB)
.INIT: $(DIRS) $(VERS$(TARGET_MACH))

all: $(ALLHDRS) $(ALL)

opt debug: $(SRCS) $(LIB) $(PLIB) $(SLIB)

$(LIB) $(PLIB): $(OBJS)
	?ar $(ARFLAGS) $@ $?
	ranlib $@

$(SLIB): $(OBJS)
	ld -o $@ $(SOBJS)

$(DIRS):
	test -d $@ || mkdir $@

install: all install_h $(VERS$(TARGET_VERS))
	install -o bin -m 755 -d $(DESTDIR)/usr/kvm
	for i in $(LIB) $(PLIB); do \
		install -m 644 $$i $(DESTDIR)/usr/kvm; \
		ranlib $(DESTDIR)/usr/kvm/$$i; \
		$(RM) $(DESTDIR)/usr/lib/$$i; \
		ln -s ../kvm/$$i $(DESTDIR)/usr/lib/$$i; \
	done
	for i in ${MACHINES}; do \
		if [ -f /usr/kvm/$$i ] && /usr/kvm/$$i; then \
			rm -f ${DESTDIR}/usr/kvm/$$i; \
			ln -s ${TRUE} ${DESTDIR}/usr/kvm/$$i; \
		else \
			rm -f ${DESTDIR}/usr/kvm/$$i; \
			ln -s ${FALSE} ${DESTDIR}/usr/kvm/$$i; \
		fi; \
	done
	for i in ${ARCHITECTURE}; do \
		if [ -f /usr/kvm/$$i ] && $$i; then \
			rm -f ${DESTDIR}/usr/kvm/$$i; \
			ln -s ${TRUE} ${DESTDIR}/usr/kvm/$$i; \
		else \
			rm -f ${DESTDIR}/usr/kvm/$$i; \
			ln -s ${FALSE} ${DESTDIR}/usr/kvm/$$i; \
		fi; \
	done
	if [ $(VERS$(TARGET_MACH)) ]; then \
		install -c -m 755 $(SLIB) \
			$(DESTDIR)/usr/kvm/$(SLIB).`cat version`; \
		$(RM) $(DESTDIR)/usr/lib/$(SLIB).`cat version`; \
		ln -s ../kvm/$(SLIB).`cat version` \
			$(DESTDIR)/usr/lib/$(SLIB).`cat version`; \
	fi;

install_h: $(HDRS)
	install -m 444 $(HDRS) $(DESTDIR)/usr/include

tags: $(ALLHDRS) $(SRCS)
	ctags -t $(ALLHDRS) $(SRCS)

clean:
	rm -rf $(DIRS) $(LIB) $(SLIB)* tags a.out
