#ifndef lint
static  char sccsid[] = "@(#)ypserv_dbmtrak.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#define HAVE_DO_NEXTKEY 1
#include <ndbm.h>
#include <stdio.h>
#include <rpc/rpc.h>		/* for bool */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_DO_NEXTKEY
datum           dbm_do_nextkey();
#endif
extern int      silent;
/*
 * Manages a collection of dbm files in an LRU discipline.
 */
#define MAXDBM	10
typedef struct {
	DBM            *db;
	char           *name;
	datum           lastkey;
	int             access;
}               trakdbm;


static trakdbm *gotdb;
static trakdbm *dbms[MAXDBM];
static int      ndbms;
static int      maxdbm = MAXDBM;
int 
dbmtrak_getaccess()
{
	if (gotdb)
		return (gotdb->access);
	else
		return (0);	/* Unknown */
}

dbmtrak_setaccess(x)
	int             x;
{
	if (gotdb)
		gotdb->access = x;
}

trakdbm        *newdbm();
closealldbm()
{
	int             i;
	for (i = 0; i < ndbms; i++) {

		if (dbms[i]) {
			if (silent == FALSE)
				printf("ALL close %d %s\n", i, dbms[i]->name);
			if (dbms[i]->db)
				dbm_close(dbms[i]->db);	/* close LRU */

			free(dbms[i]->name);	/* forget name */
			free(dbms[i]);
			dbms[i] = 0;
		}
	}
	ndbms = 0;
	gotdb = 0;
}
trakdbm        *
getdbm(name)
{
	int             i;
	int             j;
	trakdbm        *hold;
	for (i = 0; i < ndbms; i++) {
		if (dbms[i]) {
			if (dbms[i]->name) {
				if (strcmp(dbms[i]->name, name) == 0) {
					/* reorder list in LRU */
					/*
					 * if (silent==FALSE){printf("gotcha
					 * %s %d\n",name,i);fflush(stdout);}
					 */
					if (i != 0) {
						/*
						 * printf("moving %d to
						 * 0\n",i);
						 */
						hold = dbms[i];
						for (j = i - 1; j >= 0; j--) {
							dbms[j + 1] = dbms[j];
						}
						dbms[0] = hold;
					}
					return (dbms[0]);
				}
			}
		}
	}
	return (newdbm(name));

}
trakdbm        *
newdbm(name)
{
	int             j;
	DBM            *data;
	if (ndbms == maxdbm) {
		if (dbms[ndbms - 1]) {
			if (silent == FALSE) {
				printf("LRU close %d %s\n", ndbms - 1, dbms[ndbms - 1]->name);
				fflush(stdout);
			}
			if (dbms[ndbms - 1] == gotdb) {
				gotdb = 0;
				printf("gotdb closed!\n");
				fflush(stdout);
			}
			/* wipe gotdb */
			if (dbms[ndbms -1] ->db)
				dbm_close(dbms[ndbms - 1]->db);	/* close LRU */

			free(dbms[ndbms - 1]->name);	/* forget name */
			free(dbms[ndbms - 1]);
		}
		for (j = ndbms - 2; j >= 0; j--) {
			dbms[j + 1] = dbms[j];
		}
	} else {
		for (j = ndbms - 1; j >= 0; j--) {
			dbms[j + 1] = dbms[j];
		}
		ndbms++;
	}
	/* Now can  allocate another dbm record */
	dbms[0] = 0;		/* for now */
	dbms[0] = (trakdbm *) malloc(sizeof(trakdbm));
	if (dbms[0] == 0) {
		printf("malloc failed\n");
		fflush(stdout);
		abort();
	}
	dbms[0]->name = (char *) malloc(1 + strlen(name));
	strcpy(dbms[0]->name, name);	/* jam the name */
	dbms[0]->lastkey.dptr = 0;
	dbms[0]->lastkey.dsize = 0;
	dbms[0]->access = 0;	/* very important to set this */
	if (dbms[0]->name == 0) {
		printf("malloc failed name\n");
		fflush(stdout);
		free(dbms[0]);
		dbms[0] = 0;
		abort();
	}
	dbms[0]->db = dbm_open(name, 0, 0);	/* Read only */
	if (dbms[0]->db == 0) {
		free(dbms[0]->name);
		free(dbms[0]);
		dbms[0] = 0;
		if (silent == FALSE) {
			printf("dbm open failed %s\n", name);
			fflush(stdout);
		}
		return (0);
	}
	if (silent == FALSE) {
		printf("dbm open ok %s\n", name);
		fflush(stdout);
	}
	return (dbms[0]);
}

