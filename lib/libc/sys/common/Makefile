#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc.
#
all: xall						
include ../../Makefile.arch
# although the assembly is machine-dependent, this list is common to
# to both sparc and m68k -note def. of PSEUDO_SRCS must precede its
# use in $(ARCH)/Makefile
#
# All system calls that take pathnames have been split out into 4.2/sys5
# because POSIX(sys5) considers a null path a bug & bsd doesn't.
PSEUDO_SRCS = \
	accept.s adjtime.s async_daemon.s	\
	bind.s close.s connect.s dup.s dup2.s	\
	exportfs.s fchdir.s fchmod.s fchown.s fchroot.s fcntl.s flock.s	\
	fpathconf.s fstat.s fstatfs.s fsync.s ftruncate.s getdents.s	\
	getdomainname.s getdtablesize.s getfh.s getgid.s 		\
	gethostid.s gethostname.s getitimer.s getmsg.s getpagesize.s	\
	getpeername.s getpid.s getpriority.s getrlimit.s getrusage.s	\
	getsockname.s getsockopt.s gettimeofday.s getuid.s ioctl.s	\
	killpg.s listen.s mctl.s mincore.s mprotect.s munmap.s nfssvc.s	\
	poll.s profil.s putmsg.s read.s readv.s recv.s recvfrom.s	\
	recvmsg.s select.s send.s sendmsg.s sendto.s setdomainname.s	\
	sethostname.s setitimer.s setpgid.s setpriority.s		\
	setregid.s setreuid.s setrlimit.s setsockopt.s		\
	settimeofday.s shutdown.s sigblock.s sigpause.s sigpending.s	\
	sigsetmask.s sigstack.s socket.s socketpair.s statfs.s sync.s	\
	sysconf.s umask.s uname.s ustat.s vhangup.s vtrace.s wait4.s	\
	write.s writev.s


include $(ARCH)/Makefile
include ../../Makefile.master
MID_SRC = auditsys.c execl.c execle.c execv.c mkfifo.c msgsys.c pathconf.c \
	rfssys.c semsys.c setsid.c shmsys.c sigvec.c wait.c wait3.c

OBJS = $(MID_SRC:%.c=$(VARIANT)/%.o) $(PSEUDO_SRCS:%.s=$(VARIANT)/%.o) $(MD_ASSRC:$(ARCH)/%.s=$(VARIANT)/%.o) $(MD_CSRC:%.c=$(VARIANT)/%.o) $(SHLIB_PSEUDO:%.s=$(VARIANT)/%.o)

xall: $$(LIBS)
$(LIBS) : symlink $$(VARIANT) $$(OBJS)
mkfifo.c pathconf.c: ../common/chkpath.h

libc_comm.so := SHLIB_PSEUDO = aiocancel.s aioread.s aiowait.s aiowrite.s

symlink:
	rm -rf strings;\
	ln -s ../../strings strings

.INIT: $(MD_HDRS)

clean: master.clean




