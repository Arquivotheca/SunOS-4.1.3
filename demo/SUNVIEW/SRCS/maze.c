#ifndef lint
static char sccsid[] = "@(#)maze.c 1.1 92/07/30 SMI";
#endif

#include <sunwindow/window_hs.h>
#include <sunwindow/cms.h>
#include <suntool/gfxsw.h>
#include <stdio.h>

#define COLOR_MAP_SIZE	4
#define LOGOSIZE	7
#define MIN_MAZE_SIZE	3

/* The following limits are adequate for displays up to 2048x2048 */
#define MAX_MAZE_SIZE_X	205
#define MAX_MAZE_SIZE_Y	205

#define LOCK_COUNT_1	50
#define LOCK_COUNT_2	100

#define MOVE_LIST_SIZE  (MAX_MAZE_SIZE_X * MAX_MAZE_SIZE_Y)

#define WALL_TOP	0x8000
#define WALL_RIGHT	0x4000
#define WALL_BOTTOM	0x2000
#define WALL_LEFT	0x1000

#define DOOR_IN_TOP	0x800
#define DOOR_IN_RIGHT	0x400
#define DOOR_IN_BOTTOM	0x200
#define DOOR_IN_LEFT	0x100
#define DOOR_IN_ANY	0xF00

#define DOOR_OUT_TOP	0x80
#define DOOR_OUT_RIGHT	0x40
#define DOOR_OUT_BOTTOM	0x20
#define DOOR_OUT_LEFT	0x10

#define START_SQUARE	0x2
#define END_SQUARE	0x1

#define SQ_SIZE_X	10
#define SQ_SIZE_Y	10

#define NUM_RANDOM	100

static u_char rmap[COLOR_MAP_SIZE], gmap[COLOR_MAP_SIZE], bmap[COLOR_MAP_SIZE];
static long int randnum[NUM_RANDOM];
static struct gfxsubwindow *gfx;
static struct rect pwrect;

static u_short maze[MAX_MAZE_SIZE_X][MAX_MAZE_SIZE_Y];

static struct {
	u_char x;
	u_char y;
	u_char dir;
	} move_list[MOVE_LIST_SIZE], save_path[MOVE_LIST_SIZE], path[MOVE_LIST_SIZE];

static int maze_size_x, maze_size_y, border_x, border_y;
static int sqnum, cur_sq_x, cur_sq_y, path_length;
static int start_x, start_y, start_dir, end_x, end_y, end_dir;
static int maze_restart_flag, lockcount, colorwindows, random_index;
static int logo_x, logo_y, global_restart;

static  u_short icon_data[256] = {
#include <images/lockscreen.icon>
};

mpr_static(icon_mpr, 64, 64, 1, icon_data);


/* main module */
main(argc,argv)
	int argc;
	char **argv;
{

	make_color_map();
	initialize_window();
	clear_window();
	srandom(getpid());
	while (1) {
		set_maze_sizes(argc, argv);
		if (global_restart) {
			global_restart = 0;
			continue;
		}


		initialize_maze();
		if (global_restart) {
			global_restart = 0;
			continue;
		}

		clear_window();
		if (global_restart) {
			global_restart = 0;
			continue;
		}

		draw_maze_border();
		if (global_restart) {
			global_restart = 0;
			continue;
		}

		create_maze();
		if (global_restart) {
			global_restart = 0;
			continue;
		}

		sleep(1);

		solve_maze();
		if (global_restart) {
			global_restart = 0;
			continue;
		}

		sleep(1);
	}
} /*  end of main() */


get_random(modulo)
	int modulo;
{
	return ((random()>>10) % modulo);

} /* end of get_random */


make_color_map() /* make the colormap */
{
	register int i;

	rmap[0] = 0;
	gmap[0] = 0;
	bmap[0] = 0;

	rmap[1] = 255;
	gmap[1] = 255;
	bmap[1] = 255;

	rmap[2] = 255;
	gmap[2] = 0;
	bmap[2] = 0;

	rmap[3] = 90;
	gmap[3] = 150;
	bmap[3] = 255;

} /* end of make_colormap() */


