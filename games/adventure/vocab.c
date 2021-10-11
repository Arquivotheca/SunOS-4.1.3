#ifndef lint
static	char sccsid[] = "@(#)vocab.c 1.1 92/07/30 SMI"; /* from UCB 4.1 82/05/11 */
#endif
#
/*      Re-coding of advent in C: data structure routines               */

# include "hdr.h"

dstroy(object)
int object;
{       move(object,0);
}

juggle(object)
int object;
{       register int i,j;
	i=place[object];
	j=fixed[object];
	move(object,i);
	move(object+100,j);
}


move(object,where)
int object,where;
{       register int from;
	if (object<=100)
		from=place[object];
	else
		from=fixed[object-100];
	if (from>0 && from<=300) carry(object,from);
	drop(object,where);
}


put(object,where,pval)
int object,where,pval;
{       move(object,where);
	return(-1-pval);
}



carry(object,where)
int object,where;
{       register int temp;
	if (object<=100)
	{       if (place[object]== -1) return;
		place[object] = -1;
		holdng++;
	}
	if (atloc[where]==object)
	{       atloc[where]=link[object];
		return;
	}
	for (temp=atloc[where]; link[temp]!=object; temp=link[temp]);
	link[temp]=link[object];
}




drop(object,where)
int object,where;
{	if (object>100) fixed[object-100]=where;
	else
	{       if (place[object]== -1) holdng--;
		place[object]=where;
	}
	if (where<=0) return;
	link[object]=atloc[where];
	atloc[where]=object;
}


vocab(word,type,value)                  /* look up or store a word      */
char *word;
int type;       /* -2 for store, -1 for user word, >=0 for canned lookup*/
int value;                              /* used for storing only        */
{       register int adr;
	register char *s,*t;
	int hash, i;
	struct hashtab *h;
	for (hash=0,s=word,i=0; i<5 &&*s; i++)  /* some kind of hash    */
		hash += *s++;           /* add all chars in the word    */
	hash = (hash*3719)&077777;      /* pulled that one out of a hat */
	hash %= HTSIZE;                 /* put it into range of table   */

	for(adr=hash;; adr++)           /* look for entry in table      */
	{       if (adr==HTSIZE) adr=0; /* wrap around                  */
		h = &voc[adr];          /* point at the entry           */
		switch(type)
		{   case -2:            /* fill in entry                */
			if (h->val)     /* already got an entry?        */
				goto exitloop2;
			h->val=value;
			h->atab=malloc(length(word));
			for (s=word,t=h->atab; *s;)
				*t++ = *s++ ^ '=';
			*t=0^'=';
			/* encrypt slightly to thwart core reader       */
		/*      printf("Stored \"%s\" (%d ch) as entry %d\n",   */
		/*              word, length(word), adr);               */
			return(0);      /* entry unused                 */
		    case -1:            /* looking up user word         */
			if (h->val==0) return(-1);   /* not found    */
			for (s=word, t=h->atab;*t ^ '=';)
				if ((*s++ ^ '=') != *t++)
					goto exitloop2;
			if ((*s ^ '=') != *t && s-word<5) goto exitloop2;
			/* the word matched o.k.                        */
			return(h->val);
		    default:            /* looking up known word        */
			if (h->val==0)
			{       printf("Unable to find %s in vocab\n",word);
				exit(0);
			}
			for (s=word, t=h->atab;*t ^ '=';)
				if ((*s++ ^ '=') != *t++) goto exitloop2;
			/* the word matched o.k.                        */
			if (h->val/1000 != type) continue;
			return(h->val%1000);
		}

	    exitloop2:                  /* hashed entry does not match  */
		if (adr+1==hash || (adr==HTSIZE && hash==0))
		{       printf("Hash table overflow\n");
			exit(0);
		}
	}
}


copystr(w1,w2)                          /* copy one string to another   */
char *w1,*w2;
{       register char *s,*t;
	for (s=w1,t=w2; *s;)
		*t++ = *s++;
	*t=0;
}

weq(w1,w2)                              /* compare words                */
char *w1,*w2;                           /* w1 is user, w2 is system     */
{       register char *s,*t;
	register int i;
	s=w1;
	t=w2;
	for (i=0; i<5; i++)             /* compare at most 5 chars      */
	{       if (*t==0 && *s==0)
			return(TRUE);
		if (*s++ != *t++) return(FALSE);
	}
	return(TRUE);
}


length(str)                             /* includes 0 at end            */
char *str;
{       register char *s;
	register int n;
	for (n=0,s=str; *s++;) n++;
	return(n+1);
}

prht()                                  /* print hash table             */
{       register int i,j,l;
	char *c;
	struct hashtab *h;
	for (i=0; i<HTSIZE/10+1; i++)
	{       printf("%4d",i*10);
		for (j=0; j<10; j++)
		{       if (i*10+j>=HTSIZE) break;
			h= &voc[i*10+j];
			putchar(' ');
			if (h->val==0)
			{       printf("-----");
				continue;
			}
			for (l=0, c=h->atab; l<5; l++)
				if ((*c ^ '=')) putchar(*c++ ^ '=');
				else putchar(' ');
		}
		putchar('\n');
	}
}

