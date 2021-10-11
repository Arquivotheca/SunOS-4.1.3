#ident	"@(#)commands.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_impl_commands_h
#define	_scsi_impl_commands_h

/*
 * Implementation dependent (i.e., Vendor Unique) command definitions.
 * This file is included by <scsi/generic/commands.h>
 */

/*
 *
 * Private Vendor Unique Commands
 *
 */

#ifdef	ACB4000
#define	SCMD_TRANSLATE		0x0F
#endif	/* ACB4000 */

/*
 *
 * Implementation dependent view of a SCSI command descriptor block
 *
 * (Original attribution: Kevin Sheehan, Sun Consulting)
 */

/*
 * Standard SCSI control blocks definitions.
 *
 * These go in or out over the SCSI bus.
 *
 * The first 11 bits of the command block are the same for all three
 * defined command groups.  The first byte is an operation which consists
 * of a command code component and a group code component. The first 3 bits
 * of the second byte are the unit number.
 *
 * The group code determines the length of the rest of the command.
 * Group 0 commands are 6 bytes, Group 1 are 10 bytes, and Group 5
 * are 12 bytes.  Groups 2-4 are Reserved. Groups 6 and 7 are Vendor
 * Unique.
 *
 */

/*
 * At present, our standard cdb's will reserve enough space for
 * use with up to Group 5 commands. This may have to change soon
 * if optical disks have 20 byte or longer commands. At any rate,
 * the Sun SCSI implementation has no problem handling arbitrary
 * length commands; it is just more efficient to declare it as
 * certain size (to avoid runtime allocation overhead).
 */

#define	CDB_SIZE	CDB_GROUP5

union scsi_cdb {		/* scsi command description block */
	struct {
		u_char	cmd;		/* cmd code (byte 0) */
		u_char	lun	:3,	/* lun (byte 1) */
			tag	:5;	/* rest of byte 1 */
		union {			/* bytes 2 - 31 */

		u_char	scsi[CDB_SIZE-2];
		/* 
	 	 *	G R O U P   0   F O R M A T (6 bytes)
		 */
#define		scc_cmd		cdb_un.cmd
#define		scc_lun		cdb_un.lun
#define		g0_addr2	cdb_un.tag
#define		g0_addr1	cdb_un.sg.g0.addr1
#define		g0_addr0	cdb_un.sg.g0.addr0
#define		g0_count0	cdb_un.sg.g0.count0
#define		g0_vu_1		cdb_un.sg.g0.vu_57
#define		g0_vu_0		cdb_un.sg.g0.vu_56
#define		g0_flag		cdb_un.sg.g0.flag
#define		g0_link		cdb_un.sg.g0.link
	/*
	 * defines for SCSI tape cdb.
	 */
#define		t_code		cdb_un.tag
#define		high_count	cdb_un.sg.g0.addr1
#define		mid_count	cdb_un.sg.g0.addr0
#define		low_count	cdb_un.sg.g0.count0
		struct scsi_g0 {
			u_char addr1;		/* middle part of address */
			u_char addr0;		/* low part of address */
			u_char count0;		/* usually block count */
			u_char vu_57	:1;	/* vendor unique (byte 5 bit 7*/
			u_char vu_56	:1;	/* vendor unique (byte 5 bit 6*/
			u_char rsvd	:4;	/* reserved */
			u_char flag	:1;	/* interrupt when done */
			u_char link	:1;	/* another command follows */
		} g0;


