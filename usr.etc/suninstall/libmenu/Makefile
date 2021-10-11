#
#	Makefile for menu library
#
#	@(#)Makefile 1.1 92/07/30
#

CFLAGS=		-O $(INCPATH)
HFILES=		menu.h menu_impl.h
INCPATH=
LIB=		libmenu.a
LINT_LIB=	llib-lmenu.ln
OBJS= \
		add_button.o \
		add_confirm.o \
		add_field.o \
		add_file.o \
		add_finish.o \
		add_item.o \
		add_noecho.o \
		add_radio.o \
		add_string.o \
		add_yesno.o \
		ask_num.o \
		ask_yesno.o \
		chk_screen.o \
		chk_swap.o \
		ckf_abspath.o \
		ckf_empty.o \
		ckf_ether.o \
		ckf_inet_addr.o \
		ckf_int.o \
		ckf_long.o \
		ckf_netabs.o \
		ckf_netpath.o \
		ckf_octal.o \
		ckf_relpath.o \
		ckf_uint.o \
		clear_button.o \
		clear_field.o \
		clear_file.o \
		clear_form.o \
		clear_item.o \
		clear_menu.o \
		clear_noecho.o \
		clear_radio.o \
		clear_string.o \
		clear_yesno.o \
		create_form.o \
		create_menu.o \
		disp_button.o \
		disp_string.o \
		display_field.o \
		display_file.o \
		display_form.o \
		display_item.o \
		display_menu.o \
		disp_noecho.o \
		display_radio.o \
		display_yesno.o \
		end_menu.o \
		find_button.o \
		find_field.o \
		find_file.o \
		find_item.o \
		find_noecho.o \
		find_radio.o \
		find_yesno.o \
		free_button.o \
		free_field.o \
		free_file.o \
		free_form.o \
		free_item.o \
		free_map.o \
		free_menu.o \
		free_noecho.o \
		free_radio.o \
		free_string.o \
		free_yesno.o \
		get_field.o \
		get_item.o \
		get_noecho.o \
		get_radio.o \
		get_yesno.o \
		globals.o \
		goto_obj.o \
		help_screen.o \
		ignore_obj.o \
		init_menu.o \
		item_status.o \
		lex_no_ws.o \
		menu_abort.o \
		menu_ack.o \
		menu_alloc.o \
		menu_cols.o \
		menu_flash.o \
		menu_lines.o \
		menu_log.o \
		menu_mesg.o \
		menu_print.o \
		menu_trap.o \
		off_button.o \
		off_field.o \
		off_file.o \
		off_form.o \
		off_item.o \
		off_menu.o \
		off_noecho.o \
		off_radio.o \
		off_shared.o \
		off_string.o \
		off_yesno.o \
		on_button.o \
		on_field.o \
		on_file.o \
		on_form.o \
		on_item.o \
		on_menu.o \
		on_noecho.o \
		on_radio.o \
		on_shared.o \
		on_string.o \
		on_yesno.o \
		paint_menu.o \
		read_char.o \
		redisplay.o \
		repeat_obj.o \
		set_menu_term.o \
		skip_io.o \
		use_form.o \
		use_menu.o
SRCS=		$(OBJS:.o=.c)

#
#	Primary targets:
#
all:		lib

lib:		$(LIB)

$(LIB):		$(OBJS)
		ar u $(LIB) $?
		ranlib $(LIB)

#
#	Administrative targets:
#

clean:
		rm -f $(LIB) $(LINT_LIB) $(OBJS)
		rm -f dummy dummy.o lint.log

depend:		$(HFILES) $(SRCS)
		rm -f makedep
		for i in $(SRCS); do \
			$(CC) -M $(INCPATH) $$i | \
			sed -e 's#/sun[1-9][0-9]*[a-zA-Z]*/#/machine/#g' | \
			sort | uniq | \
			awk '\
			{ \
				if ($$1 != prev) { \
					if (rec != "") \
						print rec; \
					rec = $$0; \
					prev = $$1; \
				} \
				else if (length(rec $$2) > 78) { \
					print rec; \
					rec = $$0; \
				} \
				else \
					rec = rec " " $$2 \
			} \
			END { print rec } '; \
		done | sort > makedep
		echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

