
#
# @(#)Makefile 1.100 90/11/01 SMI; from UCB 4.38 85/08/26
#
# Makefile /usr/src/sundist/sunupgrade/tools

CFLAGS=	-O $(IFLAGS)

LDFLAGS=
CC= /bin/cc
S5CC= /usr/5bin/cc
CHOWN=	/usr/etc/chown
RPCCOM=rpcgen
SUBDIR=egrep ldconfig sh nawk xdrtoc ls5
# This destdir definition is for testing only
# DESTDIR=/usr/src/sundist/sunupgrade/tools
#
CLEANFILES= cat cat.c chmod chmod.c chown chown.c comm \
comm.c cp cp.c cut cut.c echo echo.c \
ed ed.c grep grep.c kill kill.c ln \
ln.c ls ls.c mkdir mkdir.c more more.c \
mount mount.c mv mv.c pwd pwd.c rm rm.c \
sed sed.h sed0.c sed0.o sed1.c sed1.o sort sort.c \
sum sum.c sync sync.c syslogd syslogd.c tar tar.c \
tee tee.c tr* tr.c newls

# files found in usr.bin -----------------------------------------------
#
USRBINPATH=	../../../usr.bin

USRBINFILES=cat chmod comm cut echo grep ln ls sum tr
YAC=
# NSTD=expr sort
NSTD=sort 
# ----------------------------------------------------------------------


# files found in etc --------------------------------------------------
#
ETCPATH=../../../etc
ETCFILES= ${STDETCFILES} ${NSTDETCFILES}
STDETCFILES=chown syslogd
NSTDETCFILES=mount
#----------------------------------------------------------------------

# files found in bin -------------------------------------------------
#
BINPATH=../../../bin
BINFILES=${STDBINFILES} ${NSTDBINFILES}
STDBINFILES=cp kill mkdir mv pwd rm sync tar tee
NSTDBINFILES=ed
# NSTDBINFILES=
#----------------------------------------------------------------------


#
# files found in ucb
UCBPATH=../../../ucb/more
UCBFILES=more
#----------------------------------------------------------------------


# sed -----------------------------------------------------------------
#
SEDPATH=../../../5bin/sed
SEDFILES=sed.h sed0.c sed1.c
#---------------------------------------------------------------------

TOOLS= sed ${UCBFILES} ${STDBINFILES} ${NSTDBINFILES} ${ETCFILES} \
	${NSTD}  ${USRBINFILES}

.KEEP_STATE:

# Programs built "static" for upgrade toolset
#

# all:	getfiles static	nonstd

all: ${ETCFILES} ${STATIC} ${NSTD} ${BINFILES} ${UCBFILES} \
		sed ${USRBINFILES} ${SUBDIR} newls


STATIC=${USRBINFILES} ${STDBINFILES} ${STDETCFILES}

static:	${STATIC}

nonstd: ${NSTD}  sed ${NSTDETCFILES} ${NSTDBINFILES} more

# get files from source directories ------------------------------------

${USRBINFILES:%=%.c}:	
	cp ${USRBINPATH}/$@ .


${BINFILES:%=%.c}:	
	cp ${BINPATH}/$@ .

${ETCFILES:%=%.c}:
	cp ${ETCPATH}/$@ .

${UCBFILES:%=%.c}:	
	cp ${UCBPATH}/$@ .


${NSTD:%=%.c}:
	cp ${USRBINPATH}/$@ .

${SEDFILES}:
	cp ${SEDPATH}/$@ .

# compile them ----------------------------------------------------------------

${STATIC}:	
	${CC} ${LDFLAGS} -Bstatic -o $@ $@.c 

sort:
	${CC} -o sort ${CFLAGS} ${LDFLAGS} -Bstatic -DPRE_S5R2 sort.c


ed:
	${CC} ${CFLAGS} -Bstatic -o ed ed.c

mount:
	${CC} ${CFLAGS} -Bstatic -o mount mount.c -lrpcsvc

more:
	${CC} -o more -Bstatic more.c -lcurses -ltermcap


# compile sed -------------------------------------------------
SFILES= sed0.o sed1.o

sed:	$(SFILES)
	$(CC) -o sed $(SFILES) -Bstatic 

sed0.o: sed0.c sed.h
	$(CC) -c $(CFLAGS) -Bstatic  sed0.c

sed1.o: sed1.c sed.h
	$(CC) -c $(CFLAGS) -Bstatic  sed1.c

# compile newls -----------------------------------------------
newls:
	$(CC) -o newls -Bstatic newls.c

#-----------------------------------------------------------------

${SUBDIR}: FRC
	cd $@; ${MAKE} ${MFLAGS} all
 
#-----------------------------------------------------------------

install: 
	install -d -m 755 ${DESTDIR}/toolkit
	for i in $(TOOLS); do \
		install -s -m 755 $$i ${DESTDIR}/toolkit/$$i; done

	for i in ${SUBDIR}; \
		do (cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} \
	install); done

	install -s -m 755 newls ${DESTDIR}/toolkit/newls

clean:
	rm -f ${CLEANFILES}
	rm -fr ${DESTDIR}/toolkit
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done


FRC:
