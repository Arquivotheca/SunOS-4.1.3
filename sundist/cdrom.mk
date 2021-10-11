#
## @(#)cdrom.mk 1.1 92/07/30 Copyright 1990 Sun Microsystems, Inc.
#
# CDROM make file
#
# Assumptions:
#
#	sizing is done by the sundist Makefile,
#	so all this has to do is grab the tar images from $(TARFILES)/$(ARCH)
#	miniroot.(ARCH) is make by sundist/Makefile
#	munixfs.(ARCH).cdrom is make by sundist/Makefile
#	MUNIX, boot, ... already built in build
#
# Deliverables
#	(1) tar file images - nothing to do, already in $(TARFILES)/$(ARCH)/*
#	(2) miniroot.$(ARCH) - nothing to do, already in miniroot.$(ARCH)
#	(3) xdrtoc - nothing to do, already in $(ARCH)_cdrom.xdr
#	(4) MUNIX/munixfs/miniroot partition image
#
# 	The MUNIX/munixfs/miniroot partition image is how the CDROM is booted
# and loads the miniroot.  This is delivered as an image that is architecture
# specific (ie there is one for each bootable architecture on the CDROM).
# The following table explains how this is laid out:
#		sun4c sun4m		non-sun4c
# 0 Meg ------------------------------------------------------
#			a bootable ufs filesystem
# 2 Meg  (munixfs inside	+-----------------------------
#		munix)		| munixfs image 
# 4 Meg ------------------------------------------------------
#		miniroot image (same miniroot as tape)
#
# 12 Meg -----------------------------------------------------
#	
# TARFILES is now passed from Makefile
# ARCH passed in from Makefile
# PROTO passed in from Makefile
#
# Note: partition assignments are defined in Makefile.common_cd
#
include Makefile.common_cd
#
# CONFIGURABLE THINGS - that might change
MINIMNT=/miniroot
#
# MINISIZE is the size of the bootable UFS filesystem put onto the CDROM.
# for sun4c and sun4m  (pre-loaded MUNIX) it is 4 Meg, for others
# it is 2 Meg.  It is in units of 512byte blocks (for use by newfs).
MINISIZE.sun4m=8192
MINISIZE.sun4c=8192
MINISIZE.sun4=4096
MINISIZE.sun3x=4096
MINISIZE.sun3=4096
MINISIZE=$(MINISIZE.$(ARCH))
#
# for non-preloaded MUNIX
# offsets into image file, 8kb units, 2Meg and 4Meg for now
# NOTE sun4c and sun4m MUNIXFS is combined with kernel in bootable portion.
OFFSET_MUNIXFS= 256
OFFSET_MINIROOT= 512
# name of image file
IMAGE= cdrom_part_$(ARCH)

INSTALLBOOT=$(PROTO)/usr/kvm/mdec/installboot
BOOTBLKS=$(PROTO)/usr/kvm/mdec/rawboot
BOOTPROG=$(PROTO)/usr/kvm/stand/boot.$(ARCH)
KERNEL= $(PROTO)/usr/kvm/stand/munix

COPYRIGHT=./Copyright
PARTNUM=./part_num_cdrom

# figure out partition for this arch, unit will be done at runtime
# XXX toss loadramdiskfrom in sys/sun/swapgeneric.c OPENPROM getchardev
# XXX  and then get rid of these too - but must also do diskette.mk...
#
# The partition assignment during boot image creation really matters
# only for sun4c and sun4m; others are bogus.  
# (This kludge will be fixed in the PIE release).
#
CDPART.sun4= sd0$(PART_gen.sun4)
CDPART.sun4m= sd0$(PART_gen.sun4m)
CDPART.sun4c= sd0$(PART_gen.sun4c)
CDPART.sun3x= sd0$(PART_gen.sun3x)
CDPART.sun3= sd0$(PART_gen.sun3)

CDPART= $(CDPART.$(ARCH))

# WHEREAT is in hex, for adb, only used for non-sun4c machines
WHEREAT= 200000
# WHEREAT=echo "{ obase=16; 8192 * ${OFFSET_MUNIXFS}; quit }" | bc
# WHEREAT=echo "obase=16; 8192 * ${OFFSET_MUNIXFS}; quit" | bc
# NOTE when this gets fixed, be sure to change the adb useage to :sh

where:
	@echo WHERE IS ${WHEREAT}
	@echo WHERE:sh is ${WHEREAT:sh}


#
# "make -f cdrom.mk PROTO=/proto ARCH=`arch -k` TARFILES=/tarfiles deliverables"
# will make the cdrom deliverables
# (if munixfs, miniroot and tarfiles already done)
#
deliverables: $(IMAGE)

