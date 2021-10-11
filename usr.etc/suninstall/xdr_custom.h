/*
 *
 * @(#)xdr_custom.h 1.1 92/07/30 Copyr 1987 Sun Micro
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 */

struct futureslop { char dummy; };
extern bool_t xdr_futureslop();

struct nomedia { char dummy; };
extern bool_t xdr_nomedia();