/* Emulation Library */
dbmclose()
{
	gotdb = 0;
	return (0);
}
dbminit(file)
	char           *file;
{
	gotdb = getdbm(file);
	if (gotdb == 0)
		return (-1);
	else
		return (0);
}
datum           trak_fetch();
datum           trak_nextkey();
datum           trak_firstkey();
datum 
fetch(key)
	datum           key;
{
	datum           fetchresult;
	fetchresult = trak_fetch(key);
	if (fetchresult.dptr)
		return (fetchresult);
	if (check_db() == 1) {
		printf("try fetch again on  %s\n", gotdb->name);
		fflush(stdout);
		fetchresult = trak_fetch(key);
	}
	return (fetchresult);
}

datum 
nextkey(key)
	datum           key;
{
	datum           fetchresult;
	fetchresult = trak_nextkey(key);
	if (fetchresult.dptr)
		return (fetchresult);
	if (check_db() == 1) {
		printf("try nextkey again on  %s\n", gotdb->name);
		fflush(stdout);
		fetchresult = trak_nextkey(key);
	}
	return (fetchresult);
}

datum 
firstkey()
{
	datum           fetchresult;
	fetchresult = trak_firstkey();
	if (fetchresult.dptr)
		return (fetchresult);
	if (check_db() == 1) {
		printf("try firstkey again on  %s\n", gotdb->name);
		fflush(stdout);
		fetchresult = trak_firstkey();
	}
	return (fetchresult);
}
static datum 
trak_fetch(key)
	datum           key;
{
	datum           fetchresult;
	char            msg[128];
	if (gotdb) {
		fetchresult = dbm_fetch(gotdb->db, key);
		if (silent == FALSE) {
			if (fetchresult.dptr == 0) {
				if (key.dsize < 127) {
					strncpy(msg, key.dptr, key.dsize);
					msg[key.dsize] = 0;
				} else {
					strncpy(msg, key.dptr, 127);
					msg[key.dsize] = 0;
				}
				printf("fetch key \'%s\' not in map %s\n", msg, gotdb->name);
				fflush(stdout);

			}
		}
		return (fetchresult);
	} else {
		if (silent == FALSE) {
			printf("fetch botched %*s gotdb=0\n", key.dsize, key.dptr);
			fflush(stdout);
		}
		fetchresult.dptr = 0;
		fetchresult.dsize = 0;
		return (fetchresult);
	}

}

static datum 
trak_firstkey()
{
	datum           fetchresult;

	if (gotdb) {
		fetchresult = dbm_firstkey(gotdb->db);
		gotdb->lastkey.dsize = fetchresult.dsize;
		gotdb->lastkey.dptr = fetchresult.dptr;
		return (fetchresult);
	} else {
		if (silent == FALSE) {
			printf("firstkey botched gotdb closed\n");
			fflush(stdout);
		}
		fetchresult.dptr = 0;
		fetchresult.dsize = 0;
		return (fetchresult);
	}

}

