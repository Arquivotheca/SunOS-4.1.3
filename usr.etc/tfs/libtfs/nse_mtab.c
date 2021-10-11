#ifndef lint
static char sccsid[] = "@(#)nse_mtab.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <netdb.h>
#include <mntent.h>
#include <nse/util.h>


/*
 * Procedures to add/remove mount entries from /etc/mtab.
 */

Nse_err		nse_mtab_read();
Nse_err		nse_mtab_add();
Nse_err		nse_mtab_remove();
void		nse_mntent_destroy();
Nse_list	nse_mtablist_create();

static Nse_err	add_mount_entry();
static void	remove_from_mntlist();


Nse_err
nse_mtab_read(mntlistp)
	Nse_list	*mntlistp;
{
	FILE		*realmnt;
	struct mntent	*mnt;

	realmnt = setmntent(MOUNTED, "r");
	if (realmnt == NULL) {
		perror(MOUNTED);
		return nse_err_format_errno("Setmntent");
	}
	*mntlistp = nse_mtablist_create();
	while ((mnt = getmntent(realmnt)) != NULL) {
		(void) nse_list_add_new_copy(*mntlistp, mnt);
	}
	(void) endmntent(realmnt);
	return NULL;
}


/*
 * Write contents of mntlist into /etc/mtab
 */
Nse_err
nse_mtab_add(mntlist)
	Nse_list	mntlist;
{
	FILE		*mnted;
	Nse_err		err;

	mnted = setmntent(MOUNTED, "r+");
	if (mnted == NULL) {
		return nse_err_format_errno("Setmntent");
	}
	err = (Nse_err) nse_list_iterate_or(mntlist, add_mount_entry, mnted);
	(void) endmntent(mnted);
	return err;
}


/*
 * Remove entries in mntlist from /etc/mtab.
 */
Nse_err
nse_mtab_remove(mntlist)
	Nse_list	mntlist;
{
	FILE		*realmnt;
	Nse_list	realmntlist;
	struct mntent	*mnt;

	realmnt = setmntent(MOUNTED, "r+");
	if (realmnt == NULL) {
		perror(MOUNTED);
		return nse_err_format_errno("Setmntent");
	}
	realmntlist = nse_mtablist_create();
	while ((mnt = getmntent(realmnt)) != NULL) {
		(void) nse_list_add_new_copy(realmntlist, mnt);
	}
	nse_list_iterate(mntlist, remove_from_mntlist, realmntlist);
	/*
	 * Build new mtab by walking mnt list
	 */
	nse_fftruncate(realmnt);
	nse_list_iterate(realmntlist, add_mount_entry, realmnt);
	(void) endmntent(realmnt);
	nse_list_destroy(realmntlist);
	return NULL;
}


static Nse_err
add_mount_entry(mnt, mnted)
	struct mntent	*mnt;
	FILE		*mnted;
{
	if (addmntent(mnted, mnt)) {
		return nse_err_format_errno("Addmntent");
	} else {
		return NULL;
	}
}


/*
 * Remove the entry 'mntl' from 'mntlist', it it's in the list.
 */
static void
remove_from_mntlist(mnt, mntlist)
	struct mntent	*mnt;
	Nse_list	mntlist;
{
	Nse_listelem	elem;

	elem = nse_list_find_elem(mntlist, (Nse_opaque) mnt);
	if (elem) {
		nse_listelem_delete(mntlist, elem);
	}
}


static Nse_opaque
mnt_create()
{
	struct mntent	*mnt;

	mnt = (struct mntent *) nse_calloc(sizeof(struct mntent), 1);
	return ((Nse_opaque) mnt);
}


static Nse_opaque
mnt_copy(mnt)
	struct mntent	*mnt;
{
	struct mntent	*new;

	new = (struct mntent *) nse_calloc(sizeof(struct mntent), 1);
	new->mnt_fsname = NSE_STRDUP(mnt->mnt_fsname);
	new->mnt_dir = NSE_STRDUP(mnt->mnt_dir);
	new->mnt_type = NSE_STRDUP(mnt->mnt_type);
	new->mnt_opts = NSE_STRDUP(mnt->mnt_opts);
	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;
	return ((Nse_opaque) new);
}


void
nse_mntent_destroy(mnt)
	struct mntent	*mnt;
{
	free(mnt->mnt_fsname);
	free(mnt->mnt_dir);
	free(mnt->mnt_type);
	free(mnt->mnt_opts);
	free((char *) mnt);
}


static void
mnt_print(mnt)
	struct mntent	*mnt;
{
	printf("%s %s %s %s %d %d\n",
		mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts,
		mnt->mnt_freq, mnt->mnt_passno);
}


static bool_t
mnt_equal(mnt1, mnt2)
	struct mntent	*mnt1;
	struct mntent	*mnt2;
{
	return (NSE_STREQ(mnt1->mnt_dir, mnt2->mnt_dir) &&
		NSE_STREQ(mnt1->mnt_type, mnt2->mnt_type));
}


static Nse_listops_rec mtablist_ops = {
	mnt_create,
	mnt_copy,
	nse_mntent_destroy,
	mnt_print,
	mnt_equal,
};


Nse_list
nse_mtablist_create()
{
	Nse_list	list;

	list = nse_list_alloc(&mtablist_ops);
	return list;
}
