#ifndef lint
static char sccsid[] = "@(#)sm_statd.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1989 by Sun Microsystems, Inc.
	 */

/*
 * sm_statd.c consists of routines used for the intermediate
 * statd implementation(3.2 rpc.statd);
 * it creates an entry in "current" directory for each site that it monitors;
 * after crash and recovery, it moves all entries in "current"
 * to "backup" directory, and notifies the corresponding statd of its recovery.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <dirent.h>
#include <rpc/rpc.h>
#include <rpcsvc/sm_inter.h>
#include <errno.h>
#include "sm_statd.h"

#define MAXPGSIZE 8192
#define SM_INIT_ALARM 15
extern int debug;
extern int errno;
extern char STATE[MAXHOSTNAMELEN], CURRENT[MAXHOSTNAMELEN], BACKUP[MAXHOSTNAMELEN];
extern char *strcpy(), *strcat();
int LOCAL_STATE;

struct name_entry {
	char *name;
	int count;
	struct name_entry *prev;
	struct name_entry *nxt;
};
typedef struct name_entry name_entry;

name_entry *find_name();
name_entry *insert_name();
name_entry *record_q;
name_entry *recovery_q;

char hostname[MAXNAMLEN];

sm_notify(ntfp)
	stat_chge *ntfp;
{
	if (debug)
		printf("sm_notify: %s state =%d\n", ntfp->name, ntfp->state);
	send_notice(ntfp->name, ntfp->state);
}

/*
 * called when statd first comes up; it searches /etc/sm to gather
 * all entries to notify its own failure
 */
statd_init()
{
	int cc, fd;
	char buf[MAXPGSIZE];
	long base;
	struct dirent *dirp;
	DIR 	*dp;
	char from[MAXNAMLEN], to[MAXNAMLEN], path[MAXNAMLEN];
	FILE *fp, *fopen();

	if (debug)
		printf("enter statd_init\n");
	(void) gethostname(hostname, MAXNAMLEN);
	if ((fp = fopen(STATE, "a+")) == NULL) {
		fprintf(stderr, "rpc.statd: fopen(stat file) error\n");
		exit(1);
	}
	if (fseek(fp, 0, 0) == -1) {
		perror("rpc.statd: fseek failed");
		fprintf(stderr, "\n");
		exit(1);
	}
	if ((cc = fscanf(fp, "%d", &LOCAL_STATE)) == EOF) {
		if (debug >= 2)
		printf("empty file\n");
		LOCAL_STATE = 0;
	}
	LOCAL_STATE = ((LOCAL_STATE%2) == 0) ? LOCAL_STATE+1 : LOCAL_STATE+2;
	if (fseek(fp, 0, 0) == -1) {
		perror("rpc.statd: fseek failed");
		fprintf(stderr, "\n");
		exit(1);
	}
	fprintf(fp, "%d", LOCAL_STATE);
	(void) fflush(fp);
	if (fsync(fileno(fp)) == -1) {
		perror("rpc.statd: fsync failed");
		fprintf(stderr, "\n");
		exit(1);
	}
	(void) fclose(fp);
	if (debug)
		printf("local state = %d\n", LOCAL_STATE);

	if ((mkdir(CURRENT, 00777)) == -1) {
		if (errno != EEXIST) {
			perror("rpc.statd: mkdir current");
			fprintf(stderr, "\n");
			exit(1);
		}
	}
	if ((mkdir(BACKUP, 00777)) == -1) {
		if (errno != EEXIST) {
			perror("rpc.statd: mkdir backup");
			fprintf(stderr, "\n");
			exit(1);
		}
	}

	/* get all entries in CURRENT into BACKUP */
	if ((dp = opendir(CURRENT)) == NULL) {
		perror("rpc.statd: open current directory");
		fprintf(stderr, "\n");
		exit(1);
	}
	for (dirp = readdir(dp); dirp != NULL; dirp = readdir(dp)) {
/*
		printf("d_name = %s\n", dirp->d_name);
*/
		if (strcmp(dirp->d_name, ".") != 0  &&
		strcmp(dirp->d_name, "..") != 0) {
		/* rename all entries from CURRENT to BACKUP */
			(void) strcpy(from, CURRENT);
			(void) strcpy(to, BACKUP);
			(void) strcat(from, "/");
			(void) strcat(to, "/");
			(void) strcat(from, dirp->d_name);
			(void) strcat(to, dirp->d_name);
			if (rename(from, to) == -1) {
				perror("rpc.statd: rename");
				fprintf(stderr, "\n");
				exit(1);
			}
			if (debug >= 2)
				printf("rename: %s to %s\n", from, to);
		}
	}
	closedir(dp);

	/* get all entries in BACKUP into recovery_q */
	if ((dp = opendir(BACKUP)) == NULL) {
		perror("rpc.statd: open backup directory");
		fprintf(stderr, "\n");
		exit(1);
	}
	for (dirp = readdir(dp); dirp != NULL; dirp = readdir(dp)) {
		if (strcmp(dirp->d_name, ".") != 0  &&
		strcmp(dirp->d_name, "..") != 0) {
		/* get all entries from BACKUP to recovery_q */
			if (statd_call_statd(dirp->d_name) != 0) {
				(void) insert_name(&recovery_q, dirp->d_name);
			}
			else { /* remove from BACKUP directory */
				(void) strcpy(path, BACKUP);
				(void) strcat(path, "/");
				(void) strcat(path, dirp->d_name);
			if (debug >= 2)
				printf("remove monitor entry %s\n", path);
			if (unlink(path) == -1) {
				fprintf(stderr, "rpc.statd: ");
				perror(path);
				fprintf(stderr, "\n");
				exit(1);
			}
			}
		}
	}
	closedir(dp);

	/* notify statd */
	if (recovery_q != NULL)
		(void) alarm(SM_INIT_ALARM);
}

