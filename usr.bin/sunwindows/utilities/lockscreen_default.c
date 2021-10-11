#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)lockscreen_default.c 1.1 92/07/30";
#endif
#endif

/*
 * 	lockscreen_default.c:	Life for lockscreen
 *				Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>

#include <sunwindow/cms.h>

#include <suntool/tool_hs.h>
#include <suntool/gfxsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

static	struct	gfxsubwindow *gfx;
static	struct	rect rectcreate;

#define STARTGLIDER 40
#define CLEARSCREEN 290
static	int startglider = STARTGLIDER;
static	int clearscreen = CLEARSCREEN;
static	int life_count;
static	int pause_var;			/* Set at the end of a cycle */

static	struct timeval sectimer = {1, 0};
static	struct timeval tooltimer = {1, 0};
static	struct timeval pausetimer = {3, 0};

static  colormonitor;			/* true if running on color monitor */

struct pixrect *logopixrect; /* FIXME */

static	gfx_selected();

/* arrays which get malloc'd to reduce BSS load in a toolmerge	*/

#define MAXPIXELS 2048		/* maximum pixel width of any screen */
#define SPACING 33
#define HASHSIZE 32

static char	  (*board)[MAXPIXELS/SPACING][MAXPIXELS/SPACING];
static int	  *addx, *addy, *xarr, *yarr;

struct xy {
	int x,y;
	short cnt;
	struct xy *nxt;		/* next in this bucket */
	struct xy *chain;	/* for chaining nonzero entries together */
};
static struct xy  *(*hash)[HASHSIZE][HASHSIZE];

static void	   alloc_memory();

long		   random();
char		   *malloc(), *sprintf();

#ifdef STANDALONE
main(argc, argv)
#else
int lockscreen_default_main(argc, argv)
#endif
	int argc;
	char **argv;
{   
    char   *progname;
    Cursor cursor;
	fd_set	ibits, obits, ebits;


    progname = argv[0];
    argv++;
    argc--;

    alloc_memory();
    while (argc > 0 && **argv == '-') {
	if (**argv == '-')
	    switch (argv[0][1]) {
	      case 's': 
		if (argc > 1 && isdigit(argv[1][0])) {
		    int k = atoi(argv[1]);
		    sectimer.tv_sec = k/10;
		    sectimer.tv_usec = (k % 10) * 100000;
		}
		else {
		    (void)fprintf(stderr,
			    "%s -s needs integer argument\n", progname);
		    goto default_case;
		}
		argv++;
		argc--;
		break;

	      case 'c':
		if (argc > 1 && isdigit(argv[1][0])) {
		    int k = atoi(argv[1]);
		    clearscreen = k;
		}
		else {
		    (void)fprintf(stderr,
			    "%s-c needs integer argument\n", progname);
		    goto default_case;
		}
		argv++;
		argc--;
		break;

	      case 'g':
		if (argc > 1 && isdigit(argv[1][0])) {
		    int k = atoi(argv[1]);
		    startglider = k;
		}
		else {
		    (void)fprintf(stderr,
			    "%s -g needs integer argument\n", progname);
		    goto default_case;
		}
		argv++;
		argc--;
		break;

	      default_case:
	      default:
		(void)fprintf(stderr,
	"%s -s tenths-of-seconds -c number-of-cycles -g glider-after-cycle-n\n",
  	                progname);
		EXIT(1);
	    }
	argv++;
	argc--;
    }

    gfx = gfxsw_init(NULL, argv);
    cursor = cursor_create(CURSOR_SHOW_CURSOR, 0, 0);
    (void)win_setcursor(gfx->gfx_windowfd, cursor);

    initcolor();

    initlife();

	FD_ZERO(&ibits); FD_ZERO(&obits); FD_ZERO(&ebits);
    /* Wait for input */
    (void)gfxsw_select(gfx, gfx_selected, ibits, obits, ebits, &tooltimer);

    EXIT(0);
}

