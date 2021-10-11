#ifndef lint
static	char sccsid[] = "%Z%%M% %I% %E% Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "sundiag.h"
#include "struct.h"

/******************************************************************************
 * Internal data structure of all possible tests.		      	      *
 * Note: should be initialized in the sequence of test i.d.(defined in	      *
 * struct.h). If you don't keep these two sequences the same, test control    *
 * panel will possibly be corrupted as data structure in a mess.	      *
 *****************************************************************************/
struct	test_info tests_base[]=
{

/***** physical memory test *****/
  MEMGROUP,			/* group */
  PMEM,				/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Physical",			/* label */
  "mem",			/* devname */
  "pmem",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** virtual memory test *****/
  MEMGROUP,			/* group */
  VMEM,				/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Virtual",			/* label */
  "kmem",			/* devname */
  "vmem",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** SCSI disk tests(read only) *****/
  DISKGROUP,			/* group */
  SCSIDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI Disk #",		/* label */
  "sd\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** SCSI disk tests(read/write) *****/
  DISKGROUP,			/* group */
  SCSIDISK2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI Disk #",		/* label */
  "sd\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** XY disk tests(read only) *****/
  DISKGROUP,			/* group */
  XYDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "XY Disk #",			/* label */
  "xy\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** XY disk tests(read/write) *****/
  DISKGROUP,			/* group */
  XYDISK2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "XY Disk #",			/* label */
  "xy\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** XD disk tests(read only) *****/
  DISKGROUP,			/* group */
  XDDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "XD Disk #",			/* label */
  "xd\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** XD disk tests(read/write) *****/
  DISKGROUP,			/* group */
  XDDISK2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "XD Disk #",			/* label */
  "xd\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** IPI disk tests(read only) *****/
  DISKGROUP,			/* group */
  IPIDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "IPI Disk #",			/* label */
  "ip\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** IPI disk tests(read/write) *****/
  DISKGROUP,			/* group */
  IPIDISK2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "IPI Disk #",			/* label */
  "ip\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** ID disk tests(read only) *****/
  DISKGROUP,			/* group */
  IDDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "ID Disk #",			/* label */
  "id\0\0\0\0\0",		/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** ID disk tests(read/write) *****/
  DISKGROUP,			/* group */
  IDDISK2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "ID Disk #",			/* label */
  "id\0\0\0\0\0",		/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** SCSI floppy disk tests( raw write/read) *****/
  DISKGROUP,			/* group */
  SFDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI Floppy Disk #",		/* label */
  "sf\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** SCSI floppy disk tests( file system read/write) *****/
  DISKGROUP,			/* group */
  SFDISK2,			/* id */
  0,				/* unit */
  1,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI Floppy Disk #",		/* label */
  "sf\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/****** on-board floppy disk tests(raw write/read) *****/
  DISKGROUP,			/* group */
  OBFDISK1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "OB Floppy Disk #",		/* label */
  "fd\0\0",			/* devname */
  "rawtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x2,			/* data(defaults to Test Mode = Readonly, Partition = 'c' */

/***** on-board floppy disk tests(file system read/write) *****/
  DISKGROUP,			/* group */
  OBFDISK2,			/* id */
  0,				/* unit */
  1,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "OB Floppy Disk #",		/* label */
  "fd\0\0",			/* devname */
  "fstest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** CD_ROM test *****/
  DISKGROUP,			/* group */
  CDROM,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI CD-ROM #",		/* label */
  "sr\0\0",			/* devname */
  "cdtest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)255,			/* special(volume defaults to 255) */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)(4+(100<<8)),		/* data(type default to "other") */

/***** MC68881 test *****/
  CPUGROUP,			/* group */
  MC68881,			/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Floating Point",		/* label */
  "68881",			/* devname */
  "mc68881",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** MC68882 test *****/
  CPUGROUP,			/* group */
  MC68882,			/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Floating Point",		/* label */
  "68882",			/* devname */
  "mc68881",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** FPU test *****/
  CPUGROUP,			/* group */
  FPUTEST,			/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "FPU",			/* label */
  "fpu",			/* devname */
  "fputest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** FPU test *****/
  CPUGROUP,			/* group */
  FPU2,				/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "FPU2",			/* label */
  "fpu",			/* devname */
  "fputest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** des test *****/
  CPUGROUP,			/* group */
  DES,				/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "DCP Chip",			/* label */
  "des\0\0",			/* devname */
  "dcptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** on board ethernet test(ie0) *****/
  CPUGROUP,			/* group */
  ENET0,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Ethernet",			/* label */
  "ie\0\0",			/* devname */
  "nettest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)10,			/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)1,			/* data */

/***** on board ethernet test(le0) *****/
  CPUGROUP,			/* group */
  ENET1,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Ethernet",			/* label */
  "le\0\0",			/* devname */
  "nettest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)10,			/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)1,			/* data */

/***** S-Bus token ring  *****/
  CPUGROUP,			/* group */
  TRNET,			/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Token Ring",			/* label */
  "tr\0\0",			/* devname */
  "nettest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)10,			/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)1,			/* data */

/***** S-BUS HSI(SunLink)test *****/
  CPUGROUP,			/* group */
  SBUS_HSI,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SunLink (HSI) #",		/* label */
  "HSI\0\0",			/* devname */
  "sunlink",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)0x40,                /* data (defaults to "0 1 2 3" loopback, */
                                /* no internal loopback and on-board clock) */

