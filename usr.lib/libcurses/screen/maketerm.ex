e caps
set noreadonly
g/^#/d
/--- begin bool/+,/--- end bool/-w bool
/--- begin num/+,/--- end num/-w num
/--- begin str/+,/--- end str/-w str
e! bool
1,$s/"	.*/", /
1,$s/.*	"/"/
1,10j
2,$j
1i
char *boolnames[] = {
.
$a
0
};
.
w boolnames
e bool
1,$s/^[^"]*"[^"]*".//
1,$s/"	.*/",/
1,10j
2,$j
1i
char *boolcodes[] = {
.
$a
0
};
.
w>> boolnames
e! bool
1,$s;"[^"]*"[ 	]*".."	;/* ;
1,$s;$; */;
1,$s;^;	;
1i
    char
.
$a
	lastbool;
.
w boolvals
e num
1,$s/"	.*/", /
1,$s/.*	"/"/
1,$j
1i
char *numnames[] = {
.
$a
0
};
.
w numnames
e num
1,$s/^[^"]*"[^"]*".//
1,$s/"	.*/",/
1,$j
1i
char *numcodes[] = {
.
$a
0
};
.
w>> numnames
e! num
1,$s;"[^"]*"[ 	]*".."	;/* ;
1,$s;$; */;
1,$s;^;	;
1i
    short
.
$a
	lastnum;
.
w numvals
e str
1,$s/"	.*/", /
1,$s/.*	"/"/
1,$-10g/^/.,+9j
+,$j
1i
char *strnames[] = {
.
$a
0
};
.
w strnames
e str
1,$s/^[^"]*"[^"]*".//
1,$s/"	.*/",/
1,$-10g/^/.,+9j
+,$j
1i
char *strcodes[] = {
.
$a
0
};
.
w>> strnames
e! str
1,$s;"[^"]*"[ 	]*".."	;/* ;
1,$s;$; */;
1,$s;^;	strs.;
$a
	laststr;
.
w strvals
1,$d
f capnames.c
r boolnames
r numnames
r strnames
w!
1,$d
f term.h
a
typedef char *charptr;
struct strs {
    charptr
.
r strvals
1,.s/strs\.//
$a
};

struct term {
.
r boolvals
r numvals
a
};
#ifndef NONSTANDARD
extern struct term *cur_term;
#endif
.
1,$s/^	./\U&/
0r boolvals
.r numvals
.r strvals
.ka
1,'av/^	/d
1,'as/^	//
1,.s/	[^	].*/	/
1,.s/,/ /
1,.s/.*/#define &	CUR \u&/
1,.s/strs\.//
1,.s/Strs.\(.\)/strs.\u\1/
1,+g/;/d
a

.
$;?^}?;.i
	struct strs strs;
	struct strs2 strs2;
	short Filedes;		/* file descriptor being written to */
#ifndef NONSTANDARD
	SGTTY Ottyb,		/* original state of the terminal */
	      Nttyb;		/* current state of the terminal */
#endif
.
" clean up unused fields
1,$s/[ 	]*$//
/Laststr/-s/,/;/
/Laststr/d
/Lastbool/-s/,/;/
/Lastbool/d
/Lastnum/-s/,/;/
/Lastnum/d
" split strings into two groups < 100 to fit in ritchie compiler
1;/CUR strs/+100;.,$s/CUR strs/CUR strs2/
/^struct strs/+101s/,/;/
a
};
struct strs2 {
    charptr
.
1i
/*
 * term.h - this file is automatically made from caps and maketerm.ex.
 *
 * Guard against multiple includes.
 */

#ifndef auto_left_margin

.
$a

#endif auto_left_margin
 
#ifdef SINGLE
extern struct term _first_term;
# define CUR	_first_term.
#else
# define CUR	cur_term->
#endif
.
w!
q
