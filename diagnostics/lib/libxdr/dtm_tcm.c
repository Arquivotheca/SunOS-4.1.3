#ifndef lint
static  char sccsid[] = "@(#)ATS dtm_tcm.c 1.1 92/07/30 Copyright Sun Microsystems Inc.";
#endif


#include <rpc/rpc.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include "dtm_tcm.h"

xdr_failure(xdrsp,failure)
     XDR *xdrsp;
     struct failure *failure;
{
  char *ptr;

  if (!xdr_long(xdrsp, &failure->hostid))
    return(0);
  if (!xdr_int(xdrsp, &failure->run_status))
    return(0);
  ptr=failure->testname;
  if (!xdr_wrapstring(xdrsp,&ptr))
    return(0);
  ptr=failure->devname;
  if (!xdr_wrapstring(xdrsp,&ptr))
    return(0);
  ptr=failure->message;
  if (!xdr_wrapstring(xdrsp,&ptr))
    return(0);
  return(1);
}


xdr_test(xdrsp,test)
     XDR *xdrsp;
     struct test *test;
{
  char *ptr;
  
  if (!xdr_long(xdrsp, &test->hostid))
    return(0);
  ptr=test->testname;
  if (!xdr_wrapstring(xdrsp,&ptr))
    return(0);
  ptr=test->devname;
  if (!xdr_wrapstring(xdrsp,&ptr))
    return(0);
  if (!xdr_int(xdrsp,&test->pass_count))
    return(0);
  return(1);
}
