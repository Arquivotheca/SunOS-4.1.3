#ifndef lint
static	char sccsid[] = "@(#)ypserv_map.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

#include "ypsym.h"
#include "ypdefs.h"
USE_YPDBPATH
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED
USE_YP_SECURE
USE_DBM

#include <ctype.h>
char current_map[sizeof (ypdbpath) + YPMAXDOMAIN + YPMAXMAP + 3];
static enum { UNKNOWN, SECURE, PUBLIC } current_map_access;
static char map_owner[MAX_MASTER_NAME + 1];

/*
 * This performs an existence check on the dbm data base files <name>.pag and
 * <name>.dir.  pname is a ptr to the filename.  This should be an absolute
 * path.
 * Returns TRUE if the map exists and is accessable; else FALSE.
 *
 * Note:  The file name should be a "base" form, without a file "extension" of
 * .dir or .pag appended.  See ypmkfilename for a function which will generate
 * the name correctly.  Errors in the stat call will be reported at this level,
 * however, the non-existence of a file is not considered an error, and so will
 * not be reported.
 */
bool
ypcheck_map_existence(pname)
	char *pname;
{
	char dbfile[MAXNAMLEN + 1];
	struct stat filestat;
	int len;

	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + sizeof (dbm_pag)) > (MAXNAMLEN + 1) ) {
		return(FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, dbm_dir);

	if (stat(dbfile, &filestat) != -1) {
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, dbm_pag);

		if (stat(dbfile, &filestat) != -1) {
			return(TRUE);
		} else {

			if (errno != ENOENT) {
				(void) fprintf(stderr,
				    "ypserv:  Stat error on map file %s.\n",
				    dbfile);
			}

			return(FALSE);
		}

	} else {

		if (errno != ENOENT) {
			(void) fprintf(stderr,
			    "ypserv:  Stat error on map file %s.\n",
			    dbfile);
		}


		return(FALSE);
	}

}

/*
 * The retrieves the order number of a named map from the order number datum
 * in the map data base.  
 */
bool
ypget_map_order(map, domain, order)
	char *map;
	char *domain;
	unsigned *order;
{
	datum key;
	datum val;
	char toconvert[MAX_ASCII_ORDER_NUMBER_LENGTH + 1];
	unsigned error;

	if (ypset_current_map(map, domain, &error) ) {
		key.dptr = yp_last_modified;
		key.dsize = yp_last_modified_sz;
		val = fetch(key);

		if (val.dptr != (char *) NULL) {

			if (val.dsize > MAX_ASCII_ORDER_NUMBER_LENGTH) {
				return(FALSE);
			}

			/*
			 * This is getting recopied here because val.dptr
			 * points to static memory owned by the dbm package,
			 * and we have no idea whether numeric characters
			 * follow the order number characters, nor whether
			 * the mess is null-terminated at all.
			 */

			bcopy(val.dptr, toconvert, val.dsize);
			toconvert[val.dsize] = '\0';
			*order = (unsigned long) atol(toconvert);
			return(TRUE);
		} else {
		    return(FALSE);
		}
		    
	} else {
		return(FALSE);
	}
}

/*
 * The retrieves the master server name of a named map from the master datum
 * in the map data base.  
 */
bool
ypget_map_master(map, domain, owner)
	char *map;
	char *domain;
	char **owner;
{
	datum key;
	datum val;
	unsigned error;

	if (ypset_current_map(map, domain, &error) ) {
		key.dptr = yp_master_name;
		key.dsize = yp_master_name_sz;
		val = fetch(key);

		if (val.dptr != (char *) NULL) {

			if (val.dsize > MAX_MASTER_NAME) {
				return(FALSE);
			}

			/*
			 * This is getting recopied here because val.dptr
			 * points to static memory owned by the dbm package.
			 */
			bcopy(val.dptr, map_owner, val.dsize);
			map_owner[val.dsize] = '\0';
			*owner = map_owner;
			return(TRUE);
		} else {
		    return(FALSE);
		}
		    
	} else {
		return(FALSE);
	}
}

/*
 * This makes a map into the current map, and calls dbminit on that map so
 * that any successive dbm operation is performed upon that map.  Returns an
 * YP_xxxx error code in error if FALSE.  
 */
bool
ypset_current_map(map, domain, error)
	char *map;
	char *domain;
	unsigned *error;
{
	char mapname[sizeof (current_map)];
	int lenm, lend;

	if (!map || ((lenm = strlen(map)) == 0) || (lenm > YPMAXMAP) ||
	    !domain || ((lend = strlen(domain)) == 0) || (lend > YPMAXDOMAIN)) {
		*error = YP_BADARGS;
		return(FALSE);
	}

	ypmkfilename(domain, map, mapname);

	if (strcmp(mapname, current_map) == 0) {
		return(TRUE);
	}

	ypclr_current_map();
	current_map_access = UNKNOWN;

	if (dbminit(mapname) >= 0) {
		(void) strcpy(current_map, mapname);
		return(TRUE);
	}

	ypclr_current_map();
	
	if (ypcheck_domain(domain)) {

		if (ypcheck_map_existence(mapname)) {
			*error = YP_BADDB;
		} else {
			*error = YP_NOMAP;
		}
		
	} else {
		*error = YP_NODOM;
	}

	return(FALSE);
}

/*
 * Checks to see if caller has permission to query the current map (as set
 * by ypset_current_map()).  Returns TRUE if access is granted and FALSE
 * otherwise.  If FALSE then sets *error to YP_xxxx.
 */
bool
yp_map_access(rqstp, transp, error)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	unsigned *error;
{
	struct sockaddr_in *caller;
	if (current_map_access == UNKNOWN) {
		current_map_access= dbmtrak_getaccess();
	}

	if (current_map_access == PUBLIC) {
		return (TRUE);
	}
	caller = svc_getcaller(transp);
	if ((caller->sin_family == AF_INET) &&
	    (ntohs(caller->sin_port)) < IPPORT_RESERVED) {
		return (TRUE);
	}
	if (current_map_access == UNKNOWN) {
		datum key;
		datum val;

		key.dptr = yp_secure;
		key.dsize = yp_secure_sz;
		val = fetch(key);
		if (val.dptr == (char *)NULL) {
			current_map_access = PUBLIC;
			dbmtrak_setaccess(current_map_access);
			return (TRUE);
		}
		current_map_access = SECURE;
		dbmtrak_setaccess(current_map_access);
	}
	/* current_map_access==SECURE and non-priviledged caller */
	*error = YP_NOMAP;
	return (FALSE);
}

/*
 * This checks to see if there is a current map, and, if there is, does a
 * dbmclose on it and sets the current map name to null.  
 */
void
ypclr_current_map()

{

	if (current_map[0] != '\0') {
		(void) dbmclose(current_map);
		current_map[0] = '\0';
	}

}
