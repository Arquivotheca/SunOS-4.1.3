#ifndef lint
static  char sccsid[] = "@(#)book.c 1.1 92/07/30 SMI";
#endif

#include "old.h"

bookm()
{
	short i, buf[2];
	short cnt;

	if(!bookp) return(0);
	lseek(bookf, bookp, 0);

       cnt = 0;
       while (1) {
               if (read(bookf, buf, 4) != 4)
			return 0;
               if(*buf < 0)
                       break;
               cnt++;
       }
       if (cnt == 0)
               return 0;
       cnt = (random() % cnt) + 1;/*  number of times to read */
       lseek(bookf, bookp, 0);
       while (cnt-- > 0)
               read(bookf, buf, 4);
       abmove = *buf;
       return(1);
}

makmov(m)
{
	short buf[2];

	out1(m);
	mantom? bmove(m): wmove(m);
	increm();
	if(!bookp) return;
	lseek(bookf, bookp, 0);

loop:
	read(bookf, buf, 4);
	if(m == *buf || *buf == 0) {
		bookp = buf[1] & ~1;
		goto l1;
	}
	if(*buf < 0) {
		bookp = 0;
		goto l1;
	}
	goto loop;

l1:
	if(!bookp) {
		return;
	}
}
