#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)help_rpc.c 1.1 92/07/30 Copyright 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <sunwindow/notify.h>
#include <suntool/help.h>

static int help_object;
static int last_pid, last_seq;
static int *help_client = &help_object;
static void (*help_proc)();

int
xdr_help(xdrsp, arg)
    XDR *xdrsp;
    Help_request *arg;
{
    return (xdr_string(xdrsp, &arg->data, HELPDATAMAX) &&
            xdr_int(xdrsp, &arg->x) &&
            xdr_int(xdrsp, &arg->y) &&
            xdr_int(xdrsp, &arg->pid) &&
            xdr_int(xdrsp, &arg->seq));
}

static Notify_value
help_rpc_service(client, fd)
    int *client;
    int fd;
{
    svc_getreq(1 << fd);
    return (NOTIFY_DONE);
}

static void
help_rpc_received(rqstp, transp)
    struct svc_req *rqstp;
    SVCXPRT *transp;
{
    Help_request arg;
    int arg_ok;


    switch (rqstp->rq_proc) {
        case NULLPROC:
            (void)svc_sendreply(transp, xdr_void, 0);
            break;
      case HELPREQUEST:
            bzero(&arg, sizeof(Help_request));
            arg_ok = svc_getargs(transp, xdr_help, &arg);
            (void)svc_sendreply(transp, xdr_void, 0);
            if (arg_ok) {
                if (last_pid != arg.pid || last_seq != arg.seq)
                        (*help_proc)(arg.data);
                last_pid = arg.pid;
                last_seq = arg.seq;
                svc_freeargs(transp, xdr_help, &arg);
            }
            break;
        default:
            svcerr_noproc(transp);
            break;
    }
}

int
help_rpc_register(func)
    void (*func)();
{
    int fd;
    extern fd_set svc_fdset;
    SVCXPRT *transp;

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL)
        return (0);
    pmap_unset(HELPDEFAULTPROG, HELPVERS);
    pmap_unset(HELPDEFAULTPROG, HELPVERS);  /* Need two for some reason */
    if (!svc_register(transp, HELPDEFAULTPROG, HELPVERS,
                      help_rpc_received, IPPROTO_UDP))
        return (0);
    help_proc = func;

    for (fd = 0; fd < FD_SETSIZE; ++fd)
        if (FD_ISSET(fd, &svc_fdset))
            notify_set_input_func(help_client, help_rpc_service, fd);

    return (1);
}

int
help_rpc_unregister()
{
    /* need two for some reason */
    pmap_unset(HELPDEFAULTPROG, HELPVERS);
    pmap_unset(HELPDEFAULTPROG, HELPVERS);
}
