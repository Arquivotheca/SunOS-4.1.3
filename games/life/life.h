/* @(#)life.h 1.1 92/07/30 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

#define NLINES	300
extern int color;
extern char board[NLINES][NLINES];
extern int rightedge;
extern int bottomedge;
extern int leftoffset;
extern int upoffset;
extern int spacing;
extern struct rect currect;

#define CMSIZE 64	/* must be power of 2 */
#define INITCOLOR 1

#define GLIDER 1
#define EIGHT 2
#define PULSAR 3
#define GUN 4
#define ESCORT 5
#define PUFFER 6
#define RESET 7
#define RUN 8
#define BARBER 9
#define TUMBLER 10
#define HERTZ 11
#define MUCHNICK 12
