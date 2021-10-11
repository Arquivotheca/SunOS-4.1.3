#! /bin/sh
#
# %Z%%M% %I% %E% Copyr 1990 Sun Microsystems, Inc.  
#
# ypinit.sh - set up a populated NIS directory structure on a master server
# or a slave server.
#

# set -xv
if [ -f /etc/security/passwd.adjunct ] ; then
	maps="bootparams ethers.byaddr ethers.byname group.adjunct.byname \
	group.bygid group.byname hosts.byaddr hosts.byname mail.aliases \
	netgroup netgroup.byuser netgroup.byhost networks.byaddr \
	networks.byname passwd.adjunct.byname passwd.byname passwd.byuid \
	protocols.byname protocols.bynumber rpc.bynumber services.byname \
	ypservers"
else
	maps="bootparams ethers.byaddr ethers.byname group.bygid \
	group.byname hosts.byaddr hosts.byname mail.aliases netgroup \
	netgroup.byuser netgroup.byhost networks.byaddr networks.byname \
	passwd.byname passwd.byuid protocols.byname protocols.bynumber \
	rpc.bynumber services.byname ypservers"
fi
yproot_dir=/var/yp
yproot_exe=/usr/etc/yp
hf=/tmp/ypinit.hostlist.$$
XFR=${YPXFR-ypxfr}

masterp=F
slavep=F
host=""
def_dom=""
master=""
got_host_list=F
exit_on_error=F
errors_in_setup=F

PATH=/bin:/usr/bin:/usr/etc:$yproot_exe:$PATH
export PATH 

case $# in
1)	case $1 in
	-m)	masterp=T;;
	*)	echo 'usage:' 1>&2
		echo '	ypinit -m' 1>&2
		echo '	ypinit -s master_server' 1>&2
		echo "" 1>&2
		echo "\
where -m is used to build a master NIS server data base, and -s is used for" 1>&2
		echo "\
a slave data base.  master_server must be an existing reachable NIS server." 1>&2
		exit 1;;
	esac;;

2)	case $1 in
	-s)	slavep=T; master=$2;;
	*)	echo 'usage:' 1>&2
		echo '	ypinit -m' 1>&2
		echo '	ypinit -s master_server' 1>&2
		echo "" 1>&2
		echo "\
where -m is used to build a master NIS server data base, and -s is used for" 1>&2
		echo "\
a slave data base.  master_server must be an existing reachable NIS server." 1>&2
		exit 1;;
	esac;;

*)	echo 'usage:' 1>&2
	echo '	ypinit -m' 1>&2
	echo '	ypinit -s master_server'  1>&2
	echo "" 1>&2
	echo "\
where -m is used to build a master NIS server data base, and -s is used for" 1>&2
	echo "\
a slave data base.  master_server must be an existing reachable NIS server." 1>&2
	exit 1;;
esac


if [ $slavep = T ]
then
	maps=`ypwhich -m | egrep $master$| awk '{ printf("%s ",$1) }' -`
	if [ -z "$maps" ]
	then
		echo "Can't enumerate maps from $master. Please check that it is running." 1>&2
		exit 1
	fi
fi

host=`hostname`

if [ $? -ne 0 ]
then 
	echo "Can't get local host's name.  Please check your path." 1>&2
	exit 1
fi

if [ -z "$host" ]
then
	echo "The local host's name hasn't been set.  Please set it." 1>&2
	exit 1
fi

def_dom=`domainname`

if [ $? -ne 0 ]
then 
	echo "Can't get local host's domain name.  Please check your path." 1>&2
	exit 1
fi

if [ -z "$def_dom" ]
then
	echo "The local host's domain name hasn't been set.  Please set it." 1>&2
	exit 1
fi

domainname $def_dom

if [ $? -ne 0 ]
then 
	echo "\
You have to be the superuser to run this.  Please log in as root." 1>&2
	exit 1
fi

if [ ! -d $yproot_dir -o -f $yproot_dir ]
then
    echo "\
The directory $yproot_dir doesn't exist.  Restore it from the distribution." 1>&2
	exit 1
fi

if [ $slavep = T ]
then
	if [ $host = $master ]
	then
		echo "\
The host specified should be a running master NIS server, not this machine." 1>&2
		exit 1
	fi
fi

if [ "$setup" != "yes" ]; then
	echo "Installing the NIS data base will require that you answer a few questions."
	echo "Questions will all be asked at the beginning of the procedure."
	echo ""
	echo -n "Do you want this procedure to quit on non-fatal errors? [y/n: n]  "
	read doexit
else
	doexit=yes
fi

case $doexit in
y*)	exit_on_error=T;;
Y*)	exit_on_error=T;;
*)	echo "\
OK, please remember to go back and redo manually whatever fails.  If you"
	echo "\
don't, some part of the system (perhaps the NIS itself) won't work.";;
esac

