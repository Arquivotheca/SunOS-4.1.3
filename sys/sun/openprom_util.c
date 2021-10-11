#ident "@(#)openprom_util.c 1.1 92/07/30 SMI"

/*LINTLIBRARY*/

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 * Routines for OBP boot support.
 */

#define	OPENPROMS
#include <mon/obpdefs.h>
#include <sun/openprom.h>
#include <sun/autoconf.h>
#include <scsi/scsi.h>

#ifndef sun4c
/*
 * None of the decoding stuff is used by sun4m, since they
 * have "range" properties available.  c.f. autoconf.c:apply_range()
 * Also, for most devices, a generic encoding function is used
 * that pulls out the reg property from the PROM and uses that as the
 * qualifier.
 */

static int units_are_hex();
#endif	! sun4c

#define	CLRBUF(a, l)			\
{					\
	register int c = l;		\
	register char *p = (char *)a;	\
	while (c-- > 0)			\
		*p++ = 0;		\
}

/*
 * Define DPRINTF and set (both) debug flags to non-zero for debugging output.
 *
 */
#define	DPRINTF	1

#ifdef	DPRINTF
static int memlistdebug = 0;
#define	dprintf		if (memlistdebug) printf
#endif	DPRINTF

struct memlist *
getmemlist(name, prop)
	char *name, *prop;
{
	int nodeid;
	int i, j, k, chunks;
	u_int propsize;
	struct dev_reg *rp;
	struct memlist *pmem;
	static caddr_t bufp = (caddr_t)0;
	static u_int left;
	extern caddr_t prom_alloc();
	extern dnode_t prom_nextnode();

	if (bufp == (caddr_t)0) {
		bufp = prom_alloc((caddr_t)0, (u_int)PAGESIZE); /* XXX */
		left = PAGESIZE;
	}

	/*
	 * First find the right node.
	 */
	nodeid = searchpromtree((int)prom_nextnode((dnode_t)0), name, bufp);
	if (nodeid == 0)
		return ((struct memlist *)0);
#ifdef	DPRINTF
	dprintf("Found %s:%s at node %x\n", name, prop, nodeid);
#endif	DPRINTF

	/*
	 * Allocate memlist elements, one per dev_reg struct.
	 */
	propsize = (u_int)getproplen(nodeid, prop);
	chunks = propsize / sizeof (struct dev_reg);
	if (chunks == 0)
		return ((struct memlist *)0);

	if (left < (chunks * sizeof (struct memlist))) {
		panic("memlists too big");
	}
	pmem = (struct memlist *)bufp;
	CLRBUF(pmem, (u_int) chunks * sizeof (struct memlist));
	left -= (u_int) chunks * sizeof (struct memlist);
	bufp += (u_int) chunks * sizeof (struct memlist);

	if (left < propsize) {
		panic("memlists too big");
	}
	rp = (struct dev_reg *)bufp;
	CLRBUF(rp, propsize);
	left -= propsize;
	bufp += propsize;

	/*
	 * Fill in memlist chunks.
	 */
	(void)prom_getprop((dnode_t)nodeid, prop, (caddr_t) rp);
	for (i = 0; i < chunks; ++i) {
#ifdef	DPRINTF
		dprintf("Chunk %d: addr %x bustype %x size %x\n",
		    i, rp[i].reg_addr, rp[i].reg_bustype, rp[i].reg_size);
#endif	DPRINTF
		if (rp[i].reg_bustype != OBMEM)
			continue;

		/*
		 * Insert the new element into the array of list nodes
		 * so that the array remains sorted in ascending order.
		 */

		/* Find the first element with a larger address */
		for (j = 0; j < i; j++)
			if (pmem[j].address > (u_int)rp[i].reg_addr)
				break;

		/* Copy following elements up to make room for the new one */
		for (k = i; k > j; --k)
			pmem[k] = pmem[k-1];

		/* Insert the new element */
		pmem[j].address = (u_int)rp[i].reg_addr;
		pmem[j].size = rp[i].reg_size;
	}
	for (i = 1; i < chunks; ++i) {
		if (pmem[i].address)
			pmem[i-1].next = &pmem[i];
	}
	return (pmem);
}

