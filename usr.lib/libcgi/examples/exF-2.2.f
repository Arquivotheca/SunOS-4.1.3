c
c	@(#)exF-2.2.f 1.1 92/07/30 Copyr 1985-9 Sun Micro
c
c Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
c Permission to use, copy, modify, and distribute this software for any
c purpose and without fee is hereby granted, provided that the above
c copyright notice appear in all copies and that both that copyright
c notice and this permission notice are retained, and that the name
c of Sun Microsystems, Inc., not be used in advertising or publicity
c pertaining to this software without specific, written prior permission.
c Sun Microsystems, Inc., makes no representations about the suitability
c of this software or the interface defined in this software for any
c purpose. It is provided "as is" without express or implied warranty.
c
c SunCGI REFERENCE MANUAL, Rev. A, 9 May 1988, PN 800-1786-10 -- SunOS 4.0
c Example F-2.1, page 156
c
      program f77colors

      include 'f77/cgidefs77.h'

      integer         name
      integer         x(2),y(2)
      integer         ncolors
      integer         red(8),grn(8),blu(8)
      ncolors = 8
C
C   Open CGI
C
      call cfopencgi()
C
C   Open a view surface
C
      call cfopenvws(name,0,0,0,1,CGPIXWINDD,ncolors,'Color',0,0)
C
C   Set color map
C
      call setupcolors( red, grn, blu)
C
C   Assign the color map
C
      call cfcotable(0,red,grn,blu,ncolors)
C
C  Draw the lines
C
      y(1) = 0
      y(2) = 30000
      do 11 i = 1, ncolors
            call cflncolor(i)
            x(1) = i * 2000
            x(2) = i * 2000
            call cfpolyline(x,y,2)
   11 continue
C
C   Wait 10 seconds
C
      call sleep(10)
C
C   Close CGI
C
      call cfclosevws(name)
      call cfclosecgi()
      end

C
C   Subroutine to set up the colormap
C
      subroutine setupcolors( red, grn, blu)
      integer red(8)
      integer grn(8)
      integer blu(8)

C   Set up colormap similar to the default colormap 
C   on page 76 except the first color is white, and
C   the last color is black.  (This makes the background
C   white, and the cursor black, too.)
C
      red(1) = 255
      grn(1) = 255
      blu(1) = 255
    
      red(2) = 255
      grn(2) = 0
      blu(2) = 0
   
      red(3) = 255
      grn(3) = 255
      blu(3) = 0
   
      red(4) = 0
      grn(4) = 255
      blu(4) = 0
   
      red(5) = 0
      grn(5) = 128
      blu(5) = 128
   
      red(6) = 0
      grn(6) = 0
      blu(6) = 255
   
      red(7) = 128
      grn(7) = 0
      blu(7) = 128
   
      red(8) = 0
      grn(8) = 0
      blu(8) = 0
   
      return
      end
