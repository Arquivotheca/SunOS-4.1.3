#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)Makefile 1.1 92/07/30 SMI; from S5R3.1 1.12
#
#	Curses Library High Level Makefile.
#
#	To INSTALL libcurses.a, the tic compiler and the tools type:
#
#		"make install"
#
#
#	To COMPILE libcurses.a, the tic compiler and the tools, type:
#
#		"make all"
#
#
#	To compile a particular file with normal compilation type:
#
#		"make FILES='<particular .o files>"
#
#
#	If debugging is desired then type:
#
#		"make O=debug FILES='<particular .o files>"
#
#
#	If tracing is desired then type:
#
#		"make O=trace FILES='<particular .o files>"
#
#
#	If profiling is desired then type:
#
#		"make O=profile FILES='<particular .o files>"
#
#
#	To compile only the tic compiler type:
#
#		"make tic"
#
#
SHELL=/bin/sh
MAKE=make CC=$(CC)
USRLIB = $(DESTDIR)/usr/5lib
CC = /usr/bin/cc

all:
	@cd ./screen ; $(MAKE) cktmp
	@cd ./screen ; $(MAKE)

libcurses.a:
	#@cd ./screen ; $(MAKE) cktmp
	@cd ./screen ; $(MAKE) libcurses.a
	@echo
	@echo "Libcurses.a has been made."
	@echo

tools:
	@cd ./screen ; $(MAKE) tools
	@cd ./screen ; $(MAKE) llib-lcurses.ln
	@echo
	@echo "Libcurses/Terminfo tools have been made."
	@echo

tic:
	@cd ./screen ; $(MAKE) tic
	@echo
	@echo "The tic compiler has been made."
	@echo
	
install:
	# make and install libcurses.a and tic
	@cd ./screen ; $(MAKE) cktmp
	@cd ./screen ; $(MAKE) install USRLIB=$(USRLIB) CC2=$(CC) ROOT2=""
	@echo
	@echo libcurses.a, the tic compiler, and associated tools have
	@echo been installed.
	@echo

install_h:
	@cd ./screen ; $(MAKE) install_h

clean:
	@cd ./screen ; $(MAKE) clean
