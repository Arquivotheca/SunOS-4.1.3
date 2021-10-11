/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)insdelchar.c 1.1 92/07/30 SMI"; /* from S5R3 1.4.1.1 */
#endif

/*
 * Modify current screen line 'old' to match desired line 'new'.
 * 'ln' is the line number on the screen.
 * x x:set tabstop=4 shiftwidth=4:
 */

#include "curses.ext"

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))

/* Macros for costs to insert and delete nchars on this terminal */
#define costic(nchars) (_cost(icfixed) + ((nchars)*_cost(icvar)) )
#define costdc(nchars) (_cost(dcfixed) + _cost(dcvar)*nchars)

#define CHECKSTEP 10	/* Check every # chars for match */
#define MINDIFF    8	/* Don't use I/D if < # chars different */

static char chardiff[256];
static int shift[50], low[50], high[50], matchcount;
static chtype *nb, *ob;
static char *cp;
static int maxlen, olen, nlen;
static int movecost = 0, firstnonblank, firstdiff;
static chtype nullline[] = {0, 0};
#ifdef DEBUG
static int call_time = 0;
#endif /* DEBUG */

_id_char (old, new, ln)
register struct line   *old, *new;
int ln;
{
	register int ur;

	matchcount = 0;

	/* Quick checks for empty lines. */
	if (old) {
		olen = old->length; ob = old->body;
	} else {
		olen = 0; ob = nullline;
	}
	if (new) {
		nlen = new->length; nb = new->body;
	} else {
		nlen = 0; nb = nullline;
	}
	if (nlen <= 0) {
		if (olen > 0)
			_clrline(ln,ob,olen);
		return;
	}
	maxlen = max(olen, nlen);

#ifdef DEBUG
	if (outf) {
		fprintf(outf, "time %d\n", ++call_time);
		_dump_idc(old, new, ln);
	}
#endif

	if (movecost == 0) {
		register int i, j;
		i = _cost(Cursor_address);
		j = _cost(Parm_right_cursor);
		movecost = _cost(Column_address);
		if (i < movecost) movecost = i;
		if (j < movecost) movecost = j;
	}

	/*
	 * If any of these functions returns non-zero, punt and be quick
	 * about it. This handles many common cases with little overhead.
	 */
	if ((ur = _idc_flag_uneq()) || _idc_findmatch() || _idc_costs()) {
		if (ur >= 0)
			_idc_repchar(old, ln);
		return;
	}

	/* Do the fancy ins/del char stuff */
#ifdef DEBUG
	_idc_delchar(old, new, ln);
	_idc_inschar(old, new, ln);
#else
	_idc_delchar(old, ln);
	_idc_inschar(old, ln);
#endif /* DEBUG */
	_chk_typeahead();
}

/*
 * Find chars that line up exactly in new and old lines.
 * Return 1 iff insert/delete should be avoided.  Return
 * -1 if the lines are identical, for immediate return.
 *
 * The variables set below are:
 *
 * given line old:|    abcdefghijklm
 *   and line new:|          ghijktuvwxyz
 * we get:
 *                |                ^ minlen = 13 = minimum length
 *                |                     ^ maxlen = 18 = maximum length
 *                |    ^ firstdiff = 0 = 1st char diff between 2 lines
 *                |          ^ firstnonblank = 6 = 1st non-blank in new line
 *                |    ^^^^^^     ^^^ seen = 8 = count diffs btw. 2 lines
 *                |               ^^^ nbseen = 3 = count diffs after firstnonblank
 *                |0000111111000001111111 <- chardiff
 *
 * chardiff[] is used by _idc_findmatch(), _idc_costs(), _idc_delchar() and
 * _idc_inschar(). firstdiff and firstnonblank are used by _idc_repchar().
 *
 */
