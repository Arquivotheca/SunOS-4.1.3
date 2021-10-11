#! /bin/sh
# %Z%%M% %I% %E% SMI
#
#       Copyright (c) 1989-1990, Sun Microsystems, Inc.  All Rights Reserved.
#       Sun considers its source code as an unpublished, proprietary
#       trade secret, and it is available only under strict license
#       provisions.  This copyright notice is placed here only to protect
#       Sun in the event the source is deemed a published work.  Dissassembly,
#       decompilation, or other means of reducing the object code to human
#       readable form is prohibited by the license agreement under which
#       this code is provided to the user or company in possession of this
#       copy.
#
#       RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
#       Government is subject to restrictions as set forth in subparagraph
#       (c)(1)(ii) of the Rights in Technical Data and Computer Software
#       clause at DFARS 252.227-7013 (Oct. 1988) and FAR 52.227-19 (c) 
#	(June 1987). Sun Microsystems, Inc., 2550 Garcia Avenue, Mountain
#	View, California 94043.
#
#
#	This is the cdrom install script for the US Encryption kit.
#
#
# variables
# 	a. TAG file, the original library will be saved in libXXXX.$TAG
# 	b. While installing files, $LOCK file will be created at
#		/usr/lib/$LOCK - During installation of server
#		/export/exec/{sun3,sun4}/lib/$LOCK - During installation of 
#			 the client.
# 		The contents of this file will be the time of the execution.
#
TAG="INTER"
LOCK=INSTALL_UNBUNDLED_LOCK
DISK_AVAIL=`df /usr | sed -n '2p' | awk '{print $4 }'`
if [ -d /export/exec ]; then
  DISK_AVAIL_OTHER=`df /export/exec | sed -n '2p' | awk '{print $4 }'`
fi
DISK_REQ=2000
DEF_RELEASE="4.1.1"
PRODUCT="sunos"

#
#	Check and create the lock file
#
create_lockfile()
{
	LOCKFILE=$INSTALL_PATH/lib/${LOCK}
      	if [ -f ${LOCKFILE} ]; then
        echo
        echo "The previous installation of the server terminated abnormally."
        echo "Do you wish to abort the current installation and check ?"
        echo "If you abort, it will be necessary to restart the"
        echo "installation from the beginning."
        echo "However, if you wish to continue, the installation"
        echo "will resume, assuming no crucial problems being present."
        echo -n "Please enter 'y' to abort or 'n' to continue: "
        while true
        do
          read ANS
          echo "${ANS}" >> $LOGFILE
          case ${ANS} in
          "y" )
            echo "Installation terminating"
            echo "Please check server's library"
            echo "and remove ${LOCKFILE} file before you"
            echo "restarting the installation."
            exit 2;
            break;;
          "n" )
            echo "Continuing the installation."
            break;;
          * )
            echo "Please enter y or n: "
          esac
        done
      	fi
      	rm -f ${LOCKFILE}
      	date > ${LOCKFILE}
}

disk_msg() 
{
	echo 
      	echo "A minimum of ${DISK_REQ} kbytes in the $1 partition is"
      	echo "required to install the ${DEF_RELEASE} US Encryption kit."
      	echo "This space requirement, for the most part, is temporary."
      	echo "The space will primarily be used to backup files"
      	echo "during the course of the installation.  Following"
      	echo "completion, the backup copies will be removed.  If you desire"
      	echo "to install the US Encryption kit, it is suggested that"
      	echo "you move non-essential files out of the $1 partition,"
      	echo "restart this install from the beginning, and restore the"
      	echo "files after the Encryption kit installation is complete."
      	echo "$1 currently has $2 kbytes available."
}

#
# 	Multi-release support, find all services of own architecture
#
find_dirs()
{
	echo "Checking for system release compatibility with installation media."
	cd $BINDIR
	FILES=`find . -type f -print | \
		egrep -v '(./share|./5lib|bin/yppasswd|lib/libc_p.a.us)'` 
	MAJOR_MINOR=`echo $FILES | sed 's/.*libc.so.//' | sed 's/.us.*$//'`
	DIR=""
	for dir in /export/exec/${THISARCH}.${PRODUCT}.${DEF_RELEASE} $1
	do
		if [ -d ${dir} -a ! -h ${dir} ]; then
    			if [ -f ${dir}/lib/libc.so.${MAJOR_MINOR} ]; then
      				DIR="${DIR} ${dir}"
    			fi
  		fi
	done
}

