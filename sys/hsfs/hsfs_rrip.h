/* @(#)hsfs_rrip.h 1.1 92/07/30 SMI */

/*
 * ISO 9660 RRIP extension filesystem specifications
 * Copyright (c) 1991 by Sun Microsystem, Inc.
 */

#ifndef	_HSFS_RRIP_H_
#define	_HSFS_RRIP_H_

/*
 * Mount options or not using RRIP. This include file is included
 * in mount.c for hsfs. Since sys/mount.h  already has flags upto 0x40, a
 * value should be used that is greater than it.
 */
#define	MS_NO_RRIP	0x800000	/* if set, don't use Rock Ridge */


/*
 * Make sure we have this first
 */

#define	RRIP_VERSION		1
#define	RRIP_SUF_VERSION	1
#define	RRIP_EXT_VERSION	1

#define	RRIP_BIT	1	/* loc. in extension_name_table in susp.c */

#define	IS_RRIP_IMPLEMENTED(fsp) (IS_IMPL_BIT_SET(fsp, RRIP_BIT) ? 1 : 0)



/*
 * RRIP signature macros
 */
#define	RRIP_CL		"CL"
#define	RRIP_NM		"NM"
#define	RRIP_PL		"PL"
#define	RRIP_PN		"PN"
#define	RRIP_PX		"PX"
#define	RRIP_RE		"RE"
#define	RRIP_RR		"RR"
#define	RRIP_SL		"SL"
#define	RRIP_TF		"TF"

/*
 * RRIP ER extension fields
 */
#define	RRIP_ER_EXT_ID		"RRIP_1991A"

#define	RRIP_ER_EXT_DES		"THE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES \
SUPPORT FOR POSIX FILE SYSTEM SEMANTICS."

#define	RRIP_ER_EXT_SRC		"PLEASE CONTACT DISC PUBLISHER FOR \
SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR \
FOR CONTACT INFORMATION."

/*
 * "TF" time macros
 */
#define	RRIP_TF_FLAGS(x)	*(RRIP_tf_flags(x))
#define	RRIP_tf_flags(x)	(&((u_char *)(x))[4])

#define	RRIP_TIME_START_BP	5

#define	RRIP_TF_TIME_LENGTH(x)	(IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
					RRIP_TF_LONG_BIT) ? \
					ISO_DATE_LEN : ISO_SHORT_DATE_LEN)

/*
 * Time location bits
 */
#define	RRIP_TF_CREATION_BIT	0x01
#define	RRIP_TF_MODIFY_BIT	0x02
#define	RRIP_TF_ACCESS_BIT	0x04
#define	RRIP_TF_ATTRIBUTES_BIT	0x08
#define	RRIP_TF_BACKUP_BIT	0x10
#define	RRIP_TF_EXPIRATION_BIT	0x20
#define	RRIP_TF_EFFECTIVE_BIT	0x40
#define	RRIP_TF_LONG_BIT	0x80



#define	RRIP_tf_creation(x)	(&((u_char *)x)[RRIP_TIME_START_BP])
#define	RRIP_tf_modify(x)	(&((u_char *)x)[RRIP_TIME_START_BP + \
			(RRIP_TF_TIME_LENGTH(x) * \
				(IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
				RRIP_TF_CREATION_BIT)))])

#define	RRIP_tf_access(x)	(&((u_char *)x)[RRIP_TIME_START_BP + \
			(RRIP_TF_TIME_LENGTH(x) * \
			(IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
					RRIP_TF_CREATION_BIT) + \
			IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
					RRIP_TF_MODIFY_BIT)))])

#define	RRIP_tf_attributes(x)	(&((u_char *)x)[RRIP_TIME_START_BP + \
			(RRIP_TF_TIME_LENGTH(x) * \
				(IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
						RRIP_TF_CREATION_BIT) + \
				IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
						RRIP_TF_MODIFY_BIT) + \
				IS_TIME_BIT_SET(RRIP_TF_FLAGS(x), \
						RRIP_TF_ACCESS_BIT)))])



/*
 * Check if TF Bits are set.
 *
 * Note : IS_TIME_BIT_SET(x, y)  must be kept returning 1 and 0.
 * 	see RRIP_tf_*(x) Macros
 */
#define	IS_TIME_BIT_SET(x, y)	(((x) & (y))  ? 1 : 0)
#define	SET_TIME_BIT(x, y)	((x) |= (y))


/*
 * "PX" Posix attibutes
 */
#define	RRIP_mode(x)		(&((u_char *)x)[4])
#define	RRIP_MODE(x)		(mode_t)BOTH_INT(RRIP_mode(x))

#define	RRIP_nlink(x)		(&((u_char *)x)[12])
#define	RRIP_NLINK(x)		(short)BOTH_INT(RRIP_nlink(x))

#define	RRIP_uid(x)		(&((u_char *)x)[20])
#define	RRIP_UID(x)		(uid_t)BOTH_INT(RRIP_uid(x))

#define	RRIP_gid(x)		(&((u_char *)x)[28])
#define	RRIP_GID(x)		(gid_t)BOTH_INT(RRIP_gid(x))

/*
 * "PN" Posix major/minor numbers
 */

#define	RRIP_major(x)		(&((u_char *)x)[4])
#define	RRIP_MAJOR(x)		BOTH_INT(RRIP_major(x))

