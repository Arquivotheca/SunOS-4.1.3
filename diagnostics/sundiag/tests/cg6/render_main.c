static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)render_main.c 1.1 7/30/92 Copyright 1988 Sun Micro";

 /* Rendering routine for wings by Curtis Priem, Oct 1988 */

#include "render_main.h"


extern int *fbc_base;
extern int *tec_base;


#define FRAMES	3
#define EXDEBUG	0

int		grid_s[65][65][2];
unsigned char	grid_c[64][64] = {
#include "create_colors.h"
};

extern
int sinehgt;

render_main(grid_z)
short	grid_z[64][64];
{
  int    itmp, itmp0, itmp1, itmp2, itmp3;
  float  ftmp, ftmp0, ftmp1, ftmp2, ftmp3;
  double dtmp, dtmp0, dtmp1, dtmp2, dtmp3;
  float  view_matrix[4][4];
  int best_view_x, best_view_y, best_view_z;
  int knt;
  int	 view_buffer;

  view_buffer = 0;

  /***** map in lego *****/
  if (map_lego()  < 0 ) {
	errorprint(-26, "render_main: can't map lego\n");
        exit(1);
  }

  /***** initialize FBC registers *****/
  write_lego_fbc(fbc_base + L_FBC_STATUS,      0x0);
  write_lego_fbc(fbc_base + L_FBC_RASTEROFFX,  0x00000000);
  write_lego_fbc(fbc_base + L_FBC_RASTEROFFY,  0x00000000);
  write_lego_fbc(fbc_base + L_FBC_AUTOINCX,    0x00000000);
  write_lego_fbc(fbc_base + L_FBC_AUTOINCY,    0x00000000);
  write_lego_fbc(fbc_base + L_FBC_CLIPCHECK,   0x00000000);
  write_lego_fbc(fbc_base + L_FBC_CLIPMINX,    Shmem->window_minx);
  write_lego_fbc(fbc_base + L_FBC_CLIPMINY,    Shmem->window_miny);
  write_lego_fbc(fbc_base + L_FBC_CLIPMAXX,    Shmem->window_maxx);
  write_lego_fbc(fbc_base + L_FBC_CLIPMAXY,    Shmem->window_maxy);
  write_lego_fbc(fbc_base + L_FBC_FCOLOR,      0x00000055);
  write_lego_fbc(fbc_base + L_FBC_BCOLOR,      0x000000aa);
  write_lego_fbc(fbc_base + L_FBC_RASTEROP,    0xe980cc00);
  write_lego_fbc(fbc_base + L_FBC_PLANEMASK,   0x000000ff);
  write_lego_fbc(fbc_base + L_FBC_PIXELMASK,   0xffffffff);
/*###
  write_lego_fbc(fbc_base + L_FBC_IPOINTABSX,  0x00000000);
  write_lego_fbc(fbc_base + L_FBC_IPOINTABSY,  0x00000000);
  write_lego_fbc(fbc_base + L_FBC_PATTALIGN,   0x00000000);
  write_lego_fbc(fbc_base + L_FBC_PATTERN0,    0x55555555);
  write_lego_fbc(fbc_base + L_FBC_PATTERN1,    0xaaaaaaaa);
  write_lego_fbc(fbc_base + L_FBC_PATTERN2,    0x55555555);
  write_lego_fbc(fbc_base + L_FBC_PATTERN3,    0xaaaaaaaa);
  write_lego_fbc(fbc_base + L_FBC_PATTERN4,    0x55555555);
  write_lego_fbc(fbc_base + L_FBC_PATTERN5,    0xaaaaaaaa);
  write_lego_fbc(fbc_base + L_FBC_PATTERN6,    0x55555555);
  write_lego_fbc(fbc_base + L_FBC_PATTERN7,    0xaaaaaaaa);
  write_lego_fbc(fbc_base + L_FBC_MISC,        0x0004aac0);
###*/

  /***** initialize TEC registers *****/
  write_lego_tec(tec_base + L_TEC_MV_MATRIX,   0x0);
  write_lego_tec(tec_base + L_TEC_CLIPCHECK,   0x0);
  write_lego_tec(tec_base + L_TEC_VDC_MATRIX,  0x0);


  /***** create tables *****/
  create_color_maps();



  sinehgt = 0;	/* will be changed in build_view_matrix */

  for ( knt = 0; knt < 32; knt++ ) {

    /***** dynamics *****/
    Shmem->pos_dx =  0.0;
    Shmem->pos_dy =  0.0;
    Shmem->pos_dz =  0.0;
    Shmem->pos_x  += Shmem->pos_dx;
    Shmem->pos_y  += Shmem->pos_dy;
    Shmem->pos_z  += Shmem->pos_dz;

    Shmem->rot_dx =  0x0180;
    Shmem->rot_dy =  0x0080;
    Shmem->rot_dz =  0x0030;
    Shmem->rot_x  += Shmem->rot_dx;
    Shmem->rot_y  += Shmem->rot_dy;
    Shmem->rot_z  += Shmem->rot_dz;



    /***** create viewing matrix *****/
    build_view_matrix();



    if (Shmem->ok_to_draw == FALSE)  {
  
      Shmem->drawing = FALSE;

      for (itmp = 0; itmp < FRAMES; itmp++)  {
      /***** wait for vblank low to high edge *****/
        while (read_lego_fbc(fbc_base+L_FBC_STATUS) & BUSY);
        write_lego_fbc(fbc_base + L_FBC_MISC,        VBLANK_OCCURED);

					/* wait for VBLANK=0 */
        while ((read_lego_fbc(fbc_base + L_FBC_MISC) & VBLANK_OCCURED)!=0 )
					/* clear VBLANK for LegoB */
            write_lego_fbc(fbc_base + L_FBC_MISC,        VBLANK_OCCURED);

					/* wait for VBLANK=1 */
        while ((read_lego_fbc(fbc_base + L_FBC_MISC) & VBLANK_OCCURED) == 0);
      }

    }  else  {

      Shmem->drawing = TRUE;

      /***** change color map and rendering buffer  *****/
      view_buffer = view_buffer ^ 1;
      update_color_map(view_buffer);

      /***** wait for vblank *****/
      while (read_lego_fbc(fbc_base+L_FBC_STATUS) & BUSY);
      write_lego_fbc(fbc_base + L_FBC_MISC,        VBLANK_OCCURED);

      while ((read_lego_fbc(fbc_base + L_FBC_MISC) & VBLANK_OCCURED) !=0)
        write_lego_fbc(fbc_base + L_FBC_MISC,        VBLANK_OCCURED);

      while ((read_lego_fbc(fbc_base + L_FBC_MISC) & VBLANK_OCCURED) == 0);

      while (read_lego_fbc(fbc_base+L_FBC_STATUS) & BUSY);
      if (view_buffer == 0)
        write_lego_fbc(fbc_base + L_FBC_PLANEMASK,   0x000000f0);
      else
        write_lego_fbc(fbc_base + L_FBC_PLANEMASK,   0x0000000f);



      /***** clear window *****/
      clear_window(fbc_base);



      /***** pick best view *****/
      write_lego_tec(tec_base+L_TEC_VDC_MATRIX,  0x00004000);
      best_view_x = read_lego_tec(tec_base+L_TEC_DATA24) ^ 0x80000000;
      best_view_y = read_lego_tec(tec_base+L_TEC_DATA25) ^ 0x80000000;
      best_view_z = read_lego_tec(tec_base+L_TEC_DATA26) ^ 0x80000000;
/*printf("best_view_x=%f	best_view_y=%f  best_view_z=%f\n", 
*(float*)&best_view_x, *(float*)&best_view_y, *(float*)&best_view_z);*/


      /***** rotate grid *****/
      write_lego_tec(tec_base+L_TEC_MV_MATRIX,   0x00000710);
      write_lego_tec(tec_base+L_TEC_CLIPCHECK,   0x00000000);
      write_lego_tec(tec_base+L_TEC_VDC_MATRIX,  0x0000100c);
      {
        register short *grid_z_ptr;
        register int   *tec_base_ptr, *grid_s_ptr;
        register int   x, y, reg_x, reg_y, reg_z;

        tec_base_ptr = tec_base;
        reg_x = 0;
        for (x=0; x<64; x++)  {
          grid_z_ptr   = &grid_z[x][0];
          grid_s_ptr   = &grid_s[x][0][0];
          reg_y = 0;
          reg_z = *grid_z_ptr++;
          write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSZ,  reg_z);
          write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSY,  reg_y);
          write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSX,  reg_x);
          for (y=64; --y>=0; )  {
            reg_y = reg_y + 90;
            reg_z = *grid_z_ptr++;
            *grid_s_ptr++ = read_lego_tec(tec_base_ptr+L_TEC_DATA08);
            *grid_s_ptr++ = read_lego_tec(tec_base_ptr+L_TEC_DATA09);
/*printf("x=%d	y=%d\n", *(grid_s_ptr-1), *(grid_s_ptr-2));*/
            write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSZ,  reg_z);
            write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSY,  reg_y);
            write_lego_tec(tec_base_ptr+L_TEC_IPOINTABSX,  reg_x);
          }
          *grid_s_ptr++ = read_lego_tec(tec_base_ptr+L_TEC_DATA08);
          *grid_s_ptr++ = read_lego_tec(tec_base_ptr+L_TEC_DATA09);
