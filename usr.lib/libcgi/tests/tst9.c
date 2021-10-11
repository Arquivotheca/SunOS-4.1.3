#ifndef lint
static char sccsid[] = "@(#)tst9.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include "cgidefs.h"
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;

main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   int surface, linetype_int;
   Cint radius;
   Cfloat linewidth;
   Clintype linetype;
   Ccoor c1,c2,c3;
   Cint name;
   double pi = 3.14159265358, start, end;
    
   if (argc < 7) {
      printf (" ... format: surface radius start end linetype linewidth\n");
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

   radius = atoi(argv[2]);

   start = atof(argv[3]) * pi;
   end = atof(argv[4]) * pi;
   linetype_int = atoi(argv[5]);
   if (linetype_int == 0)
      linetype = SOLID;
   else if (linetype_int == 1)
      linetype = DASHED;
   else if (linetype_int == 2)
      linetype = DOTTED;
   else if (linetype_int == 3)
      linetype = DASHED_DOTTED;
   else if (linetype_int == 4)
      linetype = DASH_DOT_DOTTED;

   linewidth = atof(argv[6]);

   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1);
/*   if ( (surface == CG1DD) || (surface == CGPIXWINDD) )
      clear_view_surface(name,ON,147); */

   line_type(linetype);
   line_width(linewidth);
   line_color(25);
   c1.x = 8192;
   c1.y = 8192;
   c2.x = c1.x + radius*cos(start);
   c2.y = c1.y + radius*sin(start);
   c3.x = c1.x + radius*cos(end);
   c3.y = c1.y + radius*sin(end);
   circular_arc_center(&c1,c2.x,c2.y,c3.x,c3.y,radius);

   line_type(linetype);
   line_width(linewidth);
   line_color(195);
   c1.x = 8192;
   c1.y = 24576;
   c2.x = c1.x + radius*cos(start);
   c2.y = c1.y + radius*sin(start);
   c3.x = c1.x + radius*cos(end);
   c3.y = c1.y + radius*sin(end);
   circular_arc_center(&c1,c3.x,c3.y,c2.x,c2.y,radius);

   line_type(linetype);
   line_width(linewidth);
   line_color(25);
   c1.x = 24576;
   c1.y = 8192;
   c2.x = c1.x + radius*cos(start);
   c2.y = c1.y + radius*sin(start);
   c3.x = c1.x + radius*cos(end);
   c3.y = c1.y + radius*sin(end);
   circular_arc_center(&c1,c2.x,c2.y,c3.x,c3.y,radius);
   line_color(195);
/*    c1.x = 24576; */
/*    c1.y = 8192; */
   circular_arc_center(&c1,c3.x,c3.y,c2.x,c2.y,radius);

   line_type(linetype);
   line_width(linewidth);
   line_color(25);
    c1.x = 24576; 
    c1.y = 24576; 
   c2.x = c1.x + radius*cos(start);
   c2.y = c1.y + radius*sin(start);
   c3.x = c1.x + radius*cos(end);
   c3.y = c1.y + radius*sin(end);
   circular_arc_center(&c1,c2.x,c2.y,c3.x,c3.y,radius);
    line_color(195); 
/*    c1.x = 24576; */
/*    c1.y = 24576; */
   circular_arc_center(&c1,c3.x,c3.y,c2.x,c2.y,radius);

   sleep(100000);
   close_cgi();
   }
