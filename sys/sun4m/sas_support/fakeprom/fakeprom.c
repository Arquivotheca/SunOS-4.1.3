#ident "@(#)fakeprom.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*
 * fake_prom.c
 *
 */

#include <mon/openprom.h>
#include <mon/sunromvec.h>
#include "fakeprom.h"

u_int debug = 0;

extern devnode_t *simnext();
extern devnode_t *simchild();
extern int simgetproplen();
extern int simgetprop();
extern int simsetprop();
extern char *simnextprop();

typedef int (*pfi)();

struct config_ops config_ops = {
	(pfi)simnext,
	(pfi)simchild,
	(pfi)simgetproplen,
	(pfi)simgetprop,
	(pfi)simsetprop,
	(pfi)simnextprop,	
};



#define MEMLIST_NULL ((struct memlist *)NULL)

struct memlist physmem = {
	MEMLIST_NULL,
	0,
	0x700000
};

struct memlist *physmemp = &physmem;

/* fakeprom virtual memory */
struct memlist monvirtmem = {
	MEMLIST_NULL,
	0xFFD00000,
	0x00200000,
};

/* kadb virtual memory - is kadb supposed to remove this like it does with pmem? */
struct memlist kadbvirtmem = {
	MEMLIST_NULL,
	0xFFC00000,
	0x00100000,
};

struct memlist *virtmemp = &monvirtmem;

struct memlist availmem1 = {
	MEMLIST_NULL,
	0x00400000,
	0x00300000,
};

#ifdef MONOFB
struct memlist availmem = {
	&availmem1,
	0x00000000,
	0x003E0000,
};
#else
struct memlist availmem = {
	MEMLIST_NULL,
	0x00000000,
	0x00700000,
};
#endif

struct memlist *availmemp = &availmem;

/* 
 * Boot Parameter Support 
 *
 * We can either boot vmunix or kadb (someday add support for real boot
 * command lines parsing.
 */
char *bootstrp = "nullboot";

char vbootstr[] = "boot sd(0,0,0)vmunix -si /sbin/init.sas";
char vbootname[] = "vmunix";
char vbootarg[] = "sd(0,0,0)vmunix";
char vbootflgs[] = "-si";
char vbootarg2[] = "/sbin/init.sas";

char kbootstr[] = "boot sd(0,0,0)kadb";
char kbootname[] = "kadb";
char kbootarg[] = "sd(0,0,0)kadb";
char kbootflgs[] = "";
char kbootarg2[] = "";

struct bootparam sd_bootparam = {
	{ 0 },
	{ 0 },
	"sd",
	0, 0, 0,
	NULL,
	&sd_boottab,
};

struct bootparam * bootparamp = &sd_bootparam;

void vcmd();

void (*vcmdptr)() = vcmd;

void
vcmd()
{
}

#define	MAXSPARE	0x100000

u_char	sparemem[MAXSPARE];
int	cur_spare = 0;

caddr_t
opalloc(ptr,size)
caddr_t	ptr;
int	size;
{
	caddr_t	rtn_addr;

	if (cur_spare + size >= MAXSPARE) {
		printf("opalloc: Not enought memory to allocate\n");
		return(0);
	}
	rtn_addr = (caddr_t) &sparemem[cur_spare];
	cur_spare +=size;
	return(rtn_addr);
}

char psname[]    = "name";
char psreg[]	 = "reg";
char psunit[] 	= "unit";
char psrange[]	 = "range";
char psccoher[]   = "cache-coherence?";
char pspagesize[] = "page-size";
char psdevtype[] = "device_type";
char psmmc[]     = "memory-controller";
char psvme[]     = "vme";
char psiommu[]   = "iommu";
char psproc[]	 = "processor";
char psgraph[]	 = "graphics";
char psblock[]	 = "block";
char psnetwork[] = "network";

/*
 * The following routines match_prop(), simnext(), simchild() and
 * simgetprop() are used when kernel resides on the high address area
 */
struct property *
match_prop(proplist, name)
        register struct property *proplist;
        register char *name;
{
	for (; proplist->name != 0; proplist++)
		if (strcmp(proplist->name,name) == 0)
			return (proplist);
	return (0);
}