install_usr()
{
        echo
        echo "Installing essential root and /usr files."
        echo
        #
        # Installing crypt only files
        #
        EXTRACT_LIST="bin/crypt bin/des"
	cd ${BINDIR} 
	tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
	if [ "$?" -ne 0 ]; then
		echo "Error in extracting software, exiting"
		exit 1
	fi

        #
        # Save back ups for /usr/lib/libc.*
        #
        EXTRACT_LIST=""
        REMOVE_LIST=""
        cd ${INSTALL_PATH}/lib
        for lib in libc.*; do
          	if [ -f ${TAG}.${lib} ]; then
            		echo "`pwd`/${lib} has been previously installed."
          	else
            		ln ${lib} ${TAG}.${lib}
            		echo "${lib} has been backed up in ${TAG}.${lib}"
          	fi
          	EXTRACT_LIST="${EXTRACT_LIST} lib/${lib}.us"
          	REMOVE_LIST="${REMOVE_LIST} lib/${TAG}.${lib}"
        done
  
        #
        # Now install localized libraries.
        # The name of installed files are /usr/lib/libc*.us
        #
        cd ${BINDIR}
	tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
	if [ "$?" -ne 0 ]; then
		echo "Error in extracting software, exiting"
		exit 1
	fi
  
        #
        # Now, rename newly installed files to its standard names.
        #
        (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
        cd ${INSTALL_PATH}/lib
        for lib in libc*.us; do
          	b=`basename $lib .us`
          	mv $lib $b
        done)
	cd ${INSTALL_PATH}
        rm ${REMOVE_LIST}
  
        #
        # Installing existing files
        #
        echo "Assembling extract list."
        EXTRACT_LIST=""
        REMOVE_LIST=""

	# Get a list of files in the usr package
	#
	cd $BINDIR
	FILES=`find . -type f -print | egrep -v '(./share|./5lib|bin/yppasswd|lib/libc_p.a.us|bin/crypt|bin/des)'`
        cd ${INSTALL_PATH}
        for f in ${FILES}; do
          	if [ -f ${f} ]; then
            		EXTRACT_LIST="${EXTRACT_LIST} ${f}"
            		ds=`dirname ${f}`
            		bs=`basename ${f} .us`
            		ln $f ${ds}/${TAG}.${bs}
            		REMOVE_LIST="${REMOVE_LIST} ${ds}/${TAG}.${bs}"
          	fi
        done
        cd ${BINDIR}
	tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
	if [ "$?" -ne 0 ]; then
		echo "Error in extracting software, exiting"
		exit 1
	fi
	cd ${INSTALL_PATH}
        rm ${REMOVE_LIST}
}
  
install_networking()
{
        #
        # If /usr/bin/yppasswd exists, copy it from the tape also.
        #
        if [ -f ${INSTALL_PATH}/bin/yppasswd ]; then
          	echo
          	echo "Installing networking files."
          	echo
          	ln ${INSTALL_PATH}/bin/yppasswd \
			${INSTALL_PATH}/bin/${TAG}.yppasswd
          	EXTRACT_LIST="bin/yppasswd"
          	REMOVE_LIST="bin/${TAG}.yppasswd"
        	cd ${BINDIR}
		tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
		if [ "$?" -ne 0 ]; then
			echo "Error in extracting software, exiting"
			exit 1
		fi
          	cd ${INSTALL_PATH}
          	rm ${REMOVE_LIST}
        fi
}
  
install_debugging()
{
        #
        # If /usr/lib/libc_p.a exists, update debugging also.
        #
        if [ -f ${INSTALL_PATH}/lib/libc_p.a ]; then
          	echo
          	echo "Installing debugging files."
          	echo
          	cd ${INSTALL_PATH}/lib
          	if [ -f ${TAG}.libc_p.a ]; then
            		echo "`pwd`/libc_p.a has been previously installed."
          	else
            		ln libc_p.a ${TAG}.libc_p.a
            		echo "libc_p.a has been backed up in ${TAG}.libc_p.a"
          	fi
          	EXTRACT_LIST="lib/libc_p.a.us"
          	REMOVE_LIST="lib/${TAG}.libc_p.a"
        	cd ${BINDIR}
		tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
		if [ "$?" -ne 0 ]; then
			echo "Error in extracting software, exiting"
			exit 1
		fi
          	#
          	# install libc_p.a and cleaning up
          	#
          	(trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
          	cd ${INSTALL_PATH}/lib
          	for lib in libc*.us
          	do
            		b=`basename ${lib} .us`
            		mv ${lib} ${b}
          	done)
          	cd ${INSTALL_PATH}
          	rm ${REMOVE_LIST}
        fi
}
  
install_system_v()
{
        #
        # If /usr/5lib exists, save and copy files as it was done for
        # /usr/lib.
        #
        if [ -d ${INSTALL_PATH}/5lib ]; then
		echo
          	echo "Installing System V files."
          	echo
		EXTRACT_LIST=""
		REMOVE_LIST=""
          	cd ${INSTALL_PATH}/5lib
          	for lib in libc.* libc_p.*
          	do
            	    if [ -f ${TAG}.${lib} ]; then
              		echo  "`pwd`/${lib} has been previously installed."
            	    else
              		ln ${lib} ${TAG}.${lib}
              		echo "5lib: ${lib} has been backed up in ${TAG}.${lib}"
            	    fi
            	    EXTRACT_LIST="${EXTRACT_LIST} 5lib/${lib}.us"
            	    REMOVE_LIST="${REMOVE_LIST} 5lib/${TAG}.${lib}"
          	done
		[ ! "$EXTRACT_LIST" ] && return
        	cd ${BINDIR}
		tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
		if [ "$?" -ne 0 ]; then
			echo "Error in extracting software, exiting"
			exit 1
		fi
          	(trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
          	cd ${INSTALL_PATH}/5lib
          	for lib in libc*.us
          	do
            		b=`basename ${lib} .us`
            		mv ${lib} ${b}
          	done)
		cd ${INSTALL_PATH}
          	rm ${REMOVE_LIST}
        fi
}
    
install_manual()
{
        #
        # If /usr/share/man/man1 exists, update manuals also.
        #
        if [ -d ${INSTALL_PATH}/share/man/man1 ]; then
        	echo
          	echo "Installing manual files."
          	echo
          	EXTRACT_LIST="share"
        	cd ${BINDIR}
		tar vcf - ${EXTRACT_LIST} | (cd ${INSTALL_PATH}; tar xvpfB -)
		if [ "$?" -ne 0 ]; then
			echo "Error in extracting software, exiting"
			exit 1
		fi
        fi
}

###########################################
#
# Start of main program
#
###########################################

# 
# You must be root
#
if [ "`whoami`x" != "root"x ]; then
   echo "You must be root do install!"
   exit 1
fi

#
# Check for extract_unbundled passed parameters
#
if [ $# -gt 0 ]
then
    for param in $*;
    do
	case "$param" in
	-m*)
		MOUNT_POINT=`expr $param : '-m\(.*\)' '|' "1"`
		if [ $MOUNT_POINT -eq 1 ]; then
			echo
			echo "Invalid mount point, exiting"
			exit 1
		fi ;;
  	-p*) 
		PRODUCT_DIR=`expr $param : '-p\(.*\)' '|' "1"`
		if [ $PRODUCT_DIR -eq 1 ]; then
			echo
			echo "Invalid product name, exiting"
			exit 1
		fi ;;
  	esac
    done
fi

#
# Mount point and product name must be set
#
if [ ! "$MOUNT_POINT" ]; then
	echo "Mount point not set, exiting"
	exit 1
fi
if [ ! "$PRODUCT_DIR" ]; then
	echo "Product name not set, exiting"
	exit 1
fi

#
# Find out the architecture of the host machine, and
# set shell variable OTHERARCH.
#
THISARCH=`/bin/arch`
case ${THISARCH} in
sun3)
  	OTHERARCH="sun4" ;;
sun4)
  	OTHERARCH="sun3" ;;
