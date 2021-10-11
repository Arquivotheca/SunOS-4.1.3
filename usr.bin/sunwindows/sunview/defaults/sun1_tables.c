#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)sun1_tables.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * sun1_tables.c:    keytables for the Sun-1 (VT-100-style) keyboard
 */


#include "setkey_tables.h"

static Pair	sun1_standard_downs[] = {
    {15, LF(2)},  {16, RF(1)},  {17, LF(1)},  {18, RF(2)},
    {35, LF(4)},  {36, RF(3)},  {37, LF(3)},  {38, RF(4)},
    {53, LF(6)},  {54, RF(5)},  {55, LF(5)},  {56, RF(6)},
    {72, LF(8)},  {73, RF(7)},  {74, LF(7)},  {75, HOLE},
    {90, LF(10)}, {91, HOLE},   {92, LF(9)},  {93, RF(8)}
};

static Key_info	 sun1_standard = {
    sun1_standard_downs, sizeof(sun1_standard_downs) / sizeof(Pair), 0, 0
};

static Pair	sun1_noarrow_downs[] = {
    {10, TF(1)}, {11, TF(2)}, {12, TF(3)}, {13, TF(4)},

    {15, LF(2)},  {16, RF(1)},  {17, LF(1)},  {18, RF(2)},
    {35, LF(4)},  {36, RF(3)},  {37, LF(3)},  {38, RF(4)},
    {53, LF(6)},  {54, RF(5)},  {55, LF(5)},  {56, RF(6)},
    {72, LF(8)},  {73, RF(7)},  {74, LF(7)},  {75, HOLE},
    {90, LF(10)}, {91, HOLE},   {92, LF(9)},  {93, RF(8)}
};

static Key_info	 sun1_standard_noarrow = {
    sun1_noarrow_downs, sizeof(sun1_noarrow_downs) / sizeof(Pair), 0, 0
};
static Key_info	 sun1_lefty = {
    sun1_standard_downs, sizeof(sun1_standard_downs) / sizeof(Pair), 0, 0
};
static Key_info	 sun1_left_noarrow = {
    sun1_noarrow_downs, sizeof(sun1_noarrow_downs) / sizeof(Pair), 0, 0
};

Keybd_info	ktbl_sun1_info = {
    &sun1_standard, &sun1_standard_noarrow, &sun1_lefty, &sun1_left_noarrow
};
