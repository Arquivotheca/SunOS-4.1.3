#ifndef lint 
#ident "@(#)hsfs_rrip.c 1.1 92/07/30" 
#endif

/*
 * Rock Ridge extensions to to the System Use Sharing protocol
 * for High Sierra filesystem
 *
 * Copyright (c) 1990 by Sun Microsystem, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem_alloc.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <ufs/inode.h>

#include <hsfs/hsfs_spec.h>
#include <hsfs/hsfs_isospec.h>
#include <hsfs/hsfs_node.h>
#include <hsfs/hsfs_susp.h>
#include <hsfs/hsfs_rrip.h>

#include <sys/mount.h>
#include <vm/swap.h>
#include <sys/errno.h>
#include <sys/debug.h>


#if defined(__STDC__)

static void form_time(int, u_char *, struct timeval *);
static void name_parse(int, u_char *, int, u_char *, int *, u_long *, int);

#else

static void form_time();
static void name_parse();

#endif

extern void hs_filldirent();

/*
 *  Signature table for RRIP
 */
ext_signature_t  rrip_signature_table[ ] = {
	RRIP_CL,	rrip_child_link,
	RRIP_NM,	rrip_name,
	RRIP_PL,	rrip_parent_link,
	RRIP_PN,	rrip_dev_nodes,
	RRIP_PX,	rrip_file_attr,
	RRIP_RE,	rrip_reloc_dir,
	RRIP_RR,	rrip_rock_ridge,
	RRIP_SL,	rrip_sym_link,
	RRIP_TF,	rrip_file_time,
	(char *)NULL,   NULL
};


/*
 * rrip_dev_nodes()
 *
 * sig_handler() for RRIP signature "PN"
 *
 * This function parses out the major and minor numbers from the "PN
 * " SUF.
 */
u_char *
rrip_dev_nodes(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*pn_ptr = sig_args_p->SUF_ptr;
	int	major_dev = RRIP_MAJOR(pn_ptr);
	int	minor_dev = RRIP_MINOR(pn_ptr);

	sig_args_p->hdp->r_dev = (dev_t) makedev(major_dev, minor_dev);

	return (pn_ptr + SUF_LEN(pn_ptr));
}

/*
 * rrip_file_attr()
 *
 * sig_handler() for RRIP signature "PX"
 *
 * This function parses out the file attributes of a file from the "PX"
 * SUF.  The attributes is finds are : st_mode, st_nlink, st_uid,
 * and st_gid.
 */
u_char *
rrip_file_attr(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*px_ptr = sig_args_p->SUF_ptr;
	register struct hs_direntry *hdp    = sig_args_p->hdp;

	hdp->mode  = RRIP_MODE(px_ptr);
	hdp->nlink = RRIP_NLINK(px_ptr);
	hdp->uid   = RRIP_UID(px_ptr);
	hdp->gid   = RRIP_GID(px_ptr);

	hdp->type = IFTOVT(hdp->mode);

	return (px_ptr + SUF_LEN(px_ptr));
}

/*
 * rrip_file_time()
 *
 * support function for rrip_file_time()
 *
 * This function decides whether to parse the times in a long time form
 * (17 bytes) or a short time form (7 bytes).  These time formats are
 * defined in the ISO 9660 specification.
 */
static void
form_time(time_length, file_time, tvp)
int time_length;
u_char *file_time;
struct timeval *tvp;
{
	if (time_length == ISO_DATE_LEN){
		hs_parse_longdate(HS_VOL_TYPE_ISO, file_time, tvp);
	} else {
		hs_parse_dirdate(HS_VOL_TYPE_ISO, file_time, tvp);
	}

}

/*
 * rrip_file_time()
 *
 * sig_handler() for RRIP signature RRIP_TF
 *
 * This function parses out the file time attributes of a file from the
 * "TI" SUF.  The times it parses are : st_mtime, st_atime and st_ctime.
 *
 * The function form_time is a support function only used in this
 * function.
 */
