#!/bin/sh
#
#  %Z%%M% %I% %E% SMI
#
# NAME: install_subs
#

# install vmunix 
#
# $1 -- the directory to install into

install_vmunix () {
        echo "installing vmunix..."
        if [ ! -s $1/vmunix ]; then
                echo "Can't find vmunix in $1."
                return
        fi
        [ -f /vmunix ] && [ ! -s /vmunix.orig ] && mv /vmunix /vmunix.orig

        for i in 1 2; do
                cp $1/vmunix /vmunix
                sync; sync
                sum1=`/usr/bin/sum $1/vmunix`
                sum2=`/usr/bin/sum /vmunix`
                [ "$sum1" = "$sum2" ] && break
                if [ "$i" = 2 ]; then
                        echo "Unable to install new kernel for $HOST."
                        [ ! -f /vmunix.orig ] && break
                        echo "Restoring original kernel..."
                        mv /vmunix.orig /vmunix
                        sync; sync
                fi
        done
}


# install kadb
#
# $1 -- the directory to install into

install_kadb () {
        echo "installing kadb..."
        if [ ! -s $1/kadb ]; then
                echo "Can't find kadb in $1."
        else
                cp $1/kadb /kadb
                sync; sync
        fi
}

# install boot block

install_boot_block () {
        echo "installing bootblock..."
        DISK=`mount | grep ' / ' | awk '{print $1}' | awk -F/ '{print $3}'`
        DISKTYPE=`echo $DISK | colrm 3`
        cp /usr/kvm/stand/boot.${KARCH} /boot
        sync; sync
        cd /usr/mdec
        ./installboot -v /boot boot"${DISKTYPE}" /dev/r"${DISK}"
        sync; sync
}

# make links in /export/exec

make_links () {
        cd /export/exec
        ln -s /usr ${AARCH}.sunos.${TO_RELEASE}
        cd /export/exec/kvm
        ln -s /usr/kvm ${KARCH}.sunos.${TO_RELEASE}
}
 
# add pcfs line to fstab if not present

fix_fstab() {
	echo "updating fstab..."
	FSTAB=/etc/fstab
        if [ ! -s $FSTAB ]; then
                echo "No $FSTAB?"
                return 1
        fi
	grep -s '/pcfs' $FSTAB || 
                echo "/dev/fd0  /pcfs   pcfs rw,noauto 0 0" >> $FSTAB
	return 0
}

# fix the release name file 

fix_release () {
	echo ${TO_RELEASE} > /usr/sys/conf.common/RELEASE
}

# fix up the suninstall data files to reflect the new release
 
fix_data_files () {
 
        release_string="${AARCH}.${KARCH}.sunos.${TO_RELEASE}"
 
        # "release" - write new release string
        echo ${release_string} > ${INSDIR}/release

	# fix the arch_info file

ed -s ${INSDIR}/arch_info << END > /dev/null
/${AARCH}.${KARCH}.sunos.${FROM_RELEASE}/
s/${FROM_RELEASE}/${TO_RELEASE}/
w
q
END

 
        # "sys_info" - put the new arch string in the sys_info file
 
ed -s ${INSDIR}/sys_info << END > /dev/null
/arch_str/
c
arch_str=${release_string}
.
w
q
END
        # "soft_info.<aarch>.<karch>.sunos.<FROM_RELEASE>" -
        # rename and edit this file
        # (this file required by add_services)
 
        [ "$REMOTE" ] && location=remote || location=local
 
        mv ${INSDIR}/soft_info.${AARCH}.${KARCH}.sunos.${FROM_RELEASE} \
                ${INSDIR}/soft_info.${release_string}
 
ed -s ${INSDIR}/soft_info.${release_string} << END > /dev/null
/arch_str/
c
arch_str=${release_string}
.
/media_dev/
c
media_dev=${DEVROOT}
.
/media_path/
c
media_path=${DEVPATH}
.
/media_loc/
c
media_loc=${location}
.
w
q
END
 
	# rename and edit appl_media_file

        mv ${INSDIR}/appl_media_file.${AARCH}.sunos.${FROM_RELEASE} \
                ${INSDIR}/appl_media_file.${AARCH}.sunos.${TO_RELEASE}


ed -s ${INSDIR}/appl_media_file.${AARCH}.${TO_RELEASE} << END > /dev/null
/arch_str/
c
arch_str=${release_string}
.
w
q
END

}

