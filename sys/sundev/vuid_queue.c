#ifndef lint
static	char sccsid[] = "@(#)vuid_queue.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Vuid (Virtual User Input Device) queue management.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sundev/vuid_event.h>
#include <sundev/vuid_queue.h>

Vuid_q_node *vq_alloc_node();
void	vq_free_node();
int	tv_equal();
struct	timeval tv_subt();
struct	timeval tv_divide();
struct	timeval tv_mult();
#define	tv_to_usec(tv)	(((tv).tv_sec * 1000000) + (tv).tv_usec)
		/* Works for small numbers (< 1000) of seconds */
struct	timeval usec_to_tv();

void
vq_initialize(vq, data, bytes)
	register Vuid_queue *vq;
	caddr_t data;
	u_int bytes;
{
	register Vuid_q_node *new_vqns, *vqn;

	/* Initialize vq */
	vq->top = vq->bottom = vq->free = VUID_Q_NODE_NULL;
	vq->size = 1 + (bytes - sizeof (Vuid_q_node)) / sizeof (Vuid_q_node);
	/* Place in pool by freeing all nodes (fudge vq->num for this) */
	new_vqns = (Vuid_q_node *)data;
	vq->num = vq->size;
	for (vqn = new_vqns; vqn < new_vqns + vq->size; vqn++)
		vq_free_node(vq, vqn);
	return;
}

Vuid_q_code
vq_put(vq, firm_event)
	Vuid_queue *vq;
	Firm_event *firm_event;
{
	register Vuid_q_node *vp;

	/* Merge into existing events based on time stamp */
	for (vp = vq->bottom; vp; vp = vp->prev) {
		/* Put later times closer to the bottom than earlier ones */
		if (timercmp(&vp->firm_event.time, &firm_event->time, <) ||
		    tv_equal(vp->firm_event.time, firm_event->time)) {
			register Vuid_q_node *vqn = vq_alloc_node(vq);

			if (vqn == VUID_Q_NODE_NULL)
				return (VUID_Q_OVERFLOW);
			vqn->firm_event = *firm_event;
			/* Insert vqn before vq (neither are null) */
			/* Initialize vqn's next and prev */
			vqn->next = vp->next;
			vqn->prev = vp;
			/* Fix up vp next's prev */
			if (vp->next != VUID_Q_NODE_NULL)
				vp->next->prev = vqn;
			/* Fix up vp's next */
			vp->next = vqn;
			/* Change bottom */
			if (vp == vq->bottom)
				vq->bottom = vqn;
			/* Change top */
			if (vq->top == VUID_Q_NODE_NULL)
				vq->top = vqn;
			return (VUID_Q_OK);
		}
	}
	/* Place at top of queue */
	return (vq_putback(vq, firm_event));
}

Vuid_q_code
vq_get(vq, firm_event)
	register Vuid_queue *vq;
	Firm_event *firm_event;
{
	register Vuid_q_node *vqn = vq->top;

	if (vqn == VUID_Q_NODE_NULL)
		return (VUID_Q_EMPTY);
	/* Conditionally copy data */
	if (firm_event != FIRM_EVENT_NULL)
		*firm_event = vqn->firm_event;
	/* Change top */
	vq->top = vqn->next;
	/* Null new top's prev */
	if (vq->top != VUID_Q_NODE_NULL)
		vq->top->prev = VUID_Q_NODE_NULL;
	/* Change bottom */
	if (vq->bottom == vqn)
		vq->bottom = VUID_Q_NODE_NULL;
	/* Release storage */
	vq_free_node(vq, vqn);
	return (VUID_Q_OK);
}

Vuid_q_code
vq_peek(vq, firm_event)
	Vuid_queue *vq;
	Firm_event *firm_event;
{
	if (vq->top == VUID_Q_NODE_NULL)
		return (VUID_Q_EMPTY);
	*firm_event = vq->top->firm_event;
	return (VUID_Q_OK);
}

Vuid_q_code
vq_putback(vq, firm_event)
	register Vuid_queue *vq;
	Firm_event *firm_event;
{
	register Vuid_q_node *vqn = vq_alloc_node(vq);

	if (vqn == VUID_Q_NODE_NULL)
		return (VUID_Q_OVERFLOW);
	vqn->firm_event = *firm_event;
	/* Change new top's next */
	vqn->next = vq->top;
	/* Null new top's prev */
	vqn->prev = VUID_Q_NODE_NULL;
	/* Change old top's prev */
	if (vq->top != VUID_Q_NODE_NULL)
		vq->top->prev = vqn;
	/* Change top */
	vq->top = vqn;
	/* Change bottom */
	if (vq->bottom == VUID_Q_NODE_NULL)
		vq->bottom = vqn;
	return (VUID_Q_OK);
}