xdr_notify(xdrs, ntfp)
	XDR *xdrs;
	stat_chge *ntfp;
{
	if (!xdr_string(xdrs, &ntfp->name, MAXNAMLEN+1)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &ntfp->state)) {
		return (FALSE);
	}
	return (TRUE);
}

statd_call_statd(name)
	char *name;
{
	stat_chge ntf;
	int err;

	ntf.name =hostname;
	ntf.state = LOCAL_STATE;
	if (debug)
		printf("statd_call_statd at %s\n", name);
	if ((err = call_tcp(name, SM_PROG, SM_VERS,
	SM_NOTIFY, xdr_notify, &ntf, xdr_void, NULL, 0))
	== (int) RPC_TIMEDOUT || err == (int) RPC_SUCCESS) {
		return (0);
	}
	else {
		fprintf(stderr, "rpc.statd: cannot talk to statd at %s\n", name);
		return (-1);
	}
}


sm_try()
{
	name_entry *nl, *next;

	if (debug >= 2)
		printf("enter sm_try: recovery_q = %s\n", recovery_q->name);
	next = recovery_q;
	while ((nl = next) != NULL) {
		next = next->nxt;
		if (statd_call_statd(nl->name) == 0) {
			/* remove entry from recovery_q */
			delete_name(&recovery_q, nl->name);
		}
	}
	if (recovery_q != NULL)
		(void) alarm(SM_INIT_ALARM);
}

char *
xmalloc(len)
	unsigned len;
{
	char *new;

	if ((new = malloc(len)) == 0) {
		perror("rpc.statd: malloc");
		fprintf(stderr, "\n");
		return (NULL);
	}
	else {
		bzero(new, len);
		return (new);
	}
}

/*
 * the following two routines are very similar to
 * insert_mon and delete_mon in sm_proc.c, except the structture
 * is different
 */
name_entry *
insert_name(namepp, name)
	name_entry **namepp;
	char *name;
{
	name_entry *new;

	new = (name_entry *) xmalloc (sizeof (struct name_entry));
	new->name = xmalloc(strlen(name) + 1);
	(void) strcpy(new->name, name);
	new->nxt = *namepp;
	if (new->nxt != NULL)
		new->nxt->prev = new;
	*namepp = new;
	return (new);
}

delete_name(namepp, name)
	name_entry **namepp;
	char *name;
{
	name_entry *nl;

	nl = *namepp;
	while (nl != NULL) {
		if (strcmp(nl->name, name) == 0) { /* found */
			if (nl->prev != NULL)
				nl->prev->nxt = nl->nxt;
			else
				*namepp = nl->nxt;
			if (nl->nxt != NULL)
				nl->nxt->prev = nl->prev;
			free(nl->name);
			free(nl);
			return;
		}
		nl = nl->nxt;
	}
	return;
}

name_entry *
find_name(namep, name)
	name_entry *namep;
	char *name;
{
	name_entry *nl;

	nl = namep;
	while (nl != NULL) {
		if (strcmp(nl->name, name) == 0) {
			return (nl);
		}
		nl = nl->nxt;
	}
	return (NULL);
}

record_name(name, op)
	char *name;
	int op;
{
	name_entry *nl;
	int fd;
	char path[MAXNAMLEN];

	if (op == 1) { /* insert */
		if ((nl = find_name(record_q, name)) == NULL) {
			nl = insert_name(&record_q, name);
			/* make an entry in current directory */
			(void) strcpy(path, CURRENT);
			(void) strcat(path, "/");
			(void) strcat(path, name);
			if (debug >= 2)
				printf("create monitor entry %s\n", path);
			if ((fd = open(path, O_CREAT, 00200)) == -1){
				fprintf(stderr, "rpc.statd: open of \n");
				perror(path);
				fprintf(stderr, "\n");
				if (errno != EACCES)
					exit(1);
			}
			else {
				if (debug >= 2)
					printf("%s is created\n", path);
				if (close(fd)) {
					perror("rpc.statd: close");
					fprintf(stderr, "\n");
					exit(1);
				}
			}
		}
		nl->count++;
	}
	else { /* delete */
		if ((nl = find_name(record_q, name)) == NULL) {
			return;
		}
		nl->count--;
		if (nl->count == 0) {
			delete_name(&record_q, name);
		/* remove this entry from current directory */
			(void) strcpy(path, CURRENT);
			(void) strcat(path, "/");
			(void) strcat(path, name);
			if (debug >= 2)
				printf("remove monitor entry %s\n", path);
			if (unlink(path) == -1) {
				fprintf(stderr, "rpc.statd: unlink of ");
				perror(path);
				fprintf(stderr, "\n");
				exit(1);
			}

		}
	}
}

sm_crash()
{
	name_entry *nl, *next;

	if (record_q == NULL)
		return;
	next = record_q;	/* clean up record queue */
	while ((nl = next) != NULL) {
		next = next->nxt;
		delete_name(&record_q, nl->name);
	}

	if (recovery_q != NULL) { /* clean up all onging recovery act */
		if (debug)
			printf("sm_crash clean up\n");
		(void) alarm(0);
		next = recovery_q;
		while ( (nl = next) != NULL) {
			next = next ->nxt;
			delete_name(&recovery_q, nl->name);
		}
	}
	statd_init();
}
