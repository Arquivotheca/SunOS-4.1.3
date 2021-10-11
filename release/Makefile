#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=
CFLAGS=	-O

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR= ssum smerge

# Shell scripts that need only be installed and are never removed.
#
SCRIPTS=linksccs sclean scheck metaclean do_sccs_report \
	editfiles rsccs check_sccs link_sys get_buginfo \
	makelibs makebins makeproto restore_40 to_41 trap_cc new_to41 \
	still_out snapshot patch_sid get_tapeinfo check_sums \
	check_copyright save_cd_depends ins_rel_conv nite_build \
	backup gsid sccspar sccslog_elmer gensid sid_cmp sid_rpt sccs_act day \
	keyword_finder

# Shell scripts that need only be installed and are never removed.
#
ETC_SCRIPTS=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	makeinstall mkSCCSdir ok unmount winstall  .mountit
ETC_STD=

# Programs that must run setuid to root
#
SETUID=makeinstall unmount winstall  .mountit
ETC_SETUID=

# C programs that live in the current directory and need explicit make lines.
#
NSTD= pacifier
ETC_NSTD=

all:	$(SUBDIR) $(STD) $(ETC_STD) $(NSTD) $(ETC_NSTD)
 
$(SUBDIR): FRC
	cd $@; $(MAKE) $(MFLAGS)

install: $(STD) $(ETC_STD) $(NSTD) $(ETC_NSTD) $(SCRIPTS)
	install -d -o bin -m 755 $(DESTDIR)/usr/release
	install -d -o bin -m 755 $(DESTDIR)/usr/release/bin
	install -d -o bin -m 755 $(DESTDIR)/usr/release/man
	for i in $(SUBDIR); do \
		(cd $$i; $(MAKE) $(MFLAGS) DESTDIR=$(DESTDIR) install); done
	install $(SCRIPTS) $(DESTDIR)/usr/release/bin
#	install $(ETC_SCRIPTS) $(DESTDIR)/usr/release/etc
	install -s $(STD) $(NSTD) $(DESTDIR)/usr/release/bin
#	install -s $(ETC_STD) $(ETC_NSTD) $(DESTDIR)/usr/release/etc
	for i in $(SETUID); do (chown root $(DESTDIR)/usr/release/bin/$$i; \
		chmod 4755 $(DESTDIR)/usr/release/bin/$$i); done
#	for i in $(ETC_SETUID); do (chown root $(DESTDIR)/usr/release/etc/$$i; \
#		chmod 4755 $(DESTDIR)/usr/release/etc/$$i); done

clean:
	rm -f a.out core *.s *.o
	for i in $(SUBDIR); do (cd $$i; $(MAKE) $(MFLAGS) clean); done
	rm -f $(STD) $(ETC_STD) $(NSTD) $(ETC_NSTD)

FRC:

depend:
	cat </dev/null >x.c
	for i in $(STD) $(ETC_STD) $(NSTD) $(ETC_NSTD); do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e '/\.\.\/ufs/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

pacifier: 
	${CC} -O -Bstatic -o pacifier pacifier.c

# Files listed in $(NSTD) and $(ETC_NSTD) have explicit make lines given below.

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
