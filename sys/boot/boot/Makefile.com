#
# @(#)Makefile.com 1.1 92/07/30
#
# This file is included by ../$(ARCH)/Makefile.
# It defines the rules needed to build the boot blocks and
# standalone programs.
#
# Certain symbols must be defined before this header file may be included:
#	$(ARCH)		target kernel architecture (e.g., sun2, sun4c)
#	$(ARCHOPTS)	CFLAGS specific to target kernel architecture
#	$(ARCHOBJ)	list of arch-dependent object files for libboot.a
#	$(ARCHLIBS)	list of arch-dependent object libraries
#	$(MACHDIR)	directory in which to find Makefile.mach (eg, ../m68k)
#	$(BOOTBLOCKS)	list of machine-dependent targets
#
# All targets are built in ../$(ARCH)

#ALL is now defined in each architecture's Makefile

BOOTDIR	=	..
SYSDIR	=	../..
LIBXX	=	libxx.a
KADB_LIB=	libkadb.a
LIBDIR	=	../lib
BOOTBLKDIR= 	$(DESTDIR)/usr/kvm/mdec
STANDDIR=	$(DESTDIR)/usr/kvm/stand
CONFDIR=	../../conf.common
ARCHDEST=	${LIBDIR}/${ARCH}
DRV_DIR	=	$(LIBDIR)/$(ARCH)
DRV_LIB	=	$(DRV_DIR)/libsa.a

LIBPROMDIR=	$(SYSDIR)/$(ARCH)
LIBPROMSRCDIR=	$(LIBDIR)/$(ARCH)
LIBPROM=	${LIBPROMDIR}/libprom.a
LIBPROMCPPOPTS=	-Dprintf=prom_printf -Dputchar=prom_putchar

SRT0	=	srt0.o
BOOT_LIB=	libboot.a
BBDRV_LIB=	$(DRV_DIR)/libxx.a
BBSRT0	=	srt0xx.o
LIBRPCSVC=	${SYSDIR}/rpcsvc/librpcsvc.a

COPTS	=	-O
CPPDEFS	=	-D$(ARCH) -DKERNEL -DUFS -DNFS -DINET -DNFS_BOOT -DNIMP=0 \
			$(ARCHOPTS)
CPPINCS	=	-I$(BOOTDIR) -I$(BOOTDIR)/boot -I$(LIBDIR) \
			-I$(SYSDIR)/$(ARCH) -I$(SYSDIR)
CPPBOOT	=
CPPDEBUG=	-DDUMP_DEBUG -DNFSDEBUG -DRPCDEBUG -DDEBUG
#CPPOPTS=	$(CPPDEFS) $(CPPINCS) $(CPPDEBUG)
CPPOPTS	=	$(CPPDEFS) $(CPPINCS)
CFLAGS	=	$(COPTS) $(CPPOPTS) $(CPPBOOT)
LDFLAGS	=	-N -T $(BRELOC) -e _start
KADBFLAGS=	-DKADB

LDIR	=	/usr/lib/lint
LINT1	=	$(LDIR)/lint1
LCOPTS	=	-C -Dlint $(CPPOPTS)
LOPTS	=	-hxbn
LTAIL	=	egrep -v 'genassym\.c' | \
		egrep -v 'struct/union .* never defined' | \
		egrep -v 'possible pointer alignment problem' ; true

.PRECIOUS: $(BOOT_LIB) $(DRV_LIB) $(BBDRV_LIB)

# Header files that need to be present
HSRC	=cmap.h inode.h nfs.h seg.h systm.h vnode.h

# these header files may go away...the normal system headers should suffice
TMPHDRS= domain.h if.h if_ether.h inet.h iob.h protosw.h

HDIR	=	../boot
HDRS	=	$(HSRC:%.h=$(HDIR)/%.h) $(TMPHDRS:%.h=$(HDIR)/%.h)

# The boot library targets and specific rules for building them are
# in Makefiles that are included by this one.  The following symbols
# must be defined:
#	$(SYSOBJ)	list of kernel object files for libboot.a
#	$(CMNOBJ)	list of common object files for libboot.a
#	$(MACHOBJ)	list of machine-dependent object files for libboot.a
#	$(BRELOC)	text segment relocation for final stage boot
#	$(BBRELOC)	text segment relocation for boot blocks
#