static
_idc_flag_uneq()
{
	register int j, seen = 0, nbseen = 0;
	register chtype *p = ob, *q = nb;
	register int minlen = olen + nlen - maxlen;

	/* nothing on old line? */
	if (olen <= 0) {
		for (firstdiff = 0;
		     (*q++ == ' ') && (firstdiff < minlen);
		     firstdiff++)
			;
		firstnonblank = firstdiff;
		return 1;
	}

	/* find first non-blank; fill in chardiff[] & seen */
	cp = chardiff;
	for (j = minlen; (j > 0) && (*q == ' '); j--)
		if (*cp++ = (*p++ != *q++))
			seen++;
	firstnonblank = minlen - j;

	/* continue with chardiff[] & seen & nbseen */
	for ( ; j>0; j--)
		if (*cp++ = (*p++ != *q++))
			seen++, nbseen++;

	/* are the lines the same? */
	if (seen == 0 && olen == nlen)
		return -1;	/* Lines are identical */

	/* find the first difference */
	if (seen == 0)
		firstdiff = minlen;
	else for (j = 0; j < minlen; j++)
		if (chardiff[j]) {
			firstdiff = j;
			break;
		}

	/* continue with chardiff[] */
	for (j=maxlen-minlen; j>0; j--)
		*cp++ = 1;
	if (olen != nlen)
		seen++, nbseen++;

#ifdef DEBUG
	if (outf) {
		fprintf(outf, "chardiff:           ");	/* line up with _dump_idc */
		for (j = 0; j < maxlen; j++)
			fprintf(outf, chardiff[j] ? "^" : ".");
		fprintf(outf, "\n");
		fprintf(outf, "maxlen = %d\n", maxlen);
	}
#endif /* DEBUG */

	/* are lines similar enough? */
	if (seen < MINDIFF || nbseen < MINDIFF) {
#ifdef DEBUG
		if(outf) fprintf(outf, "noid because diffs seen (%d) < MINDIFF %d, or diffs seen after first nonblank (%d after %d) < MINDIFF\n", seen, MINDIFF, nbseen, firstnonblank);
#endif
		return 1;
	}
	return 0;
}

/*
 * Find matching parts in the new and old lines.
 * Return 1 iff insert/delete should be avoided.
 *
 * As an example, consider the following case of old and new lines. The
 * booleans set in chardiff[] denoting differing characters are indicated
 * with a '^'.
 *
 * old:   79 SEND: Send all messages to recipients and of the messageABC23456789| PUSH ENTER
 * new:   79 SEND: Send the message to all recipients the message center23456789| PUSH ENTER
 *           0    +    1    +    2    +    3    +    4    +    5    +    6    +    7    +
 * chardiff: ...........^^^........^^^^^^^^^^^^^^^^^^^^^^.^^^^^^^^^^^^^^....................
 *
 * The output from this routine is the set of arrays low, high and shift.
 * The low and high arrays indicate groups of characters in the new line
 * which match groups of characters in the old line. If it takes an
 * insertion or deletion to make the groups match, the distance away is
 * indicated by the shift value. The corresponding low and high columns from
 * the old line are gotten from subtracting the shift value for that group.
 * Characters between the groups will be overstruck. The values of low,
 * high and shift for this example is given here. The corresponding low
 * and high columns from the old line are indicated by the olow and ohigh
 * columns.
 *
 *    i   low   high    shift   olow    ohigh
 *    0   -1    -1      0       -1      -1
 *    1   0     10      0       0       10
 *    2   29    40      3       26      37
 *    3   41    51      -4      45      55
 *    4   59    78      0       59      78
 *    5   79    79      0       79      79
 *
 * Group 1, columns 0-10 from old "SEND: Send " match columns 0-10 from new.
 * Group 1-2, columns 11-25 from old "all messages to" will be overstruck
 *    with "the message to ".
 * Group 2, columns 26-37 from old " recipients " match new columns 29-40
 *    " recipients ". The old must be shifted right 3 columns by deleting
 *    "and" from the end of old and inserting "all" at the beginning.
 * Group 2-3, there is nothing to overstrike between.
 * Group 3, columns 45-55 from old "the message" match new columns 41-51
 *    "the message". The old must be shifted left 4 columns by deleting
 *    "the ".
 * Group 3-4, columns 52-58. After making the above deletions, the old
 *    "ABC" will be above the new " cen", which must be overstruck
 *    followed by an insertion of "ter".
 * Group 4, at this point columns 59-78 of old "23456789| PUSH ENTER" will
 *    match columns 59-78 of new "23456789| PUSH ENTER" and the line is
 *    done.
 * Groups 0 and 5 act as sentinels during the algorithms.
 */
