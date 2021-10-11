#ifndef lint
static char sccsid[] = "@(#)db.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * dialbox stream module: Converts raw dialbox stream into vuid events.
 * buttonbox stream module: Converts raw buttonbox stream into vuid events.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/time.h>
#include <sundev/vuid_event.h>
#include <sundev/dbio.h>

/* This is so we can build on machines without the new vuid_event.h file */

#ifndef DIAL_DEVID
#define	DIAL_DEVID  0x7B
#endif

#ifndef BUTTON_DEVID
#define	BUTTON_DEVID  0x7A
#endif

#ifndef DIAL_DEBUG
#define	db_debug(x, y, z)
#endif DIAL_DEBUG

/* Local Defines */

/*  States of the input routine */

#ifndef GTCO
#define	SYNC	0
#define	MSB	6
#define	LSB	7

#define	NDIALS	8

#define	DIAL_SCALE(n) ((n) * 90)

#else

#define	SYNC    0
#define	DIAL    1
#define	SIGN    2
#define	HUNDREDS 3
#define	TENS    4
#define	ONES    5
#define	NDIALS  9

#define	DIAL_SCALE(n) ((n) * 192)
/*
 * convert  device units to 1/64th degrees
 */
#endif

static struct module_info rminfo = {
	0, "db", 0, INFPSZ, 0, 0};


static struct module_info wminfo = {
	0, "db", 0, INFPSZ, 0, 0};


static int
dbopen(), dbrput(), dbwput(), dbclose(), send_event(),
button_init();

static struct qinit rinit = {
	dbrput, NULL, dbopen, dbclose, NULL, &rminfo, NULL
};


static struct qinit winit = {
	dbwput, NULL, NULL, NULL, NULL, &wminfo, NULL
};


struct streamtab dbinfo = {
	&rinit, &winit, NULL, NULL};


/* Local state information of devices */

static struct db_state {
	int	read_mode;	/* read mode */
	int	delta[NDIALS];	/* Accumulated deltas since last read */
	/* The following variable are used for raw input processing */
	/* together they determine the state of the input stream */
	int	in_state;	/* Current state of input processing */
	int	in_dial;	/* current dial being read */
	int	in_sign;	/* direction of delta */
	int	in_delta;	/* and its delta so far */
}	db_table[1];
/*
 * shound be NDB to allow for multiple instances
 */

static struct db_state *dbp = db_table;

/* ARGSUSED q */
static int
dbopen(q, dev, flag, sflag)
queue_t    *q;		/* pointer to read queue */
dev_t	dev;		/* Major/minor device number (0 for module) */
int	flag;		/* file open flag (0 for module) */
int	sflag;		/* stream open flag */
{
	db_debug(1, "dbo\n", 1);
	dbp->read_mode = VUID_FIRM_EVENT;
	dbp->in_state = SYNC;
	dbinit(WR(q));
	button_init(WR(q));
	return (0);			/* OK */
}


static int
dbwput(q, mp)			/* Write put procedure */
queue_t	*q;		/* pointer to write queue */
mblk_t	*mp;		/* message pointer */
{

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		db_debug(1, "dbio %X\n", *(mp->b_wptr));
		dbioctl(q, mp);
		break;
	default:
		db_debug(1, "dbw %X\n", *(mp->b_wptr));
		putnext(q, mp);		/* Just pass the message through */
		break;
	}
}


