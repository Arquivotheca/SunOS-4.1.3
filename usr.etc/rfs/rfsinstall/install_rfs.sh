#! /bin/sh
# 
# install_rfs :   script written to install rfs
#
#	%Z%  %M% %I% %E%
#
#  	Copyright (c) 1987 by Sun Microsystems, Inc.
#

HOME=/; export HOME
PATH=/bin:/usr/bin:/etc:/usr/etc:/usr/ucb:/usr/sys:/usr/rfs:/usr/rfs/install

CMDNAME=$0
RELEASE="4.0"
SYSARCH=`/bin/arch`

echo
echo "	>>>	Sun Microsystems RFS Installation Tool		<<<"
echo "	>>>		    Release $RELEASE				<<<"
#
# Specify system type : standalone or server or diskless or dataless
#
while true; do
        echo
        echo -n "Enter system type ? [standalone | server | diskless | dataless]: "
        read MACHINE;
        case "$MACHINE" in
        "standalone" | "server" )
		ARCHLIST="${SYSARCH}"
		PATHLIST="/"
                break ;;
	"diskless" | "dataless" )
                break ;;
        * )
                echo "${CMDNAME}: invalid machine type \"${MACHINE}\"." ;;
        esac
done
if [ "$MACHINE" = "server" ]; then
       	while true; do
		echo
		case "$SYSARCH" in
		"sun2" )
               		echo -n "Enter additional architecture type ? [ sun3 | sun4 | sun386 | none ]: " ;;
		"sun3" )
                        echo -n "Enter additional architecture type ? [ sun2 | sun4 | sun386 | none ]: " ;;
		"sun4" ) 
                        echo -n "Enter additional architecture type ? [ sun2 | sun3 | sun386 | none ]: " ;;
		"sun386" )
                        echo -n "Enter additional architecture type ? [ sun2 | sun3 | sun4 | none ]: " ;;
		esac
               	read ARCH;
               	case "$ARCH" in
               	"sun2" | "sun3" | "sun4" | "sun386" )
                       	ARCHLIST="${ARCHLIST} ${ARCH}"
                       	while true; do
                               	echo
                               	echo -n "Enter pathname for $ARCH executables ?"
                                read EXEC;
                               	case "$EXEC" in
                               	"" )
                                       	echo "${CMDNAME}: invalid pathname \"${EXEC}\"." ;;
                               	* )
					rm -rf $EXEC
                                       	mkdir $EXEC
                                       	PATHLIST="${PATHLIST} ${EXEC}"
                                       	break ;;
                               	esac
                       	done ;;
               	"none" )
                       	break ;;
               	* )
                       	echo "${CMDNAME}: invalid architecture \"${ARCH}\"." ;;
               	esac
       	done
fi
#
# Specify name of kernel configuration file
#
while true; do
        echo
        echo -n "Enter name of kernel configuration file ? "
        read NAME;
        case "$NAME" in
        "" )
                echo "${CMDNAME}: invalid name \"${NAME}\"." ;;
	* ) 
                break ;;
        esac
done
#
# Specify tape drive type : local or remote
# If remote, specify tape host and ethernet type : ec, ie or le
#
if [ "$MACHINE" = "standalone" -o "$MACHINE" = "server" ]; then
	while true; do
        	echo
        	echo -n "Enter tape drive type ? [local | remote]: "
        	read DRIVE;
        	case "$DRIVE" in
        	"local" )
                	break ;;
        	"remote" )
                	echo;echo -n "Enter host of remote drive ? "
                	read TAPEHOST;
			REMOTE="rsh $TAPEHOST -n"
			break ;;
		* )
			echo "${CMDNAME}: invalid tape drive type \"${DRIVE}\"." ;;
		esac
	done
	#
	# Specify tape type : ar, st, mt or xt
	#
	while true; do 
		echo
		echo -n "Enter tape type ? [ar0 | ar8 | st0 | st8 | mt0 | xt0]: "
		read TAPE;
		case "$TAPE" in
       		"ar0" | "ar8" | "st0" | "st8" )
			BS=126
			cd /dev
			/dev/MAKEDEV ${TAPE} 2> /dev/null
			break ;;
		"mt0" | "xt0" )
			BS=20 
			cd /dev
			rm -f *mt*
			/dev/MAKEDEV ${TAPE} 2> /dev/null
			TAPE="mt0"
			break ;;
       		* )
               		echo "${CMDNAME}: invalid tape type \"${TAPE}\"." ;;
		esac
	done
fi
#
# Prompt user attention last time before starting to build
#
while true; do
        echo;echo -n "Are you ready to start the installation ? [y/n] : "
        read READY;
        case "${READY}" in
        "y" | "yes" )
                break ;;
        "n" | "no" )
                echo;echo "Installation procedure terminates."
                exit 1 ;;
        * )
                echo;echo "Please answer only yes or no."
        esac
done

