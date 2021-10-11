#ifndef lint
static	char sccsid[] = "@(#)perfmon.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This code was lifted from standard UNIX "perfmon" utility, and then modified
 * to run on a canvas subwindow in Sundiag.
 */

/*
 * Simple graphical performance monitor for system-wide data.
 */
#include <stdio.h>

#include <strings.h>
	/*	************* \/ vmstat \/ ***************	*/
#include <sys/param.h>
#include <sys/vm.h>
#include <sys/dk.h>
/* #include <nlist.h> */
#include <signal.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
	/*      ************** ^ vmstat ^ ****************      */
	/*	*********** \/ netstat/if.c \/ *************	*/
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
	/*      ************ ^ netstat/if.c ^ **************    */
/*	param.h already does ... #include <sys/types.h>	*/
#include <sys/file.h>
#include <sys/time.h>

#include <rpc/rpc.h>
#include <utmp.h>
#include <rpcsvc/rstat.h>
#include <netdb.h>

#include "sundiag.h"		/* -JCH- will also include SunView files */

extern	time_t	time();

struct packet {
	int	input, output, collisions
};
static	struct packet packets, old_packets;

#define NUM_VALS_PER	1000
struct statistic {
	int	min_val, max_val;
	int	value[NUM_VALS_PER];
	char	*label, *label2;
};

#define SECS_PER_TIME_TICK	15
static	char do_time[NUM_VALS_PER];
static	struct timeval current_time, saved_time;
static	struct timezone dummy_zone;
static	struct timeval TIMEOUT = {60, 0};

/*
 * The array stats always has valid info for stats[i], 0 <= i < num_stats.
 * For each valid stats[i], stats[i].value[j] is valid for 0 <= j < num_of_val.
 * The info for the k-th possible statistic of interest is recorded, if it is
 *   recorded at all, in stats[possible_stats[k]].
 */

#define	NO_STAT			-1
#define USER_CPU_PERCENTAGE	0
#define SYSTEM_CPU_PERCENTAGE	1
#define PACKETS			2
#define PAGING			3
#define INTERRUPTS		4
#define DISK_TRANSFERS		5
#define CONTEXT_SW		6
#define LOAD_AVE		7
#define COLLISION_PACKETS	8
#define ERROR_PACKETS		9
#define NUM_POSSIBLE_STATS	10
static	int			possible_stats[NUM_POSSIBLE_STATS];
#define WANT_STAT(x)		(possible_stats[(x)] != NO_STAT)

#define	MAX_STATS		11

static	struct statistic	*p_stats;

static	struct
{
	int		gfx_flags;
	int		gfx_reps;
	struct pixwin	*gfx_pixwin;
	struct rect	gfx_rect;
} mygfx, *gfx = &mygfx;					/* -JCH- */

static	int	first=1;		/* -JCH- the first time to display */

static	int num_stats, num_of_val = 0, shift_left_on_new_cycle = -1;
static	int graph_x_offset = 0;
static	struct rect rect;
static	struct pixfont *font;

#define FORALLPOSSIBLESTATS(stat)					\
	for (stat = 0; stat < NUM_POSSIBLE_STATS; stat++)
#define FORALLSTATS(stat)						\
	for (stat = 0; stat < num_stats; stat++)

	/*	************* \/ vmstat \/ ***************	*/

/* forward declaration */
static	void	redisplay();
extern	xdr_statsswtch();

char	*sprintf(), *strcpy();
long	lseek();
int     firstfree, maxfree;
int     hz;
int     sick;
int     getdata_swtch(); 	/* version 3 (RSTATVERS_SWTCH) of RSTATPROG  */
int     getdata_var();  	/* version 4 (RSTATVERS_VAR) */


struct  vmmeter osum;
int     deficit;
double  etime;
int     mf;

long t;
static  int ans[MAX_STATS]; 
int	debug = 0;

#define steal(where, var)  \
	(void)lseek(mf, (long)where, 0); \
	(void)read(mf, (char *)(LINT_CAST(&var)), sizeof var);

#define pgtok(a) ((a)*NBPG/1024)
	/*	************** ^ vmstat ^ ****************	*/


