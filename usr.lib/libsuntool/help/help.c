#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)help.c 1.1 92/07/30 Copyright 1987 Sun Micro";
#endif
#endif




/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */




#include <stdio.h>
#include <rpc/rpc.h>
#include <sunwindow/defaults.h>
#include <suntool/sunview.h>
#include <suntool/alert.h>
#include <netdb.h>
#include <suntool/help.h>
#include <sys/socket.h>         /* for callrpc2() */
#include <sys/time.h>           /* for callrpc2() */
#include <strings.h>            /* for callrpc2() */





#define HELPNAMELENGTH 256
#define HELPTEXTLINES 32




static short    help_image[] = {
#include <images/help.icon>
};
mpr_static(help_request_icon, 64, 64, 1, help_image);

static struct timeval timeout1 = { 5, 0 };
static struct timeval timeout2 = { 25, 0 };

static void     help_request_default();
static void     (*help_request_func)() = help_request_default;

static void     help_more_default();
static void     (*help_more_func)();

static void     help_set_server();
static char     help_server_name[HELPNAMELENGTH];

static int      help_request_seq;




static void
help_request_failed(window, data, str, why, event)
Window window;
char    *data, *str, *why;
Event   *event;
{
        char    message[256];

        if (data) {
                sprintf(message, "%s for %s.", str, data);
        } else {
                sprintf(message, "%s.", str);
        }
        alert_prompt(NULL, event,
            ALERT_POSITION, ALERT_SCREEN_CENTERED,
            ALERT_MESSAGE_STRINGS, message, 0,
            ALERT_BUTTON_YES, "OK", 0,
            HELP_DATA, why,
            0);
}




/*
 * Default more help delivery function
 */

static void
help_more_default(client, data, event)
Window  client;
char    *data;
Event   *event;
{
        Help_request    arg;
        char            command[HELPNAMELENGTH+2];
        int             ret, callrpc2();

        arg.data = data;
        if (event) {
                arg.x    = event_x(event);
                arg.y    = event_y(event);
        }
        arg.pid  = getpid();
        arg.seq  = ++help_request_seq;
        if (help_server_name[0] == '\0') {
                /* Get server name from defaults database */
                strncpy(help_server_name,
                    defaults_get_string("/Help/Server", HELPDEFAULTSERVER, 0),
                    HELPNAMELENGTH - 1);
        }
        if (callrpc2("localhost", HELPDEFAULTPROG, HELPVERS, HELPREQUEST, 
                    xdr_help, &arg, xdr_void, NULL, &timeout1)) {

                /* This fixes a bug where the user quits & restarts help servers */
                /* between rpc calls. If this is the case, the second call should */
                /* now go through: */

                ret = callrpc2("localhost", HELPDEFAULTPROG, HELPVERS, HELPREQUEST, 
                                xdr_help, &arg, xdr_void, NULL, &timeout2);
                if (ret) {
                        /* Request failed, spawn help server */
                        if (alert_prompt(NULL, NULL,
                                         ALERT_POSITION, ALERT_SCREEN_CENTERED,
                                         ALERT_MESSAGE_STRINGS,
                                         "A help server to display more help",
                                         "was not found. There will be a short",
                                         "delay while one is started.",
                                         " ",
                                         " ",
                                         NULL,
                                         ALERT_BUTTON_YES, "Continue",
                                         ALERT_BUTTON_NO, "Cancel",
                                         NULL)
                        == ALERT_YES) {
                                sprintf(command, "%s '%s' &", help_server_name, data);
                                system(command);
                        }
                }
        }
        return;
}




/*
 * Default request function
 */