static int
dbrput(q, mp)			/* read put procedure */
queue_t	*q;		/* pointer to read queue */
mblk_t	*mp;		/* message pointer */
{
	switch (mp->b_datap->db_type) {
	case M_DATA:
		switch (dbp->read_mode) {
		case VUID_NATIVE:	/* unfiltered */
			db_debug(1, "dbn %X\n", *(mp->b_rptr));
			putnext(q, mp);	/* Just pass the message through */
			break;
		case VUID_FIRM_EVENT:
		    {	/* read everything and output events */
				register mblk_t *bp;
				mblk_t	* nmp = NULL,
				    *nbp = NULL;
				register int	i,
				n_events;
				static Firm_event db_event;

				/*
				 * Process all of our messages and accumulate
				 * deltas
				 */
				for (bp = mp; bp != NULL; bp = bp->b_cont) {
					while (bp->b_rptr < bp->b_wptr) {
						db_debug(2,
							"dbf %X\n",
							 *(bp->b_rptr));
						dbinput(*bp->b_rptr, q);
					/*
					 * 'process' this  character
					 */
						bp->b_rptr++;
					}
				}

				/*
				 * free up old message, we are done with it
				 */
				freemsg(mp);

				/*
				 * now we have read all of our data.
				 * Count events
				 */
				n_events = 0;
				for (i = 0; i < NDIALS; i++) {
					/* count how many dials have changed */
					if (dbp->delta[i]) {
						n_events++;
					}
				}
				/*
				 * if we have any events, get a message
				 * block to put them in. if allocation
				 * fails, it doesn't matter since we
				 * still have the deltas (however, time
				 * info may be lost
				 */
				if ((n_events != 0) &&
				    ((nmp = (mblk_t *) allocb(n_events
				    * sizeof (Firm_event), BPRI_MED))
				    != NULL)) {
					nbp = nmp;
					for (i = 0; i < NDIALS; i++) {
						/*
						 * put the events into output
						 * buffer
						 */
						if (dbp->delta[i]) {
							db_event.id =
							    vuid_id_addr(vuid_first(DIAL_DEVID)) |
							    vuid_id_offset(i);
							db_event.pair_type = FE_PAIR_NONE;
							db_event.pair = 0;
							db_event.value = DIAL_SCALE(dbp->delta[i]);
							dbp->delta[i] = 0;	/* clear out delta */
							uniqtime(&db_event.time);	/* get time */
							/* Copy Event struct to new data block */
							bcopy((char *) & db_event, (char *) nbp->b_wptr,
							    sizeof (Firm_event));
							nbp->b_wptr += sizeof (Firm_event);
						}
					}
					/* Now we have a single message block */
					putnext(q, nmp);	/* pass it on */
				}
			}
		default:
			db_debug(2, "default!! %X\n", *(mp->b_rptr));
			break;
		}
		break;
	default:
		db_debug(2, "pass thru %X\n", *(mp->b_rptr));
		putnext(q, mp);		/* pass them through */
		break;
	}
}


/* ARGSUSED */
static int
dbclose(q, flag)
queue_t    *q;		/* pointer to read queue */
int	flag;		/* file open flags, 0 for read queue */
{
	db_debug(1, "dbc\n", 1);
}


/*
 * dbinput() processes a single character from the dialbox. It accumulates
 * deltas in the local structure referneced by the dbp pointer. Instead of
 * putting the events into an intermediate queue, it simply keeps track of
 * the deltas since the last read. This may need to change if timing
 * information becomes important. It does keep the buffer requirements small
 * though.
 */

#ifdef DIAL_DEBUG
int	db_err = 0;		/* for debugging */

#endif DIAL_DEBUG