#
# Installation starts
#
if [ "$MACHINE" = "standalone" -o "$MACHINE" = "server" ]; then
	for CURRENTARCH in ${ARCHLIST}; do
		echo
		echo "Beginning Installation for the ${CURRENTARCH} architecture."
		count=0
        	for i in ${ARCHLIST}; do
                	count=`expr $count + 1`
                	if [ "$i" = "$CURRENTARCH" ]; then
                        	break
                	fi
        	done
        	count1=0
        	for i in ${PATHLIST}; do
                	count1=`expr $count1 + 1`
                	if [ "$count" = "$count1" ]; then
                        	CURRENTPATH=$i
                        	break
                	fi
        	done
		#
		# extract files from release tape
		#
		if [ ! -f /tmp/TOC ]; then
			verify_tapevol_arch ${CURRENTARCH} 1 /dev/nr${TAPE} ${TAPEHOST}
		fi
		TAPE_NUM=`cat /tmp/TOC | awk '$3 == "rfs" {print $1}'`
		NUM=`cat /tmp/TOC | awk '$3 == "rfs" {print $2}'`
		SKIP=`expr $NUM - 1`
		verify_tapevol_arch ${CURRENTARCH} ${TAPE_NUM} /dev/nr${TAPE} ${TAPEHOST}
		if [ "${CURRENTARCH}" = "${SYSARCH}" ]; then
			rm -rf /usr/include/netnpack
                	rm -rf /usr/include/nettli
			rm -rf /usr/include/rfs
		else
			rm -rf ${CURRENTPATH}/include/netnpack
			rm -rf ${CURRENTPATH}/include/nettli
			rm -rf ${CURRENTPATH}/include/rfs
		fi
		cd ${CURRENTPATH}
                extracting /dev/nr${TAPE} ${SKIP} ${BS} "rfs" ${TAPEHOST}
	done
fi
# 
# fix /usr/share/sys/sysarch/conf/NAME
#
echo
cd /usr/share/sys/${SYSARCH}/conf
if [ ! -f $NAME ]; then
        cp GENERIC $NAME
        chmod 755 $NAME
else
	cp $NAME $NAME.save
	rm -rf /usr/share/sys/${SYSARCH}/$NAME
fi
ed - /usr/share/sys/${SYSARCH}/conf/$NAME <<END
/CRYPT/
a
options		RFS
.
w
/snit/
-6
i
pseudo-device	tim64
pseudo-device	tirw64
pseudo-device	np
.
w
/mcpa64/
s/^/#/
w
/mcp0/
s/^/#/
w
/mcpintr/
s/^/#/
w
/mcp1/
s/^/#/
w
/mcpintr/
s/^/#/
w
/mcp2/
s/^/#/
w
/mcpintr/
s/^/#/
w
/mcp3/
s/^/#/
w
/mcpintr/
s/^/#/
w
/xdc0/
s/^/#/
w
/xdc1/
s/^/#/
w
/xdc2/
s/^/#/
w
/xdc3/
s/^/#/
w
/xd0/
s/^/#/
w
/xd1/
s/^/#/
w
/xd2/
s/^/#/
w
/xd3/
s/^/#/
w
/xd4/
s/^/#/
w
/xd5/
s/^/#/
w
/xd6/
s/^/#/
w
/xd7/
s/^/#/
w
/xd8/
s/^/#/
w
/xd9/
s/^/#/
w
/xd10/
s/^/#/
w
/xd11/
s/^/#/
w
/xd12/
s/^/#/
w
/xd13/
s/^/#/
w
/xd14/
s/^/#/
w
/xd15/
s/^/#/
w
/taac0/
s/^/#/
w
/vpc0/
s/^/#/
w
/vpc1/
s/^/#/
w
q
END
#
# fix /usr/share/sys/conf.common/files.cmn
#
cd /usr/share/sys/conf.common
cp /usr/share/sys/conf.common/files.cmn /usr/share/sys/conf.common/files.cmn.save
ed - /usr/share/sys/conf.common/files.cmn <<END
/ti_mod.c/
i
netnpack/npack.c	optional np
.
w
q
END
# 
# fix /usr/share/sys/sun/conf.c
#
cd /usr/share/sys/sun
cp /usr/share/sys/sun/conf.c /usr/share/sys/sun/conf.c.save
ed - /usr/share/sys/sun/conf.c <<END
/seltrue()/
a

/* NPACK */
#include "np.h"
#if NNP > 0
extern struct streamtab pckinfo;
#define nptab	&pckinfo
#else  NNP > 0
#define nptab   0
#endif NNP > 0

.
w
/62/
+3
a
    { 
        nodev,          nodev,          nodev,          nodev,          /*63*/
        nodev,          nodev,          nodev,          0,
        nptab,
    },
.
w
q
END
#
# configure kernel
#
echo 
echo "Configure a new kernel."
echo
cd /usr/share/sys/${SYSARCH}/conf
config $NAME
cd ../$NAME
make
mv /vmunix /vmunix.save
mv vmunix /vmunix

#
# make devices
#
cd /dev
rm -f spx npack
mknod spx c 37 35 2>/dev/null
chmod 666 spx
mknod npack c 37 63 2>/dev/null
chmod 666 npack

${REMOTE} mt -f /dev/nr${TAPE} rew
sync; sync

echo
echo "RFS Installation Completed."
echo "Remember to reboot your system."
exit 0
