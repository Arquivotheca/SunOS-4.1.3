/******* @(#)vfc_defs.h 1.1 7/30/92 *********/
/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _VIDEOPICS_VFC_DEFS_H
#define	_VIDEOPICS_VFC_DEFS_H

#ident	"@(#)vfc_defs.h	1.4	90/12/12 SMI"

#define DEVSIZE 0x8000

	/* Defines for computing virtual addresses */
#define CSR_VADDR(addr)     ((unsigned)addr + 0x4000)
#define PORT1_VADDR(addr)   ((unsigned)addr + 0x5000)
#define PORT2_VADDR(addr)   ((unsigned)addr + 0x7000)
#define IICDATA_VADDR(addr) ((unsigned)addr + 0x6000)
#define IICCSR1_VADDR(addr) ((unsigned)addr + 0x6008)
#define IICCSR2_VADDR(addr) ((unsigned)addr + 0x600c)
#define IICOWN_VADDR(addr)  ((unsigned)addr + 0x6004)

	/* Board CSR bit field */
struct devcsr {
  unsigned reset  :1;		/* Board reset */
  unsigned alive  :1;		/* Board alive bit */
  unsigned memrst :1;		/* FIFO ptr reset */
  unsigned normal :1;		/* normal/diag mode */
  unsigned capstat:1;		/* capture status */
  unsigned capack :1;		/* capture ack */
  unsigned capcmd :1;		/* capture cmd */
  unsigned        :25;		/* unused */
};

struct rcv_data {
  u_int status;
  int error;
};


/* Defines related to IIC registers */

#define ACCESS_IVR    0x10000000
#define ACCESS_CLK    0x20000000
#define ACCESS_DATA   0x40000000
#define BUS_NOT_BUSY  0x01000000
#define XMIT_START    0xc4000000
#define XMIT_STOP     0xc2000000
#define CLK_BITS      0x14000000
#define VID_ADDR      0x8a000000
#define WR_DIR        0x00000000
#define RD_DIR        0x01000000
#define XFER_IS_ON    0x80000000
#define MAX_XFER_TIME 0x60000

	/* Video Defines */
#define NUM_VIDREGS   13

static u_int vidntsc_init[] =
{
  0x00000000,
  0x64000000,
  0x72000000,
  0x52000000,
  0x36000000,
  0x18000000,
  0xff000000,
  0x20000000,
  0xfc000000,
  0x77000000,
  0xe3000000,
  0x50000000,
  0x3e000000,
};

#define CAPTURE_DONE	0x08000000

/* Misc. defines */
#define	TRUE		1
#define FALSE		0


/* Defines for time */
#define SEC_2		2000000
#define MICROSEC_100	100
#define MICROSEC_500	500

#endif /* !_VIDEOPICS_VFC_DEFS_H */