int
vq_compress(vq, factor)
	register Vuid_queue *vq;
	int factor;
{
	struct	timeval tv_interval, tv_avg_diff, tv_diff; /* Intermediates */
	struct	timeval tv_threshold;
	register Vuid_q_node *base, *victim;
	Vuid_q_node *victim_next;
	int num_start;

	if (vq->top == VUID_Q_NODE_NULL)
		return (0);
	num_start = vq->num;
	/*
	 * Determine the threshold time interval under which events of
	 * the same type (valuator only) are collapsed.
	 *   min_time_betvqnen_values = ((first_entry_time - last_entry_time) /
         *     max_number_of_queue_entries) * factor_by_which_to_compress_queue;
	 */
	tv_interval = tv_subt(vq->bottom->firm_event.time,
	    vq->top->firm_event.time);
	tv_avg_diff = tv_divide(tv_interval, vq->num);
	tv_threshold = tv_mult(tv_avg_diff, factor);
	/* March down list */
	for (base = vq->top; base; base = base->next) {
		/* See if valuator event */
		if (!vq_is_valuator(base))
			continue;
		/* Run down list looking for a collapse victim */
		for (victim = base->next; victim; victim = victim_next) {
			/* Remember next victim incase axe victim below */
			victim_next = victim->next;
			/* Fail if not valuator event */
			if (!vq_is_valuator(victim))
				goto Advance_Base;
			/*
			 * May peek ahead and do the collapse as long as the
			 * intervening times of other valuator event types
			 * are the same.  Fail if intervening event's time
			 * differs from victim's.
			 */
			if (victim->prev != base) {
				if (!tv_equal(victim->prev->firm_event.time,
				    victim->firm_event.time))
					goto Advance_Base;
			}
			/* Fail if time difference is above threshold */
			tv_diff = tv_subt(victim->firm_event.time,
			    base->firm_event.time);
			/* Zero factor means collapse regardless of threshold */
			if ((factor > 0) &&
			    (timercmp(&tv_diff, &tv_threshold, >)))
				goto Advance_Base;
			/* Do collapse if same event id */
			if (victim->firm_event.id == base->firm_event.id){
				/* Collapse value into base event */
				switch (base->firm_event.pair_type) {
				case FE_PAIR_ABSOLUTE:
					/* id is delta */
					base->firm_event.value +=
					    victim->firm_event.value;
					break;
				case FE_PAIR_DELTA:
					/* id is absolute */
					/* Fall through */
				default:
					/* Assume id is absolute */
					base->firm_event.value =
					    victim->firm_event.value;
					break;
				}
				/* Remove victim node */
				vq_delete_node(vq, victim);
			}
		}
Advance_Base:
	{}
	}
	return (num_start - vq->num);
}

int
vq_is_valuator(vqn)
	register Vuid_q_node *vqn;
{
	return ((vqn->firm_event.value < 1 && vqn->firm_event.value > -1) ||
	    (vqn->firm_event.pair_type == FE_PAIR_DELTA) ||
	    (vqn->firm_event.pair_type == FE_PAIR_ABSOLUTE));
}

void
vq_delete_node(vq, vqn)
	register Vuid_queue *vq;
	register Vuid_q_node *vqn;
{
	/* Use get if removing off top of queue */
	if (vqn == vq->top) {
		(void) vq_get(vq, FIRM_EVENT_NULL);
		return;
	}
	/* Update previous next link (not null else vqn would be top) */
	vqn->prev->next = vqn->next;
	/* Change bottom */
	if (vq->bottom == vqn)
		vq->bottom = vqn->prev;
	else
		/* Update next previous link (if null else vqn is bottom) */
		vqn->next->prev = vqn->prev;
	/* Release storage */
	vq_free_node(vq, vqn);
	return;
}

/*
 * Caller must initialize data returned from vq_alloc_node.
 * VUID_Q_NODE_NULL is possible.
 */
Vuid_q_node *
vq_alloc_node(vq)
	register Vuid_queue *vq;
{
	register Vuid_q_node *vqn;

	if (vq->free == VUID_Q_NODE_NULL)
		return(VUID_Q_NODE_NULL);
	vqn = vq->free;
	vq->free = vq->free->next;
	vq->num++;
	vqn->next = VUID_Q_NODE_NULL;
	return(vqn);
}

void
vq_free_node(vq, vqn)
	register Vuid_queue *vq;
	Vuid_q_node *vqn;
{
	vqn->next = vq->free;
	vqn->prev = VUID_Q_NODE_NULL;
	vq->free = vqn;
	vq->num--;
}

int
tv_equal(a, b)
	struct timeval a, b;
{
	return(a.tv_sec == b.tv_sec && a.tv_usec == b.tv_usec);
}

/* atv-btv */
struct	timeval
tv_subt(atv, btv)
	struct	timeval atv, btv;
{
	if ((atv.tv_usec < btv.tv_usec) && atv.tv_sec) {
		atv.tv_usec += 1000000;
		atv.tv_sec--;
	}
	if (atv.tv_usec > btv.tv_usec)
		atv.tv_usec -= btv.tv_usec;
	else
		atv.tv_usec = 0;
	if (atv.tv_sec > btv.tv_sec)
		atv.tv_sec -= btv.tv_sec;
	else {
		if  (atv.tv_sec < btv.tv_sec)
			atv.tv_usec = 0;
		atv.tv_sec = 0;
	}
	return(atv);
}

/* tv / dividend */
struct	timeval
tv_divide(tv, dividend)
	struct	timeval tv;
	int	dividend;
{
	int usecs;

	if (dividend == 0)
		return (tv);
	usecs = tv_to_usec(tv);
	usecs /= dividend;
	tv = usec_to_tv(usecs);
	return (tv);
}

/* tv * multiplier (works for small multipliers * seconds, < 1000) */
struct	timeval
tv_mult(tv, multiplier)
	struct	timeval tv;
	int	multiplier;
{
	int usecs;

	usecs = tv_to_usec(tv);
	usecs *= multiplier;
	tv = usec_to_tv(usecs);
	return (tv);
}

struct	timeval
usec_to_tv(usec)
	int	usec;
{
	struct	timeval tv;

	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec % 1000000;
	return (tv);
}

