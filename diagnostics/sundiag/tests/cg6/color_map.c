static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)color_map.c 1.1 7/30/92 Copyright 1988 Sun Micro";

/* Rendering routine for wings by Curtis Priem, Oct 1988 */

#include "render_main.h"

#define R       0
#define G       1
#define B       2
char   lut[2][3][256];





create_color_maps()
{
  int  itmp;
  unsigned char ctmp;

  for (itmp=0; itmp<WATER; itmp++)  {		/* dark green to light green */
    ctmp = (itmp * 127.0 / (WATER-1));
    color_map_element(itmp, 0, ctmp+128, ctmp+128);
  }
  color_map_element(WATER,   0,   0, 192);	/* dark blue */
  color_map_element(SKY,     0,   0,   0);	/* light blue */

  lut[0][R][254]=255; lut[0][G][254]=255; lut[0][B][254]=255; /* background */
  lut[0][R][255]=0;   lut[0][G][255]=0;   lut[0][B][255]=0;   /* foreground */
  lut[1][R][254]=255; lut[1][G][254]=255; lut[1][B][254]=255; /* background */
  lut[1][R][255]=0;   lut[1][G][255]=0;   lut[1][B][255]=0;   /* foreground */

}





color_map_element(index, red, green, blue)
char red, green, blue;
int  index;
{
  register int  index0, itmp;

  for (itmp=16; --itmp>=0; )  {

    index0 = (itmp<<4) + index;
    lut[0][R][index0] = red;
    lut[0][G][index0] = green;
    lut[0][B][index0] = blue;

    index0 = (index<<4) + itmp;
    lut[1][R][index0] = red;
    lut[1][G][index0] = green;
    lut[1][B][index0] = blue;
  }
}



extern Pixrect *prfd;

#include <pixrect/pixrect_hs.h>

update_color_map(view_buffer)
int view_buffer;
{
  register char  *lut_ptr;
  register int   *dac_base_ptr;
  register int   color;
  register short count;
  char *red1, *grn1, *blu1;

  count = 255;
  lut_ptr = &lut[view_buffer][0][0];

	red1 = lut[view_buffer][R];
	grn1 = lut[view_buffer][G];
	blu1 = lut[view_buffer][B];
	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
/*###
  dac_base_ptr = dac_base;
  *(dac_base_ptr + 0x0) = 0x00000000;
  do  {
    color = *lut_ptr++ & 0x000000ff;
    color = (color<<24) | (color<<00);
    *(dac_base_ptr + 0x1) = color;

    color = *lut_ptr++ & 0x000000ff;
    color = (color<<24) | (color<<00);
    *(dac_base_ptr + 0x1) = color;

    color = *lut_ptr++ & 0x000000ff;
    color = (color<<24) | (color<<00);
    *(dac_base_ptr + 0x1) = color;
  }  while (--count != -1);
###*/

}