#ifndef GTCO
dbinput(c, q)
register unsigned char	c;	/* The input character */
queue_t    *q;
/*
 * and the pointer to read queue from which it came
 */
{
	Firm_event  db_e;
	static int	map[] = {
		25, 26, 27, 28, 29, 30, 31, 32,
		17, 18, 19, 20, 21, 22, 23, 24,
		9, 10, 11, 12, 13, 14, 15, 16,
		1, 2, 3, 4, 5, 6, 7, 8	};

	switch (dbp->in_state) {
	case SYNC:			/* awaiting lead byte */
		/* Input is from a button box */
		if ((c >= 0x80) && (c <= 0x87)) {
			dbp->in_state = MSB;
			dbp->in_dial = c & 7;
		} /* Input is from a button box */
		else if ((c >= 0xc0) && (c <= 0xdf)) {
			db_e.id = vuid_id_addr(vuid_first(BUTTON_DEVID)) |
				map[c - 0xc0];
			db_e.pair_type = FE_PAIR_NONE;
			db_e.pair = 0;
			db_e.value = 1;	/* Down */
			uniqtime(&db_e.time);	/* get time */
			/* send this event off to the queue */
			send_event(&db_e, q);
		} else if ((c >= 0xe0) && (c <= 0xff)) {
			db_e.id = vuid_id_addr(vuid_first(BUTTON_DEVID)) |
				map[c - 0xe0];
			db_e.pair_type = FE_PAIR_NONE;
			db_e.pair = 0;
			db_e.value = 0;	/* Up  */
			uniqtime(&db_e.time);	/* get time */
			/* send this event off to the queue */
			send_event(&db_e, q);
		} else {
			/* Re-initialize, maybe box has been plugged back in */
			if (c != 0x20) {	/* Response from last init ? */
				dbinit(WR(q));
			}
		}
		break;
	case MSB:			/* Most significant, includes sign */
		/*
	 * There is a possibility that when twisting the dials really really
	 * fast that data bytes will be dropped. The data very seldom falls in
	 * the range 0x80 through 0x87 so we ignore the chance that we are
	 * throwing away valid data.
	 *
	 * The consequences of not doing this is that button events are
	 * generated, if there is a drop out that is not taken care of this
	 * way...
	 *
	 */
		if ((c >= 0x80) && (c <= 0x87)) {
			dbp->in_state = MSB;
			dbp->in_dial = c & 7;
		} else {
			dbp->in_delta = (unsigned int) c;
			dbp->in_state = LSB;
		}
		break;
	case LSB:
		if ((c >= 0x80) && (c <= 0x87)) {
			dbp->in_state = MSB;
			dbp->in_dial = c & 7;
		} else {
			dbp->delta[dbp->in_dial] +=
			    (short) ((dbp->in_delta << 8) | (c));
			dbp->in_state = SYNC;
		}
		break;
	default:
		dbp->in_state = SYNC;
		break;
	}

}


dbinit(q)
queue_t    *q;		/* Write queue */
{
	mblk_t	* mb;		/* Message Block */
	static char	init_cmd[] = {
		0x90, 0, 0x0A
	};
	/*
	 * 0x20 is reset command for button box. Limits transmission to 30
	 * per second for dials
	 */


	/* Initialise the dialbox and button box  */
	if ((mb = (mblk_t *) allocb(sizeof (init_cmd), BPRI_MED)) != NULL) {
		bcopy(init_cmd, (char *) mb->b_wptr, sizeof (init_cmd));
		mb->b_wptr += sizeof (init_cmd);
		putnext(q, mb);		/* put it on the write queue */

	}
}


static int
button_init(q)
queue_t    *q;		/* Write queue */
{
	mblk_t	* mb;		/* Message Block */
	static char	init_cmd[] = {
		0x20
	};
	/*
	 * 0x20 is reset command for button box. Initialise the button box
	 */
	if ((mb = (mblk_t *) allocb(sizeof (init_cmd), BPRI_MED)) != NULL) {
		bcopy(init_cmd, (char *) mb->b_wptr, sizeof (init_cmd));
		mb->b_wptr += sizeof (init_cmd);
		putnext(q, mb);		/* put it on the write queue */

	}
}


#else GTCO
dbinput(c)
register char	c;
{
	if (c == 0x1B) {
		dbp->in_state = DIAL;	/* re sync */
	} else {
		switch (dbp->in_state) {
		case SYNC:		/* awaiting an escape */
			if (c == 0x1B) {
				dbp->in_state = DIAL;
				dbp->in_delta = 0;
			}
			break;
		case DIAL:		/* dial number */
			if (c >= '0' && c <= '8') {
				dbp->in_dial = c - '0';
					/*
					 * atoi for non strings
					 */
				dbp->in_state = SIGN;
			} else {
				dbp->in_state = SYNC;
			}
			break;
		case SIGN:		/* sign byte */
			dbp->in_state = HUNDREDS;
					/*
					 * next state, unless error
					 */
			switch (c) {
			case '+':
				dbp->in_sign = 1;
				break;
			case '-':
				dbp->in_sign = -1;
				break;
			default:
				dbp->in_state = SYNC;
				break;
			}
			break;
		case HUNDREDS:		/* 100's */
			if ((c >= '0') && (c <= '9')) {
				dbp->in_delta = 100 * (c - '0');
				dbp->in_state = TENS;
			} else {
				dbp->in_state = SYNC;
			}
			break;
		case TENS:		/* 10's */
			if ((c >= '0') && (c <= '9')) {
				dbp->in_delta += 10 * (c - '0');
				dbp->in_state = ONES;
			} else {
				dbp->in_state = SYNC;
			}
			break;
		case ONES:		/* 1's */
			if ((c >= '0') && (c <= '9')) {
				dbp->in_delta += (c - '0');
				dbp->delta[dbp->in_dial] +=
					dbp->in_sign *
					dbp->in_delta;
			}
			dbp->in_state = SYNC;
			break;
		default:
			dbp->in_state = SYNC;
			break;
		}
	}
	return;
}


