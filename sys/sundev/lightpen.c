#ifndef lint
static char sccsid[] = "@(#)lightpen.c  1.1 92/07/30 SMI";
#endif

/*
 * Light Pen stream driver: Gets the hardware lightpen interrupts and
 * formats a VUID event and sends it through the streams.
 * Currently this driver supports only one lightpen device at 
 * a time. But it can be easily ported to handle multiple 
 * lightpen devices.
 */

#include "gt.h"			/* Number of GT's */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/stropts.h>
#include <sundev/vuid_event.h>
#include <sundev/lightpenreg.h>
#include <sundev/msreg.h>

/*
 * This is so we can build on machines without the new vuid_event.h file 
 */

#ifndef LIGHTPEN_DEVID
#define LIGHTPEN_DEVID 0x79
#endif

#define LIGHTPEN_MAX -9999
#define DEF_X_CALIBRATION -80
#define DEF_Y_CALIBRATION 0
/* Defined in sbusdev/gt.c */
extern int lightpen_opened;

static struct module_info rminfo = { 0, "lightpen", 0, INFPSZ, 0, 0 };
static struct module_info wminfo = { 0, "lightpen", 0, INFPSZ, 0, 0 };

static int 	lightpenopen(),
	 	lightpenwput(),
	 	lightpenclose(),
		lightpenpoll(),
		lightpenrserv();

static struct qinit rinit = {
	NULL,
	lightpenrserv,
	lightpenopen,
	lightpenclose,
	NULL,
	&rminfo,
	NULL
};

static struct qinit winit = {
	lightpenwput,
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	&wminfo, 
	NULL
};

struct streamtab lightpenptab = { &rinit, &winit, NULL, NULL };

#define ABS(x) ((x) < 0? -(x) : (x))

/* Local state information of devices */

static struct lightpen_state {
    unsigned 	flags;		/* Flags field */
#define LP_OPEN		0x1
#define LP_CALIBRATE	0x2
#define LP_FILTER	0x4
#define LP_RESOLUTION	0x8
    int 	read_mode;	/* read mode */
    lightpen_calibrate_t calibrate; /* calibration values */
    lightpen_filter_t filter; /* calibration values */
    int		x_values[LIGHTPEN_FILTER_MAXCOUNT];
    int		y_values[LIGHTPEN_FILTER_MAXCOUNT];
    int 	ix;
    int		iy;
    int		prev_x;
    int		prev_y;
    int		resolution;
    queue_t	*rq;
} lightpen_table[NGT]; 

static struct lightpen_state *lightpenp = lightpen_table;

int	lightpen_first_start=1;

/* Refer to sbusdev/gt.c for lightpen_events */
extern struct lightpen_events *lightpen_readbuf;

/* ARGSUSED  */
static int
lightpenopen(q, dev, flag, sflag)
	queue_t *q;		/* pointer to read queue */
	dev_t   dev;		/* Major/minor device number(0 for module) */
	int     flag;		/* file open flag (0 for module) */
	int     sflag;		/* stream open flag */
{
	int	i;

	if (lightpen_first_start) { 
		if (setsh_options(q))
			return (OPENFAIL);

		lightpenp->flags = 0; 
		lightpenp->flags |= LP_OPEN; 
		lightpen_opened = 1;
		lightpenp->read_mode = VUID_FIRM_EVENT; 
		for (i = 0; i < LIGHTPEN_FILTER_MAXCOUNT; i++){
			lightpenp->x_values[i] = LIGHTPEN_MAX;
			lightpenp->y_values[i] = LIGHTPEN_MAX;
		}

		lightpenp->calibrate.x = DEF_X_CALIBRATION;
		lightpenp->calibrate.y = DEF_Y_CALIBRATION;
		lightpenp->filter.flag = 0;
		lightpenp->filter.tolerance = 0;
		lightpenp->filter.count = 1;
		lightpenp->resolution = 0;
		lightpenp->prev_x = LIGHTPEN_MAX;
		lightpenp->prev_y = LIGHTPEN_MAX;
		/*
		 * Initialize the read queue for later use
		 */
		lightpenp->rq = q;
		lightpen_first_start = 0; 
    		timeout(lightpenpoll, (caddr_t)0, LPTICKS);
	}

	(void) putctl1(q->q_next,M_FLUSH, FLUSHR);

	return (0);
}

/*
 * Write put procedure
 */
static int
lightpenwput(q, mp)
	queue_t *q;		/* pointer to write queue */ 
	mblk_t  *mp;		/* message pointer */ 
{
	switch (mp->b_datap->db_type) {

		case M_IOCTL:
			lightpenioctl(q, mp);
			break;	

		default:
			printf("lightpenwput: incorrect write option\n");
			break;
   	 }
}


/*
 * Read service routine
 */
