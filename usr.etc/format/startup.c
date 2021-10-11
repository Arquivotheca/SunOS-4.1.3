
#ifndef lint
static  char sccsid[] = "@(#)startup.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains the code to perform program startup.  This
 * includes reading the data file and the search for disks.
 */
#include "global.h"
#include <sys/fcntl.h>
#include "startup.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <scsi/generic/mode.h>
#include <scsi/generic/sense.h>
#include <scsi/generic/commands.h>
#include <scsi/generic/dad_mode.h>
#include <scsi/impl/mode.h>
#include <scsi/impl/uscsi.h>
#include <scsi/impl/commands.h>


extern	struct ctlr_type ctlr_types[];
extern	int nctypes;
extern	char * strchr();

char	**search_path;

struct chg_list	*new_chg_list();


/*
 * This global is used to store the current line # in the data file.
 * It must be global because the I/O routines are allowed to side
 * effect it to keep track of backslashed newlines.
 */
int	data_lineno;			/* current line # in data file */

/*
 * This routine digests the options on the command line.  It returns
 * the index into argv of the first string that is not an option.  If
 * there are none, it returns -1.
 */
int
do_options(argc, argv)
	int	argc;
	char	*argv[];
{
	register char *ptr;
	register int i, next;

	/*
	 * Default is no extended messages.  Can be enabled manually.
	 */
	option_msg = 0;

	/*
	 * Loop through the argument list, incrementing each time by
	 * an amount determined by the options found.
	 */
	for (i = 1; i < argc; i = next) {
		/*
		 * Start out assuming an increment of 1.
		 */
		next = i + 1;
		/*
		 * As soon as we hit a non-option, we're done.
		 */
		if (*argv[i] != '-')
			return (i);
		/*
		 * Loop through all the characters in this option string.
		 */
		for (ptr = argv[i] + 1; *ptr != '\0'; ptr++) {
			/*
			 * Determine each option represented.  For options
			 * that use a second string, increase the increment
			 * of the main loop so they aren't re-interpreted.
			 */
			switch (*ptr) {
			    case 's':
			    case 'S':
				option_s = 1;
				break;
			    case 'f':
			    case 'F':
				option_f = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    case 'l':
			    case 'L':
				option_l = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    case 'x':
			    case 'X':
				option_x = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    case 'd':
			    case 'D':
				option_d = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    case 't':
			    case 'T':
				option_t = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    case 'p':
			    case 'P':
				option_p = argv[next++];
				if (next > argc)
					goto badopt;
				break;
			    default:
badopt:
				/*
				 * If there was a problem, coach the user.
				 */
				eprint("Usage:  format [-s][-d disk_name]");
				eprint("[-t disk_type][-p partition_name]\n");
				eprint("\t[-f cmd_file][-l log_file]");
				eprint("[-x data_file] disk_list\n");
				fullabort();
			}
		}
	}
	/*
	 * All the command line strings were options.  Return that fact.
	 */
	return (-1);
}

/*
 * This routine reads in and digests the data file.  The data file contains
 * definitions for the search path, known disk types, and known partition
 * maps.
 */