static
_idc_findmatch()
{
	register int k, h, l;
	register int nhub, ohub, nb_nhub;
	register chtype *chp;
	int ostart, highwat;

	/* Search for matches */
	shift[0] = 0; low[0] = high[0] = -1; matchcount = 1; highwat = 0;

	/* Look for ohub code matching nhub, stepping nhub through by CHECKSTEP. */
	for (nhub=0; nhub<nlen; nhub += CHECKSTEP) {

		/* ohub value for where to start looking */
		ostart = chardiff[nhub] ? highwat : nhub;

		/*
		 * This loop is likely to burn most of the CPU time, hence
		 * extra effort is taken to make it run quickly.
		 */
		nb_nhub = nb[nhub];	/* constant within loop */
		for (chp=ob+ostart,ohub=ostart; ohub<olen; ohub++) {
			if (nb_nhub == *chp++ /*ob[ohub]*/) {
				/*
				 * Found a possible match.  See how far in
				 * both directions it goes.  The double loops
				 * are a speed trick.  This is really ugly,
				 * but has to be very fast.
				 */
				/* to the left .. */
				if (nhub <= ohub) {
					for (l=nhub,k=ohub; l>=0 && nb[l]==ob[k]; l--,k--)
						;
				} else {
					for (l=nhub,k=ohub; k>=0 && nb[l]==ob[k]; l--,k--)
						;
				}
				l++;
				if (l < 0)
					l = 0;
				/* .. and the right */
				if (nhub >= ohub) {
					for (h=nhub,k=ohub; h<maxlen && nb[h]==ob[k]; h++,k++)
						;
				} else {
					for (h=nhub,k=ohub; k<maxlen && nb[h]==ob[k]; h++,k++)
						;
				}
				h--;
				if (h >= maxlen)
					h = maxlen - 1;
				if (h-l > MINDIFF) {	/* Count if at least MINDIFF long */
					int s = nhub-ohub;		/* amount shifted */
					int oh;				/* old high value */
					/* Maybe this match includes the previous match */
					if (l <= low[matchcount-1])
						matchcount--;
					/* If we overlap, do not count that part */
					if (l <= high[matchcount-1])
						l = high[matchcount-1] + 1;
					/* Cannot overlap in the old part, either */
					oh = high[matchcount-1] - shift[matchcount-1];
					if (l - s <= oh)
						l = oh + s + 1;
					/* Record high, low, nhub, ohub in list of matches */
					high[matchcount] = h;
					low[matchcount] = l;
					shift[matchcount++] = s;
					highwat = h - s + 2;
					nhub = highwat;
					break;
				}
			}
		}
	}
	shift[matchcount] = 0; low[matchcount] = high[matchcount] = maxlen;

#ifdef DEBUG
	if (outf) {
		fprintf(outf, "matchcount = %d\n", matchcount);
		_dump_shift((struct line *)0, (struct line *)0, 0, 0, "", 0);
	}
#endif

	return matchcount <= 1;
}

/*
 * Calculate the approximate costs with and without insert/delete char.
 * Return 1 iff insert/delete should be avoided.
 *
 * The cost NOT using insert/delete is derived from the chardiff[]
 * array. Groups of characters that are different would have to be
 * overstruck. Groups of characters that are the same would be skipped
 * over using cursor_address, parm_right_cursor or column_address.
 *
 * Similarly, the cost using insert/delete also includes the cost to
 * insert or delete characters from the ends of the groups.
 */
static
_idc_costs()
{
	register int i, j, k;
	register int cid, cnoid;

	cid = 0;
	if (low[1] > 0)
		cid += movecost + low[1];
	for (i=1; i<matchcount; i++) {
		j = shift[i] - shift[i-1];
		k = (j==0) ? 0 : (j<0) ? costdc(-j) : costic(j);
		if (j || (low[i+1] > high[i] + 1)) {
			cid += movecost +	/* to get cursor there */
				k +				/* i/d char cost */
				(low[i+1]-high[i] - 1);		/* cost to redraw next diff */
#ifdef DEBUG
			if (outf) fprintf(outf, "i %d, add %d + %d + %d for %d\n", i,
				movecost , k, (low[i+1]-high[i] - 1) , cid);
#endif
		}
	}
	cnoid = 0;
	for (i=0; i<nlen; i++)
		if (chardiff[i]) {
			cnoid++;
			if (!chardiff[i-1])
				cnoid += movecost;
		}
	if (nlen < maxlen)
		cnoid += _cost(Clr_eol);

#ifdef DEBUG
	if (outf)
		fprintf(outf, "costid %d, costnoid %d, noid %d\n", cid, cnoid, cnoid <= cid);
#endif
	return cnoid <= cid;
}

