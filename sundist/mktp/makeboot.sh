#! /bin/sh
#
#      %Z%  %M%  %I%  %E%  SMI
#
#	makeboot arch file [ seek ]
#
#	write a raw bootable image to standard output in
#	form for disk or tape
#
#	seek is used for disks
#
if [ $# -lt 2 -o $# -gt 3 ]
then
	echo "usage: $0 arch file [ seek ]" >> /dev/tty
	exit 1
elif [ $#  -eq 3 ]
then
	seek="seek=$3"
	file=$1
else
	seek=""
	file=$2
fi
machtype=$1
case  "$machtype" in
        "sun2" )
                pagesize=2k ;;
        "sun4c" | "sun4m" )
                pagesize=4k ;;
        "sun3" | "sun3x" | "sun4" )   
                pagesize=8k ;;
        * )    
                pagesize=8k ;
		echo ">>>>> pagesize defaulting to 8k."  >> /dev/tty;;
esac
dd if=$file bs=$pagesize conv=sync $seek
exit $?
