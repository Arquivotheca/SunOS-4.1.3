#
# 	@(#)Makefile 1.1 92/07/30 SMI
#

CC=/usr/5bin/cc
CFLAGS=-I../xpginclude -O
LIB=libxpg.a
LIBP=libxpg_p.a

#libxpg_p.a	:= CFLAGS += -DPROF

ARCH-mc68020=	m68k
ARCH-sparc = sparc
ARCH= $(ARCH$(TARGET_MACH))

LIBDIR= $(DESTDIR)/usr/xpg2lib

.KEEP_STATE:

all: $(LIB) $(LIBP)

CFILES= chroot.c catgets.c getcwd.c getut.c l3tol.c logname.c \
	malloc.c swab.c langinfo.c gettext.c 

SFILES= $(ARCH)/sbrk.s
HDRS=  mallint.h
OBJS1= $(CFILES:%.c=obj/%.o)
OBJS2= $(SFILES:$(ARCH)/%.s=obj/%.o)
$(OBJS1):	obj
$(OBJS2):	obj
obj/%.o:	$(ARCH)/%.s
	cd $(ARCH); $(MAKE) $(MFLAGS) OBJDIR=../obj
obj/%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<
obj:
	test -d $@ || mkdir $@




$(LIB): $(HDRS) $(OBJS1) $(OBJS2)
	ar rv $(LIB) $(OBJS1)
	ar rv $(LIB) $(OBJS2)
	ranlib $(LIB)

# profiled xopen library
POBJS1= $(CFILES:%.c=obj_p/%.o)
POBJS2= $(SFILES:$(ARCH)/%.s=obj_p/%.o)
$(POBJS1):	obj_p
$(POBJS2):	obj_p
obj_p/%.o:	$(ARCH)/%.s
	cd $(ARCH); $(MAKE) $(MFLAGS) OBJDIR=../obj_p CPPFLAGS=-DPROF
obj_p/%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ -p $<
obj_p:
	test -d $@ || mkdir $@


$(LIBP): $(POBJS1) $(POBJS2)
	$(AR) rv $(LIBP) $(POBJS1)
	$(AR) rv $(LIBP) $(POBJS2)
	ranlib $(LIBP)

install: all
	install -d -o bin -m 755 $(LIBDIR)
	install -m 644 $(LIB) $(LIBDIR)
	install -m 644 $(LIBP) $(LIBDIR)
	ranlib $(LIBDIR)/$(LIB)
	ranlib $(LIBDIR)/$(LIBP)

clean:
	rm -f $(OBJS1)
	rm -f $(OBJS2)
	rm -f $(POBJS1)
	rm -f $(POBJS2)
	rm -f $(LIB) $(LIBP)

