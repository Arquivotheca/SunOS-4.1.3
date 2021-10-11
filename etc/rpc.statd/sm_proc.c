#ifndef lint
static char sccsid[] = "@(#)sm_proc.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1989 by Sun Microsystems, Inc.
	 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/sm_inter.h>

extern int debug;
int local_state;		/* fake local sm state */
int remote_state = 3; 		/* fake remote sm state for testing */

#define SM_TCP_TIMEOUT 0

struct mon_entry {
	mon id;
	struct mon_entry *prev;
	struct mon_entry *nxt;
};
typedef struct mon_entry mon_entry;
struct mon_entry *monitor_q;
char *malloc();

sm_stat_res *
sm_stat_1(namep)
	sm_name *namep;
{
	static sm_stat_res resp;

	if (debug)
		printf("proc sm_stat: mon_name = %s\n", namep);

	/* fake answer */
	resp.res_stat = stat_fail;
	resp.state = -1;
	return (&resp);
}

sm_stat_res *
sm_mon_1(monp)
	mon *monp;
{
	static sm_stat_res resp;
/*
	mon_id *monidp;
	monidp = &monp->mon_id;

	if (debug)
		printf("proc sm_mon: mon_name = %s, id = %d\n",
		monidp->mon_name, * ((int * )monp->priv));
*/
	/* store monitor request into monitor_q */
	insert_mon(monp);
	pr_mon();
	resp.res_stat = stat_succ;
	resp.state = local_state;
	return (&resp);
}

sm_stat *
sm_unmon_1(monidp)
	mon_id *monidp;
{
	static sm_stat resp;

/*
	if (debug)
		printf("proc sm_unmon: mon_name = %s, [%s, %d, %d, %d]\n",
		monidp->mon_name, monidp->my_id.my_name,
		monidp->my_id.my_prog, monidp->my_id.my_vers,
		monidp->my_id.my_proc);
*/
	delete_mon(monidp->mon_name, &monidp->my_id);
	pr_mon();
	resp.state = local_state;
	return (&resp);
}

sm_stat *
sm_unmon_all_1(myidp)
	my_id *myidp;
{
	static sm_stat resp;

	if (debug)
		printf("proc sm_unmon_all: [%s, %d, %d, %d]\n",
		myidp->my_name,
		myidp->my_prog, myidp->my_vers,
		myidp->my_proc);

	delete_mon(NULL, myidp);
	pr_mon();
	resp.state = local_state;
	return (&resp);
}

void *
sm_simu_crash_1()
{
	if (debug)
		printf("proc sm_simu_crash\n");
/*
	if (monitor_q != NULL)
*/
	sm_crash();
	return;
}


insert_mon(monp)
	mon *monp;
{
	mon_entry *new, *next;
	my_id *my_idp, *nl_idp;

	if ((new = (mon_entry *) malloc (sizeof (mon_entry))) == 0) {
		perror("rpc.statd: insert_mon: malloc");
		abort();
	}
	bzero (new, sizeof (mon_entry));
	new->id = *monp;
	monp->mon_id.mon_name = NULL;
	monp->mon_id.my_id.my_name = NULL;

	if (debug)
		printf("add_mon(%x) %s (id=%d)\n",
		new, new->id.mon_id.mon_name, * ((int * )new->id.priv));

	record_name(new->id.mon_id.mon_name, 1);
	if (monitor_q == NULL) {
		new->nxt = new->prev = NULL;
		monitor_q = new;
		return;
	}
	else {
		next = monitor_q;
		my_idp = &monp->mon_id.my_id;
		while (next != NULL)  { /* look for other mon_name */
			if (strcmp(next ->id.mon_id.mon_name, new->id.mon_id.mon_name) == 0) { /* found */
				nl_idp = &next->id.mon_id.my_id;
				if (strcmp(new->id.mon_id.my_id.my_name, nl_idp->my_name) == 0 &&
				my_idp->my_prog == nl_idp->my_prog &&
				my_idp->my_vers == nl_idp->my_vers &&
				my_idp->my_proc == nl_idp->my_proc) {
					/* already exists an identical one */
					free(new->id.mon_id.mon_name);
					free(new->id.mon_id.my_id.my_name);
					free(new);
					return;
				}
				else {
					new->nxt = next->nxt;
					if (next->nxt != NULL)
						next->nxt->prev = new;
					next->nxt = new;
					return;
				}
			}
			next = next->nxt;
		}
		/* not found */
		new->nxt = monitor_q;
		if (new->nxt != NULL)
			new->nxt->prev = new;
		monitor_q = new;
		return;
	}
}