static int
lightpenrserv(q)
	queue_t *q;		/* pointer to read queue */
{
	int ret_val;

	switch (lightpenp->read_mode) {

	case VUID_FIRM_EVENT: { /* read everything and output events */

    		mblk_t			*bp   = NULL;
    		static Firm_event	*fep  = NULL;
		struct lightpen_events_data	*datap = NULL;

		lightpen_readbuf->flags |= LP_READING;

		/*
		 * If nobody is reading we don't want to hang the system
		 */
                if (!canput(q->q_next)) {
			(void) putctl1(q->q_next,M_FLUSH, FLUSHR);
                }

		bp = (mblk_t *) allocb((int) (lightpen_readbuf->event_count * 
		    sizeof (Firm_event)), BPRI_HI);

		if (bp != NULL) {
			while (lightpen_readbuf->event_list != NULL) {
			
				ret_val = 0;
				datap = lightpen_readbuf->event_list;
				fep = (Firm_event *) bp->b_wptr;
				fep->id = 
				    vuid_id_addr(vuid_first(LIGHTPEN_DEVID))
					 | vuid_id_offset(datap->event_type);
				fep->pair_type = FE_PAIR_NONE;
				fep->pair      = 0;
				if ((datap->event_type == LIGHTPEN_MOVE)
					|| (datap->event_type ==
						LIGHTPEN_DRAG))
					ret_val = lightpen_compute_value(
						datap->event_value,
						 fep);
				else 
					fep->value = datap->event_value;

				uniqtime(&fep->time);

				if (ret_val == 0)
				   bp->b_wptr += sizeof (Firm_event);
				lightpen_readbuf->event_list =
					 datap->next_event;
				kmem_free((caddr_t)datap,
				    sizeof(struct lightpen_events_data));
			}
			putnext(q,bp);

		} else {
			printf("lightpenrserv: could not allocate buffer memory\n");
			break;
		}

		/* 
		 * Finished with all the messages on this buffer
		 */
		lightpen_readbuf->flags &= ~(LP_READING);
		lightpen_readbuf->event_count = 0;
	}
	break;

	default:
		printf("lightpenrserv: Cannot support event type\n");

	}
}

		
/* ARGSUSED */
static int
lightpenclose(q, flag)
	queue_t	*q;		/* pointer to read queue */
	int	 flag;		/* file open flags, 0 for read queue */
{
	lightpen_first_start = 1;
	lightpen_opened      = 0;

	lightpenp->flags &= ~(LP_OPEN);
	(void) putctl1(q->q_next,M_FLUSH, FLUSHR);
	untimeout(lightpenpoll, (caddr_t)0);
}


/*
 * ioctl, called from write put routine
 */
lightpenioctl(q, mp)
	queue_t *q;		/* pointer to write queue */
	mblk_t  *mp;		/* message pointer */
{
	struct iocblk	*iocp;
	int i;
	int		 err = 0;
	lightpen_calibrate_t *lc;
	lightpen_filter_t *lf;
	iocp = (struct iocblk *)mp->b_rptr;	/* ioctl is in first block */

	switch (iocp->ioc_cmd) {

	case VUIDGFORMAT: { /* Get Read mode */
		register mblk_t *datap;

		datap = (mblk_t *)allocb(sizeof(int), BPRI_HI);

		if (datap == NULL) {
			u.u_error = EINVAL;
			err = 1;
		} else {
			*(int *)datap->b_wptr = lightpenp->read_mode;
			datap->b_wptr += sizeof (int);
			mp->b_cont = datap;
			iocp->ioc_count = sizeof (int);
		}
	}
	break;

	case VUIDSFORMAT: {			/* Set Read Mode */
		if ((iocp->ioc_count != sizeof(int)) || 
		    (mp->b_cont == NULL) ||
		    (mp->b_cont->b_rptr == NULL)) {
			u.u_error = EINVAL;
			err = 1;
		} else {
			lightpenp->read_mode =
				 *(int *)mp->b_cont->b_rptr;
		}
	}
	break;

	case LIGHTPEN_CALIBRATE: { /* Setup the calibration value */
		if ((iocp->ioc_count != sizeof(lightpen_calibrate_t)) || 
		    (mp->b_cont == NULL) ||
		    (mp->b_cont->b_rptr == NULL)) {
			u.u_error = EINVAL;
			err = 1;
		} else {
			lightpenp->flags |= LP_CALIBRATE;
			lc = (lightpen_calibrate_t *)
				mp->b_cont->b_rptr;

			lightpenp->calibrate.x = lc->x; 
			lightpenp->calibrate.y = lc->y;
		}
	}
	break;

	case LIGHTPEN_FILTER: {	/* Setup the filter values */

		if ((iocp->ioc_count != sizeof(lightpen_filter_t)) || 
		    (mp->b_cont == NULL) ||
		    (mp->b_cont->b_rptr == NULL)) {
			u.u_error = EINVAL;
			err = 1;
		} else {
			lf = (lightpen_filter_t *)
					mp->b_cont->b_rptr;
			if (lf->count > (LIGHTPEN_FILTER_MAXCOUNT  - 1) ||
				lf->count == 0){
				u.u_error = EINVAL;
				err = 1;
			} else {
				for (i = 0; i < LIGHTPEN_FILTER_MAXCOUNT;
					 i++){
					lightpenp->x_values[i] = -9999;
					lightpenp->y_values[i] = -9999;
				}
				lightpenp->ix = 0;
				lightpenp->iy = 0;
				lightpenp->flags |= LP_FILTER;
				lightpenp->filter.flag =
						 lf->flag; 
				lightpenp->filter.tolerance =
						 lf->tolerance;
				lightpenp->filter.count =
						 lf->count;
			}
		}
	}
	break;
	} /* end of switch */

	if (err) {
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_rval = -1;
	} else {
		mp->b_datap->db_type = M_IOCACK;
	}

	qreply(q, mp);
}


