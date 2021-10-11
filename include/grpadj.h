/*      @(#)grpadj.h 1.1 92/07/30 SMI      */ /* c2 secure */

#ifndef _grpadj_h
#define _grpadj_h

struct	group_adjunct { /* see getgraent(3) */
	char	*gra_name;
	char	*gra_passwd;
};

struct group_adjunct *getgraent(), *getgragid(), *getgranam();

#endif /*!_grpadj_h*/
