#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# @(#)makefile 1.1 92/07/30 SMI; from S5R3.1 1.31.1.45
#

#
# cursor package maker
#

# Sources
SLIB=	libcurses.a
PSLIB=	libcurses_p.a
MYBIN=	$(DESTDIR)/usr/5bin
USRLIB = $(DESTDIR)/usr/5lib
CTAGS=	ctags
BUILT=	term.h keycaps curses.h keyname.c termcap.c tiget.c tnames.c tinames.c tifnames.c tifget.c
LINT=	/usr/5bin/lint
LPASS1=	/usr/lib/lint/lint1
TMP=	./tmp
AR=	TMPDIR=$(TMP) ar
RM=	rm
LN=	ln
CPP=	/lib/cpp
CPPFLAGS=	-DS5EMUL -DNOBLIT -I.
COPTS=	-O
CFLAGS=	$(CPPFLAGS) $(COPTS)
SUBOBJS=
LDFLAGS=
LIBS= 

HEADERS=curses.h unctrl.h curses_inc.h curshdr.h term.h uparm.h

#
#	tic source and header files
#
TICHDRS= compiler.h object.h
TICSRC= tic_main.c tic_hash.c tic_captab.c tic_scan.c tic_error.c \
	tic_parse.c tic_read.c tnames.c tinames.c tifnames.c
TICOBJS= $(TICSRC:.c=.o)

CHDRS= print.h otermcap.h

BSRC= print.c iexpand.c
BOBJ= print.o iexpand.o

CSRC= captoinfo.c otermcap.c $(BSRC)
COBJ= captoinfo.o otermcap.o $(BOBJ)

ISRC= infocmp.c cexpand.c infotocap.c $(BSRC)
IOBJ= infocmp.o cexpand.o infotocap.o $(BOBJ)

all: date $(SLIB) $(PSLIB) llib-lcurses.ln tools

date:
	@echo Started: `date`
 
tools: $(SLIB) tic tput captoinfo infocmp show dumpti chgtinfo

CFILES1=_addch.c _addchnstr.c _addchstr.c _addnstr.c _addstr.c \
	_attroff.c _attron.c _attrset.c _beep.c _bkgd.c \
	_bkgdset.c _border.c _box.c _clear.c _clrtobot.c \
	_clrtoeol.c _crmode.c _del_curterm.c _delch.c _deleteln.c \
	_echo.c _echochar.c _erase.c _fixterm.c _flash.c \
	_garbagdlins.c _garbagedwin.c _getch.c _getstr.c \
	_halfdelay.c _hline.c _inch.c _inchnstr.c _inchstr.c \
	_innstr.c _insch.c _insdelln.c _insertln.c _insnstr.c \
	_insstr.c _instr.c _intrflush.c _meta.c _move.c \
	_mvaddch.c _mvaddchnstr.c _mvaddchstr.c _mvaddnstr.c \
	_mvaddstr.c _mvdelch.c _mvgetch.c _mvgetstr.c _mvhline.c \
	_mvinch.c _mvinchnstr.c _mvinchstr.c _mvinnstr.c \
	_mvinsch.c _mvinsnstr.c _mvinsstr.c _mvinstr.c _mvvline.c \
	_mvwaddch.c _mvwaddchnst.c _mvwaddchstr.c _mvwaddnstr.c \
	_mvwaddstr.c _mvwdelch.c _mvwgetch.c _mvwgetstr.c \
	_mvwhline.c _mvwinch.c _mvwinchnst.c _mvwinchstr.c \
	_mvwinnstr.c _mvwinsch.c _mvwinsnstr.c _mvwinsstr.c \
	_mvwinstr.c _mvwvline.c _newterm.c _nl.c _nocrmode.c \
	_noecho.c _nonl.c _noqiflush.c _overlay.c _overwrite.c \
	_qiflush.c _refresh.c _resetterm.c _saveterm.c \
	_scr_init.c _scr_restore.c _scr_set.c _scrl.c _scroll.c \
	_set_curterm.c _set_term.c _setscrreg.c _slk_init.c \
	_standend.c _standout.c _subpad.c _timeout.c _touchline.c \
	_unctrl.c _vline.c _waddchstr.c _waddstr.c _wclear.c \
	_wdeleteln.c _werase.c _winsertln.c _winsstr.c \
	_wstandend.c _wstandout.c V2.__sscans.c V2._sprintw.c \
	V2.makenew.c V3.box.c V3.initscr.c V3.m_addch.c V3.m_addstr.c \
	V3.m_clear.c V3.m_erase.c V3.m_initscr.c V3.m_move.c V3.m_newterm.c \
	V3.m_refresh.c V3.newterm.c V3.pechochar.c V3.upd_old_y.c \
	V3.vidattr.c V3.vidputs.c V3.waddch.c V3.wattroff.c V3.wattron.c \
	V3.wattrset.c V3.wechochar.c V3.winsch.c baudrate.c cbreak.c \
	chkinput.c clearok.c copywin.c curs_set.c \
	curserr.c curses.c def_prog.c delay.c delay_out.c \
	delkey.c delkeymap.c delscreen.c delterm.c delwin.c \
	derwin.c doupdate.c draino.c dupwin.c endwin.c \
	erasechar.c flushinp.c getattrs.c getbegyx.c getbkgd.c \
	getmaxyx.c getparyx.c getsyx.c gettmode.c getwin.c \
	getyx.c has_ic.c has_il.c idcok.c idlok.c \
	immedok.c init_acs.c init_costs.c initscr.c is_linetou.c \
	is_wintou.c keyname.c keypad.c killchar.c leaveok.c

