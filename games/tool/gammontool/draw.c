#ifndef lint
static	char sccsid[] = "@(#)draw.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <suntool/tool_hs.h>
#include "defs.h"

static short    gray25_data[] = {
#include <images/square_25.pr>
};
mpr_static(gray25_patch, 16, 16, 1, gray25_data);

static short    gray75_data[] = {
#include <images/square_75.pr>
};
mpr_static(gray75_patch, 16, 16, 1, gray75_data);

#define OVERLAP	(CircDiam / 3)

extern struct toolsw	*brdsw;
extern struct pixwin	*brdwin;
extern struct pixfont	*msgfont;
extern struct rect	brdrect;

	/* 2 sets of two triangles each for top and bottom and for two */
	/* colors 0 and 2 are for top, 1 and 3 for bottom; 0 and 1 are */
	/* light gray and 2 and 3 are dark gray */
static struct pixrect *Triangle[4];
static struct pixrect *Disk;	/* circle pattern for playing pieces */
static struct pixrect *Circle;	/* outline of a circle for white piece */
static struct pixrect *EraseBuf;/* memory for reconstructing the scene */
				/* behind a piece which is removed */
extern struct pixrect *dice[6];	/* die faces */
extern struct pixrect *dblecubes[7][2];	/* doubling cube faces */

static int	TriHeight,		/* height of a point on the board */
		TriWidth,		/* width of a point */
		CircDiam;		/* diameter of a playing piece */

static struct timeval flashdelay = {0, 50000};

