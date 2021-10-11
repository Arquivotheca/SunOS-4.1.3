/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)colorlut.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */


/*===============================================================
 *   This file contains some routines to load the color map and 
 *  initialize the color graphics board.
 *  PWC 2/12/82.      
 *
 *	init_color_fbuf()
 *	_core_write_cmap(color1,color2,color3,cplane)
 *		uchar *color1,*color2,*color3;
 *		short cplane;
 *	_core_read_cmap(color1,color2,color3,cplane)
 *		uchar *color1,*color2,*color3;
 *		short cplane;
 *  Global Variable Definitions. The following globals hold the 
 *  data currently in the status, color, and function registers.
 *  One variable defines which color plane (0-3) we are currently
 *  loading. Another variable shows if we are 1/3 or 2/3 of the
 *  way through with writing of the new color map plane.
 *=============================================================== */
   
/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine goes into paint mode and writes a block of the 
      color frame buffer as quickly as possible. The pixels specified
      will be written only on fixed 5 pixel boundaries. 
   Data: The user is responsible for making sure that the values supplied
      lie in the frame buffer. DX5 and DY must be non-negative.
   ====================================================================== */
#include <colorbuf.h>

static set_fbuf_5x(x,y,dx5,dy,color) short x,y,dx5,dy,color;
{
   uchar register *xaddr, *yaddr, ch_color;
   short register i,j;

   ch_color = (uchar) color;
   *GR_sreg |= GR_paint;
   yaddr = (uchar *) (GR_bd_sel + GR_y_select + y);
  
   for (j=dy;j>0;j--) {
      *yaddr++ = ch_color;    /* Set row */
      xaddr = (uchar *) (GR_bd_sel + GR_x_select + GR_update + x);

      for (i= dx5;i>0;i--) {  
         *xaddr = ch_color;
         xaddr += 5;
      }
   }

   *GR_sreg &= ~GR_paint;   /* Turn off? paint mode */

}    /* End of procedure set_fbuf */



static short gr_curr_color;   /* The third of the color map we are writing. 
                          1 = reds. 2 = greens. 3 = blues. */
static uchar *gr1_color,*gr2_color,*gr3_color;

/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine takes pointers to three memory arrays with which
      we will load into the color map using the color plane specified.
   ====================================================================== */

_core_write_cmap(color1,color2,color3,new_cplane)
   uchar *color1,*color2,*color3;
   short new_cplane;
{
   short j,i;				/* Temp var */
   uchar *color,tcolor;

   gr1_color = color1;
   gr2_color = color2;
   gr3_color = color3;
   Set_RW_Cmap(new_cplane);
   gr_curr_color = 1;    /* Start by writing reds. */
   wrcmap();
} 

/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose:
      Write_cmap. It loads the color map with the values given, clears
      the interrupt, and sets up the new colorplane in use. 
   ====================================================================== */

static wrcmap()
{
   uchar register *color,*color_map;
   short register i;
   uchar register worked;

   do {
      while( !GR_retrace);
      if (gr_curr_color==1) {
         color = (uchar *)gr1_color;
         color_map = (uchar *) (GR_bd_sel + GR_red_cmap);
      } else if (gr_curr_color==2) {
         color = (uchar *)gr2_color;
         color_map = (uchar *) (GR_bd_sel + GR_grn_cmap);
      } else {
         color = (uchar *)gr3_color;
         color_map = (uchar *) (GR_bd_sel + GR_blu_cmap);
      } 
   
      /* Do 8 blocks of 32 moves to the color_map */

      for (i=8;i>0;i--) {
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
   
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;

         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
   
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
         *color_map++ = *color++; *color_map++ = *color++;
      }
      worked = GR_retrace;	       /* True if in Vertical retrace */
      if (worked) gr_curr_color += 1;  /* Go on to next color in color map */

   } while (gr_curr_color <= 3);

}

/* ======================================================================
   Author: Peter Costello
   Date :  April 25, 1982
   Purpose: This routine takes pointers to three memory arrays with which
      we will load from the color map using the color plane specified.
   ====================================================================== */

_core_read_cmap(color1,color2,color3,cplane)
   uchar *color1,*color2,*color3;  short cplane;
{
   gr1_color = (uchar *) color1;
   gr2_color = (uchar *) color2;
   gr3_color = (uchar *) color3;

   *GR_sreg = 0;
   Set_RW_Cmap(cplane);
   
   gr_curr_color = 1;    /* Start by writing reds. */
   rdcmap();
}

/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
      Read_cmap. It loads the arrays from the color plane specified.
   ====================================================================== */
static rdcmap()
{
   uchar register *color, *color_map;
   short register i;
   uchar register worked;

   do {
      while( !GR_retrace);
      if (gr_curr_color==1) {
         color = (uchar *)gr1_color;
         color_map = (uchar *) (GR_bd_sel + GR_red_cmap);
      } else if (gr_curr_color==2) {
         color = (uchar *)gr2_color;
         color_map = (uchar *) (GR_bd_sel + GR_grn_cmap);
      } else {
         color = (uchar *)gr3_color;
         color_map = (uchar *) (GR_bd_sel + GR_blu_cmap);
      } 
   
      /* Do 8 blocks of 32 moves from the color_map */

      for (i=8;i>0;i--) {
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
   
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;

         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;

         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
         *color++ = *color_map++; *color++ = *color_map++;
      }
      worked = GR_retrace;	       /* True if in Vertical retrace */
      if (worked) gr_curr_color += 1;  /* Go on to next color in color map */
   } while (gr_curr_color <= 3);
}


/* ======================================================================
   Author : Peter Costello
   Date   : April 21, 1982
   Purpose : This routine initializes the color board. It clears the 
       frame buffer to color 0. It initializes the color_register to 
       zero. Sets the function register to load data from the multibus,
       and initializes the status register to color_plane zero, display
       on, and all other bits off. 
   ====================================================================== */
static init_color_fbuf()
{
   int set_fbuf_5x();  /* Routine does a fast clear of the frame buffer */

    *GR_sreg =  0;	/* First disable DAC outputs */
    *GR_freg = GR_copy;	/* Set function register to write through */
    *GR_creg = 0;	/* Clear color register */
					/* Clear the frame buffer */
    set_fbuf_5x(0,0,128,512,0);		/* Set to color 0 */
    *GR_sreg = GR_disp_on;		/* set sreg to real data */

}


