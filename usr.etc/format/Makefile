#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

#
# Architecture-specific programs
#
ASP= format

#
# Install directory for architecture-specific programs, absolute path
#
ASDIR= /usr/kvm

#
# Install directory for architecture-specific programs, relative to /usr/etc
#
RELASDIR= ../kvm

INCLUDE = -I../../sys -I../../sys/sys
CC = cc $(INCLUDE) -O
OBJS= ctlr_acb4000.o init_ctypes.o io.o label.o \
	main.o ctlr_md21.o menu.o init_menus.o misc.o startup.o \
	menu_command.o menu_partition.o menu_analyze.o menu_defect.o \
	ctlr_xy450.o ctlr_xd7053.o ctlr_id.o defect.o partition.o \
	analyze.o ctlr_scsi.o
SRCS= $(OBJS:.o=.c)
HDRS= analyze.h defect.h global.h \
	hardware_structs.h io.h menu.h misc.h param.h \
	scsi_com.h startup.h

.KEEP_STATE:

.INIT: $(HDRS)

all: $(ASP)

$(ASP): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(ASP)
	install -d -o bin -m 755 $(DESTDIR)$(ASDIR)
	@set -x; for i in $(ASP); do (install -s $$i $(DESTDIR)$(ASDIR)/$$i; \
		rm -f $(DESTDIR)/usr/etc/$$i; \
		/usr/bin/ln -s $(RELASDIR)/$$i $(DESTDIR)/usr/etc/$$i); \
	done

clean:
	$(RM) $(ASP) $(OBJS)

lint: $(SRCS)
	$(LINT.c) -I$(INCLUDE) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)
