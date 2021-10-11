#ifndef lint
static char sccsid[] = "@(#)tst2.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include <cgidefs.h>
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;
Ccoor point1, point2;
Ccoorlist pline;
Ccoor plarr[2]; 

main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   int surface,name;
   int i, linetype_int, linecolor;
   Clintype linetype;
   float linewidth;
   double pi = 3.14159265358;
   Ccoor cpts[66];
   if (argc < 5) {
      printf (" ... format: funcname surface linetype linecolor linewidth\n");
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
   linetype_int = atoi(argv[2]);
   linecolor = atoi(argv[3]);
   linewidth = atof(argv[4]);
   for (i=0;i<64;i += 2) {
      cpts[i].x = (int)( 16384 + 14000 * cos(i/32. * pi) );
      cpts[i].y = (int)( 16384 + 14000 * sin(i/32. * pi) );
      cpts[i+1].x = 16384;
      cpts[i+1].y = 16384;
   }

   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1); 
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
/*   for (i=0;i<32;i++) {
      plarr[0].x = 16384;
      plarr[0].y = 16384;
      plarr[1].x = (int)( 16384 + 16000 * cos(i/16. * pi) );
      plarr[1].y = (int)( 16384 + 16000 * sin(i/16. * pi) ); */
      pline.ptlist=cpts;
      pline.n=64;
      disjoint_polyline(&pline);
/*        }  */
   sleep(100000);
   close_cgi();
   }
