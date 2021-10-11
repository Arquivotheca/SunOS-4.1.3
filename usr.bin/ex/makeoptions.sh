#! /bin/sh -
#
#	%Z%%M% %I% %E% SMI; from S5R3.1 1.5
#

#
#	make options --should be run when adding/deleting options
#			This script should be run to create a new
#			ex_vars.h (based on information in ex_data.c
#			Ex_data.c should be writeable.
#
	cp ex_data.c $$.c
#
#	write all lines which are not includes to $$.c
#
	set -x
	ex - $$.c << 'STOP'
g/^#include/d
w
q
STOP
cc -E $* $$.c > foo.c
#
#	use EX to clean up preprocessor output
#
	ex - foo.c << 'STOP'
!echo STEP1
$-1,$d
1,/option options/d
!echo STEP2
g/^# /d
!echo STEP3
g/^[	]*$/d
!echo STEP4
%s/	\"//
!echo STEP5
%s/\".*//
!echo STEP6
1m$
!echo STEP7
w! tmpfile1
!pr -t -n tmpfile1 > tmpfile2
!echo STEP8
q
STOP
#
	ex -s tmpfile2 << 'STOP'
$t0
!echo STEP9
1s/....../    0	/
!echo STEP10
w len4
%s/\(......\)\(.*\)/vi_\U\2\L		\1/
!echo STEP11
%s/	$//
!echo STEP12
%s/[ ][ ]*/	/
!echo STEP13
$s/.*	/vi_NOPTS		/
!echo STEP14
%s/\([A-Z][A-Z][A-Z][A-Z][A-Z][A-Z]*\)\(	\)/\1/
!echo STEP15
%s/.*/#define	&/
!echo STEP16
$i

.
0a
#ident "@(#)vi:port/makeoptions	1.5"
.
w! ex_vars.h
q
STOP
echo finished....