changeboardsize()
{
	struct pixrect *pattern[2];
	struct pixrect *stencil[2];
	int midx, xval, c, x, y, d;
	float height, slope;

	/* First, get the required dimensions */

	win_getsize(brdsw->ts_windowfd, &brdrect);
	TriWidth = brdrect.r_width / 15;
	if (TriWidth % 2 == 0)
		TriWidth--;
	TriHeight = (brdrect.r_height * 3) / 7;

	/* Next, make the required pixrects */

	for (c = 0; c < 4; c++) {
		if (Triangle[c] != 0)
			pr_destroy(Triangle[c]);
		Triangle[c] = mem_create(TriWidth, TriHeight, 1);
	}
	for (c = 0; c < 2; c++)
		pattern[c] = mem_create(TriWidth, TriHeight, 1);
	for (c = 0; c < 2; c++)
		stencil[c] = mem_create(TriWidth, TriHeight, 1);

	/* Next, make two gray shades */

	pr_replrop(pattern[0], 0, 0, TriWidth, TriHeight, PIX_SRC, &gray25_patch, 0, 0);
	pr_replrop(pattern[1], 0, 0, TriWidth, TriHeight, PIX_SRC, &gray75_patch, 0, 0);

	/* Next, make two triangle patterns (one up and one down) */

	slope = (float) TriHeight / (float) TriWidth * 2;
	midx = TriWidth / 2;
	for (height=TriHeight, xval=0; height > 0; height -= slope, xval++) {
		pr_vector(stencil[0], midx - xval, (int)height, midx - xval, 0, PIX_SRC, 1);
		pr_vector(stencil[0], midx + xval, (int)height, midx + xval, 0, PIX_SRC, 1);
		pr_vector(stencil[1], midx - xval, TriHeight - (int)height, midx - xval, TriHeight, PIX_SRC, 1);
		pr_vector(stencil[1], midx + xval, TriHeight - (int)height, midx + xval, TriHeight, PIX_SRC, 1);
	}

	/* Next, stencil gray through pattern to make a gray triangle */

	pr_stencil(Triangle[0], 0, 0, TriWidth, TriHeight, PIX_SRC,
			stencil[0], 0, 0, pattern[0], 0, 0);
	pr_stencil(Triangle[1], 0, 0, TriWidth, TriHeight, PIX_SRC,
			stencil[1], 0, 0, pattern[0], 0, 0);
	pr_stencil(Triangle[2], 0, 0, TriWidth, TriHeight, PIX_SRC,
			stencil[0], 0, 0, pattern[1], 0, 0);
	pr_stencil(Triangle[3], 0, 0, TriWidth, TriHeight, PIX_SRC,
			stencil[1], 0, 0, pattern[1], 0, 0);

	/* Finally, draw a solid black border around triangle */

	for (c = 0; c < 3; c += 2) {
		pr_vector(Triangle[c], 0, 0, midx, TriHeight, PIX_SRC, 1);
		pr_vector(Triangle[c], TriWidth, 0, midx, TriHeight, PIX_SRC, 1);
		pr_vector(Triangle[c+1], 0, TriHeight, midx, 0, PIX_SRC, 1);
		pr_vector(Triangle[c+1], midx, 0, TriWidth, TriHeight, PIX_SRC, 1);
	}

	/* Now, make circle playing pieces */

	CircDiam = TriHeight / 5;
	if (CircDiam > TriWidth-2)
		CircDiam = TriWidth-2;
	if (Disk != 0) {
		pr_destroy(Disk);
		pr_destroy(Circle);
		pr_destroy(EraseBuf);
	}
	Disk = mem_create(CircDiam, CircDiam, 1);
	Circle = mem_create(CircDiam, CircDiam, 1);
	EraseBuf = mem_create(TriWidth, CircDiam, 1);

	/* Draw filled circle pattern */

	x = 0;
	y = CircDiam / 2 - 1;
	d = 3 - 2 * (CircDiam / 2 - 1);
	while (x < y) {
		pr_vector(Disk, CircDiam/2-y, CircDiam/2+x, CircDiam/2+y, CircDiam/2+x, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-x, CircDiam/2+y, CircDiam/2+x, CircDiam/2+y, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-y, CircDiam/2-x, CircDiam/2+y, CircDiam/2-x, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-x, CircDiam/2-y, CircDiam/2+x, CircDiam/2-y, PIX_SRC, 1);
		pr_put(Circle, CircDiam/2-y, CircDiam/2+x, 1);
		pr_put(Circle, CircDiam/2+y, CircDiam/2+x, 1);
		pr_put(Circle, CircDiam/2-x, CircDiam/2+y, 1);
		pr_put(Circle, CircDiam/2+x, CircDiam/2+y, 1);
		pr_put(Circle, CircDiam/2-y, CircDiam/2-x, 1);
		pr_put(Circle, CircDiam/2+y, CircDiam/2-x, 1);
		pr_put(Circle, CircDiam/2-x, CircDiam/2-y, 1);
		pr_put(Circle, CircDiam/2+x, CircDiam/2-y, 1);
		if (d < 0) {
			d = d + 4 * x + 6;
		} else {
			d = d + 4 * (x - y) + 10;
			y--;
		}
		x++;
	}
	if (x == y) {
		pr_vector(Disk, CircDiam/2-y, CircDiam/2+x, CircDiam/2+y, CircDiam/2+x, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-x, CircDiam/2+y, CircDiam/2+x, CircDiam/2+y, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-y, CircDiam/2-x, CircDiam/2+y, CircDiam/2-x, PIX_SRC, 1);
		pr_vector(Disk, CircDiam/2-x, CircDiam/2-y, CircDiam/2+x, CircDiam/2-y, PIX_SRC, 1);
		pr_put(Circle, CircDiam/2-y, CircDiam/2+x, 1);
		pr_put(Circle, CircDiam/2+y, CircDiam/2+x, 1);
		pr_put(Circle, CircDiam/2-x, CircDiam/2+y, 1);
		pr_put(Circle, CircDiam/2+x, CircDiam/2+y, 1);
		pr_put(Circle, CircDiam/2-y, CircDiam/2-x, 1);
		pr_put(Circle, CircDiam/2+y, CircDiam/2-x, 1);
		pr_put(Circle, CircDiam/2-x, CircDiam/2-y, 1);
		pr_put(Circle, CircDiam/2+x, CircDiam/2-y, 1);
	}

	/* Clean up memory we no longer need */

	for (c = 0; c < 2; c++) {
		pr_destroy(pattern[c]);
		pr_destroy(stencil[c]);
	}
}

