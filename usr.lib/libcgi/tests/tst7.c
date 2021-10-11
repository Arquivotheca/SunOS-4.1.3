#ifndef lint
static char sccsid[] = "@(#)tst7.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#include "cgidefs.h"
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;

main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   Cint font, color, path_int, halign_int, valign_int, surface, i;
   Cfloat efac, spcratio, xup, yup, xbase, ybase, hcalind, vcalind;
   Cint height;
   double pi = 3.14159265358;
   Cpathtype path;
   Chaligntype halign;
   Cvaligntype valign;
   Ccoor text_point;
   int fix,name;
   Ctextfinal flagf;
   
   if (argc < 13) {
      printf (" ... format: funcname surface font color efac spcratio hgt path halign valign hcalind vcalind fixed_width\n");
      exit(1);
      }

   surface_string = argv[1];
   if (!strcmp(surface_string,"cg1dd"))
      surface = CG1DD;
   else if (!strcmp(surface_string,"bw1dd"))
      surface = BW1DD;
   else if (!strcmp(surface_string,"bw2dd"))
      surface = BW2DD;
   else if (!strcmp(surface_string,"pixwindd"))
      surface = PIXWINDD;
   else if (!strcmp(surface_string,"cgpixwindd"))
      surface = CGPIXWINDD;
   else {
      printf (" ... invalid surface name ... exitting\n");
      exit(1);
   }

   font = atoi(argv[2]);
   color = atoi(argv[3]);
   efac = atof(argv[4]);
   spcratio = atof(argv[5]);
   height = atoi(argv[6]);

   path_int = atoi(argv[7]);
   if (path_int == 0)
      path = RIGHT;
   else if (path_int == 1)
      path = LEFT;
   else if (path_int == 2)
      path = UP;
   else if (path_int == 3)
      path = DOWN;

   halign_int = atoi(argv[8]);
   if (halign_int == 0)
      halign = LFT;
   else if (halign_int == 1)
      halign = CNTER;
   else if (halign_int == 2)
      halign = RGHT;
   else if (halign_int == 3)
      halign = NRMAL;
   else if (halign_int == 4)
      halign = CNT;
  
   valign_int = atoi(argv[9]);
   if (valign_int == 0)
      valign = TOP;
   else if (valign_int == 1)
      valign = CAP;
   else if (valign_int == 2)
      valign = HALF;
   else if (valign_int == 3)
      valign = BASE;
   else if (valign_int == 4)
      valign = BOTTOM;
   else if (valign_int == 5)
      valign = NORMAL;
   else if (valign_int == 6)
      valign = CONT;

   hcalind = atof(argv[10]);
   vcalind = atof(argv[11]);
   fix =  atoi(argv[12]);
   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1);
   if ( (surface == CG1DD) || (surface == CGPIXWINDD) )
      clear_view_surface(100);
   clip_indicator(NOCLIP);

   text_precision(CHARACTER);
   fixed_font(fix);
   character_set_index(1);
   text_font_index(font);
   character_expansion_factor(efac);
   text_color(color);
   character_height(height);
   character_spacing(spcratio);
   character_path(path);
/*
   text_alignment(halign,valign,hcalind,vcalind);
*/

   for (i=0; i<16; i++) {
      xup = cos((double)(i/8.*pi));
      yup = sin((double)(i/8.*pi));
      text_point.x = 16000 + 1000 * xup;
      text_point.y = 16000 + 1000 * yup;
      text_point.x = 16000;
      text_point.y = 16000;
      character_orientation(-yup,xup,xup,yup);
      text(&text_point,"ABCDabcd");
      append_text(flagf,"0123");
   }
   sleep(10000);
   close_cgi();
}