/***** sp(ttya & ttyb) test *****/
  CPUGROUP,			/* group */
  CPU_SP,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Serial Ports",		/* label */
  "zs\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)3,			/* data(defaults to a-b) */

/***** sp(ttyc & ttyd) test *****/
  CPUGROUP,			/* group */
  CPU_SP1,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Serial Ports",		/* label */
  "zs\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)3,			/* data(defaults to c-d) */

/***** on-board printer port test(pp) *****/
  CPUGROUP,			/* group */
  PP,				/* id */
  0,				/* unit */
  2,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Printer Port #",		/* label */
  "pp\0\0",			/* devname */
  "pptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** on-board audio port test(audio) *****/
  CPUGROUP,			/* group */
  AUDIO,			/* id */
  -1,				/* unit, single */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Audio Port",			/* label */
  "audio\0\0",			/* devname */
  "autest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)100,			/* special(volume) */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** monochrome test(bwtwo0) *****/
  CPUGROUP,                     /* group */
  BW2,                          /* id */
  0,                            /* unit */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Monochrome FB",              /* label */
  "bwtwo\0\0",                  /* devname */
  "mono",                       /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */
 
/***** color test(cgthree0) *****/
  CPUGROUP,	 		/* group */
  COLOR3,			/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Color FB",			/* label */
  "cgthree\0\0",		/* devname */
  "color",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** color test(cgfour0) *****/
  CPUGROUP,			/* group */
  COLOR4,			/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Color FB",			/* label */
  "cgfour\0\0",			/* devname */
  "color",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** color test(cgsix0) *****/
  CPUGROUP,                     /* group */
  COLOR6,                       /* id */
  0,                            /* unit */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Color FB",                   /* label */
  "cgsix\0\0",                  /* devname */
  "cg6test",                    /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */
 
/***** c24 test(cgeight) *****/
  CPUGROUP,			/* group */
  COLOR8,			/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Color FB",			/* label */
  "cgeight\0\0",		/* devname */
  "c24",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** egret (cg12) test *****/
  CPUGROUP,                     /* group */
  CG12,                         /* id */
  0,                            /* unit */
  0,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Color FB GS",                /* label */
  "cgtwelve\0\0",               /* devname */
  "cg12",                       /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  (char *) 1,                   /* special - 1 loop per board test*/
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  (char *) (1<<12),             /* data - 1 loop per function test */

/***** Hawk (gttest) *****/
  CPUGROUP,                     /* group */
  GT,                           /* id */
  0,                            /* unit */
  0,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Graphics Tower GT#",         /* label */
  "gt\0\0",                     /* devname */
  "gttest",                     /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  (char *) 1,                   /* special - 1 loop per board test*/
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  (char *) (0x027FFFF),        /* data - 1 loop per function test, 1023 is max */

