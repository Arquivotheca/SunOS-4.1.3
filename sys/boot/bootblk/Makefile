#
# @(#)Makefile 1.1 92/07/30 SMI
#

MACH=	`mach`
ARCH=	`arch`
CC=	/bin/cc  -${ARCH}
AS=	as -${MACH}
RM=	rm -f

DESTDIR= 
SYSDIR=	../..
LIBDIR=	../lib
MONDIR= ../mon
LIBXX=	libxx.a
BBSU=	../${ARCH}/srt0xx.o
BBSU1=	../${ARCH}/srt0.o
LINTOBJ= ${LIBDIR}/llib-lsa.ln
ARCHDEST= ${LIBDIR}/${ARCH}

BOOTDEFS= "-Dprintf=(*romp->v_printf)" "-Dputchar=(*romp->v_putchar)"
CPPDEFS= -D${ARCH}
COPTS=	-O -Qoption as -O
CFLAGS=	${COPTS} ${BOOTDEFS}
LDFLAGS= -N -e _start

LDIR=	/usr/lib/lint
LINT1=	${LDIR}/lint1
LCOPTS=	-C -Dlint ${CPPOPTS} -DLOAD=0x${LOAD} 
LOPTS=	-hxb -n
LTAIL=	egrep -v 'struct/union .* never defined' | \
	egrep -v 'possible pointer alignment problem' ; true

# BRELOC is text segment start for final stage (2nd or 3rd stage) boots
# Note that this must be higher than the kernel's edata and should be
# higher than the kernel's bss so the bss can be cleared
BRELOC= 240000
# LOAD is text segment start for booted user programs
LOAD=	4000

PROGS=	bootxy bootsd installboot
SUN2BB= sun2.bb

SUN2PROGS= ${PROGS} ${SUN2BB}
SUN3PROGS= ${PROGS}
SUN4PROGS= ${PROGS}

ALL=	archdep

all: ${ALL}
archdep:
	@case ${ARCH} in \
	sun2)	${MAKE} -e ${MFLAGS} ${SUN2PROGS};;\
	sun3)	${MAKE} -e ${MFLAGS} ${SUN3PROGS};;\
	sun4)	${MAKE} -e ${MFLAGS} ${SUN4PROGS};;\
	esac


#
# The sun2 tftp boot block
# NOTE: The sun2/50 prom ethernet driver seems to be writing to location
# a0400 when it gets opened, so we change the relocation value to a
# slightly lower value to avoid getting our program overwritten.
#
SUN2BBRELOC= 9e000
SUN2BBSRCS= bootnd.c udp.c tftp.c
SUN2BBFILES= ${SUN2BBSRCS:.c=.o}
${SUN2BB}: ${BBSU1} ${SUN2BBFILES} ${LIBXX} sizecheck
	ld ${LDFLAGS} -T ${SUN2BBRELOC} ${BBSU1} ${SUN2BBFILES} \
		${ARCHDEST}/${LIBXX} -lc
	sizecheck
	cp a.out b.out;strip b.out;dd if=b.out of=$@ ibs=32 skip=1

# this little piece of chicanery gets the right version of bootsd made...
# the first time thru, a recursive make is done;
# the second time thru, BOOTSD is resolved to the arch-dependent filename
#
BOOTSD=FRC
bootsd: ${BOOTSD}
	@if [ ${BOOTSD} = FRC ]; then \
		case ${ARCH} in \
		sun2)   ${MAKE} -e ${MFLAGS} $@ BOOTSD=big.bootsd ;;\
		sun3)   ${MAKE} -e ${MFLAGS} $@ BOOTSD=bootpr ;;\
		sun4)   ${MAKE} -e ${MFLAGS} $@ BOOTSD=bootpr ;;\
		esac \
	else \
		cp ${ARCHDEST}/${BOOTSD} ${ARCHDEST}/$@; \
	fi

# We need to search libc.a to complete this load since the C
# compiler generates calls to ulmult.
#
bootxy: ${BBSU1} bootblk.o confxy.o ${LIBXX} sizecheck
	ld ${LDFLAGS} -T ${BRELOC} -o ${ARCHDEST}/$@ ${BBSU1} ${ARCHDEST}/bootblk.o \
${ARCHDEST}/confxy.o ${ARCHDEST}/${LIBXX} -lc
	sizecheck ${ARCHDEST}/$@

big.bootsd: ${BBSU1} bootblk.o confsd.o ${LIBXX} sizecheck
	ld ${LDFLAGS} -T ${BRELOC} -o ${ARCHDEST}/$@ ${BBSU1} ${ARCHDEST}/bootblk.o \
${ARCHDEST}/confsd.o ${ARCHDEST}/${LIBXX} -lc
	sizecheck ${ARCHDEST}/$@

bootpr: ${BBSU1} bootblk.o confpr.o ${LIBXX} sizecheck
	ld ${LDFLAGS} -T ${BRELOC} -o ${ARCHDEST}/$@ ${BBSU1} ${ARCHDEST}/bootblk.o \
${ARCHDEST}/confpr.o ${ARCHDEST}/${LIBXX} -lc
	sizecheck ${ARCHDEST}/$@

bootxd: ${BBSU1} bootblk.o confxd.o ${LIBXX} sizecheck
	ld ${LDFLAGS} -T ${BRELOC} -o ${ARCHDEST}/$@ ${BBSU1} ${ARCHDEST}/bootblk.o \