static  int oldsocket = -1;
#define VER_NONE -1
static  int vers = VER_NONE ;
static  int cpustates ;
static  int dk_ndrive ;
perfmon_main()
{
	extern	char *calloc();
	int	stat;
	int	have_disk;

	if ((p_stats =
	    (struct statistic *)(LINT_CAST(calloc(MAX_STATS,
	     sizeof(struct statistic))))) == (struct statistic *)0) {
		(void)fprintf(stderr, "malloc failed (out of swap space?)\n");
		sundiag_exit(1);
	}
	(void)setup();
	if (vers == RSTATVERS_VAR)
	  (void)getdata_var();
	else
	  (void)getdata_swtch();

	etime = 1.0;
	have_disk = (ans[DISK_TRANSFERS] ? 1 : 0);

	/* Initialize stats */
	FORALLPOSSIBLESTATS(stat)
		possible_stats[stat] = NO_STAT;
	num_stats = 0;

	if (num_stats == 0)
	    FORALLPOSSIBLESTATS(stat) {
		if ((stat == DISK_TRANSFERS) && (have_disk == 0)) continue;
		possible_stats[stat] = num_stats++;
		if (num_stats == MAX_STATS) break;
	    }
	init_stat(USER_CPU_PERCENTAGE, 100, "User", " CPU");
	init_stat(SYSTEM_CPU_PERCENTAGE, 100, "System", " CPU");
	init_stat(PACKETS, (have_disk ? 40 : 60), "Total", " packets");
	init_stat(PAGING, 20, "", "Paging");
	init_stat(INTERRUPTS, 400, "Inter-", "    rupts");
	init_stat(DISK_TRANSFERS, 40, "Disk", " transfers ");
	init_stat(CONTEXT_SW, 128, "Context", "  Switches");
	init_stat(LOAD_AVE, 10, "Load", "  Average");
	init_stat(COLLISION_PACKETS, (have_disk ? 4 : 8),
						"Collision", " packets");
	init_stat(ERROR_PACKETS, (have_disk ? 10 : 20), "Error", " packets");

	font = sundiag_font;		/* always use the Sundiag font -JCH- */

	FORALLSTATS(stat) {
	    struct pr_size text_size;
	    text_size = pf_textwidth(
			strlen(p_stats[stat].label), font, p_stats[stat].label);
	    graph_x_offset = max(graph_x_offset, text_size.x);
	    text_size = pf_textwidth(
			strlen(p_stats[stat].label2), font, p_stats[stat].label2);
	    graph_x_offset = max(graph_x_offset, text_size.x);
	}
	graph_x_offset += 15;
	(void)gettimeofday(&saved_time, &dummy_zone);

/* -JCH- modified so that it will run on a canvas subwindow. */
	gfx->gfx_pixwin = canvas_pixwin(sundiag_perfmon);
	(void)window_set(sundiag_perfmon, CANVAS_RESIZE_PROC, redisplay, 0);
/* -JCH- */
}

init_stat(index_local, maxval, label_1, label_2)
	int index_local, maxval;
	char *label_1, *label_2;
{
	if WANT_STAT(index_local) {
		index_local = possible_stats[index_local];
		p_stats[index_local].max_val = maxval;
		p_stats[index_local].label = label_1;
		p_stats[index_local].label2 = label_2;
	}
}


#define	YORIGIN_FOR_STAT(num)					\
	(((num)*rect.r_height)/num_stats)
#define	YMIDPOINT_FOR_STAT(num)					\
	(((num)*rect.r_height+rect.r_height/2)/num_stats - 6)	/* -JCH- */


static	int	Y_FOR_STAT_VAL(stat, num_of_val, height_of_stat)
int	stat, num_of_val, height_of_stat;
{
  int	temp;

  (void)signal(SIGFPE, SIG_IGN);
  temp = YORIGIN_FOR_STAT(stat)+5+height_of_stat - min(height_of_stat,
	 (height_of_stat*(p_stats[stat].value[num_of_val]-p_stats[stat].min_val)/
	 (p_stats[stat].max_val-p_stats[stat].min_val)));
  (void)signal(SIGFPE, SIG_DFL);
  return(temp);
}

display_dividers(pw, clear_first)
	struct pixwin *pw;
	int clear_first;
{
	register int	i, stat;
	struct rect	div_rect;

	div_rect.r_left = graph_x_offset;
	div_rect.r_width = rect.r_width-graph_x_offset;
	div_rect.r_height = 5;
	FORALLSTATS(stat) {
	    register int y_origin_of_stat = YORIGIN_FOR_STAT(stat);
	    if (stat == 0)
		continue;
	    div_rect.r_top = y_origin_of_stat-2;
	    (void)pw_lock(pw, &div_rect);
	    if (clear_first)
		(void)pw_writebackground(pw, div_rect.r_left, div_rect.r_top,
		    div_rect.r_width, div_rect.r_height, PIX_CLR);
	    /* Draw the horizontal line and then add the tick marks */
	    (void)pw_vector(pw, div_rect.r_left, y_origin_of_stat,
		rect.r_width, y_origin_of_stat, PIX_SRC, -1);
	    for (i = 0; i < num_of_val; i++) {
		if (do_time[i])
		    (void)pw_vector(pw, graph_x_offset+i, div_rect.r_top,
			graph_x_offset+i, y_origin_of_stat+2, PIX_SRC, -1);
	    }
	    (void)pw_unlock(pw);
	}
}

