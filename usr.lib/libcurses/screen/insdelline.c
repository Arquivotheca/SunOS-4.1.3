/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)insdelline.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

/* Routines to deal with calculation of how to do ins/del line optimally */
#include "curses.ext"
int InputPending;

struct st {		/* symbol table */
	int hash;	/* hashed value of line ("text") */
	char oc, nc;	/* 0, 1, or many: # times this value occurs */
	short olno;	/* line # of line in "old" array */
};

/*
 * Update the Screen.  Consider using insert and delete line.
 * Using Heckel's algorithm from April 1978 CACM.
 *
 * This algorithm is based on two observations:
 *  (1) A line that occurs once and only once in each file must be
 *	the same line (unchanged but possibly moved).
 *  (2) If i each file immediately adjacent to a "found" line pair
 *	there are lines identical to each other, these lines must be
 *	the same line.  Repeated application will "find" sequences
 *	of unchanged lines.
 *
 * We view the old and new screens' hashed values as the old and new "files".
 * Since crossing match lines cannot be taken advantage of with real insert
 * and delete line, we ignore those matches - this is the one real disadvantage
 * of this algorithm.
 *
 * first and last are the integer line numbers (first line is 1) of the
 * portion of the screen to consider.
 */
_id_line(first, last)
register int first, last;
{
	register int i, j, k, n;
	int base, ndel;
	int idl_offset = 0;	/* number of extra del lines done so far. */
#ifdef NONSTANDARD
	static
#endif /* NONSTANDARD */
		struct st symtab[256];
	/*
	 * oa and na represent the old and new files (SP->cur_body and
	 * SP->std_body, respectively.)  If they are > 0 they are
	 * subscripts in each other (e.g. na[4] = 3 means that na[4]
	 * points to oa[3]).  If they are <= 0 their negative is taken
	 * as a symbol table index into the symtab array.
	 */
#ifdef NONSTANDARD
	static
#endif /* NONSTANDARD */
		short oa[256], na[256];

	for (i=0; i<256; i++)
		symtab[i].hash = symtab[i].oc = symtab[i].nc = 0;

#ifdef DEBUG
	if(outf) fprintf(outf, "_id_line(%d, %d)\n", first, last);
#endif
	/* Pass 1: enter old file into symtab */
	for (i=first; i<=last; i++) {
		n = _getst(SP->std_body[i]->hash, symtab);
		symtab[n].nc++;
		na[i] = -n;
	}

#ifdef LONGDEBUG
	if(outf) fprintf(outf, "Pass 2\n");
	if(outf) fprintf(outf, "na[12] = %d\n", na[12]);
#endif
	/* Pass 2: enter new into symtab */
	for (i=first; i<=last; i++) {
		n = _getst(SP->cur_body[i]->hash, symtab);
		symtab[n].oc++;
		symtab[n].olno = i;
		oa[i] = -n;
	}

#ifdef LONGDEBUG
	if(outf) fprintf(outf, "Pass 3\n");
	if(outf) fprintf(outf, "\nsymtab	oc	nc	olno	hash\n");
	for (i=0; i<256; i++)
		if (symtab[i].hash)
			if(outf) fprintf(outf, "%d	%d	%d	%d	%d\n",
				i, symtab[i].oc, symtab[i].nc,
				symtab[i].olno, symtab[i].hash);
#endif
	/*
	 * Pass 3: Match all lines with exactly one match.
	 * Matching lines in oa and na point at each other.
	 */
	for (i=first; i<=last; i++) {
		n = -na[i];
		if (symtab[n].oc == 1 && symtab[n].nc == 1) {
			na[i] = symtab[n].olno;
			oa[na[i]] = i;
		}
	}
	oa[first-1] = na[first-1] = first-1;
	oa[last+1] = na[last+1] = last+1;

	/* Pass 4: Find following implied matches based on sequence */
#ifdef DEBUG
	if(outf) fprintf(outf, "Pass 4\n");
#endif
	for (i=first; i<=last; i++) {
		if (na[i] > 0 && na[i+1] <= 0) {
			j = na[i];
			if (na[i+1] == oa[j+1]) {
				oa[j+1] = i+1;
				na[i+1] = j+1;
			}
		}
	}

	/* Pass 5: Find preceding implied matches based on sequence */
#ifdef DEBUG
	if(outf) fprintf(outf, "Pass 5\n");
	if(outf) fprintf(outf, "na[12] = %d\n", na[12]);
#endif
	for (i=last; i>=first; i--) {
		if (na[i] > 0 && na[i-1] <= 0) {
			j = na[i];
			if (na[i-1] == oa[j-1]) {
				oa[j-1] = i-1;
				na[i-1] = j-1;
			}
		}
	}

#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 5\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\n");
#endif
	/* Pass 6: Find matches from more than one line, in order.  */
	for (i=first; i<=last; i++) {
		if (na[i] < -1 && symtab[-na[i]].nc > 1) {
			j = na[i];
#ifdef DEBUG
			if(outf) fprintf(outf, "i %d, na[i] %d\n", i, na[i]);
#endif
			for (k=first; k<last; k++) {
				if (j == oa[k]) {
#ifdef DEBUG
			if(outf) fprintf(outf, "k %d, oa[i] %d, matching them up\n", k, oa[i]);
#endif
					na[i] = k;
					oa[k] = i;
					break;
				}
			}
		}
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 6\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\n");
#endif

	/*
	 * Passes 7abcd: Remove any match lines that cross backwards.
	 * (Draw lines connecting the matching lines on debug output
	 * to see what I mean about crossing lines.)
	 * The na's must be monotonically increasing except for those < 0
	 * which say there is no match.  k counts the number of matches
	 * we have to throw away.
	 */
	n = k = 0;	/* k is the added cost to throw away nw-se matches */
	/* 7a: get nw-se cost in k */
	for (i=first; i<=last; i++) {
		if (na[i] > 0 && na[i] < n) {
			k += SP->std_body[i]->bodylen;
		}
		if (na[i] > n)
			n = na[i];
	}
	/* 7b: get sw-ne cost in j */
	/* Consider throwing away in the other direction.  */
	if (k > 0) {
		j = 0;	/* j is the added cost to throw away sw-ne matches */
		n = last+1;
		for (i=last; i>=first; i--) {
			if (na[i] > 0 && na[i] > n) {
				j += SP->std_body[i]->bodylen;
			}
			if (na[i] > 0 && na[i] < n)
				n = na[i];
		}
	}
	else
		j = 1;	/* will be > 0 for sure */
#ifdef DEBUG
	if(outf) fprintf(outf, "forward, k is %d, backward, j is %d\n", k, j);
#endif

	/* Remove whichever is cheapest. */
	if (k < j) {
		/* 7c: remove nw-se */
		n = 0;
		for (i=first; i<=last; i++) {
			if (na[i] > 0 && na[i] < n) {
				oa[na[i]] = -1;
				na[i] = -1;
			}
			if (na[i] > n)
				n = na[i];
		}
	} else {
		/* 7d: remove sw-ne */
		n = last+1;
		for (i=last; i>=first; i--) {
			if (na[i] > 0 && na[i] > n) {
				oa[na[i]] = -1;
				na[i] = -1;
			}
			if (na[i] > 0 && na[i] < n)
				n = na[i];
		}
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 7\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\nPass 7\n");
	if(outf) fprintf(outf, "ILmf %d/32, ILov %d\n",
		SP->term_costs.ilvar, SP->term_costs.ilfixed);
	if(outf) fprintf(outf, "i	base	j	k	DC	n\n");
#endif

	/*
	 * Pass 8: we go through and remove all matches if the
	 * overall ins/del lines would be too expensive to capatilize on.
	 * j is cost with ins/del line, k is cost without it.  base is the
	 * logical beginning of the oa array, as the array would be after
	 * inserts and deletes, ignoring anything before row i.  This
	 * approximates things by not taking into account SP->curptr motion,
	 * funny insert line costs, or lines that are partially equal.
	 * This macro is the cost to insert or delete n lines at screen line i.
	 * It is approximated for speed and simplicity, but shouldn't be.
	 * The approximation assumes each line is deleted separately.
	 */

#define idlcost(n, i) n * \
	(((SP->term_costs.ilvar * (lines-i))>>5) + SP->term_costs.ilfixed)

	base = ndel = 0;
	j = k = 0;
	for (i=first; i<=last; i++) {
#ifdef DEBUG
		n = 0;
#endif
		if (oa[i] != na[i]) {
			/* Cost to turn Phys[i] into Des[i] on same line */
			if (SP->cur_body[i] == 0 || SP->cur_body[i]->length == 0) {
				k += SP->std_body[i]->bodylen;
			} else if (SP->std_body[i] == 0 ||
				   SP->std_body[i]->length == 0) {
				k += 2;	/* guess at cost of clr to eol */
			} else {
				register chtype *p0, *p1, *p2;
				n = SP->cur_body[i]->length -
				    SP->std_body[i]->length;
				if (n > 0) {
					k += n;
					n = SP->std_body[i]->length;
				} else {
					k += -n;
					n = SP->cur_body[i]->length;
				}
				p0 = &SP->std_body[i]->body[0];
				p1 = &SP->std_body[i]->body[n];
				p2 = &SP->cur_body[i]->body[n];
				while (--p1 >= p0)
					if (*p1 != *--p2)
						k++;
			}
		}
		/* cost to do using ins/del line */
		if (na[i] <= 0) {
			/* totally different - plan on redrawing whole thing
			 * (even though chances are good they are partly the
			 * same - _id_char will take care of this, if it
			 * happens to get the same line on the screen).
			 * Should probably figure out what will be there on
			 * the screen and do above k algorithm on it.
			 */
			j += SP->std_body[i]->bodylen;
		} else if ((n = (na[i] - base) - i) > 0) {
			/* delete line */
			j +=  idlcost(n, i);
			ndel += n;
			base += n;
		} else if (n < 0) {
			/* insert line */
			j +=  idlcost((-n), i);
			ndel += n;
			base += n;
		}
		/* else they are the same line: no cost */
#ifdef DEBUG
		if (outf) {
			fprintf(outf, "%d	%d	%d	%d	%d", i, base, j, k, SP->std_body[i]->bodylen);
			if (n < 0)
				fprintf(outf, "	%d lines inserted", -n);
			else if (n > 0)
				fprintf(outf, "	%d lines deleted", n);
			fprintf(outf, "\n");
		}
#endif
	}
	if (j >= k) {
		/* It's as cheap to just redraw, so do it. */
		for (i=first; i<=last; i++)
			na[i] = oa[i] = -1;
	} else if (ndel < 0) {
		ndel = -ndel;
#ifdef DEBUG
		if(outf) fprintf(outf, "will do %d extra inserts, so del now.\n", ndel);
#endif
		if (SP->des_bot_mgn - last >= 0) {
			_pos(last-ndel, 0);
			idl_offset += ndel;
			_dellines(ndel);
		}
	}

#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 8\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\n");
#endif
	/* Pass 9: Do any delete lines that are needed */
	k = first-1;	/* previous matching spot in oa */
	ndel = 0;
	for (i=first; i<=last; i++) {
		if (oa[i] > 0) {
			n = (i-k) - (oa[i]-oa[k]);
#ifdef DEBUG
			if(outf) fprintf(outf,
			"del loop, i %d, k %d, oa[i] %d, oa[k] %d, n %d\n",
			i, k, oa[i], oa[k], n);
#endif
			if (n > 0) {
				if (i-n == SP->des_top_mgn+1) {
					_scrollf(n);
				} else {
					_pos(i-n-1, 0);
					_dellines(n);
				}
				idl_offset += n;
				ndel += n;
				for (j=i-n; j<=last-n; j++) {
					if (oa[j] > 0 && na[oa[j]] > 0)
						na[oa[j]] -= n;
					_line_free(SP->cur_body[j]);
					SP->cur_body[j] = SP->cur_body[j+n];
					oa[j] = oa[j+n];
				}
				for ( ; j<=last; j++) {
					if (oa[j] > 0 && na[oa[j]] > 0)
						na[oa[j]] -= n;
					_line_free(SP->cur_body[j]);
					SP->cur_body[j] = NULL;
					oa[j] = -1;
				}
				i -= n;
			}
			k = i;
		}
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 9\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\n");
#endif

	/* Half way through - check for typeahead */
	_chk_typeahead();
	if (idl_offset == 0 && InputPending) {
#ifdef DEBUG
		if (outf) fprintf(outf, "InputPending %d, aborted after dellines\n", InputPending);
#endif
		return;
	}

	/* for pass 10, j is the next line above i that will be used */
	for (j=first; na[j] <= 0; j++)
		;
	
	/* Pass 10: insert and fix remaining lines */
	for (i=first; i<=last; i++) {
		if (j <= i)
			for (j++; na[j] <= 0; j++)
				;
#ifdef DEBUG
		if(outf) {
			fprintf(outf, "i %d, j %d, na[i] %d, na[j] %d\n",
				i, j, na[i], na[j]);
			if (na[i] > 0 && na[i] != i)
				fprintf(outf, "OOPS: na[%d] is %d\n", i, na[i]);
		}
#endif
		/* There are two cases: na[i] <= 0  or na[i] == i */
		if (na[i] <= 0) {
			/*
			 * This line must be fixed from scratch.  First
			 * check to see if the line physically there is
			 * going to be used later, if so, move it down.
			 */
			if (na[j]==i || ndel+i>last && last<SP->des_bot_mgn+1) {
				_pos(i-1, 0);
				n = j - i;
				idl_offset -= n;
				ndel -= n;
				_inslines(n);
				_chk_typeahead();
				if (idl_offset == 0 && InputPending) {
#ifdef DEBUG
					if (outf) fprintf(outf, "InputPending %d, aborted after dellines\n", InputPending);
#endif
					return;
				}
				for (k=last; k>=j; k--) {
					if (na[k] > 0)
						na[k] += n;
					_line_free(SP->cur_body[k]);
					SP->cur_body[k] = SP->cur_body[k-n];
					oa[k] = oa[k-n];
				}
				for ( ; k>=i; k--) {
					if (na[k] > 0)
						na[k] += n;
					_line_free(SP->cur_body[k]);
					SP->cur_body[k] = NULL;
					oa[k] = 0;
				}
			}
		}
		/* Now transform line i to new line j */
		if (!InputPending) {
			_id_char (SP->cur_body[i], SP->std_body[i], i-1);
			if (SP->cur_body[i] != SP->std_body[i])
				_line_free (SP->cur_body[i]);
			SP->cur_body[i] = SP->std_body[i];
		}
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "\ni	oa	na After pass 10: _id_line completed\n");
	for (i=first; i<=last; i++)
		if(outf) fprintf(outf, "%d	%d	%d\n", i, oa[i], na[i]);
	if(outf) fprintf(outf, "\n");
#endif
}

static int
_getst(val, symtab)
register struct st *symtab;
{
	register int i;
	register int h;
	register int hopcount = 256;

	i = val & 0377;
	while ((h=symtab[i].hash) && h != val) {
		if (++i >= 256)
			i = 0;
		if (--hopcount <= 0) {
			_ec_quit("Hash table full in dispcalc", "");
		}
	}
	symtab[i].hash = val;
#ifdef LONGDEBUG
	if(outf) fprintf(outf, "_getst, val %d, init slot %d, return %d\n",
		val, val & 0377, i);
#endif
	return i;
}

/*
 * If it's been long enough, check to see if we have any typeahead
 * waiting.  If so, we quit this update until next time.
 */
_chk_typeahead()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "end of _id_char: --SP->check_input %d, InputPending %d, chars buffered %d: ", SP->check_input-1, InputPending, (SP->term_file->_ptr-SP->term_file->_base));
#endif
	if(--SP->check_input<0 && !InputPending &&
	    ((SP->term_file->_ptr - SP->term_file->_base) > 20)) {
		/* __cflush(); */
		if (SP->check_fd >= 0)
			InputPending = _chk_input();
		else
			InputPending = 0;
		SP->check_input = SP->baud / 2400;
#ifdef DEBUG
		if(outf) fprintf(outf, "flush, ioctl returns %d, SP->check_input set to %d\n", InputPending, SP->check_input);
#endif
	}
#ifdef DEBUG
	if(outf) fprintf(outf, ".\n");
#endif
}