		/*
		 *	G R O U P   1   F O R M A T  (10 byte)
		 */
#define		g1_reladdr	cdb_un.tag
#define		g1_rsvd0	cdb_un.sg.g1.rsvd1
#define		g1_addr3	cdb_un.sg.g1.addr3	/* msb */
#define		g1_addr2	cdb_un.sg.g1.addr2
#define		g1_addr1	cdb_un.sg.g1.addr1
#define		g1_addr0	cdb_un.sg.g1.addr0	/* lsb */
#define		g1_count1	cdb_un.sg.g1.count1	/* msb */
#define		g1_count0	cdb_un.sg.g1.count0	/* lsb */
#define		g1_vu_1		cdb_un.sg.g1.vu_97
#define		g1_vu_0		cdb_un.sg.g1.vu_96
#define		g1_flag		cdb_un.sg.g1.flag
#define		g1_link		cdb_un.sg.g1.link
		struct scsi_g1 {
			u_char addr3;		/* most sig. byte of address*/
			u_char addr2;
			u_char addr1;
			u_char addr0;
			u_char rsvd1;		/* reserved (byte 6) */
			u_char count1;		/* transfer length (msb) */
			u_char count0;		/* transfer length (lsb) */
			u_char vu_97	:1;	/* vendor unique (byte 9 bit 7*/
			u_char vu_96	:1;	/* vendor unique (byte 9 bit 6*/
			u_char rsvd0	:4;	/* reserved */
			u_char flag	:1;	/* interrupt when done */
			u_char link	:1;	/* another command follows */
		} g1;


		/*
		 *	G R O U P   5   F O R M A T  (12 byte)
		 */
#define		scc5_reladdr	cdb_un.tag
#define		scc5_addr3	cdb_un.sg.g5.addr3	/* msb */
#define		scc5_addr2	cdb_un.sg.g5.addr2
#define		scc5_addr1	cdb_un.sg.g5.addr1
#define		scc5_addr0	cdb_un.sg.g5.addr0	/* lsb */
#define		scc5_count1	cdb_un.sg.g5.count1	/* msb */
#define		scc5_count0	cdb_un.sg.g5.count0	/* lsb */
#define		scc5_vu_1	cdb_un.sg.g5.v1
#define		scc5_vu_0	cdb_un.sg.g5.v0
#define		scc5_flag	cdb_un.sg.g5.flag
		struct scsi_g5 {
			u_char addr3;	/* most sig. byte of address*/
			u_char addr2;
			u_char addr1;
			u_char addr0;
			u_char rsvd3;	/* reserved */
			u_char rsvd2;	/* reserved */
			u_char rsvd1;	/* reserved */
			u_char count1;	/* transfer length (msb) */
			u_char count0;	/* transfer length (lsb) */
			u_char vu_117	:1; /* vendor unique (byte 11 bit 7*/
			u_char vu_116	:1; /* vendor unique (byte 11 bit 6*/
			u_char rsvd0	:4; /* reserved */
			u_char flag	:1; /* interrupt when done */
			u_char link	:1; /* another command follows */
		} g5;
		}sg;
	} cdb_un;
	u_char cdb_opaque[CDB_SIZE];	/* addressed as an opaque char array */
	u_long cdb_long[CDB_SIZE / sizeof (long)]; /* as a longword array */
};

/*
 *	Various useful Macros for SCSI commands
 */

/*
 * defines for getting/setting fields within the various command groups
 */

#define	GETCMD(cdb)		((cdb)->scc_cmd & 0x1F)
#define	GETGROUP(cdb)		(CDB_GROUPID((cdb)->scc_cmd))

#define FORMG0COUNT(cdb, cnt)	(cdb)->g0_count0  = (cnt)
#define FORMG0ADDR(cdb, addr) 	(cdb)->g0_addr2  = (addr) >> 16;\
				(cdb)->g0_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->g0_addr0  = (addr) & 0xFF
#define	GETG0ADDR(cdb)		(((cdb)->g0_addr2 & 0x1F) << 16) + \
		 		((cdb)->g0_addr1 << 8) + ((cdb)->g0_addr0)
#define	GETG0TAG(cdb)		((cdb)->g0_addr2)

#define FORMG0COUNT_S(cdb,cnt) 	(cdb)->high_count  = (cnt) >> 16;\
				(cdb)->mid_count = ((cnt) >> 8) & 0xFF;\
				(cdb)->low_count= (cnt) & 0xFF