static int
lightpenpoll()
{

	if (lightpen_readbuf != NULL && 
		lightpen_readbuf->event_list != NULL){
		qenable(lightpenp->rq);
	}
	if (lightpenp->flags & LP_OPEN)
		timeout(lightpenpoll, (caddr_t)0, LPTICKS);
}


static int
setsh_options(q)
	queue_t *q;		/* pointer to read queue */
{
	mblk_t		  *bp = NULL;
	struct stroptions *sop = NULL;

	bp = (mblk_t *)allocb(sizeof(struct stroptions), BPRI_HI);

	if (bp != NULL) {
		sop = (struct stroptions *) bp->b_wptr;
		sop->so_flags = SO_VMIN;
		sop->so_vmin = 1;
		bp->b_datap->db_type = M_SETOPTS;
		bp->b_wptr += sizeof (struct stroptions);
		putnext(q, bp);
	} else {
		printf("setsh_options: could not allocate buffer\n");
		return (1);
	}
	return (0);
}


static int
lightpen_compute_value(value, fep)
	u_int value;
	Firm_event *fep;
{
	int raw_x,loc_x,final_x;
	int raw_y,loc_y,final_y;
	int i;
	
	raw_x = GET_LIGHTPEN_X(value);
	raw_y = GET_LIGHTPEN_Y(value);

	if (lightpenp->flags & LP_CALIBRATE){
		raw_x += lightpenp->calibrate.x;
		raw_y += lightpenp->calibrate.y;
	}

	final_x = raw_x;
	final_y = raw_y;
	if (lightpenp->flags & LP_FILTER) {
		/* Do the filtering */

		/* Calculate the Manhattan distance */
	  	if (((ABS( raw_x - lightpenp->prev_x))  +
			(ABS( raw_y - lightpenp->prev_y))) <
			 lightpenp->filter.tolerance) {
		/* compute average for x */
		lightpenp->x_values[lightpenp->ix] = 
				raw_x;
		if (lightpenp->ix ==
			 (lightpenp->filter.count - 1))
			lightpenp->ix = 0;
		else 
			lightpenp->ix++;
		for (i = 0, loc_x = 0; i < lightpenp->filter.count; i++)
			loc_x += lightpenp->x_values[i];
		
		loc_x = loc_x / lightpenp->filter.count;

		/* compute average for y */
		lightpenp->y_values[lightpenp->iy] = 
				raw_y;
		if (lightpenp->iy ==
			 (lightpenp->filter.count - 1))
			lightpenp->iy = 0;
		else 
			lightpenp->iy++;
		for (i = 0, loc_y = 0; i < lightpenp->filter.count; i++)
			loc_y += lightpenp->y_values[i];
		
		loc_y = loc_y / lightpenp->filter.count;

		} else {

			/* Initialize the array values of x to the new
			   value */
			loc_x = raw_x;
			for (i = 0; i < lightpenp->filter.count; i++)
			 lightpenp->x_values[i]= loc_x;

			/* Initialize the array values of y to the new
			   value */
			loc_y = raw_y;
			for (i = 0; i < lightpenp->filter.count; i++)
			 lightpenp->y_values[i]= loc_y;
		}
		lightpenp->prev_x = loc_x;
		final_x = loc_x;
		lightpenp->prev_y = loc_y;
		final_y = loc_y;
	}

	/* NOW store the button state X & Y on the event struct */
	fep->value = (value & LIGHTPEN_MASK_BUTTON) | 
		     (final_y << LIGHTPEN_SHIFT_Y) | 
		     (final_x << LIGHTPEN_SHIFT_X);
	return(0);
}