$(IMAGE): bootimage.cdrom.$(ARCH) munixfs.$(ARCH).cdrom miniroot.$(ARCH)
	echo "Copying bootable filesystem ... "
	cat bootimage.cdrom.$(ARCH) /dev/zero |\
		dd ibs=1b count=$(MINISIZE) obs=8k conv=sync > $(IMAGE)
	if [ $(ARCH) != "sun4c" -a $(ARCH) != "sun4m" ]; then \
		echo "Copying munixfs ..." ; \
		cat munixfs.$(ARCH).cdrom /dev/zero |\
			dd bs=2k count=1024 conv=sync >> $(IMAGE) ; \
	fi
	echo "Copying the miniroot to cdrom boot image ..."
	dd if=miniroot.$(ARCH) bs=8k >> $(IMAGE) 
	echo "CDROM architecture partition image done"

#
# turn a generic MUNIX kernel into one that knows what to use for
# root and swap
# if it is pre-loadable, load it, else and tell it how to load the ramdisk
#
munix.cdrom.${ARCH}: ${KERNEL}
	@ echo "snarfing memory unix kernel ..."
	cp ${KERNEL} munix.cdrom.${ARCH}
	@ echo "making rootfs \"4.2\" + \"rd0a\"; swapfile \"spec\" + \"ns0b\""
	( echo "rootfs?W '4.2'" ; \
	  echo "rootfs+10?W 'rd0a'" ; \
	  echo "swapfile?W 'spec'" ; \
	  echo "swapfile+10?W 'ns0b'" ; \
	  echo '$$q'; ) | adb -w munix.cdrom.${ARCH}
	# set memory to 8MB to allow large memory 4c systems to be installed
	if [ $(ARCH) = "sun4c" ]; then \
		( echo "physmem?W 0x800" ; \
		echo '$$q'; ) | adb -w munix.cdrom.${ARCH};\
	fi
	# if sun4c or sun4m, then stamp the munixfs in to the ramdisk
	if [ $(ARCH) = "sun4c" -o $(ARCH) = "sun4m" ]; then \
		`pwd`/bin/stamprd munix.cdrom.$(ARCH) munixfs.$(ARCH).cdrom;  \
		( echo "ramdiskispreloaded?W 1"; \
		  echo '$$q'; ) | adb -w munix.cdrom.${ARCH} ; \
	else \
		( echo "loadramdiskfrom?W '${CDPART}'" ; \
		  echo "loadramdiskfile?W ${WHEREAT}" ; \
		  echo '$$q'; ) | adb -w munix.cdrom.${ARCH} ;\
	fi


#
# make a bootable cdrom image, with munix and /boot.sun4c on it
# we use mkfs vs. newfs because we want to put our own boot on it
# NOTE: cdrom must have 0 rotational delay for optimum performance
#
bootimage.cdrom.$(ARCH): munix.cdrom.${ARCH} ${BOOTPROG}
	MINIDEV=`grep ${MINIMNT} /etc/fstab | sed -e 's,[ 	].*,,'` ;\
	if [ $$MINIDEV"x" = "x" ]; then \
		echo "$0: can't find device for mount point ${MINIMNT}" ;\
		exit 1 ;\
	fi ;\
	umount ${MINIMNT} ;\
	BDEV=`basename $$MINIDEV` ;\
	RDEV=/dev/r$$BDEV ;\
	echo "making filesystem on $$RDEV" ;\
	mkfs $$RDEV ${MINISIZE} ;\
	fsck -p $$RDEV ;\
	tunefs -m 0 $$RDEV ;\
	mount ${MINIMNT} ;\
        cp ${BOOTPROG} ${MINIMNT}/boot.${ARCH} ;\
	cp munix.cdrom.$(ARCH) ${MINIMNT}/vmunix ;\
	# create the copyright file on each ufs partition \
	# note: the same copyright file will be created for the ISO \
	#       partion, see cdimage.mk \
	rm -f ${CDROM_DEST_TARFILES}/Copyright ;\
	cat $(PROTO)/usr/kvm/sys/conf.common/RELEASE >>${MINIMNT}/Copyright;\
	echo ${CDLABEL_ARCH} >> ${MINIMNT}/Copyright ;\
	echo ${CDLABEL_MEDIA} >> ${MINIMNT}/Copyright ;\
	cat ${PARTNUM} >> ${MINIMNT}/Copyright ;\
	cat ${COPYRIGHT} >> ${MINIMNT}/Copyright ;\
        sync; sync; sleep 8 ;\
	if [ X${ARCH} = Xsun4c -o X${ARCH} = Xsun4m ]; then \
		${INSTALLBOOT} -hv ${MINIMNT}/boot.${ARCH} ${PROTO}/usr/mdec/rawboot $$RDEV ;\
	else \
		${INSTALLBOOT} -v ${MINIMNT}/boot.${ARCH} ${PROTO}/usr/mdec/rawboot $$RDEV ;\
	fi ;\
	sync ;\
	sync ;\
	umount ${MINIMNT} ;\
	fsck -p $$RDEV ;\
	echo dd if=$$RDEV of=bootimage.cdrom.$(ARCH) bs=8k count=`expr ${MINISIZE} / 16` ;\
	dd if=$$RDEV of=bootimage.cdrom.$(ARCH) bs=8k count=`expr ${MINISIZE} / 16` ;\
	echo "Made cdrom bootimage for $(ARCH)"

clean:
	rm -f $(IMAGE) munix.cdrom.$(ARCH)

