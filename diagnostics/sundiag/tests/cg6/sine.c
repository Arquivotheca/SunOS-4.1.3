static char version[] = "%M";
static  char sccsid[] = "@(#)sine.c 1.1 92/07/30 SMI";

 /* Rendering routine for wings by Curtis Priem, Oct 1988 */

#include "render_main.h"

int *fbc_base;
int *tec_base;
extern height, width;

struct shared_buffer *Shmem, Shmem_declare;

short	grid_z[64][64] = {
#include "create_grid.h"
};


run_sine()
{
  int itmp0, itmp1;
  double dtmp, dtmp0, dtmp1, dtmp2;


  /***** initialize variables *****/
  Shmem = &Shmem_declare;
  Shmem->ok_to_draw  	= TRUE;
  Shmem->window_minx	=   50;
  Shmem->window_miny	=   50;
  Shmem->window_maxx	= width -50;
  Shmem->window_maxy	=  height -50;
  Shmem->mouse_rel_x	=  525;
  Shmem->mouse_rel_y	=  400;

  Shmem->pos_x =  2880.0;
  Shmem->pos_y =  2880.0;
  Shmem->pos_z =  2000.0;

  Shmem->rot_x =  0x0000;
  Shmem->rot_y =  0x0000;
  Shmem->rot_z =  0x0000;


  render_main(grid_z);
}

extern Pixrect *prfd;

map_lego()
{

	fbc_base = (int *) cg6_d(prfd)->cg6_fbc;
	tec_base = (int *) cg6_d(prfd)->cg6_tec;

	return 0;

}
