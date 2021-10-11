#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1991 by Sun Microsystems, Inc.

all: posix_fcntl.o

posix_fcntl.o:
	as -Isparc -sparc -P -DLOCORE -I/usr/5include -DS5EMUL -I/usr/src/include -I/usr/src/lib/libc/sys/common/sparc -P posix_fcntl.s -o posix_fcntl.o
