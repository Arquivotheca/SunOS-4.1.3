/*      @(#)llib-lprom.c 1.1 92/07/30 SMI      */
/*LINTLIBRARY*/

#include <promcommon.h>

caddr_t prom_alloc(virthint,size)  caddr_t virthint; u_int size; { return((caddr_t) 0);}
char *prom_bootargs() {return((char *)0);}
struct bootparam *prom_bootparam() { return((struct bootparam *)0);}
char *prom_bootpath() { return ((char *)0);}
int prom_close(fd) int fd; { return (0);}
int prom_devicetype(id, type) dnode_t id; char *type; { return(0);}
void prom_enter_mon() {;}
void prom_exit_to_mon() {;}
int prom_stdout_is_framebuffer() { return(0); }
void prom_free(virt, size) caddr_t	virt; u_int	size; {;}
u_char prom_getchar() { return ((u_char)0); }
int prom_getproplen(nodeid, name) dnode_t nodeid; caddr_t name; { return (0); }
int prom_getprop(nodeid, name, value) dnode_t nodeid; caddr_t name; caddr_t value; { return (0); }
u_int prom_gettime() { return ((u_int) 0); }
int prom_sethandler(v0_func, v2_func) void (*v0_func)(); void (*v2_func)(); { return (0); }
int prom_idlecpu(node) dnode_t	node; { return (0); }
char prom_getidprom(addr, size) caddr_t addr; int size; { return ((char) 0); }
void prom_init(pgmname) char *pgmname; {;}
char *prom_stdinpath() { return ((char *)0); }
void prom_interpret(forthstring) char	*forthstring; { ; }
prom_stdin_is_keyboard() { return(0); }
caddr_t prom_map(virthint, space, phys, size) caddr_t	virthint; u_int	space, phys, size; { return((caddr_t) 0); }
int prom_mayget() { return (0); }
int prom_mayput(c) char c; { return (0); }
caddr_t prom_nextprop(nodeid, previous) dnode_t nodeid; caddr_t previous; { return((caddr_t) 0);}
dnode_t prom_nextnode(nodeid) dnode_t  nodeid; { return ((dnode_t) nodeid); }
dnode_t prom_childnode(nodeid) dnode_t nodeid; { return ((dnode_t) nodeid); }
int prom_open(name) char	*name; { return(0); }
char * prom_stdoutpath() { return ((char *)0); } 
#ifndef lint
int panic(string) char *string; {return(0); }
#endif lint
phandle_t prom_getphandle(i) ihandle_t	i; { return( (phandle_t) 0); }
/*VARARGS1*/
prom_printf(fmt, va_alist) char *fmt; { ; }
void prom_printn(n, b, width) u_long n; int b, width; { ; }
void prom_putchar(c) char c; { ; }
void prom_reboot(s) char *s; {;}
int prom_seek(fd, low, high) int	fd, low, high; { return(0); }
void prom_setcxsegmap(c, v, seg) int c; addr_t v; u_char seg; { ; }
int prom_setprop(nodeid, name, value, len) dnode_t nodeid; caddr_t name; caddr_t value; int	len; { return (0); }
int prom_startcpu(node, context, whichcontext, pc) dnode_t		node; struct dev_reg	*context; int		whichcontext; addr_t		pc;
{
  return(0);
}

int prom_resumecpu(node) dnode_t		node; {return(0); }
int prom_stopcpu(node) dnode_t		node; {return(0); }
char *strcpy(s1, s2) char *s1, *s2; {return((char *) 0);} 
struct dev_info *prom_get_boot_devi() { return ((struct dev_info *)0); }
int prom_get_boot_dev_name(p, bufsize) char *p; int bufsize;
{
		return (0);
}
int prom_get_boot_dev_unit() { return(0); }
int prom_get_boot_dev_part() { return(0); }
char * prom_get_path_option(path) char *path; { return ((char *)0); }
prom_get_stdin_dev_name(path, len) char *path; int   len; { return (0); }
prom_get_stdout_dev_name(path, len) char *path; int   len; { return (0); }
int prom_get_stdin_unit() { return (-1); }
int prom_get_stdout_unit() { return (-1); }
char * prom_get_stdin_subunit() { return ((char *)0); }
char * prom_get_stdout_subunit() { return ((char *)0); }
int prom_stdin_stdout_equivalence() { return (0); }
int montrap(funcptr) int (*funcptr)(); { return (*funcptr)(); }
int prom_getversion() { return(0);}
void prom_writestr(buf, bufsize) char *buf; u_int bufsize; { ; }


