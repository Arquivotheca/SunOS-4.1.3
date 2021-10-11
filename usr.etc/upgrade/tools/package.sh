#! /bin/sh
#  %Z%%M% %I% %E% SMI
#
#  this script puts the files output from protocmp into 
#  the release packages.
#   
#  usage:  package input_filename output_filename
#
#  the input_filename is the output file from the script protocmp.
#  the output_filename will list packages and filenames.
#
#  package attempts to mount /tarfiles.
#  you should have an entry for /tarfiles in your fstab, eg:
#
#    werewolf:/tarfiles        /tarfiles   nfs ro,soft,bg,intr,noauto 0 0
#
#  steps:
#    make sure /tarfiles are mounted
#    search for lines in the input file beginning with obsolete, new, different
#    skip lines beginning with obsolete
#    strip off beginning of filename (/usr, /usr/kvm)
#    search /tarfiles/sun4c/packages for the file 
#    write the file name out to stdout 
#    check for any files that can't be assigned to a package 

CMD=`basename $0`
USAGE="Usage: $CMD input_filename output_filename"
cleanup='rm -rf /tmp/mk$$*'
trap 'eval "$cleanup"; exit 1' 1 2 3 15
TARFILES=/tarfiles
mount=/etc/mount
status=0

F1=$1
F2=$2
if [ $# -lt 2 ]; then
     echo $USAGE
     exit 1
fi

#
# Mount /tarfiles
#
$mount | grep -s $TARFILES
if [ $? -ne 0 ]; then
        [ ! -d $TARFILES ] && mkdir $TARFILES
        grep -v '^#' /etc/fstab | grep -s $TARFILES 
        if [ $? -eq 0 ]; then
                $mount $TARFILES
                if [ $? -ne 0 ]; then
                        echo "$CMD:  Unable to mount $TARFILES."
                        eval "$cleanup" && exit 1
                else
                        echo "Mounting $TARFILES..."
                        echo ""
                fi
        else
                echo "$CMD:  No entry for $TARFILES in fstab."
                eval "$cleanup" && exit 1
        fi
fi
 
# read the file and look for lines beginning with 'new' or 'different'

egrep '(^new * |^different *)' $F1 | awk '{printf "%s\n", $2}' |
sed "s/^\.\/usr\/kvm/\./" | sed "s/^\.\/usr/\./" | sort > /tmp/mk$$.1
if [ ! -s /tmp/mk$$.1 ] ; then
    echo "$CMD: Cannot find any new or different files in $F1."
    eval "$cleanup"; exit 1
fi

cat /tmp/mk$$.1 | tr '\012' ' ' > /tmp/mk$$.2

# loop through list of packages and make the output file

rm -f $F2; touch $F2
for tarfile in `ls $TARFILES/sun4c` 
do
    echo "doing $tarfile..."
    tar tf $TARFILES/sun4c/$tarfile `cat /tmp/mk$$.2`  |
          sed "s/^/$tarfile	/" >> $F2
done

# tell the user about any files that didn't get put into a package

echo "checking for unassigned files..."
cat $F2 | awk '{printf "%s\n", $2}' | sort | uniq > /tmp/mk$$.found 
comm -3 /tmp/mk$$.1 /tmp/mk$$.found > /tmp/mk$$.3
if [ -s /tmp/mk$$.3 ]; then
    cat /tmp/mk$$.3 | sed "s/^/UNASSIGNED	/" >> $F2
fi

eval "$cleanup"; exit 0