static	void redisplay()
{
	register int height_of_stat, stat;

	rect = *(struct rect *)window_get(sundiag_perfmon, WIN_RECT);

	(void)pw_writebackground(gfx->gfx_pixwin, 0, 0,
	    rect.r_width, rect.r_height, PIX_CLR);

	display_dividers(gfx->gfx_pixwin, 0);
	height_of_stat = YORIGIN_FOR_STAT(1) - YORIGIN_FOR_STAT(0) - 10;
	FORALLSTATS(stat) {
	    register int font_height = (font->pf_defaultsize.y)+2;
	    register int y_origin_of_stat = YORIGIN_FOR_STAT(stat);
	    struct pr_size text_size;
	    char temp[10];
	    (void)pw_text(gfx->gfx_pixwin, 0, YMIDPOINT_FOR_STAT(stat),
		PIX_SRC, font, p_stats[stat].label);
	    (void)pw_text(gfx->gfx_pixwin, 0,
		YMIDPOINT_FOR_STAT(stat)+font_height,
		PIX_SRC, font, p_stats[stat].label2);
	    (void)sprintf(temp, "%d", p_stats[stat].max_val);
	    text_size = pf_textwidth(strlen(temp), font, temp);
	    (void)pw_text(gfx->gfx_pixwin, graph_x_offset-5-text_size.x,
		y_origin_of_stat-3+font_height, PIX_SRC, font, temp);
	    (void)sprintf(temp, "%d", p_stats[stat].min_val);
	    text_size = pf_textwidth(strlen(temp), font, temp);
	    (void)pw_text(gfx->gfx_pixwin, graph_x_offset-5-text_size.x,
		y_origin_of_stat+8+height_of_stat, PIX_SRC, font, temp);
	}
	if (num_of_val > 0) FORALLSTATS(stat) {
	    redisplay_stat_values(
		gfx->gfx_pixwin, height_of_stat, stat, 0, num_of_val);
	}
}

redisplay_stat_values(pw, height_of_stat, stat, start, stop_plus_one)
	struct pixwin *pw;
	int height_of_stat, stat, start, stop_plus_one;
{
	int j, newY;
	(void)pw_lock(pw, &rect);
	newY = Y_FOR_STAT_VAL(stat, start, height_of_stat);
	for (j = start; j < stop_plus_one; ) {
		int jcopy = j, oldY = newY;
		do {
			newY = Y_FOR_STAT_VAL(stat, j, height_of_stat);
			j++;
		} while ((oldY == newY) && (j < stop_plus_one));
		if (j > jcopy+1)
			(void)pw_vector(pw, graph_x_offset+jcopy, oldY,
				graph_x_offset+j-2, oldY, PIX_SRC, 1);
		(void)pw_vector(pw, graph_x_offset+j-2, oldY,
			graph_x_offset+j-1, newY, PIX_SRC, 1);
	}
	(void)pw_unlock(pw);
}

set_clipping_equal_fixup(pw)
	struct pixwin *pw;
{
	int screenX, screenY;
	struct rect screenrect;
	screenrect = rect;
	(void)win_getscreenposition(
		pw->pw_clipdata->pwcd_windowfd, &screenX, &screenY);
	screenrect.r_left = screenX;
	screenrect.r_top = screenY;
	(void)rl_free(&pw->pw_clipdata->pwcd_clipping);
	pw->pw_clipdata->pwcd_clipping = pw->pw_fixup;
	pw->pw_fixup = rl_null;
	(void)_pw_setclippers(pw, &screenrect);
}

next_display(pw)
	struct pixwin *pw;
{
	int stat, height_of_stat;

