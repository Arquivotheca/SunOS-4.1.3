# @(#)Makefile 1.1 92/07/30

DESTDIR=
CFLAGS += -O
CPPFLAGS += -DUSE_ASM
OBJS = crc.o addbfcrc$(TARGET_MACH).o

.KEEP_STATE:

all: crc

crc: $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS)

install: crc
	install $@ $(DESTDIR)

clean:
	$(RM) $(OBJS) core crc
