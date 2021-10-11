#! /bin/sh
# %Z%%M% %I% %E% SMI
#
#       Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.
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
#       clause at DFARS 52.227-7013 and in similar clauses in the FAR and
#       NASA FAR Supplement.
#
#
#	This is the install script for the US localization package.
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
MESSAGE1="To install this update remotely your hostname should exist"
TAG="INTER"
LOCK=INSTALL_UNBUNDLED_LOCK
DISK_AVAIL=`df /usr | sed -n '2p' | awk '{print $4 }'`
if [ -d /export/exec ]; then
  DISK_AVAIL_OTHER=`df /export/exec | sed -n '2p' | awk '{print $4 }'`
fi
DISK_REQ=2000
DEF_RELEASE="4.1"
PRODUCT="sunos"
EXCLUDE="bin/crypt bin/des bin/yppasswd lib/libc_p.a 5lib share"
#
# Check if the user is root or not, and if not exit.
#
echo
echo "You are about to install the ${DEF_RELEASE} US Encryption package."
echo
echo "A log file, /var/tmp/${DEF_RELEASE}_US_Encryption.log, is being created"
echo "to record the events of this installation."
echo
if [ `/usr/ucb/whoami` != "root" ]; then
  echo "Must be superuser to install this package, exiting now."
  echo "Become root and restart."
  exit 0
fi

#
# Check for extract_unbundled passed parameters
#
REMOTE=""
MEDIALOC="local"
FE=""
if [ "${1}" != "" ] ; then
  FE="E"
  FLAG=`expr substr ${1} 1 2`
  case "${FLAG}" in
  "-d")
    MEDIA=`expr substr ${1} 3 4`
    break ;;
  "-r")
    MEDIAHOST=`expr substr ${1} 3 99`
    REMOTE="rsh ${MEDIAHOST} -n"
    MEDIALOC=${MEDIAHOST}
    break ;;
  esac
fi
if [ "${2}" != "" ] ; then
  FE="E"
  FLAG=`expr substr ${2} 1 2`
  case "${FLAG}" in
  "-d")
    MEDIA=`expr substr ${2} 3 4`
    break ;;
  "-r")
    MEDIAHOST=`expr substr ${2} 3 99`
    REMOTE="rsh ${MEDIAHOST} -n"
    MEDIALOC=${MEDIAHOST}
    break ;;
  esac
fi
#
# Find out where(local or remote) the tape is.
#
if [ "${FE}" != "E" ] ; then
  MEDIALOC="UNKNOWN"
  while [ ${MEDIALOC} = "UNKNOWN" ] ; do
    echo
    echo -n "Where is the tape drive located? (local | remote): "
    read DRIVE;
    echo "${DRIVE}" >> $LOGFILE
    case "${DRIVE}" in
      "local" )
        REMOTE=""
        MEDIALOC="local"
        break ;;
      "remote" )
        echo
        echo -n "Enter host name of the remote tape drive? "
        read MEDIAHOST;
        echo "${MEDIAHOST}" >> $LOGFILE
        rsh ${MEDIAHOST} ls /dev > /dev/null 2>&1
        if [ $? = 0 ] ; then
          REMOTE="rsh ${MEDIAHOST} -n"
          MEDIALOC=${MEDIAHOST}
        else
          echo
          echo "Host ${MEDIAHOST} not found."
          echo  ${MESSAGE1}
          echo "in ${MEDIAHOST}'s '/.rhosts' file."
          echo -n "Do you wish to retry? (y/n) "
          #
          # Ask the user if he wants to continue or not
          #
          while true
          do
            read ANS
            echo "${ANS}" >> $LOGFILE
            case "${ANS}" in 
            "n" )
              echo "Exiting from install script, bye."
              exit 0
              break;;
            "y" )
              break;;
            * )
              echo "Do you wish to retry ?"
              echo -n "Please enter y or n "
            esac
          done
          MEDIALOC="UNKNOWN"
        fi ;;
      * )
        echo "Invalid tape drive location \"${DRIVE}\"." ;;
      esac
  done