/*
 * Simple replacement strategy, not using insert/delete char.
 * _clrbol() gets rid of any leading blanks and
 * showstring does all the replacement work.
 */
static
_idc_repchar(old, ln)
register struct line *old;
{
#ifdef DEBUG
	if (outf)
		fprintf(outf, "doing simple replace char, starting at col %d with blanks to col %d\n", firstdiff, firstnonblank);
#endif

	firstdiff = _clrbol(firstnonblank, firstdiff, ln, ob);

	/* no need to blank out blanks */
	if (firstdiff == olen && nb[firstdiff] == ' ')
		while (nb[++firstdiff] == ' ')
			;

	/* Avoid extra cursor motion to left margin or to same text */
/* - untested
	while ((chardiff[firstdiff] == 0) && (firstdiff < minlen))
		firstdiff++;
*/
	_pos(ln, firstdiff);
	_showstring(ln, firstdiff, nb+firstdiff, nb+nlen-1, old);
	if (nlen < maxlen) {
		_pos(ln, nlen);
		_clreol();
	}
	_chk_typeahead();
}

/*
 * Ins/del char update strategy.  Do all deletions first to avoid
 * losing text from right edge of screen.
 */
#ifdef DEBUG
static
_idc_delchar(old, new, ln)
register struct line *old, *new;
#else
static
_idc_delchar(old, ln)
register struct line *old;
#endif /* DEBUG */
{
	register int i, l, oh_im1_p1, k;
	register chtype *p, *q, *r;
	int hi_im1, shif_im1, j;

#ifdef DEBUG
	if (outf) fprintf(outf, "doing delete chars\n");
#endif
	/* Do the deletions */
	for (i=1; i<matchcount; i++) {
#ifdef DEBUG
		if (outf) fprintf(outf, "comparing shift[i=%d] = %d to shift[i-1=%d] = %d.\n", i, shift[i], i-1, shift[i-1]);
#endif /* DEBUG */
		if (shift[i] < shift[i-1]) {		/* If it is a deletion */
			/*
			 * Overall strategy: there are two possibilities:
			 * the previous area is about to be shifted off
			 * the right end, so that we need to delete something
			 * from the end of that area, or we are in a region
			 * that needs to have text removed from the
			 * beginning. Both may occur together.
			 *
			 * In the first case, in which shift[i-1] > 0,
			 * we move to the end of the previous area and
			 * delete the extra characters.
			 *
			 *    QfghijkAB
			 *    QdefghiAB
			 *
			 * In the above example, the "jk" needs to be
			 * deleted so that the "de" may be inserted later
			 * in the insert routine.
			 *
			 * In the second case, there are two parts:
			 * first overstrike things that do not need to
			 * be deleted, then delete the rest.
			 *
			 *    Q d e f g h i A B		new
			 *    Q w x y z A B		old
			 *
			 * In the above example, we overstrike the "wxyz"
			 * over the old "defg", then delete the old "hi".
			 *
			 * If both cases occur together, we move to the
			 * end of the previous area and do the overstrikes
			 * there. Then both sets of character deletions
			 * get done together.
			 *
			 * In both cases, shift[i-1]-shift[i] will be the
			 * number of characters to be deleted, and
			 * low[i] - high[i-1] + 1 will be the number of
			 * characters to be overstruck.
			 */
			hi_im1 = high[i-1];
			shif_im1 = shift[i-1];
			oh_im1_p1 = hi_im1 - shif_im1 + 1;/* old start col (ohigh[i-1]+1)*/
			/* output overstrike part */
			p = nb + hi_im1 + 1;
			q = nb + low[i] - 1;
			/* number overstruck = low[i] - high[i-1] + 1 */
			if (p <= q) {
#ifdef DEBUG
				if (outf) fprintf(outf, "del-overstriking %d chars at column %d\n", q - p + 1, oh_im1_p1);
#endif /* DEBUG */
				_pos(ln, oh_im1_p1);
				_showstring(ln, oh_im1_p1, p, q, old);
				/* update internal copy of screen */
				r = ob + oh_im1_p1;
				oh_im1_p1 += q - p + 1;
				while (p <= q)	/* update old buffer */
					*r++ = *p++;
#ifdef DEBUG
				_dump_shift(old, new, ln, 1, "del-replacement", i);
#endif /* DEBUG */
			}

			if (shif_im1 > 0)	/* position to delete */
				l = oh_im1_p1;
			else
				l = low[i];
			k = shif_im1 - shift[i];		/* # chars to delete */
#ifdef DEBUG
			if (outf) fprintf(outf, "deleting %d chars at column %d\n", k, l);
#endif /* DEBUG */
			_pos(ln, l);
			_delchars(k);
			/* Update the data structures accordingly */
			for (j=i; j<matchcount; j++)	/* old is now shifted */
				shift[j] += k;
			for (j=l; j<maxlen; j++)
				ob[j] = ob[j+k];
			olen -= k;
			if (old)
				old->length = olen;
			low[i] = hi_im1 + 1;
#ifdef DEBUG
			_dump_shift(old, new, ln, 1, "deletion", i);
#endif
		}
	}
}

