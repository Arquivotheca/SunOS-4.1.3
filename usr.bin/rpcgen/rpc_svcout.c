#ifndef lint
static char sccsid[] = "@(#)rpc_svcout.c 1.1 92/07/30 (C) 1987 SMI";
#endif

/*
 * rpc_svcout.c, Server-skeleton outputter for the RPC protocol compiler
 * Copyright (C) 1987, Sun Microsytsems, Inc. 
 */
#include <stdio.h>
#include <strings.h>
#include "rpc_parse.h"
#include "rpc_util.h"

static char RQSTP[] = "rqstp";
static char TRANSP[] = "transp";
static char ARG[] = "argument";
static char RESULT[] = "result";
static char ROUTINE[] = "local";

char _errbuf[256];	/* For all messages */

/*
 * write most of the service, that is, everything but the registrations. 
 */
void
write_most(infile)
	char *infile;		/* our name */
{
	list *l;
	definition *def;
	version_list *vp;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			for (vp = def->def.pr.versions; vp != NULL;
					vp = vp->next) {
				f_print(fout, "\nstatic void ");
				pvname(def->def_name, vp->vers_num);
				f_print(fout, "();");
			}
		}
	}
	if (timerflag)
		f_print(fout, "\nstatic void closedown();");
	f_print(fout, "\n");
	if (inetdflag) {
		f_print(fout, "\nstatic int _rpcpmstart;");
		f_print(fout, "\t\t/* Started by a port monitor ? */\n");
		f_print(fout, "static int _rpcfdtype;");
		f_print(fout, "\t\t/* Whether Stream or Datagram ? */\n");
		if (timerflag) {
			f_print(fout, "static int _rpcsvcdirty;");
			f_print(fout, "\t/* Still serving ? */\n");
		}
	}
	f_print(fout, "\nmain()\n");
	f_print(fout, "{\n");
	f_print(fout, "\tregister SVCXPRT *%s;\n", TRANSP);
	if (inetdflag) {
		write_inetmost(infile); /* Includes call to write_rpc_svc_fg() */
	} else {
		f_print(fout, "\n");
		print_pmapunset("\t");
	}

	if (logflag && !inetdflag) {
		open_log_file(infile, "\t");
	}
}

/*
 * write a registration for the given transport for Inetd
 */
void
write_register(transp)
	char *transp;
{
	list *l;
	definition *def;
	version_list *vp;
	char *sp;
	int isudp;
	char tmpbuf[32];

	if (inetdflag)
		sp = "\t";
	else
		sp = "";
	if (streq(transp, "udp"))
		isudp = 1;
	else
		isudp = 0;
	f_print(fout, "\n");
	if (inetdflag) {
		f_print(fout, "\tif ((_rpcfdtype == 0) || (_rpcfdtype == %s)) {\n",
				isudp ? "SOCK_DGRAM" : "SOCK_STREAM");
	}
	f_print(fout, "%s\t%s = svc%s_create(%s",
		sp, TRANSP, transp, inetdflag? "sock": "RPC_ANYSOCK");
	if (!isudp)
		f_print(fout, ", 0, 0");
	f_print(fout, ");\n");
	f_print(fout, "%s\tif (%s == NULL) {\n", sp, TRANSP);
	(void) sprintf(_errbuf, "cannot create %s service.", transp);
	(void) sprintf(tmpbuf, "%s\t\t", sp);
	print_err_message(tmpbuf);
	f_print(fout, "%s\t\texit(1);\n", sp);
	f_print(fout, "%s\t}\n", sp);

	if (inetdflag) {
		f_print(fout, "%s\tif (!_rpcpmstart)\n\t", sp);
		f_print(fout, "%s\tproto = IPPROTO_%s;\n",
				sp, isudp ? "UDP": "TCP");
	}
	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			f_print(fout, "%s\tif (!svc_register(%s, %s, %s, ",
				sp, TRANSP, def->def_name, vp->vers_name);
			pvname(def->def_name, vp->vers_num);
			if (inetdflag)
				f_print(fout, ", proto)) {\n");
			else 
				f_print(fout, ", IPPROTO_%s)) {\n",
					isudp ? "UDP": "TCP");
			(void) sprintf(_errbuf, "unable to register (%s, %s, %s).",
					def->def_name, vp->vers_name, transp);
			print_err_message(tmpbuf);
			f_print(fout, "%s\t\texit(1);\n", sp);
			f_print(fout, "%s\t}\n", sp);
		}
	}
	if (inetdflag)
		f_print(fout, "\t}\n");
}