#define FORMG1COUNT(cdb, cnt)	(cdb)->g1_count1 = ((cnt) >> 8);\
				(cdb)->g1_count0 = (cnt) & 0xFF
#define FORMG1ADDR(cdb, addr)	(cdb)->g1_addr3  = (addr) >> 24;\
				(cdb)->g1_addr2  = ((addr) >> 16) & 0xFF;\
				(cdb)->g1_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->g1_addr0  = (addr) & 0xFF
#define	GETG1ADDR(cdb)		((cdb)->g1_addr3 << 24) + \
				((cdb)->g1_addr2 << 16) + \
				((cdb)->g1_addr1 << 8)  + \
				((cdb)->g1_addr0)
#define	GETG1TAG(cdb)		(cdb)->g1_reladdr

#define FORMG5COUNT(cdb, cnt)	(cdb)->scc5_count1 = ((cnt) >> 8);\
				(cdb)->scc5_count0 = (cnt) & 0xFF
#define FORMG5ADDR(cdb, addr)	(cdb)->scc5_addr3  = (addr) >> 24;\
				(cdb)->scc5_addr2  = ((addr) >> 16) & 0xFF;\
				(cdb)->scc5_addr1  = ((addr) >> 8) & 0xFF;\
				(cdb)->scc5_addr0  = (addr) & 0xFF
#define	GETG5ADDR(cdb)		((cdb)->scc5_addr3 << 24) + \
				((cdb)->scc5_addr2 << 16) + \
				((cdb)->scc5_addr1 << 8)  + \
				((cdb)->scc5_addr0)
#define	GETG5TAG(cdb)		(cdb)->scc5_reladdr

/*
 * Shorthand macros for forming commands
 */

#define	MAKECOM_COMMON(pktp, devp, flag, cmd)	\
	(pktp)->pkt_address = (devp)->sd_address, \
	(pktp)->pkt_flags = (flag), \
	((union scsi_cdb *)(pktp)->pkt_cdbp)->scc_cmd = (cmd), \
	((union scsi_cdb *)(pktp)->pkt_cdbp)->scc_lun = \
	    (pktp)->pkt_address.a_lun

#define	MAKECOM_G0(pktp, devp, flag, cmd, addr, cnt)	\
	MAKECOM_COMMON((pktp), (devp), (flag), (cmd)), \
	FORMG0ADDR(((union scsi_cdb *)(pktp)->pkt_cdbp), (addr)), \
	FORMG0COUNT(((union scsi_cdb *)(pktp)->pkt_cdbp), (cnt))


#define	MAKECOM_G0_S(pktp, devp, flag, cmd, cnt, fixbit)	\
	MAKECOM_COMMON((pktp), (devp), (flag), (cmd)), \
	FORMG0COUNT_S(((union scsi_cdb *)(pktp)->pkt_cdbp), (cnt)), \
	((union scsi_cdb *)(pktp)->pkt_cdbp)->t_code = (fixbit)


#define	MAKECOM_G1(pktp, devp, flag, cmd, addr, cnt)	\
	MAKECOM_COMMON((pktp), (devp), (flag), (cmd)), \
	FORMG1ADDR(((union scsi_cdb *)(pktp)->pkt_cdbp), (addr)), \
	FORMG1COUNT(((union scsi_cdb *)(pktp)->pkt_cdbp), (cnt))


#define	MAKECOM_G5(pktp, devp, flag, cmd, addr, cnt)	\
	MAKECOM_COMMON((pktp), (devp), (flag), (cmd)), \
	FORMG5ADDR(((union scsi_cdb *)(pktp)->pkt_cdbp), (addr)), \
	FORMG5COUNT(((union scsi_cdb *)(pktp)->pkt_cdbp), (cnt))


#ifdef	KERNEL
extern void makecom_g0(), makecom_g0_s(), makecom_g1(), makecom_g5();
#endif

#endif	/* !_scsi_impl_commands_h */