/*
 * Second half of ins/del char strategy: do all insertions and replacements.
 */
#ifdef DEBUG
static
_idc_inschar(old, new, ln)
register struct line *old, *new;
#else
static
_idc_inschar(old, ln)
register struct line *old;
#endif /* DEBUG */
{
	register int i, j, m, lhs_col;
	register chtype *p, *q, *r, *t;
	register int num_inserted, olhs_col, low_i, shif_i, i_1;

#ifdef DEBUG
	if (outf) fprintf(outf, "doing insert chars\n");
#endif
	/* Do the insertions for each match found */
	for (i = 1, i_1 = 0; i <= matchcount; i++, i_1++) {
#ifdef DEBUG
		if (outf) fprintf(outf, "comparing shift[i=%d] = %d to shift[i-1=%d] = %d.\n", i, shift[i], i_1, shift[i-1]);
#endif /* DEBUG */
		if (shift[i] >= shift[i_1]) {	/* if not a deletion */

			/*
			 * Overall strategy: two parts, first overstrike
			 * things that do not need to be inserted, then
			 * insert the rest.
			 *
			 *    Q w x y z A B		old
			 *    Q d e f g h i A B		new
			 *
			 * In the above example, we overstrike the "defg" over
			 * the old "wxyz", then insert "hi".
			 */

			low_i = low[i];
			shif_i = shift[i];

			/*
			 * Overstrike section.
			 * Find the first part of the area to overstrike.
			 * p points to the first char in the area, q to
			 * the last char in the area.  (In a pure insertion,
			 * q will point to the last char of the prev field.)
			 */
			lhs_col = high[i_1] + 1;	/* lhs of area */
			if (lhs_col >= low_i) {
				/* 0 or negative # of chars to show */
				continue;
			}

			p = nb + lhs_col;		/* beg of area in new */
			q = nb + low_i - 1 - shif_i;	/* end .. in new */

			if (p <= q) {
				int rightedge;
				_pos(ln, lhs_col);
#ifdef DEBUG
				if (outf) fprintf(outf, "ins-overstriking %d chars at column %d\n", q - p + 1, lhs_col);
#endif /* DEBUG */
				_showstring(ln, lhs_col, p, q, old);	/* do ovrstk */
				rightedge = lhs_col + q - p + 1;
				if (olen < rightedge)
					olen = rightedge;
				/* update internal copy of screen */
				r = ob + lhs_col;
				while (p <= q)	/* update old buffer */
					*r++ = *p++;
#ifdef DEBUG
				_dump_shift(old, new, ln, 1, "ins-replacement", i);
#endif
			}

			/* now do insertion part */
			num_inserted = shif_i - shift[i_1];
			if (num_inserted > 0) {
				lhs_col = low_i - shif_i;	/* lhs_col is pos of insertion */
				p = q + 1;	/* p, q: area to be inserted */
				q = p + num_inserted - 1;
				olhs_col = p - nb;
#ifdef DEBUG
				if (outf) fprintf(outf, "inserting %d chars at column %d\n", num_inserted, olhs_col);
# define putd(x)		if (outf) fprintf(outf, "x=%d\n", x);
#endif /* DEBUG */
				/* update some of what is about to happen */
				olen += num_inserted;
				if (olen > columns)
					olen = columns;
				if (maxlen < olen)
					maxlen = olen;
				m = olhs_col + num_inserted;

#ifdef DEBUG
				putd(olhs_col);
				putd(p-nb);
				putd(q-nb);
				putd(num_inserted);
				putd(m);
				putd(olen);
				putd(maxlen);
#endif /* DEBUG */

				/* do the insertion */
				_pos(ln, lhs_col);
				/*
				 * This if stmt should also check
				 * the relative cost of doing
				 * (parm_ich x n) versus
				 * (smir + n*ich + rmir).
				 */
				if (num_inserted > 1 && parm_ich) {
					/*
					 * Insert the characters and
					 * then draw on the blanks.
					 */
					_clearhl();
					_inschars(num_inserted);
					/* fix up what _inschars did */
					for (j = maxlen - 1,
					     r = ob + j,
					     t = ob + j - num_inserted;
					     j-- >= m; )
					        *r-- = *t--;
					for (r = ob + olhs_col,
					     j = num_inserted; j-- > 0; )
						*r++ = ' ';
					/*
					 * Now output the string
					 * in non-insert mode.
					 */
					_showstring(ln, lhs_col, p, q, (struct line *) NULL);
					/* fix up what _showstring did */
					for (r = ob + olhs_col; p <= q; )
						*r++ = *p++;
				} else {
					/*
					 * Type the characters in
					 * "insert mode".  This includes
					 * having to send insert_character
					 * before each character
					 * is output.
					 */
					_insmode(1);
					_showstring(ln, lhs_col, p, q, (struct line *) NULL);
					/* fix up what _showstring did */
					for (j = maxlen - 1,
					     r = ob + j,
					     t = ob + j - num_inserted;
					     j-- >= m; )
					        *r-- = *t--;
					for (r = ob + olhs_col; p <= q; )
						*r++ = *p++;
				}
				_insmode(0);

				/* finish fixing up notion of what happened */
				for (j = i; j < matchcount; j++)
					shift[j] -= num_inserted;
				if (old)
					old->length = olen;
#ifdef DEBUG
				_dump_shift(old, new, ln, 1, "insertion", i);
#endif
			}
		}
	}

	/* Clean up any junk off the end of the line */
	if (olen > high[matchcount]) {
		_pos(ln, high[matchcount]);
		_clreol();
		if (old)
			old->length = high[matchcount];
	}
}

#ifdef DEBUG
/*
 * Debugging routine to show current state of affairs.  old is what's
 * physically on the screen (reflecting any partial updates so far)
 * and new is what we want.  ln is the line number.
 */
static
_dump_idc(old, new, ln)
register struct line   *old, *new;
{
	register int n, j;

	if (!outf)
		return;
	fprintf(outf, "_id_char(%x, %x, %d)\n", old, new, ln);
	fprintf(outf, "old: ");
	fprintf(outf, "%8x: ", old);
	if (old == NULL) {
		fprintf(outf, "()\n");
	} else {
		fprintf(outf, "%4d ", old->length);
		for (j=0; j<old->length; j++) {
			n = old->body[j];
			if (n & A_ATTRIBUTES)
				putc('\'', outf);
			n &= 0177;
			fprintf(outf, "%c", n>=' ' ? n : '.');
		}
		fprintf(outf, "\n");
	}
	fprintf(outf, "new: ");
	fprintf(outf, "%8x: ", new);
	if (new == NULL) {
		fprintf(outf, "()\n");
	} else {
		fprintf(outf, "%4d ", new->length);
		for (j=0; j<new->length; j++) {
			n = new->body[j];
			if (n & A_ATTRIBUTES)
				putc('\'', outf);
			n &= 0177;
			fprintf(outf, "%c", n>=' ' ? n : '.');
		}
		fprintf(outf, "\n");
	}
	/* dump a ruler */
	fprintf(outf, "                    ");
	for (j = 0; j < maxlen; j++)
		fprintf(outf, (j % 10) ? (j % 5 ? " " : "+" ) : "%1d", j / 10);
	fprintf(outf, "\n");
}