/*
 * write the rest of the service 
 */
void
write_rest()
{
	f_print(fout, "\n");
	if (inetdflag) {
		f_print(fout, "\tif (%s == (SVCXPRT *)NULL) {\n", TRANSP);
		(void) sprintf(_errbuf, "could not create a handle");
		print_err_message("\t\t");
		f_print(fout, "\t\texit(1);\n");
		f_print(fout, "\t}\n");
		if (timerflag) {
			f_print(fout, "\tif (_rpcpmstart) {\n");
			f_print(fout, "\t\t(void) signal(SIGALRM, closedown);\n");
			f_print(fout, "\t\t(void) alarm(_RPCSVC_CLOSEDOWN);\n");
			f_print(fout, "\t}\n");
		}
	}
	f_print(fout, "\tsvc_run();\n");
	(void) sprintf(_errbuf, "svc_run returned");
	print_err_message("\t");
	f_print(fout, "\texit(1);\n");
	f_print(fout, "\t/* NOTREACHED */\n");
	f_print(fout, "}\n");
}

void
write_programs(storage)
	char *storage;
{
	list *l;
	definition *def;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_program(def, storage);
		}
	}
}

static
write_program(def, storage)
	definition *def;
	char *storage;
{
	version_list *vp;
	proc_list *proc;
	int filled;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		f_print(fout, "\n");
		if (storage != NULL) {
			f_print(fout, "%s ", storage);
		}
		f_print(fout, "void\n");
		pvname(def->def_name, vp->vers_num);
		f_print(fout, "(%s, %s)\n", RQSTP, TRANSP);
		f_print(fout, "	struct svc_req *%s;\n", RQSTP);
		f_print(fout, "	register SVCXPRT *%s;\n", TRANSP);
		f_print(fout, "{\n");

		filled = 0;
		f_print(fout, "\tunion {\n");
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			if (streq(proc->arg_type, "void")) {
				continue;
			}
			filled = 1;
			f_print(fout, "\t\t");
			ptype(proc->arg_prefix, proc->arg_type, 0);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "_arg;\n");
		}
		if (!filled) {
			f_print(fout, "\t\tint fill;\n");
		}
		f_print(fout, "\t} %s;\n", ARG);
		f_print(fout, "\tchar *%s;\n", RESULT);
		f_print(fout, "\tbool_t (*xdr_%s)(), (*xdr_%s)();\n", ARG, RESULT);
		f_print(fout, "\tchar *(*%s)();\n", ROUTINE);
		f_print(fout, "\n");

		if (timerflag)
			f_print(fout, "\t_rpcsvcdirty = 1;\n");
		f_print(fout, "\tswitch (%s->rq_proc) {\n", RQSTP);
		if (!nullproc(vp->procs)) {
			f_print(fout, "\tcase NULLPROC:\n");
			f_print(fout,
			"\t\t(void) svc_sendreply(%s, xdr_void, (char *)NULL);\n",
					TRANSP);
			print_return("\t\t");
			f_print(fout, "\n");
		}
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			f_print(fout, "\tcase %s:\n", proc->proc_name);
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", ARG, 
				stringfix(proc->arg_type));
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", RESULT, 
				stringfix(proc->res_type));
			f_print(fout, "\t\t%s = (char *(*)()) ", ROUTINE);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, ";\n");
			f_print(fout, "\t\tbreak;\n\n");
		}
		f_print(fout, "\tdefault:\n");
		printerr("noproc", TRANSP);
		print_return("\t\t");
		f_print(fout, "\t}\n");

		f_print(fout, "\tbzero((char *)&%s, sizeof(%s));\n", ARG, ARG);
		printif("getargs", TRANSP, "&", ARG);
		printerr("decode", TRANSP);
		print_return("\t\t");
		f_print(fout, "\t}\n");

		f_print(fout, "\t%s = (*%s)(&%s, %s);\n", RESULT, ROUTINE,
			ARG, RQSTP);
		f_print(fout, 
			"\tif (%s != NULL && !svc_sendreply(%s, xdr_%s, %s)) {\n",
			RESULT, TRANSP, RESULT, RESULT);
		printerr("systemerr", TRANSP);
		f_print(fout, "\t}\n");

		printif("freeargs", TRANSP, "&", ARG);
		(void) sprintf(_errbuf, "unable to free arguments");
		print_err_message("\t\t");
		f_print(fout, "\t\texit(1);\n");
		f_print(fout, "\t}\n");
		print_return("\t");
		f_print(fout, "}\n");
	}
}