redisplay()
{
	pw_lock(brdwin, &brdrect);

	drawboard();
	drawpieces();
	drawdice(BOTH, NOFLASH);
	drawcube();
	drawscore(BOTH);

	pw_unlock(brdwin);
}

static
drawboard()
{
	int xpos, c;

	pw_writebackground(brdwin, 0, 0, brdrect.r_width, brdrect.r_height, PIX_SRC);

	xpos = TriWidth;
	for (c = 0; c < 6; c++) {
		pw_write(brdwin, xpos, 0, TriWidth, TriHeight, PIX_SRC, Triangle[c % 2 == 0 ? 0 : 2], 0, 0);
		pw_write(brdwin, xpos, brdrect.r_height - TriHeight, TriWidth, TriHeight, PIX_SRC, Triangle[c % 2 == 0 ? 3 : 1], 0, 0);
		xpos += TriWidth;
	}
	xpos += TriWidth;
	for (c = 0; c < 6; c++) {
		pw_write(brdwin, xpos, 0, TriWidth, TriHeight, PIX_SRC, Triangle[c % 2 == 0 ? 0 : 2], 0, 0);
		pw_write(brdwin, xpos, brdrect.r_height - TriHeight, TriWidth, TriHeight, PIX_SRC, Triangle[c % 2 == 0 ? 3 : 1], 0, 0);
		xpos += TriWidth;
	}
	pw_vector(brdwin, TriWidth, 0, TriWidth, brdrect.r_height, PIX_SRC, 1);
	pw_vector(brdwin, 7*TriWidth, 0, 7*TriWidth, brdrect.r_height, PIX_SRC, 1);
	pw_vector(brdwin, 8*TriWidth, 0, 8*TriWidth, brdrect.r_height, PIX_SRC, 1);
	pw_vector(brdwin, 14*TriWidth, 0, 14*TriWidth, brdrect.r_height, PIX_SRC, 1);
}

drawpieces()
{
	int c;

	for (c = 0; c < 26; c++)
		drawpoint(c);
}

drawpoint(point)	/* draw the pieces on a point */
int point;
{
	if (point == BAR) {
		putpiecesonpnt(leftcoor(point), 1, humanboard[point], humancolor);
		putpiecesonpnt(leftcoor(point), 0, computerboard[point], computercolor);
	} else if (point == HOME) {
		putpiecesonpnt(leftcoor(point), 0, humanboard[point], humancolor);
		putpiecesonpnt(leftcoor(point), 1, computerboard[point], computercolor);
	} else {
		putpiecesonpnt(leftcoor(point), isontop(point), humanboard[point], humancolor);
		putpiecesonpnt(leftcoor(point), isontop(point), computerboard[point], computercolor);
	}
}

putpiecesonpnt(xcor, top, number, color)
int xcor, top, number, color;
{
	int	offset,	/* if more than 5 pieces on point, must draw */
			/* others on top of first 5 offset by a small */
			/* amount so bottom ones are partially exposed */
		ycor,	/* height at which current piece should be drawn */
		piece;	/* counter for number of pieces to draw */

	if (number == 0)
		return;
	offset = OVERLAP;
	if (top) {	/* top half of board */
		for (ycor=0, piece=0; piece < number;
					ycor += CircDiam, piece++) {
			if (piece == 5 || piece == 10) {
				ycor = offset;
				offset += OVERLAP;
			}
			drawpieceat(xcor, ycor, color);
		}
	} else {		/* bottom half of board */
		for (ycor = brdrect.r_height-CircDiam, piece = 0;
				piece < number;
				ycor -= CircDiam, piece++) {
			if (piece == 5 || piece == 10) {
				ycor = brdrect.r_height - offset - CircDiam;
				offset += OVERLAP;
			}
			drawpieceat(xcor, ycor, color);
		}
	}
}

