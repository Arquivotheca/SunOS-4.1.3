# @(#)Makefile 1.1 92/07/30 SMI;
#
# PAX - read and write POSIX conformant tar and cpio archives 
#
# Written by Mark H. Colburn (mark@jhereg.mn.org)
#
# $Id: Makefile,v 1.1 88/12/23 18:02:42 mark Rel $
#

#
# CONFIGURATION SECTION
#
# The following defines may need to be changed for each system which PAX
# is installed on.  Please review these settings before installing on your
# system.

#
# You should define _POSIX_SOURCE if you are running on a POSIX system.  This
# include has to be in the command line because it has to appear before any
# include file is included in the source.  For most systems in use today,
# it should be left blank.
# 
# POSIX= -D_POSIX_SOURCE
POSIX= 

#
# Set CFLAGS to whatever makes your C compiler happy.  Be sure to include 
# the definition of $(POSIX) in the flag.
#
DESTDIR =
CFLAGS = -O $(POSIX)
CC = cc

#
# Set LFLAGS to whatever makes your linker happy
#
#LDFLAGS = -s
LDFLAGS = 

#
# Set LINTFLAGS to whatever makes your implementation of lint happy.  If
# you don't undef __STDC__ and you have an ANSI C compiler, lint will choke 
# on the function prototypes present in func.h
#
LINTFLAGS = -U__STDC__ $(POSIX)

#
# BINDIR - points to the directory in which you want the final pax, ustar and
# paxcpio binaries installed in.
#
BINDIR = /usr/5bin

#
# MANDIR - specify the directory in which the man pages will be installed
#
#MAN5 = /usr/man/man5
#MAN1 = /usr/man/man1

#
# There are three different ways to get POSIX or BSD conformant directory 
# access routines: 1) they are installed in your system library, 2) you 
# are using Doug Gwyn's dirent library (/usr/lib/libdirent.a), or 3) you 
# need the source for the dirent package.  Based on that, pick one of the 
# following three options:
#
# 1. Pick the first dirent line and make sure that config.h is defined
#    correctly for your version of directory access routines.  THIS IS
#    THE LINE WHICH SHOULD BE USED FOR BSD SYSTEMS.
# 2. Chose the second dirent line which  used a library at link time.  You
#    may need to change the name of the library to match your system.
# 3. If you need #3, then you must copy everything in the subdirectory dirent 
#    to this directory and choose the DIROBJ lines.  Please note that this 
#    version of dirent has been modified to work as a stand-alone. 
#
DIRENT=
#DIRENT= -ldirent
#DIROBJ= paxdir.o

#
# END CONFIGURATION SECTION 
#
# Nothing beyond this point should need to be changed.
#

MISC   = Makefile pax.1 ustar.1 paxcpio.1 README PATCHLEVEL
HEADERS= config.h func.h limits.h port.h pax.h paxdir.h regexp.h 
SOURCE= pax.c append.c buffer.c paxcpio.c create.c extract.c fileio.c\
	link.c list.c mem.c namelist.c names.c pass.c pathname.c\
	port.c regexp.c replace.c ustar.c ttyio.c warn.c wildmat.c
OBJECT= pax.o append.o buffer.o paxcpio.o create.o extract.o fileio.o\
	link.o list.o mem.o namelist.o names.o pass.o pathname.o\
	port.o regexp.o replace.o ustar.o ttyio.o warn.o wildmat.o $(DIROBJ)
PROGS = pax ustar paxcpio
#PMAN1 = pax.1 ustar.1 paxcpio.1
#PMAN5 = pax.5 ustar.5 paxcpio.5

all: $(PROGS)

install: ${PROGS}
	install -s pax ${DESTDIR}$(BINDIR)/pax
	install -s ustar ${DESTDIR}$(BINDIR)/ustar
	install -s paxcpio ${DESTDIR}$(BINDIR)/paxcpio

clean:
	rm -f $(OBJECT)
	rm -f $(PROGS) a.out *.BAK *.bak 

lint:
	lint $(LINTFLAGS) $(SOURCE)

pax : $(OBJECT)
	$(CC) $(CFLAGS) -o pax $(OBJECT) $(DIRENT) 

ustar: pax
	rm -f ustar
	ln pax ustar

paxcpio: pax
	rm -f paxcpio
	ln pax paxcpio

$(OBJECT): $(HEADERS)
