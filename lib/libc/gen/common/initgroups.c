#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)initgroups.c 1.1 92/07/30 SMI"; /* from UCB 4.4 83/06/17 */
#endif


/*
 * initgroups NIS
 */
#include <stdio.h>
#include <sys/param.h>
#include <grp.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <string.h>

struct group   *getgrent_withinfo();	/* secret interface */
static int 
getuidfromyellow(name, id)
	int            *id;
	char           *name;
{
	int             reason;

	char           *domain = 0;
	char           *val = 0;
	char           *xxx;
	int             vallen = 0;
	yp_get_default_domain(&domain);
	if (domain == 0)
		return (-1);
	reason = yp_match(domain, "passwd.byname", name, strlen(name), &val, &vallen);
	if (reason)
		return (reason);
	val[vallen] = 0;	/* null terminate it */
	if (val == NULL) {
		return (-1);
	}
	xxx = strtok(val, ":");	/* name */
	if (xxx == NULL) {
		free(val);
		return (-1);
	}
	xxx = strtok(NULL, ":");/* pw */
	if (xxx == NULL) {
		free(val);
		return (-1);
	}
	xxx = strtok(NULL, ":");/* uid */
	if (xxx == NULL) {
		free(val);
		return (-1);
	}
	*id = atoi(xxx);
	if (*id < 0) {
		free(val);
		return (-1);
	}
	free(val);
	return (0);
}

initgroups(uname, agroup)
	char           *uname;
	int             agroup;
{
	int             groups[NGROUPS], ngroups = 0;
	int             groupsc[NGROUPS];
	register struct group *grp;
	register struct passwd *pw;
	register int    i;
	register int    j;
	int             filter;
	int             uid1;
	int             gid;
	int             uid;
	int             numgids;
	int             doplus, gotplus;
	char            mynetname[256];	/* figure out constant */
	char           *nm;
	struct mlist {
		char           *name;
		struct mlist   *nxt;
	};
	struct mlist   *minuslist, *ls, *groupminuslist;
	doplus = 0;
	if (agroup >= 0)
		groups[ngroups++] = agroup;



	/* look in the local /etc/groups first */
	setgrent();
hardway:
	while (grp = getgrent_withinfo(doplus, &gotplus, &minuslist)) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++)
			if (!strcmp(grp->gr_mem[i], uname)) {
				if (ngroups == NGROUPS) {
					fprintf(stderr, "initgroups: %s is in too many groups\n", uname);
					goto toomany;
				}
				/* filter out duplicate group entries */
				filter = 0;
				for (j = 0; j < ngroups; j++)
					if (groups[j] == grp->gr_gid) {
						filter++;
						break;
					}
				if (!filter)
					groups[ngroups++] = grp->gr_gid;
			}
	}
	/*
	 * done with /etc/groups look in NIS -- should look for + before doing
	 * this
	 */

	if ((doplus == 0) && gotplus) {
		groupminuslist = minuslist;

		if (getuidfromyellow(uname, &uid) != 0) {
			doplus = 1;
			goto hardway;
			}

		if (uid == 0) {
			doplus = 1;
			goto hardway;
			}



		user2netname(mynetname, uid, NULL);
		if (netname2user(mynetname, &uid1, &gid, &numgids, groupsc) == TRUE) {
			for (i = 0; i < numgids; i++) {

				if (ngroups == NGROUPS) {
					fprintf(stderr, "initgroups: %s is in too many groups\n", uname);
					goto toomany;
				}
				/* filter out duplicate group entries */
				filter = 0;
				for (j = 0; j < ngroups; j++)
					if (groups[j] == groupsc[i]) {
						filter++;
						break;
					}
				if (!filter) {
					if (groupminuslist == NULL)
						groups[ngroups++] = groupsc[i];
					else {
						grp = getgrgid(groupsc[i]);
						if (grp == NULL)
							continue;

						nm = grp->gr_name;
						for (ls = groupminuslist; ls != NULL; ls = ls->nxt)
							if (strcmp(ls->name, nm) == 0) {
								grp = NULL;
								break;
							}
						if (grp == NULL)
							continue;
						else
							groups[ngroups++] = groupsc[i];
					}

				}
			}
		} else {
			doplus = 1;
			goto hardway;
		}

	}
toomany:
	endgrent();
doset:
	if (setgroups(ngroups, groups) < 0) {
		perror("setgroups");
		return (-1);
	}
	return (0);
}
