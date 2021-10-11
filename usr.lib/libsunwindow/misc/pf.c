#ifdef sccs
#ifndef lint
static char sccsid[] = "@(#)pf.c 1.1 92/07/30 SMI from pf.c 1.20 87/12/18";
#endif
#endif

/*
 * Copyright 1986-1989 Sun Microsystems, Inc.
 */

/*
 * Pixfont open/close: pf_open(), pf_open_private(), pf_close()
 */

#include <vfont.h>
#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>

#ifndef MP_FONT	/* in case we have the wrong memvar.h */
#define	MP_FONT		32	/* Pixrect is a part of a Pixfont */
#endif

#ifndef	NO_SHARED_LIB_FONTS
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <link.h>

extern int			errno;
extern struct link_dynamic	_DYNAMIC;

extern int			getpagesize();
extern char			*getenv();
#endif

extern char *calloc(), *malloc();

extern Pixfont *pf_resident(), *pf_bltindef();
static Pixfont *pf_load_vfont();

#ifndef	NO_SHARED_LIB_FONTS
/*	Following added to allow a master program (e.g. sunview)
 * to change allocation for fonts to force them into particular VM addresses
 * so that slave programs (e.g. shelltool) can try to share the same fonts.
 * This only works if both master and slave are linked against the same set
 * of shared libraries with the same initial stack sizes (the common case),
 * but mem_ops fields exist to allow this condition to be checked.
 *	_use_vm has the following meanings (assuming pf_linked_dynamic() is
 * true, otherwise we have to use private fonts):
 * 0: slave, try to get font from vm
 * 1: master, try to put font into vm
 * 2: either, vm did not work, don't try it any longer.
 */
static	int _use_vm; 
static	int vm_fd;		/* Only valid while master loading fonts */
static	char *next_free, *end_of_vm_blk;

typedef	struct pixrectops Pr_ops;
#define PF_VM_VERSION	1
#define MAX_FONT_COUNT	4
typedef struct pf_vm_header {
	int		 version;	 /* Should be PF_VM_VERSION */
	caddr_t		 mapped_at;	 /* Assigned VM addr. */
	int		 size_of_vm_blk; /* In bytes, including header */
	Pr_ops		*mem_ops;	 /* Must match or don't share */
	Pr_ops		 mem_ops_procs;  /*   fonts as VM layout bad */
	int		 font_count;	 /* No. of valid fonts */
	Pixfont		*fonts[MAX_FONT_COUNT];
	char		 font_names[MAX_FONT_COUNT][1024];
	int		 vm_blk[1];	 /* Lie: actually much bigger */
} *Pf_vm_header;
static	Pf_vm_header vm_header;

#define	VM_FONT_FILE	"/usr/tmp/vm_fonts-"
#endif

/* shared font list */
static struct font_use {
	struct font_use *next;
	char *name;
	Pixfont *pf;
	int count;
	int resident;
} Fonts[1];


/* public functions */
#ifndef NO_SHARED_LIB_FONTS
int
pf_linked_dynamic()
{
	return (&_DYNAMIC != (struct link_dynamic *)0);
}

int
pf_use_vm(use_vm)
	int	use_vm;
{
	int	result = _use_vm;

	/* Make sure that we actually got built with shared libraries */
	if (pf_linked_dynamic()) {
		if (_use_vm == 1 && use_vm == 0 && vm_fd > 0) {
			/*
			 * Turning off vm => close backing file.
			 * Change permissions to make it read only for others.
			 * (We still retain write access.)
			 */
			if (fchmod(vm_fd, 0444) < 0) {
				perror("fchmod");
			}
			close(vm_fd);
			vm_fd = 0;
		}
		_use_vm = use_vm ? 1 : 0;

	}
	return result;
}
#endif

Pixfont *
pf_open(fontname)
	char *fontname;
{
	register struct font_use *use, *prev;
	register char *basename;
#ifndef NO_SHARED_LIB_FONTS
	int		from_vm;
#endif

	if (fontname == 0) 
		return pf_default();

	if (basename = rindex(fontname, '/'))
		basename++;
	else
		basename = fontname;

	/* look for the font in the shared font list */
	for (prev = Fonts; use = prev->next; prev = use) 
		if (strcmp(use->name, basename) == 0) {
			/* found it */
			use->count++;
			return use->pf;
		}

	/* didn't find it, add it to the end of the list */
	if (use = (struct font_use *) 
		malloc((unsigned) (sizeof *use + strlen(basename) + 1))) {
		use->next = 0;
		(void) strcpy(use->name = (char *) (use + 1), basename);
		use->count = 1;
		use->resident = 0;

		if (use->pf = pf_resident(basename))
			use->resident = 1;
		else {
#ifndef NO_SHARED_LIB_FONTS
			use->pf = pf_load_vfont(fontname, &from_vm);
			use->resident = from_vm;
#else
			use->pf = pf_load_vfont(fontname);
#endif
		}
		if (use->pf) {
			prev->next = use;
			return use->pf;
		}
		else
			(void) free((char *) use);
	}

	return 0;
}

Pixfont *
pf_open_private(fontname)
	char *fontname;
{
#ifndef NO_SHARED_LIB_FONTS
	/* Set allocator to calloc() to force private copy of font. */
	int		save_use_vm = _use_vm;
	int		from_vm;
#endif
	Pixfont		*result;
	/* 
	 * We don't know the name of the default font, and
	 * there is no code to copy it, so we punt.
	 */
	if (fontname == 0)
		return 0;

#ifndef NO_SHARED_LIB_FONTS
	_use_vm = 2;
	result = pf_load_vfont(fontname, &from_vm);
	_use_vm = save_use_vm;
#else
	result = pf_load_vfont(fontname);
#endif
	return result;
}

pf_close(pf)
	Pixfont *pf;
{
	register struct font_use *use, *prev;

	if (pf == 0 || pf == pf_bltindef())
		return 0;

	/* look for a font in the shared font list */
	for (prev = Fonts; use = prev->next; prev = use) {
		if (use->pf == pf) {
			/* found it */
			if (--use->count <= 0) {
				if (!use->resident)
					free((char *) pf);

				prev->next = use->next;
				(void) free((char *) use);
			}
			return 0;
		}
	}

	/* didn't find font, must be private */
	free((char *) pf);

	return 0;
}


/* implementation */
#ifndef	NO_SHARED_LIB_FONTS
/* Following routine computes the file name of the vm backing storage.
 * It will try to use the information in the WINDOW_PARENT environment
 * variable to generate names unique for a desktop, so that users with
 * multiple monitors can run multiple sunviews without getting collisions.
 */
static void
pf_vm_font_file(buf)
	char	*buf;
/* Caller must provide a buf that is at least 1024 chars long. */
{
	char	*window_parent = getenv("WINDOW_PARENT");

	strcpy(buf, VM_FONT_FILE);
	if (window_parent != NULL && window_parent[0] != '\0') {
		strcat(buf, &window_parent[strlen(window_parent)-2]);
	}
}

static char *
pf_alloc_vm(to_alloc)
	int	 to_alloc;
