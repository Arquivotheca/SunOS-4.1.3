#! /bin/csh -f
#
#	%Z%%M% %I% %E% SMI; from UCB 4.2 83/02/14
#
#	which : tells you which program you get
#
# Set prompt so .cshrc will think we're interactive and set aliases.
# Save and restore path to prevent .cshrc from messing it up.
set _which_saved_path_ = ( $path )
set prompt = ""
if ( -r ~/.cshrc && -f ~/.cshrc ) source ~/.cshrc
set path = ( $_which_saved_path_ )
unset prompt _which_saved_path_
set noglob
foreach arg ( $argv )
    set alius = `alias $arg`
    switch ( $#alius )
	case 0 :
	    breaksw
	case 1 :
	    set arg = $alius[1]
	    breaksw
        default :
	    echo ${arg}: "	" aliased to $alius
	    continue
    endsw
    unset found
    if ( $arg:h != $arg:t ) then
	if ( -e $arg ) then
	    echo $arg
	else
	    echo $arg not found
	endif
	continue
    else
	foreach i ( $path )
	    if ( -x $i/$arg && ! -d $i/$arg ) then
		echo $i/$arg
		set found
		break
	    endif
	end
    endif
    if ( ! $?found ) then
	echo no $arg in $path
    endif
end
