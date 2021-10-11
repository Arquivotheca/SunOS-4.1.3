#  @(#)Makefile 1.1 92/07/30 SMI;
#
#  Description file for making the section 8 manual page sources.
#
#  NOTE that the list of filenames in the description file must be
#  kept up to date with the actual manual pages, including ".so" files.

HERE=man8

# --------------------------------------------------------------- #

FILES= \
Intro.8 intro.8 List.8 list.8 \
C2conv.8 C2unconv.8 MAKEDEV.8 ac.8 acct.8 acctcms.8 acctcon.8 \
acctcon1.8 acctcon2.8 acctdisk.8 acctdusg.8 acctmerg.8 accton.8 \
acctprc.8 acctprc1.8 acctprc2.8 acctsh.8 acctwtmp.8 adbgen.8 \
add_client.8 add_services.8 adduser.8 add_user.8 adv.8 analyze.8 arp.8c audit.8 \
audit_warn.8 auditd.8 automount.8 biod.8 boot.8s bootparamd.8 c2conv.8 \
c2unconv.8 captoinfo.8v catman.8 change_login.8 chargefee.8 check4.8 \
chown.8 chroot.8 chrtbl.8 ckpacct.8 client.8 clri.8 colldef.8 comsat.8c \
config.8 copy_home.8 crash.8 crash.8s cron.8 dbconfig.8 dcheck.8 \
devinfo.8s devnm.8v diskusg.8 dkctl.8 dkinfo.8 dmesg.8 dname.8 dodisk.8 \
dorfs.8 dump.8 dumpfs.8 edquota.8 eeprom.8s etherd.8c etherfind.8c \
exportfs.8 extract_files.8 extract_patch.8 extract_unbundled.8 \
fastboot.8 fasthalt.8 fingerd.8c fontflip.8 format.8s fpa_download.8 \
fparel.8 fpaversion.8 fpurel.8 fpuversion4.8 fsck.8 fsirand.8 ftpd.8c \
fumount.8 fusage.8 fuser.8 fwtmp.8 gencat.8 gettable.8c getty.8 \
gid_allocd.8c gpconfig.8 grpck.8v gtconfig.8 gt_lpconfig.8 gxtest.8s \
halt.8 hostconfig.8c hostrfs.8 htable.8 \
icheck.8 idload.8 ifconfig.8c imemtest.8s in.comsat.8c in.fingerd.8c \
in.ftpd.8c in.named.8c in.rexecd.8c in.rlogind.8c in.routed.8c \
in.rshd.8c in.rwhod.8c in.talkd.8c in.telnetd.8c in.tftpd.8c \
in.tnamed.8c in.uucpd.8c inetd.8c infocmp.8v init.8 \
install_small_kernel.8 installboot.8s installtxt.8 intr.8 iostat.8 \
ipallocd.8c kadb.8s keyenvoy.8c keyserv.8c kgmon.8 lastlogin.8 \
ldconfig.8 link.8v list_files.8 listen.8 lockd.8c logintool.8 lpc.8 \
lpd.8 mailstats.8 makedbm.8 makedev.8 makekey.8 mc68881version.8 \
mconnect.8 mkfile.8 mkfs.8 mknod.8 mkproto.8 modload.8 modstat.8 \
modunload.8 monacct.8 monitor.8s mount.8 mount_tfs.8 mountd.8c named.8c \
ncheck.8 ndbootd.8c netconfig.8c netstat.8c newaliases.8 newfs.8 \
newkey.8 nfsd.8 nfsstat.8c nlsadmin.8 nslookup.8c nsquery.8 nulladm.8 \
old-analyze.8 openboot.8s pac.8 panic.8s ping.8c pnp.s386.8c pnpboot.8c \
pnpd.8c portmap.8c praudit.8 prctmp.8 prdaily.8 prtacct.8 pstat.8 pwck.8v \
pwdauthd.8c quot.8 quotacheck.8 quotaoff.8 quotaon.8 rarpd.8c rc.8 \
rc.boot.8 rc.local.8 rdate.8c rdump.8 reboot.8 renice.8 repquota.8 \
restore.8 rexd.8c rexecd.8c rfadmin.8 rfpasswd.8 rfstart.8 rfstop.8 \
rfuadmin.8 rfudaemon.8 rlogind.8c rm_client.8 rmail.8c rmntstat.8 \
rmt.8c route.8c routed.8c rpc.etherd.8c rpc.lockd.8c rpc.mountd.8c \
rpc.rexd.8c rpc.rquotad.8c rpc.rstatd.8c rpc.rusersd.8c rpc.rwalld.8c \
rpc.sprayd.8c rpc.statd.8c rpc.yppasswdd.8c rpc.ypupdated.8c rpcinfo.8c \
rquotad.8c rrestore.8 rshd.8c rstatd.8c runacct.8 rusage.8 rusersd.8c \
rwalld.8c rwhod.8c sa.8 savecore.8 sendmail.8 set4.8 setsid.8v \
showfh.8c showfhd.8c showmount.8 showrev.8 shutacct.8 shutdown.8 \
spray.8c sprayd.8c start_applic.8 startup.8 statd.8c sticky.8 sundiag.8 \
suninstall.8 swapon.8 sys-unconfig.8 syslogd.8 talkd.8c telnetd.8c \
tfsd.8 tftpd.8c tic.8v tnamed.8c trpt.8c ttysoftcar.8 tunefs.8 \
turnacct.8 tzsetup.8 uid_allocd.8c umount.8 umount_tfs.8 unadv.8 \
unconfigure.8 unixname2bootname.8 unlink.8v unset4.8 update.8 \
user_agentd.8 uucheck.8c uucico.8c uuclean.8c uucleanup.8c uucpd.8c \
uusched.8c uuxqt.8c vipw.8 vmstat.8 wtmpfix.8 ypbatchupd.8c ypbind.8 \
ypinit.8 ypmake.8 yppasswdd.8c yppoll.8 yppush.8 ypserv.8 ypset.8 \
ypsync.8 ypupdated.8c ypxfr.8 ypxfr_1perday.8 ypxfr_1perhour.8 \
ypxfr_2perday.8 ypxfrd.8 zdump.8 zic.8

#----------------------------------------------------------------#

include ../Makefile.master