/* Caller must have rounded to_alloc to page boundary, and should only
 * call if pf_linked_dynamic() is true.
 */
{
	register struct link_map	*lmp;
	int				retried = 0;
	char				vm_file_name[1024];
	caddr_t				mappedat;
	/*
	 * Map in the file and set its length long enough to hold
	 * the mapped region.
	 */
	pf_vm_font_file(vm_file_name);
Open:
	if ((vm_fd = open(vm_file_name,
			  O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
		if (errno == EACCES && !retried) {
			/* Assume left over from prior master and remove. */
			if (unlink(vm_file_name) == 0) {
				retried = 1;
				goto Open;
			}
		}
		perror("open");
		goto Error_return;
	}
	if (ftruncate(vm_fd, to_alloc) < 0) {
		perror("ftruncate");
		goto Error_return;
	}
	mappedat = mmap(0, to_alloc, PROT_WRITE | PROT_READ,
			MAP_SHARED, vm_fd, (off_t) 0);
	if (mappedat == (caddr_t) -1) {
		perror("mmap");
		goto Error_return;
	}
	return(mappedat);

Error_return:
	if (vm_fd) {
		close(vm_fd);
		vm_fd = 0;
	}
	return(0);
}

static Pf_vm_header
pf_map_vm()
/* Caller should only call if pf_linked_dynamic() is true.
 */
{
	struct pf_vm_header	vm_header;
	register int		fd, len, page_size;
	char			mincore_status, vm_file_name[1024];
	register caddr_t	mapped_at;

	/*
	 * Get a handle on the file to be mapped in, and read the header.
	 */
	pf_vm_font_file(vm_file_name);
	if ((fd = open(vm_file_name, O_RDONLY)) < 0) {
		if (errno != ENOENT) {
			perror("open");
		}
		goto Error_return;
	}
	len = read(fd, &vm_header, sizeof(vm_header));
	if (len != sizeof(vm_header)) {
		perror("read");
		goto Error_return;
	}
	page_size = getpagesize();
	if (vm_header.version != PF_VM_VERSION ||
	    ((int)(vm_header.mapped_at) % page_size) != 0 ||
	    vm_header.size_of_vm_blk > 1024000) {
		/* Header looks bad: punt. */
		goto Error_return;
	}
	/*
	 * Check to see if all of specified block is unmapped.
	 * If so, attempt to map file at the specified address.
	 */
	for (len = vm_header.size_of_vm_blk, mapped_at = vm_header.mapped_at;
	     len > 0; len -= page_size, mapped_at += page_size) {
		if (mincore(mapped_at, page_size, &mincore_status) != -1 ||
		    errno != ENOMEM) {
			/* Block of memory is partially mapped: punt. */
			goto Error_return;
		}
	}
	mapped_at = mmap(vm_header.mapped_at, vm_header.size_of_vm_blk,
			PROT_READ, MAP_SHARED | MAP_FIXED, fd, (off_t) 0);
	if (mapped_at == (caddr_t) -1) {
		perror("mmap");
		goto Error_return;
	}
	close(fd);	/* fd not needed after mmap completes. */
	return (Pf_vm_header)mapped_at;

Error_return:
	if (fd >= 0) {
		close(fd);
	}
	return (Pf_vm_header)0;
}

static Pixfont *
pf_alloc_font(fontname, size, from_vm)
	register char		*fontname;
	register int		*from_vm;
{
	register Pixfont	*pf;
	register int		 to_alloc;
	register int		 page_size;

	if (!pf_linked_dynamic() || (_use_vm != 1)) {
		goto No_vm;
	}

	/* Defining fonts (master) : place in VM */
	if (next_free == (char *)0) {
		/* Allocate in pages, and guess a little high in case
		 * the first font is a small font and to allow some
		 * room for the vm_header itself.
		 */
		page_size = getpagesize();
		to_alloc = (2+MAX_FONT_COUNT) * size;
		to_alloc = ((to_alloc+page_size-1)/page_size) *
			   page_size;
		next_free = pf_alloc_vm(to_alloc);
		if (next_free == (char *)0) {
			_use_vm = 0;
			goto No_vm;
		}
		end_of_vm_blk = next_free + to_alloc;
		vm_header = (Pf_vm_header)next_free;
		vm_header->version = PF_VM_VERSION;
		vm_header->mapped_at = (caddr_t)vm_header;
		vm_header->size_of_vm_blk = to_alloc;
		vm_header->mem_ops = &mem_ops;
		bcopy((char *)&mem_ops,
		      (char *)&(vm_header->mem_ops_procs),
		      sizeof(mem_ops));
		vm_header->font_count = 0;
		next_free = (char *)vm_header->vm_blk;
	}
	if (vm_header->font_count == MAX_FONT_COUNT ||
	    next_free+size >= end_of_vm_blk) {
		goto No_vm;
	}
	pf = (Pixfont *)next_free;
	*from_vm = 1;
	/* Always align to next int boundary */
	next_free += ((size+sizeof(int)-1)/sizeof(int)) * sizeof(int);
	/* Remember this font */
	strcpy(vm_header->font_names[vm_header->font_count], fontname);
	vm_header->fonts[vm_header->font_count] = pf;
	vm_header->font_count++;
	return pf;

No_vm:	*from_vm = 0;
	pf = (Pixfont *)calloc(1, size);
	return pf;
}
#endif

static Pixfont *
#ifndef	NO_SHARED_LIB_FONTS
pf_load_vfont(fontname, from_vm)
	char *fontname;
	int  *from_vm;
#else
pf_load_vfont(fontname)
	char *fontname;
#endif
{
	FILE *fontf;
	struct header hd;
	int defx, defy;
	register struct dispatch *d;
	register unsigned nfont, nimage;
	register Pixfont *pf = 0;
	struct dispatch disp[NUM_DISPATCH];
#ifndef	NO_SHARED_LIB_FONTS
	int	i;

	/* Try looking in VM iff a slave. */
	if (pf_linked_dynamic() && (_use_vm == 0)) {
		if (vm_header == (Pf_vm_header)0) {
			vm_header = pf_map_vm();
			if (vm_header == (Pf_vm_header)0	||
			    vm_header->mem_ops != &mem_ops	||
			    bcmp((char *)&mem_ops,
				 (char *)&(vm_header->mem_ops_procs),
				 sizeof(mem_ops)) != 0	) {
				_use_vm = 2;
				goto No_vm;
			}
		}
		for (i = 0; i < vm_header->font_count; i++) {
			if (0 == strcmp(fontname, vm_header->font_names[i])) {
				pf = vm_header->fonts[i];
				*from_vm = 1;
				return pf;
			}
		}
	}
No_vm:
#endif
	if (!(fontf = fopen(fontname, "r")))
		return pf;

	if (fread((char *) &hd, sizeof hd, 1, fontf) != 1 ||
		fread((char *) disp, sizeof disp, 1, fontf) != 1 ||
		hd.magic != VFONT_MAGIC)
		goto bad;

	/* 
	 * Set default sizes.  The default width of the font is taken to be
	 * the width of a lower-case a, if there is one. The default
	 * interline spacing is taken to be 3/2 the height of an
	 * upper-case A above the baseline.
	 */
	if (disp['a'].nbytes && disp['a'].width > 0 &&
		disp['A'].nbytes && disp['A'].up > 0) {
		defx = disp['a'].width;
		defy = disp['A'].up * 3 >> 1;
	}
	else {
		defx = hd.maxx;
		defy = hd.maxy + hd.xtend;
	}

	if (defx <= 0 || defy <= 0)
		goto bad;

	/* compute total font and image size */
	nfont = sizeof (struct pixfont);
	nimage = 0;

	for (d = disp; d < &disp[NUM_DISPATCH]; d++)
		if (d->nbytes) {
			register int w, h;

			if ((w = d->left + d->right) <= 0 ||
				(h = d->up + d->down) <= 0) {
				d->nbytes = 0;
				continue;
			}

			nfont += sizeof (struct pixrect) +
				sizeof (struct mpr_data);

			w = w <= 16 ? 2 : w + 31 >> 5 << 2;

			/* may need extra space to preserve 32 bit alignment */
			if (w > 2 && nimage & 2)
				nimage += 2;

			nimage += pr_product(w, h);
		}

	/* allocate space for pixfont, pixrects, and images */
	nimage += nfont;
#ifndef	NO_SHARED_LIB_FONTS
	pf = pf_alloc_font(fontname, nimage, from_vm);
	if ((pf == (Pixfont *)0) || (*from_vm && (_use_vm == 0))) {
		goto bad;	/* bit of a misnomer */
	} else {
#else
	if (pf = (Pixfont *) calloc(1, nimage)) {
#endif
		pf->pf_defaultsize.x = defx;
		pf->pf_defaultsize.y = defy;

		/* construct pixfont */
		if (pf_build(fontf, disp, &pf->pf_char[0], 
			(caddr_t) (pf + 1), PTR_ADD(pf, nfont))) {
			free((char *) pf);
			pf = 0;
		}
	}

bad:
	(void) fclose(fontf);
	return pf;
}

static
pf_build(fontf, disp, pc, data, image)
	FILE *fontf;
	register struct dispatch *disp;
	struct pixchar *pc;
	caddr_t data;
	char *image;
{
	int nchar;
	register unsigned short addr = 0;

	/* hack to recycle registers */
	register caddr_t Aptr, Bptr;

#define	Apc	((struct pixchar *) Aptr)
#define	Bpr	((Pixrect *) Bptr)
#define Aprd	((struct mpr_data *) Aptr)
#define	Bimage	((char *) Bptr)
#define	Afile	((FILE *) Aptr)

	for (nchar = NUM_DISPATCH; --nchar >= 0; disp++, pc++)
		if (disp->nbytes) {
			register int w, h, pad;

			/* create pixchar */
			Aptr = (caddr_t) pc;
			w = disp->left;
			Apc->pc_home.x = -w;
			w += disp->right;
			h = disp->up;
			Apc->pc_home.y = -h;
			h += disp->down;
			Apc->pc_adv.x = disp->width;
			Apc->pc_adv.y = 0;

			/* create pixrect */
			Bptr = (caddr_t) data;
			Apc->pc_pr = Bpr;
			Bpr->pr_ops = &mem_ops;
			Bpr->pr_size.x = w;
			Bpr->pr_size.y = h;
			Bpr->pr_depth = 1;

			/* create pixrect data */
			Aptr = (caddr_t) (Bpr + 1);
			Bpr->pr_data = (caddr_t) Aprd;
			w = (w + 7) >> 3;
			pad = w <= 2 ? 2 : w + 3 & ~3;
			Aprd->md_linebytes = pad;

			Bptr = (caddr_t) image;
			if (pad > 2 && (int) Bptr & 2)
				Bptr += 2;
			Aprd->md_image = (short *) Bimage;
			Aprd->md_flags = w <= 2 ? MP_FONT : 0;
			data = (caddr_t) (Aprd + 1);

			Aptr = (caddr_t) fontf;

			if (addr != disp->addr) {
				(void) fseek(Afile, 
					(long) (sizeof (struct header) + 
					sizeof (struct dispatch) * 
						NUM_DISPATCH + 
					disp->addr),
					0);
				addr = disp->addr;
			}

#ifdef MUTANT
			/* check for mutant short-padded vfont */
			if (w & 1 && pr_product(w + 1, h) == disp->nbytes)
				w++;
#endif MUTANT

			pad -= w;
			while (--h >= 0) {
				PR_LOOPP(w - 1,
					*Bimage = getc(Afile);
					Bptr++);
				Bptr += pad;
				addr += w;
			}

			image = Bptr;
		}

	return ferror(Afile) || feof(Afile);
}