/***** mptest (mp-4) test *****/
  CPUGROUP,                     /* group */
  MP4,                          /* id */
  0,                            /* unit */
  1,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  DISABLE,                      /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Multiprocessors",            /* label */
  "mp\0\0",                    /* devname */
  "mptest",                     /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                   	/* data - test all functions */

/***** spif test *****/
  CPUGROUP,			/* group */
  SPIF,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SPC/S #",			/* label */
  "stc\0\0",			/* devname */
  "spiftest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)0x103601,		/* data */

#ifdef sun386
/***** i387 test *****/
  CPUGROUP,                     /* group */
  I387,				/* id */
  -1,				/* unit, single */
  2,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  DISABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Floating Point Accelerator", /* label */
  "387i",			/* devname */
  "i387",                       /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */

/***** FPX/WEITEK test *****/
  DEVGROUP,                     /* group */
  FPX,                          /* id */
  -1,				/* unit, single */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Sun 386i FPX",               /* label */
  "fpx",			/* devname */
  "fpx",                        /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */
/***** V7-VGA test *****/
  DEVGROUP,                     /* group */
  SUNVGA,                       /* id */
  -1,				/* unit, single */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "SunVGA",                     /* label */
  "vga",			/* devname */
  "sunvga",                     /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */
#endif

/***** 2nd ethernet test(ie1) *****/
  DEVGROUP,			/* group */
  ENET2,			/* id */
  -1,				/* unit, single(with non-zero device number) */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Ethernet",			/* label */
  "ie\0\0",			/* devname */
  "nettest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)10,			/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)1,			/* data */

/***** Interfaces's Network Coprocessor: OMNI board ethernet test(ne0) *****/
  DEVGROUP,                     /* group */
  OMNI_NET,                     /* id */
  0,                            /* unit */
  0,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Ethernet",                   /* label */
  "ne\0\0",                     /* devname */
  "nettest",                    /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  (char *)10,                   /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  (char *)1,                    /* data */


/***** fddi *****/
  DEVGROUP,			/* group */
  FDDI,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Fddi",			/* label */
  "fddi\0\0",			/* devname */
  "nettest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)10,			/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)1,			/* data */

/***** flamingo test(tvone0) *****/
  DEVGROUP,			/* group */
  TV1,				/* id */
  0,				/* unit */
  0,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SunVideo",			/* label */
  "tvone\0\0",			/* devname */
  "tv1test",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** color test(cgtwo) *****/
  DEVGROUP,			/* group */
  COLOR2,			/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Color FB",			/* label */
  "cgtwo\0\0",			/* devname */
  "color",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

#ifdef sun386
/***** color test RRacer (cgfive) *****/
  DEVGROUP,         /* group */
  COLOR5,           /* id */
  0,                /* unit */
  0,                /* selection type */
  0,                /* popup */
  1,                /* test_no */
  1,                /* which_test */
  ENABLE,           /* dev_enable */
  ENABLE,           /* enable */
  0,                /* pass */
  0,                /* error */
  "10",             /* priority */
  0,                /* pid */
  "RRColor FB",     /* label */
  "cgfive\0\0",     /* devname */
  "color",          /* testname */
  NULL,             /* tail */
  NULL,             /* environ */
  NULL,             /* env */
  NULL,             /* special */
  NULL,             /* conf */
  NULL,             /* select */
  NULL,             /* option */
  NULL,             /* msg */
  NULL,             /* data */

#else	sun386

/***** color test(cgfive) *****/
  DEVGROUP,			/* group */
  COLOR5,			/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Double Color FB",		/* label */
  "cgtwo\0\0",			/* devname */
  "cg5test",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */
#endif	sun386

/***** color test(cgnine0) *****/
  DEVGROUP,                     /* group */
  COLOR9,                       /* id */
  0,                            /* unit */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Color FB",                   /* label */
  "cgnine\0\0",                 /* devname */
  "c24",                  	/* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */
 
/***** FPA test *****/
  DEVGROUP,			/* group */
  FPA,				/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Floating Point Accelerator",	/* label */
  "fpa\0\0",			/* devname */
  "fpatest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** FPA3X test *****/
  DEVGROUP,                     /* group */
  FPA3X,                        /* id */
  0,                            /* unit */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Floating Point Accelerator 3x", /* label */
  "fpa\0\0",                    /* devname */
  "fpatest",                    /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */

/***** sky test *****/
  DEVGROUP,			/* group */
  SKY,				/* id */
  0,				/* unit */
  0,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SKY Board",			/* label */
  "sky\0\0",			/* devname */
  "ffpusr",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** gp&b test *****/
  DEVGROUP,			/* group */
  GP,				/* id */
  0,				/* unit */
  1,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "GP/GB",			/* label */
  "gpone\0\0",			/* devname */
  "gpmtest",			/* testname */
  NULL,				/* tail */
  "SYSDIAG_DIRECTORY=.",	/* environ */
  "SYSDIAG_DIRECTORY=.",	/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** gp2 test *****/
  DEVGROUP,			/* group */
  GP2,				/* id */
  0,				/* unit */
  1,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "GP2",			/* label */
  "gpone\0\0",			/* devname */
  "gp2test",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** mti(ALM) test *****/
  DEVGROUP,			/* group */
  MTI,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "ALM #",			/* label */
  "mti\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** mcp(ALM2, SCP2, printer) test *****/
  DEVGROUP,			/* group */
  MCP,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  1,				/* which_test */
  DISABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "ALM2 #",			/* label */
  "mcp\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** printer test *****/
  DEVGROUP,			/* group */
  PRINTER,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  2,				/* test_no */
  2,				/* which_test */
  DISABLE,			/* dev_enable */
  ENABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "MCP Printer #",		/* label */
  "mcpp\0\0",			/* devname */
  "printer",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** SCP2(SunLink) test *****/
  DEVGROUP,			/* group */
  SCP2,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SunLink(MCP) #",		/* label */
  "mcph\0\0",			/* devname */
  "sunlink",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** DCP(SunLink)test *****/
  DEVGROUP,			/* group */
  SCP,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SunLink(DCP) #",		/* label */
  "dcp\0\0",			/* devname */
  "sunlink",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** HSI(SunLink)test *****/
  DEVGROUP,			/* group */
  HSI,				/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SunLink(HSI) #",		/* label */
  "hs\0\0",			/* devname */
  "sunlink",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (caddr_t)3,			/* data(defaults to 0-1, 2-3 loopback) */

/***** SCSI sp test #1 *****/
  DEVGROUP,			/* group */
  SCSISP1,			/* id */
  -1,				/* unit, single(with non-zero device number) */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI ttys",			/* label */
  "zs\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** SCSI sp test #2 *****/
  DEVGROUP,			/* group */
  SCSISP2,			/* id */
  -1,				/* unit, single(with non-zero device number) */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI ttyt",			/* label */
  "zs\0\0",			/* devname */
  "sptest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** TAAC test *****/
  DEVGROUP,			/* group */
  TAAC,				/* id */
  0,				/* unit */
  1,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "TAAC",			/* label */
  "taac\0\0",			/* devname */
  "taactest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL, 			/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** prestoserve test *****/
  DEVGROUP,                     /* group */
  PRESTO,                       /* id */
  0,                            /* unit */
  0,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                      /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "NFS Accelerator",            /* label */
  "pr\0\0",                     /* devname */
  "pstest",                     /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */

/***** snapshot test *****/
  DEVGROUP,                     /* group */
  VFC,                          /* id */
  0,                            /* unit */
  0,                            /* selection type */
  0,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  ENABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Vid Frame Capture",        	/* label */
  "vfc\0\0",                    /* devname */
  "vfctest",                    /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  NULL,                         /* data */

/***** Zebra (SBus Printer) bpp test - Bidirectional Parallel port *****/
  DEVGROUP,                     /* group */
  ZEBRA1,                       /* id */
  0,                            /* unit */
  2,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  DISABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Parallel Port",            /* label */
  "bpp\0\0",                    /* devname */
  "bpptest",                    /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  (char *) (0<<2),              /* data - fast mode */ 

