#
# @(#)Makefile 1.1 92/07/30 SMI
#
CHOWN=/etc/chown

HDIR=$(DESTDIR)/usr/include/sys

HFILES= acct.h asynch.h audit.h bk.h bootconf.h buf.h callout.h clist.h \
	conf.h core.h debug.h des.h dir.h dirent.h dk.h dkbad.h dnlc.h \
	domain.h dumphdr.h errno.h exec.h fcntl.h fcntlcom.h filai.h file.h \
	filio.h gprof.h ieeefp.h init_audit.h ioccom.h ioctl.h ipc.h \
	kernel.h kmem_alloc.h label.h limits.h lock.h lockfs.h map.h \
	mbuf.h mman.h mount.h msg.h msgbuf.h mtio.h \
	param.h pathconf.h pathname.h poll.h proc.h protosw.h ptrace.h \
	ptyvar.h reboot.h resource.h \
	sem.h session.h shm.h signal.h socket.h socketvar.h sockio.h \
	stdtypes.h stat.h stream.h stropts.h strstat.h \
	syscall.h syslog.h sysmacros.h systm.h \
	termio.h termios.h time.h timeb.h times.h trace.h \
	ttold.h tty.h ttychars.h ttycom.h ttydev.h types.h \
	ucred.h uio.h un.h unistd.h unpcb.h user.h utsname.h vaccess.h \
	vadvise.h var.h varargs.h vcmd.h vfs.h vfs_stat.h vlimit.h \
	vm.h vmmac.h vmmeter.h vmparam.h vmsystm.h vnode.h vtimes.h \
	wait.h

all:

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:
