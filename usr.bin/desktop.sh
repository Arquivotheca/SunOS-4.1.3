#!/bin/sh
#
#       %W% %G% SMI
#
# 	Copyright (c) 1990 by Sun Microsystems, Inc.
#
# desktop - switch the window system to be invoked upon login
#
#
#
option=$1

# no more than 1 argument is accepted
if [ $# -gt 1 ]; then
	echo "usage: $0 [openwin] [sunview]"
	exit 1
fi

# if no argument passwd, ask question and have user choose one from the menu,
while [ ! "$option" ];  do
	echo ""
	echo "    Choose a window system that is invoked upon your login, press RETURN."
	echo ""
	echo " 	  o = OpenWindows"
	echo " 	  s = SunView"
	echo -n "    > "
	read ans
	case $ans in
	[Oo] )
		option=openwin
		break;;
	[Ss] )
		option=sunview
		break;;
	* )
		echo "unknown input: ${ans}" ;;
	esac 
done

# finally, set mychoice and set the default window system needs to be replaced
case $option in
openwin )
	if [ ! "$OPENWINHOME" ]; then
	    OPENWINHOME=/usr/openwin
	    export OPENWINHOME
	fi
	if [ -x "$OPENWINHOME/bin/openwin" ]; then
	    mychoice=openwin
	    theother=sunview
	    break
       	 else
	    echo ""
	    echo "   $OPENWINHOME/bin/openwin not found -- no change made."
	    echo ""
	    exit
	fi;;
sunview )
	mychoice=sunview
	theother=openwin
	break;;
# Also, for user who gives argument but the argument is not a name of the default
# windows, exit him/her to the shell and show him/her the correct command.  
* )  
	echo "usage: $0 [openwin] [sunview]"
	exit;;
esac

# edit this file to change choice "forever"
if [ ! -f $HOME/.cshrc ]; then
	echo ""
	echo "    Your .cshrc file with window system invocation code not found --"
	echo "    no change made."
	echo ""
	exit
fi

if [ -w $HOME/.cshrc ]; then
	remake_read_only=false
else
	chmod -f u+w $HOME/.cshrc
	if [ -w $HOME/.cshrc ]; then	
		remake_read_only=true
	else
		echo ""
		echo "    Cannot change window system invocation code in .cshrc"
		echo "    because of ownership conflict -- no change made."
		echo ""
		exit
	fi
fi

# set choice variable, which will be printed out in a informational message.		
if [ ${mychoice} = "openwin" ]; then
	choice=OpenWindows
else
	choice=SunView
fi
# search for the right string
grep "^set mychoice=${theother}" $HOME/.cshrc > /dev/null
if [ $? -ne 0 ]; then
	grep "^set mychoice=${mychoice}" $HOME/.cshrc > /dev/null
	if [ $? -ne 0 ]; then
		echo ""
		echo "    Cannot change window system invocation code in .cshrc"
		echo "    because invocation code not found -- no change made."
		echo ""
		if [ ${remake_read_only} = "true" ]; then
			chmod u-w $HOME/.cshrc
		fi
		exit
	else
		echo ""
		echo "    Your window system invoked upon your login is already ${choice}."
		echo 
		if [ ${remake_read_only} = "true" ]; then
			chmod u-w $HOME/.cshrc
		fi
		exit
	fi
else
	echo ""
	echo "    Your window system invoked upon your login has been changed to ${choice}."
	echo ""
fi
# replace theother with mychoice
#
( echo "/^set mychoice=${theother}/" ; \
  echo "s/${theother}/${mychoice}/" ; \
  echo "w" ; \
  echo "q" \
) | ed -s $HOME/.cshrc > /dev/null
#
	
if [ ${remake_read_only} = "true" ]; then
	chmod u-w $HOME/.cshrc
fi

echo
echo 
echo "    To activate your invoked window system, please log out and log back in."
echo
echo 

