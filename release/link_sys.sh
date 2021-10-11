:
#@(#)link_sys.sh 1.1 92/07/30 SMI
######################################################################
## purpose:  To duplicate the /usr/src directories
##	     under a feature source tree, and link all the
##	     required sources from /usr/src TO the feature
##	     source tree.  This saves disk space, in that a single set	
##	     of /usr/src files (mounted read only) can be used
##	     for multiple feature tape builds on the same system,
##	     without populating the feature tape source
##	     directories with actual copies of referenced sources.
##
##	     A typical application would be to run 'link_sys' under 
##	     $feature_tree/sys to build a kernel to support new drivers.
#
## general steps:
##
##	1)   Set up your feature source tree with proper SCCS links
##	2)   Do a recursive get of your modified sources	
##	3)   Run 'link_sys'
##
##	All intermediate files generated during 'make' will be 
##	located under the feature tree.
##	
##--------------------------EXAMPLE-----------------------------------
## 
##	If you want to build a kernel using modified drivers (in sys/sundev)
##	from the 4.0.3 reference sources located on due you would do these:
##	
##	1)   Find a system running 4.0.3 FCS
##
##	2)   Mount the 4.0.3 reference source; you will use this to 
##	     flesh out the dirs required to build your new kernel
##		due:/usr/src.403 /refsrc/403 nfs ro,hard,intr 0 0
##
##	3)   Set up your own source tree to contain your changes:
##
##	     If you already have your feature source tree available, 
##	     just mount it and do a recursive get to populate your 
##	     work area, and proceed to Step 4.
##
##	     OTHERWISE, create them now using these steps:
##
##	     Create the SCCS_DIRECTORIES structure:
##		set linksrc=/home/linksrc (or whatever)
##		mkdir $linksrc
##		mkdir $linksrc/SCCS_DIRECTORIES
##		mkdir $linksrc/SCCS_DIRECTORIES/sys
##		mkdir $linksrc/SCCS_DIRECTORIES/sys/sundev
##		mkdir $linksrc/SCCS_DIRECTORIES/sys/sundev/SCCS
##	        (repeat with other dirs, as necessary)
##
##	     Create your links to SCCS_DIRECTORIES, thus:
##		cd $linksrc
##		using RE utils:
##			linksccs $linksrc 
##		or manually:
##			mkdir sys
##			mkdir sys/sundev
##			cd sys/sundev
##			ln -s ../../SCCS_DIRECTORIES/sys/sundev/SCCS SCCS
##			cd $linksrc/sys/sundev
##			cp -p newdriver/st.c .
##			sccs create st.c; rm ,*
##			...	
##	4)  Link up to everything else in sys from 403 reference src,
##	    using this utility.
##
##	    cd $linksrc/sys
##	    link_sys  (Takes ~20 minutes to process sys hierarchy)
##	    Typical response to script queries are:
##	        DIR=/home/linksrc/sys? (y/n)   y
##		Link to which directory? (eg: /usr/src/sys)? /refsrc/403/sys
##		(Select #3 from the menu which appears)  
##
##
##	    Your src/sys structure is now ready, with all links set up.
##	    You should be able to do all normal sccs functions including
##	    'smerge' within that hierarchy--just make sure that any 
##	    SCCS directories which will contain modified or new files 
##	    actually point to your local SCCS_DIRECTORIES.
##
##	5)  Build your modified kernel
##		cd `arch -k`/conf	
##		/etc/config -g GENERIC
##		cd ../GENERIC
##		make
##----------------------END EXAMPLE-----------------------------------
##		
## Please send bugs/RFE's to Janet Timbol (janet@firenze)
##
######################################################################

get_yn='(while read yn
do
    case "$yn" in
    [Yy]* ) exit 0;;
    [Nn]* ) exit 1;;
        * ) echo -n "Please answer y or n:  ";;
esac
done)'
top=`pwd`
mk_dirs=
ln_files=

echo -n "DIR=$top? (y/n)   "
eval "$get_yn"
[ "$?" = 1 ] && exit 1

#dirname=/usr/src/sys

echo -n "Link to which directory? (eg: /usr/src/sys)?  "
read dirname

[ ! -d $dirname ] && echo "$dirname not found" && exit 1

echo -n '
	1)	Create unbundled directory tree
	2)	Link files recursively under unbundled tree
	3)	Do #1 and #2

	Do what?  '
read choice

case $choice in
	[1Mm]* ) mk_dirs=1;;
	[2Ll]* ) ln_files=1;;
        [3Dd]* ) mk_dirs=1; ln_files=1;;
	    *  ) echo "$choice not valid!"  && exit 1;;
esac

cd $dirname 
echo "Output file is /tmp/dir$$.  Please wait..."
find . -type d -print >/tmp/dir$$
cd $top

if [ "$mk_dirs" ] ; then
	for eachdir in `cat /tmp/dir$$`
	do
		echo "Creating $eachdir"
		mkdir $eachdir
	done
fi

#
#Create generic links to SCCS/s.* files and 'gotten' sources,
#if they do not exist in feature source tree
#
#Create links made by  '/usr/src/makelinks'.  Links should point to
#generic source links, unless they have been modified in the
#feature tree.
#
if [ "$ln_files" ] ; then
	for eachdir in `cat /tmp/dir$$ | sed -e '/^.\//s///g'`
	do
	   echo "Linking files in $eachdir"
	   for eachfile in `ls $dirname/$eachdir`
	   do
		while :
		do
		  linkit=
		  linksrc=
		  if [ "$eachfile" = "SCCS" ]; then
		 	if [ ! -h $top/$eachdir/SCCS ] ; then
			  linkit=1
			else
			  linksrc=1
			fi
			break
		  fi
		  if [ -f $dirname/$eachdir/SCCS/s.$eachfile ]; then
			linkit=1   #assumes 'sccs get' done for feature src
			break
		  fi
		  if [ -h $dirname/$eachdir/$eachfile ] ; then
			linkto=`ls -l $dirname/$eachdir/$eachfile | \
			    awk '{print $NF}'`
		        cd $top/$eachdir
			if [ -d $linkto -o -f $linkto ] ; then
			   ln -s $linkto $eachfile 
			else
			   linkit=1	
			fi
			cd $top
		  fi
		  break
			
		 done
		if [ "$linksrc" ] ; then
	   		for eachsrc in `ls $dirname/$eachdir/SCCS`
			do
				if [ ! -f $eachdir/SCCS/$eachsrc ]; then
				   ln -s $dirname/$eachdir/SCCS/$eachsrc \
					$eachdir/SCCS
				fi
			done	
		fi
		[ "$linkit" ] || continue
		if [ $eachdir = '.' ] ; then
			ln -s $dirname/$eachfile $eachdir
		else
			ln -s $dirname/$eachdir/$eachfile $eachdir
		fi
	   done
	done
fi

echo "DONE."

rm -f /tmp/dir$$
cd $top
