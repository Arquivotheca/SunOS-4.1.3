/* @(#)hsfs_susp.h 1.1 92/07/30 SMI	*/

/*
 * ISO 9660 RRIP extension filesystem specifications
 * Copyright (c) 1991 by Sun Microsystem, Inc.
 */

#ifndef	_HSFS_SUSP_H_
#define	_HSFS_SUSP_H_

#define	DEBUGGING_ALL	0
#define	DEBUGGING	0
#define	DPRINTF		if (DEBUGGING) printf
#define	DPRINTF_ALL	if (DEBUGGING_ALL) printf

/*
 * 	return values from SUA parsing
 */
#define	SUA_NULL_POINTER	-1
#define	END_OF_SUA_PARSING	-2
#define	END_OF_SUA		-3
#define	GET_CONTINUATION	-4
#define	SUA_ENOMEM		-5
#define	SUA_EINVAL		-6
#define	RELOC_DIR		-7	/* actually for rrip */

/*
 * For dealing with implemented bits...
 *    These here expect the fsp and a bit as an agument
 */
#define	SET_IMPL_BIT(fsp, y)	((fsp->ext_impl_bits)  |= (0x01L) << (y))
#define	UNSET_IMPL_BIT(fsp, y)	((fsp->ext_impl_bits)  &= ~((0x01L) << (y)))
#define	IS_IMPL_BIT_SET(fsp, y)	((fsp->ext_impl_bits)  & ((0x01L) << (y)))

/*
 * For dealing with implemented bits...
 *    These here expect just the fsp->ext_impl_bits
 */
#define	SET_SUSP_BIT(fsp)	(SET_IMPL_BIT((fsp), 0))
#define	UNSET_SUSP_BIT(fsp)	(UNSET_IMPL_BIT((fsp), 0))
#define	IS_SUSP_IMPLEMENTED(fsp) (IS_IMPL_BIT_SET(fsp, 0) ? 1 : 0)


#define	SUSP_VERSION	1

/*
 * SUSP signagure definitions
 */
#define	SUSP_SP		"SP"
#define	SUSP_CE		"CE"
#define	SUSP_PD		"PD"
#define	SUSP_ER		"ER"
#define	SUSP_ST		"ST"

/*
 * 	Generic System Use Field (SUF) Macros and declarations
 */

#define	SUF_SIG_LEN	2			/* length of signatures */
#define	SUF_MIN_LEN	4			/* minimum length of a */
						/* 	signature field */

#define	SUF_LEN(x)	*(SUF_len(x))		/* SUF length */
#define	SUF_len(x)	(&((u_char *)x)[2])	/* SUF length */

#define	SUF_VER(x)	*(SUF_ver(x))		/* SUF version */
#define	SUF_ver(x)	(&((u_char *)x)[3])	/* SUF version */

/*
 * Extension Reference Macros
 */

#define	ER_ID_LEN(x)	*(ER_id_len(x))	/* Extension ref id length */
#define	ER_id_len(x)	(&((u_char *)x)[4])	/* Extension ref id length */


#define	ER_DES_LEN(x)	*(ER_des_len(x))	/* Extension ref description */
						/* 	length */
#define	ER_des_len(x)	(&((u_char *)x)[5])	/* Extension ref description */
						/* 	length */

#define	ER_SRC_LEN(x)	*(ER_src_len(x))	/* Extension ref source */
						/* 	description length */

#define	ER_src_len(x)	(&((u_char *)x)[6])	/* Extension ref source */
						/* description length */


#define	ER_EXT_VER(x)	*(ER_ext_ver(x))	/* Extension ref description */
						/*  length */
#define	ER_ext_ver(x)	(&((u_char *)x)[7])	/* Extension ref description */
						/* length */

#define	ER_EXT_ID_LOC	8			/* location where the ER id */
						/* string begins */

#define	ER_ext_id(x)	(&((u_char *)x)[ER_EXT_ID_LOC])
						/* extension id string */

#define	ER_ext_des(x)	(&((u_char *)x)[ER_EXT_ID_LOC + ER_ID_LEN(x)])
						/* ext. description string */

#define	ER_ext_src(x)	(&((u_char *)x)[ER_EXT_ID_LOC + ER_ID_LEN(x) + \
					ER_DES_LEN(x)])
						/* ext. source string */


/*
 * Continuation Area Macros
 */
#define	CE_BLK_LOC(x)	BOTH_INT(CE_blk_loc(x))	/* cont. starting block */
#define	CE_blk_loc(x)	(&((u_char *)x)[4])	/* cont. starting block */

#define	CE_OFFSET(x)	BOTH_INT(CE_offset(x))	/* cont. offset */
#define	CE_offset(x)	(&((u_char *)x)[12])	/* cont. offset */

#define	CE_CONT_LEN(x)	BOTH_INT(CE_cont_len(x))	/* continuation len */
#define	CE_cont_len(x)	(&((u_char *)x)[20])	/* continuation len */


/*
 * Sharing Protocol (SP) Macros
 */
#define	SP_CHK_BYTE_1(x)	*(SP_chk_byte_1(x))	/* check bytes */
#define	SP_chk_byte_1(x)	(&((u_char *)x)[4])	/* check bytes */

#define	SP_CHK_BYTE_2(x)	*(SP_chk_byte_2(x))	/* check bytes */
#define	SP_chk_byte_2(x)	(&((u_char *)x)[5])	/* check bytes */

#define	SUSP_CHECK_BYTE_1	(u_char)0xBE		/* check for 0xBE */
#define	SUSP_CHECK_BYTE_2	(u_char)0xEF		/* check for 0xEF */

#define	CHECK_BYTES_OK(x)	((SP_CHK_BYTE_1(x) == SUSP_CHECK_BYTE_1) && \
				(SP_CHK_BYTE_2(x) == SUSP_CHECK_BYTE_2))

#define	SP_SUA_OFFSET(x)	*(SP_sua_offset(x))	/* SUA bytes to skip */
#define	SP_sua_offset(x)	(&((u_char *)x)[6])	/* SUA bytes to skip */



/*
 * Forward declarations
 */

extern u_char *share_protocol();
extern u_char *share_ext_ref();
extern u_char *share_continue();
extern u_char *share_padding();
extern u_char *share_stop();

/*
 * Extension signature structure, to corrolate the handler functions
 * with the signatures
 */
struct extension_signature_struct {
	char	*ext_signature;		/* extension signature */
	u_char	*(*sig_handler)();	/* extension handler function */
};

typedef	struct extension_signature_struct	ext_signature_t;


/*
 * Extension name structure, to corrolate the extensions with their own
 * 	signature tables.
 */
struct extension_name_struct {
	char  		*extension_name;	/* ER field identifier */
	u_short		ext_version;		/* version # of extensions */
	ext_signature_t	*signature_table;	/* pointer to signature */
						/*   table for appropriate */
						/*   extension */
};

typedef	struct extension_name_struct extension_name_t;

/*
 * Extern declaration for all supported extensions
 */
struct	cont_info_struct	{
	u_int	cont_lbn;	/* location  of cont */
	u_int	cont_offset;	/* offset into cont */
	u_int	cont_len;	/* len of cont */
};

typedef struct cont_info_struct	cont_info_t;

/*
 * Structure for passing arguments to sig_handler()'s.  Since there are
 * so many sig_handler()'s, it would be slower to pass multiple
 * arguments to all of them. It would also ease maintainance
 */
struct sig_args_struct {
	u_char			*dirp;		/* pointer to ISO dir entry */
	u_char			*name_p;	/* dir entry name */
	int			*name_len_p;	/* dir entry name length */
	short			flags;		/* misc flags */
	u_long			name_flags;		/* misc flags */
	u_char			*SUF_ptr;	/* pointer to current SUF */
	struct hs_direntry	*hdp;		/* directory entry  */
	struct hsfs		*fsp;		/* file system  */
	cont_info_t		*cont_info_p;	/* continuation area */
};

typedef struct sig_args_struct	sig_args_t;


/*
 * Extern declaration for all supported extensions
 */

extern ext_signature_t		rrip_signature_table[];
extern ext_signature_t		susp_signature_table[];
extern extension_name_t		extension_name_table[];

extern ext_signature_t		*susp_sp;

#ifdef __STDC__
extern int parse_sua(u_char *, int *name_len_p, int *, u_char *, struct
	hs_direntry *,	struct hsfs *,	u_char	*, int search_num);
#else
extern int parse_sua();
#endif

#endif /* !_HSFS_SUSP_H_ */