static void
help_request_default(client_window, client_data, client_event)
Window  client_window;
caddr_t client_data;
Event   *client_event;
{
        char    *arg = help_get_arg(client_data),
                *text = (arg ? help_get_text() : NULL),
                buffer[HELPTEXTLINES][80],
                *array[HELPTEXTLINES+1];
        Event   event;
        int     i, more_info = TRUE;
        int     use_doc = (arg ? *arg != '\0' : FALSE);


        if (client_event) {
                event = *client_event;
        }
        if (text) {
                for (i = 0; text && i < HELPTEXTLINES; ++i, (text = help_get_text())) {
                        array[i] = (char *)strncpy(buffer[i], text, sizeof(buffer[i]) - 1);
                        if (buffer[i][strlen(buffer[i]) - 1] == '\n')
                                buffer[i][strlen(buffer[i]) - 1] = '\0';
                }
                array[i] = NULL;
                more_info = (alert_prompt(NULL, &event,
                    HELP_DATA, "sunview:helpalert",
                    ALERT_IMAGE, &help_request_icon,
                    ALERT_POSITION, ALERT_SCREEN_CENTERED,
                    ALERT_NO_BEEPING, TRUE,
                    ALERT_MESSAGE_STRINGS_ARRAY_PTR, array,
                    ALERT_BUTTON_YES, "Done",
                    (use_doc ? ALERT_BUTTON_NO : 0), "More Help",
                    0)
                     == ALERT_NO);
        } else if (text || (!text && !use_doc)) {
                help_request_failed(client_window, client_data,
                    "No help is available", "sunview:helpnoinfo",
                    &event);
        }
        /* This sends a help request directly to the help */
        /* server, with no intervening Spot Help message. */
        if (use_doc && more_info) {
                help_more(client_window, arg, &event);
        }
}




/*
 * Public interfaces
 */

void
help_request(client, data, event)
Window  client;
caddr_t data;
Event   *event;
{
        if (help_request_func) {
                help_request_func(client, data, event);
        }
}




void
help_more(client, data, event)
Window  client;
char    *data;
Event   *event;
{
        if (! help_more_func) {
                help_more_func = help_more_default;
        }
        (*help_more_func)(client, data, event);
}




void
help_set_request_func(func)
void    (*func)();
{
        help_request_func = func;
}




void
help_set_more_func(func)
void    (*func)();
{
        help_more_func = func;
}




/*
 * callrpc(), with timeout parameter:
 */

static struct callrpc_private {
        CLIENT  *client;
        int     socket;
        int     oldprognum, oldversnum, valid;
        char    *oldhost;
} *callrpc_private;

callrpc2(host, prognum, versnum, procnum, inproc, in, outproc, out, tottimeout)
        char *host;
        xdrproc_t inproc, outproc;
        char *in, *out;
        struct timeval *tottimeout;
{
        register struct callrpc_private *crp = callrpc_private;
        struct sockaddr_in server_addr;
        enum clnt_stat clnt_stat;
        struct hostent *hp;
        struct timeval timeout;

        if (crp == 0) {
                crp = (struct callrpc_private *)calloc(1, sizeof (*crp));
                if (crp == 0)
                        return (0);
                callrpc_private = crp;
        }
        if (crp->oldhost == NULL) {
                crp->oldhost = malloc(256);
                crp->oldhost[0] = 0;
                crp->socket = RPC_ANYSOCK;
        }
        if (crp->valid && crp->oldprognum == prognum && crp->oldversnum == versnum
                && strcmp(crp->oldhost, host) == 0) {
                /* reuse old client */          
        } else {
                crp->valid = 0;
                (void)close(crp->socket);
                crp->socket = RPC_ANYSOCK;
                if (crp->client) {
                        clnt_destroy(crp->client);
                        crp->client = NULL;
                }
                if ((hp = gethostbyname(host)) == NULL)
                        return ((int) RPC_UNKNOWNHOST);
                timeout.tv_usec = 0;
                timeout.tv_sec = 5;
                bcopy(hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
                server_addr.sin_family = AF_INET;
                server_addr.sin_port =  0;
                if ((crp->client = clntudp_create(&server_addr, (u_long)prognum,
                    (u_long)versnum, timeout, &crp->socket)) == NULL)
                        return ((int) rpc_createerr.cf_stat);
                crp->valid = 1;
                crp->oldprognum = prognum;
                crp->oldversnum = versnum;
                (void) strcpy(crp->oldhost, host);
        }
        clnt_stat = clnt_call(crp->client, procnum, inproc, in,
            outproc, out, *tottimeout);
        /* 
         * if call failed, empty cache
         */
        if (clnt_stat != RPC_SUCCESS)
                crp->valid = 0;
        return ((int) clnt_stat);
}
