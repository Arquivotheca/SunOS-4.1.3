# put.awk - to build the command file to put the stuff onto diskettes
#
# @(#)diskette.awk 1.1 92/07/30 Copyright 1989 Sun Microsystems Inc.
#
# input variables:
#    tarfiles - directory of where the compressed tar images are
#
#
# look thru xdrtoc style input for things to put onto diskettes
#
BEGIN {
    # config params
    disksize = 79 * 18 * 2 * 512;
    #
    ncmds = 0;
    doit = 0;        # set when we find something interesting in xdrtoc
    doffset = 0;
    Vol = 1;
    fileoffset = 0;
    printf("onerror() { /usr/5bin/tput bel; echo ERROR; };\n");
    printf("while : ; do\n");
    printf("if make -f diskette.mk newdisk VOL=%d; then : ;\n", Vol++ );
    printf("else\n onerror; continue\nfi\n" );
    printf("rm -f FLOPPY\n" );
    if( tarfiles == "" ) {
        tarfiles = ".";
    }
    ondisk = 0;        # flag for fast write to disk
}
{
    if( doit == 0 ) {
        if( $1 != "0" ) {
            next;
        }
        doit = 1;
    }
    if( $5 != "tarZ" )
        next;
# could do consistancy check here - Vol vs. volno
    volno = $1;
    doffset = $2;
    name = $3;
    size = $4;
    # round up to next 512 byte boundary
    size = sprintf("%d", ((size + 511) / 512) );
    size = size * 512;		# whew, there, wasn`t that easy :-)
    while( size ) {
    towrite = disksize - doffset;
    if( towrite > size )    # takes less than the rest of diskette
        towrite = size;
    # optimize for rapid disk xfer if possible
    if( (doffset == 0) && (towrite == disksize) ) {
        # go from file to diskette
        printf("if dd if=%s/%s of=/dev/rfd0a ibs=1b count=%d skip=%d obs=18b conv=sync; then : ;\n", tarfiles, name, towrite/512, fileoffset/512 );
        ondisk = 1;
    } else {        # go to temp file (fast seeking)
	# NOTE: can't use of=FLOPPY, dd O_CREATs it each time
        printf("if dd if=%s/%s bs=1b count=%d skip=%d conv=sync >> FLOPPY; then : ;\n", tarfiles, name, towrite/512, fileoffset/512 );
    }
    printf("else\n onerror; continue\nfi\n" );
    fileoffset += towrite;
    size -= towrite;
    doffset += towrite;
    if( doffset >= disksize ) {
        # start a new disk
        if( ondisk == 0 ) {
            printf("if dd if=FLOPPY of=/dev/rfd0a bs=36b count=79; then : ;\n");
            printf("else\n onerror; continue\nfi\n" );
        }
        printf("break\ndone\nwhile : ; do\n");
        printf("if make -f diskette.mk newdisk VOL=%d; then : ;\n", Vol++ );
        printf("else\n onerror; continue\nfi\n" );
        printf("rm -f FLOPPY\n" );
        doffset = 0;
        ondisk = 0;
    }
    }
    fileoffset = 0;
}
END {
    if( ondisk == 0 ) {
        printf("if dd if=FLOPPY of=/dev/rfd0a bs=36b count=79 conv=sync; then : ;\n");
        printf("else\n onerror; continue\nfi\n" );
    }
    printf("break\ndone\neject\n");
}
