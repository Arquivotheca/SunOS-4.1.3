/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)runtime.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

/*	ctrace - C program debugging tool
 *
 *	run-time package of trace functions
 *
 */
#include <varargs.h>

#ifndef B_CT_

/* signal catching function used by u_ct_ */
static jmp_buf sj_ct_;
static
f_ct_()
{
	longjmp(sj_ct_, 1);
}
#endif

#ifdef LM_CT_
#define I_CT_(x)	((x + LM_CT_) % LM_CT_)

/* global data used by loop detection code */
static	int	ts_ct_;	/* trace state */
#endif

/* global data used by duplicate variable trace code */
static	int	vc_ct_;	/* var trace count within statement */

static	struct	{	/* var values within statement */
	char	*vn_ct_;	/* var name */
	int	vt_ct_;		/* var type (0 is string, > 0 is size) */
	union	{		/* var value */
		char	*vs_ct_;/* string */
		int	vi_ct_;	/* int */
		long	vl_ct_;	/* long */
		double	vd_ct_;	/* double */
	} vv_ct_;
} v_ct_[VM_CT_];

/* trace on/off control */
static	int	tr_ct_ = 1;

static
ctron()
{
	tr_ct_ = 1;
	PF_CT_" \b\n    /* trace on */ \b");
}

static
ctroff()
{
	tr_ct_ = 0;
	PF_CT_" \b\n    /* trace off */ \b");
}
/* print the statement text */

static
t_ct_(text)
register char	*text;
{
#ifdef LM_CT_
	static	int	loop_start, next_stmt;
	static	char	*stmt[LM_CT_];
	static	long	loops;
	register int	i;
	register char	*s;
	register char	c;
#endif

	/* return if tracing is off */
	if (!tr_ct_) {
		return;
	}
#ifdef LM_CT_
	if (ts_ct_ == 2)	/* if not tracing */
		if (strcmp(text, stmt[next_stmt]) == 0) {
			if (strcmp(text, stmt[loop_start]) == 0) {
				++loops;
				if (loops % 1000 == 0)
					PF_CT_" \b\n    /* still repeating after %ld times */ \b", loops);
				next_stmt = loop_start;
			}
			next_stmt = I_CT_(next_stmt + 1);
			vc_ct_ = 0;	/* reset the var count */
			return;
		}
		else {	/* doesn't match next statement */
			if (loops == 0)
				PF_CT_" \b\n    /* repeated < 1 time */ \b");
			else
				PF_CT_" \b\n    /* repeated %ld times */ \b", loops);
			loops = 0;
			PF_CT_" \b%s \b", stmt[I_CT_(next_stmt - 1)]); /* print last statement */
			ts_ct_ = 4;	/* force var printing */
			for (i = 0; i < vc_ct_; ++i) /* print its vars */
				if (v_ct_[i].vt_ct_ == 0) /* string? */
					s_ct_(v_ct_[i].vn_ct_, v_ct_[i].vv_ct_.vs_ct_); /* yes */
				else if (v_ct_[i].vt_ct_ == sizeof(int))
					u_ct_(v_ct_[i].vn_ct_, v_ct_[i].vt_ct_, v_ct_[i].vv_ct_.vi_ct_);
				else if (v_ct_[i].vt_ct_ == sizeof(long))
					u_ct_(v_ct_[i].vn_ct_, v_ct_[i].vt_ct_, v_ct_[i].vv_ct_.vl_ct_);
				else	/* double */
					u_ct_(v_ct_[i].vn_ct_, v_ct_[i].vt_ct_, v_ct_[i].vv_ct_.vd_ct_);
			ts_ct_ = 0;	/* start tracing */
		}
#endif
	vc_ct_ = 0;	/* reset the var count */