#
#	The dummy program exists for verifying the loadability of the menu
#	library.  It also gives a handle on the size of the library when
#	loaded with curses, termlib and the system library.
#
dummy:		dummy.o $(LIB)
		$(CC) -o dummy dummy.o $(LIB) -lcurses -ltermlib

ccbd:		ccbd.log

ccbd.log:	$(HFILES) $(SRCS)
		ccbd $(HFILES) $(SRCS) > ccbd.log

lint:		dummy.c lint.log

lint.log:	$(HFILES) $(SRCS) dummy.c
		lint $(INCPATH) -lcurses $(SRCS) dummy.c > lint.log 2>&1

lintlib:	$(LINT_LIB)

$(LINT_LIB):	$(HFILES) $(SRCS)
		lint $(INCPATH) -Cmenu $(SRCS)

.PRECIOUS:	$(LIB)

# DO NOT DELETE THIS LINE -- make depend uses it
add_button.o: ./menu.h ./menu_impl.h add_button.c
add_confirm.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
add_confirm.o: /usr/include/stdio.h /usr/include/sys/filio.h
add_confirm.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_confirm.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
add_confirm.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_confirm.o: /usr/include/sys/ttydev.h add_confirm.c
add_field.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
add_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
add_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
add_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_field.o: /usr/include/sys/ttydev.h add_field.c
add_file.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
add_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
add_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
add_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_file.o: /usr/include/sys/ttydev.h add_file.c
add_finish.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
add_finish.o: /usr/include/stdio.h /usr/include/sys/filio.h
add_finish.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_finish.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
add_finish.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_finish.o: /usr/include/sys/ttydev.h add_finish.c
add_item.o: ./menu.h ./menu_impl.h add_item.c
add_noecho.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
add_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
add_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
add_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
add_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
add_noecho.o: /usr/include/sys/ttydev.h add_noecho.c
add_radio.o: ./menu.h ./menu_impl.h add_radio.c
add_string.o: ./menu.h ./menu_impl.h add_string.c
add_yesno.o: ./menu.h ./menu_impl.h add_yesno.c
ask_yesno.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
ask_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
ask_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
ask_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
ask_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
ask_yesno.o: /usr/include/sys/ttydev.h /usr/include/varargs.h ask_yesno.c
ckf_abspath.o: ./menu.h ckf_abspath.c
ckf_empty.o: ./menu.h ckf_empty.c
ckf_ether.o: ./menu.h /usr/include/net/if.h /usr/include/net/if_arp.h
ckf_ether.o: /usr/include/netinet/if_ether.h /usr/include/netinet/in.h
ckf_ether.o: /usr/include/sys/label.h /usr/include/sys/socket.h
ckf_ether.o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h ckf_ether.c
ckf_inet_addr.o: ./menu.h ckf_inet_addr.c
ckf_int.o: ./menu.h ckf_int.c
ckf_long.o: ./menu.h ckf_long.c
ckf_netabs.o: ./menu.h /usr/include/string.h ckf_netabs.c
ckf_netpath.o: ./menu.h /usr/include/string.h ckf_netpath.c
ckf_octal.o: ./menu.h ckf_octal.c
ckf_relpath.o: ./menu.h ckf_relpath.c
ckf_uint.o: ./menu.h /usr/include/string.h ckf_uint.c
clear_button.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_button.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_button.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_button.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_button.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_button.o: /usr/include/sys/ttydev.h clear_button.c
clear_field.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_field.o: /usr/include/sys/ttydev.h clear_field.c
clear_file.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_file.o: /usr/include/sys/ttydev.h clear_file.c
clear_form.o: ./menu.h clear_form.c
clear_item.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_item.o: /usr/include/sys/ttydev.h clear_item.c
clear_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_menu.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_menu.o: /usr/include/sys/ttydev.h clear_menu.c
clear_noecho.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_noecho.o: /usr/include/sys/ttydev.h clear_noecho.c
clear_radio.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_radio.o: /usr/include/sys/ttydev.h clear_radio.c
clear_string.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_string.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_string.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_string.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_string.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_string.o: /usr/include/sys/ttydev.h clear_string.c
clear_yesno.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
clear_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
clear_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
clear_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
clear_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
clear_yesno.o: /usr/include/sys/ttydev.h clear_yesno.c
create_form.o: ./menu.h ./menu_impl.h /usr/include/curses.h
create_form.o: /usr/include/sgtty.h /usr/include/stdio.h
create_form.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
create_form.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
create_form.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
create_form.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
create_form.o: create_form.c
create_menu.o: ./menu.h ./menu_impl.h /usr/include/curses.h
create_menu.o: /usr/include/sgtty.h /usr/include/stdio.h
create_menu.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
create_menu.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
create_menu.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
create_menu.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
create_menu.o: create_menu.c
disp_button.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
disp_button.o: /usr/include/stdio.h /usr/include/sys/filio.h
disp_button.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disp_button.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
disp_button.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disp_button.o: /usr/include/sys/ttydev.h disp_button.c
disp_noecho.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
disp_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
disp_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disp_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
disp_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disp_noecho.o: /usr/include/sys/ttydev.h disp_noecho.c
disp_string.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
disp_string.o: /usr/include/stdio.h /usr/include/sys/filio.h
disp_string.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
disp_string.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
disp_string.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
disp_string.o: /usr/include/sys/ttydev.h disp_string.c
display_field.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_field.o: /usr/include/sys/ttydev.h display_field.c
display_file.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_file.o: /usr/include/sys/ttydev.h display_file.c
display_form.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_form.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_form.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_form.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_form.o: /usr/include/sys/ttydev.h display_form.c
display_item.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_item.o: /usr/include/sys/ttydev.h display_item.c
display_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_menu.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_menu.o: /usr/include/sys/ttydev.h display_menu.c
display_radio.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_radio.o: /usr/include/sys/ttydev.h display_radio.c
display_yesno.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
display_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
display_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
display_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
display_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
display_yesno.o: /usr/include/sys/ttydev.h display_yesno.c
end_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
end_menu.o: /usr/include/signal.h /usr/include/stdio.h /usr/include/sys/filio.h
end_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
end_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
end_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
end_menu.o: /usr/include/sys/ttydev.h /usr/include/vm/faultcode.h end_menu.c
find_button.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_button.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_button.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_button.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_button.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_button.o: /usr/include/sys/ttydev.h find_button.c
find_field.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_field.o: /usr/include/sys/ttydev.h find_field.c
find_file.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
find_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_file.o: /usr/include/sys/ttydev.h find_file.c
find_item.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_item.o: /usr/include/sys/ttydev.h find_item.c
find_noecho.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_noecho.o: /usr/include/sys/ttydev.h find_noecho.c
find_radio.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_radio.o: /usr/include/sys/ttydev.h find_radio.c
find_yesno.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
find_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
find_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
find_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
find_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
find_yesno.o: /usr/include/sys/ttydev.h find_yesno.c
free_button.o: ./menu.h free_button.c
free_field.o: ./menu.h free_field.c
free_file.o: ./menu.h ./menu_impl.h free_file.c
free_form.o: ./menu.h free_form.c
free_item.o: ./menu.h free_item.c
free_map.o: ./menu.h free_map.c
free_menu.o: ./menu.h free_menu.c
free_noecho.o: ./menu.h free_noecho.c
free_radio.o: ./menu.h free_radio.c
free_string.o: ./menu.h ./menu_impl.h free_string.c
free_yesno.o: ./menu.h free_yesno.c
get_field.o: ./menu.h ./menu_impl.h /usr/include/ctype.h /usr/include/curses.h
get_field.o: /usr/include/sgtty.h /usr/include/stdio.h /usr/include/sys/filio.h
get_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
get_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_field.o: /usr/include/sys/ttydev.h get_field.c
get_item.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
get_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
get_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
get_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_item.o: /usr/include/sys/ttydev.h get_item.c
get_noecho.o: ./menu.h ./menu_impl.h /usr/include/ctype.h /usr/include/curses.h
get_noecho.o: /usr/include/sgtty.h /usr/include/stdio.h
get_noecho.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
get_noecho.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
get_noecho.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
get_noecho.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h get_noecho.c
get_radio.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
get_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
get_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
get_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_radio.o: /usr/include/sys/ttydev.h get_radio.c
get_yesno.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
get_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
get_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
get_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
get_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
get_yesno.o: /usr/include/sys/ttydev.h get_yesno.c
globals.o: ./menu.h /usr/include/signal.h /usr/include/vm/faultcode.h globals.c
goto_obj.o: ./menu.h ./menu_impl.h goto_obj.c
help_screen.o: ./menu.h ./menu_impl.h /usr/include/curses.h
help_screen.o: /usr/include/sgtty.h /usr/include/stdio.h
help_screen.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
help_screen.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
help_screen.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
help_screen.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
help_screen.o: help_screen.c
ignore_obj.o: ./menu.h ./menu_impl.h ignore_obj.c
init_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
init_menu.o: /usr/include/signal.h /usr/include/stdio.h
init_menu.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
init_menu.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
init_menu.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
init_menu.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
init_menu.o: /usr/include/vm/faultcode.h init_menu.c
item_status.o: ./menu.h ./menu_impl.h /usr/include/curses.h
item_status.o: /usr/include/sgtty.h /usr/include/stdio.h
item_status.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
item_status.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
item_status.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
item_status.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
item_status.o: item_status.c
lex_no_ws.o: ./menu.h /usr/include/ctype.h lex_no_ws.c
menu_abort.o: ./menu.h menu_abort.c
menu_ack.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
menu_ack.o: /usr/include/stdio.h /usr/include/sys/filio.h
menu_ack.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_ack.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
menu_ack.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
menu_ack.o: /usr/include/sys/ttydev.h menu_ack.c
menu_alloc.o: ./menu.h menu_alloc.c
menu_cols.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
menu_cols.o: /usr/include/stdio.h /usr/include/sys/filio.h
menu_cols.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_cols.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
menu_cols.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
menu_cols.o: /usr/include/sys/ttydev.h menu_cols.c
menu_flash.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
menu_flash.o: /usr/include/stdio.h /usr/include/sys/filio.h
menu_flash.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_flash.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
menu_flash.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
menu_flash.o: /usr/include/sys/ttydev.h /usr/include/varargs.h menu_flash.c
menu_lines.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
menu_lines.o: /usr/include/stdio.h /usr/include/sys/filio.h
menu_lines.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_lines.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
menu_lines.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
menu_lines.o: /usr/include/sys/ttydev.h menu_lines.c
menu_log.o: ./menu.h /usr/include/curses.h /usr/include/machine/param.h
menu_log.o: /usr/include/sgtty.h /usr/include/stdio.h /usr/include/sys/filio.h
menu_log.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_log.o: /usr/include/sys/param.h /usr/include/sys/signal.h
menu_log.o: /usr/include/sys/sockio.h /usr/include/sys/sysmacros.h
menu_log.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
menu_log.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
menu_log.o: /usr/include/sys/types.h /usr/include/varargs.h
menu_log.o: /usr/include/vm/faultcode.h menu_log.c
menu_mesg.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
menu_mesg.o: /usr/include/stdio.h /usr/include/sys/filio.h
menu_mesg.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
menu_mesg.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
menu_mesg.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
menu_mesg.o: /usr/include/sys/ttydev.h /usr/include/varargs.h menu_mesg.c
menu_print.o: /usr/include/curses.h /usr/include/sgtty.h /usr/include/stdio.h
menu_print.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
menu_print.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
menu_print.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
menu_print.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h
menu_print.o: /usr/include/varargs.h menu_print.c
menu_trap.o: ./menu.h /usr/include/signal.h /usr/include/vm/faultcode.h
menu_trap.o: menu_trap.c
off_button.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_button.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_button.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_button.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_button.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_button.o: /usr/include/sys/ttydev.h off_button.c
off_field.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_field.o: /usr/include/sys/ttydev.h off_field.c
off_file.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_file.o: /usr/include/sys/ttydev.h off_file.c
off_form.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_form.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_form.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_form.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_form.o: /usr/include/sys/ttydev.h off_form.c
off_item.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_item.o: /usr/include/sys/ttydev.h off_item.c
off_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_menu.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_menu.o: /usr/include/sys/ttydev.h off_menu.c
off_noecho.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_noecho.o: /usr/include/sys/ttydev.h off_noecho.c
off_radio.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_radio.o: /usr/include/sys/ttydev.h off_radio.c
off_shared.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_shared.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_shared.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_shared.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_shared.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_shared.o: /usr/include/sys/ttydev.h off_shared.c
off_string.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_string.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_string.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_string.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_string.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_string.o: /usr/include/sys/ttydev.h off_string.c
off_yesno.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
off_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
off_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
off_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
off_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
off_yesno.o: /usr/include/sys/ttydev.h off_yesno.c
on_button.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_button.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_button.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_button.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_button.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_button.o: /usr/include/sys/ttydev.h on_button.c
on_field.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_field.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_field.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_field.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_field.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_field.o: /usr/include/sys/ttydev.h on_field.c
on_file.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_file.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_file.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_file.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_file.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_file.o: /usr/include/sys/ttydev.h on_file.c
on_form.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_form.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_form.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_form.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_form.o: /usr/include/sys/ttydev.h on_form.c
on_item.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_item.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_item.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_item.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_item.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_item.o: /usr/include/sys/ttydev.h on_item.c
on_menu.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_menu.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_menu.o: /usr/include/sys/ttydev.h on_menu.c
on_noecho.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_noecho.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_noecho.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_noecho.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_noecho.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_noecho.o: /usr/include/sys/ttydev.h on_noecho.c
on_radio.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_radio.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_radio.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_radio.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_radio.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_radio.o: /usr/include/sys/ttydev.h on_radio.c
on_shared.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_shared.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_shared.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_shared.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_shared.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_shared.o: /usr/include/sys/ttydev.h on_shared.c
on_string.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_string.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_string.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_string.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_string.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_string.o: /usr/include/sys/ttydev.h on_string.c
on_yesno.o: ./menu.h /usr/include/curses.h /usr/include/sgtty.h
on_yesno.o: /usr/include/stdio.h /usr/include/sys/filio.h
on_yesno.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
on_yesno.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
on_yesno.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
on_yesno.o: /usr/include/sys/ttydev.h on_yesno.c
paint_menu.o: /usr/include/curses.h /usr/include/sgtty.h /usr/include/stdio.h
paint_menu.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
paint_menu.o: /usr/include/sys/ioctl.h /usr/include/sys/sockio.h
paint_menu.o: /usr/include/sys/ttold.h /usr/include/sys/ttychars.h
paint_menu.o: /usr/include/sys/ttycom.h /usr/include/sys/ttydev.h paint_menu.c
read_char.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
read_char.o: /usr/include/stdio.h /usr/include/sys/filio.h
read_char.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
read_char.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
read_char.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
read_char.o: /usr/include/sys/ttydev.h read_char.c
redisplay.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
redisplay.o: /usr/include/stdio.h /usr/include/sys/filio.h
redisplay.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
redisplay.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
redisplay.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
redisplay.o: /usr/include/sys/ttydev.h redisplay.c
repeat_obj.o: ./menu.h ./menu_impl.h repeat_obj.c
set_menu_term.o: /usr/include/curses.h /usr/include/sgtty.h
set_menu_term.o: /usr/include/stdio.h /usr/include/sys/filio.h
set_menu_term.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
set_menu_term.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
set_menu_term.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
set_menu_term.o: /usr/include/sys/ttydev.h set_menu_term.c
skip_io.o: ./menu.h ./menu_impl.h skip_io.c
use_form.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
use_form.o: /usr/include/stdio.h /usr/include/sys/filio.h
use_form.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
use_form.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
use_form.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
use_form.o: /usr/include/sys/ttydev.h use_form.c
use_menu.o: ./menu.h ./menu_impl.h /usr/include/curses.h /usr/include/sgtty.h
use_menu.o: /usr/include/stdio.h /usr/include/sys/filio.h
use_menu.o: /usr/include/sys/ioccom.h /usr/include/sys/ioctl.h
use_menu.o: /usr/include/sys/sockio.h /usr/include/sys/ttold.h
use_menu.o: /usr/include/sys/ttychars.h /usr/include/sys/ttycom.h
use_menu.o: /usr/include/sys/ttydev.h use_menu.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
