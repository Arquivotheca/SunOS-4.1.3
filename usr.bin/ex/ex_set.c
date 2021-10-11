/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_set.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.12 */
#endif

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"

/*
 * Set command.
 */
char	optname[ONMSZ];

set()
{
	register char *cp;
	register struct option *op;
	register int c;
	bool no;
	extern short ospeed;
#ifdef TRACE
	int k, label;
	line *tmpadr;
#endif

	setnoaddr();
	if (skipend()) {
		if (peekchar() != EOF)
			ignchar();
		propts();
		return;
	}
	do {
		cp = optname;
		do {
			if (cp < &optname[ONMSZ - 2])
				*cp++ = getchar();
		} while (isalnum(peekchar()));
		*cp = 0;
		cp = optname;
		if (eq("all", cp)) {
			if (inopen)
				pofix();
			prall();
			goto next;
		}
		no = 0;
#ifdef TRACE
 		/*
 		 * General purpose test code for looking at address of those
 		 * invisible marks (as well as the visible ones).
 		 */
 		if (eq("marks",cp)) {
 			printf("Marks   Address\n\r");
 			printf("					\n");
 			printf("\n");
 			for (k=0; k<=25; k++) 
 				printf("Mark:%c\t%d\n",k+'a',names[k]);
 		goto next;
 		}
 		
		/*
		 * General purpose test code for looking at
		 * named registers.
		 */

		if (eq("named",cp)) {
			if (inopen)
				pofix();
			shownam();
			goto next;
		}

		/*
	   	 * General purpose test code for looking at
		 * numbered registers.
		 */

		if (eq("nbrreg",cp)) {
			if (inopen)
				pofix();
			shownbr();
			goto next;
		}

		/*
 		 * General purpose test code for looking at addresses
		 * in the edit and save areas of VI.
 		 */
 
 		if (eq("buffers",cp)) {
 			if (inopen)
				pofix();
			printf("\nLabels   Address	Contents\n");
 			printf("======   =======	========");
			for (tmpadr = zero; tmpadr <= dol; tmpadr++) {
 				label =0;
 				if (tmpadr == zero) {
 					printf("ZERO:\t");
 					label = 2;
 				}
 				if (tmpadr == one) {
 					if (label > 0)
 						printf("\nONE:\t");
 					else
 						printf("ONE:\t");
 					label = 1;
 				}
 				if (tmpadr == dot) {
 					if (label > 0) 
 						printf("\nDOT:\t");
 					else
 						printf("DOT:\t");
 					label = 1;
 				}
 				if (tmpadr == undap1) {
 					if (label > 0)
 						printf("\nUNDAP1:\t");
 					else
 						printf("UNDAP1:\t");
 					label = 1;
 				}
 				if (tmpadr == undap2) {
 					if (label > 0)
 						printf("\nUNDAP2:\t");
 					else
 						printf("UNDAP2:\t");
 					label = 1;
 				}
 				if (tmpadr == unddel) {
 					if (label > 0)
 						printf("\nUNDDEL:\t");
 					else
 						printf("UNDDEL:\t");
 					label = 1;
 				}
 				if (tmpadr == dol) {
 					if (label > 0)
 						printf("\nDOL:\t");
 					else
 						printf("DOL:\t");
 					label = 1;
 				}
 				for (k=0; k<=25; k++) 
 					if (names[k] == (*tmpadr &~ 01)) {
 						if (label > 0)
 							printf("\nMark:%c\t%d\t",k+'a',names[k]);
 						else
 							printf("Mark:%c\t%d\t",k+'a',names[k]);
 						label=1;
 					}
 				if (label == 0) 
 					continue;
 				
 				if (label == 2)
 					printf("%d\n",tmpadr);
 				else  {
 					printf("%d\t",tmpadr);
 					getline(*tmpadr);
 					pline(lineno(tmpadr));
 					putchar('\n');
 				}
 			}
 			
 			for (tmpadr = dol+1; tmpadr <= unddol; tmpadr++) {
 				label =0;
 				if (tmpadr == dol+1) {
 					printf("DOL+1:\t");
 					label = 1;
 				}
 				if (tmpadr == unddel) {
 					if (label > 0)
 						printf("\nUNDDEL:\t");
 					else
 						printf("UNDDEL:\t");
 					label = 1;
 				}
 				if (tmpadr == unddol) {
 					if (label > 0)
 						printf("\nUNDDOL:\t");
 					else
 						printf("UNDDOL:\t");
 					label = 1;
 				}
 				for (k=0; k<=25; k++) 
 					if (names[k] == (*tmpadr &~ 01)) {
 						if (label > 0)
 							printf("\nMark:%c\t%d\t",k+'a',names[k]);
 						else
 							printf("Mark:%c\t%d\t",k+'a',names[k]);
 						label=1;
 					}
 				if (label == 0)
 					continue;
 				if (label == 2)
 					printf("%d\n",tmpadr);
 				else  {
 					printf("%d\t",tmpadr);
 					getline(*tmpadr);
 					pline(lineno(tmpadr));
 					putchar('\n');
 				}
 			}
 			goto next;
 		}
#endif 			
		if (cp[0] == 'n' && cp[1] == 'o' && cp[2] != 'v') {
			cp += 2;
			no++;
		}
		/* Implement w300, w1200, and w9600 specially */
		if (eq(cp, "w300")) {
			if (ospeed >= B1200) {
dontset:
				ignore(getchar());	/* = */
				ignore(getnum());	/* value */
				continue;
			}
			cp = "window";
		} else if (eq(cp, "w1200")) {
			if (ospeed < B1200 || ospeed >= B2400)
				goto dontset;
			cp = "window";
		} else if (eq(cp, "w9600")) {
			if (ospeed < B2400)
				goto dontset;
			cp = "window";
		}
		for (op = options; op < &options[vi_NOPTS]; op++)
			if (eq(op->oname, cp) || op->oabbrev && eq(op->oabbrev, cp))
				break;
		if (op->oname == 0)
			serror("%s: No such option@- 'set all' gives all option values", cp);
		c = skipwh();
		if (peekchar() == '?') {
			ignchar();
printone:
			propt(op);
			noonl();
			goto next;
		}
		if (op->otype == ONOFF) {
			op->ovalue = 1 - no;
			if (op == &options[vi_PROMPT])
				oprompt = 1 - no;
			goto next;
		}
		if (no)
			serror("Option %s is not a toggle", op->oname);
		if (c != 0 || setend())
			goto printone;
		if (getchar() != '=')
			serror("Missing =@in assignment to option %s", op->oname);
		switch (op->otype) {

		case NUMERIC:
			if (!isdigit(peekchar()))
				error("Digits required@after =");
			op->ovalue = getnum();
			if (value(vi_TABSTOP) <= 0)
				value(vi_TABSTOP) = TABS;
			if (op == &options[vi_WINDOW]) {
				if (value(vi_WINDOW) >= lines)
					value(vi_WINDOW) = lines-1;
				vsetsiz(value(vi_WINDOW));
			}
			break;

		case STRING:
		case OTERM:
			cp = optname;
			while (!setend()) {
				if (cp >= &optname[ONMSZ])
					error("String too long@in option assignment");
				/* adb change:  allow whitepace in strings */
				if( (*cp = getchar()) == '\\')
					if( peekchar() != EOF)
						*cp = getchar();
				cp++;
			}
			*cp = 0;
			if (op->otype == OTERM) {
/*
 * At first glance it seems like we shouldn't care if the terminal type
 * is changed inside visual mode, as long as we assume the screen is
 * a mess and redraw it. However, it's a much harder problem than that.
 * If you happen to change from 1 crt to another that both have the same
 * size screen, it's OK. But if the screen size if different, the stuff
 * that gets initialized in vop() will be wrong. This could be overcome
 * by redoing the initialization, e.g. making the first 90% of vop into
 * a subroutine. However, the most useful case is where you forgot to do
 * a setenv before you went into the editor and it thinks you're on a dumb
 * terminal. Ex treats this like hardcopy and goes into HARDOPEN mode.
 * This loses because the first part of vop calls oop in this case.
 */
				if (inopen)
					error("Can't change type of terminal from within open/visual");
				setterm(optname);
			} else {
				CP(op->osvalue, optname);
				op->odefault = 1;
			}
			break;
		}
next:
		flush();
	} while (!skipend());
	eol();
}


setend()
{

	return (iswhite(peekchar()) || endcmd(peekchar()));
}

prall()
{
	register int incr = (vi_NOPTS + 2) / 3;
	register int rows = incr;
	register struct option *op = options;

	for (; rows; rows--, op++) {
		propt(op);
		gotab(24);
		propt(&op[incr]);
		if (&op[2*incr] < &options[vi_NOPTS]) {
			gotab(56);
			propt(&op[2 * incr]);
		}
		putNFL();
	}
}

propts()
{
	register struct option *op;

	for (op = options; op < &options[vi_NOPTS]; op++) {
		if (op == &options[vi_TTYTYPE])
			continue;
		switch (op->otype) {

		case ONOFF:
		case NUMERIC:
			if (op->ovalue == op->odefault)
				continue;
			break;

		case STRING:
			if (op->odefault == 0)
				continue;
			break;
		}
		propt(op);
		putchar(' ');
	}
	noonl();
	flush();
}

propt(op)
	register struct option *op;
{
	register char *name;
	
	name = op->oname;

	switch (op->otype) {

	case ONOFF:
		printf("%s%s", op->ovalue ? "" : "no", name);
		break;

	case NUMERIC:
		printf("%s=%d", name, op->ovalue);
		break;

	case STRING:
	case OTERM:
		printf("%s=%s", name, op->osvalue);
		break;
	}
}
