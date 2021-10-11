#! /bin/sh
#
#       %Z%  %M% %I% %E%
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#
#	maketape tape_device tape_volume
#
HOME=/; export HOME
PATH=/bin:/usr/bin:/etc:/usr/etc:/usr/ucb:/usr/rfs/install

CMDNAME=$0
MYPATH=`pwd`

USAGE="usage: 
${CMDNAME} tape_device tape_volume protopath
where:
        tape_device = \"ar\" or \"st\" or \"mt\" or \"xt\"
        tape_volume = positive integer
	protopath   = full path of proto
"
#
# Verify number of arguments
#
if [ $# -ne 3 ]; then
        echo "${CMDNAME}: incorrect number of arguments."
        echo "${USAGE}"
        exit 1
fi

if [ `whoami` != root ]; then
        echo ${cmdname}: must be run as root
        exit 2
fi

tapetype=$1
PROTO=$3
case "${tapetype}" in
"mt" | "xt" )
	bs=20
	cd /dev
	rm *mt*
	/dev/MAKEDEV ${tapetype}0 2> /dev/null
	tape=/dev/nr${tapetype}0 ;;
"ar" | "st" )
	bs=200
	cd /dev
	/dev/MAKEDEV ${tapetype}0 2> /dev/null
	tape=/dev/nr${tapetype}0 ;;
*)
	echo "${CMDNAME}: missing or invalid tape type \"${tapetype}\"."
	exit 1 ;;
esac

#
# create table of contents for each tape
#
if [ -f /bin/arch ]; then
        archtype=`/bin/arch`
	cp ${MYPATH}/toc ${MYPATH}/toc.${archtype}
	ed - ${MYPATH}/toc.${archtype} <<END
/ARCH/
s/ARCH/$archtype/p
w
q
END
	toc="toc.${archtype}"
else
        echo;echo "Error: Can't find file /bin/arch !! "
        exit 2
fi

sync
sync
if [ -z "$2" -o "$2" = 1 ]; then
	while true; do
                STRING=`mt -f ${tape} rew 2>&1`
                if [ -z "${STRING}" ]; then
                        break
                else
                        echo "${STRING}"
                        echo "Tape drive ${tape} not ready."
                        echo -n "Load tape and hit <RETURN>: "
                        read x
                fi
        done
	# Copyright
	cd ${MYPATH}
        dd if=Copyright of=$tape conv=sync
        # toc
        dd if=${toc} of=$tape conv=sync
	# install util
	cd ${MYPATH}
	tar cpvfb $tape $bs install_rfs extracting verify_tapevol_arch
	# rfs
	cd $PROTO
	tar cpvfb $tape $bs .
	# Copyright
	cd ${MYPATH}
	dd if=Copyright of=$tape conv=sync
	mt -f $tape rew
	echo "Completed tape #1, remove tape"
fi