	height_of_stat = YORIGIN_FOR_STAT(1) - YORIGIN_FOR_STAT(0) - 10;
	FORALLSTATS(stat) {
	    int newY, oldY;
	    newY = Y_FOR_STAT_VAL(stat, num_of_val, height_of_stat);
	    if (num_of_val == 0)
		oldY = newY;
	    else
		oldY = Y_FOR_STAT_VAL(stat, num_of_val-1, height_of_stat);
	    (void)pw_vector(pw,
		graph_x_offset+num_of_val, oldY,
		graph_x_offset+num_of_val+1, newY, PIX_SRC, 1);
	    if ((stat != 0) && do_time[num_of_val]) {
		int y_origin_of_stat = YORIGIN_FOR_STAT(stat);
		(void)pw_vector(pw, graph_x_offset+num_of_val,
				y_origin_of_stat-2,
		    		graph_x_offset+num_of_val,
				y_origin_of_stat+2, PIX_SRC, -1);
	    }
	}
	if (++num_of_val >= NUM_VALS_PER ||
	num_of_val >= rect.r_width-graph_x_offset) {
	    if (shift_left_on_new_cycle) {
		int num_shift_left = (rect.r_width-graph_x_offset)/2;
		int width = (rect.r_width-graph_x_offset) - num_shift_left;
		register int j;

		if (num_shift_left < 0) num_shift_left = num_of_val-1;
		/* -JCH- fixed the bug which causes core dump when the window
			 was resized to reveal only the label portion */

		for (j = num_shift_left; j < num_of_val; j++)
		    do_time[j-num_shift_left] = do_time[j];
		FORALLSTATS(stat) {
		    int ys = YORIGIN_FOR_STAT(stat)+5;
		    for (j = num_shift_left; j < num_of_val; j++)
			p_stats[stat].value[j-num_shift_left] =
			  p_stats[stat].value[j];
		    (void)pw_copy(pw, graph_x_offset, ys,
				  width, height_of_stat+1, PIX_SRC,
				  pw, graph_x_offset+num_shift_left, ys);
		    if (!rl_empty(&pw->pw_fixup)) {
			set_clipping_equal_fixup(pw);
			(void)pw_writebackground(
			    pw, 0, 0, rect.r_width, rect.r_height, PIX_SRC);
			redisplay_stat_values(
			    pw, height_of_stat, stat, 0,
			    num_of_val-num_shift_left-1);
			(void)pw_exposed(pw);
		    }
		    (void)pw_writebackground(
			pw, graph_x_offset+num_shift_left, ys,
			width, height_of_stat+1, PIX_SRC);
		}
		num_of_val -= num_shift_left+1;
		display_dividers(pw, 1);
	    } else {
		num_of_val = 0;
	    }
	}
}


#define	TIMER_EXPIRED(timer)						\
	(*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0))


perfmon_update()
{
	int *target[CPUSTATES-1], trash;

	if (first == 1)		/* -JCH- */
	{
	  redisplay();		/* display the graphics for the first time */
	  first = 0;		/* reset the flag */
	}
	/* get stats from rstatd - fill in ans[]. */
	if (vers == RSTATVERS_VAR)
	  (void)getdata_var();
	else
	  (void)getdata_swtch();
	
	if WANT_STAT(USER_CPU_PERCENTAGE)
	  p_stats[possible_stats[USER_CPU_PERCENTAGE]].value[num_of_val] =
	    ans[USER_CPU_PERCENTAGE];
	if WANT_STAT(SYSTEM_CPU_PERCENTAGE)
	  p_stats[possible_stats[SYSTEM_CPU_PERCENTAGE]].value[num_of_val] =
	    ans[SYSTEM_CPU_PERCENTAGE];
	if WANT_STAT(DISK_TRANSFERS)
	  p_stats[possible_stats[DISK_TRANSFERS]].value[num_of_val] =
            ans[DISK_TRANSFERS];
	if WANT_STAT(INTERRUPTS)
	  p_stats[possible_stats[INTERRUPTS]].value[num_of_val] =
	    ans[INTERRUPTS];
	if WANT_STAT(PACKETS)
	  p_stats[possible_stats[PACKETS]].value[num_of_val] =
	    ans[PACKETS];
	if WANT_STAT(ERROR_PACKETS)
	  p_stats[possible_stats[ERROR_PACKETS]].value[num_of_val] =
	    ans[ERROR_PACKETS] / FSCALE;
	if WANT_STAT(COLLISION_PACKETS)
	  p_stats[possible_stats[COLLISION_PACKETS]].value[num_of_val] =
	     ans[COLLISION_PACKETS] / FSCALE;
	if WANT_STAT(LOAD_AVE)
	  p_stats[possible_stats[LOAD_AVE]].value[num_of_val] =
	     ans[LOAD_AVE] / FSCALE;
	if WANT_STAT(PAGING)
	  p_stats[possible_stats[PAGING]].value[num_of_val] =
	     ans[PAGING];
	if WANT_STAT(CONTEXT_SW)
	  p_stats[possible_stats[CONTEXT_SW]].value[num_of_val] =
	     ans[CONTEXT_SW];

	(void)gettimeofday(&current_time, &dummy_zone);
	if (current_time.tv_sec < saved_time.tv_sec) {
	  /* Super-user must have set the clock back */
	  saved_time = current_time;
	  saved_time.tv_sec -= SECS_PER_TIME_TICK;
	}
	if (saved_time.tv_sec+SECS_PER_TIME_TICK <= current_time.tv_sec) {
	  saved_time = current_time;
	  do_time[num_of_val] = 1;
	} else
	  do_time[num_of_val] = 0;

	next_display(gfx->gfx_pixwin);
}


