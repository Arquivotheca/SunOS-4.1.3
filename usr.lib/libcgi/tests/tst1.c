#ifndef lint
static char sccsid[] = "@(#)tst1.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include "cgidefs.h"
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;
Ccoor point1, point2;
Ccoorlist pline;
Ccoor plarr[100]; 
Cint name;

main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   int surface;
   int i,numpts,linetype_int,linecolor;
   float linewidth;
   Clintype linetype;
   if (argc < 6) {
      printf (" ... format: funcname surface numpts linetype linecolor linewidth\n");
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

   numpts = atoi(argv[2]);
   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1);
   srand(getpid());
   pline.ptlist=plarr;
   pline.n=numpts;
   for (i=0;i<numpts;i++) {
      plarr[i].x = rand(1)%32768;
      plarr[i].y = rand(1)%32768;
      }
   linetype_int = atoi(argv[3]);
   linecolor = atoi(argv[4]);
   linewidth = atof(argv[5]);
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
   line_type(linetype);
   line_color(linecolor);
   line_width(linewidth);
   polyline(&pline);
   sleep(100000);
   close_cgi();
   }