fi

#
# Find out the type of the tape drive
#
while true
do
  if [ "${FE}" != "E" ] ; then
    echo
    echo -n 'Enter drive type (st* | mt* | fd*): '
    read MEDIA
    echo "${MEDIA}" >> $LOGFILE
  fi
  case "${MEDIA}" in
  st? )
    BS=126
    BTR="tar"
    BTROPTION="xvpfb"
    MEDIATYPE=tape
    break;;
  mt? )
    BS=20
    BTR="tar"
    BTROPTION="xvpfb"
    MEDIATYPE=tape
    break;;
  fd?c | fd? )
    if [ -n "${REMOTE}" ]; then
      echo "Remote option cannot be used with diskette installations."
      exit 1
    fi
    BS=18
    BTR="bar"
    BTROPTION="xvpfbTZ"
    MEDIATYPE=floppy
    break;;
  * )
    echo "Invalid media ${MEDIA}, try again.";;
  esac
done
if [ ${MEDIATYPE} = "tape" ]; then
  ${REMOTE} mt -f /dev/r${MEDIA} rew
fi

#
# Find out the architecture of the host machine, and
# set shell variable OTHERARCH.
#
OWNARCH=`/bin/arch`
case ${OWNARCH} in
sun3)
  MACH="mc68"
  MACH_OTHER="sparc"
  OTHERARCH="sun4"
  START=2;;
sun4)
  MACH="sparc"
  MACH_OTHER="mc68"
  OTHERARCH="sun3"
  START=7;;
*)
  echo
  echo "Invalid architecture, exiting."
  exit 1
esac
echo
echo "Installing the ${DEF_RELEASE} US Encryption package for the ${OWNARCH} architecture."

#
# Multi-release support, find all services of own architecture
#
echo "Checking for system release compatibility with installation media."
if [ ${MEDIATYPE} = "tape" ]; then
  $REMOTE mt -f /dev/nr${MEDIA} asf $START
else
  echo -n "Insert diskette 1 (if not already in disk drive) and press return:"
  read t
fi
if [ "$REMOTE" = "" ]; then
  FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s\n", $8}'`
else
  FILES=`($REMOTE dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s\n", $8}'`
fi
MAJOR_MINOR=`echo $FILES | sed 's/.*libc.so.//' | sed 's/.us.*$//'`
DIR=""
for dir in /export/exec/${OWNARCH}.${PRODUCT}.* /usr
do
  if [ -d ${dir} -a ! -h ${dir} ]; then
    if [ -f ${dir}/lib/libc.so.${MAJOR_MINOR} ]; then
      DIR="${DIR} ${dir}"
    fi
  fi
done