delete_mon(mon_name, my_idp)
	char *mon_name;
	my_id *my_idp;
{
	struct mon_entry *next, *nl;
	my_id *nl_idp;

	if (mon_name != NULL)
		record_name(mon_name, 0);

	next = monitor_q;
	while ((nl = next) != NULL) {
		next = next->nxt;
		if (mon_name == NULL || (mon_name != NULL &&
			strcmp(nl ->id.mon_id.mon_name, mon_name) == 0)) {
			nl_idp = &nl->id.mon_id.my_id;
			if (strcmp(my_idp->my_name, nl_idp->my_name) == 0 &&
			my_idp->my_prog == nl_idp->my_prog &&
			my_idp->my_vers == nl_idp->my_vers &&
			my_idp->my_proc == nl_idp->my_proc) {
				/* found */
				if (debug)
					printf("delete_mon(%x): %s\n", nl, mon_name);
				if (nl->prev != NULL)
					nl->prev->nxt = nl->nxt;
				else {
					monitor_q = nl->nxt;
				}
				if (nl->nxt != NULL)
					nl->nxt->prev = nl->prev;
				free(nl->id.mon_id.mon_name);
				free(nl_idp->my_name);
				free(nl);
			}
		} /* end of if mon */
	}
	return;
}

send_notice(mon_name, state)
	char *mon_name;
	int state;
{
	struct mon_entry *next;

	next = monitor_q;
	while (next != NULL) {
		if (strcmp(next->id.mon_id.mon_name, mon_name) == 0) {
			if (statd_call_lockd(&next->id, state) == -1) {
/*
				if (debug)
					printf("problem with notifying %s failure, give up\n", mon_name);
*/
			}
		}
		next = next->nxt;
	}
}

crash_notice()
{
	struct mon_entry *next;

	remote_state ++;
	next = monitor_q;
	while (next != NULL) {
		if (statd_call_lockd(&next->id, remote_state) == -1) {
			/* tcp problem */
		}
		next = next->nxt;
	}
}

recovery_notice()
{
	struct mon_entry *next;

	remote_state = remote_state +2;
	next = monitor_q;
	while (next != NULL) {
		if (statd_call_lockd(&next->id, remote_state) == -1) {
			/* tcp problem */
		}
		next = next->nxt;
	}
}

statd_call_lockd(monp, state)
	mon *monp;
	int state;
{
	struct status stat;
	my_id *my_idp;
	char *mon_name;
	int i, err;

	mon_name = monp->mon_id.mon_name;
	my_idp = &monp->mon_id.my_id;
	bzero (&stat, sizeof (struct status));
	stat.mon_name = mon_name; /* may be dangerous */
	stat.state = state;
	for (i = 0; i < 16; i++) {
		stat.priv[i] = monp->priv[i];
	}
	if (debug)
		printf("statd_call_lockd: %s state = %d\n",
		stat.mon_name, stat.state);
	if ((err = call_tcp(my_idp->my_name, my_idp->my_prog, my_idp->my_vers,
		my_idp->my_proc, xdr_status, &stat, xdr_void, NULL,
		SM_TCP_TIMEOUT)) != (int) RPC_SUCCESS &&
		err != (int) RPC_TIMEDOUT ) {
		fprintf(stderr, "rpc.statd:  cannot contact local lockd on %s status change, give up!\n", mon_name);
		return (-1);
		}
	else
		return (0);
}

pr_mon()
{
	mon_entry *nl;

	if (!debug)
		return;
	if (monitor_q == NULL) {
		printf("*****monitor_q = NULL\n");
		return;
	}

	nl= monitor_q;
	printf("*****monitor_q: ");
	while (nl != NULL) {
		printf("(%x), ", nl);
		nl = nl->nxt;
	}
	printf("\n");
}