searchpromtree(node, name, buf)
	int node;
	char *name;
	char *buf;
{
	int id;
	extern dnode_t prom_nextnode(), prom_childnode();

	*buf = '\0';
	(void)prom_getprop((dnode_t)node, "name", buf);
	if (strcmp(buf, name) == 0) {
		return (node);
	}
	id = (int)prom_nextnode((dnode_t)node);
	if (id && (id = searchpromtree(id, name, buf)))
		return (id);
	id = prom_childnode((dnode_t)node);
	if (id && (id = searchpromtree(id, name, buf)))
		return (id);
	return (0);
}

#ifdef	DPRINTF
static int path_debug = 0;
#undef	dprintf
#define	dprintf		if (path_debug) printf
#endif	DPRINTF

int (*
path_getdecodefunc(dip))()
	struct dev_info *dip;
{
	struct dev_path_ops *dpop;

	for (dpop = dev_path_opstab; dip && dpop->devp_name; ++dpop) {
		if (strcmp(dip->devi_name, dpop->devp_name) == 0)
			return (dpop->devp_decode_regprop);
	}
	return ((int (*)())0);
}

char *(*
path_getencodefunc(dip))()
	struct dev_info *dip;
{
	struct dev_path_ops *dpop;

#ifdef	DPRINTF
	dprintf("path_getencodefunc: for device <%s>\n", dip->devi_name);
#endif	DPRINTF

	for (dpop = dev_path_opstab; dip && dpop->devp_name; ++dpop) {
		if (strcmp(dip->devi_name, dpop->devp_name) == 0)
			return (dpop->devp_encode_reg);
	}
	return ((char *(*)())0);
}

int (*
path_getmatchfunc(dip))()
	struct dev_info *dip;
{
	struct dev_path_ops *dpop;

	for (dpop = dev_path_opstab; dip && dpop->devp_name; ++dpop) {
		if (strcmp(dip->devi_name, dpop->devp_name) == 0)
			return (dpop->devp_match_addr);
	}
	return ((int (*)())0);
}

static char *
path_gettoken(from, to)
	char *from, *to;
{
	while (*from) {
		switch (*from) {
		case '/':
		case '@':
		case ':':
			*to = '\0';
			return (from);
		default:
			*to++ = *from++;
		}
	}
	*to = '\0';
	return (from);
}

struct _p2d {
	char *name;
	char *addrspec;
	int unit;
	struct dev_info *dev;
};

static int
path_findmatch(dip, arg)
	struct dev_info *dip;
	caddr_t arg;
{
	struct _p2d *p2d = (struct _p2d *)arg;
	int (*match_func)();
	extern int obio_match();

	/*
	 * "A Grail? But we already have one!"
	 */
	if (p2d->dev)
		return ((int)(p2d->dev));

#ifdef	DPRINTF
	dprintf("\tchecking %s\n", dip->devi_name);
#endif	DPRINTF
	/*
	 * "These aren't the droids you're looking for. Move along."
	 */
	if ((strcmp(p2d->name, "sd") == 0) &&
	    (strcmp(dip->devi_name, "sr") == 0))
		goto sr_hack;
	if (strcmp(dip->devi_name, p2d->name) != 0)
		return (0);

sr_hack:
	/*
	 * XXX - If no unit is specified, take the first.
	 * There should be some concept of "take the default".
	 */
	if (p2d->addrspec[0] == '\0')
		return ((int)(p2d->dev = dip));

	/*
	 * We may need to call a matching function to decode the address.
	 * First find out if there even is one. Note that the address encoding
	 * is a function of our *parent*.
	 */
	match_func = path_getmatchfunc(dip->devi_parent);
	/*
	 * XXX - No matching function? Use "obio" by default.
	 */
	if (match_func == (int (*)())0)
		match_func = obio_match;

	if (match_func(dip, p2d->addrspec))
		return ((int)(p2d->dev = dip));
	return ((int)(p2d->dev = (struct dev_info *)0));
}

static struct dev_info *
path_matchtoken(name, addrspec, dip)
	char *name, *addrspec;
	struct dev_info *dip;
{
	struct _p2d p2d;

	p2d.name = name;
	p2d.addrspec = addrspec;
	p2d.dev = NULL;
#ifdef	DPRINTF
	dprintf("walk layer at %x (%s); looking for '%s' '%s'\n",
	    dip, dip->devi_name, name, addrspec);
#endif	DPRINTF
	walk_layer(dip->devi_slaves, path_findmatch, (caddr_t)&p2d);
	return (p2d.dev);
}

