/*	@(#)main.c 1.1 92/07/30 SMI	*/
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "nres.h"
struct hostent *nres_getanswer();
extern int      h_errno;
struct rpc_as   input;
int             askuser();
asktime()
{
	printf("timeout unexpected!\n");
}
main()
{
	input.as_fd = 0;
	input.as_timeout_flag = 0;
	input.as_recv = askuser;
	input.as_timeout = asktime;
	rpc_as_register(&input);
	printf("host> ");
	fflush(stdout);
	svc_run_as();
}
void 
my_done(temp, theans,info,code)
	struct nres    *temp;
	struct hostent *theans;
	int code;
	int info; /* */
{
	h_errno= code;
	printf("***done*** %d\n",info);
	if (theans)
		printf("%s:got answer.\n", temp->search_name);
	else
		herror(temp->search_name);

	printf("host> ");
	fflush(stdout);
}
struct nres    *temp;
askuser()
{				/* simulates a call */
	struct nres    *temp;
	char            junk[512];
	long            addr;
	int             n;
	int             st;
	static int	qnum;
	temp = NULL;
	qnum++;
	if (scanf("%s", junk) <= 0)
		exit(-1);
	if (strcmp(junk, "vc") == 0)
		_res.options |= RES_USEVC;
	else if (strcmp(junk, "~vc") == 0)
		_res.options &= (~RES_USEVC);
	else if (strcmp(junk, "debug") == 0)
		_res.options |= (RES_DEBUG);
	else if (strcmp(junk, "~debug") == 0)
		_res.options &= (~RES_DEBUG);
	/* else if (strcmp(junk,"go")==0) return(0); */
	else if (strncmp(junk, "addr=", 5) == 0) {
		addr = inet_addr(&junk[5]);
		temp=nres_gethostbyaddr(&addr, 4, AF_INET, &my_done, qnum/*info*/);
		if (temp== NULL) herror(junk);

	} else {
		temp=nres_gethostbyname(junk, &my_done, qnum/*info*/);
		if (temp== NULL) herror(junk);
	}

	printf("host> ");
	fflush(stdout);
}
