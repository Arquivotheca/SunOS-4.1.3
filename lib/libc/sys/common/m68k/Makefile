#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#

CPPFLAGS$(TARGET_MACH) = -I../../crt/$(ARCH) -I$(ARCH) 

MD_HDRS = $(ARCH)/SYS.h $(ARCH)/PIC.h
ASSRC = Ovfork.s _exit.s brk.s cerror.s errno.s \
	fork.s getegid.s geteuid.s getppid.s mmap.s\
	pipe.s ptrace.s reboot.s sbrk.s \
	sigtramp.s _sigvec.s syscall.s vadvise.s
MD_ASSRC = $(ASSRC:%.s=$(ARCH)/%.s)

$(PSEUDO_SRCS:%.s=obj/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:obj/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s

$(PSEUDO_SRCS:%.s=obj_p/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:obj_p/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s

$(PSEUDO_SRCS:%.s=shared/%.o) shared/aiocancel.o shared/aioread.o shared/aiowrite.o shared/aiowait.o:
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:shared/%.o=%))'; \
	echo 'RET') | $(CPP)  $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s

$(PSEUDO_SRCS:%.s=s5/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:s5/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s

$(PSEUDO_SRCS:%.s=s5_p/%.o):
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:s5_p/%.o=%))'; \
	echo 'RET') | $(CPP) $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s

$(PSEUDO_SRCS:%.s=shared_s5/%.o) shared_s5/aiocancel.o shared_s5/aioread.o shared_s5/aiowrite.o shared_s5/aiowait.o:
	@(echo '#include "SYS.h"'; \
	echo 'SYSCALL($(@:shared_s5/%.o=%))'; \
	echo 'RET') | $(CPP)  $(CPPFLAGS) $(CPPFLAGS$(TARGET_MACH)) >tmp.s
	$(AS) $(ASFLAGS) $(ASFLAGS$(TARGET_MACH)) tmp.s -o $@
	@rm -f tmp.s
