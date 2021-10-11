#
# @(#)Makefile 1.1 92/07/30 SMI
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#

all : xall

include ../../Makefile.arch
include $(ARCH)/Makefile
include ../../Makefile.master

MID_HDRS = char_to_decimal.h mallint.h base_conversion.h codeset.h\
	euc.h iso2022.h mbextern.h _Qglobals.h _Qquad.h
MID_COMMSRC =  _crypt.c _nlist.c _perror.c _psignal.c a64l.c abort.c abs.c alarm.c\
	asctime.c atof.c atoi.c atol.c audit_args.c bcmp.c bcopy.c bsearch.c\
	bzero.c clock.c closedir.c crypt.c ctime.c ctype_.c\
	drand48.c dysize.c errlst.c execvp.c exit.c exportent.c ecvt.c\
	euc.multibyte.c\
	fabs.c fgetpwaent.c fgetpwent.c fmod.c frexp.c fstab.c ftok.c ftw.c\
	getacinfo.c getauditflags.c \
	getcwd.c getenv.c getfaudflgs.c getgraent.c getgrent.c\
	getopt.c getsubopt.c getpwaent.c  getttyent.c\
	getttynam.c getusershell.c getwd.c grpauth.c hsearch.c\
	ieee_globals.c index.c initgroups.c isatty.c iso.multibyte.c\
	issecure.c jcsetpgrp.c\
	l64a.c labeltostring.c ldexp.c lfind.c lsearch.c\
	madvise.c mallopt.c mblib.c mbstowcs.c mbtowc.c \
	memalign.c memccpy.c memchr.c memcmp.c\
	memcpy.c memset.c mkstemp.c mktemp.c mlock.c mlockall.c\
	mntent.c modf.c msync.c munlock.c munlockall.c\
	nl_cxtime.c nl_strncmp.c  ndbm.c on_exit.c\
	opendir.c optind.c perror.c plock.c \
	posix_tty.c popen.c psignal.c putenv.c putpwent.c \
	pwdauth.c pwdnm.c qsort.c\
	random.c readdir.c realpath.c regex.c rindex.c scandir.c seekdir.c\
	setegid.c seteuid.c setlocale.c setrgid.c setruid.c sigfpe.c siglist.c\
	siginterrupt.c ssignal.c strcat.c strchr.c strcmp.c stricmp.c strcpy.c\
	strcspn.c strdup.c strftime.c strlen.c strncat.c strncmp.c strncpy.c \
	strpbrk.c strptime.c \
	strrchr.c strspn.c strstr.c  strtod.c strtok.c strtol.c strxfrm.c swab.c syslog.c\
	system.c telldir.c tfind.c time.c toupper.c tolower.c tsearch.c ttyname.c\
	ualarm.c usleep.c valloc.c ecvt.c string_decim.c \
	file_decim.c func_decim.c econvert.c seconvert.c gconvert.c \
	decimal_bin.c float_decim.c double_decim.c \
	pack_float.c _unpack_dble.c unpack_float.c waitpid.c\
	_base_S.c _base_il.c _base_sup.c _big_power.c _small_power.c \
	_sprintf_sup.c _times_power.c qeconvert.c \
	wcstombs.c wctomb.c xccs.multibyte.c \
	_Q_neg.c _Q_add.c _Q_sub.c _Q_mul.c _Q_div.c _Q_cmp.c _Q_cmpe.c \
        _Q_stoq.c _Q_dtoq.c _Q_itoq.c _Q_utoq.c _Q_qtos.c _Q_qtod.c \
        _Q_qtoi.c _Q_qtou.c _Q_sqrt.c _Qfaddsub.c _Qfcompare.c _Qfdiv.c \
        _Qfmul.c _Qfpack.c _Qfunpack.c _Qfutility.c _Q_fcc.c _Q_get_rp_rd.S
MID_4.2SRC = \
	calloc.c malloc.c posix_sig.c \
	getlogin.c getpass.c getpwent.c localtime.c 
MID_S5SRC = $(MID_4.2SRC)

MID_FASTSRC = malloc.bsd43.c

libc_comm.so :=  MID_CSRC = $(MID_COMMSRC)
libc.so := MID_CSRC = $(MID_4.2SRC) 
libc.a libc_p.a := MID_CSRC = $(MID_4.2SRC) $(MID_COMMSRC)
libcs5.so := MID_CSRC = $(MID_S5SRC)
libcs5.a libcs5_p.a := MID_CSRC = $(MID_S5SRC) $(MID_COMMSRC) 
libbsdmalloc.a := MID_CSRC = $(MID_FASTSRC)

OBJS = $(MD_CSRC:$(ARCH)/%.c=$(VARIANT)/%.o) \
	$(MD_ASSRC:$(ARCH)/%.s=$(VARIANT)/%.o) \
	$(MID_CSRC:%.S=$(VARIANT)/%.o) \
	$(MID_CSRC:%.c=$(VARIANT)/%.o)

xall:	$$(LIBS)


$(LIBS) : symlink $$(VARIANT) $$(OBJS)

clean: master.clean

.INIT: $(MID_HDRS) $(MD_HDRS) $(MD_FPA_HDRS)

shared/strxfrm.o: strxfrm.c
	$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -c -o $@ strxfrm.c

shared/_small_power.o: _small_power.c
	$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -R -c -o $@ _small_power.c

shared/_big_power.o: _big_power.c
	$(CC) $(CFLAGS$(TARGET_MACH)) $(CFLAGS) -R -c -o $@ _big_power.c

#shared/fgetpwent.o \
#shared_4.2/getlogin.o shared_s5/getlogin.o \
#shared_4.2/getpwent.o shared_s5/getpwent.o \
shared/stricmp.o \
shared/ndbm.o \
shared/regex.o \
shared/getopt.o := CFLAGS += -R

shared/putpwent.o := CFLAGS += -R

obj/_crypt.o obj_p/_crypt.o s5/_crypt.o s5_p/_crypt.o \
shared/_crypt.o := CFLAGS += -DCRYPT -R

obj/_nlist.o obj_p/_nlist.o s5/_nlist.o s5_p/_nlist.o \
shared/_nlist.o := CFLAGS += -D$(TARGET)

obj/ssignal.o obj_p/ssignal.o s5/ssignal.o s5_p/ssignal.o \
shared/ssignal.o := CFLAGS += -w

# Shared versions use dlopen, dlclose, dlsym routines
shared/mblib.o \
shared/mbstowcs.o \
shared/mbtowc.o \
shared/wcstombs.o \
shared/wctomb.o := CFLAGS += -DPIC

obj/_Q_get_rp_rd.o obj_p/_Q_get_rp_rd.o s5/_Q_get_rp_rd.o s5_p/_Q_get_rp_rd.o \
shared/_Q_get_rp_rd.o := CPPFLAGS += -I../../sys/common/$(ARCH)

symlink:
	rm -rf strings;\
	ln -s ../../strings strings
