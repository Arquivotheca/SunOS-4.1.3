#	@(#)Makefile 1.1 92/07/30 SMI
#
# Sun-4 kernel debugger (kadb) makefile
#
CC=	cc
AS=	as
LD=	ld
AR=	ar
RM=	rm -f
TOUCH=	touch -cf
GREP=	egrep

MACHINES= -DSUN4_260 -DSUN4_110 -DSUN4_330 -DSUN4_470
CPPOPTS= -DKADB -Dsun4 -Dsparc -Umc68000 -DKERNEL -I. -I.. -I../.. $(MACHINES)
#CPPOPTS= -DKADB -Dsun4 -Dsparc -Umc68000 -DKERNEL -DSAS -I..
CFLAGS=	$(CPPOPTS)
ASFLAGS= -P $(CPPOPTS) -DLOCORE
DDIR=	/usr/src/sun/bin/adb

HFILES=\
	reg.h allregs.h asm_linkage.h

CFILES=\
	machdep.c\
	../../debug/main.c\
	../../debug/support.c

SFILES=\
	locore.s\
	map.s

SPECIAL=\
	locore.o

OBJ=\
	$(SPECIAL)\
	machdep.o\
	map.o\
	main.o\
	support.o

.INIT:	$(HFILES)

.KEEP_STATE:

debug.o: $(OBJ)
	$(LD) -r -o $@ $(OBJ)

assym.s: genassym.c
	$(CC) $(CFLAGS) -I. genassym.c
	-if [ `/bin/arch` = sun4 ] ; \
	then \
		./a.out >assym.s; \
	else \
		sas -R a.out >assym.s 2>/dev/null; \
	fi
	rm a.out

main.o: ../../debug/main.c
	$(CC) $(CFLAGS) -c ../../debug/main.c

support.o: ../../debug/support.c
	$(CC) $(CFLAGS) -c ../../debug/support.c

locore.o: assym.s

reloc:
	@echo 'FFC00000'

tags:	$(HFILES) $(CFILES) $(DDIR)/*.c $(DDIR)/*.h
	ctags $(HFILES) $(CFILES) $(DDIR)/*.c $(DDIR)/*.h

clean:
	$(RM) a.out *.o assym.s
