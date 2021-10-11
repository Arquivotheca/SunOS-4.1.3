#! /bin/sh
#
#  %Z%%M% %I% %E% SMI
#
#   a modified version of dircmp which compares all files in 
#   two proto directory trees, and lists whether the files are
#   obsolete, new, or different.
#   also compares archives by extracting objects and listing 
#   whether the objects are obsolete, new or different. 
#   files are reported as different if they do not pass the
#   cmp program; file protection, owner and group changes are 
#   not taken into account.
#
#   usage:   protocmp old_proto new_proto
#
#   output:        
#
#       obsolete      filename     (file is not in new_proto)
#       new           filename     (file is not in old_proto)
#       different     filename     (files are different)
#
#   sample output:
#
#	obsolete     ./usr/include/scsi/adapters/screg.h
#	obsolete     ./usr/include/sundev/cg6reg.h
#       new          ./etc/install/category.repreinstalled
#       new          ./etc/install/label_script
#       different    ./etc/format.dat
#	different    ./usr/lib/libpixrect.a
#	             new          ./cg12.o
#	             new          ./cg12_batch.o
#	             different    ./cg8_colormap.o
#       different    ./usr/etc/devinfo
#
PATH=/bin:/usr/bin:/usr/ucb
USAGE="usage: protocmp old_proto new_proto"
trap "rm -rf /usr/tmp/dc$$*;exit" 1 2 3 15
release="usr/sys/conf.common/RELEASE"
mount=/etc/mount

D0=`pwd`
D1=$1
D2=$2
if [ $# -lt 2 ]
then echo $USAGE
     exit 1
elif [ ! -d "$D1" ]
then echo $D1 not a directory !
     exit 2
elif [ ! -d "$D2" ]
then echo $D2 not a directory !
     exit 2
fi

#
# You must be root to run this script
#
if [ "`whoami`x" != "root"x ]; then
   echo "You must be root to do $0."
   exit 1
fi

cmd=$0
slash=`echo $0 | colrm 2`
[ $slash != "/" ] && cmd=${D0}/$0

#
# print header
#
if [ -f $D1/$release -a -f $D2/$release ]; then
    from1=`$mount | grep $D1 | awk '{ printf $1 }' `
    from2=`$mount | grep $D2 | awk '{ printf $1 }' `
    [ "$from1" ] && from1="mounted from $from1"
    [ "$from2" ] && from2="mounted from $from2"

    echo "Comparison of $D1 `echo $from1` (`cat $D1/$release`) "
    echo "          and $D2 `echo $from2` (`cat $D2/$release`)"
fi

cd $D1
find . -print | sort > /usr/tmp/dc$$a
cd $D0
cd $D2
find . -print | sort > /usr/tmp/dc$$b

comm /usr/tmp/dc$$a /usr/tmp/dc$$b | sed -n \
	-e "/^		/w /usr/tmp/dc$$c" \
	-e "/^	[^	]/w /usr/tmp/dc$$d" \
	-e "/^[^	]/w /usr/tmp/dc$$e"
rm -f /usr/tmp/dc$$a /usr/tmp/dc$$b

sed "s/^/obsolete     /" /usr/tmp/dc$$e
sed "s/^./new          /" /usr/tmp/dc$$d
rm -f /usr/tmp/dc$$e /usr/tmp/dc$$d

here1=""
slash=`echo $D1 | colrm 2`
[ $slash != "/" ] && here1="$D0/"
here2=""
slash=`echo $D2 | colrm 2`
[ $slash != "/" ] && here2="$D0/"

sed -e s/..// < /usr/tmp/dc$$c > /usr/tmp/dc$$f
rm -f /usr/tmp/dc$$c
cd $D0
while read a
do
	if [ -d $D1/"$a" ]; then
             :
	elif [ -f $D1/"$a" ]; then

             file $D1/"$a" | fgrep -s archive

             # archive file
             if [ $? -eq 0 ]; then
                mkdir /usr/tmp/dc$$.1; cd /usr/tmp/dc$$.1; ar x ${here1}$D1/"$a"
                mkdir /usr/tmp/dc$$.2; cd /usr/tmp/dc$$.2; ar x ${here2}$D2/"$a"

                rm -f /usr/tmp/dc$$.1/*SYMDEF /usr/tmp/dc$$.2/*SYMDEF
                $cmd /usr/tmp/dc$$.1 /usr/tmp/dc$$.2 > /usr/tmp/dc$$.diffs
                if [  -s /usr/tmp/dc$$.diffs ]; then
                    echo "different    $a"
                    sed "s/^/             /" /usr/tmp/dc$$.diffs
                fi
                cd $D0
                rm -rf /usr/tmp/dc$$.1 /usr/tmp/dc$$.2 /usr/tmp/dc$$.diffs

             # non-archive file
	     else cmp -s $D1/"$a" $D2/"$a" 
                if [ $? -ne 0 ]; then 
                    echo "different    $a" 
                fi 
             fi	
	fi
done < /usr/tmp/dc$$f 
rm -f /usr/tmp/dc$$*

