/*	@(#)disk_strings.h 1.1 92/07/30 Copyright Sun Micro	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/* disk controller names as per Sun Unix 4.1 */
char	*disk_ctlr[] = {
	"Unknown",                      /* 0 */
	"Unknown",                      /* 1 used to be ip2180 */
	"Unknown",                      /* 2 used to be wdc2880 */
        "Unknown",                      /* 3 used to be ip2181 */
        "Unknown",                      /* 4 used to be xy440 */
        "Unknown",                      /* 5 used to be dsd5215 */
        "Xylogics 450/451",             /* 6 */
        "Adaptec ACB4000",              /* 7 */
        "Emulex MD21",                  /* 8 */
        "Unknown",                      /* 9 used to be xd751 */
        "SCSI Floppy",                  /* 10 */
        "Xylogics 7053",                /* 11 */
#ifndef	sun386
        "SCSI SMS Floppy",              /* 12 */
        "SCSI CCS",                     /* 13 */
        "Onboard 82072 Floppy",         /* 14 */
        "IPI ISP-80",                   /* 15(IPI Panther) */
        "Virtual Disk",                 /* 16 meta-disk (virtual-disk) driver */
        "CDC 9057",     /* 17 CDC 9057-321 (CM-3) IPI String Controller */
        "Fujitsu M1060",        /* 18 Fujitsu/Intellistor M1060 IPI-3 SC */
        "Onboard 82077 Floppy",         /* 19 */
#else	sun386
	"SCSI CCS",			/* 12 */
	"Onboard NEC765 Floppy",	/* 13 */
#endif	sun386
};

#define	NDISKCTLR	sizeof(disk_ctlr)/sizeof(char *)
