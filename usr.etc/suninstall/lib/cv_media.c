#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)cv_media.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)cv_media.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_media.c
 *
 *	Description:	Routines for manipulating media codes.
 */

#include "install.h"
#include "media.h"

/*
 * we define a private table of all known devices and their attributes
 * everything is in one table, so it doesn't get disjointed!
 */
static struct media_attr {
	int	media_dev;	/* token (assigned at init time) */
	char	*media_name;	/* the short name, "st0", "fd0", ... */
				/* NULL ptr (vs. null string) ends list */
	int	media_type;	/* type code, MEDIAT_xxx */
	int	media_flags;	/* (unused) attribute flags */
	int	block_size;	/* media specific "block factor" */
	/* FUTURE: have ptr to an array of ops here */
	/* FUTURE: have flags for which architectures are applicable */
} media_attr[] = {
/* 	{0, "ar0", MEDIAT_TAPE, 0, BS_QUARTER }, going, going, ... */
/* 	{0, "ar8", MEDIAT_TAPE, 0, BS_QUARTER }, going, going, ... */
	{0, "st0", MEDIAT_TAPE, 0, BS_QUARTER },
	{0, "st1", MEDIAT_TAPE, 0, BS_QUARTER },
	{0, "st2", MEDIAT_TAPE, 0, BS_QUARTER },

	{0, "st3", MEDIAT_TAPE, SPECIAL_ST_FORM, BS_QUARTER },
	{0, "st4", MEDIAT_TAPE, SPECIAL_ST_FORM, BS_QUARTER },
	{0, "st5", MEDIAT_TAPE, SPECIAL_ST_FORM, BS_QUARTER },
	{0, "st6", MEDIAT_TAPE, SPECIAL_ST_FORM, BS_QUARTER },
	{0, "st7", MEDIAT_TAPE, SPECIAL_ST_FORM, BS_QUARTER },

/*	{0, "st8", MEDIAT_TAPE, 0, BS_QUARTER }, this hack done in software now */
	{0, "xt0", MEDIAT_TAPE, 0, BS_HALF },
	{0, "mt0", MEDIAT_TAPE, 0, BS_HALF },
	{0, "fd0", MEDIAT_FLOPPY, 0, BS_DISKETTE },
	{0, "sr0", MEDIAT_CD_ROM, 0, BS_CD_ROM },
	{0, (char *)0, 0, 0, 0 }	/* null name ptr ends table */
};

static	int	media_attr_notinited = 1;
int first_cd_rom = 0;

/*
 *	media_attr_init()
 * private function for media_attr table, it assigns tokens for the devices
 * NOTE: tokens start at "1", token == 0 means "unassigned yet or invalid".
 */
void
media_attr_init()
{
	struct media_attr *ma;
	int	i;

	if (media_attr_notinited == 0)
		return;

	i = 1;
	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		ma->media_dev = i++;
		if (ma->media_type == MEDIAT_CD_ROM && !first_cd_rom)
			first_cd_rom = ma->media_dev;
	}
	media_attr_notinited = 0;
}

/*
 *	Name:		cv_media_to_str()
 *
 *	Description:	Convert a media code into a string.  Returns NULL
 *		if the code cannot be converted.
 */

char *
cv_media_to_str(media_p)
	int	*media_p;
{
	struct media_attr *ma;

	if (media_attr_notinited)
		media_attr_init();

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (ma->media_dev == *media_p) {
			return(ma->media_name);
		}
	}
	return (NULL);

}


/*
 *	Name:		cv_str_to_media()
 *
 *	Description:	Convert a string into a media code.  Returns 1 if
 *		string was converted and 0 otherwise.
 */

int
cv_str_to_media(str, data_p)
	char	*str;
	int	*data_p;
{
	struct media_attr *ma;

	if (media_attr_notinited)
		media_attr_init();

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (strcmp(ma->media_name, str) == 0) {
			*data_p = ma->media_dev;
			return (1);
		}
	}
	*data_p = 0;
	return (0);
}

/*
 *	Name:		media_block_size()
 *
 *	Description:	Determine the block size associated with the
 *		given media code.  Returns 0 if the media code cannot
 *		be converted.
 */

int
media_block_size(soft_p)
	soft_info	*soft_p;
{
	struct media_attr *ma;

	if (media_attr_notinited)
		media_attr_init();

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (ma->media_dev == soft_p->media_dev) {
			/*
			 * because of multiple scsi tape types,
			 * we get the appropriate blocking factor
			 */
/* XXX should have a flag for "ASK_TAPE_BLKSIZE" */
			if (is_scsi_tape(ma->media_name))
				return (scsi_block_size(ma->media_name));
			else		       
				return (ma->block_size); /* return table value*/
		}
	}
	return (0);
}

/*
 *	Name:		media_dev_name()
 *
 *	Description:	Return the generic device name associated with
 *		the given media code.  Returns NULL if the code cannot
 *		be converted.
 */

char *
media_dev_name(code_num)
	int		code_num;
{
	struct media_attr *ma;

	if (media_attr_notinited)
		media_attr_init();

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (ma->media_dev == code_num)
			return (ma->media_name);
	}
	return (NULL);
}

/*
 * media_set_soft
 *	called when media is selected, sets info into
 * 	the soft_info structure for the chosen media.
 */
void
media_set_soft(soft_p, code_num)
	soft_info	*soft_p;
	int		code_num;
{
	struct media_attr *ma;

	/* media_attr already inited when menu is first displayed */

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (ma->media_dev == code_num) {
			soft_p->media_type = ma->media_type;
			soft_p->media_flags = ma->media_flags;
			/* XXX could fill in name too? */
			/* XXX could fill in block size too? */
			return;
		}
	}
}


/*
 * 	Name:		get_media_flag()
 *
 *	Description: 	Given a media number, return the flag for that media
 *
 *	Return Value: 	the "media_flags" value of the media_attr structure
 */
int
get_media_flag(code_num)
	int	code_num;
{
	struct media_attr *ma;

	/* media_attr already inited when menu is first displayed */

	for (ma = media_attr; ma->media_name != (char *)0; ma++) {
		if (ma->media_dev == code_num) {
			return(ma->media_flags);
		}
	}

	return(0);	/* did not find that code number */
}
