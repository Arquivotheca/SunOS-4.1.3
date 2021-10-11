/*	@(#)modload.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sun/vddrv.h>
#include <nlist.h>
#include <fcntl.h>
#include <string.h>
#include <kvm.h>

char *rawinputfile = NULL;	/* input file as specified by the user */
char *inputfilename = NULL;	/* input file name without any extension */
char *outputfile = NULL;	/* name of output file */

char *unixfile = "/vmunix";	/* default kernel image to link with */
char *entrypoint;		/* module entry point routine */
char *execfile;			/* name of user file to exec after loading */
char *conffile;			/* name of user configuration file */

int dopt = 0;			/* debug option */
int sopt = 0;			/* option to generate a symbol table */
int vopt = 0;			/* verbose option */
int mod_id = 0;			/* module ID returned by driver */
u_long mod_addr;		/* virtual address at which module is loaded */
u_int mod_size;			/* size in bytes of module */
int vd;				/* file descriptor for /dev/vd */

void quit(), usage(), error(), fatal(), vinfo();
static caddr_t linkmodule();
static int checkversion();
static void getargs(), exec_userfile();
static void strip(), ld(), exec_ld();
extern struct vdconf *getconfinfo();
extern char *malloc();

/*
 * Load a module.
 */

main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	struct vdioctl_load vdiload;	/* structure used with VDLOAD ioctl */

	/*
	 * Get arguments
	 */
	getargs(argc, argv);

	/*
	 * Open the pseudo driver to load the module
	 */
	if ((vd = open("/dev/vd", O_RDWR)) < 0)
		error("can't open /dev/vd");

	/*
	 * Catch signals so we can cleanup
	 */
	(void) signal(SIGINT, quit);

	/*
	 * Link the module at its proper virtual address.
	 * Verify this it is being linked with the proper kernel.
	 * Generate the symbol table.
	 */
	vdiload.vdi_mmapaddr = linkmodule(vd, &vdiload.vdi_symtabsize);
	vdiload.vdi_id = mod_id;

	/*
	 * Get configuration information
	 */
	vdiload.vdi_userconf = getconfinfo(conffile);

	/*
	 * Now load the module into the system
	 */
	if (!sopt)
		strip(outputfile);

	if (ioctl(vd, VDLOAD, &vdiload) < 0)
		error("can't load this module");

	/*
	 * Exec the user's file (if any)
	 */
	if (execfile)
		exec_userfile(vd, mod_id, envp);

	printf("module loaded; id = %d\n", mod_id);
	exit(0);	/* success */
}

/*
 * Link the module at its proper virtual address.
 *
 * First we have to link the module (any address will do!) to find out its size.
 * This is because the size of the bss is not determined until the final link.
 * After we know the size, we can ask the kernel (VDGETVADDR ioctl) for the
 * next available address of this size.  Then we link the module at its
 * final virtual address.
 *
 * In order to get the actual module size, we have to generate the symbol
 * table here.
 */
