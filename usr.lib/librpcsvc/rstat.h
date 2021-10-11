/*	@(#)rstat.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef _rpcsvc_rstat_h
#define _rpcsvc_rstat_h

#ifndef CPUSTATES
#include <sys/dk.h>
#endif

#include <rpc/types.h>

struct stats {				/* version 1 */
	int cp_time[4];
	int dk_xfer[4];
	u_int v_pgpgin;		/* these are cumulative sum */
	u_int v_pgpgout;
	u_int v_pswpin;
	u_int v_pswpout;
	u_int v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
};
typedef struct stats stats;
bool_t xdr_stats();

struct statsswtch {				/* version 2 */
	int cp_time[4];
	int dk_xfer[4];
	u_int v_pgpgin;
	u_int v_pgpgout;
	u_int v_pswpin;
	u_int v_pswpout;
	u_int v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	u_int v_swtch;
	long avenrun[3];
	struct timeval boottime;
};
typedef struct statsswtch statsswtch;
bool_t xdr_statsswtch();

struct statstime {				/* version 3 */
	int cp_time[4];
	int dk_xfer[4];
	u_int v_pgpgin;
	u_int v_pgpgout;
	u_int v_pswpin;
	u_int v_pswpout;
	u_int v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	u_int v_swtch;
	long avenrun[3];
	struct timeval boottime;
	struct timeval curtime;
};
typedef struct statstime statstime;
bool_t xdr_statstime();

struct statsvar {				/* version 4 */
	struct {
		u_int cp_time_len;
		int *cp_time_val;
	} cp_time;			/* variable sized */
	struct {
		u_int dk_xfer_len;
		int *dk_xfer_val;
	} dk_xfer;			/* variable sized */
	u_int v_pgpgin;
	u_int v_pgpgout;
	u_int v_pswpin;
	u_int v_pswpout;
	u_int v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	u_int v_swtch;
	long avenrun[3];
	struct timeval boottime;
	struct timeval curtime;
};
typedef struct statsvar statsvar;
bool_t xdr_statsvar();

#define RSTATPROG ((u_long)100001)
#define RSTATVERS_SWTCH ((u_long)2)
#define RSTATPROC_STATS ((u_long)1)
extern statsswtch *rstatproc_stats_2();
#define RSTATPROC_HAVEDISK ((u_long)2)
extern long *rstatproc_havedisk_2();
#define RSTATVERS_TIME ((u_long)3)
extern statstime *rstatproc_stats_3();
extern long *rstatproc_havedisk_3();
#define RSTATVERS_VAR ((u_long)4)
extern statsvar *rstatproc_stats_4();
extern long *rstatproc_havedisk_4();
int havedisk();

#endif /*!_rpcsvc_rstat_h*/