static
printerr(err, transp)
	char *err;
	char *transp;
{
	f_print(fout, "\t\tsvcerr_%s(%s);\n", err, transp);
}

static
printif(proc, transp, prefix, arg)
	char *proc;
	char *transp;
	char *prefix;
	char *arg;
{
	f_print(fout, "\tif (!svc_%s(%s, xdr_%s, %s%s)) {\n",
		proc, transp, arg, prefix, arg);
}

nullproc(proc)
	proc_list *proc;
{
	for (; proc != NULL; proc = proc->next) {
		if (streq(proc->proc_num, "0")) {
			return (1);
		}
	}
	return (0);
}

static
write_inetmost(infile)
	char *infile;
{
	f_print(fout, "\tint sock;\n");
	f_print(fout, "\tint proto;\n");
	f_print(fout, "\tstruct sockaddr_in saddr;\n");
	f_print(fout, "\tint asize = sizeof(saddr);\n");
	f_print(fout, "\n");
	f_print(fout,
	"\tif (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0) {\n");
	f_print(fout, "\t\tint ssize = sizeof(int);\n\n");
	f_print(fout, "\t\tif (saddr.sin_family != AF_INET)\n");
	f_print(fout, "\t\t\texit(1);\n");
	f_print(fout, "\t\tif (getsockopt(0, SOL_SOCKET, SO_TYPE,\n");
	f_print(fout, "\t\t\t\t(char *)&_rpcfdtype, &ssize) == -1)\n");
	f_print(fout, "\t\t\texit(1);\n");
	f_print(fout, "\t\tsock = 0;\n");
	f_print(fout, "\t\t_rpcpmstart = 1;\n");
	f_print(fout, "\t\tproto = 0;\n");
	open_log_file(infile, "\t\t");
	f_print(fout, "\t} else {\n");
	write_rpc_svc_fg(infile, "\t\t");
	f_print(fout, "\t\tsock = RPC_ANYSOCK;\n");
	print_pmapunset("\t\t");
	f_print(fout, "\t}\n");
}

static
print_return(space)
	char *space;
{
	if (exitnow)
		f_print(fout, "%sexit(0);\n", space);
	else {
		if (timerflag)
			f_print(fout, "%s_rpcsvcdirty = 0;\n", space);
		f_print(fout, "%sreturn;\n", space);
	}
}

static
print_pmapunset(space)
	char *space;
{
	list *l;
	definition *def;
	version_list *vp;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			for (vp = def->def.pr.versions; vp != NULL;
					vp = vp->next) {
				f_print(fout, "%s(void) pmap_unset(%s, %s);\n",
					space, def->def_name, vp->vers_name);
			}
		}
	}
}

static
print_err_message(space)
	char *space;
{
	if (logflag)
		f_print(fout, "%ssyslog(LOG_ERR, \"%s\");\n", space, _errbuf);
	else if (inetdflag)
		f_print(fout, "%s_msgout(\"%s\");\n", space, _errbuf);
	else
		f_print(fout, "%sfprintf(stderr, \"%s\");\n", space, _errbuf);
}

/*
 * Write the server auxiliary function ( _msgout, timeout)
 */
void
write_svc_aux()
{
	if (!logflag)
		write_msg_out();
	write_timeout_func();
}

/*
 * Write the _msgout function
 */