CFILES2=longname.c makenew.c memSset.c meta.c mvcur.c \
	mvderwin.c mvprintw.c mvscanw.c mvwin.c mvwprintw.c \
	mvwscanw.c napms.c newkey.c newpad.c newscreen.c \
	newwin.c nocbreak.c nodelay.c noraw.c \
	notimeout.c outch.c overlap.c pechochar.c pnoutref.c \
	prefresh.c printw.c putp.c putwin.c quick_echo.c \
	raw.c redrawwin.c reset_sh.c resetty.c restart.c \
	ring.c ripoffline.c savetty.c scanw.c scr_all.c \
	scr_dump.c scr_ll_dump.c scr_reset.c scrollok.c setcurscreen.c \
	setcurterm.c setecho.c setkeymap.c setnonl.c setqiflush.c \
	setsyx.c setterm.c setupterm.c slk_clear.c slk_label.c \
	slk_noutref.c slk_refresh.c slk_restore.c slk_set.c slk_start.c \
	slk_touch.c subwin.c syncok.c tcsearch.c termattrs.c \
	termcap.c termerr.c termname.c tgetch.c tgoto.c \
	tifget.c tifnames.c tiget.c tinames.c tinputfd.c \
	tnames.c touchwin.c tparm.c tputs.c trace.c \
	tstp.c ttimeout.c typeahead.c unctrl.c ungetch.c \
	untouchwin.c vidputs.c vidupdate.c vsscanf.c vwprintw.c \
	vwscanw.c waddch.c waddchnstr.c waddnstr.c wattroff.c \
	wattron.c wattrset.c wbkgd.c wbkgdset.c wborder.c \
	wclrtobot.c wclrtoeol.c wdelch.c wechochar.c wgetch.c \
	wgetstr.c whline.c winch.c winchnstr.c winchstr.c \
	winnstr.c winsch.c winsdelln.c winsnstr.c winstr.c \
	wmove.c wnoutref.c wprintw.c wredrawln.c wrefresh.c \
	wscanw.c wscrl.c wsetscrreg.c wsyncdown.c wsyncup.c \
	wtimeout.c wtouchln.c wvline.c

# Regular, optimized objects
OBJS1=	$(CFILES1:%.c=obj/%.o)
OBJS2=	$(CFILES2:%.c=obj/%.o)
$(OBJS1): obj ${HEADERS}
$(OBJS2): obj ${HEADERS}
obj/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
obj:
	test -d $@ || mkdir $@

