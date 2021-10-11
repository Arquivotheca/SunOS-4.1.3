#ifndef lint
static char sccsid[] = "@(#)openprom.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Openprom eeprom options/devinfo driver.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <mon/obpdefs.h>

#include <sundev/openpromio.h>

static int options_id = 0;
static int current_id = 0;

extern addr_t	getlongprop();
extern dnode_t	prom_nextnode(), prom_childnode();
extern int	prom_getprop();
extern caddr_t  prom_nextprop();

extern void	devi_to_path();

/*
#define	OPENPROMDEBUG
*/

/*
 * On first open, find the options node id.
 */
opromopen(dev)
	dev_t dev;
{
	register dnode_t i;
	register dnode_t root_id;
	static char *options_name = "options";
	static char *name = "name";
	register char *namebuf;
	register int namesize;
	register int s;

	if (minor(dev) != 0)
	    return (EINVAL);
	if (options_id != 0)
	    return (0);
	/*
	 * options node must be child of root device configuration
	 */
	s = splhigh();
	root_id = prom_nextnode((dnode_t)0);
	for (i = prom_childnode(root_id); i != 0; i = prom_nextnode(i)) {
		namesize = getproplen((int) i, name);
		if (namesize <= 0)	/* Hmmm, should have a name */
		    continue;
		namebuf = (char *)getlongprop((int) i, name);
		if (namebuf == NULL) {	/* Hmmm, no memory? */
		    (void)splx(s);
		    return (ENOMEM);
		}
		if (strcmp(namebuf, options_name) == 0) {
		    options_id = (int)i;
		    kmem_free((caddr_t) namebuf, (u_int) namesize);
		    break;
		}
		kmem_free((caddr_t) namebuf, (u_int) namesize);
	}
	(void)splx(s);
	if (options_id == 0)
	    return (ENOTTY);
	return (0);
}

