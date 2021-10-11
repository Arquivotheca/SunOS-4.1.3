/*      @(#)intr_routine.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "signal.h"

void
intr_routine(child)
{
        (void) kill(child,SIGINT);
        exit(1);
}

