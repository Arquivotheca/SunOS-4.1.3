c
c	@(#)exF-2.1.f 1.1 92/07/30 Copyr 1985-9 Sun Micro
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
	program f77mglass

        include 'f77/cgidefs77.h'
C
        parameter (ibignum=256)
C
        integer name
        character screenname*(ibignum)
        character windowname*(ibignum)
        integer windowfd
        integer retained
        integer dd
        integer cmapsize
        character cmapname*(ibignum)
        integer flags
        character ptr*(ibignum)
        integer noargs
C
        integer xc(10), yc(10), n
        integer xc2(2), yc2(2)
        data xc /0, -10, -1, -1, -15, 15, 1, 1, 10, 0/
        data yc /0,   0,  1, 20,  35, 35, 20, 1, 0, 0/
        data xc2 /-12, 12/
        data yc2 /33,  33/
C
C       Initialize CGI and view surface
C
        call cfopencgi()
        dd = PIXWINDD
        call cfopenvws(name, screenname,  windowname, 
     .          windowfd, retained, dd, cmapsize,
     .          cmapname,  flags, ptr, noargs)
        call cfvdcext( -50, -10, 50, 80)
C       
C       Set clipping off
C
        call cfclipind(0)
C
C       Draw the martini glass
C
        n = 10
        call cfpolyline( xc, yc, n)
        n = 2
        call cfpolyline( xc2, yc2, n)
C
C       Display the glass for 5 seconds
C
        call sleep(5)
C
C       Terminate graphics
C
        call cfclosevws(name)
        call cfclosecgi()
        end