/*printf("x=%d	y=%d\n\n", *(grid_s_ptr-1), *(grid_s_ptr-2));*/
          reg_x = reg_x + 90;
        }
      }
      {
        register int clipcheck;
        clipcheck = read_lego_tec(tec_base+L_TEC_CLIPCHECK);
        if (clipcheck < 0)
	errorprint(-27, "render_main: TEC_EXCEPTION\n");
      }



      /***** draw grid *****/
      while (read_lego_fbc(fbc_base+L_FBC_STATUS) & BUSY);
      {
        register int  *fbc_base_ptr, *grid_s0_ptr, *grid_s1_ptr;
        register unsigned char *grid_c_ptr;
        register int  x, y, x_reg, y_reg, drawstatus;
        fbc_base_ptr = fbc_base;

        if (best_view_y < 0)  {
          for (x=0; x<63; x++)  {
            if (best_view_x < 0)  {
              grid_s0_ptr = &grid_s[x][0][X];
              grid_s1_ptr = &grid_s[x+1][0][X];
              grid_c_ptr  = &grid_c[x][0];
            }  else  {
              grid_s0_ptr = &grid_s[63-x][0][X];
              grid_s1_ptr = &grid_s[63-x-1][0][X];
              grid_c_ptr  = &grid_c[62-x][0];
            }
            x_reg = *grid_s0_ptr++;
            y_reg = *grid_s0_ptr++;
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
            x_reg = *grid_s1_ptr++;
            y_reg = *grid_s1_ptr++;
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), *(grid_c_ptr-1));*/
            for (y=31; --y>=0;)  {
              write_lego_fbc(fbc_base_ptr+L_FBC_FCOLOR,     *grid_c_ptr++);
              x_reg = *grid_s1_ptr++;
              y_reg = *grid_s1_ptr++;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
              x_reg = *grid_s0_ptr++;
              y_reg = *grid_s0_ptr++;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), *(grid_c_ptr-1));*/
              if ((drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS)) < 0)  {
                while (drawstatus & FULL)
                  drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS);
                if (drawstatus < 0)
                  if (EXDEBUG)  printf("004: DRAWSTATUS=0x%08x\n", drawstatus);
              }
              write_lego_fbc(fbc_base_ptr+L_FBC_FCOLOR,     *grid_c_ptr++);
              x_reg = *grid_s0_ptr++;
              y_reg = *grid_s0_ptr++;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
              x_reg = *grid_s1_ptr++;
              y_reg = *grid_s1_ptr++;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), *(grid_c_ptr-1));*/
              if ((drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS)) < 0)  {
                while (drawstatus & FULL)
                  drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS);
                if (drawstatus < 0)
                  if (EXDEBUG)  printf("005: DRAWSTATUS=0x%08x\n", drawstatus);
              }
            }