$(SLIB): $(OBJS1) $(OBJS2)
	@echo
	@echo Building libcurses archive
	@echo
	${AR} rv $(SLIB) $(OBJS1)
	${AR} rv $(SLIB) $(OBJS2)
	ranlib $(SLIB)
	@echo
	@echo libcurses has been archived
	@echo

# Profiled, optimized objects
POBJS1=	$(CFILES1:%.c=obj_p/%.o)
POBJS2=	$(CFILES2:%.c=obj_p/%.o)
$(POBJS1): obj_p ${HEADERS}
$(POBJS2): obj_p ${HEADERS}
obj_p/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ -p $<
obj_p:
	test -d $@ || mkdir $@

$(PSLIB): $(POBJS1) $(POBJS2)
	@echo
	@echo Building libcurses_p archive
	@echo
	${AR} rv $(PSLIB) $(POBJS1)
	${AR} rv $(PSLIB) $(POBJS2)
	ranlib $(PSLIB)
	@echo
	@echo libcurses_p has been archived
	@echo

setupterm.o: term.h 
term.h tnames.c tinames.c tifnames.c: caps maketerm.ed copyright.h
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo
	@echo Building term.h
	@echo
	if [ -r term.h ];then mv -f term.h term.h.SAVE;fi
	if [ -r tnames.c ];then mv -f tnames.c tnames.c.SAVE;fi
	if [ -r tinames.c ];then mv -f tinames.c tinames.c.SAVE;fi
	if [ -r tifnames.c ];then mv -f tifnames.c tifnames.c.SAVE;fi
	ed - < maketerm.ed
	$(CH) chmod 0664 term.h tnames.c tinames.c tifnames.c
	if [ -f term.h.SAVE ];then if cmp -s term.h term.h.SAVE;then mv -f term.h.SAVE term.h;else rm -f term.h.SAVE;fi;fi
	if [ -f tnames.c.SAVE ];then if cmp -s tnames.c tnames.c.SAVE;then mv -f tnames.c.SAVE tnames.c;else rm -f tnames.c.SAVE;fi;fi
	if [ -f tinames.c.SAVE ];then if cmp -s tinames.c tinames.c.SAVE;then mv -f tinames.c.SAVE tinames.c;else rm -f tinames.c.SAVE;fi;fi
	if [ -f tifnames.c.SAVE ];then if cmp -s tifnames.c tifnames.c.SAVE;then mv -f tifnames.c.SAVE tifnames.c;else rm -f tifnames.c.SAVE;fi;fi
	@echo
	@echo "term.h has been built"
	@echo

keycaps: caps
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo
	@echo Building keycaps
	@echo
	grep "KEY_" caps > $(TMP)/keycaps
	if cmp -s $(TMP)/keycaps keycaps;then :;else cp $(TMP)/keycaps keycaps;fi
	rm -f $(TMP)/keycaps
	@echo 
	@echo "keycaps has been built"
	@echo

curses.h: keycaps curses.ed copyright.h
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo 
	@echo Building ./curses.h
	@echo 
	if [ -r curses.h ];then mv -f curses.h curses.h.SAVE;fi
	ed - < curses.ed
	$(CH) chmod 0664 curses.h
	if [ -f curses.h.SAVE ];then if cmp -s curses.h curses.h.SAVE;then mv -f curses.h.SAVE curses.h;else rm -f curses.h.SAVE;fi;fi
	rm -rf $(TMP)/keycaps
	@echo 
	@echo "curses.h has been built"
	@echo 

keyname.c: keycaps keyname.sh copyright.h
	@echo
	@echo Building keyname.c
	@echo
	if [ -r keyname.c ];then mv -f keyname.c keyname.c.SAVE;fi
	sh keyname.sh
	$(CH) chmod 0664 keyname.c
	if [ -f keyname.c.SAVE ];then if cmp -s keyname.c keyname.c.SAVE;then mv -f keyname.c.SAVE keyname.c;else rm -f keyname.c.SAVE;fi;fi
	@echo
	@echo "keyname.c has been built"
	@echo

