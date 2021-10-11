/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char *sccsid = "@(#)isencrypt.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1 */
#endif

#include <ctype.h>

#define FBSZ 64
char *getenv();

/*
 * function that uses heuristics to determine if
 * a file is encrypted
 */

isencrypt(fbuf, ninbuf)
register char *fbuf;
int ninbuf;
{
	register char *fp;
	char *p;
	int crflag = 0;
	int i;
	if(ninbuf == 0)
		return 0;
	fp = fbuf;
	while (fp < &fbuf[ninbuf]) {

	/* First of all, if isprint fails, then we have a locale independant
	 * way of knowing for sure that we *think* we have an encrypted
	 * file. Granted we could be decrypting a binary file, but thats
	 * tough 
	 */
		if ( (!isprint(*fp)) && (!isascii(*fp)) )  {
			return 1;
		}
	fp++;
	}

	/* now continue on this doubtful (but useful) second test ... */

	if(ninbuf >= 64){

		/*
		 * Use chi-square test to determine if file 
		 * is encrypted if there are more
		 * than 64 characters in buffer.
		 */

		int bucket[8];
		float cs;

		for(i=0; i<8; i++) bucket[i] = 0;

		for(i=0; i<64; i++) bucket[(fbuf[i]>>5)&07] += 1;

		cs = 0.;
		for(i=0; i<8; i++) cs += (bucket[i]-8)*(bucket[i]-8);
		cs /= 8.;

		if(cs <= 24.322) {
			return 1;
		}
		
		return 0;
	}

	/* If we get here we have less than 64 bytes */

	if (fbuf[ninbuf - 1] != '\n')
		return 1;

	return 0;
}
