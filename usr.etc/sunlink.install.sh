#! /bin/csh -f
#
# %W%  %G%
# 
# Copyright (c) 1988 by Sun Microsystems, Inc.
#
#

# Script for installing SunLink products on sun3x and sun4x machines

set PRODUCTS=(6.1_BSC3270 6.0_BSCRJE 6.0_Channel_Adapter \
  6.0_DDN 6.0_SCP 6.0_DNI 6.0_HSI 6.0_INR 6.0_MCP 6.1_SNA3270 \
  6.0_Peer-to-Peer )

set DIRECTORIES=(bsc3270 bscrje chat \
	ddn dcp dni hsi inr mcp sna3270 \
	snap2p )

set MAJORARCH=`arch`
set MINORARCH=`arch -k`

if (`whoami` != "root") then
        echo "You must be root to run this script."
        exit 1
endif

echo " "
echo "This script 'fixes' the SunLink 6.0/6.1 installation and configuration"
echo "scripts so that they are compatible with this release.  You should"
echo "use extract_unbundled to extract the SunLink product off the tape "
echo "and then run this script BEFORE running any installation/configuration "
echo "scripts that come with the product."
echo " "
echo "This script is only required for the following SunLink products:"
echo " "
set i=1
foreach PRODUCT ($PRODUCTS)
	echo ${i}.  $PRODUCT
	@ i = $i + 1
end

while (1)
	echo " "
	echo -n "Which of the above products are you installing [1-$#PRODUCTS]? "
	set a=($<)
	echo " "
	if ($a == "") exit 
  	set NUMBER=`echo $a | grep -v -c "[^0-9]"`
  	if ($NUMBER == "0") then
       		echo "Not between 1 and $#PRODUCTS."
      		continue
      	endif
	if ($a < 1 || $a > $#PRODUCTS) then
	       	echo "Not between 1 and $#PRODUCTS."
      		continue
      	endif
      	set PRODUCT=$PRODUCTS[$a]
      	set DIRECTORY=/usr/sunlink/$DIRECTORIES[$a]
      	if (! -e $DIRECTORY) then
      		echo "${PRODUCT}: The product's directory (${DIRECTORY})"
      		echo "does not exist.  Have you used extract_unbundled"
      		echo "to install the product?"
      		continue
      	endif
      	break
end

echo "Fixing files in $DIRECTORY for product: $PRODUCT"
echo " "

# Copy the OBJ files
setenv MAJOR $DIRECTORY/sys/$MAJORARCH
setenv MINOR $DIRECTORY/sys/$MINORARCH

# If the MAJOR directory exists, try and copy the files to MINOR directory.

if ("$MAJORARCH" == "$MINORARCH") goto nokernel
if (! -e $MAJOR) goto nokernel

# If the minor directory exists, delete it (will fail unless empty)
if (-e $MINOR) then
	echo "Kernel files for this product for minor architecture $MINORARCH "
	echo -n "already exist.  Ok to overwrite them [y or n] "
	while (1)
		set a=($<)
		echo " "
		if (($a == "n") || ($a == "N")) goto nokernel
		if (($a == "y") || ($a == "Y")) break
		echo -n "Ok to overwrite them? [y or n] "
	end
endif

echo "Copying OBJ files from $MAJOR "
echo "to $MINOR."
if (! -e $MINOR) mkdir $MINOR
(cd $MAJOR ; tar cf - . ) | (cd $MINOR ; tar xfB - )
echo " "

nokernel:	
  
# Modify all the SunLink scripts to use "arch -k" instead of "arch"
# Save the original script in xxxxx.orig.  If we find either an xxxx.orig
# file or the script doesn't contain "arch", we don't modify it.
  
# First find all the possible files that might be installation scripts
set SCRIPTS=`find $DIRECTORY \( -name 'install.*' -o -name 'dni_*install' -o -name 'sys_install' \) -print`

set TEMPFILE=/tmp/sunlink_fixup.$$
echo "Modifying scripts in $DIRECTORY to use 'arch -k'".
  
# For each possible script, attempt to modify it
foreach SCRIPT ($SCRIPTS)
	# Check and see if the script contains "arch" and if so
	# modify it
	grep '`arch -k`' $SCRIPT  > /dev/null
	if ($status == 0) then
		echo "  ${SCRIPT}: previously modified."
		continue
	endif
	grep '`arch`' $SCRIPT  > /dev/null
	if ($status == 0) then
		echo "  ${SCRIPT}: modified."
		cp $SCRIPT $TEMPFILE
		sed 's/`arch`/`arch -k`/' < $TEMPFILE > $SCRIPT
		chmod 0755 $SCRIPT
	endif
end
echo " "

set TEMPFILE=/tmp/sunlink_fixup.$$
echo "Modifying scripts in $DIRECTORY to use" '''/export/exec/`arch`'''.
  
# For each possible script, attempt to modify it
foreach SCRIPT ($SCRIPTS)
	# Check and see if the script contains "arch" and if so
	# modify it
	grep '/export/exec/`arch`' $SCRIPT  > /dev/null
	if ($status == 0) then
		echo "  ${SCRIPT}: previously modified."
		continue
	endif
	grep '/export/exec/$ARCH' $SCRIPT  > /dev/null
	if ($status == 0) then
		echo "  ${SCRIPT}: modified."
		cp $SCRIPT $TEMPFILE
		sed 's{/export/exec/\$ARCH{/export/exec/`arch`{' < $TEMPFILE > $SCRIPT
		chmod 0755 $SCRIPT
	endif
end
echo " "
 
if (-e $TEMPFILE) /bin/rm /tmp/sunlink_fixup.$$
			 