${ARCHDEST}/confxd.o ${ARCHDEST}/${LIBXX} -lc
	sizecheck ${ARCHDEST}/$@

# FRC ensures we just do it once.
bootblk.o:	FRC
	${MAKE} ARCH=${ARCH} ${ARCHDEST}/bootblk.o

${ARCHDEST}/bootblk.o:	 ../boot/bootblk.c
	${CC} -c ${CFLAGS} -DLOAD=0x${LOAD} -D${ARCH} -DBRELOC=0x${BRELOC} -o ${ARCHDEST}/bootblk.o ../boot/bootblk.c
installboot: ../boot/installboot.c
	${CC} ${COPTS} -o ${ARCHDEST}/installboot ../boot/installboot.c

# FRC ensures we just do it once.
confxy.o:	FRC
	${MAKE} ARCH=${ARCH} ${ARCHDEST}/confxy.o

${ARCHDEST}/confxy.o:	 ../boot/confxx.c
	cd ../boot; $(MAKE) $(MFLAGS) confxy.o

# FRC ensures we just do it once.
confxd.o:	FRC
	${MAKE} ARCH=${ARCH} ${ARCHDEST}/confxd.o

${ARCHDEST}/confxd.o:	../boot/confxx.c
	cd ../boot; $(MAKE) $(MFLAGS) confxd.o

# FRC ensures we just do it once.
confsd.o:	FRC
	${MAKE} ARCH=${ARCH} ${ARCHDEST}/confsd.o

${ARCHDEST}/confsd.o:	 ../boot/confxx.c
	cd ../boot; $(MAKE) $(MFLAGS) confsd.o

# FRC ensures we just do it once.
confpr.o:	FRC
	${MAKE} ARCH=${ARCH} ${ARCHDEST}/confpr.o

${ARCHDEST}/confpr.o:	 ../boot/confpr.c
	cd ../boot; $(MAKE) $(MFLAGS) confpr.o

# FRC ensures we just do it once.
# This version gets built to include the block list used by installboot.
${BBSU}: FRC
	cd ../${ARCH}; ${MAKE} -e $(@F)

# This version gets built without the block list used by installboot.
${BBSU1}: FRC
	cd ../${ARCH}; ${MAKE} -e $(@F)

${LIBXX}:	FRC
	cd ../lib; $(MAKE) ARCH=${ARCH} ${LIBXX}

sizecheck: ../bootblk/sizecheck.sh
	cp ../bootblk/sizecheck.sh sizecheck
	chmod a+xw sizecheck

rawboot:
	cd ../boot; $(MAKE) boot
	cp ../boot/boot .; strip boot
	dd if=boot of=rawboot bs=64 skip=1;
clean:
	${RM} ${LIBDIR}/sun2/bootxy ${LIBDIR}/sun2/bootsd ${LIBDIR}/sun2/installboot ${LIBDIR}/sun2/rawboot
	${RM} ${LIBDIR}/sun2/*.o ${LIBDIR}/sun2/core ${LIBDIR}/sun2/a.out
	${RM} ${LIBDIR}/sun3/bootxy ${LIBDIR}/sun3/bootsd ${LIBDIR}/sun3/installboot ${LIBDIR}/sun3/rawboot
	${RM} ${LIBDIR}/sun3/*.o ${LIBDIR}/sun3/core ${LIBDIR}/sun3/a.out
	${RM} ${LIBDIR}/sun4/bootxy ${LIBDIR}/sun4/bootsd ${LIBDIR}/sun4/installboot ${LIBDIR}/sun4/rawboot
	${RM} ${LIBDIR}/sun4/*.o ${LIBDIR}/sun4/core ${LIBDIR}/sun4/a.out

${LINTOBJ}:
	cd ${LIBDIR}; ${MAKE} -e ${MFLAGS} lint

lint: ${LINTOBJ} boot.c readfile.c
	@${RM} lint.ln
	@-(for i in readfile.c ; do \
		${CC} -E ${LCOPTS} $$i | \
		${LINT1} ${LOPTS} >> lint.ln; done ) 2>&1 | ${LTAIL}
	lint -Dlint ${CPPOPTS} ${LOPTS} boot.c lint.ln ${LINTOBJ} | ${LTAIL}
	@${RM} lint.ln

depend:
install_h:
FRC:

install: ${ALL}
	-mkdir ${DESTDIR}/tftpboot  && \
		chown bin ${DESTDIR}/tftpboot && \
		chmod 755 ${DESTDIR}/tftpboot
	if [ ${ARCH} = sun2 ]; then \
		install ${SUN2BB} ${DESTDIR}/tftpboot; \
	fi
	-mkdir ${DESTDIR}/usr/mdec && \
		chown bin ${DESTDIR}/usr/mdec && \
		chmod 755 ${DESTDIR}/usr/mdec
	install -c ${ARCHDEST}/bootxy ${DESTDIR}/usr/mdec/bootxy
	install -c ${ARCHDEST}/bootsd ${DESTDIR}/usr/mdec/bootsd
	install -c ${ARCHDEST}/bootsd ${DESTDIR}/usr/mdec/rawboot
	install -c -m 755 ${ARCHDEST}/installboot ${DESTDIR}/usr/mdec/installboot
