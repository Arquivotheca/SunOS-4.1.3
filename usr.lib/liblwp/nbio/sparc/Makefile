#
# 	Makefile	
#       @(#) Makefile 1.1 92/07/30 Copyr 1987 Sun Micro
#
CFLAGS = -O
AR= /bin/ar
RANLIB=  /usr/bin/ranlib
INSTALL= /usr/bin/install
CC= /bin/cc
d-mc68020=m68k
d-mc68010=m68k
d-sparc=sparc
MACH= $(d$(TARGET_MACH))
PIPEDIR= /usr/src/lib/libc/sys/common/$(MACH)
ASSRC= $(PIPEDIR)/pipe.s
LOPTS=	-hbxn
GREP= /bin/grep
LTAIL=  ${GREP} -v 'struct/union .* never defined' | \
	${GREP} -v 'syscall' | \
	${GREP} -v 'possible pointer alignment problem' ; true

libnbio.a: nb.o mpipe.o
	$(AR) cru libnbio.a $? 
	$(RANLIB) $@

install: FRC libnbio.a
	$(INSTALL) -c -m 644 libnbio.a $(DESTDIR)/usr/lib
	$(RANLIB) $(DESTDIR)/usr/lib/libnbio.a

nb.o: ../nb.c
	$(CC) $(CFLAGS) -c $?

mpipe.o: $$(ASSRC) $(PIPEDIR)/SYS.h $(PIPEDIR)/PIC.h
	cp $(ASSRC) /tmp/tmppipe.s
	/lib/cpp -E -I$(PIPEDIR) /tmp/tmppipe.s > /tmp/cpptmppipe.s
	sed                                     \
                -e 's/pipe/mpipe/g'             \
                -e 's/SYS_mpipe/SYS_pipe/'      \
                /tmp/cpptmppipe.s > mpipe.s
	/bin/as -o $@ mpipe.s
	/bin/rm -f /tmp/cpptmppipe.s /tmp/tmppipe.s

tags: ../nb.c
	ctags -tw $?

.IGNORE:
lint:
	lint -vu $(LOPTS) ../nb.c -llwp | $(LTAIL) > linted

clean:
	rm -f libnbio.a linted made *.o tags *.s

FRC:
