/* @(#)fakeprom.h	1.1 8/6/90 SMI */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

#include "module.h"
#include "cpu.h"

#define ROMVEC_ADDR 	0x70000
#define ARRAY(what)	sizeof(what),   (caddr_t)&what
#define RARRAY(what)	sizeof(what),   (caddr_t)what
#define BOOLEAN		0, 0

#define RSTRCPY(a, b)		use strcpy(a, b)
#define RSTRCMP(a, b)		use strcmp(a, b)
#define RSTRNCMP(a, b, c)	use strncmp(a, b, c)

struct getprop_info {
	char  	  *name;
	caddr_t   var;
};
struct convert_info {
	char *name;
	u_int var;
};

struct node_info {
	caddr_t name;
	int	size;
	caddr_t prop;
	caddr_t	prop_end;
	caddr_t value;
};




typedef struct dev_element {
        struct dev_element *next;
        struct dev_element *slaves;
        struct property    *props;
} devnode_t;

struct property {
        char 	*name;
        int 	size;
        caddr_t value;
};

struct dev_reg {
	int reg_bustype;                /* cookie for bus type it's on */
	addr_t reg_addr;                /* address of reg relative to bus */
	u_int reg_size;                 /* size of this register set */
};

extern devnode_t root_info; /* root node */
extern devnode_t mod0_info; /* link from system nodes to module nodes */
extern char psname[];
extern char psreg[];
extern char psunit[];
extern char psrange[];
extern char psccoher[];
extern char pspagesize[];
extern char psdevtype[];
extern char psmmc[];
extern char psvme[];
extern char psiommu[];
extern char psproc[];
extern char psgraph[];

extern struct config_ops config_ops;
extern char *bootstrp;
extern struct memlist * virtmemp;
extern struct memlist * availmemp;
extern struct memlist * physmemp;
extern u_char simcinc();
extern void simcoutc();
extern u_int open(), close(), readblks(), writeblks();
extern void do_boot();
extern void do_enter();
extern void do_exit();
extern int printf();
extern void (*vcmdptr)();
extern caddr_t opalloc();
extern void v_putchar();
extern int v_mayget();
extern struct bootparam *bootparamp;
extern u_int debug;
extern struct boottab sd_boottab;