struct property *
next_prop(proplist, name)
        register struct property *proplist;
        register char *name;
{
        for (; proplist->name != 0; proplist++)
		if (strcmp(proplist->name,name) == 0)
			return (++proplist);
	return (0);
}

devnode_t *
simnext(nodeid)
        register devnode_t *nodeid;
{
	return nodeid ? nodeid->next : &root_info;
}

devnode_t *
simchild(parentid)
        register devnode_t *parentid;
{
	return parentid ? parentid->slaves : &root_info;
}

int
simgetprop(nodeid, name, buf)
        register devnode_t *nodeid;
        char *name;
	char *buf;
{
        register struct property *prop;
	register char	*bufp;
	register char	*propval;
	register u_int	i;

        if (!nodeid)
                nodeid = &root_info;
	prop = match_prop(nodeid->props, name);
        if (!prop)
                return (-1);
	propval = prop->value;
	for (i=0; i < (u_int)prop->size; i++)
		*buf++ = *propval++;
        return (prop->size);
}

simgetproplen(nodeid, name)
        register devnode_t *nodeid;
        char *name;
{
        register struct property *prop;
	register char	*propval;
	register u_int	i;

        if (!nodeid)
                nodeid = &root_info;
	prop = match_prop(nodeid->props, name);
        if (!prop)
                return (-1);
        return (prop->size);
}

simsetprop()
{
}

char *
simnextprop(nodeid, prev)
	register devnode_t *nodeid;
	caddr_t	prev;
{
        register struct property *prop;

        if (!nodeid)
                nodeid = &root_info;
	if (!prev)
		return nodeid->props->name;
	prop = next_prop(nodeid->props, prev);
        if (!prop)
                return (char *)(-1);
	return prop->name;
}

simnop()
{
}

int	olfcr = 0;		/* send cr after lf */

getchar()
{
	return v_mayget();
}

void
v_putchar(c)
u_int	c;
{
	simcoutc(c);
	if (olfcr && (c == '\n'))
		simcoutc('\r');
		
}

int
v_mayget()
{
	int c = simcinc();
	return ((c == 0) ? -1 : c);
}

int runstate = 0;

void
do_cstart()
{
	int cmd;
	int loadbb = 0;

	printf("ok ");

	switch(runstate) {
		/* normal state run code at 0x4000 - vmunix -bsi */
	      default:
	      case 0:
		boot_vmunix();
		printf("go 0x4000 (vmunix)\n");
		return;
		
	      case 1:
		boot_kadb();
		printf("go 0x4000 (kadb)\n");
		break;

		/* load bootblock & run vmunix */
	      case 2:
		boot_vmunix();
		printf("%s\n", bootstrp);
		loadbb = 1;
		break;

		/* load bootblock & run kadb */
	      case 3:
		boot_kadb();
		printf("%s\n", bootstrp);
		loadbb = 1;
		break;

	      case 4:
		loadbb = do_interactive();
		break;
	}
	if (loadbb)
		load_bootblocks();
}

do_interactive()
{
	int c;
	int loadbb = 0;

	printf("[vV]munix, [kK]adb\nok ");
	while ((c = getchar()) == -1)
		;
	putchar(c);

	switch (c) {
	      case 'K':
		loadbb = 1;
	      case 'k':
		boot_kadb();
		printf(" %s\n", bootstrp);
		break;

	      case 'V':
		loadbb = 1;
	      case 'v':
	      default:
		boot_vmunix();
		printf(" %s\n", bootstrp);
		break;
	}
	return loadbb;
}

void
do_cexit()
{
	printf("ok exit!!\n");
}

boot_vmunix()
{
	register struct bootparam *bp = bootparamp;

	bootstrp = vbootstr;
	bp->bp_argv[0] = vbootarg;
	bp->bp_argv[1] = vbootflgs;
	bp->bp_argv[2] = vbootarg2;
	bp->bp_name = vbootname;
}

boot_kadb()
{
	register struct bootparam *bp = bootparamp;

	bootstrp = kbootstr;
	bp->bp_argv[0] = kbootarg;
	bp->bp_argv[1] = kbootflgs;
	bp->bp_argv[2] = NULL;
	bp->bp_name = kbootname;
	monvirtmem.next = &kadbvirtmem;
}