static
gfx_selected(gfx_local, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx_local;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	
	if (*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)) {
	    drawlife();
	} else if (*ibits) {
	    (void)gfxsw_selectdone(gfx_local);
	}
	
	if (gfx_local->gfx_flags & GFX_DAMAGED) {
	    (void)gfxsw_handlesigwinch(gfx_local);
	}
	if (gfx_local->gfx_flags & GFX_RESTART) {
	    gfx_local->gfx_flags &= ~GFX_RESTART;
	    initlife();
	}

	*ibits = *obits = *ebits = 0;
	tooltimer = pause_var ? pausetimer : sectimer;
	*timer = &tooltimer;
}


#define CMSIZE 32

static
initcolor()
{   
    char cmsname[CMS_NAMESIZE];
    u_char red[CMSIZE];
    u_char green[CMSIZE];
    u_char blue[CMSIZE];
    int i;
    
    (void)sprintf(cmsname, "lockscreen%d", getpid());
    (void)pw_setcmsname(gfx->gfx_pixwin, cmsname);
    
    for(i = 1; i <= CMSIZE-2; i++)
        hsv_to_rgb(((i-1)*(360-225))/(CMSIZE-3) + 225, 255, 255,
            &red[i], &green[i], &blue[i]);
    red[0] = green[0] = blue[0] = 0;
    red[CMSIZE-1] = green[CMSIZE-1] = blue[CMSIZE-1] = 255;

    (void)pw_putcolormap(gfx->gfx_pixwin, 0, CMSIZE, red, green, blue);
    
    colormonitor = gfx->gfx_pixwin->pw_pixrect->pr_depth > 1;
}

/*
 * Convert hue/saturation/value to red/green/blue.
 */
hsv_to_rgb(h, s, v, rp, gp, bp)
	int h, s, v;
	u_char *rp, *gp, *bp;
{
	int i, f;
	u_char p, q, t;

	if (s == 0)
		*rp = *gp = *bp = v;
	else {
		if (h == 360)
			h = 0;
		h = h * 256 / 60;
		i = h / 256 * 256;
		f = h - i;
		p = v * (256 - s) / 256;
		q = v * (256 - (s*f)/256) / 256;
		t = v * (256 - (s * (256 - f))/256) / 256;
		switch (i/256) {
		case 0:
			*rp = v; *gp = t; *bp = p;
			break;
		case 1:
			*rp = q; *gp = v; *bp = p;
			break;
		case 2:
			*rp = p; *gp = v; *bp = t;
			break;
		case 3:
			*rp = p; *gp = q; *bp = v;
			break;
		case 4:
			*rp = t; *gp = p; *bp = v;
			break;
		case 5:
			*rp = v; *gp = p; *bp = q;
			break;
		}
	}
}

/* 
 * stuff for life
 */
#define PAUSE 10

#define INITCOLOR 1

#define NUMPATTERNS 9
#define EIGHT 0
#define PULSAR 1
#define BARBER 2
#define TUMBLER 3
#define HERTZ 4
#define PERIOD4 5
#define PERIOD5 6
#define PERIOD6 7
#define PINWHEEL 8

#define STARTSIZE 13

static struct {	int x,y;}
startpos[STARTSIZE] = {
    {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6},
    {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}
};

static	int rightedge;
static	int bottomedge;

static short logo_data[256]={
#include <images/sun30.icon>
};
static mpr_static(logo_mpr, 64, 64, 1, logo_data);

static
initlife()
{   
    /* 
     * Initialize life variables
     */
    life_count = 0;
    logopixrect = &logo_mpr;
    srandom(time((time_t *)0));
    (void)win_getrect(gfx->gfx_windowfd, &rectcreate);
    rightedge = rectcreate.r_width/SPACING;
    bottomedge = rectcreate.r_height/SPACING;
    clearpicture();
}

static
drawlife()
{
	int x, y, z;
	static int dullcnt = 0;
	static int oldsums[3];
	int newptr, oldptr;
	static int ptr;

	pause_var = 0;
	if (life_count == 0) {
		dullcnt = 0;
		ptr = 0;
		oldsums[0] = 1;
		oldsums[1] = 2;
		oldsums[2] = 3;
		drawpattern((int)(random()%NUMPATTERNS));
	}
	else if (life_count == clearscreen || (life_count > startglider
	    && dullcnt >= PAUSE)) {
		clearpicture();
		pause_var = 1;
		life_count = 0;
		return;
	}
	else if (life_count == startglider) {
		dullcnt = 0;
		z = random()%STARTSIZE;
		x = startpos[z].x;
		y = startpos[z].y;
		simplepaint(x+3,y+2);
		simplepaint(x+4,y+2);
		simplepaint(x+5,y+2);
		simplepaint(x+4,y);
		simplepaint(x+5,y+1);
	}
	newgen();
		if (ptr == 2)
		newptr = 0;
	else
		newptr = ptr+1;
	if (ptr == 0)
		oldptr = 2;
	else
		oldptr = ptr-1;
	oldsums[newptr] = checksum();
	if (oldsums[ptr] == oldsums[newptr]) {
		dullcnt++;
	}
	else if (oldsums[oldptr] == oldsums[newptr]) {
		dullcnt++;
	}
	else
		dullcnt = 0;
	ptr = newptr;
	life_count++;
}

static
lock()
{
	(void)pw_lock(gfx->gfx_pixwin, &rectcreate);
}

static
unlock()
{
	(void)pw_unlock(gfx->gfx_pixwin);
}

static
board_init()
{
	register int i,j;

	(void)pw_writebackground(gfx->gfx_pixwin, 0, 0, rectcreate.r_width,
	    rectcreate.r_height, PIX_CLR);
	for (i = 0; i < rightedge; i++) {
		for (j = 0; j < bottomedge; j++) {
			if ((*board)[i][j])
				paint_stone(i, j, (*board)[i][j]);
		}
	}
}

static
clearpicture()
{
	int i,j;
	
	for(i = 0; i < rightedge; i++)
	for(j = 0; j < bottomedge; j++)
		(*board)[i][j] = 0;
	zerolist();
	refreshboard();
	board_init();
}

static
paint_stone(i, j, color)
{ 
	int diameter;
	int op = (colormonitor)? PIX_SRC|PIX_COLOR(color): PIX_SRC ^ PIX_DST;
	
	diameter = SPACING - 3;
	(void)pw_write(gfx->gfx_pixwin, i*SPACING+2, j*SPACING+2, diameter, diameter,
	    op, logopixrect, 0, 0);
	(*board)[i][j] = color;
} 

static
erase_stone(i, j)
{ 
	int diameter;
	
	diameter = SPACING - 3;
	(void)pw_writebackground(gfx->gfx_pixwin, i*SPACING+2, j*SPACING+2,
	    diameter, diameter, PIX_CLR);
	(*board)[i][j] = 0;
} 

static
simplepaint(x,y)
{
	paint_stone(x,y,INITCOLOR);
	addpoint(x,y);
}

#define MAXINT 0x7fffffff
#define MININT 0x80000000
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define STACK 2000		/* max number of points added at any one
				   step */

static int stack;

struct point {
    unsigned char p_color;
    int p_y;			/* x from row pointer */
    short p_delete;
    struct point *p_nxt;
};

static
struct row {
    int r_x;
    struct point *r_point;
    struct row *r_nxt;
} *row;

static
addpoint(x,y)
{
	struct row *rw, *oldrw, *newrw;

	if (row == NULL) {
		row = (struct row *)(LINT_CAST(malloc(sizeof(struct row))));
		row->r_x = x;
		row->r_point = NULL;
		addtorow(row, y);
		row->r_nxt = NULL;
		return;
	}			
	if (x < row->r_x) {
		rw = (struct row *)(LINT_CAST(malloc(sizeof(struct row))));
		rw->r_x = x;
		rw->r_point = NULL;
		addtorow(rw, y);
		rw->r_nxt = row;
		row = rw;
		return;
	}
	for(rw = row, oldrw = row; rw != NULL; oldrw = rw, rw = rw->r_nxt) {
		if (rw->r_x == x) {
			addtorow(rw, y);
			return;
		}
		if (x < rw->r_x)
			break;
	}
	newrw = (struct row *)(LINT_CAST(malloc(sizeof(struct row))));
	newrw->r_x = x;
	newrw->r_point = NULL;
	addtorow(newrw, y);
	newrw->r_nxt = rw;
	oldrw->r_nxt = newrw;
}

