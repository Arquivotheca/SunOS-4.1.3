#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: utility_subs
#
#



#
# the only place we read from stdin
#
stdin_log () {
    read $1
}

#
# return "yes" or "no" into shell variable named $1.  $2 is the user prompt.
#
get_yesno () {
    yesno=""
    while [ ! "$yesno" ]
    do
        echo ""
        echo -n "$2 [yes|no]? "
        stdin_log yesno
        case "$yesno" in
        [yY]*)  yesno=yes ;;
        [nN]*)  yesno=no ;;
        *)      echo "" ; echo 'Please enter "yes" or "no".' ; yesno="" ;;
        esac
    done
    eval $1="$yesno"
}

#
# return "yes" or "no" into shell variable named $1.  $2 is the user prompt.
# $3 is the default answer to use
#
get_yesno_def () {
    yesno=""
    while [ ! "$yesno" ]
    do
        echo ""
        echo -n "$2 [yes|no] ($3)? "
        stdin_log yesno
        case "$yesno" in
        [yY]*)  yesno=yes ;;
        [nN]*)  yesno=no ;;
        "")     yesno="$3" ;;
        *)      echo "" ; echo 'Please enter "Yes" or "No".' ; yesno="" ;;
        esac
    done
    eval $1="$yesno"
}
 
 
#
# generic user prompt routine with no default
# $1 is the variable to set
# $2 is the prompt
#
get_ans () {
    echo ""
    echo -n "$2? "
    stdin_log new_ans
    eval $1="$new_ans"
}


# generic user prompt routine with default
# $1 is the variable to set
# $2 is the prompt
# $3 is the default answer
#
get_ans_def() {
    echo ""
    echo -n "$2 ($3)? "
    stdin_log new_ans
    if [ "$new_ans" = "" ]
    then
        new_ans="$3"
    fi
    eval $1="$new_ans"
}

#
# provide a single script exit point -- clean up and go away
#
cleanup () {
    rm -f $TOCFILE
    exit $1
}


#
# wait for some user input
#
return_when_ready () {
    echo
    echo -n "Press return when ready:"
    read ans
}