#define	RRIP_minor(x)		(&((u_char *)x)[12])
#define	RRIP_MINOR(x)		BOTH_INT(RRIP_minor(x))


/*
 *  "NM" alternate name and "SL" symbolic link macros...
 */

#define	RESTORE_NM(tmp, orig)	if (is_rrip && *(tmp) != '\0') \
					(void) strcpy((orig), (tmp))

#define	SYM_LINK_LEN(x)		(strlen(x) + 1)
#define	RRIP_NAME_LEN_BASE	5
#define	RRIP_NAME_LEN(x)	(SUF_LEN(x) - RRIP_NAME_LEN_BASE)

#define	RRIP_NAME_FLAGS(x)	(((u_char *)x)[4])

/*
 * These are for the flag bits in the NM and SL and must remain <= 8 bits
 */
#define	RRIP_NAME_CONTINUE	0x01
#define	RRIP_NAME_CURRENT	0x02
#define	RRIP_NAME_PARENT	0x04
#define	RRIP_NAME_ROOT		0x08
#define	RRIP_NAME_VOLROOT	0x10
#define	RRIP_NAME_HOST		0x20


/*
 * These are unique to use in the > 8 bits of sig_args.name_flags
 * They are > 8 so that we can share bits from above.
 * This can grow to 32 bits.
 */
#define	RRIP_NAME_CHANGE	0x40
#define	RRIP_SYM_LINK_COMPLETE	0x80	/* set if sym link already read */
					/* from SUA (no longer than a short) */

/*
 * Bit handling....
 */
#define	SET_NAME_BIT(x, y)	((x) |= (y))
#define	UNSET_NAME_BIT(x, y)	((x) &= ~(y))
#define	IS_NAME_BIT_SET(x, y)	((x) & (y))
#define	NAME_HAS_CHANGED(flag)	\
			(IS_NAME_BIT_SET(flag, RRIP_NAME_CHANGE) ? 1 : 0)

#define	RRIP_name(x)		(&((u_char *)x)[5])
#define	RRIP_NAME(x)		RRIP_name(x)

/*
 * SL Symbolic link macros (in addition to common name flag macos
 */
/* these two macros are from the SL SUF pointer */
#define	RRIP_sl_comp(x)		(&((u_char *)x)[5])
#define	RRIP_SL_COMP(x)		RRIP_sl_comp(x)
#define	RRIP_SL_FLAGS(x)	(((u_char *)x)[4])


/* these macros are from the component pointer within the SL SUF */
#define	RRIP_comp(x)		(&((u_char *)x)[2])
#define	RRIP_COMP(x)		RRIP_comp(x)
#define	RRIP_COMP_FLAGS(x)	(((u_char *)x)[0])
#define	RRIP_COMP_LEN(x)	(RRIP_COMP_NAME_LEN(x) + 2)
#define	RRIP_COMP_NAME_LEN(x)	(((u_char *)x)[1])


/*
 * Directory hierarchy macros
 */

/*
 * Macros for checking relocation bits in flags member of dlist
 * structure defined in iso_impl.h
 */
#define	IS_RELOC_BIT_SET(x, y)	(((x) & (y))  ? 1 : 0)
#define	SET_RELOC_BIT(x, y)	((x) |= (y))

#define	CHILD_LINK		0x01
#define	PARENT_LINK		0x02
#define	RELOCATED_DIR		0x04

#define	RRIP_child_lbn(x)	(&((u_char *)x)[4])
#define	RRIP_CHILD_LBN(x)	(u_int)BOTH_INT(RRIP_child_lbn(x))

#define	RRIP_parent_lbn(x)	(&((u_char *)x)[4])
#define	RRIP_PARENT_LBN(x)	(u_int)BOTH_INT(RRIP_parent_lbn(x))


/*
 * Forward declarations
 */
#ifdef  __STDC__

extern u_char *rrip_name(sig_args_t *);
extern u_char *rrip_file_attr(sig_args_t *);
extern u_char *rrip_dev_nodes(sig_args_t *);
extern u_char *rrip_file_time(sig_args_t *);
extern u_char *rrip_file_time(sig_args_t *);
extern u_char *rrip_sym_link(sig_args_t *);
extern u_char *rrip_parent_link(sig_args_t *);
extern u_char *rrip_child_link(sig_args_t *);
extern u_char *rrip_reloc_dir(sig_args_t *);
extern u_char *rrip_rock_ridge(sig_args_t *);

extern void hs_check_root_dirent(struct vnode *vp, struct hs_direntry *hdp);

extern int rrip_namecopy(char *from, char *to, char *tmp_name, u_char *dirp,
				struct hsfs *fsp, struct hs_direntry *hdp);
#else
extern u_char *rrip_name();
extern u_char *rrip_file_attr();
extern u_char *rrip_dev_nodes();
extern u_char *rrip_file_time();
extern u_char *rrip_file_time();
extern u_char *rrip_sym_link();
extern u_char *rrip_parent_link();
extern u_char *rrip_child_link();
extern u_char *rrip_reloc_dir();
extern u_char *rrip_rock_ridge();

extern void hs_check_root_dirent();

extern int rrip_namecopy();

#endif

#endif /* !_HSFS_RRIP_H_ */
