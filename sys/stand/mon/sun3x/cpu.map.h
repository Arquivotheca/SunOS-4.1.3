/*
 * @(#)cpu.map.h 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 *******************************************************************
 * cpu.map.h - Memory Mapping and Paging on the Sun 3x
 *
 * This file is used for all standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  
 *
 *******************************************************************
 */
#ifndef ADRSPC_SIZE

/*
 * The address space available to a single process is 256 Megabytes.
 */
#define	ADRSPC_SIZE	0x10000000
 
 
/*
 * Each page can be mapped to memory, I/O, or a global bus (eg, P4 or VMEbus)
 * or can be inaccessible (INVALID).  Pages can be made Read/Writable,
 * set for Cachinhibit, Marked Used, Write-protect, or Modified, and Contain
 * a 2 bit (DT) Descriptor Type Field:
 *
 * The struct pgmapent holds the bit fields for a normal short page
 * descriptor used in the 68030 PMMU page table.
 */
struct pgmapent {
        unsigned        pm_page         :19;    /* Page # in physical memory */
        unsigned                        :6;     /* #7-#12 Not used           */
        unsigned        pm_cacheinhibit :1;     /* #6 Cache Inhibit          */
        unsigned                        :1;     /* #5 Reserved               */
        unsigned        pm_modified     :1;     /* #4 Modified bit           */
        unsigned        pm_used         :1;     /* #3 Used Bit               */
        unsigned        pm_wrpro        :1;     /* #2 Write Protect Bit      */
        unsigned        pm_dt		:2;     /* #0-#1 Descriptor Type     */
};
 
/*
 * The Following are the Sun-3x function code defines.
 */
#define	FC_UD		1	/* User Data */
#define	FC_UP		2	/* User Program */
#define	FC_USER		3	/* Sun 3x User Reserve Space */
#define	FC_SD		5	/* Supervisor Data */
#define	FC_SP		6	/* Supervisor Program */
#define	FC_CPU		7	/* CPU Space (Int Ack, Co-processors, ...) */
#endif ADRSPC_SIZE