static
addtorow(rw, y)
	struct row *rw;
{
	struct point *pt, *oldpt, *newpt;
	
	if (rw->r_point == NULL) {
		pt = (struct point *)(LINT_CAST(malloc(sizeof(struct point))));
		pt->p_color = INITCOLOR;
		pt->p_y = y;
		pt->p_delete = 0;
		pt->p_nxt = NULL;
		rw->r_point = pt;
		return;
	}			
	if (y < rw->r_point->p_y) {
		pt = (struct point *)(LINT_CAST(malloc(sizeof(struct point))));
		pt->p_color = INITCOLOR;
		pt->p_y = y;
		pt->p_delete = 0;
		pt->p_nxt = rw->r_point;
		rw->r_point = pt;
		return;
	}
	for(pt = rw->r_point, oldpt = pt; pt != NULL;
	    oldpt = pt, pt = pt->p_nxt) {
		if (pt->p_y == y)
			return;
		if (y < pt->p_y)
			break;
	}
	newpt = (struct point *)(LINT_CAST(malloc(sizeof(struct point))));
	newpt->p_color = INITCOLOR;
	newpt->p_y = y;
	newpt->p_delete = 0;
	newpt->p_nxt = pt;
	oldpt->p_nxt = newpt;
}

static
deletepoint(x,y)
{
	struct row *rw, *oldrw;
	struct point *pt, *oldpt;
	
	for (rw = row, oldrw = row; rw != NULL; oldrw = rw, rw = rw->r_nxt) {
		if (rw->r_x == x)
			break;
	}
	if (rw == NULL) {
		(void)fprintf(stderr, "tried to delete nonexistent point\n");
		exit(1);
	}
	for (pt = rw->r_point, oldpt = pt; pt != NULL;
	    oldpt = pt, pt = pt->p_nxt) {
		if (pt->p_y == y)
			break;
	}
	if (pt == NULL) {
		(void)fprintf(stderr, "tried to delete nonexistent point\n");
		exit(1);
	}
	if (oldpt == pt) {
		rw->r_point = pt->p_nxt;
		if (rw->r_point == NULL) {
			deleterow(oldrw, rw);			
		}
	}
	else
		oldpt->p_nxt = pt->p_nxt;
	free((char *)(LINT_CAST(pt)));
}

static
deleterow(oldrw, rw)
    struct row *oldrw, *rw;
{
	if (oldrw == rw) {
		if (rw->r_nxt == NULL)
			row = NULL;
		else
			row = rw->r_nxt;
	}
	else
		oldrw->r_nxt = rw->r_nxt;
	free((char *)(LINT_CAST(rw)));
}

static
zerolist()
{
	struct row *rw;
	struct point *pt;

	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
			free((char *)(LINT_CAST(pt)));
		free((char *)(LINT_CAST(rw)));
	}
	row = NULL;
}

static
refreshboard()
{
	struct row *rw;
	struct point *pt;
	int i,j;
	
	for(i = 0; i < rightedge; i++)
	for(j = 0; j < bottomedge; j++)
		(*board)[i][j] = 0;
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
			i = rw->r_x;
			j = pt->p_y;
			if (i >= 0 && i < rightedge && j >= 0
			    && j < bottomedge)
				(*board)[i][j] = pt->p_color;
		}
	}
}

/* 
 * called from lifetool
 */