/* The following procedure must be EXACTLY analogous to drawhiddenpieceat() */

drawpieceat(x, y, color)
int x, y, color;
{
	pw_stencil(brdwin, x, y, CircDiam, CircDiam, PIX_SRC | PIX_COLOR(color == BLACK), Disk, 0, 0, 0, 0, 0);
	if (color == WHITE)	/* must circle the white piece in black */
		pw_write(brdwin, x, y, CircDiam, CircDiam, PIX_SRC | PIX_DST, Circle, 0, 0);
	else		/* and black Pieces in white */
		pw_stencil(brdwin, x, y, CircDiam, CircDiam, PIX_SRC, Circle, 0, 0, 0, 0, 0);
}

leftcoor(col)
int col;
{
	if (col == BAR)
		return(7 * TriWidth + (TriWidth - CircDiam) / 2);
	if (col == HOME)
		return(14 * TriWidth + (TriWidth - CircDiam) / 2);
	if (isontop(col))
		return((col - 12 + (col >= 19 ? 1 : 0)) * TriWidth
						+ (TriWidth - CircDiam) / 2);
	return((13 - col + (col <= 6 ? 1 : 0)) * TriWidth
						+ (TriWidth - CircDiam) / 2);
}

erasepiece(point, color, oldnum)
int point, color, oldnum; /* oldnum is original # of piece on point */
{
	int tri_index, height, xcor, ycor, c, top, overlap;

	/* first, determine correct index into Triangle array for the */
	/* color and orientation we want */

	if (point % 2 == 0)
		tri_index = 2;
	else
		tri_index = 0;
	if (isonbot(point))
		tri_index++;

	/* next, figure out what height on the triangle we should copy from */
	/* also, save the height value for when we rop the buffer later */

	if (oldnum > 10)
		height = 2 * OVERLAP + (oldnum - 11) * CircDiam;
	else if (oldnum > 5)
		height = OVERLAP + (oldnum - 6) * CircDiam;
	else
		height = (oldnum - 1) * CircDiam;
	if (isonbot(point) || (point == BAR && color == computercolor)
				|| (point == HOME && color == humancolor)) {
		ycor = brdrect.r_height - height - CircDiam;
		height = TriHeight - height - CircDiam;
		top = 0;
	} else {
		ycor = height;
		top = 1;
	}

	/* now, white out the buffer */

	pr_rop(EraseBuf, 0, 0, TriWidth, CircDiam, PIX_SRC, 0, 0, 0);

	/* next, draw the right piece of a triangle into the buffer */

	if (point != BAR && point != HOME)
		pr_rop(EraseBuf, 0, 0, TriWidth, CircDiam, PIX_SRC,
				Triangle[tri_index], 0, height);

	/* if this piece is next to a left edge of the board, the */
	/* vertical line must be drawn in too */

	if (point == 12 || point == 13 || point == 6 || point == 19
					|| point == BAR || point == HOME)
		pr_vector(EraseBuf, 0, 0, 0, CircDiam, PIX_SRC, 1);

	/* now, draw in previously hidden circles if necessary */

	xcor = (TriWidth - CircDiam) / 2;
	if (oldnum > 10) {
		overlap = (top ? 1 : 2) * OVERLAP;
		addhiddenpieces(EraseBuf, xcor, color, overlap, oldnum, top);
	}
	if (oldnum > 5) {
		overlap = (top ? 2 : 1) * OVERLAP;
		addhiddenpieces(EraseBuf, xcor, color, overlap, oldnum, top);
	}

	/* finally, compute the x coordinate for ropping in buffer and */
	/* do the rop */

	if (point == BAR)
		xcor = 7 * TriWidth;
	else if (point == HOME)
		xcor = 14 * TriWidth;
	else {
		if (isontop(point))
			c = point - 12;
		else
			c = 13 - point;
		if (c > 6)
			c++;
		xcor = c * TriWidth;
	}

	pw_write(brdwin, xcor, ycor, TriWidth, CircDiam, PIX_SRC,
							EraseBuf, 0, 0);
	if (oldnum == 15 && point != HOME && point != BAR)
		drawdice(BOTH, NOFLASH);	/* since dice get chopped */
}