echo ""

for dir in $yproot_dir/$def_dom
do

	if [ -d $dir ]; then
		if [ "$setup" != "yes" ]; then
			echo -n "Can we destroy the existing $dir and its contents? [y/n: n]  "
			read kill_old_dir
		else 
			kill_old_dir=yes
		fi

		case $kill_old_dir in
		y*)	rm -r -f $dir

			if [ $?  -ne 0 ]
			then
			echo "Can't clean up old directory $dir.  Fatal error." 1>&2
				exit 1
			fi;;

		Y*)	rm -r -f $dir

			if [ $?  -ne 0 ]
			then
			echo "Can't clean up old directory $dir.  Fatal error." 1>&2
				exit 1
			fi;;

		*)    echo "OK, please clean it up by hand and start again.  Bye"
			exit 0;;
		esac
	fi

	mkdir $dir

	if [ $?  -ne 0 ]
	then
		echo "Can't make new directory $dir.  Fatal error." 1>&2
		exit 1
	fi

done

if [ $slavep = T ]
then

	echo "\
There will be no further questions. The remainder of the procedure should take"
	echo "a few minutes, to copy the data bases from $master."

	for dom in  $def_dom
	do
		for map in $maps
		do
			echo "Transferring $map..."
			$XFR -h $master -c -d $dom $map

			if [ $?  -ne 0 ]
			then
				errors_in_setup=T

				if [ $exit_on_error = T ]
				then
					exit 1
				fi
			fi
		done
	done

	echo ""
	echo -n "${host}'s NIS data base has been set up"

	if [ $errors_in_setup = T ]
	then
		echo " with errors.  Please remember" 1>&2
		echo "to figure out what went wrong, and fix it." 1>&2
	else
		echo " without any errors."
	fi

	echo ""
	echo "\
At this point, make sure that /etc/passwd, /etc/hosts, /etc/networks,"
	echo "\
/etc/group, /etc/protocols, /etc/services/, /etc/rpc and /etc/netgroup have"
	echo "\
been edited so that when the NIS is activated, the data bases you"
	echo "\
have just created will be used, instead of the /etc ASCII files."

	exit 0
else

	rm -f $yproot_dir/*.time

	while [ $got_host_list = F ]; do
		echo $host >$hf
		if [ "$setup" != "yes" ]; then
			echo ""
			echo "\
	At this point, we have to construct a list of the hosts which will run NIS"
			echo "\
	servers.  $host is in the list of NIS server hosts.  Please continue to add"
			echo "\
	the names for the other hosts, one per line.  When you are done with the"
			echo "list, type a <control D>."
			echo "	next host to add:  $host"
			echo -n "	next host to add:  "

			while read h
			do
				echo -n "	next host to add:  "
				echo $h >>$hf
			done

			echo ""
			echo "The current list of NIS servers looks like this:"
			echo ""

			cat $hf
			echo ""
			echo -n "Is this correct?  [y/n: y]  "
			read hlist_ok

			case $hlist_ok in
			n*)	got_host_list=F
				echo "Let's try the whole thing again...";;
			N*)	got_host_list=F
				echo "Let's try the whole thing again...";;
			*)	got_host_list=T;;
			esac
		else 
			got_host_list=T
		fi
	done

	echo "\
There will be no further questions. The remainder of the procedure should take"
	echo "5 to 10 minutes."

	echo "Building $yproot_dir/$def_dom/ypservers..."
	cat $hf | awk '{print $$0, $$0}' \
	    | makedbm - $yproot_dir/$def_dom/ypservers

	if [ $?  -ne 0 ]
	then
		echo "\
Couldn't build yp data base $yproot_dir/ypservers."
		errors_in_setup=T

		if [ $exit_on_error = T ]
		then
			exit 1
		fi
	fi

	rm $hf

	in_pwd=`pwd`
	cd $yproot_dir
	echo -n "Running "
	echo -n $yproot_dir
	echo "/Makefile..."
	make NOPUSH=1 

	if [ $?  -ne 0 ]
	then
		echo "\
Error running Makefile." 1>&2
		errors_in_setup=T
		
		if [ $exit_on_error = T ]
		then
			exit 1
		fi
	fi

	cd $in_pwd
	echo ""
	echo -n "\
$host has been set up as a NIS master server"

	if [ $errors_in_setup = T ]
	then
		echo " with errors.  Please remember" 1>&2
		echo "to figure out what went wrong, and fix it." 1>&2
	else
		echo " without any errors."
	fi

	echo ""
	echo "\
If there are running slave NIS servers, run yppush now for any data bases"
	echo "\
which have been changed.  If there are no running slaves, run ypinit on"
	echo "\
those hosts which are to be slave servers."

fi
