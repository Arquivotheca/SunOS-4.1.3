#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX
#
DESTDIR=
CFLAGS=	-O
ARCH:sh = arch -k
#
# Programs that live in subdirectories, and have makefiles of their own.
#
INSTALL_DIRS= h sun2 sun3 sun3x sun4
SUN2DIR= SUN2 SUN1
SUN3DIR= CARRERA M25 SIRIUS PRISM SUN3F
SUN4DIR= SUNRISE

SUBDIR=	${SUN3DIR} ${SUN2DIR}

all:    ${SUBDIR}

${SUBDIR}: FRC
	@case ${ARCH} in \
        sun2) for i in ${SUN2DIR}; do (cd $$i; make ${MFLAGS} ); done ;; \
        sun3) for i in ${SUN3DIR}; do (cd $$i; make ${MFLAGS} ); done ;; \
        sun4) for i in ${SUN4DIR}; do (cd $$i; make ${MFLAGS} ); done ;; \
        esac

# clean the build directories
clean:
	@case ${ARCH} in \
        sun2) for i in ${SUN2DIR}; do (cd $$i; make ${MFLAGS} clean ); done ;; \
        sun3) for i in ${SUN3DIR}; do (cd $$i; make ${MFLAGS} clean ); done ;; \
        sun4) for i in ${SUN4DIR}; do (cd $$i; make ${MFLAGS} clean ); done ;; \
        esac

# this used to be done by 'make clean'....would anyone really want to do this?
clobber:
	@cd ${ARCH} ;sccs clean
	@cd sys  ;sccs clean
	@cd dev  ;sccs clean
	@cd diag ;sccs clean
	@cd h    ;sccs clean

# does an sccs check of all source files and Makefiles
check:
	cd sun2 ;sccs info
	cd sun3 ;sccs info
	cd sys  ;sccs info
	cd dev  ;sccs info
	cd diag ;sccs info
	cd SUN1 ;sccs info
	cd SUN2 ;sccs info
	for i in ${SUN3DIR}; do (cd $$i; sccs info); done

# make a test copy of mon's SCCS files
monTST:
	rm -r -f /usr/src/sun/SCCS_DIRECTORIES/sys/stand/monTST
	rcp -r argon:/usr/src/SCCS_DIRECTORIES/sys/stand/mon \
                     /usr/src/sun/SCCS_DIRECTORIES/sys/stand/monTST

# mounts to argon's SCCS directory
argonMNT:
	mount argon:/usr/src/SCCS_DIRECTORIES/sys/stand/mon \
                    /usr/src/sun/SCCS_DIRECTORIES/sys/stand/argonMNT

# saves a copy of argon's SCCS files
argonSAV:
	rm -r -f /usr/src/sun/SCCS_DIRECTORIES/sys/stand/argonSAV
	rcp -r argon:/usr/src/SCCS_DIRECTORIES/sys/stand/mon \
                     /usr/src/sun/SCCS_DIRECTORIES/sys/stand/argonSAV

# saves a copy of mon's SCCS files
monSAV:
	rm -r -f /usr/src/sun/SCCS_DIRECTORIES/sys/stand/monSAV
	cp -r /usr/src/sun/SCCS_DIRECTORIES/sys/stand/mon \
              /usr/src/sun/SCCS_DIRECTORIES/sys/stand/monSAV

# recopy image from argon
image:
	rm -r -f /usr/src/sun/SCCS_DIRECTORIES/sys/stand/mon
	rcp -r argon:/usr/src/SCCS_DIRECTORIES/sys/stand/mon \
                     /usr/src/sun/SCCS_DIRECTORIES/sys/stand/mon
	
install_h: FRC
	@set -x;for i in ${INSTALL_DIRS};  \
		do (cd $$i; make ${MFLAGS} install_h DESTDIR=$(DESTDIR) ); done

FRC:
