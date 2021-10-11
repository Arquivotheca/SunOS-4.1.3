#ifndef lint
static	char sccsid[] = "@(#)showstring.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Dump the string running from first to last out to the terminal.
 * Take into account attributes, and attempt to take advantage of
 * large pieces of white space and text that's already there.
 * oldline is the old text of the line.
 *
 * Variable naming convention: *x means "extension", e.g. a rubber band
 * that briefly looks ahead; *c means a character version of an otherwise
 * chtype pointer; old means what was on the screen before this call;
 * left means the char 1 space to the left.
 */
_showstring(sline, scol, first, last, oldlp)
int sline, scol;
chtype *first, *last; 
struct line *oldlp;
{
	register int hl = 0;	/* nontrivial line, highlighted or with holes */
	int prevhl=SP->virt_gr, thishl;	/* highlight state tty is in	 */
	register chtype *p, *px;	/* current char being considered */
	register chtype *oldp, *oldpx;	/* stuff under p and px		 */
	register char *pc, *pcx;	/* like p, px but in char buffer */
	chtype *tailoldp;		/* last valid oldp		 */
	int oldlen;			/* length of old line		 */
	int lcol, lrow;			/* position on screen		 */
	int oldc;			/* char at oldp			 */
	int leftoldc, leftnewc;		/* old & new chars to left of p  */
	int diff_cookies;		/* magic cookies changed	 */
	int diff_attrs;			/* highlights changed		 */
	chtype *oldline;
#ifdef NONSTANDARD
	static
#endif NONSTANDARD
	   char firstc[256], *lastc;	/* char copy of input first, last */

#ifdef DEBUG
	if(outf) fprintf(outf, "_showstring((%d,%d) %d:'", sline, scol, last-first+1);
	if(outf)
		for (p=first; p<=last; p++) {
			thishl = *p & A_ATTRIBUTES;
			if (thishl)
				putc('\'', outf);
			putc(*p & A_CHARTEXT, outf);
		}
	if(outf) fprintf(outf, "').\n");
#endif
	if (last-first > columns) {
		_pos(lines-1, 0);
#ifndef		NONSTANDARD
		fprintf(stderr, "Bad call to _showstring, first %x, last %x,\
 diff %d\pcx\n", first, last, last-first);
#endif
		abort();
	}
	if (oldlp) {
		oldline = oldlp->body;
		oldp = oldline+scol;
	}
	else
		oldp = 0;
	for (p=first,lastc=firstc; p<=last; ) {
		if (*p & A_ATTRIBUTES)
			hl++;	/* attributes on the line */
		if (oldp && (*oldp++ & A_ATTRIBUTES))
			hl++;	/* attributes on old line */
		if (*p==' ' && (px=p+1,*px++==' ') && *px++==' ' && *px==' ')
			hl++;	/* a run of at least 4 blanks */
		*lastc++ = *p & A_CHARTEXT;
		p++;	/* On a separate line due to C optimizer bug */
#ifdef FULLDEBUG
	if(outf) fprintf(outf, "p %x '%c' %o, lastc %x %o, oldp %x %o, hl %d\n", p, p[-1], p[-1], lastc, lastc[-1], oldp, oldp ? oldp[-1] : 0, hl);
#endif
	}
	lastc--;

	lcol = scol; lrow = sline;
	if (oldlp) {
		oldline = oldlp->body;
		oldlen = oldlp->length;
		/* Check for runs of stuff that's already there. */
		for (p=first,oldp=oldline+lcol; p<=last; p++,oldp++) {
			if (*p==*oldp && (px=p+1,oldpx=oldp+1,*px++==*oldpx++)
					  && *px++==*oldpx++ && *px==*oldpx)
				hl++;	/* a run of at least 4 matches */
#ifdef FULLDEBUG
	if(outf) fprintf(outf, "p %x '%c%c%c%c', oldp %x '%c%c%c%c', hl %d\n",
	p, p[0], p[1], p[2], p[3],
	oldp, oldp[0], oldp[1], oldp[2], oldp[3],
	hl);
#endif
		}
	} else {
		oldline = NULL;
		oldlen = 0;
	}