initialize_window() /* take over and initialize window */
{
	char cmsname[CMS_NAMESIZE];
        gfx = gfxsw_init(0,0);
	sprintf(cmsname, "maze%d", COLOR_MAP_SIZE);
        pw_setcmsname(gfx->gfx_pixwin, cmsname);
	colorwindows = gfx->gfx_pixwin->pw_pixrect->pr_depth > 1;
	pw_putcolormap(gfx->gfx_pixwin, 0,
		colorwindows ? COLOR_MAP_SIZE : 2, rmap, gmap, bmap);
	pw_exposed(gfx->gfx_pixwin);
        win_getrect(gfx->gfx_pixwin->pw_clipdata->pwcd_windowfd, &pwrect);
} /* end of initialize_window() */


clear_window() /* blank the window */
{
        pw_rop(gfx->gfx_pixwin, 0, 0,
                gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.x,
                gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.y,
                PIX_CLR, 0, 0, 0);

} /* end of clear_window() */

 
set_maze_sizes(argc, argv)
	int argc;
	char **argv;
{
	border_x = 20;
	border_y = 20;

	maze_size_x = ( gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.x - 
		border_x) / SQ_SIZE_X;
	maze_size_y = ( gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.y - 
		border_y ) / SQ_SIZE_Y;

	if ( maze_size_x < 3 || SQ_SIZE_Y < 3) {
		fprintf(stderr, "maze too small, exitting\n");
		exit (1);
	}

	border_x = ( gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.x -
                maze_size_x * SQ_SIZE_X)/2;
	border_y = ( gfx->gfx_pixwin->pw_clipdata->pwcd_prmulti->pr_size.y -
                maze_size_y * SQ_SIZE_Y)/2;

} /* end of set_maze_sizes */


initialize_maze() /* draw the surrounding wall and start/end squares */
{
	register int i, j, wall;

	/* initialize all squares */
	for ( i=0; i<maze_size_x; i++) {
		for ( j=0; j<maze_size_y; j++) {
			maze[i][j] = 0;
		}
	}

	/* top wall */
	for ( i=0; i<maze_size_x; i++ ) {
		maze[i][0] |= WALL_TOP;
	}

	/* right wall */
	for ( j=0; j<maze_size_y; j++ ) {
		maze[maze_size_x-1][j] |= WALL_RIGHT;
	}

	/* bottom wall */
	for ( i=0; i<maze_size_x; i++ ) {
		maze[i][maze_size_y-1] |= WALL_BOTTOM;
	}

	/* left wall */
	for ( j=0; j<maze_size_y; j++ ) {
		maze[0][j] |= WALL_LEFT;
	}

	/* set start square */
	wall = get_random(4);
	switch (wall) {
		case 0:	
			i = get_random(maze_size_x);
			j = 0;
			break;
		case 1:	
			i = maze_size_x - 1;
			j = get_random(maze_size_y);
			break;
		case 2:	
			i = get_random(maze_size_x);
			j = maze_size_y - 1;
			break;
		case 3:	
			i = 0;
			j = get_random(maze_size_y);
			break;
	}
	maze[i][j] |= START_SQUARE;
	maze[i][j] |= ( DOOR_IN_TOP >> wall );
	maze[i][j] &= ~( WALL_TOP >> wall );
	cur_sq_x = i;
	cur_sq_y = j;
	start_x = i;
	start_y = j;
	start_dir = wall;
	sqnum = 0;

        /* set end square */
        wall = (wall + 2)%4;
        switch (wall) {
                case 0:
			i = get_random(maze_size_x);
                        j = 0;
                        break;
                case 1:
                        i = maze_size_x - 1;
			j = get_random(maze_size_y);
                        break;
                case 2:
			i = get_random(maze_size_x);
                        j = maze_size_y - 1;
                        break;
                case 3:
                        i = 0;
			j = get_random(maze_size_y);
                        break;
        }
        maze[i][j] |= END_SQUARE;
        maze[i][j] |= ( DOOR_OUT_TOP >> wall );
	maze[i][j] &= ~( WALL_TOP >> wall );
	end_x = i;
	end_y = j;
	end_dir = wall;

	/* set logo */
	if ( (maze_size_x > 15) && (maze_size_y > 15) ) {
		logo_x = get_random(maze_size_x - LOGOSIZE - 6) + 3;
		logo_y = get_random(maze_size_y - LOGOSIZE - 6) + 3;

		for (i=0; i<LOGOSIZE; i++) {
			for (j=0; j<LOGOSIZE; j++) {
				maze[logo_x + i][logo_y + j] |= DOOR_IN_TOP;
			}
		}
	}
	else
		logo_y = logo_x = -1;
}