static caddr_t
linkmodule(vd, symtabsize)
	int vd;
	u_int *symtabsize;
{
	register int fd;
	register caddr_t mmapaddr;

	struct vdioctl_getvaddr vdigetvaddr;
	struct stat statbuf;

	u_int tvaddr, tsize, toffset;
	u_int dvaddr, dsize, doffset;
	u_int bvaddr, bsize;

	/*
	 * Link at any address just so we can find out the size of the bss.
	 */
	ld((u_long)0);

	/*
	 * Make sure the version of unix that the module was linked with
	 * is the same as the running version of unix.
	 */
	if (checkversion() != 0)
		fatal(
"Module must be relinked with the running kernel (use modload -A option)\n");

	if (!sopt)
		strip(outputfile);

	if ((fd = open(outputfile, 0)) < 0)
		error("can't open %s", outputfile);

	if (fstat(fd, &statbuf) < 0)
		error("can't stat %s", outputfile);

	/*
	 * mmap the module image file
	 */
	mmapaddr = mmap((caddr_t)0, (int)statbuf.st_size,
			PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, (off_t)0);
	if (mmapaddr == (caddr_t)-1)
		error("can't mmap %s", outputfile);

	readhdr(mmapaddr, &tvaddr, &tsize, &toffset,
			&dvaddr, &dsize, &doffset, &bvaddr, &bsize);
	checkhdr(tvaddr, tsize, toffset, dvaddr, dsize, doffset, bvaddr, bsize);

	vdigetvaddr.vdi_size = (bvaddr - tvaddr + bsize);

	if (sopt) {
		/*
		 * Generate symbol table and get its size.
		 */
		vdigetvaddr.vdi_size +=
			getsymtab(outputfile, mmapaddr,
			(u_long)0, vdigetvaddr.vdi_size);
	}

	close(fd);		/* close and munmap the image */

	/*
	 * Now that we know the module size, we ask the kernel for
	 * the virtual address at which to link the module.
	 */
	if (ioctl(vd, VDGETVADDR, &vdigetvaddr) < 0)
		error("can't get a virtual address for this module");

	/*
	 * Add header sizes to the virtual address.  This is the final
	 * link address for the module.
	 */
	vdigetvaddr.vdi_vaddr += gethdrsize();

	/*
	 * Save module info for later use
	 */
	mod_id = vdigetvaddr.vdi_id;
	mod_addr = vdigetvaddr.vdi_vaddr;
	mod_size = vdigetvaddr.vdi_size;
	vinfo("module id = %d, vaddr = %x, size = %x\n",
		mod_id, mod_addr, mod_size);

	ld(mod_addr);	/* link the module again */

	if ((fd = open(outputfile, 0)) < 0)
		error("can't open %s", outputfile);

	if (fstat(fd, &statbuf) < 0)
		error("can't stat %s", outputfile);

	/*
	 * mmap the module image file again now that we've relinked.
	 */
	mmapaddr = mmap((caddr_t)0, (int)statbuf.st_size,
			PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, (off_t)0);
	if (mmapaddr == (caddr_t)-1)
		error("can't mmap %s", outputfile);

	*symtabsize = (sopt) ?
			getsymtab(outputfile, mmapaddr, mod_addr, mod_size)
			: 0;
	return (mmapaddr);
}

/*
 * Exec "ld" to link the module at a specified virtual address.
 */
static void
ld(vaddr)
	u_long vaddr;
{
	int child;
	union wait status;

	vinfo("Linking the module...\n");

	child = fork();
	if (child == -1)
		error("can't fork ld");
	else if (child == 0)
		exec_ld(vaddr);
	else {
		wait(&status);		/* wait for ld to finish */
		if (status.w_retcode != 0) {
			vinfo("ld failed, status = %x\n", status.w_retcode);
			exit(-2);	/* ld already printed messages */
		}
	}
}

static void
strip(file)
	char *file;
{
	int child;
	union wait status;

	vinfo("Removing symbol table...\n");

	child = fork();
	if (child == -1)
		error("can't fork strip");
	else if (child == 0) {
		execlp("strip", "strip", file, (char *)0);
		error("couldn't exec strip");
	} else {
		wait(&status);		/* wait for ld to finish */
		if (status.w_retcode != 0) {
			vinfo("ld failed, status = %x\n", status.w_retcode);
			exit(-2);	/* ld already printed messages */
		}
	}
}

/*
 * Exec strip to remove symbol table
 */
#ifdef sun386
#define	EXECTYPE	"-z"
#define	DEFAULTENTRY	"xxxinit"
#else
#define	EXECTYPE	"-N"
#define	DEFAULTENTRY	"_xxxinit"
#endif sun386

