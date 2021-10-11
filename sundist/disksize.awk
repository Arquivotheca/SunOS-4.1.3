#
# @(#)disksize.awk 1.1 92/07/30 Copyright 1989 Sun Microsystems, Inc.
#
# snarfs the Name and Size from the input and generates a shell script
# that will run the commands and put the stuff into files.
# the commands are run in a sub-shell, cause they "cd" around
#
#
# input variables:
#    dir = where to put the result of the comands
#
BEGIN {
	gotname = 0;
}
/^\*Name=/ {
	gotname = 1;
	name = $2;
	next;
}
/^\*Size=/ {
	if (gotname == 0) {
		printf("opps, name/size mismatch");
		exit(1);
	}
	# 7 as in *Size= is 6
	printf("( %s ) > %s/%s\n", substr( $0, 7, 256), dir, name);
	gotname = 0;
	next;
}
END { }