#
# First let's do it for the own architectures.
# Loop through all potential install directories because of matching
# libc major and minor numbers.
#
for INSTALL_PATH in ${DIR}
do
  #
  # Check for available disk space in /usr or /export/exec, minimum of 2000 kbytes
  #
  if [ ${INSTALL_PATH} = "/usr" ]; then
    if [ ${DISK_AVAIL} -lt ${DISK_REQ} ]; then
      echo "A minimum of ${DISK_REQ} kbytes in the /usr partition is"
      echo "required to install the ${DEF_RELEASE} US Encryption package.  This"
      echo "space requirement, for the most part, is temporary."
      echo "The space will primarily be used to backup files"
      echo "during the course of the installation.  Following"
      echo "completion the backup copies will be removed.  If you desire"
      echo "to install the US localization package, it is suggested that"
      echo "you move non-essential files out of the /usr partition,"
      echo "restart this install from the beginning, and restore the"
      echo "files following the localization package installation's completion."
      echo "/usr currently has ${DISK_AVAIL} kbytes available."
      exit 1
    fi
  else
    if [ ${DISK_AVAIL_OTHER} -lt ${DISK_REQ} ]; then
      echo "A minimum of ${DISK_REQ} kbytes in the /usr partition is"
      echo "required to install the ${DEF_RELEASE} US Encryption package.  This"
      echo "space requirement, for the most part, is temporary."
      echo "The space will primarily be used to backup files"
      echo "during the course of the installation.  Following"
      echo "completion the backup copies will be removed.  If you desire"
      echo "to install the US localization package, it is suggested that"
      echo "you move non-essential files out of the /usr partition,"
      echo "restart this install from the beginning, and restore the"
      echo "files following the localization package installation's completion."
      echo "/usr currently has ${DISK_AVAIL} kbytes available."
      exit 1
    fi
  fi
  echo
  echo "Do you wish to install the ${DEF_RELEASE} US Encryption kit under ${INSTALL_PATH}"
  echo "for the ${OWNARCH} architecture?"
  echo -n "Please enter y to continue or n to skip this directory: "
  while true 
  do
    if [ -d ${INSTALL_PATH}/lib ]; then
      read ANS
      echo "${ANS}" >> $LOGFILE
    fi
    case ${ANS} in
    "y" )
      #
      # Check and create the lock file
      #
      LOCKFILE=`pwd`/${LOCK}
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
            echo "and remove `pwd`/${LOCK} file before you"
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

      #
      # Install floppy crypt kit
      #
      if [ ${MEDIATYPE} = "floppy" ]; then
        for i in 1 2 3
        do
          cd ${INSTALL_PATH}
          if [ $i != 1 ]; then
            echo -n "Insert diskette $i and press return: "
            read t
          fi
          if [ $i = 1 ]; then
            cd /tmp
            echo "Checking for machine architecture and floppy architecture match."
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} bin/ed
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} bin/ed
            fi
            file /tmp/bin/ed | grep ${MACH} > /dev/null
            if [ $? = 1 ]; then
              echo
              echo "Floppy architecture does not match install directory architecture."
              echo "Skipping ${OWNARCH} install in ${INSTALL_PATH}"
              echo
              rm -rf /tmp/bin
              break
            fi
            rm -rf /tmp/bin
            cd ${INSTALL_PATH}
            EXTRACT_LIST="bin/crypt bin/des"
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
            fi
          fi
          EXTRACT_LIST=""
          if [ "${REMOTE}" = "" ]; then
            FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
          else
            FILES=`(${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
          fi
          for f in ${FILES}
          do
            dir=`dirname ${f}`
            basefile=`basename ${f} .us`
            if [ -f ${dir}/${basefile} ]; then
              EXTRACT_LIST="${EXTRACT_LIST} ${f}"
            fi
          done
          for f in ${EXTRACT_LIST}
          do
            ds=`dirname ${f}`
            bs=`basename ${f} .us`
            ln ${ds}/${bs} ${ds}/${TAG}.${bs}
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${f}
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${f}
            fi 
            #
            # Now, rename newly installed files to its standard names.
            # 
            if [ `expr substr ${bs} 1 4` = "libc" ]; then
              (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
              cd ${INSTALL_PATH}
              b=`basename ${f}`
              mv ${ds}/${b} ${ds}/${bs}
              cd ${INSTALL_PATH})
            fi
            rm ${ds}/${TAG}.${bs}
          done
          eject
        done
      else
      #
      # Install tape crypt kit
      #
        EXTRACT_LIST="danw "
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH}/lib
        echo
        echo "Installing essential root and /usr files."
        echo
        #
        # Installing crypt only files
        #
        cd ${INSTALL_PATH}
        EXTRACT_LIST="bin/crypt bin/des"
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
        fi
        #
        # Save back ups for /usr/lib/libc.*
        #
        EXTRACT_LIST="danw "
        REMOVE_LIST=""
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH}/lib
        for lib in libc.*
        do
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
        # Now read the tape and install localized libraries.
        # The name of installed files are /usr/lib/libc*.us
        #
        cd ${INSTALL_PATH}
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
        fi
  
        #
        # Now, rename newly installed files to its standard names.
        #
        (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
        cd ${INSTALL_PATH}/lib
        for lib in libc*.us
        do
          b=`basename $lib .us`
          mv $lib $b
        done)
        rm ${REMOVE_LIST}
  
        #
        # Installing existing files
        #
        echo "Assembling extract list."
        EXTRACT_LIST="danw "
        REMOVE_LIST=""
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH}
        if [ "${REMOTE}" = "" ]; then
          FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
        else
          FILES=`(${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
        fi
        for f in ${FILES}
        do
          if [ -f ${f} ]; then
            EXTRACT_LIST="${EXTRACT_LIST} ${f}"
            ds=`dirname ${f}`
            bs=`basename ${f} .us`
            ln $f ${ds}/${TAG}.${bs}
            REMOVE_LIST="${REMOVE_LIST} ${ds}/${TAG}.${bs}"
          fi
        done
        for f in `echo ${EXCLUDE}`
        do
          echo ${f} >> /tmp/exclude
        done
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION}X /dev/r${MEDIA} ${BS} /tmp/exclude ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION}X - ${BS} /tmp/exclude ${EXTRACT_LIST}
        fi
        rm ${REMOVE_LIST} /tmp/exclude
  
        #
        # If /usr/bin/yppasswd exists, copy it from the tape also.
        #
        if [ -f ${INSTALL_PATH}/bin/yppasswd ]; then
          echo
          echo "Installing networking files."
          echo
          ln ${INSTALL_PATH}/bin/yppasswd ${INSTALL_PATH}/bin/${TAG}.yppasswd
          cd ${INSTALL_PATH}
          EXTRACT_LIST="bin/yppasswd"
          REMOVE_LIST="bin/${TAG}.yppasswd"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 1`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          rm ${REMOVE_LIST}
        fi
  
        #
        # If /usr/lib/libc_p.a exists, update debugging also.
        #
        if [ -f ${INSTALL_PATH}/lib/libc_p.a ]; then
          echo
          echo "Installing debugging files."
          echo
          cd /${INSTALL_PATH}/lib
          if [ -f ${TAG}.libc_p.a ]; then
            echo "`pwd`/libc_p.a has been previously installed."
          else
            ln libc_p.a ${TAG}.libc_p.a
            echo "libc_p.a has been backed up in ${TAG}.libc_p.a"
          fi
          cd ${INSTALL_PATH}
          EXTRACT_LIST="lib/libc_p.a.us"
          REMOVE_LIST="lib/${TAG}.libc_p.a"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 2`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          #
          # install libc_p.a and cleaning up
          #
          (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
          cd ${INSTALL_PATH}/lib
          for lib in libc*.us
          do
            b=`basename ${lib} .us`
            mv ${lib} $[b}
          done)
          rm ${REMOVE_LIST}
        fi
  
        #
        # If /usr/5lib exists, save and copy files as it was done for
        # /usr/lib.
        #
        if [ -d ${INSTALL_PATH}/5lib ]; then
          echo
          echo "Installing System V files."
          echo
          EXTRACT_LIST="danw "
          REMOVE_LIST=""
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 3`
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
          cd ${INSTALL_PATH}
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
          cd ${INSTALL_PATH}/5lib
          for lib in libc*.us
          do
            b=`basename ${lib} .us`
            mv ${lib} ${b}
          done)
          rm ${REMOVE_LIST}
        fi
    
        #
        # If /usr/share/man/man1 exists, update manuals also.
        #
        if [ -d ${INSTALL_PATH}/share/man/man1 ]; then
          echo
          echo "Installing manual files."
          echo
          cd ${INSTALL_PATH}
          EXTRACT_LIST="share"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 4`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
        fi
      fi

      #
      # Remove the lock file created
      #
      rm -f ${LOCKFILE}
      break;;
    "n")
      echo "Skipping the installation of the ${DEF_RELEASE} US Encryption"
      echo "package for the ${OWNARCH} architecture under ${INSTALL_PATH}."
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