static void
exec_ld(vaddr)
	u_long vaddr;
{
	char addr[12];

	sprintf(addr, "%lx", vaddr);
	if (entrypoint == NULL)
		entrypoint = DEFAULTENTRY;

	vinfo("ld -o %s -Bstatic -T %s -A %s %s -e %s %s %s\n",
		    outputfile, addr, unixfile, EXECTYPE, entrypoint,
		    sopt == 0 ? "-s": "", rawinputfile);

	execlp("ld", "ld", "-o", outputfile, "-Bstatic", "-T", addr,
		"-A", unixfile, EXECTYPE, "-e", entrypoint,
		rawinputfile, (char *)0);
	error("couldn't exec ld");
}

/*
 * Make sure that the module is being linked with the running kernel.
 *
 * The module has been linked with the kernel named <unixfile>.
 * This routine finds the version string in <unixfile> and compares
 * it to the version string in memory.  They must match.
 */

static struct nlist nl[] = {
#ifdef COFF
	{ "version" },
#else
	{ "_version"},
#endif
	"",
};

#define	MAXVERSIONLEN 256	/* max length of kernel version string */

static int
checkversion()
{
	kvm_t *kd;
	int kmemf, unixf;
	char *kaddr, *uaddr;
	u_int datavaddr, dataoffset, voff;

	struct stat statbuf;	/* file status buffer */

	if ((kmemf = open("/dev/kmem", 0)) < 0)
		error("can't open /dev/kmem");

	if ((unixf = open(unixfile, 0)) < 0)
		error("can't open %s", unixfile);

	if (fstat(unixf, &statbuf) < 0)
		error("can't stat %s", unixfile);

	kd = kvm_open(unixfile, (char *)NULL, (char *)NULL,
			O_RDONLY, "kvm_open");
	if (kd == NULL)
		error("modload failed");

	if (kvm_nlist(kd, nl) < 0)
		fatal("bad namelist in kernel symbol table\n");

	if (nl[0].n_value == 0)
		fatal("missing 'version' symbol in kernel symbol table\n");

	uaddr = mmap((caddr_t)0, (int)statbuf.st_size, PROT_READ,
			MAP_PRIVATE, unixf, (off_t)0);

	if (uaddr == (caddr_t)-1)
		error("can't mmap %s", unixfile);

	kaddr = mmap((caddr_t)0, 2 * MMU_PAGESIZE, PROT_READ, MAP_SHARED,
		    kmemf, (off_t)(nl[0].n_value & ~PAGEOFFSET));

	if (kaddr == (caddr_t)-1)
		error("can't mmap /dev/kmem");

	/* get data virtual address and offset */
	getkerneldata(uaddr, &datavaddr, &dataoffset);

	/* offset of version in image file */
	voff = nl[0].n_value - datavaddr + dataoffset;

	if (strncmp(uaddr + voff, kaddr + (nl[0].n_value & PAGEOFFSET),
		    MAXVERSIONLEN) != 0)
		return (-1);

	return (0);
}

/*
 * Exec the user's file
 */
static void
exec_userfile(vd, id, envp)
	int vd;
	int id;
	char **envp;
{
	struct vdioctl_stat vdistat;
	struct vdstat vdstat;

	int child;
	union wait status;
	char module_id[8];
	char magic[16];
	char blockmajor[8];
	char charmajor[8];
	char sysnum[8];

	child = fork();
	if (child == -1)
		error("can't fork %s", execfile);
	/*
	 * exec the user program.
	 */
	if (child == 0) {
		vdistat.vdi_vdstat = &vdstat;			/* status buf */
		vdistat.vdi_vdstatsize = sizeof (struct vdstat); /* size */
		vdistat.vdi_id = id; 				/* module id */
		vdistat.vdi_statsubfcn = SUNSTATSUBFCN; 	/* subfcn */

		if (ioctl(vd, VDSTAT, &vdistat) < 0)
			error("can't get module status");

		sprintf(module_id, "%d", vdstat.vds_id);
		sprintf(magic, "%x", vdstat.vds_magic);

		if (vdstat.vds_magic == VDMAGIC_DRV ||
		    vdstat.vds_magic == VDMAGIC_MBDRV ||
		    vdstat.vds_magic == VDMAGIC_PSEUDO) {
			sprintf(blockmajor, "%d", vdstat.vds_modinfo[0] & 0xff);
			sprintf(charmajor,  "%d", vdstat.vds_modinfo[1] & 0xff);
			execle(execfile, execfile, module_id, magic, blockmajor,
				charmajor, (char *)0, envp);
		} else if (vdstat.vds_magic == VDMAGIC_SYS) {
			sprintf(sysnum, "%d", vdstat.vds_modinfo[0] & 0xff);
			execle(execfile, execfile, module_id, magic,
				sysnum, (char *)0, envp);
		} else {
			execle(execfile, execfile, module_id, magic,
				(char *)0, envp);
		}

		/* Shouldn't get here if execle was successful */

		error("couldn't exec %s", execfile);
	} else {
		wait(&status);		/* wait for exec'd program to finish */
		if (status.w_retcode != 0) {
			vinfo("%s returned error %d.\n", status.w_retcode);
			exit(-2);
		}
	}
}

