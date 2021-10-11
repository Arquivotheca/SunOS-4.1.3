#ifndef lint
static char sccsid[] = "@(#)tst8.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include "cgidefs.h"
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;

static int pattern0[4]={50,0,0,75};
static int pattern1[9]={50,0,0,0,75,0,100,0,0};
static int pattern2[16]={0,50,0,0,75,100,125,0,0,150,0,0,0,0,0,0};
static int pattern3[16]={50,75,100,125,150,0,0,175,200,0,0,225,250,275,300,325};
static int pattern4[16]={50,75,100,0,125,0,150,0,175,200,225,0,0,0,0,0};
static int pattern5[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int pattern6[16]={50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50};

main(argc, argv) int argc; char *argv[];
   {
   char *surface_string;
   int dx,dy;
   int radius, index, perimtype_int, perimvis_int, surface;
   Cfloat perimwidth;
   Cflag perimvis;
   Clintype perimtype;
   Ccoor circlepoint;
   Ccoor patrefpoint;
   Cint name;
    
   if (argc < 11) {
      printf (" ... format: surface radius perimvis perimtype perimwidth index patptx patpty dx dy\n");
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

   perimvis_int = atoi(argv[3]);
   if (perimvis_int == 0)
      perimvis = OFF;
   else if (perimvis_int == 1)
      perimvis = ON;

   perimtype_int = atoi(argv[4]);
   if (perimtype_int == 0)
      perimtype = SOLID;
   else if (perimtype_int == 1)
      perimtype = DASHED;
   else if (perimtype_int == 2)
      perimtype = DOTTED;
   else if (perimtype_int == 3)
      perimtype = DASHED_DOTTED;
   else if (perimtype_int == 4)
      perimtype = DASH_DOT_DOTTED;

   perimwidth = atof(argv[5]);
   index = atoi(argv[6]);
   patrefpoint.x = (int)(atoi(argv[7]));
   patrefpoint.y = (int)(atoi(argv[8]));
   dx = (int)atoi(argv[9]);
   dy = (int)atoi(argv[10]);


   open_cgi();
   device1.dd=surface;
   open_vws(&name,&device1);
   /* initialize the pattern table */
   pattern_table(0,2,2,pattern0);
   pattern_table(1,3,3,pattern1);
   pattern_table(2,4,4,pattern2);
   pattern_table(3,4,4,pattern3);
   pattern_table(4,4,4,pattern4);
   pattern_table(5,4,4,pattern5);
   pattern_table(6,4,4,pattern6);

   interior_style(HOLLOW,perimvis);
   fill_color(100);
   perimeter_type(perimtype);
   perimeter_width(perimwidth);
   perimeter_color(25);
   circlepoint.x = 8192;
   circlepoint.y = 8192;
   circle(&circlepoint, radius);

   interior_style(SOLIDI,perimvis);
   fill_color(100);
   perimeter_type(perimtype);
   perimeter_width(perimwidth);
   perimeter_color(25);
   circlepoint.x =  8192;
   circlepoint.y = 24576;
   circle(&circlepoint, radius);

   interior_style(HATCH,perimvis);
   hatch_index(index);
   fill_color(100);
   perimeter_type(perimtype);
   perimeter_width(perimwidth);
   perimeter_color(25);
   circlepoint.x = 24576;
   circlepoint.y =  8192;
   circle(&circlepoint, radius);

   interior_style(PATTERN,perimvis);
   pattern_index(index);
   pattern_size(dx,dy);
   pattern_reference_point(&patrefpoint);
   fill_color(100);
   perimeter_type(perimtype);
   perimeter_width(perimwidth);
   perimeter_color(25);
   circlepoint.x = 24576;
   circlepoint.y = 24576;
   circle(&circlepoint, radius);

   sleep(100000);
   close_cgi();
   }