#ifdef LM_CT_
	if (ts_ct_ == 0) {	/* if looking for the start of a loop */
		/* if statement in list */
		for (i = I_CT_(next_stmt - 2); i != I_CT_(next_stmt - 1); i = I_CT_(i - 1))
			if ((s = stmt[i]) != 0 &&	/* saved text could be null */
			    strcmp(text, s) == 0 &&	/* if text matches */
			    (c = s[strlen(s) - 1]) != '{' && c != '}') { /* and is not a brace */
				ts_ct_ = 1;	/* look for the loop end */
				loop_start = i;
				next_stmt = I_CT_(loop_start + 1);
				goto print_stmt;
			}
	}
	else	/* if looking for the loop end */
		if (strcmp(text, stmt[loop_start]) == 0) { /* if start stmt */
			ts_ct_ = 2;			/* stop tracing */
			PF_CT_" \b\n    /* repeating */ \b");
			stmt[next_stmt] = text;	/* save as end marker */
			next_stmt = I_CT_(loop_start + 1);
			return;
		}
		else if (strcmp(text, stmt[next_stmt]) != 0) /* if not next stmt */
			ts_ct_ = 0; /* look for the start of a loop */
	stmt[next_stmt] = text;	/* save this statement */
	next_stmt = I_CT_(next_stmt + 1); /* inc the pointer */
print_stmt:
#endif
	PF_CT_" \b%s \b", text); /* print this statement */
#ifndef B_CT_
	fflush(stdout);		/* flush the output buffer */
#endif
}
/* dump a string variable */

static
s_ct_(name, value)
register char	*name;
register char	*value;
{
	/* return if tracing is off */
	if (!tr_ct_) {
		return;
	}
#ifdef LM_CT_
	/* save the var name and value */
	if (ts_ct_ != 4) {	/* if not forcing var printing */
		v_ct_[vc_ct_].vn_ct_ = name;
		v_ct_[vc_ct_].vt_ct_ = 0;	/* var type is string */
		v_ct_[vc_ct_].vv_ct_.vs_ct_ = value;
		++vc_ct_;
	}

	if (ts_ct_ == 2)	/* if not tracing */
		return;
#endif

	PF_CT_" \b\n    %s == \"", name);
	/* flush before printing the string because it may cause an
	   abort if it is not null terminated */
#ifndef B_CT_
	fflush(stdout);
#endif
	PF_CT_"%s\" */ \b", value);
#ifndef B_CT_
	fflush(stdout);
#endif
}
/* dump a variable of an unknown type */

static
u_ct_(va_alist)
va_dcl
{
#ifndef isprint
#include <ctype.h>
#endif
	va_list ap;
	register int	i;
	register char	*s;
	register char	c;
	register char	*name;
	register int	_size;		/* size is a macro in <macros.h> */
	union {
		char	*p;
		int	i;
		long	l;
		double	d;
	} value;

	va_start(ap);
	name = va_arg(ap, char *);
	_size = va_arg(ap, int);

	/* return if tracing is off */
	if (!tr_ct_) {
		return;
	}
	/* normalize the size (pointer and float are the same size as either int or long) */
	if (_size == sizeof(char) || _size == sizeof(short))
		_size = sizeof(int);
	else if (_size != sizeof(int) && _size != sizeof(long) && _size != sizeof(double))
		/* this is an extern pointer (size=0), or array or struct address */
		_size = sizeof(char *);

	/* assign the union depending on size */
	if (_size == sizeof(int))
		value.i = va_arg(ap, int);

	else if (_size == sizeof(double))
		value.d = va_arg(ap, double);

	else if (_size == sizeof(long))
		value.l = va_arg(ap, long);

	else if (_size == sizeof(char *))
		value.p = va_arg(ap, char *);

	va_end(ap);

#ifdef LM_CT_
	if (ts_ct_ != 4) {	/* if not forcing var printing */
#endif
		/* don't dump the variable if its value is the same */
		for (i = 0; i < vc_ct_; ++i)
			if (_size == v_ct_[i].vt_ct_ && strcmp(name, v_ct_[i].vn_ct_) == 0)
				if (_size == sizeof(int)) {
					if (value.i == v_ct_[i].vv_ct_.vi_ct_)
						return;
				}
				else if (_size == sizeof(long)) {
					if (value.l == v_ct_[i].vv_ct_.vl_ct_)
						return;
				}
				else /* double */
					if (value.d == v_ct_[i].vv_ct_.vd_ct_)
						return;
	
		/* save the var name and value */
		v_ct_[vc_ct_].vn_ct_ = name;
		v_ct_[vc_ct_].vt_ct_ = _size;
		if (_size == sizeof(int)) {
			v_ct_[vc_ct_].vv_ct_.vi_ct_ = value.i;
		}
		else if (_size == sizeof(long)) {
			v_ct_[vc_ct_].vv_ct_.vl_ct_ = value.l;
		}
		else /* double */
			v_ct_[vc_ct_].vv_ct_.vd_ct_ = value.d;
		++vc_ct_;
#ifdef LM_CT_
	}

	if (ts_ct_ == 2)	/* if not tracing */
		return;
#endif
	/* determine the variable type and print it */
	PF_CT_" \b\n    %s == ", name);
	if (_size == sizeof(int)) {
		PF_CT_"%d", value.i);	/* decimal */
