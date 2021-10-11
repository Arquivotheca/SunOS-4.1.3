case $1 in 
-i)	shift ; inter=1
;;
esac

date=`date`
dy=`echo $date | awk '{ print $3 }'`
mo=`echo $date | awk '{ print $2 }'`
yr=`echo $date | awk '{ print $6 }'`

case $mo in
Jan)	mo=January	 ;;
Feb)	mo=February	 ;;
Mar)	mo=March  	 ;;
Apr)	mo=April  	 ;;
May)	mo=May    	 ;;
Jun)	mo=June   	 ;;
Jul)	mo=July    	 ;;
Aug)	mo=August  	 ;;
Sep)	mo=September ;;
Oct)	mo=October	 ;;
Nov)	mo=November	 ;;
Dec)	mo=December	 ;;
esac

for i in $*
do
	case $inter in
	1) echo $i
	;;
	esac
	( if test ! -w $i ; then chmod +w $i ; fi )
	ed - $i <<EOT > /dev/null 2>&1
/^\.TH/s/".*"/"$dy $mo ${yr}"/
w
q
EOT
done