termcap.c:	termcap.ed caps copyright.h
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo 
	@echo Building termcap.c
	@echo
	if [ -r termcap.c ];then mv -f termcap.c termcap.c.SAVE;fi
	ed - < termcap.ed
	rm -f bool num str
	$(CH) chmod 0664 termcap.c
	if [ -f termcap.c.SAVE ];then if cmp -s termcap.c termcap.c.SAVE;then mv -f termcap.c.SAVE termcap.c;else rm -f termcap.c.SAVE;fi;fi
	@echo 
	@echo "termcap.c has been built"
	@echo

tifget.c:	tifget.ed caps
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo
	@echo Building tifget.c
	@echo
	ed - < tifget.ed
	$(CH) chmod 0664 tifget.c
	@echo
	@echo "tifget.c has been built"
	@echo

tiget.c:	tiget.ed caps
	@if [ ! -d "$(TMP)" ] ;\
	then	echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;
	@echo
	@echo Building tiget.c
	@echo
	ed - < tiget.ed
	$(CH) chmod 0664 tiget.c
	@echo
	@echo "tiget.c has been built"
	@echo

tic_captab.o:	tic_captab.c compiler.h
tic_error.o:	tic_error.c compiler.h
tic_hash.o:	tic_hash.c compiler.h
tic_main.o:	tic_main.c compiler.h curses.h
tic_parse.o:	tic_parse.c compiler.h object.h
tic_read.o:	tic_read.c object.h
tic_scan.o:	tic_scan.c compiler.h

clean:
	rm -f $(OBJS1)
	rm -f $(OBJS2)
	rm -f $(POBJS1)
	rm -f $(POBJS2)
	rm -f $(SLIB) $(PSLIB) $(TICOBJS) $(OTICOBJS)
	rm -f $(BOBJ) $(COBJ) $(IOBJ) show.o tput.o
	rm -f tnames.o tinames.o tifnames.o termerr.o
	rm -rf trace a.out dumpti.c
	rm -rf $(BUILT) *.SAVE $(TMP)
	rm -rf captoinfo chgtinfo dumpti infocmp show llib-lcurses.ln tic tput 

tags: ${CFILES} $(HEADERS)
	${CTAGS} ${CFILES} $(HEADERS)

lint:
	-ln llib-lcurses llib.c
	@echo ${LINT} ${CPPDEFS} > lint.out
	-${LINT} -hxb ${CPPDEFS} ${CFILES} llib.c >> lint.out
	@echo ${LINT} ${DCPPDEFS} >> lint.out
	-${LINT} -hxb ${DCPPDEFS} ${CFILES} llib.c >> lint.out
	rm -f llib.c

lpr:
	-pr `ls ${HEADERS} | sort` | lpr
	-pr `ls ${CFILES} | sort` | lpr
	-lpq

TPFILES= makefile makefile.bsd makefile.sysv \
	${HEADERS} ${CFILES} \
	${TICHDRS} ${TICSRC} \
	${CHDRS} ${BSRC} ${CSRC} ${ISRC} \
	curses.ed \
	termcap.ed tiget.ed tifget.ed \
	tags llib-lcurses TODO caps getopt.c \
	keyname.sh otic.c \
	show.c testterminfo.c tput.c

tp:
	tp crm0 `ls ${TPFILES} | uniq`

tar:
	tar crvf curses.tar `ls ${TPFILES} | uniq`

cpio:
	ls ${TPFILES} | uniq | cpio -o > curses.cpio

cpio-c:
	ls ${TPFILES} | uniq | cpio -oc > curses.cpio

cpiotape:
	ls ${TPFILES} | uniq | cpio -oBc > /dev/rmt8

llib-lcurses.ln: llib-lcurses
	$(CPP) -E -C -I. -I/usr/include -Dlint llib-lcurses | ${LPASS1} -L > llib-lcurses.ln

install:	all install_h
	install -m 644 $(SLIB) $(USRLIB)
	ranlib $(USRLIB)/$(SLIB)
	install -m 644 $(PSLIB) $(USRLIB)
	ranlib $(USRLIB)/$(PSLIB)
	rm -f $(USRLIB)/libtermcap.a $(USRLIB)/libtermlib.a
	ln $(USRLIB)/libcurses.a $(USRLIB)/libtermcap.a
	ln $(USRLIB)/libcurses.a $(USRLIB)/libtermlib.a
	rm -f $(USRLIB)/libtermcap_p.a $(USRLIB)/libtermlib_p.a
	ln $(USRLIB)/libcurses_p.a $(USRLIB)/libtermcap_p.a
	ln $(USRLIB)/libcurses_p.a $(USRLIB)/libtermlib_p.a
	test -d $(USRLIB)/lint || mkdir $(USRLIB)/lint && \
		chown bin $(USRLIB)/lint && \
		chmod 755 $(USRLIB)/lint 
	install -m 644 llib-lcurses $(USRLIB)/lint
	install -m 644 llib-lcurses.ln $(USRLIB)/lint
	install -s tic $(MYBIN)
	install -s captoinfo $(MYBIN)
	install -s infocmp $(MYBIN)
	install -s tput $(MYBIN)
#	cpset ../termcap $(ROOT)/etc 555 bin bin

install_h: curses.h term.h unctrl.h
	install -m 444 curses.h $(DESTDIR)/usr/5include
	install -m 444 term.h $(DESTDIR)/usr/5include
	install -m 444 unctrl.h $(DESTDIR)/usr/5include

cktmp:
	@echo 
	@echo libcurses.a is built using $(TMP) for work space.
	@echo Verifying the existence of $(TMP).
	@echo
	@if [ -d "$(TMP)" ] ;\
	then \
		echo "	$(TMP) exists";\
		echo;\
	else  \
		echo "	$(TMP) not found.";\
		echo "";\
		echo "	Creating $(TMP)";\
		echo "";\
		mkdir $(TMP); \
	fi;

dumpti: setupterm.c curses.h term.h tnames.o tinames.o tifnames.o termerr.o
	@echo
	@echo Building dumpti
	@echo
	rm -f dumpti.c
	ln setupterm.c dumpti.c
	$(CC) $(CFLAGS) -DDUMPTI dumpti.c tnames.o tinames.o tifnames.o termerr.o -o dumpti ; rm -rf dumpti.c
	@echo
	@echo dumpti has been built
	@echo

captoinfo: $(COBJ) $(SLIB)
	$(CC) $(COBJ) $(SLIB) -o captoinfo

infocmp: $(IOBJ) $(SLIB)
	$(CC) $(IOBJ) $(SLIB) -o infocmp

captoinfo.o:	captoinfo.c curses.h otermcap.h print.h
otermcap.o:	otermcap.c otermcap.h
print.o:	print.c curses.h print.h
iexpand.o:	iexpand.c print.h
cexpand.o:	cexpand.c print.h
infotocap.o:	infotocap.c curses.h
infocmp.o:	infocmp.c curses.h term.h print.h
testterminfo.o:	curses.h term.h
tput.o:		curses.h term.h

memSset.o:	curses.h
otic.o:		curses.h
show.o:		curses.h

tic: ${TICOBJS}
	@echo
	@echo Building the tic compiler
	@echo
	${CC} -o tic ${TICOBJS}
	@echo
	@echo the tic compiler has been built
	@echo

OTICOBJS= otic.o tnames.o tinames.o tifnames.o unctrl.o

otic: ${OTICOBJS}
	${CC} -o otic ${OTICOBJS}

linttic: ${TICSRC}
	${LINT} ${CPPDEFS} ${TICSRC}

lintinfocmp: ${ISRC}
	${LINT} ${CPPDEFS} ${ISRC}

lintcaptoinfo: ${CSRC}
	${LINT} ${CPPDEFS} ${CSRC}

show:	$(SLIB) show.o
	${CC} ${LDFLAGS} ${CFLAGS} -o show show.o $(SLIB)

tput:	$(SLIB) tput.o tnames.o tinames.o tifnames.o
	${CC} ${LDFLAGS} ${CFLAGS} -o tput tput.o $(SLIB)

testterminfo:	$(SLIB) testterminfo.o
	${CC} ${LDFLAGS} ${CFLAGS} -o testterminfo testterminfo.o $(SLIB)

chgtinfo:	curses.h chgtinfo.c
	${CC} ${LDFLAGS} ${CFLAGS} -o chgtinfo chgtinfo.c