/*
    Debugging routine to print out status of low, high and shift arrays.
*/
static
_dump_shift(old, new, ln, dumpidc, pass, i)
register struct line *old, *new;
char *pass;
int ln, dumpidc, i;
{
	register int j;
	if (!outf)
		return;
	if (dumpidc) {
		fprintf(outf, "After %s pass i=%d\n", pass, i);
		_dump_idc(old, new, ln);
	}
#define olow(j)		low[j] - shift[j]
#define ohigh(j)	high[j] - shift[j]
	fprintf(outf, " i  low	high	shift	olow	ohigh\n");
	for (j=0; j<=matchcount; j++)
		fprintf(outf, "%2d  %d	%d	%d	%d	%d\n",
			j, low[j], high[j], shift[j],
			olow(j), ohigh(j));
	fprintf(outf, "\n");
	fflush(outf);
}
#endif

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
static
_showstring(sline, scol, first, last, oldlp)
int sline, scol;
chtype *first, *last;	/* pointers to beg/end of chtype string to be shown */
struct line *oldlp;	/* pointer to old line being overwritten */
{
	register int hl = 0;	/* nontrivial line, highlighted or with holes */
	int prevhl=SP->virt_gr, thishl;	/* highlight state tty is in	 */
	register chtype *p, *px;	/* current char being considered */
	register chtype *oldp, *oldpx;	/* stuff under p and px		 */
	register char *pc, *pcx;	/* like p, px but in char buffer */
	chtype *tailoldp;		/* last valid oldp		 */
	int oldlen;			/* length of old line		 */
	int lcol, lrow;			/* position on screen		 */
	chtype oldc;			/* char at oldp			 */
	chtype leftattr, leftnewc;	/* attr,char to left of p	 */
	int diff_cookies;		/* magic cookies changed	 */
	int diff_attrs;			/* highlights changed		 */
	chtype *oldline;
#ifdef NONSTANDARD
	static
#endif /* NONSTANDARD */
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
		_writechars(firstc, lastc, first[-1]);
		return;
	}

#ifdef DEBUG
	if(outf) {
		fprintf(outf, "oldlp %x, oldline %x, oldlen %d 0x%x\n",
			oldlp, oldline, oldlen, oldlen);
		fprintf(outf, "old body('");
		if (oldlp)
			for (p=oldline; p<oldline+oldlen; p++) {
				if (*p & A_ATTRIBUTES)
					fprintf(outf, "'");
				fprintf(outf, "%c", *p & A_CHARTEXT);
			}
		fprintf(outf, "').\n");
	}
#endif
	if (scol > 0)	oldc = first[-1];
	else		oldc = ' ';
	tailoldp = oldline + oldlen;
	for (p=first, oldp=oldline+lcol, pc=firstc; pc<=lastc; p++,oldp++,pc++) {
		thishl = *p & A_ATTRIBUTES;
#ifdef DEBUG
		if(outf) fprintf(outf, "prevhl %o, thishl %o\n", prevhl, thishl);
#endif
		if (p > first)	leftnewc = p[-1];
		else		leftnewc = ' ';
		leftattr = leftnewc & A_ATTRIBUTES;
		diff_cookies = magic_cookie_glitch>=0 &&
			((oldc&A_ATTRIBUTES) != leftattr);
		diff_attrs = ceol_standout_glitch && (((*p)&A_ATTRIBUTES) != leftattr);
		if (oldp >= tailoldp)
			oldc = ' ';
		else
			oldc = *oldp;
#ifdef xxDEBUG
/* This causes core dump on Sun and other systems that trap on ref to loc 0 */
		if(outf) fprintf(outf, "p %x *p %o, pc %x *pc %o, oldp %x, *oldp %o, lcol %d, lrow %d, oldc %o\n", p, p ? *p : 0, pc, pc ? *pc : 0, oldp, oldp ? *oldp : 0, lcol, lrow, oldc);
#endif
		if (*p != oldc || SP->virt_irm == 1 || diff_cookies || diff_attrs ||
				insert_null_glitch && oldp >= oldline+oldlen) {
			register int n;

			_pos(lrow, lcol);

			/*
			 * All HP terminals except 2621 (as of 1984).
			 * This forces it to be right no matter what
			 * was there before.  This is quite nonoptimal
			 * but the alternative is to maintain a map of
			 * cookie locations on the screen (you can't
			 * get rid of a cookie short of clr_eol).
			 *
			 * There is a case where this is wrong: if you
			 * draw something highlighted, then overwrite it
			 * with something unhighlighted, then overwrite
			 * it again with something highlighted, it will
			 * only highlight the first character.  This could
			 * be fixed by removing the 3rd clause below, at the
			 * cost of much worse performance in average cases
			 * for simple highlighting.  To make this work
			 * right in every case, this clause has been
			 * commented out.
			 *
			 * If you must use an HP terminal, HP has an ANSI
			 * option for most of its terminals that does
			 * things right.  You can order that option.
			 * This code deals with the standard HP terminals.
			 */
			if (ceol_standout_glitch &&
				thishl != (oldc&A_ATTRIBUTES)
				/* && (oldc&A_ATTRIBUTES) */
				) {
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
			_writechars(pc, pcx, leftnewc);
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
