/*	@(#)inet.h 1.1 92/07/30 SMI; from UCB 5.1 5/30/85	*/
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * External definitions for
 * functions in inet(3N)
 */

#ifndef _arpa_inet_h
#define _arpa_inet_h

unsigned long inet_addr();
char	*inet_ntoa();
struct	in_addr inet_makeaddr();
unsigned long inet_network();

#endif /*!_arpa_inet_h*/