u_char *
rrip_file_time(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*tf_ptr = sig_args_p->SUF_ptr;


	if (IS_TIME_BIT_SET(RRIP_TF_FLAGS(tf_ptr), RRIP_TF_ACCESS_BIT)) {
		form_time(RRIP_TF_TIME_LENGTH(tf_ptr),
		    RRIP_tf_access(tf_ptr),
		    &sig_args_p->hdp->adate);
	}

	if (IS_TIME_BIT_SET(RRIP_TF_FLAGS(tf_ptr), RRIP_TF_MODIFY_BIT)) {
		form_time(RRIP_TF_TIME_LENGTH(tf_ptr), RRIP_tf_modify(tf_ptr),
		    &sig_args_p->hdp->mdate);
	}

	if (IS_TIME_BIT_SET(RRIP_TF_FLAGS(tf_ptr), RRIP_TF_ATTRIBUTES_BIT)) {
		form_time(RRIP_TF_TIME_LENGTH(tf_ptr),
		    RRIP_tf_attributes(tf_ptr),
		    &sig_args_p->hdp->cdate);
	}

	return (tf_ptr + SUF_LEN(tf_ptr));
}



/*
 * name_parse()
 *
 * This is a generic fuction used for sym links and filenames.  The
 * flags passed to it effect the way the name/component field is parsed.
 *
 * The return value will be the NAME_CONTINUE or NAME_CHANGE value.
 *
 */
static void
name_parse(rrip_flags, SUA_string, SUA_string_len,
		to_string, to_string_len_p, name_flags_p, max_name_len)
	register int		rrip_flags;	/* compontent/name flag */
	register u_char		*SUA_string;	/* string from SUA */
	register int		SUA_string_len; /* length of SUA string */
	register u_char		*to_string;	/* string to copy to */
	register int		*to_string_len_p; /* ptr to cur. str len */
	u_long			*name_flags_p;	/* internal name flags */
	int			max_name_len;	/* limit dest string to */
						/* this value */
{
	register int	tmp_name_len;

	if (IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_ROOT)) {
		(void) strcpy((char *)to_string, "/");
		*to_string_len_p = 1;
	}

	if (IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_CURRENT)) {
		SUA_string = (u_char *)".";
		SUA_string_len = 1;
	}

	if  (IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_PARENT)) {
		SUA_string = (u_char *)"..";
		SUA_string_len = 2;
	}

	/*
	 * XXX
	 * For now, ignore the following flags and return.
	 * have to figure out how to get host name in kernel.
	 * Unsure if this even should be done.
	 */
	if (IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_VOLROOT) ||
	    IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_HOST)) {
		printf("VOLUME ROOT and NAME_HOST currently unsupported\n");
		return;
	}

	/*
	 * Remember we must strncpy to the end of the curent string
	 * name because there might be something there.  Also, don't
	 * go past the max_name_len system boundry.
	 */
	tmp_name_len = strlen((char *)to_string);

	SUA_string_len = (tmp_name_len + SUA_string_len) > max_name_len ?
		(max_name_len - tmp_name_len) : (SUA_string_len);

	(void) strncpy((char *)(to_string + tmp_name_len),
		(char *)SUA_string, SUA_string_len);

	/* NULL terminate string */
	*(to_string + tmp_name_len + SUA_string_len) = '\0';
	*(to_string_len_p) += SUA_string_len;

	if (IS_NAME_BIT_SET(rrip_flags, RRIP_NAME_CONTINUE))
		SET_NAME_BIT(*(name_flags_p), RRIP_NAME_CONTINUE);
	else
		SET_NAME_BIT(*(name_flags_p), RRIP_NAME_CHANGE);


}

/*
 * rrip_name()
 *
 * sig_handler() for RRIP signature RRIP_NM
 *
 * This function handles the name of the current file.  It is case
 * sensitive to whatever was put into the field and does NO
 * translation. It will take whatever characters were in the field.
 *
 * Because the flags effect the way the name is parsed the same way
 * that the sym_link component parsing is done, we will use the same
 * function to do the actual parsing.
 */
u_char  *
rrip_name(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*nm_ptr = sig_args_p->SUF_ptr;

	if ((sig_args_p->name_p == (u_char *)NULL) ||
	    (sig_args_p->name_len_p == (int *)NULL))
		goto end;
	/*
	 * If we have a "." or ".." directory, we should not look for
	 * an alternate name
	 */
	if (HDE_NAME_LEN(sig_args_p->dirp) == 1) {
		if (*((char *)HDE_name(sig_args_p->dirp)) == '\0') {
			(void) strcpy((char *)sig_args_p->name_p, ".");
			*sig_args_p->name_len_p = 1;
			goto end;
		} else if (*((char *)HDE_name(sig_args_p->dirp)) == '\1') {
			(void) strcpy((char *)sig_args_p->name_p, "..");
			*sig_args_p->name_len_p = 2;
			goto end;
		}
	}

	name_parse((int)RRIP_NAME_FLAGS(nm_ptr), RRIP_name(nm_ptr),
	    (int)RRIP_NAME_LEN(nm_ptr), sig_args_p->name_p,
	    sig_args_p->name_len_p, &(sig_args_p->name_flags),
	    MAXNAMELEN);

end:

	return (nm_ptr + SUF_LEN(nm_ptr));
}



/*
 * rrip_sym_link()
 *
 * sig_handler() for RRIP signature RRIP_SL
 *
 * creates a symlink buffer to simulate sym_links.
 */
u_char *
rrip_sym_link(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*sl_ptr = sig_args_p->SUF_ptr;
	register u_char		*comp_ptr;
	register char		*tmp_sym_link;
	register struct hs_direntry *hdp = sig_args_p->hdp;
	int			sym_link_len = 0;
	char 			*sym_link;

	if (hdp->type != VLNK)
		goto end;

	/*
	 * If the sym link has already been created, don't recreate it
	 */
	if (IS_NAME_BIT_SET(sig_args_p->name_flags, RRIP_SYM_LINK_COMPLETE))
		goto end;

	sym_link = (char *)new_kmem_alloc(MAXPATHLEN + 1, KMEM_SLEEP);
	if (sym_link == (char *)NULL) {
		sig_args_p->flags = SUA_ENOMEM;
		return ((u_char *)NULL);
	}

	sym_link[0] = '\0';

	/*
	 * put original string into sym_link[]
	 */
	if (hdp->sym_link != (char *)NULL) {
		(void) strcpy(sym_link, hdp->sym_link);
	}

	/* for all components */
	for (comp_ptr = RRIP_sl_comp(sl_ptr);
		comp_ptr < (sl_ptr + SUF_LEN(sl_ptr));
		comp_ptr += RRIP_COMP_LEN(comp_ptr)) {

		name_parse((int)RRIP_COMP_FLAGS(comp_ptr),
		    RRIP_comp(comp_ptr),
		    (int)RRIP_COMP_NAME_LEN(comp_ptr), (u_char *)sym_link,
		    &sym_link_len, &(sig_args_p->name_flags),
		    MAXPATHLEN);

		/*
		 * If the component is continued, Don't put a
		 * '/' in the pathname, but do NULL terminate it.
		 * And avoid 2 '//' in a row, or if '/' was wanted
		 */
		if (IS_NAME_BIT_SET(RRIP_COMP_FLAGS(comp_ptr),
				    RRIP_NAME_CONTINUE) ||
		    (sym_link[sym_link_len - 1] == '/')) {

			sym_link[sym_link_len] = '\0';
		} else {
			sym_link[sym_link_len] = '/';
			sym_link[sym_link_len + 1] = '\0';

			/* add 1 to sym_link_len for '/' */
			sym_link_len++;
		}

	}

	/*
	 * take out  the last slash
	 */
	if (sym_link[sym_link_len - 1] == '/')
		sym_link[--sym_link_len] = '\0';

	/*
	 * if no memory has been allocated, get some, otherwise, append
	 * to the current allocation
	 */

	tmp_sym_link = (char *)new_kmem_alloc((u_int)SYM_LINK_LEN(sym_link),
								KMEM_SLEEP);

	if (tmp_sym_link == (char *)NULL) {
		kmem_free(sym_link, MAXPATHLEN + 1);
		sig_args_p->flags = SUA_ENOMEM;
		return ((u_char *)NULL);
	}

	(void) strcpy(tmp_sym_link, sym_link);

	if (hdp->sym_link != (char *)NULL) {
		kmem_free(hdp->sym_link, (u_int)SYM_LINK_LEN(hdp->sym_link));
	}

	hdp->sym_link = (char *)&tmp_sym_link[0];

	if (!IS_NAME_BIT_SET(RRIP_SL_FLAGS(sl_ptr), RRIP_NAME_CONTINUE))
		SET_NAME_BIT(sig_args_p->name_flags, RRIP_SYM_LINK_COMPLETE);

	kmem_free(sym_link, MAXPATHLEN + 1);
end:
	return (sl_ptr + SUF_LEN(sl_ptr));
}

