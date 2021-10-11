/*
 * colldef grammer
 */

/* static  char *sccsid = "@(#)colldef.y 1.1 92/07/30 SMI"; */

%{
#include "colldef.h"
#include "extern.h"

#define INIT	0
#define ATOM	1
#define COMP_ATOM	2
#define GOT_ELLIP	3

int order_state = INIT;
int secondary_state = OFF;
int comp_atom = OFF;
%}
	
%union {              
	unsigned int  val;      /* value itself */
	char *id;     		/* pointer to idname */
	struct type_var *var;	/* type holder */
}


%token <val> CHARMAP	/* character mapping */
%token <val> SUBSTITUTE	/* substitute macro */
%token <val> WITH	/* substitute with */
%token <val> ORDER	/* order */
%token <val> ELLIPSIS	/* sllipsis symbol */

%token <id> QSTRING	/* quoted character string */
%token <id> SYM_NAME	/* symbol name */
%token <id> STRING	/* string, but not symbol name */
%token <val> H_NUM	/* hexical number */
%token <val> O_NUM	/* octal number */

%type <id> string
%type <var> subval
%type <var> atom, coll_element
%type <val> number

%%
script  : NLS charmap subs order
	{
		return(0);
	}
	| charmap subs order
	{
		return(0);
	}
	| error	{printf("bad syntax\n"); }
	;
charmap : /* null */
	| CHARMAP string NLS
	{
		/*
		 * set up charmap table
		 */
		setup_charmap($2);
	}
	;
string	: QSTRING
	{ $$ = strsave($1); }
	| STRING
	{ $$ = strsave($1); }
	| SYM_NAME
	{ $$ = strsave($1); }
	;
subs	: /* null */
	| substitutes
	;
substitutes : substitute 
	| substitutes substitute 
	;
substitute : SUBSTITUTE subval WITH subval NLS
	{
		/*
		 * setup 1_to_m table
		 */
		set_1_to_m($2, $4);
		free_type_var($2);
		free_type_var($4);
	}
	| error NLS
	;
subval	: string
	{
		$$ = TYPE_VARmalloc();
		$$->type = STRING;
		$$->type_val.str_val = $1;
	}
	; 
number	: H_NUM
	{ $$ = $1; }
	| O_NUM
	{ $$ = $1; }
	;
order	: ORDER primaries NLS
	;
primaries : primary
	| primaries ';' primary
	;
primary	: coll_element
	| ELLIPSIS
	{
		if (order_state != ATOM) {
			warning("... not following atom.");
			order_state = INIT;
		}
		else {
			/*
			 * set the state
			 */
			order_state = GOT_ELLIP;
		}
	}
	| error  ';'
	;
coll_element : atom
	{
		static int beg_index = 0;
		int i;
		int my_val;
		int my_type = $1->type;
		struct charmap *map;
		int dont_ellip = OFF;
		int error = OFF;

		switch (my_type) {
		case SYM_NAME:
			map = lookup_map($1->type_val.str_val);
			if (map == (struct charmap *)NULL) {
				warning("undefined literal:", 
					$1->type_val.str_val);
				error = dont_ellip = ON;
			}
			else 
				my_val = map->map_val;
			break;
		case H_NUM:
			my_val = $1->type_val.num_val;
			break;
		case STRING:
			if (strlen($1->type_val.str_val) >= 2)
				dont_ellip = ON;
			else
				my_val = (unsigned char)$1->type_val.str_val[0];
			break;
		default:
			panic("Illegal type in coll_element: atom", my_type, 0);
			break;
		}

		/*
		 * set up table 
		 */
		switch (order_state) {
		case GOT_ELLIP:
			if (dont_ellip == OFF) {
				if (my_val < beg_index) {
					warning("ELLIP, large;...;small");
					goto out;
				}
				colldef.primary_sort[(unsigned int)beg_index%256] = DONT_CARE_P;
				for (i = beg_index; i <= my_val; i++) {
					set_prime(i, prime_val++);
				}
			}
out:			order_state = INIT;
			break;
		case ATOM:
		case INIT:
			if (dont_ellip == OFF) {
				order_state = ON;
				beg_index = my_val;
			}
			else 
				order_state = INIT;
			if (my_type == H_NUM)
				set_prime(my_val, prime_val++);
			else if (my_type == SYM_NAME && error == OFF)
				set_prime(my_val, prime_val++);
			else if (my_type == STRING) {
				if (strlen($1->type_val.str_val) >= 2)
					set_2_to_1($1->type_val.str_val, 
						   prime_val++, 0);
				else
					set_prime(my_val, prime_val++);
			}
			break;
		default:
			panic("illegal order_state", 0, 0);
			break;
		}

		/*
		 * Free this atom
		 */
		free_type_var($1);
	}
	| comp_atom
	{
		/*
		 * check state
		 */
		if (order_state == GOT_ELLIP)
			warning("... followed by illegal token");
		order_state = INIT;
	}
	;
comp_atom: '(' { comp_atom = ON; secondary_state = ON; } atoms ')'
	{
		secondary_state = OFF;
		comp_atom = OFF;
		prime_val++;
	}
	| '{' { comp_atom = ON; } atoms '}'
	{
		comp_atom = OFF;
		prime_val++;
	}
	;
atom	: number
	{
		$$ = TYPE_VARmalloc();
		$$->type = H_NUM;
		$$->type_val.num_val = $1;
		if (comp_atom) {
			set_comp_atom($$, secondary_state);
			free_type_var($$);
		}
	}
	| SYM_NAME
	{
		$$ = TYPE_VARmalloc();
		$$->type = STRING;
		$$->type_val.str_val = strsave($1);
		if (comp_atom) {
			set_comp_atom($$, secondary_state);
			free_type_var($$);
		}
	}
	| '<' SYM_NAME '>'
	{
		$$ = TYPE_VARmalloc();
		$$->type = SYM_NAME;
		$$->type_val.str_val = strsave($2);
		if (comp_atom) {
			set_comp_atom($$, secondary_state);
			free_type_var($$);
		}
	}
	;
atoms	: atom
	| atoms ',' atom
	;
NLS	: '\n'
	| NLS '\n'
