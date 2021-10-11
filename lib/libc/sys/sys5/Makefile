#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

SRC =   access.c acct.c  chdir.c chmod.c chown.c chroot.c creat.c execve.c \
	getgroups.c getpgrp.c kill.c link.c lseek.c lstat.c mkdir.c mknod.c \
	mount.c open.c quotactl.c readlink.c rename.c rmdir.c setgroups.c \
	setpgrp.c stat.c stime.c swapon.c symlink.c truncate.c unlink.c \
	unmount.c utimes.c

OBJS=	$(SRC:%.c=$(VARIANT)/%.o) $(MD_ASSRC:$(ARCH)/%.s=$(VARIANT)/%.o)

all: xall

include ../../Makefile.arch
include $(ARCH)/Makefile
include ../../Makefile.master

.KEEP_STATE:

xall: $$(LIBS)
$(LIBS): symlink $$(VARIANT) $$(OBJS)
#
# XXX - This is so that chkpath.h will be gotten before the compile.  
#
$(SRC): ../common/chkpath.h

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

clean: master.clean

.INIT: $(MD_HDRS)
