# include	"../hdr/defines.h"
# include	<sys/dir.h>

SCCSID(@(#)dofile.c 1.1 92/07/30 SMI); /* from System III 5.1 */

int	nfiles;
char	had_dir;
char	had_standinp;


do_file(p,func)
register char *p;
int (*func)();
{
	extern char *Ffile;
	char str[FILESIZE];
	char ibuf[FILESIZE];
	DIR *iop;
	register struct direct *dp;
	register char *s;
	int fd;

	if (p[0] == '-') {
		had_standinp = 1;
		while (gets(ibuf) != NULL) {
			if (sccsfile(ibuf)) {
				Ffile = ibuf;
				(*func)(ibuf);
				nfiles++;
			}
		}
	}
	else if (exists(p) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
		had_dir = 1;
		Ffile = p;
		if((iop = opendir(p)) == NULL)
			return;
		while ((dp = readdir(iop)) != NULL) {
			if (dp->d_ino == 0)
				continue;
			if (strcmp(dp->d_name, ".") == 0)
				continue;
			if (strcmp(dp->d_name, "..") == 0)
				continue;
			sprintf(str,"%s/%s",p,dp->d_name);
			if(sccsfile(str)) {
				Ffile = str;
				(*func)(str);
				nfiles++;
			}
		}
		closedir(iop);
	}
	else {
		Ffile = p;
		(*func)(p);
		nfiles++;
	}
}
