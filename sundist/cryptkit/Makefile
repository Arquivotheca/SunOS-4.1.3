#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR	=	/usr/tmp/unbundled
CPP	=	/lib/cpp -C -P
CPPOPTS	=	-undef -D_MACH_=$(MACH) -D_ARCH_=$(ARCH) \
		-D_SUNBIN_=$(SUNBIN:sh) -D_PART_=$(PART:sh) -D_SIZE_=$(SIZE)
ARCH:sh =	/bin/arch
MACH:sh	=	/bin/mach
DEV	=	st8
BS	=	126
KLUDGE	=	q
SIZE	=	"1/4\" Tape, tar format, QIC-24, 1 of 1"
SUNBIN	=	awk -F\t '/$(MACH).$(DEV)/ { print $$2 }' Part_Numbers
PART	=	awk -F\t '/$(MACH).$(DEV)/ { print $$3 }' Part_Numbers

MEDIA	=	quarter half floppy 
half	:=	DEV = mt0
half	:=	BS = 20
half	:=	KLUDGE = h
half	:=	SIZE = "1/2\" Tape, tar format, 1600 BPI, 1 of 1"
floppy	:=	DEV = fd0
floppy	:=	BS = 18
floppy	:=	KLUDGE = d
floppy	:=	SIZE = "3-1/2\" Diskette, bar format"
cdrom	:=	DEV = sr0
cdrom	:=	KLUDGE = r
cdrom	:=	SIZE = "CD-ROM (UFS file system format) 1 of 1"

FILES	=	install_unbundled 4.1.2_US_Encryption mktape
CDFILES = 	mkcdrom install_unbundled cdrom/4.1.3_US_Encryption \
		cdrom/_info cdrom/cdm cdrom/cdmanager cdrom/cdruninstall \
		cdrom/extract_unbundled cdrom/Session.icon
EXCL	=	exclude.debug exclude.usr
COPYRIGHT=	copyright.$(KLUDGE)
CRYPTKIT=	../../CRYPTKIT

.INIT:		Part_Numbers

%.$$(KLUDGE):	%
	$(CPP) $(CPPOPTS) $< > $@

all:		$(MEDIA)

quarter half floppy:	$(FILES) $(EXCL) $$(COPYRIGHT)
	mv $(COPYRIGHT) Copyright
	mktape $(DEV)

cdrom:	$$(COPYRIGHT) $(CDFILES)
	mkcdrom

$(DESTDIR): FRC
	rm -rf $(DESTDIR)
	install -d $(DESTDIR)
	@k=`df ${DESTDIR} |sed 1d|awk ' { print $$4 }'`;\
	if [ $$k -lt 3000 ] ; then \
		echo ${DESTDIR} "does not have enough space in it!"; \
		echo "Make at least 3M available and try again."; \
		false;\
	fi

$(CRYPTKIT):	FRC
	cd $(@D); $(MAKE) $(MFLAGS) $(@F)

FRC:

