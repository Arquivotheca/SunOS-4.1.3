#!/bin/sh
# %W%
#
# This script creates a directory sub-tree that duplicates a sub-tree in
# .../usr/src/SCCS_DIRECTORIES. It will create symbolic links to SCCS
# directories in /usr/src/SCCS_DIRECTORIES.
#
# The path to the current working directory must contain "usr/src". The path
# up to this point is considered the base directory and must contain a
# directory or symbolic link named "SCCS_DIRECTORIES".
#
# To use this command; cd to the working directory (i.e. /usr/src,
# ~/usr/src/sys, /usr/src/lang/adb) and type "linksccs".
#
# the variable "src_base" has been added to allow the source to
# exist at a point other than /usr/src: if the default value
# of /usr/src is to be overridden, the source pathname must
# be entered on the command line as the first parameter:
#
#       linksccs /usr/src_3.4
#

src_base=${1:-usr/src}
set -u
umask 2
here=`pwd`/
node=`echo $here | sed -e "s=\(.*${src_base}\).*=\1="`
subdir=`echo $here | sed -e "s=\(.*${src_base}\)\(.*\)=\2="`
cd $node/SCCS_DIRECTORIES$subdir
find * -type d ! -name SCCS -print | cpio -pd $here
cd $here
(echo .;find `ls | grep -v SCCS_DIRECTORIES` -type d -print ) | while read dir
do
	from=$node/SCCS_DIRECTORIES/$subdir$dir/SCCS
	if [ -d $from ]
	then
		to=$here/$dir/SCCS
		rm -rf $to
		/bin/ln -s `/usr/release/bin/relpath $from $to`
	fi
done
