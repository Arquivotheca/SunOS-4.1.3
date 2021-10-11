#ident  "@(#)module_conf.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>

/*
 * Eventually, when we want to edit all our kernel
 * configuration files, we can make the various
 * bits of code here for each module type #ifdef'd
 * on a symbol from the config file.
 */
 
/************************************************************************
 *		add extern declarations for module drivers here
 ************************************************************************/

extern void	ross_module_setup();
extern void	vik_module_setup();

/************************************************************************
 *		module driver table
 ************************************************************************/

struct module_linkage module_info[] = {
	{ 0xF0000000, 0x10000000, ross_module_setup },
	{ 0xF0000000, 0x00000000, vik_module_setup },

/************************************************************************
 *		add module driver entries here
 ************************************************************************/
};
int	module_info_size = sizeof module_info / sizeof module_info[0];