/*
 * Given a path specifying a node in the prom's device tree,
 * return the kernel's corresponding dev_info node.
 * XXX - doesn't handle units yet.
 */
struct dev_info *
path_to_devi(p)
	char *p;
{
	char name[128], addrspec[128];
	struct dev_info *dip = top_devinfo;

	if (!p)
		return ((struct dev_info *)0);

	while (*p && *p == '/')
		++p;
#ifdef	DPRINTF
	dprintf("path: '%s'\n", p);
#endif	DPRINTF
	while (*p) {
		p = path_gettoken(p, name);
#ifdef	DPRINTF
		dprintf("name: '%s' remainder: '%s'\n", name, p);
#endif	DPRINTF
		if (*p == '@')
			p = path_gettoken(++p, addrspec);
		else
			addrspec[0] = '\0';
#ifdef	DPRINTF
		dprintf("addrspec: '%s' remainder: '%s'\n", addrspec, p);
#endif	DPRINTF
		dip = path_matchtoken(name, addrspec, dip);
		if (dip == (struct dev_info *)0)
			break;
		while (*p && *p++ != '/')
			;
	}
	return (dip);
}

dnode_t
path_to_nodeid(p)
	char *p;
{
	struct dev_info *dip;
	dip = path_to_devi(p);
	return (dip ? (dnode_t)dip->devi_nodeid : OBP_BADNODE);
}

/*
 * path_to_subdev: return the subdevice of the given
 * path, defined as the string following the *FINAL*
 * colon in the path.
 *
 * %%% if we want to use more than one colon in a
 * path, or strange semantics of colons earlier in
 * the path than the final component, fix this.
 */
char *
path_to_subdev(p)
	char *p;
{
	register char ch;
	while (ch = *p++)
		if (ch == ':')
			return (p);
	return ((char *)0);
}

/*
 * Given a path "/foo/bar/mumble/disk@blah,blah:part" where
 * part is an integer, return the partition number.
 * XXX - This needs to be designed better. Should part be a block offset,
 * or an integer, rather than a character?
 */
int
get_part_from_path(p)
	char *p;
{
	char *cp;

	if (!p)
		return (0);
	cp = p + strlen(p);
	while (--cp != p) {
		if (*cp == ':')
			break;
	}
	if ((cp++ == p) || (*cp == '\0'))
		return (0);
	if (*cp >= 'a' && *cp <= 'z')	/* letter */
		return (*cp - 'a');
	if (*cp >= '0' && *cp <= '9')	/* digit */
		return (*cp - '0');
	return (*cp);			/* raw integer? */
}

static int
path_matchnameunit(dip, arg)
	struct dev_info *dip;
	caddr_t arg;
{
	struct _p2d *p2d = (struct _p2d *)arg;

	if (p2d->dev)
		return ((int)(p2d->dev));

#ifdef	DPRINTF
	dprintf("\tchecking devi <%x> unit <%d> name <%s>\n",
	    (int)dip, (int)dip->devi_unit, dip->devi_name);
#endif	DPRINTF
	if (strcmp(dip->devi_name, p2d->name) != 0)
		return (0);

	/*
	 * XXX - If no unit is specified, take the first.
	 * There should be some concept of "take the default".
	 */
	if (p2d->unit == -1 || p2d->unit == dip->devi_unit)
		return ((int)(p2d->dev = dip));

	return ((int)(p2d->dev = (struct dev_info *)0));
}

static struct dev_info *
path_findnodebyname(name, unit, dip)
	char *name;
	int unit;
	struct dev_info *dip;
{
	struct _p2d p2d;

	p2d.name = name;
	p2d.unit = unit;
	p2d.dev = NULL;
#ifdef	DPRINTF
	dprintf("walk tree at %x (%s); looking for '%s' '%d'\n",
	    dip, dip->devi_name, name, unit);
#endif	DPRINTF
	walk_devs(dip, path_matchnameunit, (caddr_t)&p2d);
	return (p2d.dev);
}