*)
  	echo
  	echo "Invalid architecture, exiting."
  	exit 1
esac

BINDIR=$MOUNT_POINT/$PRODUCT_DIR/$THISARCH

echo
echo "Installing the ${DEF_RELEASE} US Encryption kit for the ${THISARCH} architecture."

#
# First let's do it for the own architectures.
# Loop through all potential install directories because of matching
# libc major and minor numbers.
#
find_dirs /usr
for INSTALL_PATH in ${DIR}
do
  echo
  echo "Do you wish to install the ${DEF_RELEASE} US Encryption kit under"
  echo "${INSTALL_PATH} for the ${THISARCH} architecture?"
  echo -n "Please enter y to continue or n to skip this directory: "
  while true 
  do
    	read ANS
    	echo "${ANS}" >> $LOGFILE
    	case ${ANS} in
    	"y" )
  		#
  		# Check for available disk space in /usr or /export/exec
  		#
  		if [ ${INSTALL_PATH} = "/usr" ]; then
			if [ ${DISK_AVAIL} -lt ${DISK_REQ} ]; then
				disk_msg /usr $DISK_AVAIL
      				exit 1
    		    	fi
  		else
    			if [ ${DISK_AVAIL_OTHER} -lt ${DISK_REQ} ]; then
				disk_msg /export/exec $DISK_AVAIL_OTHER
				exit 1
			fi
    		fi
		create_lockfile
		install_usr
		install_networking
		install_debugging
		install_system_v
		install_manual
      		rm -f ${LOCKFILE}
      		break;;
    	"n")
      		echo "Skipping the installation of the ${DEF_RELEASE} US Encryption"
      		echo "kit for the ${THISARCH} architecture under ${INSTALL_PATH}."
      		break;;
    	*)
      		echo -n  "Please enter y or n: ";;
    	esac
  done
done
#
# END OF SERVER PART
#
#
# Second, let's do it for clients.
#
ANS=""
while true
do
  	case ${ANS} in
    	"y")
      		break ;;
    	"n")
      		/usr/etc/ldconfig
      		echo
      		echo "${DEF_RELEASE} US Encryption kit installation completed"
      		echo
      		echo "Please check the log file, /var/tmp/${DEF_RELEASE}_US_Encryption.log"
      		exit 0 ;;
    	*)
		echo
      		echo "Do you wish to check for client support of other architectures?"
      		echo -n "Enter y to continue n to quit: "
	  	read ANS ;;
  	esac
done

THISARCH=$OTHERARCH
BINDIR=$MOUNT_POINT/$PRODUCT_DIR/$THISARCH

#
# Loop through all potential install directories because of matching
# libc major and minor numbers.
#
find_dirs
for INSTALL_PATH in ${DIR}
do
  echo
  echo "Do you wish to install the ${DEF_RELEASE} US Encryption kit under"
  echo "${INSTALL_PATH} for the ${THISARCH} architecture?"
  echo -n "Please enter y to continue or n to skip this directory: "
  while true
  do
    	#
    	# Ask the user if he wants to continue or not.
   	#
    	read ANS
    	echo "${ANS}" >> $LOGFILE
    	case ${ANS} in
    	"y")
      		#
      		# Check for available disk space in /export/exec, minimum 2000 kbytes
      		#
      		if [ ${DISK_AVAIL_OTHER} -lt ${DISK_REQ} ]; then
			disk_msg /export/exec $DISK_AVAIL_OTHER
        		exit 1
      		fi
      		#
      		# Check and create the lock file
      		#
		create_lockfile
		install_usr
		install_networking
		install_debugging
		install_system_v
		install_manual
      		rm -f ${LOCKFILE}
      		break;;
    	"n")
      		echo "Skipping the installation of the ${DEF_RELEASE} US Encryption"
      		echo "kit for the ${THISARCH} architecture."
      		break;;
    	*)
      		echo -n "Please enter y or n: ";;
    	esac
  done
done

#
# Installation almost complete.
# Notify the ld that libraries got changed.
#
/usr/etc/ldconfig
echo
echo "${DEF_RELEASE} US Encryption kit installation completed"
echo
echo "Please check the log file, /var/tmp/${DEF_RELEASE}_US_Encryption.log"
exit 0
