#! /bin/csh -f
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	%Z%%M% %I% %E% SMI; from UCB 5.3 (Berkeley) 11/13/85
#
# vgrind

# Get formatter from environment, if present, using "troff" as default.
if ( $?TROFF ) then
	set troff = "$TROFF"
else
	set troff = "troff"
endif

# Since vfontedpr and the macro package are unlikely to be in the path,
# name them explicitly.  Note that we only refer to $macros when in filter
# mode; otherwise we rely on troff's -m flag to locate the macro package.
set vfontedpr	= /usr/lib/vfontedpr
set macros	= /usr/lib/tmac/tmac.vgrind

# We explicitly recognize some arguments as options that should be passed
# along to $troff.  Others go directly to $vfontedpr.  We collect the former
# in troffopts and the latter in args.  We treat certain arguments as file
# names for the purpose of processing the index.  When we encounter these,
# we collect them into the files variable as well as adding them to args.
set troffopts
set args
set files

# Initially not in filter mode.
unset filter

# Collect arguments.  The code partitions arguments into the troffopts,
# args, and files variables as described above, but otherwise leaves their
# relative ordering undisturbed.  Also, note the use of `:q' in variable
# references to preserve each token as a single word.
top:
if ($#argv > 0) then
    switch ( $1:q )

    # Options to be passed along to the formatter ($troff).

    case -o*:
	set troffopts = ( $troffopts:q $1:q )
	shift
	goto top

    case -P*:    # Printer specification -- pass on to troff for disposition
	set troffopts = ( $troffopts:q $1:q )
	shift
	goto top

    case -T*:    # Output device specification -- pass on to troff
	set troffopts = ( $troffopts:q $1:q )
	shift
	goto top

    case -t:
	set troffopts = ( $troffopts:q -t )
	shift
	goto top

    case -W:	# Versatec/Varian toggle.
	set troffopts = ( $troffopts:q -W )
	shift
	goto top

    # Options to be passed along to vfontedpr.  The last two cases cover
    # ones that we don't explicitly recognize; the others require special
    # processing of one sort or another.

    case -d:
    case -h:
	if ($#argv < 2) then
	    echo "vgrind: $1:q option must have argument"
	    goto done
	else
	    set args = ( $args:q $1:q $2:q )
	    shift
	    shift
	    goto top
	endif

    case -f:
	set filter
	set args = ( $args:q $1:q )
	shift
	goto top

    case -w:    # Alternative tab width (4 chars) -- vfontedpr wants it as -t
	set args = ( $args:q -t )
	shift
	goto top

    case -*:
	# We call this out as an explicit option to prevent flags that we
	# pass on to vfontedpr from being treated as file names.  Of course
	# this keeps us from dealing gracefully with files whose names do
	# start with `-'...
	set args  = ( $args:q  $1:q )
	shift
	goto top

    default:
    case -:
	# Record both as ordinary argument and as file name.  Note that
	# `-' behaves as a file name, even though it may well not make
	# sense as one to the commands below that see it.
	set files = ( $files:q $1:q )
	set args  = ( $args:q  $1:q )
	shift
	goto top
    endsw
endif

if (-r index) then
    echo > nindex
    foreach i ( $files )
	#	make up a sed delete command for filenames
	#	being careful about slashes.
	echo "? $i ?d" | sed -e "s:/:\\/:g" -e "s:?:/:g" >> nindex
    end
    sed -f nindex index > xindex
    if ($?filter) then
	$vfontedpr $args:q | cat $macros -
    else
	$vfontedpr $args:q | \
	    /bin/sh -c "$troff -rx1 $troffopts -i -mvgrind 2>> xindex"
    endif
    sort -df +0 -2 xindex > index
    rm nindex xindex
else
    if ($?filter) then
	$vfontedpr $args:q | cat $macros -
    else
	$vfontedpr $args:q | $troff -i $troffopts -mvgrind
    endif
endif

done:
