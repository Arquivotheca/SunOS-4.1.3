/*      @(#)tape_strings.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/* tape type names as per Sun Unix 3.5.1 */
struct mt_tape_info {
        short   t_type;         /* type of magtape device */
        char    *t_name;        /* printing name */
        char    *t_dsbits;      /* "drive status" register */
        char    *t_erbits;      /* "error" register */
} tapes[] = {
        { MT_ISCPC,      "TapeMaster",          TMS_BITS,       0 },
        { MT_ISAR,       "Multibus Archive",    ARCH_CTRL_BITS, ARCH_BITS },
        { MT_ISSC,       "SCSI Archive",                0,      ARCH_BITS },
        { MT_ISXY,       "Xylogics 472",        XTS_BITS,       0 },
        { MT_ISSYSGEN11, "SCSI Sysgen, QIC-11 only",    0,      0 },
        { MT_ISSYSGEN,   "SCSI Sysgen QIC-24/11",       0,      0 },
        { MT_ISDEFAULT,  "SCSI generic CCS",            0,      0 },
        { MT_ISMT02,     "SCSI Emulex MT02",            0,      0 },
        { MT_ISVIPER1,   "SCSI Archive QIC-150 Viper",  0,      0 },
        { MT_ISWANGTEK1, "SCSI Wangtek QIC-150",        0,      0 },
        { MT_ISCCS7,     "SCSI generic CCS",            0,      0 },
        { MT_ISCCS8,     "SCSI generic CCS",            0,      0 },
        { MT_ISCCS9,     "SCSI generic CCS",            0,      0 },
        { MT_ISCCS11,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS12,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS13,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS14,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS15,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS16,    "SCSI generic CCS",            0,      0 },
        { MT_ISCDC,      "SCSI CDC 1/2 cartridge",      0,      0 },
        { MT_ISFUJI,     "SCSI Fujitsu 1/2 cartridge",  0,      0 },
        { MT_ISKENNEDY,  "SCSI Kennedy 1/2 reel",       0,      0 },
        { MT_ISHP,       "SCSI HP 1/2 reel",            0,      0 },
        { MT_ISCCS21,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS22,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS23,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS24,    "SCSI generic CCS",            0,      0 },
        { MT_ISEXABYTE,  "SCSI Exabyte 8mm cartridge",  0,      0 },
        { MT_ISCCS26,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS27,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS28,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS29,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS30,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS31,    "SCSI generic CCS",            0,      0 },
        { MT_ISCCS32,    "SCSI generic CCS",            0,      0 },
        { 0 }
};
