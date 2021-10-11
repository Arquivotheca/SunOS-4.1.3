#!/bin/sh
#
#       %W% %G% SMI
#
# showrev - tells things about this system for user reporting a bug
#
# use absolute paths - the system/PATH may be hosed!
#
check_dir_to_dir_symlink()
{
thispath=$real_path
current_dir=`/usr/bin/dirname $real_path`
perm=`/usr/bin/ls -ld $real_path|(read perm nlink owner sz dm dd\
      dt dir pt link;/usr/bin/echo $perm)`
link=`/usr/bin/ls -ld $real_path|(read perm nlink owner sz dm dd\
      dt dir pt link;/usr/bin/echo $link)`
case $perm in
  l*) symlink=yes
      dir_to_dir_link=yes
      case $link in
        /*) real_path=$link;; 
         *) real_path=$current_dir/${link} # relative to current_dir
            case $real_path in
              //*) real_path=`/usr/bin/echo $real_path|/usr/bin/tr -s '/' '/'`;;
            esac;;
      esac 
      /usr/bin/echo "     \"$thispath\" Symbolic link to $real_path"
      keep_tracing_the_path=yes
      for p in $all_paths
      do
	if [ $real_path = "$p" ]; then
		keep_tracing_the_path=no
	fi
      done
      if [ $keep_tracing_the_path = "yes" ]; then
		check_dir_to_dir_symlink
      fi;;
  *) dir_to_dir_link=no
     check_file_to_file_symlink
esac
}

check_file_to_file_symlink()
{
thispath=$real_path
current_dir=$real_path
perm=`/usr/bin/ls -ld $real_path/$file|(read perm nlink owner sz dm dd\
      dt dir pt link;/usr/bin/echo $perm)`
link=`/usr/bin/ls -ld $real_path/$file|(read perm nlink owner sz dm dd\
      dt dir pt link;/usr/bin/echo $link)`
case $perm in
  l*) symlink=yes
      case $link in
        /*) real_path=`/usr/bin/dirname $link`
	    file=`/usr/bin/basename $link`
            check_dir_to_dir_symlink;;
         *) real_path=`/usr/bin/dirname $current_dir/$link` # relative to current_\dir
	    file=`/usr/bin/basename $link`
            case $real_path in
              //*) real_path=`/usr/bin/echo $real_path|/usr/bin/tr -s '/' '/'`;;
            esac;;
      esac
      /usr/bin/echo "     \"$thispath/$file\" Symbolic link to $link"
      check_dir_to_dir_symlink;;
   *) all_real_pathnames="${all_real_pathnames} ${real_path}/${file}";;
esac
}

findfile()
{
if [ -f $thispath/$file ]; then
	case $symlink in
		yes) ;;  # doesn't count found yet, need to follow links
		no) /usr/bin/echo
 		    /usr/bin/echo "* -- \"$file\" found in \"$thispath\" --";;
	esac
	real_path=$thispath
	check_dir_to_dir_symlink
	if [ -f $real_path/$file ]; then
		foundinthepath="yes"
	fi
fi
}

general_sys_rev_info()
{
hostname=`/sbin/hostname`
/usr/bin/echo "* Hostname: \"${hostname}\""
# check to see if set, check against /etc/hosts
# note: [ 	] contains a space and a tab
egrep "([ 	]${hostname}[ 	])|([ 	]${hostname}\$)" /etc/hosts > /dev/null
if [ $? -ne 0 ]; then
	/usr/bin/echo  "ERROR: the hostname doesn't match with /etc/hosts"
fi
hostid=`hostid`
/usr/bin/echo "* Hostid: \"${hostid}\""
karch=`/usr/bin/arch -k`
/usr/bin/echo "* Kernel Arch: \"${karch}\""
aarch=`/usr/bin/arch`
/usr/bin/echo "* Application Arch: \"${aarch}\""
kline=`/usr/ucb/strings /vmunix | /usr/bin/grep SunOS | (read junk1 junk2 the_rest; /usr/bin/echo $the_rest)`
/usr/bin/echo "* Kernel Revision:"
/usr/bin/echo "  $kline"
# try to find /usr/sys/conf.common/RELEASE
if [ -f /usr/sys/conf.common/RELEASE ]; then
	/usr/bin/echo "* Release: `/usr/bin/cat /usr/sys/conf.common/RELEASE`"
elif [ -f /etc/install/release ]; then
	/usr/bin/echo "* Release: `/usr/bin/cat /etc/install/release`"
fi 
if [ -f /usr/kvm/showrev.dat ]; then
	/usr/bin/echo "* `/usr/bin/cat /usr/kvm/showrev.dat`" 
fi
}

print_info()
{
/usr/bin/echo "     a) Library information:"
/usr/bin/echo "`/usr/bin/ldd ${real_path_dir}/${real_path_file}`"
/usr/bin/echo "     b) Sccs Id:   `/usr/ucb/what ${real_path_dir}/${real_path_file}`"
/usr/bin/echo -n "     c) Permission: "
w1=`/usr/bin/ls -ldg ${real_path_dir}/${real_path_file} | (read w1 w2 w3 w4 junk; /usr/bin/echo $w1)`
w2=`/usr/bin/ls -ldg ${real_path_dir}/${real_path_file} | (read w1 w2 w3 w4 junk; /usr/bin/echo $w2)`
w3=`/usr/bin/ls -ldg ${real_path_dir}/${real_path_file} | (read w1 w2 w3 w4 junk; /usr/bin/echo $w3)`
w4=`/usr/bin/ls -ldg ${real_path_dir}/${real_path_file} | (read w1 w2 w3 w4 junk; /usr/bin/echo $w4)`
/usr/bin/echo " $w1 $w2 $w3 $w4"
/usr/bin/echo "     d) Sum: `/usr/bin/sum ${real_path_dir}/${real_path_file}`"
/usr/bin/echo
}

command_rev_info()
{
all_paths=`/usr/bin/echo $PATH | /usr/bin/tr ':' ' '`
found=no
found_no=0
file=$1
for thispath in $all_paths
do 
      foundinthepath=no
      symlink=no
      file=$1
      findfile 
      case $foundinthepath in
        yes) found=yes;;
         no) if test $symlink = yes
               then /usr/bin/echo "$file doesn't exist in point to path"
             fi;;
      esac
done
case $found in
	no) /usr/bin/echo " \"$file\" is not found";;
	yes) path_printed=""
	     for path in $all_real_pathnames
	     do
	     if [ "$path" != "$path_printed" ]; then
		/usr/bin/echo
		/usr/bin/echo "     $path"
		real_path_dir=`/usr/bin/dirname $path`
		real_path_file=`/usr/bin/basename $path`
		print_info
		path_printed=$path
	     fi
	     done;;
esac
/usr/bin/echo
if [ "`/usr/bin/echo $LD_LIBRARY_PATH`" = "" ]; then
	/usr/bin/echo "* LD_LIBRARY_PATH not set"
else	/usr/bin/echo "* LD_LIBRARY_PATH is: "
	/usr/bin/echo $LD_LIBRARY_PATH
fi
/usr/bin/echo
/usr/bin/echo "* Path is: "
/usr/bin/echo $PATH
}

patches_info()
{
/usr/bin/echo
/usr/bin/echo "* Patch:"
for i in /etc/install/patch*; do
	if [ -f ${i} ]; then
		/usr/bin/cat ${i} | /usr/bin/sed s/\^/"  "/g
	else
		/usr/bin/echo "  No patch information found."
	fi
done
}

all_sys_info()
{
general_sys_rev_info
if [ "`/usr/bin/echo $OPENWINHOME`" = "" ]; then
	OPENWINHOME=/usr/openwin
	export OPENWINHOME
fi
if [ -f $OPENWINHOME/etc/NeWS/basics.ps ]; then
owrelease=`cat $OPENWINHOME/etc/NeWS/basics.ps | /usr/bin/grep OpenWin | /usr/bin/awk '{print $2}' | /usr/bin/tr '()' ' '`
	/usr/bin/echo "* OpenWindows: OW$owrelease"
else
	/usr/bin/echo "* OpenWindows:"
	/usr/bin/echo "  OpenWindows path information not found."
fi
if [ -f /usr/bin/sunview ]; then
/usr/bin/echo "* SunView:"
ldd /usr/bin/sunview | /usr/bin/awk '{print $2$3}' | /usr/bin/sed s/\.so\./"	:"/g | /usr/bin/sed s/\=\>/"	"/g | /usr/ucb/head -3
ldd /usr/bin/sunview | /usr/bin/awk '{print $2$3}' | /usr/bin/sed s/\.so\./"	:"/g | /usr/bin/sed s/\=\>/"	"/g | /usr/ucb/tail -2 | sed s/:/"	:"/g
fi
patches_info
}

#===================== MAIN ======================
/usr/bin/echo
/usr/bin/echo "***************  showrev version %I%  *****************"

#
flag_set=0
aflag=0
cflag=0
pflag=0
#
# parse options
while getopts apc: c
do
	case $c in
	  p) pflag=1
	     flag_set=1;;
	  c) cflag=1
	     flag_set=1
	     c_arg=$OPTARG;;
	  a) aflag=1
	     flag_set=1;;
	 \?) # XXX may need to add more options
	     /usr/bin/echo "usage: showrev [ -a ] [ -p ] [ [-c] command ]"
	     exit 1
	esac
done
shift `expr $OPTIND - 1`
arg=$1
#
# no flags are set
if [ $flag_set -eq 0 ]; then
	if [ "$arg" = "" ]; then
		general_sys_rev_info
	else	command_rev_info $arg
	fi
fi
#
#
if [ $pflag -eq 1 ]; then
	patches_info
fi
if [ $cflag -eq 1 ]; then
	command_rev_info $c_arg
fi
if [ $aflag -eq 1 ]; then
	all_sys_info
fi
/usr/bin/echo
/usr/bin/echo "*******************************************************"
