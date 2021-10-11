#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: cdrom_subs
#

cd_mount_path="/usr/etc/install/tar"                        # cdrom mount path
cd_usr_path="$cd_mount_path/export/exec/sun4_sunos_4_1"     # cdrom usr path
cd_share_path="$cd_mount_path/export/share/sunos_4_1"       # cdrom share path
cd_root="$cd_mount_path/export/exec/proto_root_sunos_4_1"   # cdrom root file
cd_toc_path="$cd_mount_path/export/exec/kvm/sun4c_sunos_4_1/xdrtoc"
                                                         # cdrom toc path



# mount the cdrom hsfs

media_start() {
        echo "Mounting the cdrom hsfs on $cd_mount_path..."
        if [ "$REMOTE" ]; then
            stat=`rsh -n "$REMOTE_HOST" "/etc/mount -rt hsfs $DEVPATH
            $cd_mount_path; echo \\$status"`
        else
            /etc/mount -rt hsfs $DEVPATH $cd_mount_path
            stat=$?
        fi
        if [ ${stat} -ne 0 ]; then
            echo "$CMD: Problem with mounting drive, exiting"
            cleanup 1
        fi
}

media_rewind(){

	return
}

media_skip() {

	return
}

media_extract() {
        temp=`echo $PACKAGE | tr 'A-Z' 'a-z'`
        this_path=${cd_usr_path}/${temp}
        [ "${PACKAGE}" = "Manual" ] && this_path=${cd_share_path}/${temp}
        [ "${PACKAGE}" = "root" ]   && this_path=${cd_root}
        if [ "$REMOTE" ]; then
            stat=`rsh -n "$REMOTE_HOST" "$DD if=${this_path} |
            $TAR xBvfp -I ${list} ; echo \\$status"`
        else
            $TAR xvfp ${this_path} -I ${list}
            stat=$? 
        fi  
	
        if [ $stat -ne 0 ]; then
            echo
            echo "$CMD: Extraction failed!"
            echo
            cleanup 1
	fi
}

media_check() {

	return
}


# extract toc from cdrom

extract_toc() {

    	echo ""
    	echo "Examining table of contents on release media..."
 
    	command="$DD if=${cd_toc_path}"
 
    	[ "$REMOTE" ] && command="rsh -n "${REMOTE_HOST}" $command"
 
    	eval `$command | ${XDRTOC} > ${TOCFILE}`
}