/*
 * rrip_namecopy()
 *
 * This function will copy the rrip name to the "to" buffer, if it
 * exists.
 *
 * XXX -  We should speed this up by implementing the search in
 * parse_sua().  It works right now, so I don't want to mess with it.
 */
int
rrip_namecopy(from, to, tmp_name, dirp, fsp, hdp)
	char 	*from;			/* name to copy */
	char 	*to;			/* string to copy "from" to */
	char  	*tmp_name;		/* temp storage for original name */
	u_char 	*dirp;			/* directory entry pointer */
	struct 	hsfs *fsp;		/* filesystem pointer */
	struct 	hs_direntry *hdp;	/* directory entry pointer to put */
					/* all that good info in */
{
	int	size = 0;
	int	change_flag = 0;
	int	ret_val;

	if ((to == (char *)NULL) ||
	    (from == (char *)NULL) ||
	    (dirp == (u_char *)NULL)) {
		return (0);
	}

	/* special handling for '.' and '..' */

	if (HDE_NAME_LEN(dirp) == 1) {
		if (*((char *)HDE_name(dirp)) == '\0') {
			(void) strcpy(to, ".");
			return (1);
		} else if (*((char *)HDE_name(dirp)) == '\1') {
			(void) strcpy(to, "..");
			return (2);
		}
	}


	ret_val = parse_sua((u_char *)to, &size, &change_flag, dirp,
			hdp, fsp, (u_char *)NULL, NULL);

	if (IS_NAME_BIT_SET(change_flag, RRIP_NAME_CHANGE) && !ret_val)
		return (size);

	/*
	 * Well, the name was not found
	 *
	 * make rripname an upper case "nm" (to), so that
	 * we can compare it the current HDE_DIR_NAME()
	 * without nuking the original "nm", for future case
	 * sensitive name comparing
	 */
	(void) strcpy(tmp_name, from);		/* keep original */

	size = hs_uppercase_copy(tmp_name, from, strlen(from));

	/* NOTE */
	return (-1);

	/* return (size); */
}



/*
 * rrip_reloc_dir()
 *
 * This function is fairly bogus.  All it does is cause a failure of
 * the hs_parsedir, so that no vnode will be made for it and
 * essentially, the directory will no longer be seen.  This is part
 * of the directory hierarchy mess, where the relocated directory will
 * be hidden as far as ISO 9660 is concerned.  When we hit the child
 * link "CL" SUF, then we will read the new directory.
 */
u_char *
rrip_reloc_dir(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*re_ptr = sig_args_p->SUF_ptr;

	sig_args_p->flags = RELOC_DIR;

	return (re_ptr + SUF_LEN(re_ptr));
}



/*
 * rrip_child_link()
 *
 * This is the child link part of the directory hierarchy stuff and
 * this does not really do much anyway.  All it does is read the
 * directory entry that the child link SUF contains.  All should be
 * fine then.
 */
u_char *
rrip_child_link(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*cl_ptr = sig_args_p->SUF_ptr;

	sig_args_p->hdp->ext_lbn = RRIP_CHILD_LBN(cl_ptr);

	hs_filldirent(sig_args_p->fsp->hsfs_rootvp, sig_args_p->hdp);

	sig_args_p->flags = 0;

	return (cl_ptr + SUF_LEN(cl_ptr));
}


/*
 * rrip_parent_link()
 *
 * This is the parent link part of the directory hierarchy stuff and
 * this does not really do much anyway.  All it does is read the
 * directory entry that the parent link SUF contains.  All should be
 * fine then.
 */
u_char *
rrip_parent_link(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*pl_ptr = sig_args_p->SUF_ptr;

	sig_args_p->hdp->ext_lbn = RRIP_PARENT_LBN(pl_ptr);

	hs_filldirent(sig_args_p->fsp->hsfs_rootvp, sig_args_p->hdp);

	sig_args_p->flags = 0;

	return (pl_ptr + SUF_LEN(pl_ptr));
}


/*
 * rrip_rock_ridge()
 *
 * This function is supposed to aid in speed of the filesystem.
 *
 * XXX - It is only here as a place holder so far.
 */
u_char *
rrip_rock_ridge(sig_args_p)
sig_args_t *sig_args_p;
{
	register u_char 	*rr_ptr = sig_args_p->SUF_ptr;

	return (rr_ptr + SUF_LEN(rr_ptr));
}
