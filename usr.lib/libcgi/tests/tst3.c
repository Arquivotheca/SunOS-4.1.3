#ifndef lint
static char sccsid[] = "@(#)tst3.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include "cgidefs.h"
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;
Ccoor point1, point2;
Ccoorlist pline;
Ccoor plarr[2]; 
int* redraw();
int name;
main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   int surface;
   int i, linetype_int, linecolor;
   Cmartype linetype;
   int specmode;
   float linewidth;
   Ccoor cpts[33];
   Cmarkatt att;
   double pi = 3.14159265358;
   if (argc < 6) {
      printf (" ... format: funcname surface markertype markercolor markerwidth specmode\n");
      exit(1);
      }

   surface_string = argv[1];
   if (!strcmp(surface_string,"cg1dd"))
      surface = CG1DD;
   else if (!strcmp(surface_string,"cg2dd"))
      surface = CG2DD;
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
   for (i=0;i<32;i++) {
      cpts[i].x = (int)( 16384 + 14000 * cos(i/16. * pi) );
      cpts[i].y = (int)( 16384 + 14000 * sin(i/16. * pi) );
   }
      cpts[32].x = 16384;
      cpts[32].y = 16384;
   linetype_int = atoi(argv[2]);
   linecolor = atoi(argv[3]);
   linewidth = atof(argv[4]);
   specmode = atof(argv[5]);
   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1); 
   set_up_sigwinch(redraw,name);

      linetype = (Cmartype) linetype_int;

   marker_type((Cmartype) linetype);
   marker_color(linecolor);
  if(specmode)  marker_size_specification_mode(ABSOLUTE);
   marker_size(linewidth);
      pline.ptlist=cpts;
      pline.n=33;
      polymarker(&pline);
	att = *inquire_marker_attributes();
	printf(" marker type %d \n",att.type);
	printf(" marker size %f \n",att.size);
	printf(" marker color %d \n",att.color);
   sleep( (unsigned)100000 );
   close_cgi();
   }

int* redraw()
{
    clear_view_surface(name,ON,0);
          polymarker(&pline);
}
