#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
all : xall

include ../../Makefile.arch
include $(ARCH)/Makefile
include ../../Makefile.master

HDRS = stdiom.h iob.h doprnt.h
COMMSRC = clrerr.c ctermid.c cuserid.c doprnt.c \
	fdopen.c feof.c ferror.c fgetc.c fgets.c \
	fileno.c findiop.c \
	fputc.c fputs.c fread.c fseek.c ftell.c fwrite.c \
	getc.c getchar.c gets.c getw.c iob.c putchar.c \
	putc.c  puts.c \
	putw.c remove.c rew.c scanf.c setbuffer.c setvbuf.c \
	tempnam.c tmpnam.c tmpfile.c ungetc.c
S5SRC = doscan.c 
4.2SRC = doscan.c

libc_comm.so := SRC = $(COMMSRC)
libc.so := SRC = $(4.2SRC)
libc.a libc_p.a := SRC = $(4.2SRC) $(COMMSRC)
libcs5.so := SRC = $(S5SRC) 
libcs5.a libcs5_p.a := SRC = $(S5SRC) $(COMMSRC)

OBJS = $(SRC:%.c=$(VARIANT)/%.o) $(MD_CSRC:$(ARCH)/%.c=$(VARIANT)/%.o) $(MD_ASSRC:$(ARCH)/%.s=$(VARIANT)/%.o)

obj/ctermid.o obj_p/ctermid.o shared/ctermid.o \
s5/ctermid.o s5_p/ctermid.o := CFLAGS += -A-R

obj/doprnt.o obj_p/doprnt.o shared/doprnt.o \
s5/doprnt.o s5_p/doprnt.o  := CFLAGS += -A

obj/doscan.o obj_p/doscan.o shared_4.2/doscan.o \
s5/doscan.o s5_p/doscan.o shared_s5/doscan.o := CFLAGS += -A-R

obj/tempnam.o obj_p/tempnam.o shared/tempnam.o \
s5/tempnam.o s5_p/tempnam.o  := CFLAGS += -A-R

obj/tmpfile.o obj_p/tmpfile.o shared/tmpfile.o \
s5/tmpfile.o s5_p/tmpfile.o := CFLAGS += -A-R

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

.INIT : $(HDRS) $(MD_HDRS)

clean: master.clean