static
newgen()
{
	struct row *oldrw, *rw;
	struct point *pt, *oldpt;
	int i,j;

	for (rw = row, oldrw = rw; rw != NULL;
	    oldrw = rw, rw = rw->r_nxt) {
		for (pt = rw->r_point, oldpt = pt; pt != NULL;
		    oldpt = pt, pt = pt->p_nxt)
			pt->p_delete = nbors(oldrw, rw, oldpt, pt);
	}

	newstuff();
	lock();
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt){
			if (pt->p_delete >=4 || pt->p_delete <=1) {
				/* 
				 * should delete pt directly
				 */
				i = rw->r_x;
				j = pt->p_y;
				deletepoint(i,j);
				if (i >= 0 && i < rightedge && j >= 0
				    && j < bottomedge && (*board)[i][j]) {
					erase_stone(i, j);
					(*board)[i][j] = 0;
				    }
			}
		}
	}

	if (colormonitor) {
		for (rw = row; rw != NULL; rw = rw->r_nxt)
			for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
				if (pt->p_color < CMSIZE-2)
					pt->p_color++;
	}

	while(stack > 0){
		stack--;
		i = addx[stack];
		j = addy[stack];
		addpoint(i, j);
		if (i >= 0 && i < rightedge && j >= 0
		    && j < bottomedge && (*board)[i][j] == 0) {
			paint_stone(i, j, INITCOLOR);
			(*board)[i][j] = 1;
		}
	}

	if (colormonitor) {
		for (rw = row; rw != NULL; rw = rw->r_nxt) {
			for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
				i = rw->r_x;
				j = pt->p_y;
				if (i >= 0 && i < rightedge && j >= 0
				    && j<bottomedge && pt->p_color > INITCOLOR)
					paint_stone(i, j, (int)pt->p_color);
			}
		}
	}
	unlock();
}

static
nbors(oldrw, rw, oldpt, pt)
	struct row *rw, *oldrw;
	struct point *pt, *oldpt;
{
	struct point *tmppt;
	int x, y;
	int cnt = 0;

	x = rw->r_x;
	y = pt->p_y;
	if (oldpt != pt && oldpt->p_y == y-1)
		cnt++;
	if (pt->p_nxt != NULL && pt->p_nxt->p_y == y+1)
		cnt++;
	if (rw != oldrw && oldrw->r_x == x-1) {
		for(tmppt = oldrw->r_point; tmppt != NULL;
		    tmppt = tmppt->p_nxt) {
			if (tmppt->p_y == y-1)
				cnt++;
			if (tmppt->p_y == y)
				cnt++;
			if (tmppt->p_y == y+1) {
				cnt++;
				break;
			}
			if (tmppt->p_y >= y+1)
				break;
		}
	}
	if (rw->r_nxt != NULL && rw->r_nxt->r_x == x+1) {
		for(tmppt = rw->r_nxt->r_point; tmppt != NULL;
		    tmppt = tmppt->p_nxt) {
			if (tmppt->p_y == y-1)
				cnt++;
			if (tmppt->p_y == y)
				cnt++;
			if (tmppt->p_y == y+1) {
				cnt++;
				break;
			}
			if (tmppt->p_y >= y+1)
				break;
		}
	}
	return cnt;
}


static	struct xy *candidates;

#define addxy(X,Y) \
	xx = (X) % HASHSIZE; \
	yy = (Y) % HASHSIZE; \
	p = (*hash)[xx][yy]; \
	for (;;) { \
		if (p == NULL) { \
			p = (struct xy *)newmalloc(); \
			p->x = (X); \
			p->y = (Y); \
			p->cnt = 1; \
			p->nxt = (*hash)[xx][yy]; \
			(*hash)[xx][yy] = p; \
			break; \
		} \
		if (p->x == (X) && p->y == (Y)) { \
			p->cnt++; \
			if (p->cnt == 3) { \
				p->chain = candidates; \
				candidates = p; \
			} \
			break; \
		} \
		p = p->nxt; \
	}

static
newstuff()
{
	register struct row *rw;
	register struct point *pt;
	register struct xy *p;
	register unsigned int x, y;
	register unsigned int xx,yy;
	static int firsttime = 1;

	if (firsttime) {
		firsttime = 0;
		initchain();
	}
	candidates = NULL;
	bzero((char *)(LINT_CAST(hash)), HASHSIZE * HASHSIZE * sizeof(struct xy *));
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		x = rw->r_x;
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
			y = pt->p_y;
			addxy(x-1, y-1);
			addxy(x-1, y);
			addxy(x-1, y+1);
			addxy(x, y-1);
			addxy(x, y+1);
			addxy(x+1, y-1);
			addxy(x+1, y);
			addxy(x+1, y+1);
		}
	}
	findbirths();
}