addhiddenpieces(pr, xcor, color, overlap, num, top)
struct pixrect *pr;
int xcor, color;
int overlap;
int num;			/* old number of pieces on point */
int top;
{
	if ((num != 15 && num != 10) || !top)	/* piece below */
		drawhiddenpieceat(pr, xcor, overlap, CircDiam,
					CircDiam - overlap, color, 0, 0);
	if ((num != 15 && num != 10) || top)	/* piece above */
		drawhiddenpieceat(pr, xcor, 0, CircDiam, overlap, color, 0,
							CircDiam - overlap);
}

/* The following procedure must be EXACTLY analogous to drawpieceat() */

drawhiddenpieceat(pr, dx, dy, w, h, color, sx, sy)
struct pixrect *pr;
int dx, dy, w, h, color, sx, sy;
{
	pr_stencil(pr, dx, dy, w, h, PIX_SRC | PIX_COLOR(color == BLACK), Disk, sx, sy, 0, 0, 0);
	if (color == WHITE)	/* must circle the white piece in black */
		pr_rop(pr, dx, dy, w, h, PIX_SRC | PIX_DST, Circle, sx, sy);
	else		/* and black Pieces in white */
		pr_stencil(pr, dx, dy, w, h, PIX_SRC, Circle, sx, sy, 0, 0, 0);
}

drawpiece(point, color, oldnum)
int point, color, oldnum;
{
	int height, xcor;

	xcor = leftcoor(point);
	if (oldnum > 9)
		height = 2 * OVERLAP + (oldnum - 10) * CircDiam;
	else if (oldnum > 4)
		height = OVERLAP + (oldnum - 5) * CircDiam;
	else
		height = oldnum * CircDiam;
	if (isonbot(point) || (point == BAR && color == computercolor)
					|| (point == HOME && color == humancolor))
		height = brdrect.r_height - height - CircDiam;
	drawpieceat(xcor, height, color);
}

drawdice(who, flash)
int who, flash;
{
	int xcor, ycor, d0, d1, color, numdice;

	if (who == BOTH) {
		drawdice(HUMAN, flash);
		drawdice(COMPUTER, flash);
		return;
	}
	if (who == HUMAN) {
		d0 = humandieroll[0] & NUMMASK;
		d1 = humandieroll[1] & NUMMASK;
		color = humancolor;
		xcor = 3 * TriWidth;
		numdice = numhdice;
	} else {
		d0 = computerdieroll[0];
		d1 = computerdieroll[1];
		color = computercolor;
		xcor = 10 * TriWidth;
		numdice = numcdice;
	}
	ycor = TriHeight + (brdrect.r_height - 2 * TriHeight - 32) / 2;
	pw_writebackground(brdwin, xcor, ycor, 80, 32, PIX_SRC);
	if (numdice == 0)
		return;
	if (numdice == 1)
		xcor += 24;
	if (flash == FLASH) {
		displaydie(xcor, ycor, d0, color, PIX_NOT(PIX_SRC));
		if (numdice == 2)
			displaydie(xcor+48, ycor, d1, color, PIX_NOT(PIX_SRC));
		select(0, 0, 0, 0, &flashdelay);
	}
	displaydie(xcor, ycor, d0, color, PIX_SRC);
	if (numdice == 2)
		displaydie(xcor+48, ycor, d1, color, PIX_SRC);
}

