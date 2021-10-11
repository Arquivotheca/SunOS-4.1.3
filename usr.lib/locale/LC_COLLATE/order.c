/*
 * order 
 */

/* static  char *sccsid = "@(#)order.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

/*
 * setup primary
 */
set_prime(index, val)
	unsigned int index;
	unsigned int val;
{
	char ebuf[64];

	index %= 256;	/* get the module */

	if (colldef.primary_sort[index] != DONT_CARE_P) {
		sprintf(ebuf, "0x%x already appeared in order list(prime)", index);
		warning(ebuf, (char *)0);
	}
	colldef.primary_sort[index] = val;
}

/*
 * setup secondary
 */
set_second(index, val)
	unsigned int index;
	unsigned int val;
{
	char ebuf[64];
	index %= 256;	/* get the module */

	if (colldef.secondary_sort[index] != DONT_CARE_S) {
		sprintf(ebuf, "0x%x already appeared in order list(second)", index);
		warning(ebuf, (char *)0);
	}
	colldef.secondary_sort[index] = val;
}

/*
 * set_comp_atom
 */
set_comp_atom(atom, second)
	struct type_var *atom;
	int second;
{
	struct charmap *map;

	switch (atom->type) {
	case STRING:
		if (strlen(atom->type_val.str_val) >= 2) 
			set_2_to_1(atom->type_val.str_val, prime_val, second);
		else {
			int index = (unsigned char)atom->type_val.str_val[0];
			set_prime(index, prime_val);
			if (second)
				set_second(index, second_val++);
		}
		break;
	case SYM_NAME:
		/*
		 * search charmap table, and get value
		 */
		map = lookup_map(atom->type_val.str_val);
		if (map == (struct charmap *)NULL) {
			warning("literal not defined:", atom->type_val.str_val);
		}
		else {
			set_prime(map->map_val, prime_val);
			if (second)
				set_second(map->map_val, second_val++);
		}
		break;
	case H_NUM:
		set_prime(atom->type_val.num_val, prime_val);
		if (second)
			set_second(atom->type_val.num_val, second_val++);
		break;
	default:
		/*
		 * internal error
		 */
		panic("set_comp_atom, illegal type", atom->type, 0);
		break;
	}
}


/*
 * set 2_to_1 table
 */
set_2_to_1(s, prime, second_flg)
	char *s;	/* character string */
	int prime;	/* primary sort value */
	int second_flg;	/* set secondary or not ? */
{
	char one = s[0];
	char two = s[1];
	int i;

	for (i = 0; i < no_of_2_to_1; i++) {
		if (one == _2_to_1[i].one && two == _2_to_1[i].two) {
			char buf[64];
			/*
			 * Found one, give warning and give new value
			 */
			 sprintf(buf, "2 to 1 multiple defined for %c%c", one, two);
			 warning(buf, (char *)0);
			 _2_to_1[i].mapped_primary = prime;
			 if (second_flg == ON)
				_2_to_1[i].mapped_secondary = second_val++;
			 else
				_2_to_1[i].mapped_secondary = 0;
			return(OK);
		}
	}
	/*
	 * New one
	 */
	if (no_of_2_to_1 >= NUM_2_TO_ONE) {
		/*
		 * If too many, ignore it.
		 */
		warning("too many 2 to 1.", (char *)0);
		return(ERROR);
	}
	 _2_to_1[no_of_2_to_1].one = one;
	 _2_to_1[no_of_2_to_1].two = two;
	 _2_to_1[no_of_2_to_1].mapped_primary = prime;
	 if (second_flg == ON)
		_2_to_1[no_of_2_to_1].mapped_secondary = second_val++;
	 else
		_2_to_1[no_of_2_to_1].mapped_secondary = 0;
	++no_of_2_to_1;
	return(OK);
}
