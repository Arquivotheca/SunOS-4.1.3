/* @(#)faketest.h	1.1 8/6/90 SMI */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */
#include <sys/types.h>
#include <mon/sunromvec.h>
#include <sun/openprom.h>

#define putchar(c) 		(*romp->v_putchar)(c)
#define open(str)  		(*romp->v_open)(str)
#define close(str)  		(*romp->v_close)(fd)
#define readblk(f,n,s,v) 	(*romp->v_read_blocks)((f), (n), (s), (v))
#define writeblk(f,n,s,v) 	(*romp->v_write_blocks)((f), (n), (s), (v))
#define printf			(*romp->v_printf)
#define NEXT                    (*romp->v_config_ops->devr_next)
#define CHILD                   (*romp->v_config_ops->devr_child)
#define getprop                 (*romp->v_config_ops->devr_getprop)
#define getproplen              (*romp->v_config_ops->devr_getproplen)
#define getlongprop             (*romp->v_config_ops->devr_getprop)
#define nextprop             	(caddr_t)(*romp->v_config_ops->devr_nextprop)
#define setprop		        (*romp->v_config_ops->devr_setprop)

#define XDRBOOL 0
#define XDRINT	1
#define XDRSTRING 2
