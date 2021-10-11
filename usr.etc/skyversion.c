#ifndef lint
static	char sccsid[] = "@(#)skyversion.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
**	utility to exercise the sky board's version number opcode and print it
**	attempts to ensure that the version number is meaningful:
**	microcode versions earlier than 3.0 have no version number op code
*/

# include <stdio.h>

double get_skyversion();

main()
{
double d1,d2;
char string[100], *cp;
int len, extra_digits;

	if(!_skyinit()) {
		fprintf(stderr,"SKYFFP board not available\n");
		exit(1);
	}
	d1 =  get_skyversion();
	d2 =  get_skyversion();
	sprintf(string,"%f",d2);
	len = strlen(string);
	extra_digits = 0;
	for(cp=string+4;cp<&string[len-1];cp++) {
		if(*cp != '0') {
			extra_digits = 1;
			break;
		}
	}
	if(
		/* extra_digits || */
		d1 != d2 || d2 < 3.) {
		fprintf(stderr,"SKYFFP microcode version number not available\n");
		exit(1);
	} else {
		string[4]='\0';
		printf("SKYFFP microcode version %s\n",string);
		exit(0);
	}
}

double
get_skyversion()
{
long context[8];
	asm("movl	__skybase,a7");
	asm("movw	#0X1022,a7@(-4)	| opcode to get microcode version in r3");
	asm("movw	#0X1040,a7@(-4)	| save ffp context to access r3");
	asm("movl	a7@,a6@(-4)		| 1st half of r0");
	asm("movl	a7@,a6@(-8)		| 2nd half");
	asm("movl	a7@,a6@(-12)");
	asm("movl	a7@,a6@(-16)");
	asm("movl	a7@,a6@(-20)");
	asm("movl	a7@,a6@(-24)");
	asm("movl	a7@,a6@(-28)	| 1st half of r3");
	asm("movl	a7@,a6@(-32)	| 2nd half");
	asm("movw	#0X1042,a7@(-4)	| convert sp version # to a double");
	asm("movl	a6@(-28),a7@");
	asm("movl	a7@,d0			| and return it");
	asm("movl	a7@,d1");
}