include ../os/Makefile.os
include $(MACHDIR)/Makefile.mach

$(BOOT_LIB) :=	LIB = $(BOOT_LIB)
$(LIBXX)    :=	LIB = $(LIBXX)
$(KADB_LIB) :=	LIB = $(KADB_LIB)
$(KADB_LIB) :=	CFLAGS += $(KADBFLAGS)

.PRECIOUS: $$(LIB)
.KEEP_STATE:

# Object files that are loaded into $(BOOT_LIB)
OBJ= $$(SYSOBJ) $$(CMNOBJ) $$(MACHOBJ) $$(ARCHOBJ)

# Except for kadb...
$(KADB_LIB) := SYSOBJ = standalloc.o
$(KADB_LIB) := CMNOBJ = $(SYSOBJ)
$(KADB_LIB) := ARCHOBJ = stubs.o
$(KADB_LIB) := MACHOBJ = $(ARCHOBJ)

$(BOOT_LIB) $(KADB_LIB): $$@($(OBJ))
	ranlib $@

# Libraries which live in other directories
$(DRV_LIB) $(BBDRV_LIB) $(LIBRPCSVC): FRC
	cd $(@D); $(MAKE) $(MFLAGS) $(@F)

$(LIBPROM): FRC
	cd $(LIBPROMSRCDIR); $(MAKE)

# boot: Standalone generic boot program -- get it from anywhere, boot anything.
boot	:= BOOT_OBJ = boot.o
boot: $$(SRT0) $$(BOOT_OBJ) vers.o $(BOOT_LIB) $(DRV_LIB) $(HDRS) $(ARCHLIBS)
	$(LD) $(LDFLAGS) -o $@ $(SRT0) $(BOOT_OBJ) vers.o $(BOOT_LIB) \
	$(DRV_LIB) $(ARCHLIBS)

# Strip the a.out headers for tape boot blocks.
%.$(ARCH): %
	cp $< $@.tmp; strip $@.tmp
	dd if=$@.tmp of=$@ ibs=32 skip=1; $(RM) $@.tmp

# Rules for building boot blocks
BB_OBJ= bootblk.o
bootpr bootsd bootxd bootxy bootid bootopen := BRELOC = $(BBRELOC)
bootpr := CONF_OBJ = confpr.o
bootsd := CONF_OBJ = confsd.o
bootxd := CONF_OBJ = confxd.o
bootxy := CONF_OBJ = confxy.o
bootid := CONF_OBJ = confip.o
bootopen := CONF_OBJ = conf.o

bootpr bootsd bootxd bootxy bootid: \
	    $(BBSRT0) $(BB_OBJ) $$(CONF_OBJ) $(BBDRV_LIB) ../boot/sizecheck
	$(LD) $(LDFLAGS) -o $@ $(BBSRT0) $(BB_OBJ) $(CONF_OBJ) $(BBDRV_LIB)
	../boot/sizecheck $@

bootopen: \
	    $(BBSRT0) $(BB_OBJ) $$(CONF_OBJ) $(BBDRV_LIB) $(ARCHLIBS) \
	    ../boot/sizecheck
	$(LD) $(LDFLAGS) -o $@ $(BBSRT0) $(BB_OBJ) $(CONF_OBJ) $(BBDRV_LIB) \
	    $(ARCHLIBS)
	../boot/sizecheck $@

#
# The sun2 tftp boot block
# NOTE: The sun2/50 prom ethernet driver seems to be writing to location
# a0400 when it gets opened, so we change the relocation value to a
# slightly lower value to avoid getting our program overwritten.
#
SUN2BBRELOC= 9e000
SUN2BBSRCS= bootnd.c udp.c tftp.c
SUN2BBFILES= ${SUN2BBSRCS:.c=.o}
sun2.bb: ${BBSRT0} ${SUN2BBFILES} ${BBDRV_LIB} ../boot/sizecheck
	ld ${LDFLAGS} -T ${SUN2BBRELOC} ${BBSRT0} ${SUN2BBFILES} \
		${BBDRV_LIB}
	../boot/sizecheck
	cp a.out b.out;strip b.out;dd if=b.out of=$@ ibs=32 skip=1

