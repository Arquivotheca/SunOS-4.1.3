/*
 * charmap 
 */

/* static  char *sccsid = "@(#)charmap.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

unsigned int get_val();

/*
 * setup charmap
 */
setup_charmap(f)
	char *f;
{
	FILE *fp;			/* file to read */
	char buf[BSIZE];
	char string[IDSIZE];
	char num_string[IDSIZE];
	unsigned int val;
	int n;
	char *p;
	int ret = OK;
	struct charmap * map;

	/*
	 * open given file
	 */
	if ((fp = fopen(f, "r")) == NULL) {
		warning("could not open file", f);
		return(ERROR);
	}

	/*
	 * setup charmap structure reading the file
	 */
	while (fgets (buf, BSIZE-1, fp) != NULL) {
		n = sscanf(buf, "%s%s", string, num_string);
		if (n != 2)
			continue;
		
		val = get_val(num_string);
#ifdef DDEBUG
	printf("val = %d, num_string = %s\n", val, num_string);
#endif
		/*
		 * set up charmap tables
		 */
		 map = lookup_map(string);
		 if (map != NULL) {	/* duplicate */
			warning ("duplicate definition in charamap file.", string);
			ret = ERROR;
			continue;
		}
		map = CHARMAPmalloc();
		if (map == (struct charmap *)NULL) {
			panic("Can't allocate charmap.", 0, (char *)0);
			/* NOTREACHED */
		}
		map->mapping = strsave(string);
		map->map_val = val;
		map->next = NULL;
		insert_map(map);

	}

	/*
	 * close the file
	 */
	fclose(fp);
	return(ret);
}

/*
 * get value
 */
unsigned int
get_val(s)
	char *s;
{
	if (*s++ != '\\') {
		warning("charmap file, value in illegal format.", s);
		return(0);
	}
	if (*s == 'x')
		return(axtoi(++s));
	else
		return(aotoi(s));
}

/*
 * insert map table
 */
insert_map(map)
	struct charmap *map;
{
	/*
	 * Insert at the head of the list
	 */
	map->next = charmap_head;
	charmap_head = map;
}

/*
 * look for the given name
 */
struct charmap *
lookup_map(s)
	char *s;
{
	struct charmap *map = charmap_head;

	while (map != (struct charmap *)NULL) {
		if (strcmp(map->mapping, s) == 0)
			return (map);
		map = map->next;
	}
	return(map);		/* returning NULL */
}