static
findbirths()
{
	register struct xy *p;
	
	for (p = candidates; p != NULL; p = p->chain) {
		if (p->cnt == 3)
			addtostack(p->x, p->y);
	}
	restorespace();
}

static
addtostack(i, j)
{
	addx[stack] = i;
	addy[stack++] = j;
	if (stack >= STACK) {
		(void)fprintf(stderr, "overflowed stack\n");
		exit(1);
	}
}

static struct chain {
	struct xy ch_xy;
	struct chain *ch_nxt;
} *chain, *headchain, *endchain;

static
initchain()
{
	headchain = (struct chain *)(LINT_CAST(malloc(sizeof(struct chain))));
	headchain->ch_nxt = NULL;
	endchain = headchain;
	chain = headchain;
}

static
restorespace()
{
	chain = headchain;
}

static
newmalloc()
{
	struct chain *ch;
	
	chain = chain->ch_nxt;
	if (chain == NULL) {
		ch = (struct chain *)(LINT_CAST(malloc(sizeof(struct chain))));
		ch->ch_nxt = NULL;
		endchain->ch_nxt = ch;
		endchain = ch;
		chain = ch;
	}
	return (int)(chain);
}

static
checksum()
{
	register struct row *rw;
	register struct point *pt;
	register int sum;
	
	sum = 0;
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		sum = 7*sum + rw->r_x;
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
			sum = 7*sum + pt->p_y;
	}
	return (sum);
}

static int cnt;

static
drawpattern(num)
{

	init();
	switch(num) {
		case EIGHT: 
			paint(6,6);
			paint(6,7);
			paint(6,8);
			paint(7,6);
			paint(7,7);
			paint(7,8);
			paint(8,6);
			paint(8,7);
			paint(8,8);
			paint(9,9);
			paint(9,10);
			paint(9,11);
			paint(10,9);
			paint(10,10);
			paint(10,11);
			paint(11,9);
			paint(11,10);
			paint(11,11);
			break;
		case PULSAR:
			paint(10,10);
			paint(11,10);
			paint(12,10);
			paint(13,10);
			paint(14,10);
			paint(10,11);
			paint(14,11);
			break;
		case BARBER:
			paint(2,2);
			paint(2,3);
			paint(3,2);
			paint(4,3);
			paint(4,5);
			paint(6,5);
			paint(6,7);
			paint(8,7);
			paint(8,9);
			paint(10,9);
			paint(10,11);
			paint(12,11);
			paint(12,13);
			paint(13,14);
			paint(14,13);
			paint(14,14);
			break;
		case HERTZ:
			paint(2,6);
			paint(2,7);
			paint(2,9);
			paint(2,10);
			paint(3,6);
			paint(3,10);
			paint(4,7);
			paint(4,8);
			paint(4,9);
			paint(6,7);
			paint(6,8);
			paint(6,9);
			paint(7,3);
			paint(7,4);
			paint(7,6);
			paint(7,8);
			paint(7,10);
			paint(7,12);
			paint(7,13);
			paint(8,3);
			paint(8,4);
			paint(8,6);
			paint(8,10);
			paint(8,12);
			paint(8,13);
			paint(9,6);
			paint(9,10);
			paint(10,6);
			paint(10,10);
			paint(11,7);
			paint(11,8);
			paint(11,9);
			paint(13,7);
			paint(13,8);
			paint(13,9);
			paint(14,6);
			paint(14,10);
			paint(15,6);
			paint(15,7);
			paint(15,9);
			paint(15,10);
			break;
		case TUMBLER:
			paint(2,6);
			paint(2,7);
			paint(2,8);
			paint(3,3);
			paint(3,4);
			paint(3,8);
			paint(4,3);
			paint(4,4);
			paint(4,5);
			paint(4,6);
			paint(4,7);
			paint(6,3);
			paint(6,4);
			paint(6,5);
			paint(6,6);
			paint(6,7);
			paint(7,3);
			paint(7,4);
			paint(7,8);
			paint(8,6);
			paint(8,7);
			paint(8,8);
			break;
		case PERIOD4:
			paint(1,3);
			paint(2,2);
			paint(2,4);
			paint(4,1);
			paint(4,2);
			paint(4,6);
			paint(5,1);
			paint(5,7);
			paint(6,4);
			paint(6,6);
			paint(7,3);
			paint(7,4);
			break;
		case PERIOD5:
			paint(1,4);
			paint(1,5);
			paint(2,3);
			paint(2,6);
			paint(3,2);
			paint(3,7);
			paint(4,1);
			paint(4,8);
			paint(5,1);
			paint(5,8);
			paint(6,2);
			paint(6,7);
			paint(7,3);
			paint(7,6);
			paint(8,4);
			paint(8,5);
			break;
		case PERIOD6:
			paint(1,2);
			paint(1,3);
			paint(2,2);
			paint(2,3);
			paint(4,2);
			paint(5,1);
			paint(5,3);
			paint(6,1);
			paint(6,4);
			paint(6,7);
			paint(6,8);
			paint(7,5);
			paint(7,7);
			paint(7,8);
			paint(8,3);
			paint(8,4);
			break;
		case PINWHEEL:
			paint(1,7);
			paint(1,8);
			paint(2,7);
			paint(2,8);
			paint(4,5);
			paint(4,6);
			paint(4,7);
			paint(4,8);
			paint(5,1);
			paint(5,2);
			paint(5,4);
			paint(5,9);
			paint(6,1);
			paint(6,2);
			paint(6,4);
			paint(6,5);
			paint(6,9);
			paint(7,4);
			paint(7,7);
			paint(7,9);
			paint(7,11);
			paint(7,12);
			paint(8,4);
			paint(8,6);
			paint(8,9);
			paint(8,11);
			paint(8,12);
			paint(9,5);
			paint(9,6);
			paint(9,7);
			paint(9,8);
			paint(11,5);
			paint(11,6);
			paint(12,5);
			paint(12,6);
			break;
		default:
			return;
	}
	doit();
}