/***** Zebra (SBus Printer) lpvi test - Serial Video port *****/
  DEVGROUP,                     /* group */
  ZEBRA2,                       /* id */
  0,                            /* unit */
  2,                            /* selection type */
  1,                            /* popup */
  1,                            /* test_no */
  1,                            /* which_test */
  ENABLE,                       /* dev_enable */
  DISABLE,                       /* enable */
  0,                            /* pass */
  0,                            /* error */
  "10",                         /* priority */
  0,                            /* pid */
  "Video Port",  	     	/* label */
  "lpvi\0\0",                   /* devname */
  "lpvitest",                     /* testname */
  NULL,                         /* tail */
  NULL,                         /* environ */
  NULL,                         /* env */
  NULL,                         /* special */
  NULL,                         /* conf */
  NULL,                         /* select */
  NULL,                         /* option */
  NULL,                         /* msg */
  (char *) (0<<2),              /* data - fast mode */

/***** magnetic tape test(mt) *****/
  TAPEGROUP,			/* group */
  MAGTAPE1,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Mag Tape #",			/* label */
  "mt\0\0",			/* devname */
  "tapetest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)25300,		/* special, 200 big blocks + 100 small blocks */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)0x190040,		/* data */

/***** magnetic tape test(xt) *****/
  TAPEGROUP,			/* group */
  MAGTAPE2,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Mag Tape #",			/* label */
  "xt\0\0",			/* devname */
  "tapetest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)25300,		/* special, 200 big blocks + 100 small blocks */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)0x190040,		/* data */

/***** SCSI tape test *****/
  TAPEGROUP,			/* group */
  SCSITAPE,			/* id */
  0,				/* unit */
  2,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "SCSI Tape #",		/* label */
  "st\0\0",			/* devname */
  "tapetest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  (char *)25300,		/* special, 200 big blocks + 100 small blocks */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  (char *)0xa0042,		/* data */

/***** ipc test *****/
  IPCGROUP,			/* group */
  IPC,				/* id */
  0,				/* unit */
  1,				/* selection type */
  1,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  "Sun-IPC #",			/* label */
  "pc\0\0",			/* devname */
  "ipctest",			/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

/***** user-defined tests *****/
  USRGROUP,			/* group */
  USER,				/* id */
  -1,				/* unit, single */
  1,				/* selection type */
  0,				/* popup */
  1,				/* test_no */
  1,				/* which_test */
  ENABLE,			/* dev_enable */
  DISABLE,			/* enable */
  0,				/* pass */
  0,				/* error */
  "10",				/* priority */
  0,				/* pid */
  NULL,				/* label */
  "user",			/* devname */
  NULL,				/* testname */
  NULL,				/* tail */
  NULL,				/* environ */
  NULL,				/* env */
  NULL,				/* special */
  NULL,				/* conf */
  NULL,				/* select */
  NULL,				/* option */
  NULL,				/* msg */
  NULL,				/* data */

};

#define	NTESTS	sizeof(tests_base)/sizeof(struct test_info)


/******************************************************************************
 * Internal data structure of all possible test groups.			      *
 ******************************************************************************/
struct	group_info groups[]=
{

/***** memory tests group(group id: 0) *****/
  "MEMORY DEVICES",		/* c_label */
  "MEMORY DEVICE TESTS:",	/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** disk tests group(goup id: 1) *****/
  "DISK DEVICES",		/* c_label */
  "DISK DEVICE TESTS:",		/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** CPU devices group(group id: 2) *****/
  "CPU DEVICES",		/* c_label */
  "CPU DEVICE TESTS:", 		/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** other device tests group(group id: 3) *****/
  "OTHER DEVICES",		/* c_label */
  "OTHER DEVICE TESTS:", 	/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** tape drive tests group(group id: 4) *****/
  "TAPE DEVICES",		/* c_label */
  "TAPE DEVICE TESTS:", 	/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** IPC board tests group(group id: 5) *****/
  "IPC BOARDS",			/* c_label */
  "IPC BOARD TESTS:", 		/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */

/***** user-defined tests group(group id: 6) *****/
  "USER TESTS",			/* c_label */
  "USER TESTS:", 		/* s_label */
  2,				/* max */
  0,				/* no_tests */
  DISABLE,			/* enable */
  -1,				/* first */
  -1,				/* last */
  NULL,				/* select */
  NULL,				/* msg */
};

#define	NGROUPS	sizeof(groups)/sizeof(struct group_info)
int	ngroups=NGROUPS;	/* total # of defined groups */