extern char	*remotehost;
static CLIENT	*client, *oldclient;

/*
 * setup sets up a client to use the remote routine RSTATPROG for collecting
 * remote machine statistics.
 */
setup()
{
        enum clnt_stat clnt_stat;
        struct hostent          *hp;
        struct timeval          timeout;
        struct sockaddr_in      serveradr;
        int                     snum = RPC_ANYSOCK;

        bzero((char *)&serveradr, sizeof (serveradr));
        if (remotehost) {
                if ((hp = gethostbyname(remotehost)) == NULL) {
                        (void)fprintf(stderr,
                            "Sorry, host %s not in hosts database\n",
                             remotehost);
                        exit(1);
                }
                bcopy(hp->h_addr, (char *)&serveradr.sin_addr, hp->h_length);
        }        
        else {
                if (hp = gethostbyname("localhost"))
                        bcopy(hp->h_addr, (char *)&serveradr.sin_addr,
                            hp->h_length);
                else
                        serveradr.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
        serveradr.sin_family = AF_INET;
        serveradr.sin_port = 0;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
	if ((vers == VER_NONE) || (vers == RSTATVERS_VAR)) {
		if ((client = clntudp_bufcreate(&serveradr, RSTATPROG,
				RSTATVERS_VAR, timeout, &snum,
				sizeof(struct rpc_msg), UDPMSGSIZE)) == NULL)
			return (-1);

                clnt_stat = clnt_call(client, NULLPROC, xdr_void, 0,xdr_void,
                                        0, TIMEOUT);
                if (clnt_stat == RPC_SUCCESS) {
                        vers = RSTATVERS_VAR;
                } else if (clnt_stat != RPC_PROGVERSMISMATCH) {
                        clnt_destroy(client);
                        return(-1);
                } else {                /* version mismatch */
                        clnt_destroy(client);
                        vers = RSTATVERS_SWTCH;
                }
        }
	if ((vers == VER_NONE) || (vers == RSTATVERS_SWTCH)) {
		snum = RPC_ANYSOCK;
        	if ((client = clntudp_bufcreate(&serveradr, RSTATPROG,
            		RSTATVERS_SWTCH, timeout, &snum, 
			sizeof(struct rpc_msg), sizeof(struct rpc_msg) + 
			sizeof(struct statsswtch))) == NULL) 
                	return(-1);
                clnt_stat = clnt_call(client, NULLPROC, xdr_void, 0,
                                      xdr_void, 0, TIMEOUT);
                if (clnt_stat == RPC_SUCCESS) {
                        vers = RSTATVERS_SWTCH;
                } else {
                        clnt_destroy(client);
                        return (-1);
                }
        }
        if (oldsocket >= 0)
                (void)close(oldsocket);
        oldsocket = snum;
        if (oldclient)
                clnt_destroy(oldclient);
        oldclient = client;
        return(0);
}


#define TRIES	8
/* static data used only by getdata_swtch(), getdata_var() */
static  struct statsswtch p_statsswtch;
static  struct statsvar stats_var;
static  int *oldtime;
static  int total;                      /* Default to zero */
static  int toterr;                     /* Default to zero */
static  int totcoll;                    /* Default to zero */
static  struct timeval tm, oldtm;
static  int *xfer1;
static  int badcnt;                     /* Default to zero */
static  int oldi, olds, oldp, oldsp;
static  int getdata_init_done;          /* Default to zero */

int
getdata_swtch()
{
        register int i, t;
        register int msecs;
        int maxtfer;
        enum clnt_stat clnt_stat;
        int intrs, swtchs, pag, spag;
        int sum, ppersec;

        clnt_stat = clnt_call(client, RSTATPROC_STATS, xdr_void, 0,
                xdr_statsswtch,  &p_statsswtch, TIMEOUT);
        if (clnt_stat == RPC_TIMEDOUT)
                return(NULL);
        if (clnt_stat != RPC_SUCCESS) {
		if (!sick) {
			sick = 1 ;
/*        clnt_perror(client, "cpugetdata"); -JCH- ignored until reason known */
		}
		redisplay();
                return(NULL);
        }

	if (oldtime == (int *)NULL) { /* allocate memory for structures */
		cpustates = 4;          /* compatibility with version 3*/
		dk_ndrive = 4;
/*
		oldtime = (int *) malloc(cpustates * sizeof(int));
		xfer1 = (int *) malloc(dk_ndrive * sizeof(int));
		if ( (oldtime == NULL) || (xfer1 == NULL)) {
			fprintf(stderr,"failed in malloc()\n");
			exit(1);
		}
*/
	}
        for (i = 0; i < cpustates; i++) {
                t = p_statsswtch.cp_time[i];
                p_statsswtch.cp_time[i] -= oldtime[i];
                oldtime[i] = t;
        }
        t = 0;
        for (i = 0; i < cpustates; i++)   /* assume IDLE is last */
                t += p_statsswtch.cp_time[i];
        if (p_statsswtch.cp_time[cpustates -1] + t <= 0) {
                t++;
                badcnt++;
                if (badcnt >= TRIES) {
			sick = 1 ;
			redisplay() ;
/*
                   (void)fprintf(stderr,
                     "perfmon: rstatd on %s returned bad data\n", remotehost);
                        exit(1);
*/
                }
        } else if (sick) {   
                badcnt = sick = 0;
		redisplay() ;
        } else {
		badcnt = 0 ;
        } 
	ans[USER_CPU_PERCENTAGE] = cpu_usage(CP_USER, &p_statsswtch) +
				   cpu_usage(CP_NICE, &p_statsswtch);
	ans[SYSTEM_CPU_PERCENTAGE] = cpu_usage(CP_SYS, &p_statsswtch);
 
        if ( gettimeofday(&tm, (struct timezone *)0) == -1 )
		perror("perfmon: gettimeofday failed");
        msecs = (1000*(tm.tv_sec - oldtm.tv_sec) +
            (tm.tv_usec - oldtm.tv_usec)/1000);
	++msecs;		/* to avoid msecs == 0 */

        sum = p_statsswtch.if_ipackets + p_statsswtch.if_opackets;
        ppersec = 1000*(sum - total) / msecs;
        total = sum;
        ans[PACKETS] = ppersec;
        ans[COLLISION_PACKETS] = FSCALE*(p_statsswtch.if_collisions - totcoll) *
				 1000 / msecs;
        totcoll = p_statsswtch.if_collisions;
        ans[ERROR_PACKETS] = FSCALE*(p_statsswtch.if_ierrors - toterr) *
			     1000 / msecs;
        toterr = p_statsswtch.if_ierrors;
 
        if (!getdata_init_done) {       
                pag = 0;
                spag = 0;
                intrs = 0;
                swtchs = 0;
                getdata_init_done = 1;
 
        } else {
                pag = p_statsswtch.v_pgpgout + p_statsswtch.v_pgpgin - oldp;
                pag = 1000*pag / msecs;
                spag = p_statsswtch.v_pswpout + p_statsswtch.v_pswpin - oldsp;
                spag = 1000*spag / msecs;
                intrs = p_statsswtch.v_intr - oldi;
                intrs = 1000*intrs / msecs;
                swtchs = p_statsswtch.v_swtch - olds;
                swtchs = 1000*swtchs / msecs;
        }
        oldp = p_statsswtch.v_pgpgin + p_statsswtch.v_pgpgout;
        oldsp = p_statsswtch.v_pswpin + p_statsswtch.v_pswpout;
        oldi = p_statsswtch.v_intr;
        olds = p_statsswtch.v_swtch;
        ans[PAGING] = pag;
        ans[INTERRUPTS] = intrs;
        ans[CONTEXT_SW] = swtchs;
        ans[LOAD_AVE] = p_statsswtch.avenrun[0]; /* use the 1 minute average */
	if (debug) {
		printf("ans[LOAD_AVE] = %d; load = %d\n",
			ans[LOAD_AVE], ans[LOAD_AVE]/FSCALE);
		printf("ans[PAGING] = %d\n", ans[PAGING]);
		printf("ans[CONTEXT_SW] = %d\n", ans[CONTEXT_SW]);
		printf("ans[INTERRUPTS] = %d\n", ans[INTERRUPTS]);
		printf("p_statsswtch.if_ierrors = %d\n", p_statsswtch.if_ierrors);
		printf("p_statsswtch.if_collisions = %d\n",
			p_statsswtch.if_collisions);
		printf("msecs = %d\n", msecs);
		printf("ans[PACKETS] = %d\n", ans[PACKETS]);
		printf("ans[COLLISION_PACKETS] = %d\n",
			ans[COLLISION_PACKETS]);
		printf("ans[ERROR_PACKETS] = %d\n\n", ans[ERROR_PACKETS]);
	}
        for (i = 0; i < DK_NDRIVE; i++) {
                t = p_statsswtch.dk_xfer[i];
                p_statsswtch.dk_xfer[i] -= xfer1[i];
                xfer1[i] = t;
        }
        maxtfer = p_statsswtch.dk_xfer[0];
        for (i = 1; i < DK_NDRIVE; i++) {
                if (p_statsswtch.dk_xfer[i] > maxtfer)
                        maxtfer = p_statsswtch.dk_xfer[i];
        }
        maxtfer = (1000*maxtfer) / msecs;
        ans[DISK_TRANSFERS] = maxtfer;
        oldtm = tm;
	for (i=0; i<NUM_POSSIBLE_STATS; i++) {
	    if (ans[i] < 0)
	        ans[i] = 0;
	}
        return (1);
}

getdata_var()
{
        register int i, t;
        register int msecs;
        int maxtfer;
        enum clnt_stat clnt_stat;
        int intrs, swtchs, pag, spag;
        int sum, ppersec;

	if (oldtime == (int *) NULL) {
		stats_var.dk_xfer.dk_xfer_val = (int *) NULL;
		stats_var.cp_time.cp_time_val = (int *) NULL;
	}
        clnt_stat = clnt_call(client, RSTATPROC_STATS, xdr_void, 0,
                xdr_statsvar,  &stats_var, TIMEOUT);
        if (clnt_stat == RPC_TIMEDOUT)
                return(NULL);
        if (clnt_stat != RPC_SUCCESS) {
		if (!sick) {
			sick = 1 ;
/*        clnt_perror(client, "cpugetdata"); -JCH- ignored until reason known */
		}
		redisplay();
                return(NULL);
        }

	if (oldtime == (int *)NULL) { /* allocate memory for structures */
		cpustates = stats_var.cp_time.cp_time_len;          
		dk_ndrive = stats_var.dk_xfer.dk_xfer_len;
		oldtime = (int *) malloc(cpustates * sizeof(int));
		xfer1 = (int *) malloc(dk_ndrive * sizeof(int));
		if ( (oldtime == NULL) || (xfer1 == NULL)) {
			fprintf(stderr,"failed in malloc()\n");
			exit(1);
		}
	}
        for (i = 0; i < cpustates; i++) {
                t = stats_var.cp_time.cp_time_val[i];
                stats_var.cp_time.cp_time_val[i] -= oldtime[i];
                oldtime[i] = t;
        }
        t = 0;
        for (i = 0; i < cpustates - 1; i++)   /* assume IDLE is last */
                t += stats_var.cp_time.cp_time_val[i];
        if (stats_var.cp_time.cp_time_val[cpustates - 1 ] + t <= 0) {
                t++;
                badcnt++;
                if (badcnt >= TRIES) {
			sick = 1 ;
			redisplay() ;
                }
        } else if (sick) {   
                badcnt = sick = 0;
		redisplay() ;
        } else {
		badcnt = 0 ;
        } 
	ans[USER_CPU_PERCENTAGE] = cpu_usage_var(CP_USER, &stats_var) +
				   cpu_usage_var(CP_NICE, &stats_var);

	ans[SYSTEM_CPU_PERCENTAGE] = cpu_usage_var(CP_SYS, &stats_var);
 
        if ( gettimeofday(&tm, (struct timezone *)0) == -1 )
		perror("perfmon: gettimeofday failed");
        msecs = (1000*(tm.tv_sec - oldtm.tv_sec) +
            (tm.tv_usec - oldtm.tv_usec)/1000);
	++msecs;		/* to avoid msecs == 0 */
	if (msecs == 0) msecs = 1;

        sum = stats_var.if_ipackets + stats_var.if_opackets;
        ppersec = 1000*(sum - total) / msecs;
        total = sum;
        ans[PACKETS] = ppersec;
        ans[COLLISION_PACKETS] = FSCALE*(stats_var.if_collisions - totcoll) *
				 1000 / msecs;
        totcoll = stats_var.if_collisions;
        ans[ERROR_PACKETS] = FSCALE*(stats_var.if_ierrors - toterr) *
			     1000 / msecs;
        toterr = stats_var.if_ierrors;
 
        if (!getdata_init_done) {       
                pag = 0;
                spag = 0;
                intrs = 0;
                swtchs = 0;
                getdata_init_done = 1;
 
        } else {
                pag = stats_var.v_pgpgout + stats_var.v_pgpgin - oldp;
                pag = 1000*pag / msecs;
                spag = stats_var.v_pswpout + stats_var.v_pswpin - oldsp;
                spag = 1000*spag / msecs;
                intrs = stats_var.v_intr - oldi;
                intrs = 1000*intrs / msecs;
                swtchs = stats_var.v_swtch - olds;
                swtchs = 1000*swtchs / msecs;
        }
        oldp = stats_var.v_pgpgin + stats_var.v_pgpgout;
        oldsp = stats_var.v_pswpin + stats_var.v_pswpout;
        oldi = stats_var.v_intr;
        olds = stats_var.v_swtch;
        ans[PAGING] = pag;
        ans[INTERRUPTS] = intrs;
        ans[CONTEXT_SW] = swtchs;
        ans[LOAD_AVE] = stats_var.avenrun[0]; /* use the 1 minute average */
	if (debug) {
		printf("ans[LOAD_AVE] = %d; load = %d\n",
			ans[LOAD_AVE], ans[LOAD_AVE]/FSCALE);
		printf("ans[PAGING] = %d\n", ans[PAGING]);
		printf("ans[CONTEXT_SW] = %d\n", ans[CONTEXT_SW]);
		printf("ans[INTERRUPTS] = %d\n", ans[INTERRUPTS]);
		printf("stats_var.if_ierrors = %d\n", stats_var.if_ierrors);
		printf("stats_var.if_collisions = %d\n",
			stats_var.if_collisions);
		printf("msecs = %d\n", msecs);
		printf("ans[PACKETS] = %d\n", ans[PACKETS]);
		printf("ans[COLLISION_PACKETS] = %d\n",
			ans[COLLISION_PACKETS]);
		printf("ans[ERROR_PACKETS] = %d\n\n", ans[ERROR_PACKETS]);
	}
        for (i = 0; i < DK_NDRIVE; i++) {
                t = stats_var.dk_xfer.dk_xfer_val[i];
                stats_var.dk_xfer.dk_xfer_val[i] -= xfer1[i];
                xfer1[i] = t;
        }
/*      Perfmeter Bug# 1028570 fix, old code commented out
 *
 *      maxtfer = stats_var.dk_xfer.dk_xfer_val[0];
 *      for (i = 1; i < DK_NDRIVE; i++) {
 *              if (stats_var.dk_xfer.dk_xfer_val[i] > maxtfer)
 *                      maxtfer = stats_var.dk_xfer.dk_xfer_val[i];
 *      }
*/

	for (i = 0, maxtfer = stats_var.dk_xfer.dk_xfer_val[0]; i < dk_ndrive; i++)
		maxtfer += stats_var.dk_xfer.dk_xfer_val[i];

        maxtfer = (1000*maxtfer) / msecs;
       	ans[DISK_TRANSFERS] = maxtfer;
        oldtm = tm;
	for (i=0; i<NUM_POSSIBLE_STATS; i++) {
	    if (ans[i] < 0)
	        ans[i] = 0;
	}
        return (1);
}

/*
 * cpu_usage(CP_which, stat)
 *	CP_which:  CP_USER, CP_NICE, CP_SYS, or CP_IDLE.
 *	stat:	   pointer to the statistics structure gotten from rpc call.
 *
 * cpu_usage calculates and returns the percentage of CPU usage.
 *
 */ 
int cpu_usage(CP_which, stat)
	int			CP_which;
	struct statsswtch	*stat;
{
	double		total_usage = 0;
	register	i = 0;

	if (CP_which<CP_USER || CP_which>CP_IDLE) {
		fprintf(stderr, "cpu_usage() error\n");
		exit(1);
	}
	for (i=0; i<CPUSTATES; i++)
		total_usage = total_usage + (*stat).cp_time[i];
	if (total_usage == 0.)
		total_usage = 1.;
	return ((*stat).cp_time[CP_which]*100./total_usage);
}

/*
 * cpu_usage_var(CP_which, stat)
 *	CP_which:  CP_USER, CP_NICE, CP_SYS, or CP_IDLE.
 *	stats_var:  pointer to the statistics structure gotten from rpc call.
 *
 * cpu_usage calculates and returns the percentage of CPU usage.
 *
 */ 
int cpu_usage_var(CP_which, stats_var)
	int			CP_which;
	struct statsvar 	*stats_var;
{
	double		total_usage = 0;
	register	i = 0;

	if (CP_which<CP_USER || CP_which>CP_IDLE) {
		fprintf(stderr, "cpu_usage() error\n");
		exit(1);
	}
	for (i=0; i<CPUSTATES; i++)
		total_usage = total_usage + (*stats_var).cp_time.cp_time_val[i];
	if (total_usage == 0.)
		total_usage = 1.;
	return ((*stats_var).cp_time.cp_time_val[CP_which]*100./total_usage);
}

