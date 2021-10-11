#ifndef lint
static	char sccsid[] = "@(#)chessgame.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <math.h>
#include <usercore.h>
#include "demolib.h"

/*
This program plays out a game of chess.  Moves are made every 3 seconds and
the final position is left intact for 5 minutes until normal termination.
This is not a very complete program. For example castling is not dealt with.
It is merely for demonstration purposes.
*/

#define LIGHT_PIECE 	2	/* fill index definitions */
#define DARK_PIECE	3
#define LIGHT_SQUARE	4
#define DARK_SQUARE	5
#define MOVE_TEXT	6
#define HEADING		7

static int cg1dd();
static int cgpixwindd();
static int gp1dd();
static int gp1pixwindd();

static float red[16];
static float grn[16];
static float blu[16];

static float square_x[] =	/* square offsets */
	{0.,1.,0.,-1.};
static float square_y[] =
	{1.,0.,-1.,0.};

/* pieces are:	0 none
		1 pawn
		2 knight
		3 bishop
		4 rook
		5 queen
		6 king	*/

static int piece[8][8];		/* which piece is on a particular square */
static int color[8][8];		/* color of a piece on a particular square */
static int piece_size[7]={1,12,20,20,21,14,29}; /* number of lines per piece */
				/* piece outlines */
static float piece_x[7][30]={
	{0.},
	{0.2,-0.1,0.1,-0.15,0.05,-0.1,-0.1,0.05,-0.15,0.1,-0.1,0.2},
	{0.4,-0.1,-0.1,-0.2,-0.15,0.,0.05,-0.1,0.05,0.0,-0.1,0.05,
		-0.2,0.05,0.05,0.3,0.,-0.1,-0.1,0.2},
	{0.3,-0.2,0.1,-0.15,0.05,0.,-0.1,0.,0.1,-0.05,0.0,-0.1,
		0.,-0.05,0.,0.05,-0.15,0.1,-0.2,0.3},
	{0.3,-0.2,0.15,0.0,-0.05,0.0,-0.05,0.,-0.1,0.,-0.1,0.,-0.1,
		0.,-0.05,0.,-0.05,0.,0.15,-0.2,0.3},
	{0.4,-0.2,0.2,-0.2,0.05,-0.15,-0.1,-0.1,-0.15,0.05,-0.2,0.2,-0.2,0.4},
	{0.4,-0.2,0.1,0.1,0.0,-0.1,-0.1,-0.1,-0.05,0.,0.05,0.,-0.05,0.,-0.1,
		0.,-0.05,0.,0.05,0.,-0.05,-0.1,-0.1,-0.1,0.,0.1,0.1,-0.2,0.4} };
static float piece_y[7][30]={
	{0.},
	{0.,0.05,0.05,0.3,0.05,0.1,-0.1,-0.05,-0.3,-0.05,-0.05,0.}, 
	{0.,0.4,0.2,0.1,0.,-0.1,-0.1,0.,0.1,0.1,0.1,-0.1,-0.45,-0.05,
		0.,0.15,-0.05,-0.1,-0.2,0.},
	{0.,0.05,0.05,0.4,0.1,0.05,-0.1,0.05,0.1,0.05,0.05,0.,
		-0.05,-0.05,-0.1,-0.1,-0.4,-0.05,-0.05,0.},
	{0.,0.05,0.05,0.7,0.,-0.2,0.,0.2,0.,-0.2,0.,0.2,0.,-0.2,
		0.,0.2,0.,-0.7,-0.05,-0.05,0.},
	{0.,0.1,0.6,-0.2,0.25,-0.2,0.25,-0.25,0.2,-0.25,0.2,-0.6,-0.1,0.},
	{0.,0.05,0.05,0.2,0.3,0.1,0.,-0.1,0.,0.05,0.0,0.1,0.,0.05,0.,-0.05,
		0.,-0.1,0.,-0.05,0.,0.1,0.,-0.1,-0.3,-0.2,-0.05,-0.05,0.} };

