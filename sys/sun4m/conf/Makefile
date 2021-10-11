#
# @(#)Makefile 1.1 92/07/30 SMI
#
# This makefile creates the distributed kernel objects and
# configuration files.

ARCH=	sun4m
MACH=	sparc

CONFDIR= ../../conf.common
MACHINES= DL NFS SDST IDST690 GENERIC_SMALL
FILES=	scsi/conf/scsi_confdata.c scsi/targets/sd_conf.c \
	scsi/targets/st_conf.c $(ARCH)/openprom_xxx.c
HDIRS=	sbusdev scsi scsi/adapters scsi/conf scsi/generic scsi/impl scsi/targets

include ../../conf.common/Makefile.install