static left;
static right;
static top;
static bottom;

static
paint(x,y)
{
	if (x < left)
		left = x;
	if (x > right)
		right = x;
	if (y < top)
		top = y;
	if (y > bottom)
		bottom = y;
	xarr[cnt] = x;
	yarr[cnt++] = y;
}

static
realpaint(l, r)
{
	int i, x, y;
	
	for (i = 0; i < cnt; i++) {
		x = xarr[i] + l;
		y = yarr[i] + r;
		addpoint(x, y);
		if (x > rightedge-1 || y > bottomedge-1)
			return;
		paint_stone(x, y, INITCOLOR);
	}
}

static
init()
{
	left = MAXINT;
	right = MININT;
	top = MAXINT;
	bottom = MININT;
	cnt = 0;
}

static
doit()
{
	realpaint(-left + (rightedge - (right - left))/2,
	     (-top + (bottomedge - (bottom - top))/2));
}


static void
alloc_memory()
{
    board = (char (*)[MAXPIXELS / SPACING][MAXPIXELS / SPACING])
		malloc(sizeof(*board));
    if ((int) board == 0)
	goto malloc_failed;

    addx = (int *) (LINT_CAST(malloc(STACK * sizeof(int))));
    if (addx == (int *) 0)
	goto malloc_failed;

    addy = (int *) (LINT_CAST(malloc(STACK * sizeof(int))));
    if (addy == (int *) 0)
	goto malloc_failed;

    hash = (struct xy *(*)[HASHSIZE][HASHSIZE]) (LINT_CAST(malloc(sizeof(*hash))));
    if ((int) hash == 0)
	goto malloc_failed;

    xarr = (int *) (LINT_CAST(malloc(200)));
    if (xarr == (int *) 0)
	goto malloc_failed;

    yarr = (int *) (LINT_CAST(malloc(200)));
    if (yarr == (int *) 0)
	goto malloc_failed;

    return;			/* Success  */

malloc_failed:
    (void)fprintf(stderr, "malloc failed (out of swap space?)\n");
    exit(1);
}