void
devi_to_path(name)
	char *name;
{
	struct dev_info *dip, *pdip;
	char tmpname[OBP_MAXDEVNAME];
	char addr[OBP_MAXDEVNAME];
	char *cp;
	int unit = 0;
	int base = 10;
	char part = '\0';
	extern char *strcpy();
	char *(*encode_func)();

	/*
	 * Scan "name", pulling out the unit number and partition.
	 * Null out the unit number in the string so that the device
	 * name becomes null-terminated.
	 */

	cp = name;

#ifndef	sun4c
	/*
	 * Special case (for IPI) if device unit number is in hex.
	 * Hex unit numbers are three digits, currently...
	 */

	if (units_are_hex(cp) != 0)  {
		if (*(cp+5))
			part = *(cp+5);
		*(cp+5) = (char)0;
		unit = atou(cp+2);
		*(cp+2) = (char)0;
	}  else  {
#endif	! sun4c
		while (*cp && *cp > '9')
			++cp;
		while (*cp) {
			if ((*cp < '0') || (*cp > '9'))
				break;
			unit = (unit * base) + (*cp - '0');
			*cp++ = '\0';
		}
		if (*cp)
			part = *cp;
#ifndef	sun4c
	}
#endif	! sun4c

	/*
	 * Locate the devinfo node for this device.
	 */
	dip = path_findnodebyname(name, unit, top_devinfo);
#ifdef	DPRINTF
	dprintf("path_to_devi: path_findnodebyname returned dip %x\n",
	    (int)dip);
#endif	DPRINTF
	*name = '\0';
	if (dip == (struct dev_info *)0)
		return;
#ifdef	DPRINTF
	dprintf("path_to_devi: devi_name %s, devi_parent is %x\n",
	    dip->devi_name, dip->devi_parent);
#endif	DPRINTF

	/*
	 * Now trace the tree bottom-to-top, translating each node
	 * into a prom-compatible specifier. Prepend each specifier
	 * to the path already built. Stop at the root node.
	 */
	for (/* empty */; ((pdip = dip->devi_parent) != (struct dev_info *)0);
	    dip = pdip)  {
#ifndef lint
		register char *p;
#endif

		/*
		 * Find the appropriate encoding funtion to translate
		 * the node's reg, target/lun, etc. information
		 * to an address specification that the prom can grok.
		 */

		encode_func = path_getencodefunc(dip->devi_parent);
		/*
		 * XXX - No matching function? Use "obio" by default.
		 */
		if (encode_func == (char *(*)())0)
			encode_func = obio_encode_reg;
		switch (encode_func(dip, addr))  {
		case 0:
			/*
			 * Error return: nullify output buffer so
			 * caller notices error... (error if output
			 * does not start with a '/' character.)
			 */
			*name = (char)0;
			return;

		case -1:
			/*
			 * Skip me -- put nothing in output buffer.
			 * Node must be a software only node.
			 * Move along to the next node...
			 */
			continue;

		case -2:
			/*
			 * Full name returned -- just take it with a slash
			 * and ignore dip->devi_name.
			 */

			if (*addr != (char)0)
				(void)sprintf(tmpname, "/%s%s",
				    addr, name);
			break;

		default:
			/*
			 * If output buffer is NULL, there is no
			 * address qualifier, else there is one.
			 */

			if (*addr != (char)0)
				(void)sprintf(tmpname, "/%s@%s%s",
				    dip->devi_name, addr, name);
			else
				(void)sprintf(tmpname, "/%s%s",
				    dip->devi_name, name);
		}

		(void)strcpy(name, tmpname);
#ifdef	DPRINTF
		dprintf("***name <%s>\n", name);
#endif	DPRINTF
	}

	if (part) {
		cp = name + strlen(name);
		(void)sprintf(cp, ":%c", part);
	}
#ifdef	DPRINTF
	dprintf("name = '%s'\n", name);
#endif	DPRINTF
}

#ifndef sun4c
static int
units_are_hex(name)
	register char *name;
{
	register char **p;
	extern char *hex_unit_devices[];

	for (p = hex_unit_devices; *p != (char *)0; ++p)  {
		if (strncmp(*p, name, 2) == 0)  {
#ifdef	DPRINTF
			dprintf("%s units are in hex.\n");
#endif	DPRINTF
			return (1);
		}
	}
#ifdef	DPRINTF
	dprintf("%s units are not in hex.\n");
#endif	DPRINTF
	return (0);
}
#endif	! sun4c