/*
 * Get user arguments.
 */
static void
getargs(argc, argv)
	int argc;
	char *argv[];
{
	char *ap, *ext;

	if (argc < 2)
		usage();

	argc--;			/* skip program name */
	argv++;

	while (argc--) {
		ap = *argv++;	/* point at next option */
		if (*ap++ == '-') {
			if (strcmp(ap, "conf") == 0) {
				if (--argc < 0)
					usage();
				conffile = *argv++;
			} else if (strcmp(ap, "d") == 0) {
				dopt = 1;
			} else if (strcmp(ap, "sym") == 0) {
				sopt = 1;
			} else if (strcmp(ap, "entry") == 0) {
				if (--argc < 0)
					usage();
				entrypoint = *argv++;
			} else if (strcmp(ap, "o") == 0) {
				if (--argc < 0)
					usage();
				outputfile = *argv++;
			} else if (strcmp(ap, "A") == 0) {
				if (--argc < 0)
					usage();
				unixfile = *argv++;
			} else if (strcmp(ap, "v") == 0) {
				vopt = 1;
			} else if (strcmp(ap, "exec") == 0) {
				if (--argc < 0)
					usage();
				execfile = *argv++;
			} else {
				usage();
			}
		} else if (rawinputfile == NULL) { /* module file */
			rawinputfile = *--argv;
			argv++;
		} else
			usage();
	}

	if (rawinputfile == NULL)
		usage();

	ext = strrchr(rawinputfile, '.');
	if (ext == NULL || strcmp(&ext[1], "o") != 0)
		fatal("file_name must end with .o\n");

	inputfilename = malloc((u_int)(ext - rawinputfile + 1));
	if (inputfilename == NULL)
		error("malloc");
	(void) strncpy(inputfilename, rawinputfile, ext - rawinputfile);

	if (outputfile == NULL)
		outputfile = inputfilename;

	vinfo("infile %s, infilename %s, outfile %s, execfile %s\n",
		rawinputfile, inputfilename, outputfile, execfile);
}

void
usage()
{
	printf("usage:  modload filename [options]\n");
	printf("	where options are:\n");
	printf("	-A <vmunix_file>\n");
	printf("	-conf <configuration_file>\n");
	printf("	-entry <entry_point>\n");
	printf("	-exec <exec_file>\n");
	printf("	-o <output_file>\n");
	printf("	-sym\n");			/* keep symbols */
	exit(-1);
}

void
quit()
{
	struct vdioctl_freevaddr vdi;

	vdi.vdi_id = mod_id;
	(void) ioctl(vd, VDFREEVADDR, (caddr_t)&vdi);
	exit(-1);
}

#include <varargs.h>

/*VARARGS0*/
void
vinfo(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	if (!vopt)
		return;

	va_start(args);
	fmt = va_arg(args, char *);
	vprintf(fmt, args);
}
