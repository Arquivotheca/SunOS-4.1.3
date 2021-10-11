/*	@(#)test.c 1.1 92/07/30 SMI	*/
#include <stdio.h>
#include <kvm.h>
#include <nlist.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>

extern int optind;
extern char *optarg;

kvm_t *cookie;
int _kvm_debug;		/* possibly defined by kvmopen.o */

struct proc *tst_getproc();
struct proc *tst_nextproc();
struct user *tst_getu();

char *name;
char *core;
char *swap;
int wflag;

struct nlist nl[] = {
	{"_free"},
	{"_fragtbl"},
	{"_freemem"},
	{0}
};


main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
	int c, errflg = 0;
	long xx;
	struct nlist *nlp;
	struct proc *proc;
	struct user *u;
	register int envc, ccnt;

	_kvm_debug = 1;
	for (envc = 0; *envp++ != NULL; envc++);
	envp -= 2;
	ccnt = (*envp - *argv) + strlen(*envp) + 1;
	printf("pid %d:: %d args; %d envs; %d chars (%x - %x)\n",
	    getpid(), argc, envc, ccnt,
	    &argv[0], *envp + strlen(*envp));

	while ((c = getopt(argc, argv, "w")) != EOF)
		switch (c) {
		case 'w':
			wflag++;
			break;
		case '?':
			errflg++;
		}
	if (errflg) {
		fprintf(stderr, "usage: %s [-w] [name] [core] [swap]\n",
		    argv[0]);
		exit(1);
	}
	if (optind < argc) {
		name = argv[optind++];
		if (*name == '\0')
			name = NULL;
	} else
		name = NULL;
	if (optind < argc) {
		core = argv[optind++];
		if (*core == '\0')
			core = NULL;
	} else
		core = NULL;
	if (optind < argc) {
		swap = argv[optind++];
		if (*swap == '\0')
			swap = NULL;
	} else
		swap = NULL;

	tst_open(name, core, swap, (wflag ? O_RDWR : O_RDONLY));
	if (cookie == NULL)
		exit(1);

	tst_nlist(nl);

	for (nlp=nl; nlp[0].n_type != 0; nlp++)
		tst_read(nlp[0].n_value, &xx, sizeof(xx));

	while ((proc = (struct proc *)tst_nextproc()) != NULL) {
		tst_getproc(proc->p_pid);
	}

	tst_setproc();

	while ((proc = (struct proc *)tst_nextproc()) != NULL) {
		if ((u = (struct user *)tst_getu(proc)) != NULL)
			tst_getcmd(proc, u);
	}
	tst_close();
	exit(0);
}

printf(a1,a2,a3,a4,a5,a6,a7,a8,a9)
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, a1,a2,a3,a4,a5,a6,a7,a8,a9);
	fflush(stderr);
}

tst_open(namelist, corefile, swapfile, flag)
char *namelist;
char *corefile;
char *swapfile;
int flag;
{
	printf("kvm_open(%s, %s, %s, %s)\n",
	    (namelist == NULL) ? "LIVE_KERNEL" : namelist,
	    (corefile == NULL) ? "LIVE_KERNEL" : corefile,
	    (swapfile == NULL) ?
		((corefile == NULL) ? "LIVE_KERNEL" : "(none)") : swapfile,
	    (flag == O_RDONLY) ? "O_RDONLY" : ((flag == O_RDWR) ?
							"O_RDWR" : "???"));
	if ((cookie =
	    kvm_open(namelist, corefile, swapfile, flag, "libkvm test")) == NULL)
		printf("kvm_open returned %d\n", cookie);
}

tst_close()
{
	register int i;

	printf("kvm_close()\n");
	if ((i = kvm_close(cookie)) != 0)
		printf("kvm_close returned %d\n", i);
}

tst_nlist(nl)
struct nlist nl[];
{
	register int i,t,x;

