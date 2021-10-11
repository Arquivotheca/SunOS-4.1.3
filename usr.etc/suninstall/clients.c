
#ifndef lint
static char sccsid[] = "@(#)clients.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * File : clients.c
 *
 * The purpose of this little programs is just to display the clients on the
 * system.
 * 
 */


#include "install.h"

#include <stdio.h>
#include <strings.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

#define FORMAT_STRING  "%-12s%-25s%-18s%s\n"

char	*progname;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	vflag = 0;

	progname = argv[0];

	if (argc > 1 && !strcmp(argv[1],"-v")) {
		vflag++;
		argv++;
	}

	if (*++argv == '\0')
		list_clients("*",vflag);
	else
		while (*argv)
			(void) list_clients(*argv++,vflag);

	exit(EX_OK);

	/* NOTREACHED */
}


list_clients(clientname,vflag)
	char *clientname;
	int vflag;
{
	FILE *fp;
	struct stat st;
	int pid;
	char cmd[LARGE_STR];
	char line[LARGE_STR];
	char str[LARGE_STR];

	if (clientname[0] == '*') {
		pid = getpid();
		(void) sprintf(cmd,
			"/bin/ls /etc/install/client.* > /tmp/clients.%d 2>/dev/null",pid);
		(void) system(cmd);
		(void) sprintf(cmd,"/tmp/clients.%d",pid);
		if (stat(cmd,&st) == -1)
			return(1);
		if (st.st_size == 0) {
			printf("No client info found.\n");
			return(1);
		}
		if ((fp = fopen(cmd,"r")) == NULL) {
			return(1);
		}
		if (!vflag)
			printf(FORMAT_STRING, "NAME", "ARCH", "IP", "ROOT");
		while (fgets(line,sizeof(line),fp) != NULL) {
			(void) sscanf(line,"%s",str);
			(void) show_client(str,clientname,vflag);
		}
		(void) sprintf(cmd,"rm -f /tmp/clients.%d",pid);
		(void) system(cmd);
		return(0);
	} else {
		(void) sprintf(str,"%s.%s",CLIENT_STR,clientname);
		(void) show_client(str,clientname,vflag);
	}
	
	return(0);

} /* end of list_clients() */


show_client(clientfile, name, vflag)
	char	*clientfile;
	char	*name;
	int	vflag;
{
	clnt_info client;
	char	buf[MEDIUM_STR];

	if (read_clnt_info(clientfile, &client) != 1) {
		fprintf(stderr, "%s: %s: no such client.\n",
			progname,name);
		return(1);
	}

	if (!vflag) {
		printf(FORMAT_STRING,
			client.hostname, aprid_to_irid(client.arch_str, buf),
			client.ip, client.root_path);
	} else {
		printf("\nclient:\t\t%s\n",	client.hostname);
		printf("arch:\t\t%s\n",		client.arch_str);
		printf("ip:\t\t%s\n",     	client.ip);
		printf("ether:\t\t%s\n",	client.ether);
		printf("swapsize:\t%s\n",	client.swap_size);
		printf("root path:\t%s\n",	client.root_path);
		printf("swap path:\t%s\n",	client.swap_path);
		printf("home path:\t%s\n",	client.home_path);
		printf("exec path:\t%s\n",	client.exec_path);
		printf("kvm path:\t%s\n",	client.kvm_path);
		printf("share path:\t%s\n",	client.share_path);
		printf("yp type:\t%s\n",	cv_yp_to_str(&client.yp_type));
		printf("domainname:\t%s\n",	client.domainname);
	}

	return(0);

} /* end of show_info() */