dbinit(q)
queue_t    *q;		/* Write queue */
{
	return (0);
}


#endif



dbioctl(q, mp)
			/*
			 * ioctl routine, called from write put routine
			 */
queue_t    *q;		/* pointer to write queue */
mblk_t	*mp;		/* message pointer */
{
	struct iocblk *iocp;
	static unsigned char	led_data[5];
	mblk_t	* mb;		/* Message Block */
	static int	in_data;


	int	err = 0;

	iocp = (struct iocblk *) mp->b_rptr;	/* Ioctl is in first block */

	switch (iocp->ioc_cmd) {
	case VUIDGFORMAT:
	    {		/* Get Read mode */
			register mblk_t *datap;

			if ((datap = (mblk_t *) allocb(sizeof (int), BPRI_HI))
			    == NULL) {
				/* We could do something smarter here */
				err = -1;
			} else {
				*(int *) datap->b_wptr = dbp->read_mode;
				datap->b_wptr += sizeof (int);
				mp->b_cont = datap;
				iocp->ioc_count = sizeof (int);
				db_debug(2, "dbg %X\n", *(datap->b_wptr));
			}
		}
		break;

	case VUIDSFORMAT:
	    {		/* Set Read Mode */
			if ((iocp->ioc_count != sizeof (int)) ||
			    (mp->b_cont == NULL) ||
			    (mp->b_cont->b_rptr == NULL)) {
				err = -2;
			} else {
				dbp->read_mode = *(int *) mp->b_cont->b_rptr;
				db_debug(2,
				    "dbs %X\n", *(int *) mp->b_cont->b_rptr);
			}
		}
		break;
	case DBIOBUTLITE:
	{		/* Turn the specified leds of/off */
			if (iocp->ioc_count != sizeof (int)) {
				printf("count wrong!\n");
				err = -3;
			} else {

				in_data = (*(int *) mp->b_cont->b_rptr);

				/* Create the buffer */
				led_data[0] = 0x75;
				led_data[4] = ((in_data >> 24) & 0xff);
				led_data[3] = ((in_data >> 16) & 0xff);
				led_data[2] = ((in_data >> 8) & 0xff);
				led_data[1] = ((in_data) & 0xff);

				iocp->ioc_count = 0;
				/* Copy this to a message block */
				if ((mb = (mblk_t *)
				    allocb(sizeof (led_data), BPRI_MED))
					!= NULL) {
					bcopy((char *)led_data,
						(char *) mb->b_wptr,
						sizeof (led_data));
					mb->b_wptr += sizeof (led_data);
					putnext(q, mb);
						/*
						 * put it on the write queue
						 */
				}
			}
			break;
		}
	}
	if (err) {
		mp->b_datap->db_type = M_IOCNAK;
	} else {
		mp->b_datap->db_type = M_IOCACK;
	}
	qreply(q, mp);
}


static int
send_event(db_ev, q)
struct firm_event *db_ev;
queue_t    *q;		/* and the pointer to destination queue  */
{
	mblk_t	* nmp = NULL;

	if ((nmp = (mblk_t *) allocb(sizeof (Firm_event), BPRI_MED)) != NULL) {

		/* Copy Event struct to new data block */
		bcopy((char *) db_ev, (char *) nmp->b_wptr,
		    sizeof (struct firm_event));
		nmp->b_wptr += sizeof (struct firm_event);
		/* Now we have a single message block */
		putnext(q, nmp);	/* pass it on */
	} else {
		printf("db: allocation failed.\n");
	}
}


#ifdef DIAL_DEBUG
db_debug(level, fmt, what)
int	level;
char	*fmt;
int	what;

{
	if (db_err >= level)
		printf(fmt, what);
}


#endif DIAL_DEBUG