	printf("kvm_nlist([nl])\n");
	if ((i = kvm_nlist(cookie, nl)) != 0)
		printf("kvm_nlist returned %d\n", i);
	for (i=0; (nl[i].n_name!=0) && (nl[i].n_name[0]!='\0'); i++) {
		x = nl[i].n_type & N_EXT;
		t = nl[i].n_type & ~N_EXT;
		printf("%s: %x (%s%s)\n",
			nl[i].n_name,
			nl[i].n_value,
			((t == N_UNDF) ? "undefined" :
			(t == N_TEXT) ? "text" :
			(t == N_DATA) ? "data" :
			(t == N_BSS) ? "bss" :
			(t == N_ABS) ? "absolute" :
			"???"),
			(x ? "-e" : ""));
	}
}

tst_read(addr, buf, nbytes)
unsigned long addr;
char *buf;
unsigned nbytes;
{
	register int e;
	register int i;
	register char *b;

	printf("kvm_read(%x, [buf], %d)\n", addr, nbytes);
	if ((e = kvm_read(cookie, addr, buf, nbytes)) != nbytes)
		printf("kvm_read returned %d instead of %d\n", e, nbytes);
	for (b=buf,i=0; i<nbytes; b++,i++) {
		printf("%x: %02x (%04o)\n", addr+i, *b&0xff, *b&0xff);
	}
	return (e);
}

tst_write(addr, buf, nbytes)
unsigned long addr;
char *buf;
unsigned nbytes;
{
	register int e;
	register int i;
	register char *b;

	printf("kvm_write(%x, [buf], %d)\n", addr, nbytes);
	if ((e = kvm_write(cookie, addr, buf, nbytes)) != nbytes)
		printf("kvm_write returned %d instead of %d\n", e, nbytes);
	if ((b = (char*)malloc(nbytes)) == 0)
		printf("malloc for readback failed\n");
	else {
		if ((i = kvm_read(cookie, addr, b, nbytes)) != nbytes)
			printf("readback returned %d\n", i);
		else if (bcmp(b, buf, nbytes))
			printf("write check failed!\n");
		(void) free(b);
	}
	return (e);
}

struct proc *
tst_getproc(pid)
int pid;
{
	struct proc *proc;

	printf("kvm_getproc(%d)\n", pid);
	if ((proc = kvm_getproc(cookie, pid)) == NULL) {
		printf("kvm_getproc returned NULL\n");
		return (proc);
	}
	printf("p_pid: %d\n", proc->p_pid);
	return (proc);
}

struct proc *
tst_nextproc()
{
	struct proc *proc;

	printf("kvm_nextproc()\n");
	if ((proc = kvm_nextproc(cookie)) == NULL) {
		printf("kvm_nextproc returned NULL\n");
		return (proc);
	}
	printf("p_pid: %d\n", proc->p_pid);
	return (proc);
}

tst_setproc()
{
	register int i;

	printf("kvm_setproc()\n");
	if ((i = kvm_setproc(cookie)) != 0)
		printf("kvm_setproc returned %d\n", i);
	return (i);
}

struct user *
tst_getu(proc)
struct proc *proc;
{
	register int e;
	struct proc tp;
	struct user *u;

	printf("kvm_getu(pid:%d)\n", proc->p_pid);
	if ((u = kvm_getu(cookie, proc)) == NULL) {
		printf("kvm_getu returned NULL\n");
		return (u);
	}
	if ((e = kvm_read(cookie, u->u_procp, &tp, sizeof (tp))) != sizeof (tp))
		printf("kvm_read returned %d instead of %d\n", e, sizeof (tp));
	printf("u_procp: %x -> p_pid: %d\n", u->u_procp, tp.p_pid);
	return (u);
}

tst_getcmd(proc, u)
struct proc *proc;
struct user *u;
{
	char **arg;
	char **env;
	register int i;
	char **p;

    printf("kvm_getcmd(pid:%d, [u], arg, env)\n", proc->p_pid);
    if ((i = kvm_getcmd(cookie, proc, u, &arg, &env)) != 0) {
	    printf("kvm_getcmd returned %d\n", i);
	    return (i);
    }
    printf("Args:  ");
    for (p = arg; *p != NULL; p++)
	    printf("%s ", *p);
    printf("\nEnv:  ");
    for (p = env; *p != NULL; p++)
	    printf("%s ", *p);
    printf("\n");
    (void) free(arg);
    (void) free(env);
    return (i);
}
