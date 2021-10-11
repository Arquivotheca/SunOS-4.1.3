#! /bin/sh
#
#	%Z%%M% %I% %E% SMI
#
#  Copyright (c) 1985 by Sun Microsystems, Inc.
#

HOME=/; export HOME
PATH=/bin:/usr/bin:/etc:/usr/etc:/usr/ucb

CMDNAME=$0

USAGE="
usage: ${CMDNAME} tape type keywords...

where:	
	tape	 = \"ar\" or \"st\" or \"mt\" or \"xt\" 

	type	 = \"tapefull\" or \"tapeless \`server_name'\"

	keywords = \"kernel\" \"network\" \"debuggers\"
		   \"suntools_users\" \"suntools_programmers\"
		   \"src\" \"text_tools\" \"setup\"
		   \"stand_diag\" \"fortran\" \"usr_diag\"
		   \"graphics\" \"pascal\" \"profiled\"
		   \"uucp\" \"system_v\" \"man\" \"demo\"
		   \"games\" \"vtroff\"

"

if [ "`whoami`" != "root" ]; then
	echo "${CMDNAME}: Must be run as root (super-user)."
        exit 1
fi

if [ "$#" -lt 3 ]; then

	echo "${CMDNAME}: Incorrect number of arguments."
	echo "${USAGE}" 
        exit 1

else

	TAPE=${1}; shift
	if [ "$TAPE" = "ar" -o "$TAPE" = "st" ]; then
		BS=200
	elif [ "$TAPE" = "mt" -o "$TAPE" = "xt" ]; then
		BS=20
	else
		echo "${CMDNAME}: Bad argument \"${TAPE}\"."
        	echo "${USAGE}"
        	exit 1
	fi

	TYPE=${1}; shift
	if [ "$TYPE" = "tapeless" ]; then
		SERVER=${1}; shift
		if rsh ${SERVER} -n date >/dev/null; then
        		:
		else
        		echo "${CMDNAME}: Cannot talk to server ${SERVER}."
        		exit 1
		fi
	elif [ "$TYPE" = "tapefull" ]; then
		:
	else
        	echo "${CMDNAME}: Unknown type specified."
		echo "${USAGE}" 
        	exit 1
	fi

	if [ -z "${1}" ]; then
        	echo "${CMDNAME}: No keywords specified."
		echo "${USAGE}" 
        	exit 1
	fi

fi

#
# Format is name="tape number,[file system, tape file number]*".
# The entire "name" must be on the same tape.
#
if [ "$TAPE" = "ar" -o "$TAPE" = "st" ]; then
	kernel="2,usr,3"
	network="2,usr,4"
	debuggers="2,usr,5"
	suntools_users="3,usr,3"
	suntools_programmers="3,usr,4"
	src="3,usr,5"
	text_tools="3,usr,6"
	setup="3,usr,7"
	stand_diag="3,usr,8"
	fortran="3,usr,9"
	usr_diag="3,usr,10"
	graphics="3,usr,11"
	pascal="3,usr,12"
	profiled="3,usr,13"
	uucp="3,usr,14"
	system_v="4,usr,3"
	man="4,usr,4"
	demo="4,usr,5"
	games="4,usr,6"
        vtroff="4,usr,7"
fi
if [ "$TAPE" = "mt" -o "$TAPE" = "xt" ]; then
	kernel="1,usr,10"
	network="1,usr,11"
	debuggers="1,usr,12"
	suntools_users="2,usr,3"
	suntools_programmers="2,usr,4"
	src="2,usr,5"
	text_tools="2,usr,6"
	setup="2,usr,7"
	stand_diag="2,usr,8"
	fortran="2,usr,9"
	usr_diag="2,usr,10"
	graphics="2,usr,11"
	pascal="2,usr,12"
	profiled="2,usr,13"
	uucp="2,usr,14"
	system_v="3,usr,3"
	man="3,usr,4"
	demo="3,usr,5"
	games="3,usr,6"
        vtroff="3,usr,7"
fi
	
TAPENUM=0

KEYWORD=${1}
while [ -n "${KEYWORD}" ]; do
	
	INFO=`eval echo '$'${KEYWORD}`
	if [ -z "${INFO}" ]; then
		echo "${CMDNAME}: Unknown keyword ${KEYWORD}."
		shift; KEYWORD=${1}
		continue
	fi
	NEWTAPENUM=`echo ${INFO} | awk '{FS=","; print $1}'`

	if [ "${NEWTAPENUM}" != "${TAPENUM}" ]; then

		TAPENUM=${NEWTAPENUM}

		REALTAPENUM=${TAPENUM}

		if [ "${TYPE}" = "tapefull" ]; then

			echo -n "${CMDNAME}: Load \"${TAPE}\" release tape #${REALTAPENUM}, then press RETURN."
                	read x
	
			if mt -f /dev/nr${TAPE}0 rew; then
				:
			else
				echo "${CMDNAME}: Tape /dev/r${TAPE}0 not ready."
			fi
		
		elif [ "${TYPE}" = "tapeless" ]; then

			echo -n "${CMDNAME}: Load \"${TAPE}\" release tape #${REALTAPENUM} on server ${SERVER}, then press RETURN."
                	read x
		
			#
			# No way to get exit code from command executed with 
			# rsh, so test if the mt -f returns an error msg, if 
			# the mt -f was successful it should be silent
			#
			STRING=`rsh ${SERVER} -n mt -f /dev/nr${TAPE}0 rew 2>&1`
			if [ -n "$STRING" ]; then
		      		echo "${STRING}"
		      		echo "${CMDNAME}: Tape /dev/r${TAPE}0 on ${SERVER} not ready."
		      		exit 1
			fi

		fi
		
	fi
	
	echo "${CMDNAME}: Extracting ${KEYWORD} files from \"${TAPE}\" release tape #${REALTAPENUM}."

	COUNT=2
	while true; do

		DIR=`echo ${INFO} | awk '{FS=","; print $'${COUNT}'}'`
		COUNT=`expr ${COUNT} + 1`
		FILE=`echo ${INFO} | awk '{FS=","; print $'${COUNT}'}'`
		COUNT=`expr ${COUNT} + 1`

		if [ -z "${DIR}" -o -z "${FILE}" ]; then
			break
		fi

		if [ "${DIR}" = "root" ]; then
			echo "Changing directory to \"/\"."
			cd /
		else
			echo "Changing directory to \"/usr\"."
			cd /usr
		fi

		SKIPFILE=`expr ${FILE} - 1`

		if [ "${TYPE}" = "tapefull" ]; then
			mt -f  /dev/nr${TAPE}0 rew
			mt -f  /dev/nr${TAPE}0 fsf ${SKIPFILE}
			tar xvpbf ${BS} /dev/nr${TAPE}0
			mt -f  /dev/nr${TAPE}0 rew
		elif [ "${TYPE}" = "tapeless" ]; then
			rsh ${SERVER} -n "mt -f  /dev/nr${TAPE}0 rew"
			rsh ${SERVER} -n "mt -f  /dev/nr${TAPE}0 fsf ${SKIPFILE}"
			rsh ${SERVER} -n "dd if=/dev/nr${TAPE}0 bs=${BS}b"|tar xvpBf -
			rsh ${SERVER} -n "mt -f  /dev/nr${TAPE}0 rew"
		fi

	done

	shift; KEYWORD=${1}

done

echo "${CMDNAME}: End of file extract."
