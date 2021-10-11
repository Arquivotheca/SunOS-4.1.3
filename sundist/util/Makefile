#
# @(#)Makefile 1.1 92/07/30 Copyr 1987 Sun Micro
#
DESTDIR=../bin
ARCH:sh = arch -k

# Shell scripts that need only be installed and are never removed.
#
STD=	Install.miniroot disk_usage fdrolloff getmini \
	getmini.internal getmunixfs makemunix \
	makexcl missing \
	extract.cdrom extract.diskette extract.tape stamprd newdisksize

README= README.cdrom README.diskette README.tape

NSTD=	fastread.$(ARCH)

SETUID=	buildmini

all:	$(STD) $(NSTD) $(SETUID) $(README)

fastread.$(ARCH): fastread.c
	cc -D${ARCH} -o $@ $?

install: all
	install -d $(DESTDIR)
	install $(STD) $(DESTDIR)
	install $(NSTD) $(DESTDIR)
	install $(README) $(DESTDIR)
	install -o root -m 4755 $(SETUID) $(DESTDIR)

clean:
	$(RM) $(STD) $(NSTD) $(SETUID)

FRC:
