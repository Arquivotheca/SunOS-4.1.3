#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)perfmon.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
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
#include <nlist.h>
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
#include <sys/types.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/gfxsw.h>

#define	USEC_INC	50000
#define	SEC_INC		1

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

/*
 * The array stats always has valid info for stats[i], 0 <= i < num_stats.
 * For each valid stats[i], stats[i].value[j] is valid for 0 <= j < num_of_val.
 * The info for the k-th possible statistic of interest is recorded, if it is
 *   recorded at all, in stats[possible_stats[k]].
 */

#define	NO_STAT			-1
#define USER_CPU_PERCENTAGE	0
#define SYSTEM_CPU_PERCENTAGE	1
#define IDLE_CPU_PERCENTAGE	2
#define FREE_MEM		3
#define DISK_TRANSFERS		4
#define INTERRUPTS		5
#define INPUT_PACKETS		6
#define OUTPUT_PACKETS		7
#define COLLISION_PACKETS	8
#define NUM_POSSIBLE_STATS	9
static	int			possible_stats[NUM_POSSIBLE_STATS];
#define WANT_STAT(x)		(possible_stats[(x)] != NO_STAT)

#define	MAX_STATS		10
static	struct statistic	*stats;

static	struct timeval timeout = {SEC_INC,USEC_INC}, timeleft;
static	struct gfxsubwindow *gfx;
static	int num_stats, num_of_val = 0, quit = 0, shift_left_on_new_cycle = -1;
static	int graph_x_offset = 0;
static	struct rect rect;
static	struct pixfont *font;

#define FORALLPOSSIBLESTATS(stat)					\
	for (stat = 0; stat < NUM_POSSIBLE_STATS; stat++)
#define FORALLSTATS(stat)						\
	for (stat = 0; stat < num_stats; stat++)

	/*	************* \/ vmstat \/ ***************	*/
struct nlist nl[] = {
#define X_CPTIME	0
	{ "_cp_time" },
#define X_RATE	  1
	{ "_rate" },
#define X_TOTAL	 2
	{ "_total" },
#define X_DEFICIT       3
	{ "_deficit" },
#define X_FORKSTAT      4
	{ "_forkstat" },
#define X_SUM	   5
	{ "_sum" },
#define X_FIRSTFREE     6
	{ "_firstfree" },
#define X_MAXFREE       7
	{ "_maxfree" },
#define X_BOOTTIME      8
	{ "_boottime" },
#define X_DKXFER	9
	{ "_dk_xfer" },
#define X_REC	   10
	{ "_rectime" },
#define X_PGIN	  11
	{ "_pgintime" },
#define X_HZ	    12
	{ "_hz" },
#define X_MBDINIT       13
	{ "_mbdinit" },
#define N_IFNET       14
	{ "_ifnet" },          /* ******** <-- netstat/if.c ******** */
	{ "" },
};

char	*sprintf(), *strcpy();
long	lseek();

char dr_name[DK_NDRIVE][10];
char dr_unit[DK_NDRIVE];
double  stat1();
int     firstfree, maxfree;
int     hz;
struct
{
	int     busy;
	long    time[CPUSTATES];
	long    xfer[DK_NDRIVE];
	struct  vmmeter Rate;
	struct  vmtotal Total;
	struct  vmmeter Sum;
	struct  forkstat Forkstat;
	unsigned rectime;
	unsigned pgintime;
} s, s1, z;
#define rate	    s.Rate
#define total	   s.Total
#define sum	     s.Sum
#define forkstat	s.Forkstat

struct  vmmeter osum;
int     zero;
int     deficit;
double  etime;
int     mf;
int     swflag;

int nintv;
double f1, f2;
long t;

#define steal(where, var)						\
	(void)lseek(mf, (long)where, 0); (void)read(mf, (char *)(LINT_CAST(&var)), sizeof var);
#define pgtok(a) ((a)*NBPG/1024)
	/*	************** ^ vmstat ^ ****************	*/

char	*options[NUM_POSSIBLE_STATS+1] = {
		"user", "system", "idle", "free", "disk", "interrupts",
		"input", "output", "collision",
		0  /* Terminator! */  };

#ifdef STANDALONE
main(argc, argv)
#else
perfmon_main(argc,argv)
#endif
	int argc;
	char **argv;
{
	extern	struct pixfont *pf_default();
	extern	char *calloc();
	int	perf_mon_selected();
	int	stat;
	struct	inputmask im;
	int	have_disk;
	fd_set  ibits, obits, ebits;

	if ((stats =
	    (struct statistic *)(LINT_CAST(calloc(MAX_STATS, sizeof(struct statistic)))))
	      == (struct statistic *)0) {
		(void)fprintf(stderr, "malloc failed (out of swap space?)\n");
		exit(1);
	}
	nintv = get_namelist("/vmunix", "/dev/kmem");
	collect_stats();
	etime = 1.0;
	have_disk = (total_disk_transfers() ? 1 : 0);

	/* Initialize stats */
	FORALLPOSSIBLESTATS(stat)
		possible_stats[stat] = NO_STAT;
	num_stats = 0;
	if (argv++, --argc) {
	    while (argc--) {
		int option = getcmd(*(argv++), options);
		if (option >= 0 && option < NUM_POSSIBLE_STATS) {
		    if (num_stats == MAX_STATS) {
			(void)fprintf(stderr,
			    "MAX_STATS exceed; please recompile!\n");
		    } else possible_stats[option] = num_stats++;
		} else
		    (void)fprintf(stderr, "'%s' is not a valid option\n", *(argv-1));
	    }
	}
	if (num_stats == 0)
	    FORALLPOSSIBLESTATS(stat) {
		if ((stat == DISK_TRANSFERS) && (have_disk == 0)) continue;
		possible_stats[stat] = num_stats++;
		if (num_stats == MAX_STATS) break;
	    }
	init_stat(USER_CPU_PERCENTAGE, 100, "User", " CPU");
	init_stat(SYSTEM_CPU_PERCENTAGE, 100, "System", " CPU");
	init_stat(IDLE_CPU_PERCENTAGE, 100, "Idle", " CPU");
	init_stat(FREE_MEM, 1000, "Free", " memory");
	init_stat(DISK_TRANSFERS, 40, "Disk", " transfers");
	init_stat(INTERRUPTS, 60, "Interrupts", "");
	init_stat(INPUT_PACKETS, (have_disk ? 20 : 40), "Input", " packets");
	init_stat(OUTPUT_PACKETS, (have_disk ? 20 : 40), "Output", " packets");
	init_stat(COLLISION_PACKETS, 10, "Collision", " packets");
	font = pf_default();
	FORALLSTATS(stat) {
	    struct pr_size text_size;
	    text_size = pf_textwidth(
			strlen(stats[stat].label), font, stats[stat].label);
	    graph_x_offset = max(graph_x_offset, text_size.x);
	    text_size = pf_textwidth(
			strlen(stats[stat].label2), font, stats[stat].label2);
	    graph_x_offset = max(graph_x_offset, text_size.x);
	}
	graph_x_offset += 15;
	(void)gettimeofday(&saved_time, (struct timezone *) NULL);
	/* Initialize gfxsw ("take over" kind) */
	if (gfx = gfxsw_init(0, argv)) {
	    /*
	     * Set up input mask
	     */
	    (void)input_imnull(&im);
	    im.im_flags |= IM_ASCII;
	    (void)gfxsw_setinputmask(gfx, &im, &im, WIN_NULLLINK, 0, 1);
	    /*
	     * Main loop
	     */
	    redisplay();
	    timeleft = timeout;
	    FD_ZERO(&ibits); FD_ZERO(&obits); FD_ZERO(&ebits);
	    (void)gfxsw_select(gfx, perf_mon_selected, ibits, obits, ebits, &timeleft);
	    /*
	     * Cleanup
	     */
	    (void)gfxsw_done(gfx);
	    exit(0);
	} else exit(1);
}

getcmd(to_match, table)			/* Modified from ucb/lpr/lpc.c */
	register char *to_match;
	register char **table;
{
	register char *p, *q;
	int found, index_local, nmatches, longest;

	longest = nmatches = 0;
	found = index_local = -1;
	for (p = *table; p; p = *(++table)) {
		index_local++;
		for (q = to_match; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return(index_local);
		if (!*q) {			/* the to_match was a prefix */
			if (q - to_match > longest) {
				longest = q - to_match;
				nmatches = 1;
				found = index_local;
			} else if (q - to_match == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return(-1);
	return(found);
}

init_stat(index_local, maxval, label_1, label_2)
	int index_local, maxval;
	char *label_1, *label_2;
{
	if WANT_STAT(index_local) {
		index_local = possible_stats[index_local];
		stats[index_local].max_val = maxval;
		stats[index_local].label = label_1;
		stats[index_local].label2 = label_2;
	}
}

#define	TIMER_EXPIRED(timer)						\
	(*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0))

perf_mon_selected(gfx_local, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx_local;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
  if (TIMER_EXPIRED(timer) || (gfx_local->gfx_flags & GFX_RESTART)) {
    if (gfx_local->gfx_reps) {
      if (gfx_local->gfx_flags&GFX_DAMAGED)
	(void)gfxsw_handlesigwinch(gfx_local);
      if (gfx_local->gfx_flags & GFX_RESTART) {
	gfx_local->gfx_flags &= ~GFX_RESTART;
	redisplay();
      } else {
	int *target[CPUSTATES-1], trash;
	collect_stats();
	for (trash = 0; trash < CPUSTATES-1; trash++)
	  target[trash] = &trash;
	if WANT_STAT(USER_CPU_PERCENTAGE)
	  target[0] =
	    &stats[possible_stats[USER_CPU_PERCENTAGE]].value[num_of_val];
	if WANT_STAT(SYSTEM_CPU_PERCENTAGE)
	  target[1] =
	    &stats[possible_stats[SYSTEM_CPU_PERCENTAGE]].value[num_of_val];
	if WANT_STAT(IDLE_CPU_PERCENTAGE)
	  target[2] =
	    &stats[possible_stats[IDLE_CPU_PERCENTAGE]].value[num_of_val];
	copy_cpu_stats(target);
	if WANT_STAT(FREE_MEM)
	  stats[possible_stats[FREE_MEM]].value[num_of_val] =
	    pgtok(total.t_free);
	if WANT_STAT(DISK_TRANSFERS)
	  stats[possible_stats[DISK_TRANSFERS]].value[num_of_val] =
	    total_disk_transfers();
	if WANT_STAT(INTERRUPTS)
	  stats[possible_stats[INTERRUPTS]].value[num_of_val] =
	    (rate.v_intr/nintv) - hz;
	if WANT_STAT(INPUT_PACKETS)
	  stats[possible_stats[INPUT_PACKETS]].value[num_of_val] =
	    packets.input - old_packets.input;
	if WANT_STAT(OUTPUT_PACKETS)
	  stats[possible_stats[OUTPUT_PACKETS]].value[num_of_val] =
	    packets.output - old_packets.output;
	if WANT_STAT(COLLISION_PACKETS)
	  stats[possible_stats[COLLISION_PACKETS]].value[num_of_val] =
	    packets.collisions - old_packets.collisions;
	(void)gettimeofday(&current_time, (struct timezone *) NULL);
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
	if (next_display(gfx_local->gfx_pixwin))
	  gfx_local->gfx_reps--;
      }
    } else (void)gfxsw_selectdone(gfx_local);
  }
	if (*ibits & (1 << gfx_local->gfx_windowfd)) {
		struct	inputevent event;

		/*
		 * Read input from window
		 */
		if (input_readevent(gfx_local->gfx_windowfd, &event)) {
			perror("perf_mon");
			return;
		}
		switch (event_action(&event)) {
		case 'f': /* faster usec timeout */
			if (timeout.tv_usec >= USEC_INC)
				timeout.tv_usec -= USEC_INC;
			else {
				if (timeout.tv_sec >= SEC_INC) {
					timeout.tv_sec -= SEC_INC;
					timeout.tv_usec = 1000000-USEC_INC;
				}
			}
			break;
		case 's': /* slower usec timeout */
			if (timeout.tv_usec < 1000000-USEC_INC)
				timeout.tv_usec += USEC_INC;
			else {
				timeout.tv_usec = 0;
				timeout.tv_sec += 1;
			}
			break;
		case 'F': /* faster sec timeout */
			if (timeout.tv_sec >= SEC_INC)
				timeout.tv_sec -= SEC_INC;
			break;
		case 'S': /* slower sec timeout */
			timeout.tv_sec += SEC_INC;
			break;
		case 'R': /* reset */
			timeout.tv_sec = SEC_INC;
			timeout.tv_usec = USEC_INC;
			num_of_val = 0;
			redisplay();
			break;
		case 'q':
		case 'Q':
			quit = -1; (void)gfxsw_selectdone(gfx_local); break;
		case 'h':
		case 'H':
		case '?': /* Help */
			(void)printf("%s\n%s\n%s\n%s\n%s\n%s\n",
				"'s' slower usec timeout",
				"'f' faster usec timeout",
				"'S' slower sec timeout",
				"'F' faster sec timeout",
				"'R' reset timeout and display",
				"'q' or 'Q' quit");
			/*
			 * Don't reset timeout
			 */
			return;
		default:
			(void)gfxsw_inputinterrupts(gfx_local, &event);
		}
	}
	*ibits = *obits = *ebits = 0;
	timeleft = timeout;
	*timer = &timeleft;
}

int total_disk_transfers()
{
	register int i, total_xfers = 0;

	for(i=0; i < DK_NDRIVE; i++)
		total_xfers += s.xfer[i];
	return(total_xfers/etime);
}

copy_cpu_stats(stat)
	int *stat[CPUSTATES-1];
{
	register int i;

	for(i=0; i<CPUSTATES; i++) {
	        float f = stat1(i);
	        if (i == 0) {           /* US+NI */
	                i++;
	                f += stat1(i);
	        }
		if (stat[i-1] != 0)
			*stat[i-1] = f;
	}
}

collect_stats()
{

	off_t ifnetaddr = (long)nl[N_IFNET].n_value;

	/*	************* \/ vmstat \/ ***************	*/
	register int i;

	(void)lseek(mf, (long)nl[X_CPTIME].n_value, 0);
	(void)read(mf, (char *)(LINT_CAST(s.time)), sizeof s.time);
	(void)lseek(mf, (long)nl[X_DKXFER].n_value, 0);
	(void)read(mf, (char *)(LINT_CAST(s.xfer)), sizeof s.xfer);
	if (nintv != 1) {
	        steal((long)nl[X_SUM].n_value, rate);
	} else {
	        steal((long)nl[X_RATE].n_value, rate);
	}
	steal((long)nl[X_TOTAL].n_value, total);
	osum = sum;
	steal((long)nl[X_SUM].n_value, sum);
	steal((long)nl[X_DEFICIT].n_value, deficit);
	etime = 0;
	for (i=0; i < DK_NDRIVE; i++) {
	        t = s.xfer[i];
	        s.xfer[i] -= s1.xfer[i];
	        s1.xfer[i] = t;
	}
	for (i=0; i < CPUSTATES; i++) {
	        t = s.time[i];
	        s.time[i] -= s1.time[i];
	        s1.time[i] = t;
	        etime += s.time[i];
	}
	if(etime == 0.)
	        etime = 1.;
#ifdef notdef
	(void)printf("%2d%2d%2d", total.t_rq, total.t_dw+total.t_pw, total.t_sw);
	(void)printf("%6d%5d", pgtok(total.t_avm), pgtok(total.t_free));
	(void)printf("%4d%3d",
	    swflag ?
	        sum.v_swpin-osum.v_swpin :
	        (rate.v_pgrec - (rate.v_xsfrec+rate.v_xifrec))/nintv,
	    swflag ?
	        sum.v_swpout-osum.v_swpout :
	        (rate.v_xsfrec+rate.v_xifrec)/nintv);
	(void)printf("%4d", pgtok(rate.v_pgpgin)/nintv);
	(void)printf("%4d%4d%4d%4d", pgtok(rate.v_pgpgout)/nintv,
	    pgtok(rate.v_dfree)/nintv, pgtok(deficit), rate.v_scan/nintv);
#endif
	etime /= 60.;
#ifdef notdef
	(void)printf("%4d%4d", (rate.v_intr/nintv), rate.v_syscall/nintv);
	(void)printf("%4d", rate.v_swtch/nintv);
#endif
	nintv = 1;

	/*      ************** ^ vmstat ^ ****************      */

	/*	*********** \/ netstat/if.c \/ *************	*/
	if (nl[N_IFNET].n_value != 0) {
		struct ifnet ifnet;
		steal((long)nl[N_IFNET].n_value, ifnetaddr);
		old_packets = packets;
		packets.input = packets.output = packets.collisions = 0;
		while (ifnetaddr) {
			steal(ifnetaddr, ifnet);
			packets.input += ifnet.if_ipackets;
			packets.output += ifnet.if_opackets;
			packets.collisions += ifnet.if_collisions;
			ifnetaddr = (off_t) ifnet.if_next;
		}
	}
	/*      ************ ^ netstat/if.c ^ **************    */

}

#define	YORIGIN_FOR_STAT(num)					\
	(((num)*rect.r_height)/num_stats)
#define	YMIDPOINT_FOR_STAT(num)					\
	(((num)*rect.r_height+rect.r_height/2)/num_stats)
#define Y_FOR_STAT_VAL(stat, num_of_val)				\
	YORIGIN_FOR_STAT(stat)+5+height_of_stat - min(height_of_stat, (	\
		height_of_stat*(					\
		  stats[stat].value[num_of_val]-stats[stat].min_val)/(	\
		  stats[stat].max_val-stats[stat].min_val)))

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

redisplay()
{
	register int height_of_stat, stat;

	(void)win_getsize(gfx->gfx_windowfd, &rect);
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
		PIX_SRC, font, stats[stat].label);
	    (void)pw_text(gfx->gfx_pixwin, 0,
		YMIDPOINT_FOR_STAT(stat)+font_height,
		PIX_SRC, font, stats[stat].label2);
	    (void)sprintf(temp, "%d", stats[stat].max_val);
	    text_size = pf_textwidth(strlen(temp), font, temp);
	    (void)pw_text(gfx->gfx_pixwin, graph_x_offset-5-text_size.x,
		y_origin_of_stat-3+font_height, PIX_SRC, font, temp);
	    (void)sprintf(temp, "%d", stats[stat].min_val);
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
	newY = Y_FOR_STAT_VAL(stat, start);
	for (j = start; j < stop_plus_one; ) {
		int jcopy = j, oldY = newY;
		do {
			newY = Y_FOR_STAT_VAL(stat, j);
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

int next_display(pw)
	struct pixwin *pw;
{
	int new_cycle = 0, stat, height_of_stat;

	height_of_stat = YORIGIN_FOR_STAT(1) - YORIGIN_FOR_STAT(0) - 10;
	FORALLSTATS(stat) {
	    int newY, oldY;
	    newY = Y_FOR_STAT_VAL(stat, num_of_val);
	    if (num_of_val == 0)
		oldY = newY;
	    else
		oldY = Y_FOR_STAT_VAL(stat, num_of_val-1);
	    (void)pw_vector(pw,
		graph_x_offset+num_of_val, oldY,
		graph_x_offset+num_of_val+1, newY, PIX_SRC, 1);
	    if ((stat != 0) && do_time[num_of_val]) {
		int y_origin_of_stat = YORIGIN_FOR_STAT(stat);
		(void)pw_vector(pw, graph_x_offset+num_of_val, y_origin_of_stat-2,
		    graph_x_offset+num_of_val, y_origin_of_stat+2, PIX_SRC, -1);
	    }
	}
	if (++num_of_val >= NUM_VALS_PER ||
	num_of_val >= rect.r_width-graph_x_offset) {
	    if (shift_left_on_new_cycle) {
		int num_shift_left = (rect.r_width-graph_x_offset)/2;
		int width = (rect.r_width-graph_x_offset) - num_shift_left;
		register int j;
		for (j = num_shift_left; j < num_of_val; j++)
		    do_time[j-num_shift_left] = do_time[j];
		FORALLSTATS(stat) {
		    int ys = YORIGIN_FOR_STAT(stat)+5;
		    for (j = num_shift_left; j < num_of_val; j++)
			stats[stat].value[j-num_shift_left] =
			  stats[stat].value[j];
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
	    new_cycle = -1;
	}
	return(new_cycle);
}

int get_namelist(kernel_name, memory_name)
	char *kernel_name, *memory_name;
{
	/*      ************* \/ vmstat \/ ***************      */
	time_t now;
	time_t boottime;
	register int i;
	int nintv_local;

	nlist(kernel_name, nl);
	if(nl[0].n_type == 0) {
	        (void)fprintf(stderr, "no %s namelist\n", kernel_name);
	        exit(1);
	}
	mf = open(memory_name, 0);
	if (mf < 0) {
	        (void)fprintf(stderr, "cannot open %s\n", memory_name);
	        exit(1);
	}
	steal((long)nl[X_FIRSTFREE].n_value, firstfree);
	steal((long)nl[X_MAXFREE].n_value, maxfree);
	steal((long)nl[X_BOOTTIME].n_value, boottime);
	steal((long)nl[X_HZ].n_value, hz);
	for (i = 0; i < DK_NDRIVE; i++) {
	        (void)strcpy(dr_name[i], "xx");
	        dr_unit[i] = i;
	}
	read_names();
	(void)time(&now);
	nintv_local = now - boottime;
	if (nintv_local <= 0 || nintv_local > 60*60*24*365*10) {
	        (void)fprintf(stderr,
		    "Time makes no sense... namelist must be wrong.\n");
	       exit(1);
	}
	/*	************** ^ vmstat ^ ****************	*/
	return(nintv_local);
}

	/*	************* \/ vmstat \/ ***************	*/
double
stat1(row)
{
        double tt;
        register i;

        tt = 0;
        for(i=0; i<CPUSTATES; i++)
                tt += s.time[i];
        if(tt == 0.)
                tt = 1.;
        return(s.time[row]*100./tt);
}

read_names()
{
	struct mb_device mdev;
	register struct mb_device *mp;
	struct mb_driver mdrv;
	short two_char;
	char *cp = (char *) &two_char;

	mp = (struct mb_device *) nl[X_MBDINIT].n_value;
	if (mp == 0) {
	        (void)fprintf(stderr, "vmstat: Disk init info not in namelist\n");
	        exit(1);
	}
	for (;;) {
	        steal(mp++, mdev);
	        if (mdev.md_driver == 0)
	                break;
	        if (mdev.md_dk < 0 || mdev.md_alive == 0)
	                continue;
	        steal(mdev.md_driver, mdrv);
	        steal(mdrv.mdr_dname, two_char);
	        (void)sprintf(dr_name[mdev.md_dk], "%c%c", cp[0], cp[1]);
	        dr_unit[mdev.md_dk] = mdev.md_unit;
	}
}
	/*      ************** ^ vmstat ^ ****************      */


