#ident	"@(#)scsi_subr.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 Sun Microsystems, Inc.
 */

#include <scsi/scsi.h>

/*
 *
 * Utility SCSI routines
 *
 */

/*
 * Polling support routines
 */

/*
 * The polling command routine still needs work
 */

static int scsi_poll_busycnt = 60;

int
scsi_poll(pkt)
struct scsi_pkt *pkt;
{
	register busy_count, rval = -1, savef;
	void (*savec)();

	/*
	 * save old flags..
	 */

	savef = pkt->pkt_flags;
	savec = pkt->pkt_comp;
	pkt->pkt_flags |= FLAG_NOINTR;
	pkt->pkt_comp = scsi_pollintr;
	for (busy_count = 0; busy_count < scsi_poll_busycnt; busy_count++) {

		if (pkt_transport(pkt) != TRAN_ACCEPT) {
			break;
		}
		if (pkt->pkt_reason == CMD_INCOMPLETE && pkt->pkt_state == 0) {
			DELAY(10000);
		} else if (pkt->pkt_reason != CMD_CMPLT) {
			break;
		} else if (((*pkt->pkt_scbp)&STATUS_MASK) == STATUS_BUSY) {
			DELAY(1000000);
		} else {
			rval = 0;
			break;
		}
	}

	pkt->pkt_flags = savef;
	pkt->pkt_comp = savec;
	if (busy_count >= scsi_poll_busycnt && rval == 0)
		return (busy_count);
	else
		return (rval);
}

/*ARGSUSED*/
void
scsi_pollintr(pkt)
struct scsi_pkt *pkt;
{
}

/*
 * Command packaging routines (here for compactness rather than speed)
 */


void
makecom_g0(pkt, devp, flag, cmd, addr, cnt)
struct scsi_pkt *pkt;
struct scsi_device *devp;
int flag, cmd, addr, cnt;
{
	MAKECOM_G0(pkt, devp, flag, cmd, addr, cnt);
}

void
makecom_g0_s(pkt, devp, flag, cmd, cnt, fixbit)
struct scsi_pkt *pkt;
struct scsi_device *devp;
int flag, cmd, cnt, fixbit;
{
	MAKECOM_G0_S(pkt, devp, flag, cmd, cnt, fixbit);
}

void
makecom_g1(pkt, devp, flag, cmd, addr, cnt)
struct scsi_pkt *pkt;
struct scsi_device *devp;
int flag, cmd, addr, cnt;
{
	MAKECOM_G1(pkt, devp, flag, cmd, addr, cnt);
}

void
makecom_g5(pkt, devp, flag, cmd, addr, cnt)
struct scsi_pkt *pkt;
struct scsi_device *devp;
int flag, cmd, addr, cnt;
{
	MAKECOM_G5(pkt, devp, flag, cmd, addr, cnt);
}

/*
 * Common iopbmap data area packet allocation routines
 */

struct scsi_pkt *
get_pktiopb(ap, datap, cdblen, statuslen, datalen, readflag, func)
struct scsi_address *ap;
int cdblen, statuslen, datalen;
caddr_t *datap;
int readflag;
int (*func)();
{
	struct scsi_pkt *pkt = (struct scsi_pkt *) 0;
	struct buf local;

	if (func != SLEEP_FUNC && func != NULL_FUNC || !datap)
		return (pkt);
	*datap = (caddr_t) 0;
	bzero ((caddr_t) &local, sizeof (struct buf));
	if ((local.b_un.b_addr = IOPBALLOC(datalen)) == (caddr_t) 0) {
		return (pkt);
	} else if (readflag)
		local.b_flags = B_READ;
	local.b_bcount = datalen;
	pkt = scsi_resalloc(ap, cdblen, statuslen, (caddr_t)&local, func);
	if (!pkt) {
		IOPBFREE(local.b_un.b_addr, datalen);
	} else {
		*datap = local.b_un.b_addr;
	}
	return (pkt);
}

