/* @(#)sm_inter.x 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Status monitor protocol specification
 * Copyright (C) 1986 Sun Microsystems, Inc.
 *
 */


program SM_PROG { 
	version SM_VERS  {
		/* res_stat = stat_succ if status monitor agrees to monitor */
		/* res_stat = stat_fail if status monitor cannot monitor */
		/* if res_stat == stat_succ, state = state number of site sm_name */
		struct sm_stat_res			 SM_STAT(struct sm_name) = 1;

		/* res_stat = stat_succ if status monitor agrees to monitor */
		/* res_stat = stat_fail if status monitor cannot monitor */
		/* stat consists of state number of local site */
		struct sm_stat_res			 SM_MON(struct mon) = 2;

		/* stat consists of state number of local site */
		struct sm_stat				 SM_UNMON(struct mon_id) = 3;

		/* stat consists of state number of local site */
		struct sm_stat				 SM_UNMON_ALL(struct my_id) = 4;

		void					 SM_SIMU_CRASH(void) = 5;

	} = 1;
} = 100024;

const	SM_MAXSTRLEN = 1024;

struct sm_name {
	string mon_name<SM_MAXSTRLEN>;
};

struct my_id {
	string	 my_name<SM_MAXSTRLEN>;		/* name of the site iniates the monitoring request*/
	int	my_prog;			/* rpc program # of the requesting process */
	int	my_vers;			/* rpc version # of the requesting process */
	int	my_proc;			/* rpc procedure # of the requesting process */
};

struct mon_id {
	string	mon_name<SM_MAXSTRLEN>;		/* name of the site to be monitored */
	struct my_id my_id;
};


struct mon{
	struct mon_id mon_id;
	opaque priv[16]; 		/* private information to store at monitor for requesting process */
};


/*
 * state # of status monitor monitonically increases each time
 * status of the site changes:
 * an even number (>= 0) indicates the site is down and
 * an odd number (> 0) indicates the site is up;
 */
struct sm_stat {
	int state;		/* state # of status monitor */
};

enum res {
	stat_succ = 0,		/* status monitor agrees to monitor */
	stat_fail = 1		/* status monitor cannot monitor */
};

struct sm_stat_res {
	res res_stat;
	int state;
};

/* 
 * structure of the status message sent back by the status monitor
 * when monitor site status changes
 */
struct status {
	string mon_name<SM_MAXSTRLEN>;
	int state;
	opaque priv[16];		/* stored private information */
};