static
write_msg_out()
{
	f_print(fout, "\n");
	f_print(fout, "static\n");
	f_print(fout, "_msgout(msg)\n");
	f_print(fout, "\tchar *msg;\n");
	f_print(fout, "{\n");
	f_print(fout, "#ifdef RPC_SVC_FG\n");
	f_print(fout, "\tif (_rpcpmstart)\n");
	f_print(fout, "\t\tsyslog(LOG_ERR, msg);\n");
	f_print(fout, "\telse\n");
	f_print(fout, "\t\t(void) fprintf(stderr, \"%%s\\n\", msg);\n");
	f_print(fout, "#else\n");
	f_print(fout, "\tsyslog(LOG_ERR, msg);\n");
	f_print(fout, "#endif\n");
	f_print(fout, "}\n");
}

/*
 * Write the timeout function
 */
static
write_timeout_func()
{
	if (!timerflag)
		return;
	f_print(fout, "\n");
	f_print(fout, "static void\n");
	f_print(fout, "closedown()\n");
	f_print(fout, "{\n");
	f_print(fout, "\tif (_rpcsvcdirty == 0) {\n");
	f_print(fout, "\t\textern fd_set svc_fdset;\n");
	f_print(fout, "\t\tstatic int size;\n");
	f_print(fout, "\t\tint i, openfd;\n");
	f_print(fout, "\n");
	f_print(fout, "\t\tif (_rpcfdtype == SOCK_DGRAM)\n");
	f_print(fout, "\t\t\texit(0);\n");
	f_print(fout, "\t\tif (size == 0)\n");
	f_print(fout, "\t\t\tsize = getdtablesize();\n");
	f_print(fout,
		"\t\tfor (i = 0, openfd = 0; i < size && openfd < 2; i++)\n");
	f_print(fout, "\t\t\tif (FD_ISSET(i, &svc_fdset))\n");
	f_print(fout, "\t\t\t\topenfd++;\n");
	f_print(fout, "\t\tif (openfd <= 1)\n");
	f_print(fout, "\t\t\texit(0);\n");
	f_print(fout, "\t}\n");
	f_print(fout, "\t(void) alarm(_RPCSVC_CLOSEDOWN);\n");
	f_print(fout, "}\n");
}


/*
 * Support for backgrounding the server if self started.
 */
static
write_rpc_svc_fg(infile, sp)
	char *infile;
	char *sp;
{
	f_print(fout, "#ifndef RPC_SVC_FG\n");
	f_print(fout, "%sint i, pid;\n\n", sp);
	f_print(fout, "%spid = fork();\n", sp);
	f_print(fout, "%sif (pid < 0) {\n", sp);
	f_print(fout, "%s\tperror(\"cannot fork\");\n", sp);
	f_print(fout, "%s\texit(1);\n", sp);
	f_print(fout, "%s}\n", sp);
	f_print(fout, "%sif (pid)\n", sp);
	f_print(fout, "%s\texit(0);\n", sp);
	f_print(fout, "%sfor (i = 0 ; i < 20; i++)\n", sp);
	f_print(fout, "%s\t(void) close(i);\n", sp);
	f_print(fout, "%si = open(\"/dev/console\", 2);\n", sp);
	f_print(fout, "%s(void) dup2(i, 1);\n", sp);
	f_print(fout, "%s(void) dup2(i, 2);\n", sp);
	f_print(fout, "%si = open(\"/dev/tty\", 2);\n", sp);
	f_print(fout, "%sif (i >= 0) {\n", sp);
	f_print(fout, "%s\t(void) ioctl(i, TIOCNOTTY, (char *)NULL);\n", sp);;
	f_print(fout, "%s\t(void) close(i);\n", sp);
	f_print(fout, "%s}\n", sp);
	if (!logflag)
		open_log_file(infile, sp);
	f_print(fout, "#endif\n");
	if (logflag)
		open_log_file(infile, sp);
}

static
open_log_file(infile, sp)
	char *infile;
	char *sp;
{
	char *s;

	s = rindex(infile, '.');
	if (s) 
		*s = '\0';
	f_print(fout,"%sopenlog(\"%s\", LOG_PID, LOG_DAEMON);\n", sp, infile);
	if (s)
		*s = '.';
}
