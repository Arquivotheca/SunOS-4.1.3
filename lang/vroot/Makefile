#
#	@(#)Makefile 1.1 92/07/30 Copyright (c) 1986 Sun Microsystems, Inc.
#
#	Build all the vroot sys calls
#

CPP	= /lib/cpp
CPP_FLAGS = -I. -DSUNPRO
COPTS	= -O
CFLAGS	= $(COPTS) $(CPP_FLAGS)
HDRS	= args.h report.h vroot.h

SRCS	= access.c chdir.c chmod.c chown.c chroot.c creat.c execve.c lock.c \
	lstat.c mkdir.c mount.c open.c readlink.c report.c rmdir.c stat.c \
	statfs.c truncate.c unlink.c unmount.c utimes.c vroot.c setenv.c

OBJS	= $(SRCS:.c=.o)

.DEFAULT:
	sccs get -G$@ $@

vroot.a: $(OBJS)
	ar rv $@ $(OBJS)
	ranlib $@

$(OBJS):
	$(CC) -c $(CFLAGS) $*.c

FRC:

clean:
	rm -f $(OBJS) vroot.a
install:
	echo Nothing to install here

depend: $(SRCS) $(HDRS) FRC
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPP_FLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

access.o: access.c
access.o: ./vroot.h
access.o: /usr/include/stdio.h
access.o: ./args.h
access.o: /usr/include/sys/syscall.h
access.o: /usr/include/errno.h
access.o: /usr/include/sys/errno.h
access.o: /usr/include/sys/time.h
access.o: /usr/include/time.h
access.o: /usr/include/sys/param.h
access.o: /usr/include/machine/param.h
access.o: /usr/include/signal.h
access.o: /usr/include/sys/types.h
access.o: /usr/include/sys/sysmacros.h
access.o: /usr/include/stdio.h
access.o: /usr/include/fcntl.h
access.o: /usr/include/sys/types.h
access.o: /usr/include/sys/stat.h
access.o: /usr/include/sys/file.h
access.o: /usr/include/sys/fcntl.h
chdir.o: chdir.c
chdir.o: ./vroot.h
chdir.o: /usr/include/stdio.h
chdir.o: ./args.h
chdir.o: /usr/include/sys/syscall.h
chdir.o: /usr/include/errno.h
chdir.o: /usr/include/sys/errno.h
chdir.o: /usr/include/sys/time.h
chdir.o: /usr/include/time.h
chdir.o: /usr/include/sys/param.h
chdir.o: /usr/include/machine/param.h
chdir.o: /usr/include/signal.h
chdir.o: /usr/include/sys/types.h
chdir.o: /usr/include/sys/sysmacros.h
chdir.o: /usr/include/stdio.h
chdir.o: /usr/include/fcntl.h
chdir.o: /usr/include/sys/types.h
chdir.o: /usr/include/sys/stat.h
chdir.o: /usr/include/sys/file.h
chdir.o: /usr/include/sys/fcntl.h
chmod.o: chmod.c
chmod.o: ./vroot.h
chmod.o: /usr/include/stdio.h
chmod.o: ./args.h
chmod.o: /usr/include/sys/syscall.h
chmod.o: /usr/include/errno.h
chmod.o: /usr/include/sys/errno.h
chmod.o: /usr/include/sys/time.h
chmod.o: /usr/include/time.h
chmod.o: /usr/include/sys/param.h
chmod.o: /usr/include/machine/param.h
chmod.o: /usr/include/signal.h
chmod.o: /usr/include/sys/types.h
chmod.o: /usr/include/sys/sysmacros.h
chmod.o: /usr/include/stdio.h
chmod.o: /usr/include/fcntl.h
chmod.o: /usr/include/sys/types.h
chmod.o: /usr/include/sys/stat.h
chmod.o: /usr/include/sys/file.h
chmod.o: /usr/include/sys/fcntl.h
chown.o: chown.c
chown.o: ./vroot.h
chown.o: /usr/include/stdio.h
chown.o: ./args.h
chown.o: /usr/include/sys/syscall.h
chown.o: /usr/include/errno.h
chown.o: /usr/include/sys/errno.h
chown.o: /usr/include/sys/time.h
chown.o: /usr/include/time.h
chown.o: /usr/include/sys/param.h
chown.o: /usr/include/machine/param.h
chown.o: /usr/include/signal.h
chown.o: /usr/include/sys/types.h
chown.o: /usr/include/sys/sysmacros.h
chown.o: /usr/include/stdio.h
chown.o: /usr/include/fcntl.h
chown.o: /usr/include/sys/types.h
chown.o: /usr/include/sys/stat.h
chown.o: /usr/include/sys/file.h
chown.o: /usr/include/sys/fcntl.h
chroot.o: chroot.c
chroot.o: ./vroot.h
chroot.o: /usr/include/stdio.h
chroot.o: ./args.h
chroot.o: /usr/include/sys/syscall.h
chroot.o: /usr/include/errno.h
chroot.o: /usr/include/sys/errno.h
chroot.o: /usr/include/sys/time.h
chroot.o: /usr/include/time.h
chroot.o: /usr/include/sys/param.h
chroot.o: /usr/include/machine/param.h
chroot.o: /usr/include/signal.h
chroot.o: /usr/include/sys/types.h
chroot.o: /usr/include/sys/sysmacros.h
chroot.o: /usr/include/stdio.h
chroot.o: /usr/include/fcntl.h
chroot.o: /usr/include/sys/types.h
chroot.o: /usr/include/sys/stat.h
chroot.o: /usr/include/sys/file.h
chroot.o: /usr/include/sys/fcntl.h
creat.o: creat.c
creat.o: ./vroot.h
creat.o: /usr/include/stdio.h
creat.o: ./args.h
creat.o: /usr/include/sys/syscall.h
creat.o: /usr/include/errno.h
creat.o: /usr/include/sys/errno.h
creat.o: /usr/include/sys/time.h
creat.o: /usr/include/time.h
creat.o: /usr/include/sys/param.h
creat.o: /usr/include/machine/param.h
creat.o: /usr/include/signal.h
creat.o: /usr/include/sys/types.h
creat.o: /usr/include/sys/sysmacros.h
creat.o: /usr/include/stdio.h
creat.o: /usr/include/fcntl.h
creat.o: /usr/include/sys/types.h
creat.o: /usr/include/sys/stat.h
creat.o: /usr/include/sys/file.h
creat.o: /usr/include/sys/fcntl.h
execve.o: execve.c
execve.o: ./vroot.h
execve.o: /usr/include/stdio.h
execve.o: ./args.h
execve.o: /usr/include/sys/syscall.h
execve.o: /usr/include/errno.h
execve.o: /usr/include/sys/errno.h
execve.o: /usr/include/sys/time.h
execve.o: /usr/include/time.h
execve.o: /usr/include/sys/param.h
execve.o: /usr/include/machine/param.h
execve.o: /usr/include/signal.h
execve.o: /usr/include/sys/types.h
execve.o: /usr/include/sys/sysmacros.h
execve.o: /usr/include/stdio.h
execve.o: /usr/include/fcntl.h
execve.o: /usr/include/sys/types.h
execve.o: /usr/include/sys/stat.h
execve.o: /usr/include/sys/file.h
execve.o: /usr/include/sys/fcntl.h
lstat.o: lstat.c
lstat.o: ./vroot.h
lstat.o: /usr/include/stdio.h
lstat.o: ./args.h
lstat.o: /usr/include/sys/syscall.h
lstat.o: /usr/include/errno.h
lstat.o: /usr/include/sys/errno.h
lstat.o: /usr/include/sys/time.h
lstat.o: /usr/include/time.h
lstat.o: /usr/include/sys/param.h
lstat.o: /usr/include/machine/param.h
lstat.o: /usr/include/signal.h
lstat.o: /usr/include/sys/types.h
lstat.o: /usr/include/sys/sysmacros.h
lstat.o: /usr/include/stdio.h
lstat.o: /usr/include/fcntl.h
lstat.o: /usr/include/sys/types.h
lstat.o: /usr/include/sys/stat.h
lstat.o: /usr/include/sys/file.h
lstat.o: /usr/include/sys/fcntl.h
mkdir.o: mkdir.c
mkdir.o: ./vroot.h
mkdir.o: /usr/include/stdio.h
mkdir.o: ./args.h
mkdir.o: /usr/include/sys/syscall.h
mkdir.o: /usr/include/errno.h
mkdir.o: /usr/include/sys/errno.h
mkdir.o: /usr/include/sys/time.h
mkdir.o: /usr/include/time.h
mkdir.o: /usr/include/sys/param.h
mkdir.o: /usr/include/machine/param.h
mkdir.o: /usr/include/signal.h
mkdir.o: /usr/include/sys/types.h
mkdir.o: /usr/include/sys/sysmacros.h
mkdir.o: /usr/include/stdio.h
mkdir.o: /usr/include/fcntl.h
mkdir.o: /usr/include/sys/types.h
mkdir.o: /usr/include/sys/stat.h
mkdir.o: /usr/include/sys/file.h
mkdir.o: /usr/include/sys/fcntl.h
mount.o: mount.c
mount.o: ./vroot.h
mount.o: /usr/include/stdio.h
mount.o: ./args.h
mount.o: /usr/include/sys/syscall.h
mount.o: /usr/include/errno.h
mount.o: /usr/include/sys/errno.h
mount.o: /usr/include/sys/time.h
mount.o: /usr/include/time.h
mount.o: /usr/include/sys/param.h
mount.o: /usr/include/machine/param.h
mount.o: /usr/include/signal.h
mount.o: /usr/include/sys/types.h
mount.o: /usr/include/sys/sysmacros.h
mount.o: /usr/include/stdio.h
mount.o: /usr/include/fcntl.h
mount.o: /usr/include/sys/types.h
mount.o: /usr/include/sys/stat.h
mount.o: /usr/include/sys/file.h
mount.o: /usr/include/sys/fcntl.h
open.o: open.c
open.o: ./vroot.h
open.o: /usr/include/stdio.h
open.o: ./args.h
open.o: /usr/include/sys/syscall.h
open.o: /usr/include/errno.h
open.o: /usr/include/sys/errno.h
open.o: /usr/include/sys/time.h
open.o: /usr/include/time.h
open.o: /usr/include/sys/param.h
open.o: /usr/include/machine/param.h
open.o: /usr/include/signal.h
open.o: /usr/include/sys/types.h
open.o: /usr/include/sys/sysmacros.h
open.o: /usr/include/stdio.h
open.o: /usr/include/fcntl.h
open.o: /usr/include/sys/types.h
open.o: /usr/include/sys/stat.h
open.o: /usr/include/sys/file.h
open.o: /usr/include/sys/fcntl.h
readlink.o: readlink.c
readlink.o: ./vroot.h
readlink.o: /usr/include/stdio.h
readlink.o: ./args.h
readlink.o: /usr/include/sys/syscall.h
readlink.o: /usr/include/errno.h
readlink.o: /usr/include/sys/errno.h
readlink.o: /usr/include/sys/time.h
readlink.o: /usr/include/time.h
readlink.o: /usr/include/sys/param.h
readlink.o: /usr/include/machine/param.h
readlink.o: /usr/include/signal.h
readlink.o: /usr/include/sys/types.h
readlink.o: /usr/include/sys/sysmacros.h
readlink.o: /usr/include/stdio.h
readlink.o: /usr/include/fcntl.h
readlink.o: /usr/include/sys/types.h
readlink.o: /usr/include/sys/stat.h
readlink.o: /usr/include/sys/file.h
readlink.o: /usr/include/sys/fcntl.h
report.o: report.c
report.o: ./report.h
report.o: /usr/include/stdio.h
report.o: /usr/include/sys/param.h
report.o: /usr/include/machine/param.h
report.o: /usr/include/signal.h
report.o: /usr/include/sys/types.h
report.o: /usr/include/sys/sysmacros.h
report.o: /usr/include/sys/wait.h
rmdir.o: rmdir.c
rmdir.o: ./vroot.h
rmdir.o: /usr/include/stdio.h
rmdir.o: ./args.h
rmdir.o: /usr/include/sys/syscall.h
rmdir.o: /usr/include/errno.h
rmdir.o: /usr/include/sys/errno.h
rmdir.o: /usr/include/sys/time.h
rmdir.o: /usr/include/time.h
rmdir.o: /usr/include/sys/param.h
rmdir.o: /usr/include/machine/param.h
rmdir.o: /usr/include/signal.h
rmdir.o: /usr/include/sys/types.h
rmdir.o: /usr/include/sys/sysmacros.h
rmdir.o: /usr/include/stdio.h
rmdir.o: /usr/include/fcntl.h
rmdir.o: /usr/include/sys/types.h
rmdir.o: /usr/include/sys/stat.h
rmdir.o: /usr/include/sys/file.h
rmdir.o: /usr/include/sys/fcntl.h
stat.o: stat.c
stat.o: ./vroot.h
stat.o: /usr/include/stdio.h
stat.o: ./args.h
stat.o: /usr/include/sys/syscall.h
stat.o: /usr/include/errno.h
stat.o: /usr/include/sys/errno.h
stat.o: /usr/include/sys/time.h
stat.o: /usr/include/time.h
stat.o: /usr/include/sys/param.h
stat.o: /usr/include/machine/param.h
stat.o: /usr/include/signal.h
stat.o: /usr/include/sys/types.h
stat.o: /usr/include/sys/sysmacros.h
stat.o: /usr/include/stdio.h
stat.o: /usr/include/fcntl.h
stat.o: /usr/include/sys/types.h
stat.o: /usr/include/sys/stat.h
stat.o: /usr/include/sys/file.h
stat.o: /usr/include/sys/fcntl.h
statfs.o: statfs.c
statfs.o: ./vroot.h
statfs.o: /usr/include/stdio.h
statfs.o: ./args.h
statfs.o: /usr/include/sys/syscall.h
statfs.o: /usr/include/errno.h
statfs.o: /usr/include/sys/errno.h
statfs.o: /usr/include/sys/time.h
statfs.o: /usr/include/time.h
statfs.o: /usr/include/sys/param.h
statfs.o: /usr/include/machine/param.h
statfs.o: /usr/include/signal.h
statfs.o: /usr/include/sys/types.h
statfs.o: /usr/include/sys/sysmacros.h
statfs.o: /usr/include/stdio.h
statfs.o: /usr/include/fcntl.h
statfs.o: /usr/include/sys/types.h
statfs.o: /usr/include/sys/stat.h
statfs.o: /usr/include/sys/file.h
statfs.o: /usr/include/sys/fcntl.h
truncate.o: truncate.c
truncate.o: ./vroot.h
truncate.o: /usr/include/stdio.h
truncate.o: ./args.h
truncate.o: /usr/include/sys/syscall.h
truncate.o: /usr/include/errno.h
truncate.o: /usr/include/sys/errno.h
truncate.o: /usr/include/sys/time.h
truncate.o: /usr/include/time.h
truncate.o: /usr/include/sys/param.h
truncate.o: /usr/include/machine/param.h
truncate.o: /usr/include/signal.h
truncate.o: /usr/include/sys/types.h
truncate.o: /usr/include/sys/sysmacros.h
truncate.o: /usr/include/stdio.h
truncate.o: /usr/include/fcntl.h
truncate.o: /usr/include/sys/types.h
truncate.o: /usr/include/sys/stat.h
truncate.o: /usr/include/sys/file.h
truncate.o: /usr/include/sys/fcntl.h
unlink.o: unlink.c
unlink.o: ./vroot.h
unlink.o: /usr/include/stdio.h
unlink.o: ./args.h
unlink.o: /usr/include/sys/syscall.h
unlink.o: /usr/include/errno.h
unlink.o: /usr/include/sys/errno.h
unlink.o: /usr/include/sys/time.h
unlink.o: /usr/include/time.h
unlink.o: /usr/include/sys/param.h
unlink.o: /usr/include/machine/param.h
unlink.o: /usr/include/signal.h
unlink.o: /usr/include/sys/types.h
unlink.o: /usr/include/sys/sysmacros.h
unlink.o: /usr/include/stdio.h
unlink.o: /usr/include/fcntl.h
unlink.o: /usr/include/sys/types.h
unlink.o: /usr/include/sys/stat.h
unlink.o: /usr/include/sys/file.h
unlink.o: /usr/include/sys/fcntl.h
unmount.o: unmount.c
unmount.o: ./vroot.h
unmount.o: /usr/include/stdio.h
unmount.o: ./args.h
unmount.o: /usr/include/sys/syscall.h
unmount.o: /usr/include/errno.h
unmount.o: /usr/include/sys/errno.h
unmount.o: /usr/include/sys/time.h
unmount.o: /usr/include/time.h
unmount.o: /usr/include/sys/param.h
unmount.o: /usr/include/machine/param.h
unmount.o: /usr/include/signal.h
unmount.o: /usr/include/sys/types.h
unmount.o: /usr/include/sys/sysmacros.h
unmount.o: /usr/include/stdio.h
unmount.o: /usr/include/fcntl.h
unmount.o: /usr/include/sys/types.h
unmount.o: /usr/include/sys/stat.h
unmount.o: /usr/include/sys/file.h
unmount.o: /usr/include/sys/fcntl.h
utimes.o: utimes.c
utimes.o: ./vroot.h
utimes.o: /usr/include/stdio.h
utimes.o: ./args.h
utimes.o: /usr/include/sys/syscall.h
utimes.o: /usr/include/errno.h
utimes.o: /usr/include/sys/errno.h
utimes.o: /usr/include/sys/time.h
utimes.o: /usr/include/time.h
utimes.o: /usr/include/sys/param.h
utimes.o: /usr/include/machine/param.h
utimes.o: /usr/include/signal.h
utimes.o: /usr/include/sys/types.h
utimes.o: /usr/include/sys/sysmacros.h
utimes.o: /usr/include/stdio.h
utimes.o: /usr/include/fcntl.h
utimes.o: /usr/include/sys/types.h
utimes.o: /usr/include/sys/stat.h
utimes.o: /usr/include/sys/file.h
utimes.o: /usr/include/sys/fcntl.h
vroot.o: vroot.c
vroot.o: ./vroot.h
vroot.o: /usr/include/stdio.h
vroot.o: ./args.h
vroot.o: /usr/include/sys/syscall.h
vroot.o: /usr/include/errno.h
vroot.o: /usr/include/sys/errno.h
vroot.o: /usr/include/sys/time.h
vroot.o: /usr/include/time.h
vroot.o: /usr/include/sys/param.h
vroot.o: /usr/include/machine/param.h
vroot.o: /usr/include/signal.h
vroot.o: /usr/include/sys/types.h
vroot.o: /usr/include/sys/sysmacros.h
vroot.o: /usr/include/stdio.h
vroot.o: /usr/include/fcntl.h
vroot.o: /usr/include/sys/types.h
vroot.o: /usr/include/sys/stat.h
vroot.o: /usr/include/sys/file.h
vroot.o: /usr/include/sys/fcntl.h
vroot.o: /usr/include/sys/param.h
vroot.o: /usr/include/sys/file.h
setenv.o: setenv.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
