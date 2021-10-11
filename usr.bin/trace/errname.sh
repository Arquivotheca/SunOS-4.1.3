#!/bin/sh
###############
#	%Z%%M% %I% %E%  SMI
#
#  Generate error name table errname.c from errno.h
#
#  Copyright (c) 1986 by Sun Microsystems, Inc.
#

awk  < /usr/include/sys/errno.h   > errname.c   '
BEGIN { printf "char *errname [] = {\n" ;
	printf "\t\"ERR0\",\t\t/*  0 - No error */\n" ;
      }

/^#define[ \t]+E/ {
   printf "\t\"%s\",\t", $2 ;
   for (f = 3 ; f <= NF && $f == "" ; f++)
      ;
   printf "/* %2d - ", $f ;
   for ( ++f ; f <= NF && $f == "" ; f++)
      ;
   for ( ++f ; f <= NF ; f++)
      printf "%s ", $f ;
   printf "\n" ;
   }

END { printf "\t\"\" } ;\n" }
   '