create_maze() /* create a maze layout given the intiialized maze */
{
	register int i, newdoor;

	lockcount = LOCK_COUNT_1;
	pw_lock(gfx->gfx_pixwin, &pwrect);

	do {
		move_list[sqnum].x = cur_sq_x;
		move_list[sqnum].y = cur_sq_y;
		move_list[sqnum].dir = newdoor;
		while ( ( newdoor = choose_door() ) == -1 ) { /* pick a door */
			if ( backup() == -1 ) { /* no more doors ... backup */
				maze_unlock(gfx->gfx_pixwin);
				return; /* done ... return */
			}
		}
		if (global_restart)
			return;

		/* mark the out door */
		maze[cur_sq_x][cur_sq_y] |= ( DOOR_OUT_TOP >> newdoor );
		
		switch (newdoor) {
			case 0: cur_sq_y--;
				break;
			case 1: cur_sq_x++;
				break;
			case 2: cur_sq_y++;
				break;
			case 3: cur_sq_x--;
				break;
		}
		sqnum++;

		/* mark the in door */
		maze[cur_sq_x][cur_sq_y] |= ( DOOR_IN_TOP >> ((newdoor+2)%4) );

		/* if end square set path length and save path */
		if ( maze[cur_sq_x][cur_sq_y] & END_SQUARE ) {
			path_length = sqnum;
			for ( i=0; i<path_length; i++) {
				save_path[i].x = move_list[i].x;
				save_path[i].y = move_list[i].y;
				save_path[i].dir = move_list[i].dir;
			}
		}

	} while (1);

} /* end of create_maze() */


choose_door() /* pick a new path */
{
	int candidates[3];
	register int num_candidates;

	num_candidates = 0;

topwall:
	/* top wall */
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_TOP )
		goto rightwall;
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_TOP )
		goto rightwall;
	if ( maze[cur_sq_x][cur_sq_y] & WALL_TOP )
		goto rightwall;
	if ( maze[cur_sq_x][cur_sq_y - 1] & DOOR_IN_ANY ) {
		maze[cur_sq_x][cur_sq_y] |= WALL_TOP;
		maze[cur_sq_x][cur_sq_y - 1] |= WALL_BOTTOM;
		draw_wall(cur_sq_x, cur_sq_y, 0);
		goto rightwall;
	}
	candidates[num_candidates++] = 0;

rightwall:
	/* right wall */
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_RIGHT )
		goto bottomwall;
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_RIGHT )
		goto bottomwall;
	if ( maze[cur_sq_x][cur_sq_y] & WALL_RIGHT )
		goto bottomwall;
	if ( maze[cur_sq_x + 1][cur_sq_y] & DOOR_IN_ANY ) {
		maze[cur_sq_x][cur_sq_y] |= WALL_RIGHT;
		maze[cur_sq_x + 1][cur_sq_y] |= WALL_LEFT;
		draw_wall(cur_sq_x, cur_sq_y, 1);
		goto bottomwall;
	}
	candidates[num_candidates++] = 1;

bottomwall:
	/* bottom wall */
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_BOTTOM )
		goto leftwall;
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_BOTTOM )
		goto leftwall;
	if ( maze[cur_sq_x][cur_sq_y] & WALL_BOTTOM )
		goto leftwall;
	if ( maze[cur_sq_x][cur_sq_y + 1] & DOOR_IN_ANY ) {
		maze[cur_sq_x][cur_sq_y] |= WALL_BOTTOM;
		maze[cur_sq_x][cur_sq_y + 1] |= WALL_TOP;
		draw_wall(cur_sq_x, cur_sq_y, 2);
		goto leftwall;
	}
	candidates[num_candidates++] = 2;

leftwall:
	/* left wall */
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_IN_LEFT )
		goto donewall;
	if ( maze[cur_sq_x][cur_sq_y] & DOOR_OUT_LEFT )
		goto donewall;
	if ( maze[cur_sq_x][cur_sq_y] & WALL_LEFT )
		goto donewall;
	if ( maze[cur_sq_x - 1][cur_sq_y] & DOOR_IN_ANY ) {
		maze[cur_sq_x][cur_sq_y] |= WALL_LEFT;
		maze[cur_sq_x - 1][cur_sq_y] |= WALL_RIGHT;
		draw_wall(cur_sq_x, cur_sq_y, 3);
		goto donewall;
	}
	candidates[num_candidates++] = 3;

