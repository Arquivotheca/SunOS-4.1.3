#
#  @(#)Makefile 1.1 92/07/30 SMI
#

SHELLSC    =    install_upgrade upgrade_exec upgrade_client tape_subs \
		utility_subs cdrom_subs toc_subs install_subs get_params
INCLUDE    =    Debugging.include Demo.include Games.include Install.include \
		Manual.include Networking.include SunView_Programmers.include \
		SunView_Users.include System_V.include User_Diag.include \
		root.include usr.include
PATCHDIR   =    /UNDISTRIBUTED/patches/upgrade

all:            $(SHELLSC) $(INCLUDE)

install_upgrade: install_upgrade.sh;  rm -f $@; cp -p $? $@; chmod +x $@
upgrade_exec:	upgrade_exec.sh;      rm -f $@; cp -p $? $@; chmod +x $@
upgrade_client:	upgrade_client.sh;    rm -f $@; cp -p $? $@; chmod +x $@
tape_subs:	tape_subs.sh;         rm -f $@; cp -p $? $@; chmod +x $@
utility_subs:	utility_subs.sh;      rm -f $@; cp -p $? $@; chmod +x $@
cdrom_subs:	cdrom_subs.sh;        rm -f $@; cp -p $? $@; chmod +x $@
toc_subs:	toc_subs.sh;          rm -f $@; cp -p $? $@; chmod +x $@
install_subs:	install_subs.sh;      rm -f $@; cp -p $? $@; chmod +x $@
get_params:	get_params.sh;        rm -f $@; cp -p $? $@; chmod +x $@

install:        all
	install -d -m 755 ${DESTDIR}${PATCHDIR}
	for i in $(SHELLSC); do \
		install -m 755 $$i ${DESTDIR}${PATCHDIR}/$$i; \
	done
	for i in $(INCLUDE); do \
		install -m 666 $$i ${DESTDIR}${PATCHDIR}/$$i; \
	done

clean:
	rm -f $(SHELLSC) 