# Rules for files whose sources live here
BDIR= ../boot
%.o: $(BDIR)/%.c
	$(CC) $(CFLAGS) -DLOAD=0x$(LOAD) -c $<

# Rule for shell scripts (e.g., sizecheck)
%: $(BDIR)/%.sh
	${RM} $@
	cat $< > $@; chmod +x $@

# Rule for programs (e.g., installboot)
%: $(BDIR)/%.c
	$(CC) -D$(ARCH) $(COPTS) $(CPPINCS) -o $@ $<

vers.o: $$(SRT0) $$(BOOT_OBJ) $(BOOT_LIB) $(DRV_LIB) $(HDRS) $(ARCHLIBS)
vers.o: $(BDIR)/bootvers.sh ${CONFDIR}/RELEASE
	@sh $(BDIR)/bootvers.sh ${CONFDIR}/RELEASE ${ARCH}
	@${CC} ${CFLAGS} -c vers.c

# Special rule for tpboot
# We need to use a special readfile, in order to eliminate
# non-block integral reads.  We then link this in before the standalone lib,
# so we pick up that version of readfile.

tpboot	:= BOOT_OBJ = tpboot.o rdfil_tp.o
tpboot: $$(BBSRT0) $$(BOOT_OBJ) $$(DRV_LIB) $(BOOT_LIB)
	$(LD) $(LDFLAGS) -o $@ $(BBSRT0) $(BOOT_OBJ) $(DRV_LIB) $(BOOT_LIB)

RDFIL= ../os/readfile.c
rdfil_tp.o: $(RDFIL)
	$(CC) $(CFLAGS) -DLOAD=0x4000 -UNFS_BOOT $(RDFIL) -c -o rdfil_tp.o

# Configuration modules for boot blocks
CONF_SRC= $(BDIR)/confxx.c
confsd.o := CONF_DRV = sddriver
confxd.o := CONF_DRV = xddriver
confxy.o := CONF_DRV = xydriver
confip.o := CONF_DRV = ipidriver

confsd.o confxd.o confxy.o confip.o: $(CONF_SRC)
	$(CC) $(CFLAGS) -Dxxdriver=$(CONF_DRV) -c -o $@ $(CONF_SRC)


clean:
	$(RM) *.a *.o *.i core a.out assym.s ../boot/sizecheck $(ALL)

install: dirs install_h all install_mdec
	install -c boot.$(ARCH) $(STANDDIR)
	install installboot $(BOOTBLKDIR)
	cd $(LIBDIR); $(MAKE) $(MFLAGS) $@

install_h: $$(HDRS)
	cd $(LIBDIR); $(MAKE) $(MFLAGS) $@

dirs:
	install -d -o bin -m 755 $(STANDDIR)
	install -d -o bin -m 755 $(BOOTBLKDIR)
	rm -f $(DESTDIR)/usr/stand; ln -s ./kvm/stand $(DESTDIR)/usr/stand
	rm -f $(DESTDIR)/usr/mdec; ln -s ./kvm/mdec $(DESTDIR)/usr/mdec

lint:
	cd $(LIBDIR)/$(ARCH); $(MAKE) lint

FRC:

tags:	FRC
	ctags *.[ch] $(MACHDIR)/*.[ch] $(BOOTDIR)/boot/*.[ch] \
		${BOOTDIR}/lib/${ARCH}/*.[ch] ${BOOTDIR}/lib/common/*.[ch] \
		$(BOOTDIR)/os/*.[ch]
	egrep 'ENTRY2*\(' *.s $(MACHDIR)/*.s ${BOOTDIR}/lib/${ARCH}/*.s \
		${BOOTDIR}/lib/${MACH}/*.s | sed	\
    -e 's|^\(.*\):\(.*ENTRY(\)\(.*\)\().*\)|\3	\1	/^\2\3\4$$/|'	\
    -e 's|^\(.*\):\(.*ENTRY2(\)\(.*\),\(.*\)\().*\)|\3	\1	/^\2\3,\4\5$$/\
\4	\1	/^\2\3,\4\5$$/|' >> tags;				\
	sort -u -o tags tags
