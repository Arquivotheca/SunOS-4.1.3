#ifndef lint
/*static  char sccsid[] = "@(#)toc.x 1.1 92/07/30 Copyr 1987 Sun Micro";*/
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * Format of New Tape Table of Contents 
 *
 *	The general layout of the table of contents is as follows:
 *
 *	MAGIC # (unsigned 32 bit integer)
 *	VERSION # (ditto)
 *	Distribution Info (structure)
 *	Volume Info (structure)
 *	Entry_0
 *	Entry_1
 *        .
 *        .
 *        .
 *	Entry_n
 *
 *	Distribution information gives some information generally
 *	describing the entire distribution. Name, where from, when
 *	made, etc..
 *
 *	Volume Info merely identifies the media that this table of contents
 *	was just pulled off of. It can be used for volume checking.
 *
 *	Following this information is an arbitrary length list of
 *	TOC entries. These entries reflect the entire table of contents
 *	of the entire distribution. It is up to the programmer to implement
 *	code to filter out entries that are not wanted in a particular
 *	context. That is, each copy of the the table of contents has
 *	common information (namely, the distribution information, and
 *	portions of the entry information).
 *
 *	Each TOC entry has an arbitrary list of addresses where the info
 *	it indexes resides.
 *
 *	Three givens:
 *
 *		For tapes, the TOC will *ALWAYS* be the second file on
 *		a volume, given > 1 file on a volume. The first file
 *		on the first tape volume will typically be a tape boot program
 *
 *		For disks, the TOC will *ALWAYS* start 1024 bytes into
 *		the disk (the first 512 bytes generally will be a SunOS
 *		disk label, the second 512 bytes will be reserved for
 *		a disk bootstrap).
 *
 *		The record size for XDR toc's will always be 512 bytes/record
 *		while on raw media (for use with xdr record format),
 *		and will be with standard i/o on a disk file.
 *
 */

#ifdef RPC_HDR
%#include <stdio.h>
%#include <rpc/rpc.h>
%#include "xdr_custom.h"
#endif

/************************************************
 * Magic # and Version of TOC			*
 ************************************************/

const TOCMAGIC = 0x674D2309;
const TOCVERSION = 0;

/************************************************
 * General XDR structure definitions		*
 ************************************************/
/*
 * Definition of a counted list of strings
 */

typedef	string stringp<>;
typedef stringp string_list<>;


/************************************************
 * Pieces of specialized XDR structures		*
 ************************************************/

/*
 * Type of file in a distribution set.
 *
 * This will imply (to an extraction or to
 * a construction program) what programs to invoke
 * to read or write the entity.
 */
enum file_type
{
	UNKNOWN,
	TOC,		/* Table of Contents		*/
	IMAGE,		/* Byte-for-Byte Image		*/
	TAR,		/* Tar File			*/
	CPIO,		/* Cpio File 			*/
	BAR,		/* Bar File	 		*/
	TARZ		/* Compressed Tar File 		*/
};

/*
 * Type of Media supported
 */

enum media_type
{
	TAPE,	/* File on Tape(s) (cartridge or 9-track)	*/
	/*
	 * The following are only partially defined, and provided
	 * only to give a clue as to the direction things ought
	 * to go...
	 */
	DISK,	/* File on Disk(s) (raw disk)			*/
	STREAM	/* File from stream connection			*/
};


/*
 * Tape type media address structure
 */

struct tapeaddress
{
	int	volno;		/* volume number in set			*/
	string	label<>;	/* tape label				*/
	int file_number;	/* file in volume			*/
	int size;		/* length of file (bytes)		*/
	int volsize;		/* total size of volume (in bytes)	*/
	string	dname<>;	/* 'root' device name			*/
				/* e.g., "st" for cartridge,		*/
				/* "mt" for 1/2" tape			*/

};



/*
 * Disk type media address structure
 */

struct diskaddress
{
	int	volno;		/* volume number in set			*/
	string label<>;		/* pack label				*/
	int offset;		/* offset from beginning of disk (bytes)*/
	int size;		/* length of file (bytes)		*/
	int volsize;		/* total size of volume (bytes)		*/
	string	dname<>;	/* 'root' device name			*/
				/* e.g., "stN" for cartridge,		*/
				/* "mtN" for 1/2" tape			*/
};

/*	
 * Address Structure - defines different
 * types of media access.
 *
 */

union Address switch (media_type dtype)
{
case TAPE:
	tapeaddress tape;
case DISK:
	diskaddress disk;
default:
	/*
	 * Has routine to handle it in xdr_custom
	 * Placed here to trap errors.
	 */
	struct nomedia junk;
};

/************************************************
 * Per File info for Table of contents itself	*
 ************************************************/

struct contents
{
	/*
	 * Just a place holder....
	 */
	char junk;
};

/************************************************
 * Per File info for Standalone images		*
 ************************************************/

struct standalone
{
	string_list arch;	/* architecture(s) this entity can run on*/
	unsigned size;		/* size, in bytes, of the image		*/
				/* This is a *supplied* value, not	*/
				/* a calculated one.			*/
};

/************************************************
 * Per File info for Executable images		*
 ************************************************/

struct executable
{
	string_list arch;	/* architecture(s) this entity can run on*/
	unsigned size;		/* size, in bytes, of the image		*/
				/* This is a *supplied* value, not	*/
				/* a calculated one.			*/
};

/************************************************
 * Per File info for Amorphous files		*
 ************************************************/

struct amorphous
{
	unsigned size;		/* size, in bytes, of the file		*/
				/* This is a *supplied* value, not	*/
				/* a calculated one.			*/
};

/************************************************
 * Per File info for Install tools		*
 ************************************************/

/*
 * Install tool: A package of installable software may require
 * one or more tools to complete an install. This is the
 * structure of the tool.
 */

struct Install_tool
{

	string_list belongs_to;	/* This identifies who this tool belongs to*/
	string	loadpoint<>;	/* place where installation tool tree resides*/
	bool moveable;		/* can it be moved?			*/
	unsigned size;		/* size of tool tree			*/
	unsigned workspace;	/* how much work space it needs		*/
				/* these are *supplied* values, not	*/
				/* calculated ones.			*/
};

/************************************************
 * Per File info for Installable software	*
 ************************************************/

struct Installable
{
	/*
	 * Architecture Information
	 */
	string_list arch_for;	/* architecture(s) this entity is for	*/
	/*
	 * Dependency Information
	 *
	 * Each installable software package may depend on a list of other
	 * packages. These files MUST be in the same distribution.
	 *
	 */
	string_list soft_depends;	/* software it depend(s) on	*/
	/*
	 * Desirability/Requirement Information
	 * The required flag may be overridden.
	 *
	 */
	bool required;		/* This file is a REQUIRED install	*/
	bool desirable;		/* This file is a DESIRABLE install	*/
	bool common;		/* This file is COMMONLY installed	*/
	/*
	 * Installation Information
	 *
	 * Each software distribution file has additional installation
	 * information. This information includes: a place where the
	 * software normally goes, a flag as to whether this can be
	 * changed, and how big the software is when installed.
	 *
	 * The 'moveable' flag may be explicitly overridden.
	 *
	 */
	string loadpoint<>;	/* default place to put it		*/
	bool moveable;		/* can one put it in other than default?*/
	unsigned size;		/* size (in bytes) of installed package	*/
				/* this is a *supplied* values, not	*/
				/* a calculated one.			*/
	string pre_install<>;	/* Name of a pre-installation install_tool*/
	string install<>;	/* Name of an installation install_tool	*/
};

/**********************************************************
 *							  *
 * For Each Table of Contents Entry, there exists	  *
 * Per File Information                                   *
 *							  *
 * A discriminant union is used to select which kind of	  *
 * information is encoded.				  *
 *							  *
 **********************************************************/

enum file_kind
{
	UNDEFINED,	/* Never use this kind directly..	*/
	CONTENTS,
	AMORPHOUS,
	STANDALONE,
	EXECUTABLE,
	INSTALLABLE,
	INSTALL_TOOL

};

union Information switch (file_kind kind)
{
case CONTENTS:
	contents Toc;
case AMORPHOUS:
	amorphous File;
case STANDALONE:
	standalone Standalone;
case EXECUTABLE:
	executable Exec;
case INSTALLABLE:
	Installable  Install;
case INSTALL_TOOL:
	Install_tool Tool;
default:
	/* reserved for UNDEFINED plus as yet unknown kinds		*/
	/* a specialized routine to handle this is in in xdr_custom.c	*/
	struct futureslop junk;
};


/**********************************************************
 *							  *
 * Information retained on HOW an entry was generated.    *
 *							  *
 * This information may only be meaningful to the release *
 * machine, but the target machine may be able to infer   *
 * something useful from it.				  *
 *							  *
 **********************************************************/


/**********************************************************
 *							  *
 * After the magic & version integers in the table of     *
 * contents is the 'per-distribution' information.        *
 *							  *
 **********************************************************/

struct	distribution_info
{
	string		Title<>;	/* TITLE of distribution	*/
	string		Source<>;	/* SOURCE of distribution	*/
	string		Version<>;	/* VERSION stamp		*/
	string		Date<>;		/* DATE stamp			*/
	int		nentries;	/* # of entries total 		*/
	int		dstate;		/* State of this distribution	*/
					/* == 0, means it is on media	*/
					/* != 0, means that it is in	*/
					/* process of being 'made'	*/
};

/**********************************************************
 *							  *
 * After the distribution info is the info about this     *
 * particular volume.                                     *
 *							  *
 **********************************************************/

struct volume_info
{
	Address	vaddr;		/* Where this particular table	*/
				/* of contents resides		*/
};

/**********************************************************
 *							  *
 * And finally, after the volume info comes an array of   *
 * table of contents entries.                             *
 *							  *
 **********************************************************/

struct	toc_entry
{
	string Name<>;			/* Short name			*/
	string LongName<>;		/* A really long descriptive	*/
	string	Command<>;		/* Command used to 'make' this	*/
					/* this entry, i.e., HOW this 	*/
					/* entity was made		*/
	Address Where;			/* WHERE it is, on what media	*/
	file_type Type;			/* WHAT format is this entity?	*/
	/*
	 * The rest of the information is on a per file KIND basis.
	 * (which is determined by the discriminant of an Information union)
	 *
	 */
	Information	Info;
};
