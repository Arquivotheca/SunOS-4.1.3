#
# 	@(#)Makefile 1.1 92/07/30 SMI
#

CC=/usr/5bin/cc
CFLAGS=-O -I/usr/include/posix -DS5EMUL -DPOSIX
LIB=libposix.a

ARCH-mc68020=	m68k
ARCH-sparc = sparc
ARCH= $(ARCH$(TARGET_MACH))

LIBDIR= $(DESTDIR)/usr/lib
INCDIR= $(DESTDIR)/usr/include/posix
INCSYSDIR= $(INCDIR)/sys

GENDIR=../lib/libc/gen/common
STDIODIR=../lib/libc/stdio/common
SYS5DIR=../lib/libc/stdio/sys5

GENFILES=	execvp.c\
		posix_sig.c 

STDFILES=	fgetc.c\
		fgets.c\
		scanf.c\
		fputs.c \
		fread.c \
		fwrite.c \
		getc.c \
		getchar.c \
		gets.c \
		putc.c

STD5FILES=	filbuf.c \
		flsbuf.c \
		fopen.c\
		fprintf.c\
		vfprintf.c \
		vprintf.c

.KEEP_STATE:

all: $(LIB) 

CFILES= cuserid.c \
	fcntl.c

INCFILES= fcntl.h

INCSYSFILES= fcntlcom.h

HDRS= 

OBJS1= $(CFILES:%.c=obj/%.o) 
OBJS2= $(GENFILES:%.c=obj/%.o)
OBJS3= $(STDFILES:%.c=obj/%.o)
OBJS4= $(STD5FILES:%.c=obj/%.o)
$(OBJS1):	obj fcntl.h fcntlcom.h
obj/%.o:	$(ARCH)/%.s
	cd $(ARCH); $(MAKE) $(MFLAGS) OBJDIR=../obj
obj/%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<
obj:
	test -d $@ || mkdir $@

$(GENFILES):
	(if [ ! -f $(GENDIR)/$@ ]; then cd $(GENDIR);sccs get $@; fi;)
	cd obj; ${CC} ${CFLAGS} -c  ../$(GENDIR)/$@

$(STDFILES):
	(if [ ! -f $(STDIODIR)/$@ ]; then cd $(STDIODIR);sccs get $@; fi;)
	cd obj; ${CC} ${CFLAGS} -c  ../$(STDIODIR)/$@

$(STD5FILES):
	(if [ ! -f $(SYS5DIR)/$@ ]; then cd $(SYS5DIR);sccs get $@; fi;)
	cd obj; ${CC} ${CFLAGS} -c  ../$(SYS5DIR)/$@

$(LIB): $(HDRS) $(OBJS1) $(GENFILES) $(STDFILES) $(STD5FILES)
	cd $(ARCH); make
	ar rv $(LIB) $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(ARCH)/*.o
	ranlib $(LIB)

install: all
	install -d -o bin -m 755 $(LIBDIR)
	install -m 644 $(LIB) $(LIBDIR)
	ranlib $(LIBDIR)/$(LIB)
	install -d -m 755 $(INCDIR)
	install -d -o bin -m 755 $(INCSYSDIR)
	install -m 444 $(INCFILES) $(INCDIR)
	install -m 444 $(INCSYSFILES) $(INCSYSDIR)

clean:
	rm -f $(OBJS1)
	rm -f $(LIB)
	rm -f obj/*.o

