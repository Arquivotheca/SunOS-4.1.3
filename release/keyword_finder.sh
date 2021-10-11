#!/bin/sh
#%Z%%M% %I% %E% SMI
 
LIST="arning ARNING ailed AILED annot denied memory working syntax rror: illegal ERROR such undefined"

for i in $LIST
do
        grep $i $1
done

grep "not defined" $1
grep "an\'t" $1