/*ARGSUSED*/
opromioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd, flag;
	caddr_t *data;		/* yes, it really is! */
{
	register struct openpromio *opp;
	register int valsize;
	register int putsize;
	register char *valbuf;
	register int error;
	register int s;
	u_int userbufsize;
	int node_id;
	caddr_t	getcons_ptr;

	switch (cmd) {
	case OPROMGETOPT:
	case OPROMNXTOPT:
		if ((flag & _FREAD) == 0) {
#ifdef OPENPROMDEBUG
			printf("opromioctl: incorrect file permission 0x%x\n",
			    flag);
#endif OPENPROMDEBUG
			return (EBADF);
		}
		node_id = options_id;
		break;
	case OPROMSETOPT:
	case OPROMSETOPT2:
		if ((flag & _FWRITE) == 0) {
#ifdef OPENPROMDEBUG
			printf("opromioctl: incorrect file permission 0x%x\n",
			    flag);
#endif OPENPROMDEBUG
			return (EBADF);
		}
		node_id = options_id;
		break;
	case OPROMNEXT:
	case OPROMCHILD:
	case OPROMGETPROP:
	case OPROMNXTPROP:
		if ((flag & _FREAD) == 0) {
#ifdef OPENPROMDEBUG
			printf("opromioctl: incorrect file permission 0x%x\n",
			    flag);
#endif OPENPROMDEBUG
			return (EBADF);
		}
		node_id = current_id;
		break;
	case OPROMU2P:
		s = splhigh();
		if (prom_getversion() == 0) {
			(void)splx(s);
			return (EINVAL);
		}
		(void)splx(s);
		break;
	case OPROMGETCONS:	/* replace v_insource/outsink prom entries */
		if ((flag & _FREAD) == 0) {
#ifdef OPENPROMDEBUG
			printf("opromioctl: incorrect file permission 0x%x\n",
			    flag);
#endif OPENPROMDEBUG
			return (EBADF);
		}
		break;
	default:
#ifdef	OPENPROMDEBUG
		printf("opromioctl: invalid command 0x%x\n", cmd);
#endif	OPENPROMDEBUG
		return (EINVAL);
	}

	error = copyin(*data, (caddr_t) &userbufsize, sizeof (u_int));
	if (error) {
#ifdef	OPENPROMDEBUG
		printf("opromioctl: error reading oprom_size\n");
#endif	OPENPROMDEBUG
		return (error);
	}
	if (userbufsize == 0 || userbufsize > OPROMMAXPARAM) {
#ifdef	OPENPROMDEBUG
		printf("opromioctl: illegal oprom_size %d (0x%x)\n",
		    userbufsize, userbufsize);
#endif	OPENPROMDEBUG
		return (EINVAL);
	}
	opp = (struct openpromio *) new_kmem_zalloc(
	    userbufsize + sizeof (u_int) + 1, KMEM_SLEEP);
	if (opp == 0) {
#ifdef	OPENPROMDEBUG
		printf("opromioctl: couldn't allocate oprom_array\n");
#endif	OPENPROMDEBUG
		return (ENOMEM);
	}
	error = copyin(*data + sizeof (u_int), opp->oprom_array, userbufsize);
	if (error) {
#ifdef	OPENPROMDEBUG
		printf("opromioctl: error reading oprom_array\n");
#endif	OPENPROMDEBUG
		goto done;
	}

	switch (cmd) {
	case OPROMGETOPT:
	case OPROMGETPROP:
		if (node_id == 0) {
			error = EINVAL;
			goto done;
		}
		s = splhigh();
		valsize = prom_getproplen((dnode_t)node_id, opp->oprom_array);
		(void)splx(s);
		if (valsize > 0 && valsize <= userbufsize) {
#ifdef	OPENPROMDEBUG
			printf("opget:property length %d for name '%s'\n",
				valsize, opp->oprom_array);
#endif	OPENPROMDEBUG
			valbuf = (char *)new_kmem_zalloc(
				(unsigned)valsize, KMEM_SLEEP);
			s = splhigh();
			(void)prom_getprop((dnode_t)node_id, opp->oprom_array,
					    valbuf);
			(void)splx(s);
			opp->oprom_size = valsize;
			bzero(opp->oprom_array, userbufsize);
			bcopy(valbuf, opp->oprom_array, (u_int) valsize);
			kmem_free(valbuf, (unsigned)valsize);
		}
		error = copyout((char *)opp, *data, userbufsize+sizeof (u_int));
		break;

	    case OPROMSETOPT:
	    case OPROMSETOPT2:
		/*
		 * The property name is the first string, value second
		 */
		valbuf = opp->oprom_array+strlen(opp->oprom_array)+1;
		if (cmd == OPROMSETOPT) {
		    valsize = strlen(valbuf) + 1;
		} else
		    valsize = (opp->oprom_array + userbufsize) - valbuf;
		s = splhigh();
		putsize = prom_setprop((dnode_t)options_id, opp->oprom_array,
					valbuf, valsize);
		(void)splx(s);
		if (putsize < 0)
		    error = EINVAL;
#ifdef	OPENPROMDEBUG
		printf("opset:putsize %d, valsize %d for name '%s', value %s\n",
			putsize, valsize, opp->oprom_array, valbuf);
#endif	OPENPROMDEBUG
		break;

	    case OPROMNXTOPT:
	    case OPROMNXTPROP:
		if (node_id == 0) {
			error = EINVAL;
			goto done;
		}
		s = splhigh();
		valbuf = (char *)prom_nextprop((dnode_t)node_id,
		    opp->oprom_array);
		(void)splx(s);
		valsize = strlen(valbuf);
		if (valsize == 0)
			opp->oprom_size = 0;
		else if (++valsize <= userbufsize) {
#ifdef	OPENPROMDEBUG
		    printf("opnxt:'%s'->'%s' (name length is %d)\n",
			opp->oprom_array, valbuf, valsize);
#endif	OPENPROMDEBUG
			opp->oprom_size = valsize;
			bzero(opp->oprom_array, userbufsize);
			bcopy(valbuf, opp->oprom_array, (u_int) valsize);
		}
		error = copyout((char *)opp, *data,
		    userbufsize + sizeof (u_int));
		break;

	    case OPROMNEXT:
	    case OPROMCHILD:
		s = splhigh();
		if (cmd == OPROMNEXT)
			current_id = (int)prom_nextnode(
			    *(dnode_t *)opp->oprom_array);
		else
			current_id = (int)prom_childnode(
			    *(dnode_t *)opp->oprom_array);
		(void)splx(s);
#ifdef	OPENPROMDEBUG
		printf("opchild:0x%x->0x%x\n", *(int *)opp->oprom_array,
		    current_id);
#endif	OPENPROMDEBUG
		opp->oprom_size = sizeof (int);
		bzero(opp->oprom_array, userbufsize);
		bcopy((caddr_t)&current_id, opp->oprom_array, sizeof (int));
		error = copyout((char *)opp, *data,
		    userbufsize + sizeof (u_int));
		break;

	    case OPROMU2P:
		devi_to_path(opp->oprom_array);
		valsize = strlen(opp->oprom_array);
		opp->oprom_size = valsize;
		error = copyout((char *)opp, *data,
		    userbufsize + sizeof (u_int));
		break;

	    case OPROMGETCONS:	/* replace v_insource/outsink prom entries */
		getcons_ptr = opp->oprom_array;
		bzero(opp->oprom_array, userbufsize);	/* zero out */
		s = splhigh(); /* need for these calls to prom? */
		/*
		 * First int we return replaces v_insource.
		 */
		if (prom_stdout_is_framebuffer()) {
		/* We have Sun console, return 0 */
			getcons_ptr++;
		} else {
		/* return 1 */
			*getcons_ptr++ = 1;
		}
		/*
		 * Second int we return replaces v_outsink.
		 */
		if (prom_stdin_is_keyboard()) {
		/* We have Sun keyboard, return 0 */
			getcons_ptr--;
		} else {
		/* return 1 */
			*getcons_ptr-- = 1;
		}
		(void)splx(s);
		error = copyout((char *)opp, *data, userbufsize+sizeof (u_int));
		break;
	}

done:
	(void) kmem_free((char *) opp, userbufsize + sizeof (u_int) + 1);
	return (error);
}
