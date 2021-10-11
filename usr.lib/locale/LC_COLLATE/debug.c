/*
 * debug routines
 */

/* static  char *sccsid = "@(#)debug.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

/*
 * dump 1_to_m
 */
dump_1_to_m()
{
	int i = 0;

	printf("dump 1_to_many\n");
	for (i = 0; i < no_of_1_to_m; ++i) {
		printf("one = '%c'  ", _1_to_m[i].one);
		printf("many= '%s'\n", _1_to_m[i].many);
	}
}

/*
 * dump 2_to_one
 */
dump_2_to_1()
{
	int i;

	printf("dump 2_to_1\n");
	for (i = 0; i < no_of_2_to_1; ++i) {
		printf("one = '%c'  ", _2_to_1[i].one);
		printf("two = '%c'  ", _2_to_1[i].two);
		printf("1st = '%3d' ", _2_to_1[i].mapped_primary);
		printf("2nd = '%3d'\n ", _2_to_1[i].mapped_secondary);
	}
}


/*
 * dmp primary
 */
dump_table(flag)
	int flag;
{
	int i, j;
	unsigned char *p = &colldef.primary_sort[0];

	if (flag == 2) {
		p = &colldef.secondary_sort[0];
		printf("dump secondary sort\n");
	} 
	else
		printf("dump primary sort\n");
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++)
			printf("%4d ", *p++);
		printf("\n");
	}
}

/*
 * dump charmap
 */

dump_charmap()
{
	struct charmap *map = charmap_head;

	printf("\n dumping charmap table\n");
	while (map != (struct charmap *)NULL) {
		printf("%s, %d\n", map->mapping, map->map_val);
		map = map->next;
	}
}