#
#	Installation for clients
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
      echo "${DEF_RELEASE} US Encryption package installation completed"
      echo
      echo "Please check the log file, /var/tmp/${DEF_RELEASE}_US_Encryption.log"
      exit 0 ;;
    *)
      echo "Do you wish to check for client support of other architectures?"
      echo -n "Enter y to continue n to quit: "
	  read ANS ;;
  esac
done
case ${START} in
  2 )
    START=7 ;;
  7 )
    START=2 ;;
  * )
    #
    # This is an internal error.
    # Should never come here!
    #
    echo "INTERNAL ERROR, exiting..."
    exit 1 ;;
esac

#
# Multi-release support, find all services of other architecture
#
echo "Checking for system release compatiblity with installation media."
if [ ${MEDIATYPE} = "tape" ]; then
  ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
else
  echo -n "Insert diskette 1 (if not already in disk drive) and press return:"
  read t
fi
if [ "${REMOTE}" = "" ]; then
  FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s\n", $8}'`
else
  FILES=`(${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s\n", $8}'`
fi
MAJOR_MINOR=`echo $FILES | sed 's/.*libc.so.//' | sed 's/.us.*$//'`
DIR=""
for dir in /export/exec/${OTHERARCH}.${PRODUCT}.*
do
  if [ -d ${dir} -a ! -h ${dir} ]; then
    if [ -f ${dir}/lib/libc.so.${MAJOR_MINOR} ]; then
      DIR="${DIR} ${dir}"
    fi
  fi
