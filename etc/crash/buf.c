#ifndef lint
static	char sccsid[] = "@(#)buf.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:buf.c	1.12.2.1"
/*
 * This file contains code for the crash functions: bufhdr, buffer, od.
 */

#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/buf.h"
#include "sys/dirent.h"
#include "sys/vnode.h"
#include "ufs/inode.h"
#include "ufs/fs.h"
#include "machine/param.h"
#include "crash.h"

#define SBUFSIZE PAGESIZE

#define BSZ  1		/* byte size */
#define SSZ  2		/* short size */
#define LSZ  4		/* long size */

static struct nlist *Buf;	/* namelist symbol pointer */
static char bformat = 'x';	/* buffer format */
static int type = LSZ;		/* od type */
static char mode = 'x';		/* od mode */
char buffer[SBUFSIZE];		/* buffer buffer */
struct	buf bbuf;		/* used by buffer for bufhdr */
extern char *ctime();
extern long lseek();
unsigned bufv;

/* get arguments for bufhdr function */
int
getbufhdr()
{
	int full = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *heading = "    ADDR MAJ/MIN  BLOCK  ADDRESS      FOR      BCK      AVF      AVB FLAGS\n";
	optind = 1;
	while((c = getopt(argcnt, args, "fpw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (!full)
		fprintf(fp, "%s", heading);
	if (args[optind]) 
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 == -1)
				continue;
			else addr = arg1;
			prbufhdr(full, phys, addr, heading);
			addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	else prbufhdr(full, phys, addr, heading);
}

/* print buffer headers */
int
prbufhdr(full, phys, addr, heading)
int full, phys;
long addr;
char *heading;
{
	struct buf bhbuf;
	register int b_flags;
	int procslot;
	extern unsigned procv, nproc;
	int all = 0;

	if (addr == -1) {
		all = 1;
		addr = bufv;
	}
	while (1) {
		readbuf(addr, addr, phys, -1,
			(char *)&bhbuf, sizeof bhbuf, "buffer header");
		if (full)
			fprintf(fp, "%s", heading);
		fprintf(fp, "%8x", addr);
		fprintf(fp, " %3u, %-3u %5x %8x",
			major(bhbuf.b_dev)&0377,
			minor(bhbuf.b_dev),
			bhbuf.b_blkno,
			bhbuf.b_un.b_addr);
		fprintf(fp, " %8x %8x %8x %8x",
			bhbuf.b_forw,
			bhbuf.b_back,
			bhbuf.av_forw,
			bhbuf.av_back);
		b_flags = bhbuf.b_flags;
		fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			b_flags == B_WRITE ? " write" : "",
			b_flags & B_READ ? " read" : "",
			b_flags & B_DONE ? " done" : "",
			b_flags & B_ERROR ? " error" : "",
			b_flags & B_BUSY ? " busy" : "",
			b_flags & B_PHYS ? " phys" : "",
			b_flags & B_PAGEIO ? " pageio" : "",
			b_flags & B_WANTED ? " wanted" : "",
			b_flags & B_DONTNEED ? " dontneed" : "",
			b_flags & B_ASYNC ? " async" : "",
			b_flags & B_TAPE ? " tape" : "",
			b_flags & B_REMAPPED ? " remapped" : "",
			b_flags & B_FREE ? " free" : "",
			b_flags & B_PGIN ? " pgin" : "",
			b_flags & B_CACHE ? " cache" : "",
			b_flags & B_INVAL ? " inval" : "",
			b_flags & B_FORCE ? " force" : "",
			b_flags & B_HEAD ? " head" : "",
			b_flags & B_NOCACHE ? " nocache" : "",
			b_flags & B_BAD ? " bad" : "");
		if (full) {
			fprintf(fp, "BCNT SIZE ERR RESI PROC  IODONE    VNODE     PAGE    CHAIN   MBINFO\n");
			fprintf(fp, "%4d %4d %3d %4d",
				bhbuf.b_bcount,
				bhbuf.b_bufsize,
				bhbuf.b_error,
				bhbuf.b_resid);
			procslot = ((long)bhbuf.b_proc - procv)/
				sizeof (struct proc);
			if ((procslot >= 0) && (procslot < nproc))
				fprintf(fp, " %4d", procslot);
			else fprintf(fp, "  - ");
			fprintf(fp, " %8x %8x %8x %8x %4x\n\n",
				bhbuf.b_iodone,
				bhbuf.b_vp,
				bhbuf.b_pages,
				bhbuf.b_chain,
				bhbuf.b_mbinfo);
		}
		if (!all) break;
		addr = (int)bhbuf.b_chain;
		if (!addr) break;
	}
}

/* get arguments for buffer function */
int
getbuffer()
{
	int phys = 0;
	int fflag = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;

	optind = 1;
	while((c = getopt(argcnt, args, "bcdrxiospw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'b' :	bformat = 'b';
					fflag++;
					break;
			case 'c' :	bformat = 'c';
					fflag++;
					break;
			case 'd' :	bformat = 'd';
					fflag++;
					break;
			case 'x' :	bformat = 'x';
					fflag++;
					break;
			case 'i' :	bformat = 'i';
					fflag++;
					break;
			case 'r' :	bformat = 'r';
					fflag++;
					break;
			case 'o' :	bformat = 'o';
					fflag++;
					break;
			case 's' :	bformat = 's';
					fflag++;
					break;
			default  :	longjmp(syn, 0);
					break;
		}
	}
	if (fflag > 1)
		longjmp(syn, 0);
	if (args[optind]) {
		getargs(0xffffffff, &arg1, &arg2);
		if (arg1 != -1) {
			addr = arg1;
			prbuffer(phys, addr);
		}
	}
	else longjmp(syn, 0);
}

/* print buffer */
int
prbuffer(phys, addr)
int phys;
long addr;
{

	readbuf(addr, addr, phys, -1,
		(char *)&bbuf, sizeof bbuf, "buffer");

	readmem((long)bbuf.b_un.b_addr, 1, -1, (char *)buffer,
		sizeof buffer, "buffer");
	switch(bformat) {
		case 'b' :	prbalpha();
				break;
		case 'c' :	prbalpha();
				break;
		case 'd' :	prbnum();
				break;
		case 'x' :	prbnum();
				break;
		case 'i' :	prbinode();
				break;
		case 'r' :	prbdirect();
				break;
		case 'o' :	prbnum();
				break;
		case 's' :	prbsblock();
				break;
		default  :	error("unknown format\n");
				break;
	}
}

/* print buffer in numerical format */
int
prbnum()
{
	int *ip, i;

	for (i = 0, ip=(int *)buffer; ip !=(int *)&buffer[SBUFSIZE]; i++, ip++) {
		if (i % 4 == 0)
			fprintf(fp, "\n%5.5x:\t", i*4);
		fprintf(fp, bformat == 'o'? " %11.11o" :
			bformat == 'd'? " %10.10u" : " %8.8x", *ip);
	}
	fprintf(fp, "\n");
}

/* print buffer in character format */
int
prbalpha()
{
	char *cp;
	int i;

	for (i=0, cp = buffer; cp != &buffer[SBUFSIZE]; i++, cp++) {
		if (i % (bformat == 'c' ? 16 : 8) == 0)
			fprintf(fp, "\n%5.5x:\t", i);
		if (bformat == 'c') putch(*cp);
		else fprintf(fp, " %4.4o", *cp & 0377);
	}
	fprintf(fp, "\n");
}

/* print buffer in inode format */
int
prbinode()
{
	struct	dinode	*dip;
	long	_3to4();
	int	i, j;

	for (i=1, dip = (struct dinode *)buffer;dip <
		 (struct dinode*)&buffer[SBUFSIZE]; i++, dip++) {
	fprintf(fp, "\ni#: %ld  md: ", (bbuf.b_blkno - 2) *
		(SBUFSIZE / sizeof (struct dinode)) + i);
		switch(dip->di_mode & IFMT) {
		case IFCHR: fprintf(fp, "c"); break;
		case IFBLK: fprintf(fp, "b"); break;
		case IFDIR: fprintf(fp, "d"); break;
		case IFREG: fprintf(fp, "f"); break;
		case IFIFO: fprintf(fp, "p"); break;
		case IFLNK: fprintf(fp, "l"); break;
		case IFSOCK: fprintf(fp, "s"); break;
		default:    fprintf(fp, "-"); break;
		}
		fprintf(fp, "\n%s%s%s%3o",
			dip->di_mode & ISUID ? "u" : "-",
			dip->di_mode & ISGID ? "g" : "-",
			dip->di_mode & ISVTX ? "t" : "-",
			dip->di_mode & 0777);
		fprintf(fp, "  ln: %u  uid: %u  gid: %u  sz: %ld",
			dip->di_nlink, dip->di_uid,
			dip->di_gid, dip->di_size);
		fprintf(fp, "\nblocks: %u gen: %u",
			dip->di_blocks, dip->di_gen);
		if ((dip->di_mode & IFMT) == IFCHR ||
			(dip->di_mode & IFMT) == IFBLK ||
			(dip->di_mode & IFMT) == IFIFO)
			fprintf(fp, "\nmaj: %d  min: %1.1o\n",
				dip->di_db[0] & 0377,
				dip->di_db[1] & 0377);
		else {
			for (j = 0; j < NDADDR; j++) {
				if (j % 7 == 0)
					fprintf(fp, "\n");
				fprintf(fp, "a%d: %ld  ", j,
					_3to4(&dip->di_db[3 * j]));
			}
			for (j = 0; j < NIADDR; j++) {
				if (j % 7 == 0)
					fprintf(fp, "\n");
				fprintf(fp, "i%d: %ld  ", j,
					_3to4(&dip->di_ib[3 * j]));
			}
		}
	
		fprintf(fp, "\nat: %s", ctime(&dip->di_atime));
		fprintf(fp, "mt: %s", ctime(&dip->di_mtime));
		fprintf(fp, "ct: %s", ctime(&dip->di_ctime));
	}
	fprintf(fp, "\n");
}

/* print buffer in directory format */
int
prbdirect()
{
	struct	dirent	*dp;
	int	i, bad;
	char *cp;

	fprintf(fp, "\n");
	for (i = 0, dp = (struct dirent *)buffer;dp <
		(struct dirent *)&buffer[SBUFSIZE];i++, dp++) {
		bad = 0;
		for (cp = dp->d_name; cp != &dp->d_name[dp->d_namlen]; cp++)
			if ((*cp < 040 || *cp > 0176) && *cp != '\0')
				bad++;
		fprintf(fp, "d%2d: %5u  ", i, dp->d_fileno);
		if (bad) {
			fprintf(fp, "unprintable: ");
			for (cp = dp->d_name; cp != &dp->d_name[dp->d_namlen];
				cp++)
				putch(*cp);
		} else fprintf(fp, "%.14s", dp->d_name);
		fprintf(fp, "\n");
	}
}

/* convert 3 byte disk block address to 4 byte address */
long
_3to4(ptr)
register  char  *ptr;
{
	long retval;
	register  char  *vptr;

	vptr = (char *)&retval;
	*vptr++ = 0;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	*vptr++ = *ptr++;
	return(retval);
}

/* print buffer in superblock format */
prbsblock()
{
	register struct fs *p = (struct fs *)buffer;
	register i, j, k;

	if (p->fs_magic != FS_MAGIC) fprintf("this superblock looks corrupted\n");
	fprintf(fp, "forw %8x back %8x\n",
		p->fs_link,
		p->fs_rlink);
	fprintf(fp, "sblock %6d cblock %6d iblock %6d dblock %6d\n",
		p->fs_sblkno,
		p->fs_cblkno,
		p->fs_iblkno,
		p->fs_dblkno);
	fprintf(fp, "cg offset %4d cg mask %8x\n",
		p->fs_cgoffset,
		p->fs_cgmask);
	fprintf(fp, "time last written %s", ctime(&p->fs_time));	
	fprintf(fp, "size %6d data %6d cylinder groups %6d\n",
		p->fs_size,
		p->fs_dsize,
		p->fs_ncg);
	fprintf(fp, "block size %4d frag size %4d frag/block %4d\n",
		p->fs_bsize,
		p->fs_fsize,
		p->fs_frag);
	fprintf(fp, "min free %3d rot delay %4d disk rev/sec %4d\n",
		p->fs_minfree,
		p->fs_rotdelay,
		p->fs_rps);
	fprintf(fp, "blk mask %8x frag mask %8x blk shift %3d frag shift %3d\n",
		p->fs_bmask,
		p->fs_fmask,
		p->fs_bshift,
		p->fs_fshift);
	fprintf(fp, "max contig %4d max blk/cg %4d\n",
		p->fs_maxcontig,
		p->fs_maxbpg);
	fprintf(fp, "frag shift %4d fsbtodb %4d actual sblock size %4d\n",
		p->fs_fragshift,
		p->fs_fsbtodb,
		p->fs_sbsize);
	fprintf(fp, "csum blk offset %4d csum blk number %4d nindir %4d\n",
		p->fs_csmask,
		p->fs_csshift,
		p->fs_nindir);
	fprintf(fp, "inode/blk %4d nspf %4d optimized for: %s\n",
		p->fs_inopb,
		p->fs_nspf,
		(p->fs_optim == FS_OPTTIME) ? "time" : "space");
	fprintf(fp, "cg summary addr %6d cg summary size %4d cg size %4d\n",
		p->fs_csaddr,
		p->fs_cssize,
		p->fs_cgsize);
	fprintf(fp, "track/cyl %4d sec/track %4d sec/cyl %4d\n",
		p->fs_ntrak,
		p->fs_nsect,
		p->fs_spc);
	fprintf(fp, "cyl/fs %8d cyl/grp %4d inode/grp %4d frag/grp %4d\n",
		p->fs_ncyl,
		p->fs_cpg,
		p->fs_ipg,
		p->fs_fpg);
	fprintf(fp, "directories %6d free blocks %6d free inodes %6d free frags %6d\n",
		p->fs_cstotal.cs_ndir,
		p->fs_cstotal.cs_nbfree,
		p->fs_cstotal.cs_nifree,
		p->fs_cstotal.cs_nffree);
	fprintf(fp, "mod %2x clean %2x readonly %2x\n",
		p->fs_fmod,
		p->fs_clean,
		p->fs_ronly);
	fprintf(fp, "mounted on: %s\n", p->fs_fsmnt);
	fprintf(fp, "cg rotor %6d cpc %4d magic %8x\n",
		p->fs_cgrotor,
		p->fs_cpc,
		p->fs_magic);
	
	fprintf(fp, "cyl summary ptrs:\n");
	for (i = 0; i < MAXCSBUFS; i++) {
		if (p->fs_csp[i] == NULL) continue;
		fprintf(fp, "%d: %8x\n", i, p->fs_csp[i]);
	}
	fprintf(fp, "pos table:\n");
	for (i = 0, k = 0; i < p->fs_cpg; i++) {
		for (j = 0; j < p->fs_nrpos; j++) {
			if (fs_postbl(p, i)[j] == -1) continue;
			if (k && ((k % 8) == 0)) printf("\n");
			k++;
			fprintf(fp, "%2d,%2d: %2d ", i, j, fs_postbl(p, i)[j]);
		}
	}
}

/* get arguments for od function */
int
getod()
{
	int phys = 0;
	int count = 1;
	int proc = Procslot;
	long addr = -1;
	int c;
	struct nlist *sp;
	int typeflag = 0;
	int modeflag = 0;

	optind = 1;
	while((c = getopt(argcnt, args, "tlxcbdohapw:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			case 'p' :	phys = 1;
					break;
			case 'c' :	mode = 'c';
					if (!typeflag)
						type = BSZ;
					modeflag++;
					break;
			case 'a' :	mode = 'a';
					if (!typeflag)
						type = BSZ;
					modeflag++;
					break;
			case 'x' :	mode = 'x';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'd' :	mode = 'd';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'o' :	mode = 'o';
					if (!typeflag)
						type = LSZ;
					modeflag++;
					break;
			case 'h' :	mode = 'h';
					type = LSZ;
					typeflag++;
					modeflag++;
					break;
			case 'b' :	type = BSZ;
					typeflag++;
					break;
			case 't' :	type = SSZ;
					typeflag++;
					break;
			case 'l' :	type = LSZ;
					typeflag++;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (typeflag > 1) 
		error("only one type may be specified:  b, t, or l\n");	
	if (modeflag > 1) 
		error("only one mode may be specified:  a, c, o, d, or x\n");	
	if (args[optind]) {
		if (*args[optind] == '(') 
			addr = eval(++args[optind]);
		else if (sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if (isasymbol(args[optind]))
			error("%s not found in symbol table\n", args[optind]);
		else addr = strcon(args[optind], 'h');	
		if (addr == -1)
			error("\n");
		if (args[++optind]) 
			if ((count = strcon(args[optind], 'h')) == -1)
				error("\n");
		prod(addr, count, phys, proc);
	}
	else longjmp(syn, 0);
}

/* print dump */
int
prod(addr, count, phys, proc)
long addr;
int count, phys, proc;
{
	int i, j;
	long padr;
	char ch;
	unsigned short shnum;
	long lnum;
	long value;
	char *format;
	int precision;
	char hexchar[16];
	char *cp;
	int nbytes;

	padr = addr;
	switch(type) {
		case LSZ:
			if (addr & 0x3) {	/* word alignment */
				fprintf(fp, "warning: word alignment performed\n");
				addr &= ~0x3;
			}
		case SSZ:
			if (addr & 0x1) {	/* word alignment */
				fprintf(fp, "warning: word alignment performed\n");
				addr &= ~0x1;
			}
	}
	if (phys)
		if (lseek(mem, padr, 0) == -1)
			error("%8x is out of range\n", addr);
	if (mode == 'h') {
		cp = hexchar;
		nbytes = 0;
	}
	for (i = 0; i < count; i++) {
		switch (type) {
			case BSZ: 
				if (phys) {
					if (read(mem, &ch, sizeof (ch)) != sizeof (ch))
						error("read error in buffer\n");
					}
				else readmem(addr, 1, proc, &ch, sizeof (ch), "buffer");
			        value = ch & 0377;
				addr += sizeof(ch);
				break;
			case SSZ: 
				if (phys) {
					if (read(mem, &shnum, sizeof (short)) != sizeof (short))
						error("read error in buffer\n");
					}
				else readmem(addr, 1, proc, (char *)&shnum, sizeof (short), "buffer");
				value = shnum;
				addr += sizeof(short);
				break;	
			case LSZ:
				if (phys) {
					if (read(mem, &lnum, sizeof (long)) != sizeof (long))
						error("read error in buffer\n");
					}
				else readmem(addr, 1, proc, (char *)&lnum, sizeof (long), "buffer");
				value = lnum;
				addr += sizeof(long);
				break;
		}
		if (((mode == 'c') && ((i % 16) == 0)) ||
			((mode != 'a') && (mode != 'c') && (i % 4 == 0))) {
				if (i != 0) {
					if (mode == 'h') {
						fprintf(fp, "   ");
						for (j = 0; j < nbytes; j++) {
							if (hexchar[j] < 040 ||
							hexchar[j] > 0176)
								fprintf(fp, ".");
							else fprintf(fp, "%c",
								hexchar[j]);
						}
						cp = hexchar;
						nbytes = 0;
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "%8x:  ", addr - type);
			}
		switch(mode) {
			case 'a' :  switch(type) {
					case BSZ :  putc(ch, fp);
						    break;
					case SSZ :  putc((char)shnum, fp);
						    break;
					case LSZ :  putc((char)lnum, fp);
						    break;
				    }
				    break;
			case 'c' :  switch(type) {
					case BSZ :  putch(ch);
						    break;
					case SSZ :  putch((char)shnum);
						    break;
					case LSZ :  putch((char)lnum);
						    break;
				    }
				    break;
			case 'o' :  format = "%.*o   ";
				    switch(type) {
					case BSZ :  precision = 3;
						    break;
					case SSZ :  precision = 6;
						    break;
					case LSZ :  precision = 11;
						    break;
			   		}
			 	    fprintf(fp, format, precision, value);
			 	    break;
			case 'd' :  format = "%.*d   ";
				    switch(type) {
					case BSZ :  precision = 3;
						    break;
					case SSZ :  precision = 5;
						    break;
					case LSZ :  precision = 10;
						    break;
				    }
			 	    fprintf(fp, format, precision, value);
			   	    break;
			case 'x' :  format = "%.*x   ";
				    switch(type) {
					case BSZ :  precision = 2;
						    break;
					case SSZ :  precision = 4;
						    break;
					case LSZ :  precision = 8;
						    break;
				    }
			 	    fprintf(fp, format, precision, value);
				    break;
			case 'h' :  fprintf(fp, "%.*x   ", 8, value);
				    *((long *)cp) = value;
				    cp +=4;
				    nbytes += 4;
				    break;
		}
	}
	if (mode == 'h') {
		if (i % 4 != 0)  
			for (j = 0; (j+(i%4)) < 4; j++)
				fprintf(fp, "           ");
		fprintf(fp, "   ");
		for (j = 0; j < nbytes; j++) 
			if (hexchar[j] < 040 || hexchar[j] > 0176)
				fprintf(fp, ".");
			else fprintf(fp, "%c", hexchar[j]);
	}
	fprintf(fp, "\n");
}