static datum 
trak_nextkey(key)
/* this sucks */
	datum           key;
{
	long            holdblk;
	datum           fetchresult;
	if (gotdb) {
		if ((gotdb->lastkey.dptr) && (key.dptr))
			if (gotdb->lastkey.dsize == key.dsize)
				if (bcmp(gotdb->lastkey.dptr, key.dptr, key.dsize) == 0) {
					/*
					 * printf("nextkey key is lastkey
					 * given!\n");
					 */
					fetchresult = dbm_nextkey(gotdb->db);
					gotdb->lastkey.dsize = fetchresult.dsize;
					gotdb->lastkey.dptr = fetchresult.dptr;
					return (fetchresult);
				}
#ifdef HAVE_DO_NEXTKEY
		fetchresult = dbm_do_nextkey(gotdb->db, key);
		gotdb->lastkey.dsize = fetchresult.dsize;
		gotdb->lastkey.dptr = fetchresult.dptr;
		return (fetchresult);
#else
		/* AAAAArgh this sucks */
		gotdb->db->dbm_keyptr = 0L;
		holdblk = (gotdb->db->dbm_blkptr = dbm_forder(gotdb->db, key));
		/* hash the block to find the lastkey	 */
		while (gotdb->db->dbm_blkptr == holdblk) {
			fetchresult = dbm_nextkey(gotdb->db);

			if ((key.dptr) && (fetchresult.dptr))
				if (fetchresult.dsize == key.dsize)
					if (bcmp(fetchresult.dptr, key.dptr, key.dsize) == 0) {
						/* gotcha */
						printf("Found key %*s the hard way! %s\n", key.dsize, key.dptr, gotdb->name);
						fflush(stdout);
						fetchresult = dbm_nextkey(gotdb->db);
						gotdb->lastkey.dsize = fetchresult.dsize;
						gotdb->lastkey.dptr = fetchresult.dptr;
						return (fetchresult);
					}
		}
		printf("Could not find lastkey %*s %s\n", key.dsize, key.dptr,
		       gotdb->name);
		fflush(stdout);
		fetchresult.dptr = 0;
		fetchresult.dsize = 0;
		gotdb->lastkey.dsize = 0;
		gotdb->lastkey.dptr = 0;
		return (fetchresult);
#endif
	} else {
		if (silent == FALSE) {
			printf("nextkey botched gotdb closed %*s\n", key.dsize, key.dptr);
			fflush(stdout);
		}
		fetchresult.dptr = 0;
		fetchresult.dsize = 0;
		return (fetchresult);
	}

}

/* 1 ->reopen */
/* 0 -> ok */
/*-1 ->fatal*/
static 
check_db()
{
	int             ok;
	if (gotdb == 0) {
		printf("gotdb null\n");
		fflush(stdout);
		return (-1);
	}
	ok = 1;			/* Its ok for now */
	if (gotdb->db == 0) {
		printf("gotdb->db null %s\n", gotdb->name);
		fflush(stdout);
		ok = 0;
	} else {

		if (dbm_error(gotdb->db)) {
			ok = 0;
			printf("dbm_error %s\n", gotdb->name);
			fflush(stdout);
		}
		if (check_fd(gotdb->db->dbm_dirf) < 0) {
			ok = 0;
			printf("dirf bad %s\n", gotdb->name);
			fflush(stdout);
		}
		if (check_fd(gotdb->db->dbm_pagf) < 0) {
			ok = 0;
			printf("pagf bad %s\n", gotdb->name);
			fflush(stdout);
		}
	}
	if (ok == 0) {
		if (gotdb->db)
			dbm_close(gotdb->db);
		gotdb->db = dbm_open(gotdb->name);

		if (gotdb->db == 0) {
			printf("dbm REopen failed %s\n", gotdb->name);
			fflush(stdout);
			closealldbm();	/* Try something desperate */
			return (-1);
		} else
			return (1);	/* Reopen OK! */

	} else
		return (0);
}

static 
check_fd(fd)
	int             fd;
{
	int             rv;
	struct stat     st;
	return (fstat(fd, &st));
}
