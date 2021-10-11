#ident "@(#)treetest.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

#include "faketest.h"

struct sunromvec * romp;

start(rp)
struct sunromvec * rp;
{
	int root;
	int fd;
	
	/* setup global */
	romp = rp;

	/* print boot flags */
	print_bootparam();

	/* print memory lists */
	print_memlist("physmemory", *romp->v_physmemory);
	print_memlist("virtmemory", *romp->v_virtmemory);
	print_memlist("availmemory", *romp->v_availmemory);

	/* print device tree */
	root = NEXT(0);
	walk_and_print_tree(root, 0);

	/* nowhere to go */
	while(1);

	/* 
	 * /boot won't load files that have text smaller than about 8k
	 *  pad for now
	 */
	asm(".text");
	asm(".skip 0x2000");
}

walk_and_print_tree(node, indent)
u_int node;
u_int indent;
{
	char tmpname[32];
	int cnode;
	caddr_t	pname;

	getprop(node, "name", tmpname);
	
	tab(indent); printf("Node '%s'\n", tmpname);
	tab(indent+1); printf("Defined Properties:\n");

	for (pname = nextprop(node, NULL); pname; pname = nextprop(node, pname))
		print_val(node, pname, indent+2);

	printf("\n");
		
	for (cnode = CHILD(node); cnode != 0 && cnode != -1; cnode = NEXT(cnode))
		walk_and_print_tree(cnode, indent+1);
}

proptype(node, name)
u_int node;
caddr_t name;
{
	int size;
	int type;

	size = getproplen(node, name);

	switch(size) {
	      case 0:
		type = XDRBOOL;
		break;

	      case 4:
		type = XDRINT;
		break;

	      default:
		type = XDRSTRING;
		break;
	}
	return type;
}

print_memlist(name, memp)
char *name;
struct memlist *memp;
{
	printf("\n%s:\n", name);
	for ( ; memp; memp = memp->next)
		printf("\t0x%x 0x%x\n", memp->address, memp->size);
	printf("\n");
}

print_val(node, name, indent)
u_int node;
caddr_t name;
u_int indent;
{
	u_int val;
	int type;
	u_char c;
	u_char d;

	type = proptype(node, name);
	getprop(node, name, &val);

	/* XXX - 3 character names are a problem, any other ideas? */
	
	c = *(u_char *)&val;
	d = val & 0xff;
	
	if (d == '\0' && isalpha(c))
	    type = XDRSTRING;

	tab(indent);

	switch (type) {
		
	      case XDRBOOL:
		printf("Type=Boolean Property='%s'\n", name);
		break;
		
	      case XDRINT:
		printf("Type=Integer Property='%s' Value=%d\n", name, val);
		break;
		
	      case XDRSTRING:
		if (!strcmp("reg", name)) {
			print_registers(node, indent+2);
			break;;
		} else if (!strcmp("range", name)) {
			print_range(node, indent+2);
			break;
		} else
			print_string(node, name, indent+2);
		break;

	      default:
		break;
	}
}

print_string(node, name, indent)
u_int node;
caddr_t name;
u_int indent;
{
	u_char buf[32];
	int len;

	getprop(node, name, buf);
	len = getproplen(node, name);

	printf("Type=String Property='%s' Value='%s' Length=%d\n", name, buf, len);
}
	
print_registers(node, indent)
u_int node;
u_int indent;
{
	int nreg;
	struct dev_reg tmp_regs[8];
	struct dev_reg *dp;
	int i;
	char *str = "reg";
	int len;

	nreg = (len = getproplen(node, str))/sizeof(struct dev_reg);
	if (nreg) {
		getlongprop(node, str, tmp_regs);
		printf("Type=String Property='%s' Length=%d\n", str, len);
		for (i = 0, dp = &tmp_regs[0]; i < nreg; ++i, ++dp) {
			tab(indent);
			printf("Bus Type=0x%x, Address=0x%x, Size=0x%x\n",
			       dp->reg_bustype, dp->reg_addr, dp->reg_size);
		}
	}
}

print_range(node, indent)
u_int node;
u_int indent;
{
	int nreg;
	struct dev_reg tmp_regs[8];
	struct dev_reg *dp;
	int i;
	char *str = "range";
	int len;

	nreg = (len = getproplen(node, str))/sizeof(struct dev_reg);
	if (nreg) {
		getlongprop(node, str, tmp_regs);
		printf("Type=String Property='%s' Length=%d\n", str, len);
		for (i = 0, dp = &tmp_regs[0]; i < nreg; ++i, ++dp) {
			tab(indent);
			printf("Bus Type=0x%x, Address=0x%x, Size=0x%x\n",
			       dp->reg_bustype, dp->reg_addr, dp->reg_size);
		}
	}
}


tab(indent)
u_int indent;
{
	static int tabsize = 4;
	int i;

	while(indent--) {
		i = tabsize;
		while (i--)
			putchar(' ');
	}
}

print_bootparam()
{
	char **cp = (*romp->v_bootparam)->bp_argv;

	printf("Boot Parameters:\n");
	while (*cp) {
		tab(1);
		printf("%s\n", *cp);
		++cp;
	}
	printf("\n");
}