done

#
# Loop through all potential install directories because of matching
# libc major and minor numbers.
#
for INSTALL_PATH_OTHER in ${DIR}
do
  echo
  echo "Do you wish to install the ${DEF_RELEASE} US Encryption kit under ${INSTALL_PATH_OTHER}"
  echo "for the ${OTHERARCH} architecture?"
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
      # Check for available disk space in /export/exec, minimum of 2000 kbytes
      #
      if [ ${DISK_AVAIL_OTHER} -lt ${DISK_REQ} ]; then
        echo "A minimum of ${DISK_REQ} kbytes in the /export/exec partition is"
        echo "required to install the ${DEF_RELEASE} US Encryption package.  This"
        echo "space requirement, for the most part, is temporary."
        echo "The space will primarily be used to backup files"
        echo "during the course of the installation.  Following"
        echo "completion the backup copies will be removed.  If you desire"
        echo "to install the US localization package, it is suggested that"
        echo "you move non-essential files out of the /export/exec partition,"
        echo "restart this install from the beginning, and restore the"
        echo "files following the localization package installation's completion."
        echo "/export/exec currently has ${DISK_AVAIL_OTHER} kbytes available."
        exit 1
      fi
      #
      # Check and create the lock file
      #
      LOCKFILE=`pwd`/${LOCK}
      if [ -f ${LOCKFILE} ]; then
        echo
        echo "The previous installation of the server terminated abnormally."
        echo "Do you wish to abort now and check ?"
        echo "If you abort, it will be necesssary to restart the"
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
            echo "and remove `pwd`/${LOCK} file before you"
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
  
      #
      # Install floppy crypt kit
      #
      if [ ${MEDIATYPE} = "floppy" ]; then
        for i in 1 2 3
        do
          cd ${INSTALL_PATH_OTHER}
          if [ $i != 1 ]; then
            echo -n "Insert diskette $i and press return: "
            read t
          fi
          if [ $i = 1 ]; then
            cd /tmp
            echo "Checking for machine architecture and floppy architecture match."
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} bin/ed
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} bin/ed
            fi
            file /tmp/bin/ed | grep ${MACH} > /dev/null
            if [ $? = 1 ]; then
              echo
              echo "Floppy architecture does not match install directory architecture."
              echo "Skipping ${OWNARCH} install in ${INSTALL_PATH_OTHER}"
              echo
              rm -rf /tmp/bin
              break
            fi
            rm -rf /tmp/bin
            cd ${INSTALL_PATH_OTHER}
            EXTRACT_LIST="bin/crypt bin/des"
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
            fi
          fi
          EXTRACT_LIST=""
          if [ "${REMOTE}" = "" ]; then
            FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
          else
            FILES=`(${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
          fi
          for f in ${FILES}
          do
            dir=`dirname ${f}`
            basefile=`basename ${f} .us`
            if [ -f ${dir}/${basefile} ]; then
              EXTRACT_LIST="${EXTRACT_LIST} ${f}"
            fi
          done
          for f in ${EXTRACT_LIST}
          do
            ds=`dirname ${f}`
            bs=`basename ${f}`
            ln $f ${ds}/${TAG}.${bs}
            if [ "${REMOTE}" = "" ]; then
              ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${f}
            else
              (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${f}
            fi 
            #
            # Now, rename newly installed files to its standard names.
            # 
            if [ `expr substr ${bs} 1 4` = "libc" ]; then
              (trap 'echo "Installing libc libraries, cannot be stopped."' 1 2 3 9 15
              cd ${INSTALL_PATH_OTHER}
              b=`basename ${f}`
              mv ${ds}/${b} ${ds}/${bs}
              cd ${INSTALL_PATH_OTHER})
            fi
            rm ${ds}/${TAG}.${bs}
          done
          eject
        done
      else
      #
      # Install tape crypt kit
      #
        EXTRACT_LIST="danw "
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH_OTHER}/lib
        echo
        echo "Installing essential root and /usr files"
        echo
        #
        # Installing crypt only files
        #
        cd ${INSTALL_PATH_OTHER}
        EXTRACT_LIST="bin/crypt bin/des"
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
        fi
        #
        # Save back ups for /usr/lib/libc.*
        #
        EXTRACT_LIST="danw "
        REMOVE_LIST=""
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH_OTHER}/lib
        for lib in libc.*
        do
          if [ -f ${TAG}.${lib} ]; then
            echo "`pwd`/${lib} has been previously installed."
          else
            ln ${lib} ${TAG}.${lib}
            echo "${lib} has been backed up in ${TAG}.${lib}"
          fi
          EXTRACT_LIST="${EXTRACT_LIST} lib/${lib}.us"
          REMOVE_LIST="${REMOVE_LIST} lib/${TAG}.${lib}"
        done
  
        cd ${INSTALL_PATH_OTHER}
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
        fi
  
        (trap 'echo "Installing libc libraries, cannot be stopped"' 1 2 3 9 15
        cd ${INSTALL_PATH_OTHER}/lib
        for lib in libc*.us
        do
          b=`basename ${lib} .us`
          mv ${lib} ${b}
        done)
        rm ${REMOVE_LIST}
  
        #
        # Installing existing files
        #
        echo "Assembling extract list"
        EXTRACT_LIST="danw "
        REMOVE_LIST=""
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        cd ${INSTALL_PATH_OTHER}
        if [ "${REMOTE}" = "" ]; then
          FILES=`${BTR} tvfb /dev/r${MEDIA} ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
        else
          FILES=`(${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} tvfb - ${BS} | awk 'BEGIN {FS = " "}{printf "%s ", $8}'`
        fi
        for f in ${FILES}
        do
          if [ -f ${f} ]; then
            EXTRACT_LIST="${EXTRACT_LIST} ${f}"
            ds=`dirname ${f}`
            bs=`basename ${f}`
            ln $f ${ds}/${TAG}.${bs}
            REMOVE_LIST="${REMOVE_LIST} ${ds}/${TAG}.${bs}"
          fi
        done
        for f in `echo ${EXCLUDE}`
        do
          echo ${f} >> /tmp/exclude
        done
        ${REMOTE} mt -f /dev/nr${MEDIA} asf ${START}
        if [ "${REMOTE}" = "" ]; then
          ${BTR} ${BTROPTION}X /dev/r${MEDIA} ${BS} /tmp/exclude ${EXTRACT_LIST}
        else
          (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION}X - ${BS} /tmp/exclude ${EXTRACT_LIST}
        fi
        rm ${REMOVE_LIST} /tmp/exclude
  
        #
        # If /export/exec/../yppasswd exists, copy it from the tape also.
        #
        if [ -f ${INSTALL_PATH_OTHER}/bin/yppasswd ]; then
          echo
          echo "Installing networking files"
          echo
          ln ${INSTALL_PATH_OTHER}/bin/yppasswd ${INSTALL_PATH_OTHER}/bin/${TAG}.yppasswd
          cd ${INSTALL_PATH_OTHER}
          EXTRACT_LIST="bin/yppasswd"
          REMOVE_LIST="bin/${TAG}.yppasswd"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 1`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          rm ${REMOVE_LIST}
        fi
  
        #
        # If /export/exec/../lib/libc_p.a exists, update debugging also.
        #
        if [ -f ${INSTALL_PATH_OTHER}/lib/libc_p.a ]; then
          echo
          echo "Installing debugging files"
          echo
          cd ${INSTALL_PATH_OTHER}/lib
          if [ -f ${TAG}.libc_p.a ]; then
            echo "`pwd`/libc_p.a has been previously installed."
          else
            ln libc_p.a ${TAG}.libc_p.a
            echo "libc_p.a has been backed up in ${TAG}.libc_p.a"
          fi
          cd ${INSTALL_PATH_OTHER}
          EXTRACT_LIST="lib/libc_p.a.us"
          REMOVE_LIST="lib/${TAG}.libc_p.a"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 2`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          #
          # install libc_p.a and cleaning up
          #
          (trap 'echo "Installing libc libraries, cannot be stopped"' 1 2 3 9 15
          cd ${INSTALL_PATH_OTHER}/lib
          for lib in libc*.us
          do
            b=`basename ${lib} .us`
            mv ${lib} ${b}
          done)
          rm ${REMOVE_LIST}
        fi
  
        #
        # If /usr/5lib exists, save and copy files as it was done for
        # /usr/lib.
        #
        if [ -d ${INSTALL_PATH_OTHER}/5lib ]; then
          echo
          echo "Installing System V files"
          echo
          EXTRACT_LIST="danw "
          REMOVE_LIST=""
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 3`
          cd ${INSTALL_PATH_OTHER}/5lib
          for lib in libc.* libc_p.*
          do
            if [ -f ${TAG}.${lib} ]; then
              echo "`pwd`/${lib} has been previously installed."
            else
              ln ${lib} ${TAG}.${lib}
              echo 5lib: "${lib} has been backed up in ${TAG}.${lib}"
            fi
            EXTRACT_LIST="${EXTRACT_LIST} 5lib/${lib}.us"
            REMOVE_LIST="${REMOVE_LIST} 5lib/${TAG}.${lib}"
          done
          cd ${INSTALL_PATH_OTHER}
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
          (trap 'echo "Installing libc libraries, cannot be stopped"' 1 2 3 9 15
          cd ${INSTALL_PATH_OTHER}/5lib
          for lib in libc*.us
          do
            b=`basename ${lib} .us`
            mv ${lib} ${b}
          done)
          rm ${REMOVE_LIST}
        fi
  
        #
        # If /usr/share/man/man1 exists, update manuals also.
        #
        if [ -d ${INSTALL_PATH_OTHER}/share/man/man1 ]; then
          echo
          echo "Installing manual files"
          echo
          cd ${INSTALL_PATH_OTHER}
          EXTRACT_LIST="share"
          ${REMOTE} mt -f /dev/nr${MEDIA} asf `expr ${START} + 4`
          if [ "${REMOTE}" = "" ]; then
            ${BTR} ${BTROPTION} /dev/r${MEDIA} ${BS} ${EXTRACT_LIST}
          else
            (${REMOTE} dd if=/dev/r${MEDIA} bs=${BS}b 2> /dev/null) | ${BTR} ${BTROPTION} - ${BS} ${EXTRACT_LIST}
          fi
        fi
      fi

      #
      # Remove the lock file created
      #
      rm -f ${LOCKFILE}
      break;;
    "n")
      echo "Skipping the installation of the ${DEF_RELEASE} US Encryption"
      echo "package for the ${OTHERARCH} architecture."
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
echo "${DEF_RELEASE} US Encryption package installation completed"
echo
echo "Please check the log file, /var/tmp/${DEF_RELEASE}_US_Encryption.log"
exit 0