donewall:
	if ( --lockcount == 0) {
		lockcount = LOCK_COUNT_1;
		maze_unlock(gfx->gfx_pixwin);
		if (global_restart)
			return;
		pw_lock(gfx->gfx_pixwin, &pwrect);
	}
	if (num_candidates == 0)
		return ( -1 );
	if (num_candidates == 1)
		return ( candidates[0] );
	return ( candidates[ get_random(num_candidates) ] );

} /* end of choose_door() */


backup() /* back up a move */
{
	sqnum--;
	cur_sq_x = move_list[sqnum].x;
	cur_sq_y = move_list[sqnum].y;
	return ( sqnum );
} /* end of backup() */


draw_maze_border() /* draw the maze outline */
{
	register int i, j;


	pw_lock(gfx->gfx_pixwin, &pwrect);
	for ( i=0; i<maze_size_x; i++) {
		if ( maze[i][0] & WALL_TOP ) {
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * i,
				border_y,
				border_x + SQ_SIZE_X * (i+1),
				border_y,
				PIX_SRC, 1);
		}
		if ((maze[i][maze_size_y - 1] & WALL_BOTTOM)) {
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * i,
				border_y + SQ_SIZE_Y * (maze_size_y),
				border_x + SQ_SIZE_X * (i+1),
				border_y + SQ_SIZE_Y * (maze_size_y),
				PIX_SRC, 1);
		}
	}
	for ( j=0; j<maze_size_y; j++) {
		if ( maze[maze_size_x - 1][j] & WALL_RIGHT ) {
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * maze_size_x,
				border_y + SQ_SIZE_Y * j,
				border_x + SQ_SIZE_X * maze_size_x,
				border_y + SQ_SIZE_Y * (j+1),
				PIX_SRC, 1);
		}
		if ( maze[0][j] & WALL_LEFT ) {
			pw_vector(gfx->gfx_pixwin,
				border_x,
				border_y + SQ_SIZE_Y * j,
				border_x,
				border_y + SQ_SIZE_Y * (j+1),
				PIX_SRC, 1);
		}
	}

	if (logo_x != -1) {
	    if (colorwindows)
		pw_rop(gfx->gfx_pixwin, border_x + 3 + SQ_SIZE_X * logo_x, 
			border_y + 3 + SQ_SIZE_Y * logo_y, 
			64, 64, PIX_SRC | PIX_COLOR(3), &icon_mpr, 0, 0);
	    else
		pw_rop(gfx->gfx_pixwin, border_x + 3 + SQ_SIZE_X * logo_x, 
			border_y + 3 + SQ_SIZE_Y * logo_y, 
			64, 64, PIX_SRC, &icon_mpr, 0, 0);
	}

	maze_unlock( gfx->gfx_pixwin );
	if (global_restart)
		return;
	if (gfx->gfx_flags & GFX_DAMAGED)
		gfxsw_handlesigwinch(gfx);
	draw_solid_square( start_x, start_y, start_dir, 1);
	draw_solid_square( end_x, end_y, end_dir, 1);
} /* end of draw_maze() */


draw_wall(i, j, dir) /* draw a single wall */
	int i, j, dir;
{
	switch (dir) {
		case 0:
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * i, 
				border_y + SQ_SIZE_Y * j,
				border_x + SQ_SIZE_X * (i+1), 
				border_y + SQ_SIZE_Y * j,
				PIX_SRC, 1);
			break;
		case 1:
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * (i+1), 
				border_y + SQ_SIZE_Y * j,
				border_x + SQ_SIZE_X * (i+1), 
				border_y + SQ_SIZE_Y * (j+1),
				PIX_SRC, 1);
			break;
		case 2:
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * i, 
				border_y + SQ_SIZE_Y * (j+1),
				border_x + SQ_SIZE_X * (i+1), 
				border_y + SQ_SIZE_Y * (j+1),
				PIX_SRC, 1);
			break;
		case 3:
			pw_vector(gfx->gfx_pixwin,
				border_x + SQ_SIZE_X * i, 
				border_y + SQ_SIZE_Y * j,
				border_x + SQ_SIZE_X * i, 
				border_y + SQ_SIZE_Y * (j+1),
				PIX_SRC, 1);
			break;
	}
} /* end of draw_wall */


