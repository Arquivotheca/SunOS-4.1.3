#ifndef lint
static char sccsid[] = "@(#)tst6.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

#include <cgidefs.h>
#include <math.h>

Cvwsurf device1;
Cvwsurf device2;
Ccoor point1, point2;
Ccoorlist pline;
Ccoor plarr[20];

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
   int i,j;
   int dx,dy;
   int surface;
   int perimcolor,fillcolor,index;
   int perimvis_int, intstyle_int, perimtype_int;
   Cint name;
   Cfloat perimwidth;
   Clintype perimtype;
   Cflag perimvis;
   Cintertype intstyle;
   if (argc < 11) {
      printf (" ... format: funcname surface intstyle fillcolor perimvis perimtype perimwidth perimcolor patindex dx dy (and up to 20 coord pairs)\n");
      exit(1);
      }

   surface_string = argv[1];
   if (!strcmp(surface_string,"cg1dd"))
      surface = CG1DD;
   if (!strcmp(surface_string,"cg2dd"))
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

   intstyle_int = atoi(argv[2]);
   fillcolor = atoi(argv[3]);
   perimvis_int = atoi(argv[4]);
   perimtype_int = atoi(argv[5]);
   perimwidth = atof(argv[6]);
   perimcolor = atoi(argv[7]);
   index = atoi(argv[8]);
   dx = atoi(argv[9]);
   dy = atoi(argv[10]);

   j=0;
   for (i=11; i< argc;) {
      plarr[j].x = atoi(argv[i++]);
      plarr[j].y = atoi(argv[i++]);
      j++;
   }

   if (intstyle_int == 0)
      intstyle = HOLLOW;
   if (intstyle_int == 1)
      intstyle = SOLIDI;
   if (intstyle_int == 2)
      intstyle = PATTERN;
   if (intstyle_int == 3)
      intstyle = HATCH;

   if (perimvis_int == 0)
      perimvis = OFF;
   if (perimvis_int == 1)
      perimvis = ON;

   if (perimtype_int == 0)
      perimtype = SOLID;
   if (perimtype_int == 1)
      perimtype = DASHED;
   if (perimtype_int == 2)
      perimtype = DOTTED;
   if (perimtype_int == 3)
      perimtype = DASHED_DOTTED;
   if (perimtype_int == 4)
      perimtype = DASH_DOT_DOTTED;

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
   pattern_index(index);
   pattern_size(dx,dy);
   pattern_reference_point(0,0);
   hatch_index(index);

   pline.ptlist=plarr;
   pline.n=j;
   fill_color(fillcolor);
   interior_style(intstyle,perimvis);
   perimeter_type(perimtype);
   perimeter_width(perimwidth);
   perimeter_color(perimcolor);
   polygon(&pline);
   sleep(100000);
   close_cgi();
   }
