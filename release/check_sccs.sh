:
  ##  @(#)check_sccs.sh	1.1
  ##  purpose:	List all source files that are not in SCCS
  ##  remarks:	Kernel directories in /usr/src/sys/`arch -k` are not processed

PWD=`pwd`
TEMP=/tmp/$$
src_dir=/usr/src
cd $src_dir
#find all directories under $src_dir, excluding SCCS*, . and ..

echo "Searching $src_dir directories..."
find `ls -a | sed -e /SCCS/d  -e '/^\.$/d' -e '/^\.\.$/d'` \
! -name SCCS -type d -print > $TEMP

echo "Kernel directories in /usr/src/sys/`arch -k` will not be examined."
grep -v "sys/`arch -k`/[A-Z]" $TEMP > $TEMP.out

echo "Output file is $TEMP.no_sccs..."
cat $TEMP.out | while read sub_dir; do	
	cd $src_dir/$sub_dir
	echo "Descending into: `pwd`" 

#Descend into all directories, one level at a time.
#Find all source files, which are named:  Make*, *.{c,s,sh,h,y,r,f,p,l},
#  and match them up with SCCS files
#Do not process zero-sized files and links

	for each_file in `ls`
	do
	  case $each_file in
		   Make*|*.sh ) ;;
	         *.[cshyrfpl] ) ;;
		           *  ) continue;;
	  esac
	  [ -f SCCS/s.${each_file} ] && continue
	  [ -h ${each_file} ] && continue
	  [ ! -s ${each_file} ] && continue
	  echo "./${sub_dir}/${each_file}" >> $TEMP.no_sccs
	done
done
rm -f $TEMP.out $TEMP
cd $PWD
[ ! -s $TEMP.no_sccs ]  && echo "All source files are in SCCS." && exit
echo "Source files are listed in $TEMP.no_sccs..."
