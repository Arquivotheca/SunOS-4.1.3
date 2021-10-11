/*	@(#)label.h 1.1 92/07/30 SMI	*/

/*
 * label - the security label structure
 */

#ifndef _sys_label_h
#define _sys_label_h

struct binary_label {
	short	sl_level;
	char	sl_categories[16];
	char	sl_unused[14];
};

typedef struct binary_label blabel_t;

#endif /*!_sys_label_h*/