#ifdef O_CT_
		if ((unsigned) value.i > 7)	/* octal */
			PF_CT_" or 0%o", value.i);
#endif
#ifdef X_CT_
		if ((unsigned) value.i > 9)	/* hexadecimal */
			PF_CT_" or 0x%x", value.i);
#endif
#ifdef U_CT_
		if (value.i < 0)		/* unsigned */
			PF_CT_" or %u", value.i);
#endif
#ifdef E_CT_
		if (_size == sizeof(float))	/* float */
			PF_CT_" or %e", value.i);
#endif
		if ((unsigned) value.i <= 255) /* character */
			if (isprint(value.i))
				PF_CT_" or '%c'", value.i);
			else if (iscntrl(value.i)) {
				switch (value.i) {
					case '\n': c = 'n'; break;
					case '\t': c = 't'; break;
					case '\b': c = 'b'; break;
					case '\r': c = 'r'; break;
					case '\f': c = 'f'; break;
					case '\v': c = 'v'; break;
					default:    c = '\0';
				}
				if (c != '\0')
					PF_CT_" or '\\%c'", c);
			}
	}
	else if (_size == sizeof(long)) {
		PF_CT_"%ld", value.l);	/* decimal */
#ifdef O_CT_
		if ((unsigned) value.l > 7)	/* octal */
			PF_CT_" or 0%lo", value.l);
#endif
#ifdef X_CT_
	if ((unsigned) value.l > 9)	/* hexadecimal */
			PF_CT_" or 0x%lx", value.l);
#endif
#ifdef U_CT_
		if (value.l < 0)		/* unsigned */
			PF_CT_" or %lu", value.l);
#endif
#ifdef E_CT_
		if (_size == sizeof(float))	/* float */
			PF_CT_" or %e", value.l);
#endif
	}
	else if (_size == sizeof(double))	/* double */
		PF_CT_"%e", value.d);
#ifndef B_CT_
	/* check for a possible non-null pointer */
	if (_size == sizeof(char *) && value.p != 0) {
		void	(*sigbus)(), (*sigsegv)();

		/* see if this is a non-null string */
		if (setjmp(sj_ct_) == 0) {
			sigbus = signal(SIGBUS, f_ct_);
			sigsegv = signal(SIGSEGV, f_ct_);
			if (*value.p != '\0')
				for (s = value.p; ; ++s) {
					if ((c = *s) == '\0') {
						PF_CT_" or \"%s\"", value.p);
						break;
					}
					/* don't use isspace(3) because \v and others will not print properly */
					if (!isprint(c) && c != '\t' && c != '\n')
						break;	/* not string */
				}
		}
		signal(SIGBUS, sigbus);
		signal(SIGSEGV, sigsegv);
	}
#endif
	PF_CT_" */ \b");
#ifndef B_CT_
	fflush(stdout);
#endif
}