	if (!hl) {
		/* Simple, common case.  Do it fast. */
		_pos(lrow, lcol);
		_hlmode(0);
		_writechars(firstc, lastc);
		return;
	}

#ifdef DEBUG
	if(outf) fprintf(outf, "oldlp %x, oldline %x, oldlen %d 0x%x\n", oldlp, oldline, oldlen, oldlen);
	if(outf) fprintf(outf, "old body('");
	if (oldlp)
		for (p=oldline; p<=oldline+oldlen; p++)
			if(outf) fprintf(outf, "%c", *p);
	if(outf) fprintf(outf, "').\n");
#endif
	oldc = first[-1];
	tailoldp = oldline + oldlen;
	for (p=first, oldp=oldline+lcol, pc=firstc; pc<=lastc; p++,oldp++,pc++) {
		thishl = *p & A_ATTRIBUTES;
#ifdef DEBUG
		if(outf) fprintf(outf, "prevhl %o, thishl %o\n", prevhl, thishl);
#endif
		leftoldc = oldc & A_ATTRIBUTES;
		leftnewc = p[-1] & A_ATTRIBUTES;
		diff_cookies = magic_cookie_glitch>=0 && (leftoldc != leftnewc);
		diff_attrs = ceol_standout_glitch && (((*p)&A_ATTRIBUTES) != leftnewc);
		if (oldp >= tailoldp)
			oldc = ' ';
		else
			oldc = *oldp;
#ifdef DEBUG
		if(outf) fprintf(outf, "p %x *p %o, pc %x *pc %o, oldp %x, *oldp %o, lcol %d, lrow %d, oldc %o\n", p, *p, pc, *pc, oldp, *oldp, lcol, lrow, oldc);
#endif
		if (*p != oldc || SP->virt_irm == 1 || diff_cookies || diff_attrs ||
				insert_null_glitch && oldp >= oldline+oldlen) {
			register int n;

			_pos(lrow, lcol);

			/*
			 * HP 2645/2626: output new for each char.
			 * This forces it to be right no matter what
			 * was there before.
			 */
			if (ceol_standout_glitch && thishl == 0 && oldc&A_ATTRIBUTES) {
#ifdef FULLDEBUG
				if(outf) fprintf(outf,
					"ceol %d, thishl %d, prevhl %d\n",
					ceol_standout_glitch, thishl, prevhl);
#endif
				_forcehl();
			}

			/* Force highlighting to be right */
			_hlmode(thishl);
			if (thishl != prevhl) {
				if (magic_cookie_glitch >= 0) {
					_sethl();
					p += magic_cookie_glitch;
					oldp += magic_cookie_glitch;
					pc += magic_cookie_glitch;
					lcol += magic_cookie_glitch;
				}
			}

			/*
			 * Gather chunks of chars together, to be more
			 * efficient, and to allow repeats to be detected.
			 * Not done for blanks on cookie terminals because
			 * the last one might be a cookie.
			 */
			if (magic_cookie_glitch<0 || *pc != ' ') {
				for (px=p+1,oldpx=oldp+1;
					px<=last && *p==*px;
					px++,oldpx++) {
					if(!(repeat_char && oldpx<tailoldp && *p==*oldpx))
						break;
				}
				px--; oldpx--;
				n = px - p;
				pcx = pc + n;
			} else {
				n = 0;
				pcx = pc;
			}
			_writechars(pc, pcx);
			lcol += n; pc += n; p += n; oldp += n;
			prevhl = thishl;
		}
		lcol++;
	}
	if (magic_cookie_glitch >= 0 && prevhl) {
		/* Have to turn off highlighting at end of line */
		_hlmode(0);
		_sethl();
	}
}