displaydie(x, y, num, color, op)
int x, y, num, color, op;
{
	if (color)
		pw_write(brdwin, x, y, 32, 32, PIX_NOT(op), dice[num-1], 0, 0);
	else
		pw_write(brdwin, x, y, 32, 32, op, dice[num-1], 0, 0);
}

drawcube()
{
	int turnindex, numindex, gval, xcor, ycor;

	switch (lastdoubled) {
	case HUMAN:
		turnindex = 1;
		break;
	case COMPUTER:
		turnindex = 0;
		break;
	case NOBODY:
		return;
	}
	numindex = 0;
	for (gval = gamevalue; gval > 2; gval /= 2, numindex++)
		;
        xcor = 7 * TriWidth + (TriWidth - 48) / 2;
        ycor = brdrect.r_height / 2 - 24;
	pw_write(brdwin, xcor, ycor, 48, 48, PIX_SRC, dblecubes[numindex][turnindex], 0, 0);
}

clearcube()
{
	int xcor, ycor;

        xcor = 7 * TriWidth + (TriWidth - 48) / 2;
        ycor = brdrect.r_height / 2 - 24;
	pw_write(brdwin, xcor, ycor, 48, 48, PIX_SRC, NULL, 0, 0);
	pw_vector(brdwin, 7 * TriWidth, ycor, 7 * TriWidth, ycor + 48, PIX_SRC, 1);
	pw_vector(brdwin, 8 * TriWidth, ycor, 8 * TriWidth, ycor + 48, PIX_SRC, 1);
}

drawoutline(x, y)
int x, y;
{
	x -= CircDiam / 2;
	y -= CircDiam / 2;
	pw_write(brdwin, x, y, CircDiam, CircDiam, PIX_SRC ^ PIX_DST,
		Circle, 0, 0);
}

getpoint(x, y)		/* given coords on screen, determine board point */
int x, y;
{
	if (!rect_includespoint(&brdrect, x, y))
		return(OUTOFBOUNDS);
	x /= TriWidth;
	if (x < 1)		/* click was off the playing board */
		return(OUTOFBOUNDS);
	if (x == 7)
		return(BAR);
	if (x >= 14)
		return(HOME);
	if (y <= TriHeight + 2 * OVERLAP)	/* top half of board */
		return(x + 12 - (x > 7 ? 1 : 0));
	else if (y >= brdrect.r_height - TriHeight - 2 * OVERLAP)
		return(13 - x + (x > 7 ? 1 : 0));	/* bottom */
	else
		return(OUTOFBOUNDS);	/* click was in between top & bot */
}

drawscore(who)
int who;
{
	int score, height, x;
	char numbuf[6], *text;

	if (who == BOTH) {
		drawscore(HUMAN);
		drawscore(COMPUTER);
	}

	pw_vector(brdwin, 0, brdrect.r_height / 2, TriWidth,
					brdrect.r_height / 2, PIX_SRC, 1);
	if (who == HUMAN) {
		height = 0;
		text = humanname;
		score = humanscore;
	} else {
		height = brdrect.r_height / 2;
		text = "Computer";
		score = computerscore;
	}
	height += ((brdrect.r_height / 2) - ((strlen(text) + 2)
					* msgfont->pf_defaultsize.y))
					/ 2 - msgfont->pf_char['n'].pc_home.y;
	x = (TriWidth - msgfont->pf_defaultsize.x) / 2;
	while (*text) {
		pw_char(brdwin, x, height, PIX_SRC, msgfont, *text++);
		height += msgfont->pf_defaultsize.y;
	}
	height += msgfont->pf_defaultsize.y;
	if (score < 10)
		sprintf(numbuf, " %d ", score);
	else if (score < 100)
		sprintf(numbuf, " %d", score);
	else
		sprintf(numbuf, "%d", score);
	x = (TriWidth - strlen(numbuf) * msgfont->pf_defaultsize.x) / 2;
	pw_text(brdwin, x, height, PIX_SRC, msgfont, numbuf);
}
