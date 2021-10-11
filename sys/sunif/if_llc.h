/*      @(#)if_llc.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* 
 * Structure of a IEEE 802.2 LLC header
 */
struct llc_hdr1 {
	u_char	d_lsap;		/* destination link service access point */
	u_char	s_lsap;		/* source link service access point */
	u_char	control;	/* short control field */
};

#define LLC_HDR1_LEN	3	/* DON'T use sizeof(struct llc_hdr1) */

struct llc_hdr2 {
	u_char	d_lsap;		/* destination link service access point */
	u_char	s_lsap;		/* source link service access point */
	u_short	control;	/* long control field */
};

#define LLC_HDR2_LEN	4	/* DON'T use sizeof(struct llc_hdr2) */

struct llc_snap_hdr {
	u_char	d_lsap;		/* destination link service access point */
	u_char	s_lsap;		/* source link service access point */
	u_char	control;	/* short control field */
	u_char	org[3];		/* Ethernet style organization field */
	u_short	type;		/* Ethernet style type field */
};

struct llc_xid {
	u_char	d_lsap;		/* destination link service access point */
	u_char	s_lsap;		/* source link service access point */
	u_char	control;	/* short control field */
	u_char	xid_formatid;	/* usually 0x81 for IEEE */
	u_char	supported_class;/* 1 for llc.1, 3 for llc.2 */
	u_char	llc2_window;	/* window size, *2 */
};

#define LLC_XID_LEN	6	/* DON'T use sizeof(struct llc_hdr1) */

#define LLC_SNAP_HDR_LEN	8	/* DON'T use sizeof(struct foo) */

struct llc_softc {		/* llc per instance data */
	struct ifnet llc_if;		/* MUST BE FIRST */
	int	llc_st_state;		/* current station state of llc FSM */
#define 	LLC_DOWN	0
#define 	LLC_UP 		1
#define 	LLC_CHECK	2
	int	llc_setmtu		/* maximum transport unit allowed */
};

#define CNTL_LLC_UI	0x03	/* un-numbered information packet */
#define CNTL_LLC_XID	0xaf	/* XID packet */
#define CNTL_LLC_TST	0xe3	/* TEST packet */

#define LLC_RESP	0x01	/* Response bit - add to the SSAP */
#define CNTL_LLC_PF_BIT	0x10	/* position of the Poll / Final bit */

#define LLC_XID_FORMAT	0x81	/* IEEE format */
/*
 * Useful macros.
 */

#define m_802addr(m)	mtod(m, struct sockaddr *)
#define llcprintf 	if(llc_debug)printf

