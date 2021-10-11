#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
CPPFLAGS += -I../common/$(ARCH)
MD_HDRS = ../common/$(ARCH)/SYS.h ../common/$(ARCH)/PIC.h
MD_PSEUDO_OBJS = \
	acct.o access.o chdir.o chmod.o chown.o chroot.o creat.o execve.o \
	getgroups.o getpgrp.o kill.o link.o lseek.o lstat.o mkdir.o mknod.o \
	mount.o open.o quotactl.o readlink.o rename.o rmdir.o setgroups.o \
	setpgrp.o stat.o swapon.o symlink.o truncate.o unlink.o unmount.o \
	utimes.o

$(MD_PSEUDO_OBJS:%.o=obj/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:obj/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) >tmp.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) tmp.s -o $@
	@rm -f tmp.s

$(MD_PSEUDO_OBJS:%.o=obj_p/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:obj_p/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) >tmp.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) tmp.s -o $@
	@rm -f tmp.s

$(MD_PSEUDO_OBJS:%.o=shared_4.2/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:shared_4.2/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) >tmp.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) tmp.s -o $@
	@rm -f tmp.s

$(MD_PSEUDO_OBJS:%.o=shared_p/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:shared_p/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) >tmp.s
	$(AS) $(ASFLAGS$(TARGET_MACH)) $(ASFLAGS) tmp.s -o $@
	@rm -f tmp.s
