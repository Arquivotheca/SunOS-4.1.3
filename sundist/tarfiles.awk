#
# @(#)tarfiles.awk 1.1 92/07/30 Copyright 1989 Sun Microsystems, Inc.
#
# snarfs the Name, File-Kind and Command from the input
# and generates a shell script
# that will run the commands and put the stuff into files.
# the commands are run in a sub-shell, cause they "cd" around
#
# NOTE: we only grab "installable" files
#
# input variables:
#    dir = where to put the files
#
BEGIN {
	gotname = 0;
	gotkind = 0;
}
/^\*Name=/ {
	gotname = 1;
	name = $2;
	next;
}
/^\*File-Kind=/ {
	if (gotname == 0) {
		printf("opps, name/kind mismatch");
		exit(1);
	}
	if( $2 != "installable" ) {
		gotkind = 0;
		next;
	}
	gotkind = 1;
	next;
}
/^\*Command=/ {
	if (gotname == 0) {
		printf("opps, name/command mismatch");
		exit(1);
	}
	# if not installable, skip
	if (gotkind == 0) {
		next;
	}
        # 10 as in *Command= is 9
        printf("( %s ) > %s/%s\n", substr( $0, 10, 400), dir, name );
        gotname = 0;
        gotkind = 0;
        next;
}
END { }