static char *moves[55] = {
	"d2d4", "g8f6", "c2c4", "c7c5", "g1f3", "e7e6", "g2g3", "c5d4", "f3d4",
	"d8c7", "b1d2", "b8c6", "d4c2", "c6e5", "e2e4", "b7b5", "c4b5", "c8b7",
	"f2f3", "a8c8", "c2e3", "f8c5", "d2b3", "c5b4", "e1f2", "e5f3", "f1g2",
	"f6e4", "f2e2", "f3e5", "d1d4", "e4g3", "h2g3", "b7g2", "e3g2", "c7b7",
	"d4f2", "c8c2", "c1d2", "b4d2", "b3d2", "b7b5", "e2d1", "b5b2", "h1f1",
	"e5f3", "f2f3", "c2d2", "d1e1", "d2g2", "f3d1", "b2e5", "d1e2", "e5e2",
	"mate"};

static int fid,quick_flag;

main(argc,argv)
	int argc;
	char *argv[];
	{
	get_view_surface(our_surface,argv);
	go_setup_core();
	quick_flag=quick_test(argc,argv);
	create_temporary_segment();
	set_pieces();
	set_color();
	go_draw_board();
	place_pieces();
	play_game();
	close_temporary_segment();
	if(quick_flag)
		sleep(10);
	else
		sleep(1000000);
	deselect_view_surface(our_surface);
	terminate_core();
	}

go_setup_core()
	{
	if(initialize_core(DYNAMICC,SYNCHRONOUS,TWOD))
		exit(1);
	our_surface->cmapsize = 16;
	if(initialize_view_surface(our_surface,FALSE))
		exit(2);
	if(select_view_surface(our_surface))
		exit(3);
	set_window(-1.,12.,-1.,10.);
	}

go_draw_board()
	{
	set_linewidth(1.0);
	set_line_index(HEADING);
	move_abs_2(8.5,0.0);
	line_abs_2(8.5,9.3);
	set_charsize(0.2,0.275);
	set_charprecision(CHARACTER);
	set_text_index(HEADING);
	set_font(ROMAN);
	move_abs_2(0.,9.);
	text("WHITE: A. Rand      BLACK: M. Weiss");
	set_font(STICK);
	move_abs_2(9.25,9.);
	text("game score");
	}

set_pieces()
	{
	int i,j;
	piece[0][0]=4; piece[0][1]=2; piece[0][2]=3; piece[0][3]=5;
	piece[0][4]=6; piece[0][5]=3; piece[0][6]=2; piece[0][7]=4;

	for(j=0;j<8;j++) {
		piece[1][j]=1;
		piece[6][j]=1;
		for(i=2;i<6;i++)
			piece[i][j]=0;
		}

	piece[7][0]=4; piece[7][1]=2; piece[7][2]=3; piece[7][3]=5;
	piece[7][4]=6; piece[7][5]=3; piece[7][6]=2; piece[7][7]=4;
	}

set_color()
	{
	int i,j;

	red[MOVE_TEXT]=0.0; blu[MOVE_TEXT]=0.0; grn[MOVE_TEXT]=0.8;
	red[HEADING]=0.95; blu[HEADING]=0.95; grn[HEADING]=0.0;

	if((our_surface->dd==cg1dd) || (our_surface->dd==cgpixwindd) ||
		(our_surface->dd==gp1dd) || (our_surface->dd==gp1pixwindd)) {
	   red[LIGHT_PIECE]=0.9; blu[LIGHT_PIECE]=0.2; grn[LIGHT_PIECE]=0.2;
	   red[DARK_PIECE]=0.2; blu[DARK_PIECE]=0.9; grn[DARK_PIECE]=0.2;
	   red[LIGHT_SQUARE]=0.5; blu[LIGHT_SQUARE]=0.0; grn[LIGHT_SQUARE]=0.7;
	   red[DARK_SQUARE]=0.8; blu[DARK_SQUARE]=0.0; grn[DARK_SQUARE]=0.8;
	   }
	else {
	   red[LIGHT_PIECE]=0.8001;grn[LIGHT_PIECE]=0.2667;blu[LIGHT_PIECE]=0.4001;
	   red[DARK_PIECE]=0.0;grn[DARK_PIECE]=0.0;blu[DARK_PIECE]=0.0;
	   red[DARK_SQUARE]=0.797;grn[DARK_SQUARE]=0.078;blu[LIGHT_SQUARE]=0.039;
	   red[LIGHT_SQUARE]=0.723;blu[LIGHT_SQUARE]=0.020;grn[LIGHT_SQUARE]=.059;
	   }

	define_color_indices(our_surface,0,15,red,grn,blu);

	for (i=0;i<2;i++)
		for(j=0;j<8;j++)
			color[i][j]=LIGHT_PIECE;
	for (i=6;i<8;i++)
		for(j=0;j<8;j++)
			color[i][j]=DARK_PIECE;
	}