/*
 *  Equivalent deallocation wrapper
 */

void
free_pktiopb(pkt, datap, datalen)
struct scsi_pkt *pkt;
caddr_t datap;
int datalen;
{
	if (datap && datalen) {
		IOPBFREE(datap, datalen);
	}
	scsi_resfree(pkt);
}

/*
 * Routine to convert a transport structure into a address cookie
 */

int
scsi_cookie(tranp)
struct scsi_transport *tranp;
{
	return ((int) tranp);
}

/*
 * Common naming functions
 */

static char scsi_tmpname[32];

char *
scsi_dname(dtyp)
int dtyp;
{
	static char *dnames[] = {
		"Direct Access",
		"Sequential Access",
		"Printer",
		"Processor",
		"Write-Once/Read-Many",
		"Read-Only Direct Access",
		"Scanner",
		"Optical",
		"Changer",
		"Communications"
	};

	if ((dtyp & DTYPE_MASK) <= DTYPE_COMM) {
		return (dnames[dtyp&DTYPE_MASK]);
	} else if (dtyp == DTYPE_NOTPRESENT) {
		return ("Not Present");
	}
	return (sprintf(scsi_tmpname,
	    "<unknown device type 0x%x>", (u_int) dtyp));

}

char *
scsi_rname(reason)
u_char reason;
{
	static char *rnames[] = {
		"cmplt",
		"incomplete",
		"dma_derr",
		"tran_err",
		"reset",
		"aborted",
		"timeout",
		"data_ovr",
		"ovr",
		"sts_ovr",
		"badmsg",
		"nomsgout",
		"xid_fail",
		"ide_fail",
		"abort_fail",
		"reject_fail",
		"nop_fail",
		"per_fail",
		"bdr_fail",
		"id_fail",
		"unexpected_bus_free"
	};
	if (reason > CMD_UNX_BUS_FREE) {
		return (sprintf(scsi_tmpname, "<unkown reason %x>", reason));
	} else {
		return (rnames[reason]);
	}
}

char *
scsi_mname(msg)
u_char msg;
{
	static char *imsgs[18] = {
		"COMMAND COMPLETE",
		"EXTENDED",
		"SAVE DATA POINTER",
		"RESTORE POINTERS",
		"DISCONNECT",
		"INITIATOR DETECTED ERROR",
		"ABORT",
		"REJECT",
		"NO-OP",
		"MESSAGE PARITY",
		"LINKED COMMAND COMPLETE",
		"LINKED COMMAND COMPLETE (W/FLAG)",
		"BUS DEVICE RESET",
		"ABORT TAG",
		"CLEAR QUEUE",
		"INITIATE RECOVERY",
		"RELEASE RECOVERY",
		"TERMINATE PROCESS"
	};
	static char *imsgs_2[4] = {
		"SIMPLE QUEUE TAG",
		"HEAD OF QUEUE TAG",
		"ORDERED QUEUE TAG",
		"IGNORE WIDE RESIDUE"
	};

	if (msg < 18) {
		return (imsgs[msg]);
	} else if (IS_IDENTIFY_MSG(msg)) {
		return ("IDENTIFY");
	} else if (IS_2BYTE_MSG(msg) && ((msg) & 0xF0) < 4) {
		return (imsgs_2[msg & 0xF0]);
	} else {
		return (sprintf(scsi_tmpname, "<unknown msg 0x%x>", msg));
	}

}

char *
scsi_cmd_decode(cmd, cmdvec)
u_char cmd;
register char **cmdvec;
{
	while (*cmdvec != (char *) 0) {
		if (cmd == (u_char) **cmdvec) {
			return ((char *)((int)(*cmdvec)+1));
		}
		cmdvec++;
	}
	return (sprintf(scsi_tmpname, "<undecoded cmd 0x%x>", cmd));
}
