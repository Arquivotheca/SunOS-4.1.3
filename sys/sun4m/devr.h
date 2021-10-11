/* @(#)devr.h 1.1 92/07/30 SMI */

/*
 * Copyright 1988, Sun Microsystems,  Inc.
 */

#ifndef	_sun4c_devr_h
#define	_sun4c_devr_h

/*
 * This structure defines the PROM-kernel interface for autoconfiguration
 * stuff. The PROM passes a pointer to this struct to UNIX via the romvec.
 * The routines are used as follows:
 *
 * devr_next(int nodeid) - if 'nodeid' = 0, returns ID of top level node.
 *	Otherwise returns ID of next sibling of 'nodeid'; 0 if none exists.
 *
 * devr_child(int parentid) - returns ID of first child of 'nodeid'; 0 if
 *	none exists.
 *
 * devr_getproplen(int nodeid, char *name) - returns the size in bytes of
 *	the value of 'name' in the property list for 'nodeid'; 0 if 'name'
 *	is not found.
 *
 * devr_getprop(int nodeid, char *name, addr_t buf) - copies the value of
 *	'name' in the property list for 'nodeid' into 'buf'. Returns the
 *	size in bytes of the value, or 0 if 'name' is not found. This call
 *	assumes sufficient space has been allocated in 'buf' for the value.
 */
struct dev_romops {
	int (*devr_next)();		/* get next sibling */
	int (*devr_child)();		/* get first child */
	int (*devr_getproplen)();	/* get value length */
	int (*devr_getprop)();		/* get value */
};

#endif /* !_sun4c_devr_h */
