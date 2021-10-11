#ifndef lint
static  char sccsid[] = "@(#)ATS tcm_dtm.c 1.1 92/07/30 Copyright Sun Microsystems Inc.";
#endif

/*
 * tcm_dtm.c
 *
 *		The xdr routines that tcm uses to send data
 *	structures to dtm.
 *
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tcm_dtm.h"

xdr_option(xdrs, pt)
     XDR *xdrs;
     struct option_file *pt;
{
  char *ptr;
  
  ptr=pt->fname;
  if (!xdr_wrapstring(xdrs, &ptr))
    return(0);
  if (!xdr_int(xdrs, &pt->action))
    return(0);
  return(1);
}

xdr_strings(xdrs, strings)
     XDR *xdrs;
     struct strings *strings;
{
  char *ptr;
  int i;
  
  if (!xdr_int(xdrs,&strings->num))
     return(0);
  if (strings->num>5)
    return(0);
  for(i=0;i<strings->num;i++) {
    ptr=strings->op[i];
    if (!xdr_wrapstring(xdrs, &ptr))
      return(0);
  }
  return(1);
}
