#ifndef lint
static char sccsid[] = "@(#)tst4.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include <cgidefs.h>
#include <math.h>

int pixwindd();
Cvwsurf device;
Ccoor point1, point2;
Ccoorlist pline;
Ccoor plarr[2]; 

main(argc, argv) int argc; char *argv[];
   {
   int i, linetype_int, linecolor;
   Clintype linetype;
   Cint name;
   Cfloat linewidth;
   if (argc < 4) {
      printf (" ... format: funcname linetype linecolor linewidth x0 y0 x1 y1\n");
      exit(1);
      }
   linetype_int = atoi(argv[1]);
   linecolor = atoi(argv[2]);
   linewidth = atof(argv[3]);
   device.dd= PIXWINDD;
   open_cgi();
   open_vws(&name,&device);
   srand(getpid());

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
   plarr[0].x = atoi(argv[4]);
   plarr[0].y = atoi(argv[5]);
   plarr[1].x = atoi(argv[6]);
   plarr[1].y = atoi(argv[7]);
   printf("line from (%d,%d) to (%d,%d)\n",plarr[0].x,plarr[0].y,
           plarr[1].x,plarr[1].y);
   pline.ptlist=plarr;
   pline.n=2;
   polyline(&pline);
   sleep(100000);
   close_cgi(&device);
   }