/*printf("\n");*/
          }

        }  else  {

          for (x=0; x<63; x++)  {
            if (best_view_x < 0)  {
              grid_s0_ptr = &grid_s[x][62][Y];
              grid_s1_ptr = &grid_s[x+1][62][Y];
              grid_c_ptr  = &grid_c[x][61];
            }  else  {
              grid_s0_ptr = &grid_s[63-x][62][Y];
              grid_s1_ptr = &grid_s[63-x-1][62][Y];
              grid_c_ptr  = &grid_c[62-x][61];
            }
            y_reg = *grid_s0_ptr--;
            x_reg = *grid_s0_ptr--;
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
            y_reg = *grid_s1_ptr--;
            x_reg = *grid_s1_ptr--;
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
            write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), *(grid_c_ptr-1));*/
            for (y=31; --y>=0;)  {
              write_lego_fbc(fbc_base_ptr+L_FBC_FCOLOR,     *grid_c_ptr--);
              y_reg = *grid_s1_ptr--;
              x_reg = *grid_s1_ptr--;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
              y_reg = *grid_s0_ptr--;
              x_reg = *grid_s0_ptr--;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), *(grid_c_ptr-1));*/
              if ((drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS)) < 0)  {
                while (drawstatus & FULL)
                  drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS);
                if (drawstatus < 0)
                  if (EXDEBUG)  printf("006: DRAWSTATUS=0x%08x\n", drawstatus);
              }
              write_lego_fbc(fbc_base_ptr+L_FBC_FCOLOR,     *grid_c_ptr--);
              y_reg = *grid_s0_ptr--;
              x_reg = *grid_s0_ptr--;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
              y_reg = *grid_s1_ptr--;
              x_reg = *grid_s1_ptr--;
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSY,  y_reg);
              write_lego_fbc(fbc_base_ptr+L_FBC_IQUADABSX,  x_reg);
