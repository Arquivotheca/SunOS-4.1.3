/* test_memtest.c */

/*
 * User-level program to test the memtest driver.
 *
 * Usage: test_memtest function [...]
 * See below for the list of functions.
 *
 * Specifying more than one function is probably a waste of time, as the
 * system will either panic or kill the process on the first error.
 *
 * TODO: catch signals and continue
 */

#include <stdio.h>
#include <setjmp.h>
#include <sys/ioctl.h>
#include <memtestio.h>

char	Usage[] = "Usage: %s function [...]\n\
where function is one (or more) of\n\
	kfetch	cause a kernel ifetch parity error\n\
	kload	cause a kernel load parity error\n\
	kstore	cause a kernel store timeout error\n\
	kloadt	cause a kernel load timeout error\n\
	kfetcht	cause a kernel fetch timeout error\n\
	ufetch	cause a user ifetch parity error\n\
	uload	cause a user load parity error\n\
	uloadc	cause a user load parity error on cache load\n\
	uloadm	cause multiple user parity errors on cache load\n\
	ustore	cause a user store timeout error\n\
	ustoreX	cause both a user store timeout and protection error\n\
	(X is one of 0, 1, 2, or 3)\n\
	uloadt	cause a user load timeout error\n\
	ufetcht	cause a user fetch timeout error\n\
	dma	cause a dma parity error\n\
";

char	*progname;
int	errcnt = 0;

int	dokfetch(), dokload(), dokstore(), dokloadt(), dokfetcht();
int	doufetch(), douload(), doustore(), douloadt(), doufetcht();
int	douloadc(), douloadm(), dodma();
int	doustorem0(), doustorem1(), doustorem2(), doustorem3();

struct command {
	char	*c_name;
	int	(*c_func)();
} command[] = {
	"kfetch",	dokfetch,
	"kload",	dokload,
	"kstore",	dokstore,
	"kloadt",	dokloadt,
	"kfetcht",	dokfetcht,
	"ufetch",	doufetch,
	"uload",	douload,
	"uloadc",	douloadc,
	"uloadm",	douloadm,
	"ustore",	doustore,
	"ustore0",	doustorem0,
	"ustore1",	doustorem1,
	"ustore2",	doustorem2,
	"ustore3",	doustorem3,
	"uloadt",	douloadt,
	"ufetcht",	doufetcht,
	"dma",		dodma,
	NULL,		NULL};

jmp_buf	env;

main(argc, argv)
	int	argc;
	char	**argv;
{
	char	*arg;

	progname = *argv++;

	if (--argc <= 0) {
		usage();
		exit(1);
	}

	init();
	setjmp(env);	/* in case of a race, we do this twice */
	catchsigs();
	setjmp(env);	/* this is where we really want to be */

	while (arg = *argv++) {
		struct command	*p;

		for (p = command; p->c_name != NULL; p++) {
			if (strcmp(arg, p->c_name) == 0)
				break;
		}

		if (p->c_func == NULL)
			error("unknown function \"%s\"\n", arg);
		else {
			fprintf(stderr, "calling %s\n", p->c_name);
			(*p->c_func)();
		}
	}

	return (errcnt);
}

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

char	*memtest = "/dev/memtest";
int	fdmemtest = -1;	/* file descriptor */
caddr_t	badmem = NULL;	/* bad memory */
caddr_t u_data = NULL;	/* data area */
extern char *valloc();

char	*filetest = "/tmp/memtest";
int	fdfiletest = -1;	/* file descriptor */
int	*fileaddr = NULL;

init()
{

	fdmemtest = open(memtest, O_RDWR);
	if (fdmemtest == -1) {
		fprintf(stderr, "%s: couldn't open ", progname);
		perror(memtest);
		exit(1);
	}

#define PROT_ALL (PROT_READ | PROT_WRITE | PROT_EXEC)
	badmem = mmap(0, getpagesize(), PROT_ALL, MAP_SHARED, fdmemtest, 0);
	if (badmem == (caddr_t)-1) {
		fprintf(stderr, "%s: couldn't mmap ", progname);
		perror(memtest);
		exit(1);
	}

	fdfiletest = open(filetest, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fdfiletest == -1) {
		fprintf(stderr, "%s: couldn't create ", progname);
		perror(filetest);
		exit(1);
	}

	if (ftruncate(fdfiletest, 8) != 0) {
		fprintf(stderr, "%s: couldn't extend ", progname);
		perror(filetest);
		exit(1);
	}

	if (fsync(fdfiletest) != 0) {
		fprintf(stderr, "%s: couldn't fsync ", progname);
		perror(filetest);
		exit(1);
	}

	fileaddr = (int *)mmap(0, getpagesize(), PROT_ALL, MAP_SHARED, fdfiletest, 0);
	if (fileaddr == (int *)-1) {
		fprintf(stderr, "%s: couldn't mmap ", progname);
		perror(filetest);
		exit(1);
	}

	u_data = valloc(getpagesize());
	if (u_data == NULL) {
		fprintf(stderr, "%s: couldn't allocate u_data\n",
			progname);
		exit(1);
	}
}

#include <varargs.h>
error(va_alist)
	va_dcl
{
	va_list	pvar;
	char	*fmt;

	va_start(pvar);
	fmt = va_arg(pvar, char *);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, pvar);
	va_end(pvar);
	usage();
}

usage()
{

	if (errcnt++ == 0)
		fprintf(stderr, Usage, progname);
}

