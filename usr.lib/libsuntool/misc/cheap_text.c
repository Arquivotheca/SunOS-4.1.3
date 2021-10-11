#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)cheap_text.c 1.1 92/07/30";
#endif
#endif

/*
 *  Copyright (c) 1984, 1985, 1986 and 1987 by Sun Microsystems Inc.
 */

/*	cheap_text.c
 *
 *	Quick and dirty text source implementation;
 *	will probably be replaced sometime soon
 */

#include <sys/types.h>
#include <stdio.h>
#include <suntool/cheap_text.h>

typedef struct cheap_text {
	char   *value;
	u_int	length;
	u_int	position;
	u_int	max_length;
}  cheap_text;

extern	text_ops cheap_text_ops;

text_obj *cheap_createtext(max, init)
u_int	max;
char   *init;
{
	text_obj   *op;
	cheap_text *tp;

	op = (text_obj *) malloc(sizeof (text_obj));
	if (op == NULL) {
		/* complain  */
		return NULL;
	}
	tp = (cheap_text *) malloc(sizeof (cheap_text));
	if (tp == NULL)  {
		/*  complain  */
		free(op);
		return NULL;
	}

	tp->value = malloc(max+1);
	if (tp->value == NULL)  {
		/*  complain  */
		free(tp);
		free(op);
		return NULL;
	}
	strncpy(tp->value, init, max);
	tp->value[max] = '\0';
	tp->max_length = max;
	tp->length = strlen(tp->value);
	tp->position = tp->length;

	op->data = (caddr_t) tp;
	op->ops = &cheap_text_ops;

	return op;
}

u_int	  cheap_commit(op)
text_obj  *op;
{
	return 0;
}

text_obj *cheap_destroy(op)
text_obj  *op;
{
	free(((cheap_text *)(op->data))->value);
	free(op->data);
	free(op);
	return NULL;
}

u_int	  cheap_sizeof_entity(op)
text_obj  *op;
{
	return 1;
}

u_int	  cheap_get_length(op)
text_obj  *op;
{
	return ((cheap_text *)(op->data))->length;
}

u_int	  cheap_get_position(op)
text_obj  *op;
{
	return ((cheap_text *)(op->data))->position;
}

u_int	  cheap_set_position(op, pos)
text_obj  *op;
u_int	   pos;
{
	register cheap_text *tp;

	tp = (cheap_text *)op->data;
	if (pos > tp->length) {
		pos = tp->length;
	}
	return (tp->position = pos);
}

u_int	  cheap_read(op, len, resultp, bufp)
	 text_obj  *op;
	 u_int	    len,
		   *resultp;
register char	   *bufp;
{
	register char	 c, *srcp;
	register int	 cnt;
	register cheap_text *tp;

	tp = (cheap_text *)op->data;
	srcp = tp->value + tp->position;
	if (tp->length - tp->position < len)  {
		len = tp->length - tp->position;
	}
	cnt = len;
	while (*bufp++ = *srcp++) {
		if (--cnt == 0) {
			*bufp = '\0';
			break;
		}
	}
	*resultp = len;
	return (tp->position += len);
}

u_int	  cheap_replace(op, end, new_len, resultp, new)
text_obj  *op;
u_int	   end,
	   new_len,
	  *resultp;
char	  *new;
{
	register cheap_text  *tp;
	int		      old_len, delta, saved, tmp;
	char		     *start, *keep, *restore,
			      buffer[CHP_TXTSIZE];

	if (new == NULL && new_len != 0)  {
		return 0xFFFFFFFF;
	}
	tp = (cheap_text *)op->data;
	if (end > tp->length) {
		end = tp->length;
	} else if (end < tp->position) {
		tmp = end;
		end = tp->position;
		tp->position = tmp;
	}
	old_len = end - tp->position;
	delta   = new_len -  old_len;

	if (tp->length + delta > tp->max_length) {
		delta   = tp->max_length - tp->length;
		new_len = old_len + delta;
	}
	start = tp->value + tp->position;
	keep  = tp->value + end;
	restore = start + new_len;
	if (delta != 0) {
		bcopy(keep, restore, tp->length - end + 1);
	}
	if (new_len > 0) {
		bcopy(new, start, new_len);
	}
	tp->position = end + delta;
	tp->length += delta;
	tp->value[tp->length] = '\0';
	*resultp = new_len;
	return restore - tp->value;
}

static u_int	cheap_bkchar(),
		cheap_bkword(),
		cheap_bkline(),
		cheap_fwdchar(),
		cheap_fwdword(),
		cheap_fwdline();

u_int
cheap_edit(op, code)
text_obj *op;
u_int	  code;
{
	if (code >= TXT_EDITCHARS) {
		return 0xFFFFFFFF;
	}
	switch (code)  {
	  case TXT_BKCHAR:	cheap_bkchar(op);
				break;
	  case TXT_BKWORD:	cheap_bkword(op);
				break;
	  case TXT_BKLINE:	cheap_bkline(op);
				break;
	  case TXT_FWDCHAR:	cheap_fwdchar(op);
				break;
	  case TXT_FWDWORD:	cheap_fwdword(op);
				break;
	  case TXT_FWDLINE:	cheap_fwdline(op);
				break;
	}
	return 0;
}

static u_int
cheap_bkchar(op)
text_obj *op;
{
	int	result;

	cheap_replace(op, cheap_get_position(op) - 1, 0, &result, NULL);
}

static u_int
cheap_bkword(op)
text_obj *op;
{
	int	result;

	cheap_replace(op, cheap_get_position(op) - 1, 0, &result, NULL);
}

static u_int
cheap_bkline(op)
text_obj *op;
{
	int	result;

	cheap_replace(op, 0, 0, &result, NULL);
}

static u_int
cheap_fwdchar(op)
text_obj *op;
{
}

static u_int
cheap_fwdword(op)
text_obj *op;
{
}

static u_int
cheap_fwdline(op)
text_obj *op;
{
}

extern
cheap_dump(fd, op)
int 	    fd;
cheap_text *op;
{
	fprintf(fd, "\nmax length:  %ld", op->max_length);
	fprintf(fd, "\ncurrent length:  %ld", op->length);
	fprintf(fd, "\nposition:  %ld", op->position);
	fprintf(fd, "\nvalue (%lx):  \"%s\"",
		op->value, (op->value ? op->value : "") );
}
