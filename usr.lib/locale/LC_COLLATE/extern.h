/*
 * external symbold
 */

/* static  char *sccsid = "@(#)extern.h 1.1 92/07/30 SMI"; */

extern int lineno;	/* keeping lineno */
extern FILE *ifp;	/* source file */

extern struct colldef colldef;
extern struct _1_to_m _1_to_m[];
extern struct _2_to_1 _2_to_1[];
extern struct charmap *charmap_head;
extern int no_of_1_to_m;
extern int no_of_2_to_1;
extern int prime_val;
extern int second_val;
