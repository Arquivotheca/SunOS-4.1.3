#ifndef _HK_COMM_
#define _HK_COMM_

/*
 ***********************************************************************
 *
 * hk_comm.h
 *
 * @(#)hk_comm.h 11.1 91/05/21 19:52:28
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Header file for Hawk communication routines.
 *
 * 11-May-89 Kevin C. Rushforth	  Initial version created.
 *  6-Sep-89 Kevin C. Rushforth	  Updates for shared memory.
 * 12-Sep-89 Nelson, Rushforth	  Add virtual communication register offsets.
 * 16-Jan-90 Scott R. Nelson	  Revisions to vcomm.
 * 15-Mar-90 Kevin C. Rushforth	  Pulled out obsolete MASTER/SLAVE constants.
 * 27-Apr-90 Kevin C. Rushforth	  Added room for kernel print buffer.
 * 18-May-90 Josh Lee		  Added a socket connection file for sim860.
 * 30-Jul-90 Kevin C. Rushforth	  hk_connect now returns an int.
 * 16-Aug-90 Vic Tolomei          Add size of print_buffer, note fact that
 *				  HK_VM_* are OFFSETS, not sizes
 * 28-Aug-90 Ranjit Oberoi	  Added a picking socket.
 * 12-Sep-90 Vic Tolomei	  Removed PRINT_STRING_BUF_SIZE, in gtmcb.h
 *  3-Apr-91 Kevin C. Rushforth	  Added hk_ioctl.
 *
 ***********************************************************************
 */

/* Global constants */
#define HK_TMP_DIR		"/usr/tmp"

/* This connects a socket to stdout (because     */
/* sim860 programs cannot go directly to stdout) */
#define HK_SO_SOCK_NAME		"sockso"	/* Socket connection file */

/* Commands from FE (or host) to FB */
#define HK_FB_SOCK_NAME		"sockfb"	/* Socket connection file */
#define HK_FB_PICK_SOCK_NAME    "sockpi"        /* Socket connection file */
#define HK_FEFB_NOP		0x00
#define HK_FEFB_READ_PBM	0x01
#define HK_FEFB_WRITE_PBM	0x02

/* Commands from HOST to FE */
#define HK_KM_SHMEM_NAME	"hawkkm"	/* Kernel shared memory file */
#define HK_VM_SHMEM_NAME	"hawkvm"	/* User shared memory file */
#define HK_VM_SOCK_NAME		"sockvm"	/* Socket connection file */
#define HK_XCPU_SOCK_NAME	"sockxc"	/* XCPU socket connection */

/* Reserved locations (fixed byte offsets from the start of VM for now) */
#define HK_VM_HOST_FLAG_0	0x0000		/* Hardware flag bits... */
#define HK_VM_HOST_FLAG_1	0x0004		/* ...1 bit per word */
#define HK_VM_HOST_FLAG_2	0x0008
#define HK_VM_HOST_FLAG_3	0x000c
#define HK_VM_HAWK_FLAG_0	0x0010
#define HK_VM_HAWK_FLAG_1	0x0014
#define HK_VM_HAWK_FLAG_2	0x0018
#define HK_VM_HAWK_FLAG_3	0x001c

/* Message communication block for commands and data between kernel and Hawk */
#define HK_VM_KERNEL_MCB	0x0100		/* offset kernel mcb */

/* Buffer used by Hawk for print mesasges (fixed at 4K) */
#define HK_VM_PRINT_BUFFER	0x1000		/* offset vcom area (bytes) */
						/* size = HKKVS_PRINT_STRING_SIZE */

/* ########## Still needs an area for WLUT and CLUT updates ########## */

/* Total size of kernel area */
#define HK_VM_VCOM_SIZE		0x2000		/* Size of vcom area (bytes) */

/* External procedures */
extern int hk_open();
extern int hk_ioctl();
extern char *hk_mmap();
extern int hk_vmalloc();
extern int hk_connect();
extern int hk_munmap();
extern int hk_disconnect();
extern void hk_close();

#endif _HK_COMM_