place_pieces()
	{
	int i,j;
	for (i=0;i<8;i++)
		for (j=0;j<8;j++) {
			place_one_piece(i,j);
			}
	}

place_one_piece(i,j)
	int i,j;
	{
	move_abs_2((float)j,(float)i);		/* draw square */
	if((i+j)%2)
		set_fill_index(DARK_SQUARE);
	else
		set_fill_index(LIGHT_SQUARE);
	polygon_rel_2(square_x,square_y,4);

	move_abs_2((float)j,(float)i);		/* outline square for effect */
	set_line_index(DARK_SQUARE);
	set_linewidth(0.1);
	polyline_rel_2(square_x,square_y,4);

	if(piece[i][j]) {
		set_fill_index(color[i][j]);	/* draw piece if there is one */
		move_abs_2(j+0.5,i+0.1);
		polygon_rel_2(piece_x[piece[i][j]],
		piece_y[piece[i][j]],piece_size[piece[i][j]]);

		set_linewidth(0.1);		/* outline piece */
		set_line_index(color[i][j]);
		move_abs_2(j+0.5,i+0.1);
		polyline_rel_2(piece_x[piece[i][j]],
			piece_y[piece[i][j]],piece_size[piece[i][j]]);
		}
	}

move_piece(row1,col1,row2,col2)
	int row1,col1,row2,col2;
	{
	piece[row2][col2]=piece[row1][col1];	/* move piece logically */
	color[row2][col2]=color[row1][col1];
	piece[row1][col1]=0;

	place_one_piece(row1,col1);		/* null from square */

	place_one_piece(row2,col2);		/* move piece physically */

	}

play_game()
	{
	int i,j,k,l,move_num;
	char move_num_str[4];
	for(move_num=0 ; i!=('m'-'a') ; move_num++) {

		if(!quick_flag)
			sleep(3);

		i=moves[move_num][0] - 'a'; /* display text of move */
		j=moves[move_num][1] - '1';
		k=moves[move_num][2] - 'a';
		l=moves[move_num][3] - '1';
		if (!(move_num%2)) {
			set_charsize(0.15,0.12);
			set_font(STICK);
			move_abs_2(9.0,8.0-(move_num/2)*0.25);

			move_num_str[0]=' ';
			if ((move_num/2+1) > 9)
				move_num_str[0]='0'+ (move_num/2+1)/10;
			move_num_str[1]='0'+ (move_num/2+1)%10;
			move_num_str[2]='.';
			move_num_str[3]='\0 ';
			set_text_index(HEADING);
			text(move_num_str);
			}
		move_abs_2(9.75+(move_num%2)*1.0,8.0-(move_num/2)*0.25);
		set_charsize(0.15,0.12);
		set_font(STICK);
		set_text_index(move_num%2+LIGHT_PIECE);
		text(moves[move_num]);
		if(i != ('m' - 'a') )
			move_piece(j,i,l,k);	/* actually move piece */
		}
	}

int quick_test(argc,argv) int argc; char *argv[];
	{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);
	}
