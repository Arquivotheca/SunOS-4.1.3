/* @(#)life.h 1.1 92/07/30 Copyr 1984 Sun Micro */

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

#define MAXINT 0x7fffffff
#define MININT 0x80000000

#define CANVAS_ORIGIN	16000	/* because scrollbars can't have neg offsets */

#define NLINES	300
extern int amicolor;
extern int leftoffset;
extern int upoffset;
extern int life_spacing;

#define CMSIZE 64	/* must be power of 2 */
#define INITCOLOR 1

#define GLIDER 1
#define EIGHT 2
#define PULSAR 3
#define GUN 4
#define ESCORT 5
#define PUFFER 6
#define CLEAR 7
#define RUN 8
#define BARBER 9
#define TUMBLER 10
#define HERTZ 11
#define MUCHNICK 12
