#
# @(#)Makefile 1.1 92/07/30 SMI
#
ARCH:sh = arch -k
IDENT=-sparc -D$(ARCH) -DGENERIC -DSUN4C_60 -DVDDRV -DCRYPT -DTCPDEBUG -DIPCSHMEM -DIPCSEMAPHORE -DIPCMESSAGE -DSYSAUDIT -DSYSACCT -DLOFS -DNFSSERVER -DNFSCLIENT -DUFS -DQUOTA -DINET -DKERNEL

CG8OPTS= -DLOADABLE
COPTS=  ${IDENT} ${CG8OPTS}
CFLAGS= -O ${COPTS}

ALLsun4c = $(BIN)
ALLsun4m = $(BIN)

INSTsun4c = $(ALLsun4c) cg8_exec.script install_lddir
INSTsun4m = $(ALLsun4m) cg8_exec.script install_lddir

BIN = cg8.o
SRC = cgeight.c cg8_wrapper.c
OBJ = ${SRC:.c=.o}

DESTDIR =
LDDIR = /usr/lddrv/cg8

.KEEP_STATE:

all:	$(ALL$(ARCH))

cg8.o: $(OBJ)
	$(LD) -o $@ -r $(OBJ)

load: cg8.o
	modload cg8.o -exec cg8_exec.script

install: $(INST$(ARCH))

install_lddir:
	-set +e ; \
	install -d -m 2755 $(DESTDIR)$(LDDIR); \
	install -c -m 555 cg8.o $(DESTDIR)$(LDDIR)/cg8.o; \
	install -c -m 555 cg8_exec.script $(DESTDIR)$(LDDIR)/cg8_exec.script

clean:
	rm -f *.o