draw_solid_square(i, j, dir, color) /* draw a solid square in a square */
	register int i, j, dir, color;
{
	int op;

	if (colorwindows || color == 0)
		op = PIX_SRC | PIX_COLOR(color);
	else
		op = PIX_SRC | PIX_COLOR(1);

	switch (dir) {
		case 0: pw_rop(gfx->gfx_pixwin, 
			border_x + 3 + SQ_SIZE_X * i, 
			border_y - 3 + SQ_SIZE_Y * j, 
			SQ_SIZE_X - 6, SQ_SIZE_Y, 
			op, 0, 0, 0);
			break;
		case 1: pw_rop(gfx->gfx_pixwin, 
			border_x + 3 + SQ_SIZE_X * i, 
			border_y + 3 + SQ_SIZE_Y * j, 
			SQ_SIZE_X, SQ_SIZE_Y - 6, 
			op, 0, 0, 0);
			break;
		case 2: pw_rop(gfx->gfx_pixwin, 
			border_x + 3 + SQ_SIZE_X * i, 
			border_y + 3 + SQ_SIZE_Y * j, 
			SQ_SIZE_X - 6, SQ_SIZE_Y, 
			op, 0, 0, 0);
			break;
		case 3: pw_rop(gfx->gfx_pixwin, 
			border_x - 3 + SQ_SIZE_X * i, 
			border_y + 3 + SQ_SIZE_Y * j, 
			SQ_SIZE_X, SQ_SIZE_Y - 6, 
			op, 0, 0, 0);
			break;
	}
	if ( --lockcount == 0) {
		lockcount = LOCK_COUNT_2;
		maze_unlock(gfx->gfx_pixwin);
		if (global_restart)
			return;
		pw_lock(gfx->gfx_pixwin, &pwrect);
	}

} /* end of draw_solid_square() */

solve_maze() /* solve it with graphical feedback */
{
	int i;


	/* plug up the surrounding wall */
	maze[start_x][start_y] |= (WALL_TOP >> start_dir);
	maze[end_x][end_y] |= (WALL_TOP >> end_dir);

	/* initialize search path */
	i = 0;
	path[i].x = end_x;
	path[i].y = end_y;
	path[i].dir = -1;

	/* do it */
	lockcount = LOCK_COUNT_2;
	pw_lock(gfx->gfx_pixwin, &pwrect);
	while (1) {
		if ( ++path[i].dir >= 4 ) {
			i--;
			draw_solid_square( (int)(path[i].x), (int)(path[i].y), 
				(int)(path[i].dir), 0);
			if (global_restart)
				return;
		}
		else if ( ! (maze[path[i].x][path[i].y] & 
				(WALL_TOP >> path[i].dir))  && 
				( (i == 0) || ( (path[i].dir != 
				(path[i-1].dir+2)%4) ) ) ) {
			enter_square(i);
			i++;
			if ( maze[path[i].x][path[i].y] & START_SQUARE ) {
				return;
			}
		} 
	maze_unlock(gfx->gfx_pixwin);
	if (global_restart)
		return;
	}
} /* end of solve_maze() */


enter_square(n) /* move into a neighboring square */
	int n;
{
	draw_solid_square( (int)path[n].x, (int)path[n].y, 
		(int)path[n].dir, 2);

	path[n+1].dir = -1;
	switch (path[n].dir) {
		case 0: path[n+1].x = path[n].x;
			path[n+1].y = path[n].y - 1;
			break;
		case 1: path[n+1].x = path[n].x + 1;
			path[n+1].y = path[n].y;
			break;
		case 2: path[n+1].x = path[n].x;
			path[n+1].y = path[n].y + 1;
			break;
		case 3: path[n+1].x = path[n].x - 1;
			path[n+1].y = path[n].y;
			break;
	}


} /* end of enter_square() */


maze_unlock(pw)
	struct pixwin *pw;
{
        pw_unlock(pw);
        if (gfx->gfx_flags & GFX_DAMAGED) {
                gfxsw_handlesigwinch(gfx);
		global_restart = 1;
                }
        if (gfx->gfx_flags & GFX_RESTART) {
                gfx->gfx_flags &= ~GFX_RESTART;
		global_restart = 1;
                }
}