sup_init()
{
	char	*filename;
	int	status;
	TOKEN	token, cleaned;

	/*
	 * If the data file was specified on the command line, use it.
	 */
	if (option_x)
		filename = option_x;
	else {
		/*
		 * Data file was not specified.  Look in the current
		 * directory first, and if that fails look in /etc.
		 * Assume the name of the file is 'format.dat'.
		 */
		filename = "format.dat";
		data_file = fopen(filename, "r");
		if (data_file != NULL)
			goto opened;
		filename = "/etc/format.dat";
	}
	/*
	 * If no data file could be found, we're history.
	 */
	data_file = fopen(filename, "r");
	if (data_file == NULL) {
		eprint("Unable to open data file '%s'.\n", filename);
		fullabort();
	}
opened:
	/*
	 * Step through the data file a meta-line at a time.  There are
	 * typically several backslashed newlines in each meta-line,
	 * so data_lineno will be getting side effected along the way.
	 */
	for (data_lineno = 1;; data_lineno++) {
		/*
		 * Get the keyword.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit the end of the data file, we're done.
		 */
		if (status == SUP_EOF)
			break;
		/*
		 * If the line is blank, skip it.
		 */
		if (status == SUP_EOL)
			continue;
		/*
		 * If the line starts with some key character, it's an error.
		 */
		if (status != SUP_STRING) {
			eprint("Expecting keyword, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Clean up the token and see which keyword it is.  Call
		 * the appropriate routine to process the rest of the line.
		 */
		clean_token(cleaned, token);
		if (strcmp(cleaned, "search_path") == 0)
			sup_setpath();
		else if (strcmp(cleaned, "disk_type") == 0)
			sup_setdtype();
		else if (strcmp(cleaned, "partition") == 0)
			sup_setpart();
		else {
			eprint("Bad keyword '%s' ", cleaned);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
	}
	/*
	 * Close the data file.
	 */
	(void) fclose(data_file);
}

/*
 * This routine processes a 'search_path' line in the data file.  The
 * search path is a list of disk names that will be searched for by the
 * program.
 */
sup_setpath()
{
	TOKEN	token, cleaned;
	int	status, count;

	/*
	 * Only allow one definition of the search path.
	 */
	if (search_path != NULL) {
		eprint("Search path redefined -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	/*
	 * Pull in some grammar.
	 */
	status = sup_gettoken(token);
	if (status != SUP_EQL) {
		eprint("Expecting '=', found '%s' ", token);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	/*
	 * Allocate space for the first entry.
	 */
	search_path = (char **)zalloc(sizeof (char *));
	count = 1;
	/*
	 * Loop through the entries.
	 */
	for (;;) {
		/*
		 * Pull in the disk name.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit end of line, we're done.
		 */
		if (status == SUP_EOL)
			break;
		/*
		 * If we hit some key character, it's an error.
		 */
		if (status != SUP_STRING) {
			eprint("Expecting value, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(cleaned, token);
		/*
		 * If there are meta characters then lets expand them
		 */
		if (strchr(cleaned,'?') || strchr(cleaned,'[')){
			add_metadevices(cleaned,&count);
		}
		else {
			add_device(cleaned,&count);
		}
		/*
		 * Pull in some grammar.
		 */
		status = sup_gettoken(token);
		if (status == SUP_EOL)
			break;
		if (status != SUP_COMMA) {
			eprint("Expecting ',', found '%s' ", token);
			eprint(" -- line %d of data file.\n", data_lineno);
			fullabort();
		}
	}
}

char *ascii = "0123456789abcdef";
/* 
 * This routine processes the meta characters in a device name
 *
 * The currently supported meta characters are 
 *	?	a digit from 0-0xF
 *      [a-b]   range of values from a to b inclusive
 *      [abc]   use a,b & c individually
 *
 */
add_metadevices(cleaned,count)
TOKEN cleaned;
int *count;
{

	TOKEN hold;
	int i;
	char *pos,*close,*cur;
	char * c;
	char *start,*end;

	/*
	 * check if there is a '?' in the string
	 */
	if (strchr(cleaned,'?') != NULL) {
		for (i = 0 ; i < 0x10 ; i++) {
			(void) strncpy(hold,cleaned,sizeof(TOKEN));
			c = strchr(hold,'?');
			if (i < 0xA)
				*c = i + '0';
			else 
				*c = (i - 0xA) + 'a';
			add_metadevices(hold,count);
		}
	} else if ((cur = pos = strchr(cleaned,'[')) != NULL) {
		close = strchr(cleaned,']');
		if (close == NULL){
			eprint("Expecting ']' ");
			goto abort;
		}
		while (++cur < close) {
			if (!isxdigit(*cur)){
				eprint("Expecting digit ");
				goto abort;
			}
			/*
			 * check if they want a range of values
			 */
			if (*(cur + 1) == '-') {
				if (!isxdigit(*(cur + 2))){
					eprint("Expecting digit ");
					goto abort;
				}
				start = strchr(ascii,*cur);
				end = strchr(ascii,*(cur + 2));
				if (start == NULL || end == NULL) {
					eprint("Expecting lower case ");
					goto abort;
				}
				/*
				 * Build a device name for each digit
				 */
				while (start <= end) {
					bzero(hold,sizeof(TOKEN));
					if (pos != cleaned)
						strncpy(hold,cleaned,pos - cleaned);
					strncat(hold,start,1);
					strcat(hold,close + 1);
					add_metadevices(hold,count);
					start++;
				}
				cur += 2;
			}
			else {
				bzero(hold,sizeof(TOKEN));
				if (pos != cleaned)
					strncpy(hold,cleaned,pos - cleaned);
				strncat(hold,cur,1);
				strcat(hold,close + 1);
				add_metadevices(hold,count);
			}
		}
	}
	else	{
		add_device(cleaned,count);
	}
	return;
abort:
	eprint(" -- line %d of data file.\n", data_lineno);
	fullabort();
}

/*
 * This routine adds a device name to the search path
 */
add_device(cleaned,count)
TOKEN cleaned;
int *count;
{
	char    filename[DK_DEVLEN + 8] ;
	struct  stat buf;
	int	i;

	/*
	 * Check if there is a device entry for it 
	 * The reason that this is done is that if the search path
	 * gets over about 4000 entries the behavior of the rezalloc,
	 * zalloc calls becomes n*n.  This eats up a lot of memory
	 * and would panic MUNIX.  This is a "feature" of the current
	 * malloc algorithm.  Doing this also protects against
	 * changes in the malloc algorithm in the future since most
	 * smaller systems will not have a lot of dev entries
	 */
	filename[0] = '\0';
	(void) strcat(filename, "/dev/r");
	(void) strncat(filename, cleaned, DK_DEVLEN);
	i = strlen(filename);
	filename[i] = 'c';
	filename[i+1] = '\0';
	if (stat(filename,&buf) < 0)
		return;

	/*
	 * Reallocate the search path array to be one larger.
	 */
	search_path = (char **)rezalloc((char *)search_path,
	    (sizeof (char *)) * (*count + 1));
	/*
	 * Allocate space for the disk name and copy it in.
	 */
	search_path[*count - 1] = zalloc(strlen(cleaned) + 1);
	(void) strcpy(search_path[*count - 1], cleaned);
	/*
	 * Terminate the array with a null.
	 */
	search_path[(*count)++] = NULL;
}

/*
 * This routine processes a 'disk_type' line in the data file.  It defines
 * the physical attributes of a brand of disk when connected to a specific
 * controller type.
 */
sup_setdtype()
{
	TOKEN	token, cleaned, ident;
	int	val, status, i;
	u_long	flags = 0;
	struct	disk_type *dtype, *type;
	struct	ctlr_type *ctype;
	char	*dtype_name, *ptr;

	/*
	 * Pull in some grammar.
	 */
	status = sup_gettoken(token);
	if (status != SUP_EQL) {
		eprint("Expecting '=', found '%s' ", token);
		eprint(" -- line %d of data file.\n", data_lineno);
		fullabort();
	}
	/*
	 * Pull in the name of the disk type.
	 */
	status = sup_gettoken(token);
	if (status != SUP_STRING) {
		eprint("Expecting value, found '%s' ", token);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	clean_token(cleaned, token);
	/*
	 * Allocate space for the disk type and copy in the name.
	 */
	dtype_name = zalloc(strlen(cleaned) + 1);
	(void) strcpy(dtype_name, cleaned);
	dtype = (struct disk_type *)zalloc(sizeof (*dtype));
	dtype->dtype_asciilabel = dtype_name;
	/*
	 * Loop for each attribute.
	 */
	for (;;) {
		/*
		 * Pull in some grammar.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit end of line, we're done.
		 */
		if (status == SUP_EOL)
			break;
		if (status != SUP_COLON) {
			eprint("Expecting ':', found '%s' ", token);
			eprint(" -- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in the attribute.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit end of line, we're done.
		 */
		if (status == SUP_EOL)
			break;
		/*
		 * If we hit a key character, it's an error.
		 */
		if (status != SUP_STRING) {
			eprint("Expecting keyword, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(ident, token);
		/*
		 * Check to see if we've got a change specification.
		 * If so, this routine will parse the entire
		 * specification, so just restart at top of loop.
		 */
		if (sup_change_spec(dtype, ident)) {
			continue;
		}
		/*
		 * Pull in more grammar.
		 */
		status = sup_gettoken(token);
		if (status != SUP_EQL) {
			eprint("Expecting '=', found '%s' ", token);
			eprint(" -- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in the value of the attribute.
		 */
		status = sup_gettoken(token);
		if (status != SUP_STRING) {
			eprint("Expecting value, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(cleaned, token);
		/*
		 * If the attribute defined the ctlr...
		 */
		if (strcmp(ident, "ctlr") == 0) {
			/*
			 * Match the value with a ctlr type.
			 */
			for (i = 0; i < nctypes; i++)
				if (strcmp(ctlr_types[i].ctype_name,
				    cleaned) == 0)
					break;
			/*
			 * If we couldn't match it, it's an error.
			 */
			if (i >= nctypes) {
				eprint("Value '%s' is not a known ctlr name ",
				    cleaned);
				eprint("-- line %d of data file.\n",
				    data_lineno);
				fullabort();
			}
			/*
			 * Found a match.  Add this disk type to the list
			 * for the ctlr type.
			 */
			ctype = &ctlr_types[i];
			type = ctype->ctype_dlist;
			if (type == NULL)
				ctype->ctype_dlist = dtype;
			else {
				while (type->dtype_next != NULL)
					type = type->dtype_next;
				type->dtype_next = dtype;
			}
			/*
			 * Note that this attribute is defined.
			 */
			flags |= SUP_CTLR;
			continue;
		}
		/*
		 * All other attributes require a numeric value.  Convert
		 * the value to a number.
		 */
		val = (int)strtol(cleaned, &ptr, 0);
		if (*ptr != '\0') {
			eprint("Value '%s' is not an integer ", cleaned);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Figure out which attribute it was and fill in the
		 * appropriate value.  Also note that the attribute
		 * has been defined.
		 */
		if (strcmp(ident, "ncyl") == 0) {
			dtype->dtype_ncyl = val;
			flags |= SUP_NCYL;
		} else if (strcmp(ident, "acyl") == 0) {
			dtype->dtype_acyl = val;
			flags |= SUP_ACYL;
		} else if (strcmp(ident, "pcyl") == 0) {
			dtype->dtype_pcyl = val;
			flags |= SUP_PCYL;
		} else if (strcmp(ident, "nhead") == 0) {
			dtype->dtype_nhead = val;
			flags |= SUP_NHEAD;
		} else if (strcmp(ident, "nsect") == 0) {
			dtype->dtype_nsect = val;
			flags |= SUP_NSECT;
		} else if (strcmp(ident, "rpm") == 0) {
			dtype->dtype_rpm = val;
			flags |= SUP_RPM;
		} else if (strcmp(ident, "bpt") == 0) {
			dtype->dtype_bpt = val;
			flags |= SUP_BPT;
		} else if (strcmp(ident, "bps") == 0) {
			dtype->dtype_bps = val;
			flags |= SUP_BPS;
		} else if (strcmp(ident, "drive_type") == 0) {
			dtype->dtype_dr_type = val;
			flags |= SUP_DRTYPE;
		} else if (strcmp(ident, "skew") == 0) {
			dtype->dtype_buf_sk = val;
			flags |= SUP_SKEW;
		} else if (strcmp(ident, "precomp") == 0) {
			dtype->dtype_wr_prcmp = val;
			flags |= SUP_PRCMP;
		} else if (strcmp(ident, "cache") == 0) {
			dtype->dtype_cache = val;
			flags |= SUP_CACHE;
		} else if (strcmp(ident, "prefetch") == 0) {
			dtype->dtype_threshold = val;
			flags |= SUP_PREFETCH;
		} else if (strcmp(ident, "min_prefetch") == 0) {
			dtype->dtype_prefetch_min = val;
			flags |= SUP_CACHE_MIN;
		} else if (strcmp(ident, "max_prefetch") == 0) {
			dtype->dtype_prefetch_max = val;
			flags |= SUP_CACHE_MAX;
		} else if (strcmp(ident, "trks_zone") == 0) {
			dtype->dtype_trks_zone = val;
			flags |= SUP_TRKS_ZONE;
		} else if (strcmp(ident, "atrks") == 0) {
			dtype->dtype_atrks = val;
			flags |= SUP_ATRKS;
		} else if (strcmp(ident, "asect") == 0) {
			dtype->dtype_asect = val;
			flags |= SUP_ASECT;
		} else if (strcmp(ident, "fmt_time") == 0) {
			dtype->dtype_fmt_time = val;
		} else if (strcmp(ident, "cyl_skew") == 0) {
			dtype->dtype_cyl_skew = val;
			flags |= SUP_CYLSKEW;
		} else if (strcmp(ident, "trk_skew") == 0) {
			dtype->dtype_trk_skew = val;
			flags |= SUP_TRKSKEW;
		} else if (strcmp(ident, "phead") == 0) {
			dtype->dtype_phead = val;
			flags |= SUP_PHEAD;
		} else if (strcmp(ident, "psect") == 0) {
			dtype->dtype_psect = val;
			flags |= SUP_PSECT;
		} else if (strcmp(ident, "read_retries") == 0) {
			dtype->dtype_read_retries = val;
			flags |= SUP_READ_RETRIES;
		} else if (strcmp(ident, "write_retries") == 0) {
			dtype->dtype_write_retries = val;
			flags |= SUP_WRITE_RETRIES;
		} else {
			eprint("Bad keyword '%s' -- line %d of data file.\n",
			    ident, data_lineno);
			fullabort();
		}
	}
	/*
	 * Check to be sure all the necessary attributes have been defined.
	 * If any are missing, it's an error.  Also, log options for later
	 * use by specific driver.
	 */
	dtype->dtype_options = flags;
	if ((flags & SUP_MIN_DRIVE) != SUP_MIN_DRIVE) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	if ((ctype->ctype_flags & CF_SMD_DEFS) && (!(flags & SUP_BPS))) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	if ((ctype->ctype_flags & CF_450_TYPES) && (!(flags & SUP_DRTYPE))) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	if ((ctype->ctype_flags & CF_BSK_WPR) && (!(flags & SUP_SKEW))) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	if ((ctype->ctype_flags & CF_BSK_WPR) && (!(flags & SUP_PRCMP))) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
}

/*
 * This routine processes a 'partition' line in the data file.  It defines
 * a known partition map for a particular disk type on a particular
 * controller type.
 */
sup_setpart()
{
	TOKEN	token, cleaned, disk, ctlr, ident;
	struct	disk_type *dtype;
	struct	ctlr_type *ctype;
	struct	partition_info *pinfo, *parts;
	char	*pinfo_name, *ptr;
	int	i, index, status, val1, val2, flags = 0;

	/*
	 * Pull in some grammar.
	 */
	status = sup_gettoken(token);
	if (status != SUP_EQL) {
		eprint("Expecting '=', found '%s' ", token);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	/*
	 * Pull in the name of the map.
	 */
	status = sup_gettoken(token);
	if (status != SUP_STRING) {
		eprint("Expecting value, found '%s' ", token);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	clean_token(cleaned, token);
	/*
	 * Allocate space for the partition map and fill in the name.
	 */
	pinfo_name = zalloc(strlen(cleaned) + 1);
	(void) strcpy(pinfo_name, cleaned);
	pinfo = (struct partition_info *)zalloc(sizeof (*pinfo));
	pinfo->pinfo_name = pinfo_name;
	/*
	 * Loop for each attribute in the line.
	 */
	for (;;) {
		/*
		 * Pull in some grammar.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit end of line, we're done.
		 */
		if (status == SUP_EOL)
			break;
		if (status != SUP_COLON) {
			eprint("Expecting ':', found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in the attribute.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit end of line, we're done.
		 */
		if (status == SUP_EOL)
			break;
		if (status != SUP_STRING) {
			eprint("Expecting keyword, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(ident, token);
		/*
		 * Pull in more grammar.
		 */
		status = sup_gettoken(token);
		if (status != SUP_EQL) {
			eprint("Expecting '=', found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in the value of the attribute.
		 */
		status = sup_gettoken(token);
		/*
		 * If we hit a key character, it's an error.
		 */
		if (status != SUP_STRING) {
			eprint("Expecting value, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(cleaned, token);
		/*
		 * If the attribute is the ctlr, save the ctlr name and
		 * mark it defined.
		 */
		if (strcmp(ident, "ctlr") == 0) {
			(void) strcpy(ctlr, cleaned);
			flags |= SUP_CTLR;
			continue;
		/*
		 * If the attribute is the disk, save the disk name and
		 * mark it defined.
		 */
		} else if (strcmp(ident, "disk") == 0) {
			(void) strcpy(disk, cleaned);
			flags |= SUP_DISK;
			continue;
		}
		/*
		 * All other attributes have a pair of numeric values.
		 * Convert the first value to a number.
		 */
		val1 = (int)strtol(cleaned, &ptr, 0);
		if (*ptr != '\0') {
			eprint("Value '%s' is not an integer ", cleaned);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in some grammar.
		 */
		status = sup_gettoken(token);
		if (status != SUP_COMMA) {
			eprint("Expecting ',', found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * Pull in the second value.
		 */
		status = sup_gettoken(token);
		if (status != SUP_STRING) {
			eprint("Expecting value, found '%s' ", token);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		clean_token(cleaned, token);
		/*
		 * Convert the second value to a number.
		 */
		val2 = (int)strtol(cleaned, &ptr, 0);
		if (*ptr != '\0') {
			eprint("Value '%s' is not an integer ", cleaned);
			eprint("-- line %d of data file.\n", data_lineno);
			fullabort();
		}
		/*
		 * The rest of the attributes are all single letters.
		 * Make sure the specified attribute is a single letter.
		 */
		if (strlen(ident) != 1) {
			eprint("Bad keyword '%s' -- line %d of data file.\n",
			    ident, data_lineno);
			fullabort();
		}
		/*
		 * Also make sure it is within the legal range of letters.
		 */
		if (ident[0] < 'a' || ident[0] > 'h') {
			eprint("Bad keyword '%s' -- line %d of data file.\n",
			    ident, data_lineno);
			fullabort();
		}
		/*
		 * Fill in the appropriate map entry with the values.
		 */
		index = ident[0] - 'a';
		pinfo->pinfo_map[index].dkl_cylno = val1;
		pinfo->pinfo_map[index].dkl_nblk = val2;
	}
	/*
	 * Check to be sure that all necessary attributes were defined.
	 */
	if ((flags & SUP_MIN_PART) != SUP_MIN_PART) {
		eprint("Incomplete specification -- line %d of data file.\n",
		    data_lineno);
		fullabort();
	}
	/*
	 * Attempt to match the specified ctlr to a known type.
	 */
	for (i = 0; i < nctypes; i++)
		if (strcmp(ctlr_types[i].ctype_name, ctlr) == 0)
			break;
	/*
	 * If no match is found, it's an error.
	 */
	if (i >= nctypes) {
		eprint("Value '%s' is not a known ctlr name ", ctlr);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	ctype = &ctlr_types[i];
	/*
	 * Attempt to match the specified disk to a known type.
	 */
	for (dtype = ctype->ctype_dlist; dtype != NULL;
	    dtype = dtype->dtype_next)
		if (strcmp(dtype->dtype_asciilabel, disk) == 0)
			break;
	/*
	 * If no match is found, it's an error.
	 */
	if (dtype == NULL) {
		eprint("Value '%s' is not a known disk name ", disk);
		eprint("-- line %d of data file.\n", data_lineno);
		fullabort();
	}
	/*
	 * Add this partition map to the list of known maps for the
	 * specified disk/ctlr.
	 */
	parts = dtype->dtype_plist;
	if (parts == NULL)
		dtype->dtype_plist = pinfo;
	else {
		while (parts->pinfo_next != NULL)
			parts = parts->pinfo_next;
		parts->pinfo_next = pinfo;
	}
}

/*
 * This routine opens the raw C partition of the specified disk with
 * the specified flags.
 */
int
open_disk(diskname, flags)
	char	*diskname;
	int	flags;
{
	char	filename[DK_DEVLEN + 8];
	int	i;

	/*
	 * Construct the pathname for the given disk.
	 */
	filename[0] = '\0';
	(void) strcat(filename, "/dev/r");
	(void) strncat(filename, diskname, DK_DEVLEN);
	i = strlen(filename);
	filename[i] = 'c';
	filename[i+1] = '\0';
	/*
	 * Open the disk and return status.
	 */
	return (open(filename, flags));
}

/*
 * This routine performs the disk search during startup.  It looks for
 * all the disks in the search path, and creates a list of those that
 * are found.
 */
do_search(arglist)
	char	*arglist[];
{
	register struct disk_info *search_disk, *dptr;
	register struct ctlr_info *search_ctlr, *cptr;
	struct	disk_type *search_dtype, *type;
	struct	partition_info *search_parts, *parts;
	struct	dk_conf search_ioctl;
	struct	dk_label search_label;
	struct	ctlr_ops *ops;
	int	search_file, i, status;

	/*
	 * Assume we're talking to a driver which supports
	 * the uscsi interface, if SCSI.  If the driver is
	 * actually the sun4 driver, the first uscsi ioctl
	 * will fail, and that and future ioctls will be
	 * redirected to the old "generic" interface via
	 * this flag.
	 */
	sun4_flag = 0;

	printf("Searching for disks...");
	(void) fflush(stdout);
	/*
	 * If there wasn't a search path defined in the data file or on
	 * the command line, there's nothing we can do.
	 */
	if (arglist == NULL)
		goto out;
	/*
	 * Step through the array of disks on the search list.
	 */
	for (; *arglist != NULL; arglist++) {
		/*
		 * Attempt to open the disk.  If it fails, skip it.
		 */
		if ((search_file = open_disk(*arglist,
		    O_RDWR | FNDELAY)) < 0)
			continue;
		/*
		 * Attempt to read the configuration info on the disk.
		 * Again, if it fails, we assume the disk's not there.
		 * Note we must close the file for the disk before we
		 * continue.
		 */
		if (ioctl(search_file, DKIOCGCONF, &search_ioctl) < 0) { 
			(void) close(search_file);
			continue;
		}
		/*
		 * Here we play a little trick, if we've got ourselves
		 * a SCSI device.  We get the inquiry information,
		 * and, if what we're looking at is not an MD21
		 * device, we convert the ctype from DKC_MD21 to
		 * DKC_SCSI_CCS.
		 */
		if (search_ioctl.dkc_ctype == DKC_MD21) {
			static char *emustring = "EMULEX  MD21/S2";
			char inqbuf[255];
			struct scsi_inquiry *inq;
			if (uscsi_inquiry(search_file, inqbuf, sizeof (inqbuf))) {
				/*
				 * Inquiry failed - punt
				 */
				(void) close(search_file);
				continue;
			}
			inq = (struct scsi_inquiry *) inqbuf;
			if (bcmp(inq->inq_vid, emustring,
					strlen(emustring)) != 0) {
				search_ioctl.dkc_ctype = DKC_SCSI_CCS;
			}
		}
		/*
		 * The disk appears to be present.  Allocate space for the
		 * disk structure and add it to the list of found disks.
		 */
		search_disk = (struct disk_info *)zalloc(sizeof *search_disk);
		if (disk_list == NULL)
			disk_list = search_disk;
		else {
			for (dptr = disk_list; dptr->disk_next != NULL;
			    dptr = dptr->disk_next)
				;
			dptr->disk_next = search_disk;
		}
		/*
		 * Fill in some info from the ioctl.
		 */
		search_disk->disk_unit = search_ioctl.dkc_unit;
		search_disk->disk_slave = search_ioctl.dkc_slave;
		/*
		 * Try to match the ctlr for this disk with a ctlr we
		 * have already found.  A match is assumed if the ctlrs
		 * are at the same address && ctypes agree
		 */
		for (search_ctlr = ctlr_list; search_ctlr != NULL;
		    search_ctlr = search_ctlr->ctlr_next)
			if (search_ctlr->ctlr_addr == search_ioctl.dkc_addr &&
			    search_ctlr->ctlr_space == search_ioctl.dkc_space
				&& search_ctlr->ctlr_ctype->ctype_ctype ==
					search_ioctl.dkc_ctype)
				break;
		/*
		 * If no match was found, we need to identify this ctlr.
		 */
		if (search_ctlr == NULL) {
			/*
			 * Match the type of the ctlr to a known type.
			 */
			for (i = 0; i < nctypes; i++)
				if (ctlr_types[i].ctype_ctype ==
				    search_ioctl.dkc_ctype)
					break;
			/*
			 * If no match was found, it's an error.
			 * Close the disk and report the error.
			 */
			if (i >= nctypes) {
				eprint("\nError: found disk attached to ");
				eprint("unsupported controller type '%d'.\n",
				    search_ioctl.dkc_ctype);
				(void) close(search_file);
				continue;
			}
			/*
			 * Allocate space for the ctlr structure and add it
			 * to the list of found ctlrs.
			 */
			search_ctlr = (struct ctlr_info *)
			    zalloc(sizeof *search_ctlr);
			search_ctlr->ctlr_ctype = &ctlr_types[i];
			if (ctlr_list == NULL)
				ctlr_list = search_ctlr;
			else {
				for (cptr = ctlr_list; cptr->ctlr_next != NULL;
				    cptr = cptr->ctlr_next)
					;
				cptr->ctlr_next = search_ctlr;
			}
			/*
			 * Fill in info from the ioctl.
			 */
			for (i = 0; i < DK_DEVLEN; i++) {
				search_ctlr->ctlr_cname[i] =
				    search_ioctl.dkc_cname[i];
				search_ctlr->ctlr_dname[i] =
				    search_ioctl.dkc_dname[i];
			}
			search_ctlr->ctlr_flags = search_ioctl.dkc_flags;
			search_ctlr->ctlr_num = search_ioctl.dkc_cnum;
			search_ctlr->ctlr_addr = search_ioctl.dkc_addr;
			search_ctlr->ctlr_space = search_ioctl.dkc_space;
			search_ctlr->ctlr_prio = search_ioctl.dkc_prio;
			search_ctlr->ctlr_vec = search_ioctl.dkc_vec;
		}
		/*
		 * By this point, we have a known ctlr.  Link the disk
		 * to the ctlr.
		 */
		search_disk->disk_ctlr = search_ctlr;
		/*
		 * Attempt to read the primary label.  Try twice.
		 */
		ops = search_ctlr->ctlr_ctype->ctype_ops;
		status = (*ops->rdwr)(DIR_READ, search_file, (daddr_t)0, 1,
		    (char *)&search_label, F_SILENT);
		/*
		 * SCSI may need second or thind chance before it's ready.
		 */
		if (status != 0) {
			(void) (*ops->rdwr)(DIR_READ, search_file, (daddr_t)0, 1,
			    (char *)&search_label, F_SILENT);
			status = (*ops->rdwr)(DIR_READ, search_file, (daddr_t)0, 1,
			    (char *)&search_label, F_SILENT);
		}
		/*
		 * Close the file for this disk.
		 */
		(void) close(search_file);
		/*
		 * If we didn't successfully read the label, or the label
		 * appears corrupt, just leave the disk as an unknown type.
		 */
		if (status)
			continue;
		if (!checklabel(&search_label))
			continue;
		if (trim_id(search_label.dkl_asciilabel))
			continue;
		/*
		 * The label looks ok.  Mark the disk as labeled.
		 */
		search_disk->disk_flags |= DSK_LABEL;
		/*
		 * Attempt to match the disk type in the label with a
		 * known disk type.
		 */
		for (search_dtype = search_ctlr->ctlr_ctype->ctype_dlist;
		    search_dtype != NULL;
		    search_dtype = search_dtype->dtype_next)
			if (dtype_match(&search_label, search_dtype))
				break;
		/*
		 * If no match was found, we need to create a disk type
		 * for this disk.
		 */
		if (search_dtype == NULL) {
			/*
			 * Allocate space for the disk type and add it
			 * to the list of disk types for this ctlr type.
			 */
			search_dtype = (struct disk_type *)
			    zalloc(sizeof *search_dtype);
			type = search_ctlr->ctlr_ctype->ctype_dlist;
			if (type == NULL)
				search_ctlr->ctlr_ctype->ctype_dlist =
				    search_dtype;
			else {
				while (type->dtype_next != NULL)
					type = type->dtype_next;
				type->dtype_next = search_dtype;
			}
			/*
			 * Fill in the drive info from the disk label.
			 */
			search_dtype->dtype_next = NULL;
			search_dtype->dtype_asciilabel =
			    zalloc(strlen(search_label.dkl_asciilabel) + 1);
			(void) strcpy(search_dtype->dtype_asciilabel,
			    search_label.dkl_asciilabel);
			search_dtype->dtype_pcyl = search_label.dkl_pcyl;
			search_dtype->dtype_ncyl = search_label.dkl_ncyl;
			search_dtype->dtype_acyl = search_label.dkl_acyl;
			search_dtype->dtype_nhead = search_label.dkl_nhead;
			search_dtype->dtype_nsect = search_label.dkl_nsect;
			search_dtype->dtype_rpm = search_label.dkl_rpm;
			/*
			 * Mark the disk as needing specification of
			 * ctlr specific attributes.  This is necessary
			 * because the label doesn't contain these attributes,
			 * and they aren't known at this point.  They will
			 * be asked for if this disk is ever selected by
			 * the user.
			 */
			search_dtype->dtype_flags |= DT_NEED_SPEFS;
		}
		/*
		 * By this time we have a known disk type.  Link the disk
		 * to the disk type.
		 */
		search_disk->disk_type = search_dtype;
		/*
		 * Attempt to match the partition map in the label with
		 * a known partition map for this disk type.
		 */
		for (search_parts = search_dtype->dtype_plist;
		    search_parts != NULL;
		    search_parts = search_parts->pinfo_next)
			if (parts_match(&search_label, search_parts))
				break;
		/*
		 * If no match was made, we need to create a partition
		 * map for this disk.
		 */
		if (search_parts == NULL) {
			/*
			 * Allocate space for the partition map and add
			 * it to the list of maps for this disk type.
			 */
			search_parts = (struct partition_info *)
			    zalloc(sizeof *search_parts);
			parts = search_dtype->dtype_plist;
			if (parts == NULL)
				search_dtype->dtype_plist = search_parts;
			else {
				while (parts->pinfo_next != NULL)
					parts = parts->pinfo_next;
				parts->pinfo_next = search_parts;
			}
			search_parts->pinfo_next = NULL;
			/*
			 * Fill in the name of the map with a name derived
			 * from the name of this disk.  This is necessary
			 * because the label contains no name for the
			 * partition map.
			 */
			i = strlen("original ") + DK_DEVLEN;
			search_parts->pinfo_name = zalloc(i);
			if (search_ctlr->ctlr_ctype->ctype_ctype != DKC_PANTHER) {
			    (void) sprintf(search_parts->pinfo_name, "%s%s%d",
				"original ", search_ctlr->ctlr_dname,
				search_disk->disk_unit);
			} else {
			    (void) sprintf(search_parts->pinfo_name, "%s%s%03x",
				"original ", search_ctlr->ctlr_dname,
				search_disk->disk_unit);
			}
			/*
			 * Fill in the partition info from the disk label.
			 */
			for (i = 0; i < NDKMAP; i++)
				search_parts->pinfo_map[i] =
				    search_label.dkl_map[i];
		}
		/*
		 * By this time we have a known partitition map.  Link the
		 * disk to the partition map.
		 */
		search_disk->disk_parts = search_parts;
	}
	/*
	 * If we didn't find any disks, give up.
	 */
out:
	if (disk_list == NULL) {
		if (geteuid() == 0) {
			eprint("No disks found!\n");
		} else {
			eprint("No permission (or no disks found)!\n");
		}
		(void) fflush(stdout);
		fullabort();
	}
	printf("done\n");
}

/*
 * This routine checks to see if a given disk type matches the type
 * in the disk label.
 */
int
dtype_match(label, dtype)
	register struct dk_label *label;
	register struct disk_type *dtype;
{

	/*
	 * If the any of the physical characteristics are different, or
	 * the name is different, it doesn't match.
	 */
	if ((strcmp(label->dkl_asciilabel, dtype->dtype_asciilabel) != 0) ||
	    (label->dkl_ncyl != dtype->dtype_ncyl) ||
	    (label->dkl_acyl != dtype->dtype_acyl) ||
	    (label->dkl_nhead != dtype->dtype_nhead) ||
	    (label->dkl_nsect != dtype->dtype_nsect))
		return (0);
	/*
	 * If those are all identical, assume it's a match.
	 */
	return (1);
}

/*
 * This routine checks to see if a given partition map matches the map
 * in the disk label.
 */
int
parts_match(label, pinfo)
	register struct dk_label *label;
	register struct partition_info *pinfo;
{
	int i;

	/*
	 * If any of the partition entries is different, it doesn't match.
	 */
	for (i = 0; i < NDKMAP; i++)
		if ((label->dkl_map[i].dkl_cylno !=
		    pinfo->pinfo_map[i].dkl_cylno) ||
		    (label->dkl_map[i].dkl_nblk !=
		    pinfo->pinfo_map[i].dkl_nblk))
			return (0);
	/*
	 * If they are all identical, it's a match.
	 */
	return (1);
}

/*
 * This routine checks to see if the given disk name refers to the disk
 * in the given disk structure.
 */
int
diskname_match(name, disk)
	char	*name;
	register struct disk_info *disk;
{
	char	*ptr;
	int	i, len;

	/*
	 * Check to be sure the disk name starts out with the correct
	 * letters for this disk type.
	 */
	len = strlen(disk->disk_ctlr->ctlr_dname);
	if (strcnt(name, disk->disk_ctlr->ctlr_dname) != len)
		return (0);
	/*
	 * Extract the unit number from the disk name.
	 */
	i = (int)strtol(name + len, &ptr, 10);
	if ((ptr - name) != strlen(name))
		return (0);
	/*
	 * Make sure the unit number matches.
	 */
	if (i != disk->disk_unit)
		return (0);
	return (1);
}



/*
 * Parse a SCSI mode page change specification.
 *
 * Return:
 *		0:  not change specification, continue parsing
 *		1:  was change specification, it was ok,
 *		    or we already handled the error.
 */
int
sup_change_spec(disk, id)
	struct disk_type	*disk;
	char			*id;
{
	char		*p;
	char		*p2;
	int		pageno;
	int		byteno;
	int		mode;
	int		value;
	TOKEN		token;
	TOKEN		ident;
	struct chg_list	*cp;
	int		tilde;
	int		i;

	/*
	 * Syntax: p[<nn>|0x<xx>]
	 */
	if (*id != 'p') {
		return (0);
	}
	pageno = (int) strtol(id+1, &p2, 0);
	if (*p2 != 0) {
		return (0);
	}
	/*
	 * Once we get this far, we know we have the
	 * beginnings of a change specification.
	 * If there's a problem now, we report the problem,
	 * and abort.
	 */
	if (!scsi_supported_page(pageno)) {
		eprint("Unsupported mode page '%s'", id);
		fullabort();
	}
	/*
	 * Next token should be the byte offset
	 */
	if (sup_gettoken(token) != SUP_STRING) {
		eprint("Unexpected value '%s'", token);
		fullabort();
	}
	clean_token(ident, token);

	/*
	 * Syntax: b[<nn>|0x<xx>]
	 */
	p = ident;
	if (*p++ != 'b') {
		eprint("Unknown keyword '%s'", ident);
		fullabort();
	}
	byteno = (int) strtol(p, &p2, 10);
	if (*p2 != 0) {
		eprint("Unknown keyword '%s'", ident);
		fullabort();
	}
	if (byteno == 0 || byteno == 1) {
		eprint("Unsupported byte offset '%s'", ident);
		fullabort();
	}

	/*
	 * Get the operator for this expression
	 */
	mode = CHG_MODE_UNDEFINED;
	switch (sup_gettoken(token)) {
	case SUP_EQL:
		mode = CHG_MODE_ABS;
		break;
	case SUP_OR:
		if (sup_gettoken(token) == SUP_EQL)
			mode = CHG_MODE_SET;
		break;
	case SUP_AND:
		if (sup_gettoken(token) == SUP_EQL)
			mode = CHG_MODE_CLR;
		break;
	}
	if (mode == CHG_MODE_UNDEFINED) {
		eprint("Unexpected operator: '%s'", token);
		fullabort();
	}

	/*
	 * Get right-hand of expression - accept optional tilde
	 */
	tilde = 0;
	if ((i = sup_gettoken(token)) == SUP_TILDE) {
		tilde = 1;
		i = sup_gettoken(token);
	}
	if (i != SUP_STRING) {
		eprint("Expecting value, found '%s'", token);
		fullabort();
	}
	clean_token(ident, token);
	value = (int) strtol(ident, &p, 0);
	if (*p != 0) {
		eprint("Expecting value, found '%s'", token);
		fullabort();
	}

	/*
	 * Apply the tilde operator, if found.
	 * Constrain to a byte value.
	 */
	if (tilde) {
		value = ~value;
	}
	value &= 0xff;

	/*
	 * We parsed a successful change specification expression.
	 * Add it to the list for this disk type.
	 */
	cp = new_chg_list(disk);
	cp->pageno = pageno;
	cp->byteno = byteno;
	cp->mode = mode;
	cp->value = value;
	return (1);
}


/*
 * Create a new chg_list structure, and append it onto the
 * end of the current chg_list under construction.  By
 * applying changes in the order in which listed in the
 * data file, the changes we make are deterministic.
 * Return a pointer to the new structure, so that the
 * caller can fill in the appropriate information.
 */
struct chg_list *
new_chg_list(disk)
	struct disk_type	*disk;
{
	struct chg_list		*cp;
	struct chg_list		*nc;

	nc = (struct chg_list *) malloc(sizeof (struct chg_list));

	if (disk->dtype_chglist == NULL) {
		disk->dtype_chglist = nc;
	} else {
		for (cp = disk->dtype_chglist; cp->next; cp = cp->next)
			;
		cp->next = nc;
	}
	nc->next = NULL;
	return (nc);
}