/*printf("x=%d	y=%d	x=%d    y=%d	c=0x%02x\n", 
*(grid_s0_ptr-1), *(grid_s0_ptr-2), 
*(grid_s1_ptr-1), *(grid_s1_ptr-2), *(grid_c_ptr-1));*/
              if ((drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS)) < 0)  {
                while (drawstatus & FULL)
                  drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS);
                if (drawstatus < 0)
                  if (EXDEBUG)  printf("007: DRAWSTATUS=0x%08x\n", drawstatus);
              }
            }
/*printf("\n");*/
          }
        }

      }

    }

  }

}





clear_window(fbc_base)
int *fbc_base;
{
  register int *fbc_base_ptr;
  register int drawstatus;

  fbc_base_ptr = fbc_base;
  while (read_lego_fbc(fbc_base_ptr+L_FBC_STATUS) & BUSY);
  write_lego_fbc(fbc_base_ptr+L_FBC_FCOLOR,      (SKY<<4) | SKY);
  write_lego_fbc(fbc_base_ptr+L_FBC_IRECTABSY,   Shmem->window_miny);
  write_lego_fbc(fbc_base_ptr+L_FBC_IRECTABSX,   Shmem->window_minx);
  write_lego_fbc(fbc_base_ptr+L_FBC_IRECTABSY,   Shmem->window_maxy);
  write_lego_fbc(fbc_base_ptr+L_FBC_IRECTABSX,   Shmem->window_maxx);
  if ((drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS)) < 0)  {
    while (drawstatus & FULL)
      drawstatus=read_lego_fbc(fbc_base_ptr+L_FBC_DRAWSTATUS);
    if (drawstatus < 0)  errorprint(-28, "clear_window: DRAWSTATUS\n");
  }
}