int
dokload()
{
	int	i;

	Sync();
	i = ioctl(fdmemtest, MEMTESTBADLOAD, 0);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("ioctl BADLOAD failed");
	}

	return i;
}

int
dokfetch()
{
	int	i;

	Sync();
	i = ioctl(fdmemtest, MEMTESTBADFETCH, 0);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("ioctl BADFETCH failed");
	}

	return i;
}

int
dokstore()
{
	int	i;

	Sync();
	i = ioctl(fdmemtest, MEMTESTBADSTORE, badmem);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("ioctl BADSTORE failed");
	}

	return i;
}

int
dokloadt()
{
	int	i;

	Sync();
	i = ioctl(fdmemtest, MEMTESTBADLOADT, badmem);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("ioctl BADLOADT failed");
	}

	return i;
}

int
dokfetcht()
{
	int	i;

	Sync();
	i = ioctl(fdmemtest, MEMTESTBADFETCHT, badmem);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("ioctl BADFETCHT failed");
	}

	return i;
}

int
douload()
{
	register int	c, d;

	c = u_data[0];
	u_data[0] = c;		/* touch the page */
	upar(&u_data[0]);
	Sync();
	d = u_data[0];		/* should crash here */

	fprintf(stderr, "test_memtest: didn't crash!?\n");
	fprintf(stderr, "\tbefore: 0x%x after: 0x%x\n", c, d);

	return(c ^ d);
}

int
douloadc()
{
	register int	c, d;
	register int	i;
	register char	*p, *q;

	p = u_data;
	p = (char *)((int)p & ~0xf);	/* cache-line boundary */
	q = p + 15;
	c = *p;
	*p = c;				/* touch the page */
	fprintf(stderr, "writing bad parity to 0x%x.\n", q);
	upar(q);
	Sync();
	for (i = 0; i < 16; i++) {
		d = p[i];	/* should eventually crash here */
	}

	fprintf(stderr, "test_memtest: didn't crash!?\n");
	fprintf(stderr, "\tbefore: 0x%x after: 0x%x\n", c, d);

	return(c ^ d);
}

int
douloadm()
{
	register int	c, d;
	register int	i;
	register char	*p, *q1, *q2;

	p = u_data;
	p = (char *)((int)p & ~0xf);	/* cache-line boundary */
	q1 = p + 6;
	q2 = p + 15;
	c = *p;
	*p = c;				/* touch the page */
	fprintf(stderr, "writing bad parity to 0x%x and 0x%x.\n",
		q1, q2);
	upar(q1);
	upar(q2);
	Sync();
	for (i = 0; i < 16; i++) {
		d = p[i];	/* should eventually crash here */
	}

	fprintf(stderr, "test_memtest: didn't crash!?\n");
	fprintf(stderr, "\tbefore: 0x%x after: 0x%x\n", c, d);

	return(c ^ d);
}

/* function used by doufetch */
extern int u_retl();

int
doufetch()
{
	register int	c, d;

	c = *(char *)u_retl;
	upar(u_retl);
	Sync();
	u_retl();		/* should crash here */
	fprintf(stderr, "test_memtest: didn't crash!?\n");

	d = *(char *)u_retl;	/* or at least here! */
	fprintf(stderr, "\tbefore: 0x%x after: 0x%x\n", c, d);

}

int
doustore()
{

	Sync();
	*badmem = 0;		/* should crash here */
	return 0;
}

int
doustorem0()
{

	Sync();
	doustorem_asm0(badmem);	/* should crash here */
	return 0;
}

int
doustorem1()
{

	Sync();
	doustorem_asm1(badmem);	/* should crash here */
	return 0;
}

int
doustorem2()
{

	Sync();
	doustorem_asm2(badmem);	/* should crash here */
	return 0;
}

int
doustorem3()
{

	Sync();
	doustorem_asm3(badmem);	/* should crash here */
	return 0;
}

int
douloadt()
{
	char	c;

	Sync();
	c = *badmem;		/* should crash here */
	return 0;
}

int
doufetcht()
{
	Sync();
	(*(int (*)())badmem)();		/* should crash here */
	return 0;
}

int
dodma()
{
	int	i;

	Sync();
	fileaddr[0] = 0x12345678;
	upar((caddr_t)fileaddr);
	fileaddr[1] = 0x9abcdef0;
	i = fsync(fdfiletest);		/* should crash here */

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("fsync failed");
	}

	return(i);
}

int
Sync()
{

	fflush(stderr);
	fsync(fileno(stderr));
	sync();
}

int
upar(addr)
	caddr_t	addr;
{
	int	i;

	i = ioctl(fdmemtest, MEMTESTBADPAR, addr);

	if (i != 0) {
		fprintf(stderr, "%s: ", progname);
		perror("couldn't set parity");
	}

	return i;
}

/*
 * Note: Do we have to be type 407 for this to work?
 * No; memtest.c has been improved to write to "read-only" pages.
 */
int
u_retl()
{
	return 0;
}

#include <signal.h>
handler(sig, code, scp, addr)
	int	sig;
	int	code;
	struct sigcontext	*scp;
	char	*addr;
{

	psignal(sig, "received signal");
	fprintf(stderr, " code=%d addr=0x%x\n", code, addr);
	longjmp(env, 1);
}

catchsigs()
{
	signal(SIGBUS, handler);
	signal(SIGSEGV, handler);
}
